#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "json.h"
#include "debug.h"

#define JSON_MAX_DEPTH 30
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define DEBUG_STATE(json) \
    DEBUG("State = %d, last JSON output: %s", \
          json->state, json->buffer + MAX(0, (long int)json->pos - 20));

typedef enum {
    JSON_STATE_START = 1,
    JSON_STATE_END,
    JSON_STATE_OBJECT,
    JSON_STATE_OBJECT_KEY,
    JSON_STATE_OBJECT_VALUE,
    JSON_STATE_ARRAY,
    JSON_STATE_ARRAY_ITEM,
    JSON_STATE_ERROR,
} json_state;

typedef enum {
    JSON_NESTING_OBJECT,
    JSON_NESTING_ARRAY,
} json_nesting;

struct json_stream {
    uint8_t *buffer;
    size_t size;
    size_t pos;

    json_state state;

    uint8_t nesting_idx;
    json_nesting nesting[JSON_MAX_DEPTH];

    json_flush_callback on_flush;
    void *context;
};


json_stream *json_new(size_t buffer_size, json_flush_callback on_flush, void *context) {
    json_stream *json = malloc(sizeof(json_stream));
    json->size = buffer_size;
    json->pos = 0;
    json->buffer = malloc(json->size);
    json->state = JSON_STATE_START;
    json->nesting_idx = 0;
    json->on_flush = on_flush;
    json->context = context;

    return json;
}

void json_free(json_stream *json) {
    free(json->buffer);
    free(json);
}

void json_flush(json_stream *json) {
    if (!json->pos)
        return;

    if (json->on_flush)
        json->on_flush(json->buffer, json->pos, json->context);
    json->pos = 0;
}

void json_write(json_stream *json, const char *format, ...) {
    va_list arg_ptr;

    va_start(arg_ptr, format);
    int len = vsnprintf((char *)json->buffer + json->pos, json->size - json->pos, format, arg_ptr);
    va_end(arg_ptr);

    if (len + json->pos > json->size - 1) {
        json_flush(json);

        va_start(arg_ptr, format);
        int len = vsnprintf((char *)json->buffer + json->pos, json->size - json->pos, format, arg_ptr);
        va_end(arg_ptr);

        if (len > json->size - 1) {
            ERROR("Write value too large");
            DEBUG("Format = %s", format);
        } else {
            json->pos += len;
        }
    } else {
        json->pos += len;
    }
}

void json_object_start(json_stream *json) {
    if (json->state == JSON_STATE_ERROR)
        return;

    switch (json->state) {
        case JSON_STATE_ARRAY_ITEM:
            json_write(json, ",");
        case JSON_STATE_START:
        case JSON_STATE_OBJECT_KEY:
        case JSON_STATE_ARRAY:
            json_write(json, "{");

            json->state = JSON_STATE_OBJECT;
            json->nesting[json->nesting_idx++] = JSON_NESTING_OBJECT;
            break;
        default:
            ERROR("Unexpected object start");
            DEBUG_STATE(json);
            json->state = JSON_STATE_ERROR;
    }
}

void json_object_end(json_stream *json) {
    if (json->state == JSON_STATE_ERROR)
        return;

    switch (json->state) {
        case JSON_STATE_OBJECT:
        case JSON_STATE_OBJECT_VALUE:
            json_write(json, "}");

            json->nesting_idx--;
            if (!json->nesting_idx) {
                json->state = JSON_STATE_END;
            } else {
                switch (json->nesting[json->nesting_idx-1]) {
                    case JSON_NESTING_OBJECT:
                        json->state = JSON_STATE_OBJECT_VALUE;
                        break;
                    case JSON_NESTING_ARRAY:
                        json->state = JSON_STATE_ARRAY_ITEM;
                        break;
                }
            }
            break;
        default:
            ERROR("Unexpected object end");
            DEBUG_STATE(json);
            json->state = JSON_STATE_ERROR;
    }
}

void json_array_start(json_stream *json) {
    if (json->state == JSON_STATE_ERROR)
        return;

    switch (json->state) {
        case JSON_STATE_ARRAY_ITEM:
            json_write(json, ",");
        case JSON_STATE_START:
        case JSON_STATE_OBJECT_KEY:
        case JSON_STATE_ARRAY:
            json_write(json, "[");

            json->state = JSON_STATE_ARRAY;
            json->nesting[json->nesting_idx++] = JSON_NESTING_ARRAY;
            break;
        default:
            ERROR("Unexpected array start");
            DEBUG_STATE(json);
            json->state = JSON_STATE_ERROR;
    }
}

void json_array_end(json_stream *json) {
    if (json->state == JSON_STATE_ERROR)
        return;

    switch (json->state) {
        case JSON_STATE_ARRAY:
        case JSON_STATE_ARRAY_ITEM:
            json_write(json, "]");

            json->nesting_idx--;
            if (!json->nesting_idx) {
                json->state = JSON_STATE_END;
            } else {
                switch (json->nesting[json->nesting_idx-1]) {
                    case JSON_NESTING_OBJECT:
                        json->state = JSON_STATE_OBJECT_VALUE;
                        break;
                    case JSON_NESTING_ARRAY:
                        json->state = JSON_STATE_ARRAY_ITEM;
                        break;
                }
            }
            break;
        default:
            ERROR("Unexpected array end");
            DEBUG_STATE(json);
            json->state = JSON_STATE_ERROR;
    }
}

void _json_number(json_stream *json, char *value) {
    if (json->state == JSON_STATE_ERROR)
        return;

    void _do_write() {
        json_write(json, value);
    }

    switch (json->state) {
        case JSON_STATE_START:
            _do_write();
            json->state = JSON_STATE_END;
            break;
        case JSON_STATE_ARRAY_ITEM:
            json_write(json, ",");
        case JSON_STATE_ARRAY:
            _do_write();
            json->state = JSON_STATE_ARRAY_ITEM;
            break;
        case JSON_STATE_OBJECT_KEY:
            _do_write();
            json->state = JSON_STATE_OBJECT_VALUE;
            break;
        default:
            ERROR("Unexpected integer");
            DEBUG_STATE(json);
            json->state = JSON_STATE_ERROR;
    }
}


void json_uint8(json_stream *json, uint8_t x) {
    char buffer[4];
    snprintf(buffer, sizeof(buffer), "%u", x);

    _json_number(json, buffer);
}

void json_uint16(json_stream *json, uint16_t x) {
    char buffer[6];
    snprintf(buffer, sizeof(buffer), "%u", x);

    _json_number(json, buffer);
}

void json_uint32(json_stream *json, uint32_t x) {
    char buffer[11];
    snprintf(buffer, sizeof(buffer), "%u", x);

    _json_number(json, buffer);
}

void json_uint64(json_stream *json, uint64_t x) {
    char buffer[21];
    buffer[20] = 0;

    char *b = &buffer[20];
    do {
        *(--b) = '0' + (x % 10);
    } while (x /= 10);

    _json_number(json, b);
}

void json_integer(json_stream *json, int x) {
    char buffer[7];
    snprintf(buffer, sizeof(buffer), "%d", x);

    _json_number(json, buffer);
}

void json_float(json_stream *json, float x) {
    char buffer[32];
    snprintf(buffer, sizeof(buffer), "%1.15g", x);

    _json_number(json, buffer);
}

void json_string(json_stream *json, const char *x) {
    if (json->state == JSON_STATE_ERROR)
        return;

    void _do_write() {
        // TODO: escape string
        json_write(json, "\"%s\"", x);
    }

    switch (json->state) {
        case JSON_STATE_START:
            _do_write();
            json->state = JSON_STATE_END;
            break;
        case JSON_STATE_ARRAY_ITEM:
            json_write(json, ",");
        case JSON_STATE_ARRAY:
            _do_write();
            json->state = JSON_STATE_ARRAY_ITEM;
            break;
        case JSON_STATE_OBJECT_VALUE:
            json_write(json, ",");
        case JSON_STATE_OBJECT:
            _do_write();
            json_write(json, ":");
            json->state = JSON_STATE_OBJECT_KEY;
            break;
        case JSON_STATE_OBJECT_KEY:
            _do_write();
            json->state = JSON_STATE_OBJECT_VALUE;
            break;
        default:
            ERROR("Unexpected string");
            DEBUG_STATE(json);
            json->state = JSON_STATE_ERROR;
    }
}

void json_boolean(json_stream *json, bool x) {
    if (json->state == JSON_STATE_ERROR)
        return;

    void _do_write() {
        json_write(json, (x) ? "true" : "false");
    }

    switch (json->state) {
        case JSON_STATE_START:
            _do_write();
            json->state = JSON_STATE_END;
            break;
        case JSON_STATE_ARRAY_ITEM:
            json_write(json, ",");
        case JSON_STATE_ARRAY:
            _do_write();
            json->state = JSON_STATE_ARRAY_ITEM;
            break;
        case JSON_STATE_OBJECT_KEY:
            _do_write();
            json->state = JSON_STATE_OBJECT_VALUE;
            break;
        default:
            ERROR("Unexpected boolean");
            DEBUG_STATE(json);
            json->state = JSON_STATE_ERROR;
    }
}

void json_null(json_stream *json) {
    if (json->state == JSON_STATE_ERROR)
        return;

    void _do_write() {
        json_write(json, "null");
    }

    switch (json->state) {
        case JSON_STATE_START:
            _do_write();
            json->state = JSON_STATE_END;
            break;
        case JSON_STATE_ARRAY_ITEM:
            json_write(json, ",");
        case JSON_STATE_ARRAY:
            _do_write();
            json->state = JSON_STATE_ARRAY_ITEM;
            break;
        case JSON_STATE_OBJECT_KEY:
            _do_write();
            json->state = JSON_STATE_OBJECT_VALUE;
            break;
        default:
            ERROR("Unexpected null");
            DEBUG_STATE(json);
            json->state = JSON_STATE_ERROR;
    }
}

