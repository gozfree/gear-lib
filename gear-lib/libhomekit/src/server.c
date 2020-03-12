#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

//#include <lwip/sockets.h>

#include <unistd.h>

#if defined(ESP_IDF)
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#elif defined(ESP_OPEN_RTOS)
#include <FreeRTOS.h>
#include <task.h>
#include <queue.h>
#else
//#error "Unknown target platform"
#include <errno.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <arpa/inet.h>
#include <libqueue.h>
#endif

#include <http-parser/http_parser.h>
#include <cJSON.h>
#include <wolfssl/wolfcrypt/hash.h>
#include <wolfssl/wolfcrypt/coding.h>

#include "base64.h"
#include "crypto.h"
#include "pairing.h"
#include "storage.h"
#include "query_params.h"
#include "json.h"
#include "debug.h"
#include "port.h"

#include <homekit/homekit.h>
#include <homekit/characteristics.h>
#include <homekit/tlv.h>


#define PORT 5556

#ifndef HOMEKIT_MAX_CLIENTS
#define HOMEKIT_MAX_CLIENTS 16
#endif

struct _client_context_t;
typedef struct _client_context_t client_context_t;


#define HOMEKIT_NOTIFY_EVENT(server, event) \
  if ((server)->config->on_event) \
      (server)->config->on_event(event);

#define portTICK_PERIOD_MS 1000


typedef enum {
    HOMEKIT_ENDPOINT_UNKNOWN = 0,
    HOMEKIT_ENDPOINT_PAIR_SETUP,
    HOMEKIT_ENDPOINT_PAIR_VERIFY,
    HOMEKIT_ENDPOINT_IDENTIFY,
    HOMEKIT_ENDPOINT_GET_ACCESSORIES,
    HOMEKIT_ENDPOINT_GET_CHARACTERISTICS,
    HOMEKIT_ENDPOINT_UPDATE_CHARACTERISTICS,
    HOMEKIT_ENDPOINT_PAIRINGS,
    HOMEKIT_ENDPOINT_RESET,
    HOMEKIT_ENDPOINT_RESOURCE,
} homekit_endpoint_t;


typedef struct {
    Srp *srp;
    byte *public_key;
    size_t public_key_size;

    client_context_t *client;
} pairing_context_t;


typedef struct {
    byte *secret;
    size_t secret_size;
    byte *session_key;
    size_t session_key_size;
    byte *device_public_key;
    size_t device_public_key_size;
    byte *accessory_public_key;
    size_t accessory_public_key_size;
} pair_verify_context_t;


typedef struct {
    char *accessory_id;
    ed25519_key *accessory_key;

    homekit_server_config_t *config;

    bool paired;
    pairing_context_t *pairing_context;

    int listen_fd;
    fd_set fds;
    int max_fd;
    int nfds;

    client_context_t *clients;
} homekit_server_t;


struct _client_context_t {
    homekit_server_t *server;
    int socket;
    homekit_endpoint_t endpoint;
    query_param_t *endpoint_params;

    byte *data;
    size_t data_size;
    size_t data_available;

    char *body;
    size_t body_length;
    http_parser *parser;

    int pairing_id;
    byte permissions;

    bool disconnect;

    homekit_characteristic_t *current_characteristic;
    homekit_value_t *current_value;

    bool encrypted;
    byte *read_key;
    byte *write_key;
    int count_reads;
    int count_writes;

#if 0
    QueueHandle_t event_queue;
#else
    struct queue *event_queue;
#endif
    pair_verify_context_t *verify_context;

    struct _client_context_t *next;
};


typedef struct {
    homekit_characteristic_t *characteristic;
    homekit_value_t value;
} characteristic_event_t;


void client_context_free(client_context_t *c);
void pairing_context_free(pairing_context_t *context);


homekit_server_t *server_new() {
    homekit_server_t *server = malloc(sizeof(homekit_server_t));
    FD_ZERO(&server->fds);
    server->max_fd = 0;
    server->nfds = 0;
    server->accessory_id = NULL;
    server->accessory_key = NULL;
    server->config = NULL;
    server->paired = false;
    server->pairing_context = NULL;
    server->clients = NULL;
    return server;
}


void server_free(homekit_server_t *server) {
    if (server->accessory_id)
        free(server->accessory_id);

    if (server->accessory_key)
        crypto_ed25519_free(server->accessory_key);

    if (server->pairing_context)
        pairing_context_free(server->pairing_context);

    if (server->clients) {
        client_context_t *client = server->clients;
        while (client) {
            client_context_t *next = client->next;
            client_context_free(client);
            client = next;
        }
    }

    free(server);
}

#ifdef HOMEKIT_DEBUG
#define TLV_DEBUG(values) tlv_debug(values)
#else
#define TLV_DEBUG(values)
#endif

#define CLIENT_DEBUG(client, message, ...) DEBUG("[Client %d] " message, client->socket, ##__VA_ARGS__)
#define CLIENT_INFO(client, message, ...) INFO("[Client %d] " message, client->socket, ##__VA_ARGS__)
#define CLIENT_ERROR(client, message, ...) ERROR("[Client %d] " message, client->socket, ##__VA_ARGS__)

void tlv_debug(const tlv_values_t *values) {
    DEBUG("Got following TLV values:");
    for (tlv_t *t=values->head; t; t=t->next) {
        char *escaped_payload = binary_to_string(t->value, t->size);
        DEBUG("Type %d value (%d bytes): %s", t->type, t->size, escaped_payload);
        free(escaped_payload);
    }
}


typedef enum {
    TLVType_Method = 0,        // (integer) Method to use for pairing. See PairMethod
    TLVType_Identifier = 1,    // (UTF-8) Identifier for authentication
    TLVType_Salt = 2,          // (bytes) 16+ bytes of random salt
    TLVType_PublicKey = 3,     // (bytes) Curve25519, SRP public key or signed Ed25519 key
    TLVType_Proof = 4,         // (bytes) Ed25519 or SRP proof
    TLVType_EncryptedData = 5, // (bytes) Encrypted data with auth tag at end
    TLVType_State = 6,         // (integer) State of the pairing process. 1=M1, 2=M2, etc.
    TLVType_Error = 7,         // (integer) Error code. Must only be present if error code is
                               // not 0. See TLVError
    TLVType_RetryDelay = 8,    // (integer) Seconds to delay until retrying a setup code
    TLVType_Certificate = 9,   // (bytes) X.509 Certificate
    TLVType_Signature = 10,    // (bytes) Ed25519
    TLVType_Permissions = 11,  // (integer) Bit value describing permissions of the controller
                               // being added.
                               // None (0x00): Regular user
                               // Bit 1 (0x01): Admin that is able to add and remove
                               // pairings against the accessory
    TLVType_FragmentData = 13, // (bytes) Non-last fragment of data. If length is 0,
                               // it's an ACK.
    TLVType_FragmentLast = 14, // (bytes) Last fragment of data
    TLVType_Separator = 0xff,
} TLVType;


typedef enum {
  TLVMethod_PairSetup = 1,
  TLVMethod_PairVerify = 2,
  TLVMethod_AddPairing = 3,
  TLVMethod_RemovePairing = 4,
  TLVMethod_ListPairings = 5,
} TLVMethod;


typedef enum {
  TLVError_Unknown = 1,         // Generic error to handle unexpected errors
  TLVError_Authentication = 2,  // Setup code or signature verification failed
  TLVError_Backoff = 3,         // Client must look at the retry delay TLV item and
                                // wait that many seconds before retrying
  TLVError_MaxPeers = 4,        // Server cannot accept any more pairings
  TLVError_MaxTries = 5,        // Server reached its maximum number of
                                // authentication attempts
  TLVError_Unavailable = 6,     // Server pairing method is unavailable
  TLVError_Busy = 7,            // Server is busy and cannot accept a pairing
                                // request at this time
} TLVError;


typedef enum {
    // This specifies a success for the request
    HAPStatus_Success = 0,
    // Request denied due to insufficient privileges
    HAPStatus_InsufficientPrivileges = -70401,
    // Unable to communicate with requested services,
    // e.g. the power to the accessory was turned off
    HAPStatus_NoAccessoryConnection = -70402,
    // Resource is busy, try again
    HAPStatus_ResourceBusy = -70403,
    // Connot write to read only characteristic
    HAPStatus_ReadOnly = -70404,
    // Cannot read from a write only characteristic
    HAPStatus_WriteOnly = -70405,
    // Notification is not supported for characteristic
    HAPStatus_NotificationsUnsupported = -70406,
    // Out of resources to process request
    HAPStatus_OutOfResources = -70407,
    // Operation timed out
    HAPStatus_Timeout = -70408,
    // Resource does not exist
    HAPStatus_NoResource = -70409,
    // Accessory received an invalid value in a write request
    HAPStatus_InvalidValue = -70410,
    // Insufficient Authorization
    HAPStatus_InsufficientAuthorization = -70411,
} HAPStatus;


pair_verify_context_t *pair_verify_context_new() {
    pair_verify_context_t *context = malloc(sizeof(pair_verify_context_t));

    context->secret = NULL;
    context->secret_size = 0;

    context->session_key = NULL;
    context->session_key_size = 0;
    context->device_public_key = NULL;
    context->device_public_key_size = 0;
    context->accessory_public_key = NULL;
    context->accessory_public_key_size = 0;

    return context;
}

void pair_verify_context_free(pair_verify_context_t *context) {
    if (context->secret)
        free(context->secret);

    if (context->session_key)
        free(context->session_key);

    if (context->device_public_key)
        free(context->device_public_key);

    if (context->accessory_public_key)
        free(context->accessory_public_key);

    free(context);
}


client_context_t *client_context_new() {
    client_context_t *c = malloc(sizeof(client_context_t));
    c->server = NULL;
    c->endpoint_params = NULL;

    c->data_size = 1024 + 18;
    c->data_available = 0;
    c->data = malloc(c->data_size);

    c->body = NULL;
    c->body_length = 0;
    c->parser = malloc(sizeof(*c->parser));
    http_parser_init(c->parser, HTTP_REQUEST);
    c->parser->data = c;

    c->pairing_id = -1;
    c->encrypted = false;
    c->read_key = NULL;
    c->write_key = NULL;
    c->count_reads = 0;
    c->count_writes = 0;

    c->disconnect = false;

#if 0
    c->event_queue = xQueueCreate(20, sizeof(characteristic_event_t*));
#endif
    c->verify_context = NULL;

    c->next = NULL;

    return c;
}


void client_context_free(client_context_t *c) {
    if (c->read_key)
        free(c->read_key);

    if (c->write_key)
        free(c->write_key);

    if (c->verify_context)
        pair_verify_context_free(c->verify_context);

#if 0
    if (c->event_queue)
        vQueueDelete(c->event_queue);
#endif

    if (c->endpoint_params)
        query_params_free(c->endpoint_params);

    if (c->parser)
        free(c->parser);

    if (c->data)
        free(c->data);

    if (c->body)
        free(c->body);

    free(c);
}




pairing_context_t *pairing_context_new() {
    pairing_context_t *context = malloc(sizeof(pairing_context_t));
    context->srp = crypto_srp_new();
    context->client = NULL;
    context->public_key = NULL;
    context->public_key_size = 0;
    return context;
}

void pairing_context_free(pairing_context_t *context) {
    if (context->srp) {
        crypto_srp_free(context->srp);
    }
    if (context->public_key) {
        free(context->public_key);
    }
    free(context);
}


void client_notify_characteristic(homekit_characteristic_t *ch, homekit_value_t value, void *client);


typedef enum {
    characteristic_format_type   = (1 << 1),
    characteristic_format_meta   = (1 << 2),
    characteristic_format_perms  = (1 << 3),
    characteristic_format_events = (1 << 4),
} characteristic_format_t;


void write_characteristic_json(json_stream *json, client_context_t *client, const homekit_characteristic_t *ch, characteristic_format_t format, const homekit_value_t *value) {
    json_string(json, "aid"); json_uint32(json, ch->service->accessory->id);
    json_string(json, "iid"); json_uint32(json, ch->id);

    if (format & characteristic_format_type) {
        json_string(json, "type"); json_string(json, ch->type);
    }

    if (format & characteristic_format_perms) {
        json_string(json, "perms"); json_array_start(json);
        if (ch->permissions & homekit_permissions_paired_read)
            json_string(json, "pr");
        if (ch->permissions & homekit_permissions_paired_write)
            json_string(json, "pw");
        if (ch->permissions & homekit_permissions_notify)
            json_string(json, "ev");
        if (ch->permissions & homekit_permissions_additional_authorization)
            json_string(json, "aa");
        if (ch->permissions & homekit_permissions_timed_write)
            json_string(json, "tw");
        if (ch->permissions & homekit_permissions_hidden)
            json_string(json, "hd");
        json_array_end(json);
    }

    if ((format & characteristic_format_events) && (ch->permissions & homekit_permissions_notify)) {
        bool events = homekit_characteristic_has_notify_callback(ch, client_notify_characteristic, client);
        json_string(json, "ev"); json_boolean(json, events);
    }

    if (format & characteristic_format_meta) {
        if (ch->description) {
            json_string(json, "description"); json_string(json, ch->description);
        }

        const char *format_str = NULL;
        switch(ch->format) {
            case homekit_format_bool: format_str = "bool"; break;
            case homekit_format_uint8: format_str = "uint8"; break;
            case homekit_format_uint16: format_str = "uint16"; break;
            case homekit_format_uint32: format_str = "uint32"; break;
            case homekit_format_uint64: format_str = "uint64"; break;
            case homekit_format_int: format_str = "int"; break;
            case homekit_format_float: format_str = "float"; break;
            case homekit_format_string: format_str = "string"; break;
            case homekit_format_tlv: format_str = "tlv8"; break;
            case homekit_format_data: format_str = "data"; break;
        }
        if (format_str) {
            json_string(json, "format"); json_string(json, format_str);
        }

        const char *unit_str = NULL;
        switch(ch->unit) {
            case homekit_unit_none: break;
            case homekit_unit_celsius: unit_str = "celsius"; break;
            case homekit_unit_percentage: unit_str = "percentage"; break;
            case homekit_unit_arcdegrees: unit_str = "arcdegrees"; break;
            case homekit_unit_lux: unit_str = "lux"; break;
            case homekit_unit_seconds: unit_str = "seconds"; break;
        }
        if (unit_str) {
            json_string(json, "unit"); json_string(json, unit_str);
        }

        if (ch->min_value) {
            json_string(json, "minValue"); json_float(json, *ch->min_value);
        }

        if (ch->max_value) {
            json_string(json, "maxValue"); json_float(json, *ch->max_value);
        }

        if (ch->min_step) {
            json_string(json, "minStep"); json_float(json, *ch->min_step);
        }

        if (ch->max_len) {
            json_string(json, "maxLen"); json_uint32(json, *ch->max_len);
        }

        if (ch->max_data_len) {
            json_string(json, "maxDataLen"); json_uint32(json, *ch->max_data_len);
        }

        if (ch->valid_values.count) {
            json_string(json, "valid-values"); json_array_start(json);

            for (int i=0; i<ch->valid_values.count; i++) {
                json_uint16(json, ch->valid_values.values[i]);
            }

            json_array_end(json);
        }

        if (ch->valid_values_ranges.count) {
            json_string(json, "valid-values-range"); json_array_start(json);

            for (int i=0; i<ch->valid_values_ranges.count; i++) {
                json_array_start(json);

                json_integer(json, ch->valid_values_ranges.ranges[i].start);
                json_integer(json, ch->valid_values_ranges.ranges[i].end);

                json_array_end(json);
            }

            json_array_end(json);
        }
    }

    if (ch->permissions & homekit_permissions_paired_read) {
        homekit_value_t v = value ? *value : ch->getter_ex ? ch->getter_ex(ch) : ch->value;

        if (v.is_null) {
            // json_string(json, "value"); json_null(json);
        } else if (v.format != ch->format) {
            ERROR("Characteristic value format is different from characteristic format");
        } else {
            switch(v.format) {
                case homekit_format_bool: {
                    json_string(json, "value"); json_boolean(json, v.bool_value);
                    break;
                }
                case homekit_format_uint8: {
                    json_string(json, "value"); json_uint8(json, v.uint8_value);
                    break;
                }
                case homekit_format_uint16: {
                    json_string(json, "value"); json_uint16(json, v.uint16_value);
                    break;
                }
                case homekit_format_uint32: {
                    json_string(json, "value"); json_uint32(json, v.uint32_value);
                    break;
                }
                case homekit_format_uint64: {
                    json_string(json, "value"); json_uint64(json, v.uint64_value);
                    break;
                }
                case homekit_format_int: {
                    json_string(json, "value"); json_integer(json, v.int_value);
                    break;
                }
                case homekit_format_float: {
                    json_string(json, "value"); json_float(json, v.float_value);
                    break;
                }
                case homekit_format_string: {
                    json_string(json, "value"); json_string(json, v.string_value);
                    break;
                }
                case homekit_format_tlv: {
                    json_string(json, "value");
                    if (!v.tlv_values) {
                        json_string(json, "");
                    } else {
                        size_t tlv_size = 0;
                        tlv_format(v.tlv_values, NULL, &tlv_size);
                        if (tlv_size == 0) {
                            json_string(json, "");
                        } else {
                            byte *tlv_data = malloc(tlv_size);
                            tlv_format(v.tlv_values, tlv_data, &tlv_size);

                            size_t encoded_tlv_size = base64_encoded_size(tlv_data, tlv_size);
                            byte *encoded_tlv_data = malloc(encoded_tlv_size + 1);
                            base64_encode(tlv_data, tlv_size, encoded_tlv_data);
                            encoded_tlv_data[encoded_tlv_size] = 0;

                            json_string(json, (char*) encoded_tlv_data);

                            free(encoded_tlv_data);
                            free(tlv_data);
                        }
                    }
                    break;
                }
                case homekit_format_data:
                    // TODO:
                    break;
            }
        }

        if (!value && ch->getter_ex) {
            // called getter to get value, need to free it
            homekit_value_destruct(&v);
        }
    }
}


int client_send_encrypted(
    client_context_t *context,
    byte *payload, size_t size
) {
    if (!context || !context->encrypted || !context->read_key)
        return -1;

    byte nonce[12];
    memset(nonce, 0, sizeof(nonce));

    byte encrypted[1024 + 18];
    int payload_offset = 0;

    while (payload_offset < size) {
        size_t chunk_size = size - payload_offset;
        if (chunk_size > 1024)
            chunk_size = 1024;

        byte aead[2] = {chunk_size % 256, chunk_size / 256};

        memcpy(encrypted, aead, 2);

        byte i = 4;
        int x = context->count_reads++;
        while (x) {
            nonce[i++] = x % 256;
            x /= 256;
        }

        size_t available = sizeof(encrypted) - 2;
        int r = crypto_chacha20poly1305_encrypt(
            context->read_key, nonce, aead, 2,
            payload+payload_offset, chunk_size,
            encrypted+2, &available
        );
        if (r) {
            ERROR("Failed to chacha encrypt payload (code %d)", r);
            return -1;
        }

        payload_offset += chunk_size;

        write(context->socket, encrypted, available + 2);
    }

    return 0;
}


int client_decrypt(
    client_context_t *context,
    byte *payload, size_t payload_size,
    byte *decrypted, size_t *decrypted_size
) {
    if (!context || !context->encrypted || !context->write_key)
        return -1;

    const size_t block_size = 1024 + 16 + 2;
    size_t required_decrypted_size =
        payload_size / block_size * 1024 + payload_size % block_size - 16 - 2;
    if (*decrypted_size < required_decrypted_size) {
        *decrypted_size = required_decrypted_size;
        return -2;
    }

    *decrypted_size = required_decrypted_size;

    byte nonce[12];
    memset(nonce, 0, sizeof(nonce));

    int payload_offset = 0;
    int decrypted_offset = 0;

    while (payload_offset < payload_size) {
        size_t chunk_size = payload[payload_offset] + payload[payload_offset+1]*256;
        if (chunk_size+18 > payload_size-payload_offset) {
            // Unfinished chunk
            break;
        }

        byte i = 4;
        int x = context->count_writes++;
        while (x) {
            nonce[i++] = x % 256;
            x /= 256;
        }

        size_t decrypted_len = *decrypted_size - decrypted_offset;
        int r = crypto_chacha20poly1305_decrypt(
            context->write_key, nonce, payload+payload_offset, 2,
            payload+payload_offset+2, chunk_size + 16,
            decrypted, &decrypted_len
        );
        if (r) {
            ERROR("Failed to chacha decrypt payload (code %d)", r);
            return -1;
        }

        decrypted_offset += decrypted_len;
        payload_offset += chunk_size + 0x12; // 0x10 is for some auth bytes
    }

    return payload_offset;
}


void homekit_setup_mdns(homekit_server_t *server);


void client_notify_characteristic(homekit_characteristic_t *ch, homekit_value_t value, void *context) {
    client_context_t *client = context;

    if (client->current_characteristic == ch && client->current_value && homekit_value_equal(client->current_value, &value))
        // This value is set by this client, no need to send notification
        return;

    DEBUG("Got characteristic %d.%d change event", ch->service->accessory->id, ch->id);

#if 0
    if (!client->event_queue) {
        ERROR("Client has no event queue. Skipping notification");
        return;
    }
#endif

    characteristic_event_t *event = malloc(sizeof(characteristic_event_t));
    event->characteristic = ch;
    homekit_value_copy(&event->value, &value);

    DEBUG("Sending event to client %d", client->socket);

#if 0
    xQueueSendToBack(client->event_queue, &event, 10);
#endif
}


void client_send(client_context_t *context, byte *data, size_t data_size) {
#if HOMEKIT_DEBUG
    if (data_size < 4096) {
        char *payload = binary_to_string(data, data_size);
        CLIENT_DEBUG(context, "Sending payload: %s", payload);
        free(payload);
    }
#endif

    if (context->encrypted) {
        int r = client_send_encrypted(context, data, data_size);
        if (r) {
            CLIENT_ERROR(context, "Failed to encrypt response (code %d)", r);
            return;
        }
    } else {
        write(context->socket, data, data_size);
    }
}


void client_send_chunk(byte *data, size_t size, void *arg) {
    client_context_t *context = arg;

    size_t payload_size = size + 8;
    byte *payload = malloc(payload_size);

    int offset = snprintf((char *)payload, payload_size, "%x\r\n", size);
    memcpy(payload + offset, data, size);
    payload[offset + size] = '\r';
    payload[offset + size + 1] = '\n';

    client_send(context, payload, offset + size + 2);

    free(payload);
}


void send_204_response(client_context_t *context) {
    static char response[] = "HTTP/1.1 204 No Content\r\n\r\n";
    client_send(context, (byte *)response, sizeof(response)-1);
}

void send_404_response(client_context_t *context) {
    static char response[] = "HTTP/1.1 404 Not Found\r\n\r\n";
    client_send(context, (byte *)response, sizeof(response)-1);
}


typedef struct _client_event {
    const homekit_characteristic_t *characteristic;
    homekit_value_t value;

    struct _client_event *next;
} client_event_t;


void send_client_events(client_context_t *context, client_event_t *events) {
    CLIENT_DEBUG(context, "Sending EVENT");
    DEBUG_HEAP();

    static byte http_headers[] =
        "EVENT/1.0 200 OK\r\n"
        "Content-Type: application/hap+json\r\n"
        "Transfer-Encoding: chunked\r\n\r\n";

    client_send(context, http_headers, sizeof(http_headers)-1);

    // ~35 bytes per event JSON
    // 256 should be enough for ~7 characteristic updates
    json_stream *json = json_new(256, client_send_chunk, context);
    json_object_start(json);
    json_string(json, "characteristics"); json_array_start(json);

    client_event_t *e = events;
    while (e) {
        json_object_start(json);
        write_characteristic_json(json, context, e->characteristic, 0, &e->value);
        json_object_end(json);

        e = e->next;
    }

    json_array_end(json);
    json_object_end(json);

    json_flush(json);
    json_free(json);

    client_send_chunk(NULL, 0, context);
}


void send_tlv_response(client_context_t *context, tlv_values_t *values);

void send_tlv_error_response(client_context_t *context, int state, TLVError error) {
    tlv_values_t *response = tlv_new();
    tlv_add_integer_value(response, TLVType_State, 1, state);
    tlv_add_integer_value(response, TLVType_Error, 1, error);

    send_tlv_response(context, response);
}


void send_tlv_response(client_context_t *context, tlv_values_t *values) {
    CLIENT_DEBUG(context, "Sending TLV response");
    TLV_DEBUG(values);

    size_t payload_size = 0;
    tlv_format(values, NULL, &payload_size);

    byte *payload = malloc(payload_size);
    int r = tlv_format(values, payload, &payload_size);
    if (r) {
        CLIENT_ERROR(context, "Failed to format TLV payload (code %d)", r);
        free(payload);
        return;
    }

    tlv_free(values);

    static char *http_headers =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: application/pairing+tlv8\r\n"
        "Content-Length: %d\r\n"
        "Connection: keep-alive\r\n\r\n";

    int response_size = strlen(http_headers) + payload_size + 32;
    char *response = malloc(response_size);
    int response_len = snprintf(response, response_size, http_headers, payload_size);

    if (response_size - response_len < payload_size + 1) {
        CLIENT_ERROR(context, "Incorrect response buffer size %d: headers took %d, payload size %d", response_size, response_len, payload_size);
        free(response);
        free(payload);
        return;
    }
    memcpy(response+response_len, payload, payload_size);
    response_len += payload_size;

    free(payload);

    client_send(context, (byte *)response, response_len);

    free(response);
}


static byte json_200_response_headers[] =
    "HTTP/1.1 200 OK\r\n"
    "Content-Type: application/hap+json\r\n"
    "Transfer-Encoding: chunked\r\n"
    "Connection: keep-alive\r\n\r\n";


static byte json_207_response_headers[] =
    "HTTP/1.1 207 Multi-Status\r\n"
    "Content-Type: application/hap+json\r\n"
    "Transfer-Encoding: chunked\r\n"
    "Connection: keep-alive\r\n\r\n";


void send_json_response(client_context_t *context, int status_code, byte *payload, size_t payload_size) {
    CLIENT_DEBUG(context, "Sending JSON response");
    DEBUG_HEAP();

    static char *http_headers =
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/hap+json\r\n"
        "Content-Length: %d\r\n"
        "Connection: keep-alive\r\n\r\n";

    CLIENT_DEBUG(context, "Payload: %s", payload);

    const char *status_text = "OK";
    switch (status_code) {
        case 204: status_text = "No Content"; break;
        case 207: status_text = "Multi-Status"; break;
        case 400: status_text = "Bad Request"; break;
        case 404: status_text = "Not Found"; break;
        case 422: status_text = "Unprocessable Entity"; break;
        case 500: status_text = "Internal Server Error"; break;
        case 503: status_text = "Service Unavailable"; break;
    }

    int response_size = strlen(http_headers) + payload_size + strlen(status_text) + 32;
    char *response = malloc(response_size);
    if (!response) {
        CLIENT_ERROR(context, "Failed to allocate response buffer of size %d", response_size);
        return;
    }
    int response_len = snprintf(response, response_size, http_headers, status_code, status_text, payload_size);

    if (response_size - response_len < payload_size + 1) {
        CLIENT_ERROR(context, "Incorrect response buffer size %d: headers took %d, payload size %d", response_size, response_len, payload_size);
        free(response);
        return;
    }
    memcpy(response+response_len, payload, payload_size);
    response_len += payload_size;
    response[response_len] = 0;  // required for debug output

    CLIENT_DEBUG(context, "Sending HTTP response: %s", response);

    client_send(context, (byte *)response, response_len);

    free(response);
}


void send_json_error_response(client_context_t *context, int status_code, HAPStatus status) {
    byte buffer[32];
    int size = snprintf((char *)buffer, sizeof(buffer), "{\"status\": %d}", status);

    send_json_response(context, status_code, buffer, size);
}


static client_context_t *current_client_context = NULL;

homekit_client_id_t homekit_get_client_id() {
    return (homekit_client_id_t)current_client_context;
}

bool homekit_client_is_admin() {
    if (!current_client_context)
        return false;

    return current_client_context->permissions & pairing_permissions_admin;
}

int homekit_client_send(unsigned char *data, size_t size) {
    if (!current_client_context)
        return -1;

    client_send(current_client_context, data, size);

    return 0;
}


void homekit_server_on_identify(client_context_t *context) {
    CLIENT_INFO(context, "Identify");
    DEBUG_HEAP();

    if (context->server->paired) {
        // Already paired
        send_json_error_response(context, 400, HAPStatus_InsufficientPrivileges);
        return;
    }

    send_204_response(context);

    homekit_accessory_t *accessory =
        homekit_accessory_by_id(context->server->config->accessories, 1);
    if (!accessory) {
        return;
    }

    homekit_service_t *accessory_info =
        homekit_service_by_type(accessory, HOMEKIT_SERVICE_ACCESSORY_INFORMATION);
    if (!accessory_info) {
        return;
    }

    homekit_characteristic_t *ch_identify =
        homekit_service_characteristic_by_type(accessory_info, HOMEKIT_CHARACTERISTIC_IDENTIFY);
    if (!ch_identify) {
        return;
    }

    if (ch_identify->setter_ex) {
        ch_identify->setter_ex(ch_identify, HOMEKIT_BOOL(true));
    }
}

void homekit_server_on_pair_setup(client_context_t *context, const byte *data, size_t size) {
    DEBUG("Pair Setup");
    DEBUG_HEAP();

#ifdef HOMEKIT_OVERCLOCK_PAIR_SETUP
    homekit_overclock_start();
#endif

    tlv_values_t *message = tlv_new();
    tlv_parse(data, size, message);

    TLV_DEBUG(message);

    switch(tlv_get_integer_value(message, TLVType_State, -1)) {
        case 1: {
            CLIENT_INFO(context, "Pair Setup Step 1/3");
            DEBUG_HEAP();
            if (context->server->paired) {
                CLIENT_INFO(context, "Refusing to pair: already paired");
                send_tlv_error_response(context, 2, TLVError_Unavailable);
                break;
            }

            if (context->server->pairing_context) {
                if (context->server->pairing_context->client != context) {
                    CLIENT_INFO(context, "Refusing to pair: another pairing in progress");
                    send_tlv_error_response(context, 2, TLVError_Busy);
                    break;
                }
            } else {
                context->server->pairing_context = pairing_context_new();
                context->server->pairing_context->client = context;
            }

            CLIENT_DEBUG(context, "Initializing crypto");
            DEBUG_HEAP();

            char password[11];
            if (context->server->config->password) {
                strncpy(password, context->server->config->password, sizeof(password));
                CLIENT_DEBUG(context, "Using user-specified password: %s", password);
            } else {
                for (int i=0; i<10; i++) {
                    password[i] = homekit_random() % 10 + '0';
                }
                password[3] = password[6] = '-';
                password[10] = 0;
                CLIENT_DEBUG(context, "Using random password: %s", password);
            }

            if (context->server->config->password_callback) {
                context->server->config->password_callback(password);
            }

            crypto_srp_init(
                context->server->pairing_context->srp,
                "Pair-Setup", password
            );

            if (context->server->pairing_context->public_key) {
                free(context->server->pairing_context->public_key);
                context->server->pairing_context->public_key = NULL;
            }
            context->server->pairing_context->public_key_size = 0;
            crypto_srp_get_public_key(context->server->pairing_context->srp, NULL, &context->server->pairing_context->public_key_size);

            context->server->pairing_context->public_key = malloc(context->server->pairing_context->public_key_size);
            int r = crypto_srp_get_public_key(context->server->pairing_context->srp, context->server->pairing_context->public_key, &context->server->pairing_context->public_key_size);
            if (r) {
                CLIENT_ERROR(context, "Failed to dump SPR public key (code %d)", r);

                pairing_context_free(context->server->pairing_context);
                context->server->pairing_context = NULL;

                send_tlv_error_response(context, 2, TLVError_Unknown);
                break;
            }

            size_t salt_size = 0;
            crypto_srp_get_salt(context->server->pairing_context->srp, NULL, &salt_size);

            byte *salt = malloc(salt_size);
            r = crypto_srp_get_salt(context->server->pairing_context->srp, salt, &salt_size);
            if (r) {
                CLIENT_ERROR(context, "Failed to get salt (code %d)", r);

                free(salt);
                pairing_context_free(context->server->pairing_context);
                context->server->pairing_context = NULL;

                send_tlv_error_response(context, 2, TLVError_Unknown);
                break;
            }

            tlv_values_t *response = tlv_new();
            tlv_add_value(response, TLVType_PublicKey, context->server->pairing_context->public_key, context->server->pairing_context->public_key_size);
            tlv_add_value(response, TLVType_Salt, salt, salt_size);
            tlv_add_integer_value(response, TLVType_State, 1, 2);

            free(salt);

            send_tlv_response(context, response);
            break;
        }
        case 3: {
            CLIENT_INFO(context, "Pair Setup Step 2/3");
            DEBUG_HEAP();
            tlv_t *device_public_key = tlv_get_value(message, TLVType_PublicKey);
            if (!device_public_key) {
                CLIENT_ERROR(context, "Invalid payload: no device public key");
                send_tlv_error_response(context, 4, TLVError_Authentication);
                break;
            }

            tlv_t *proof = tlv_get_value(message, TLVType_Proof);
            if (!proof) {
                CLIENT_ERROR(context, "Invalid payload: no device proof");
                send_tlv_error_response(context, 4, TLVError_Authentication);
                break;
            }

            CLIENT_DEBUG(context, "Computing SRP shared secret");
            DEBUG_HEAP();
            int r = crypto_srp_compute_key(
                context->server->pairing_context->srp,
                device_public_key->value, device_public_key->size,
                context->server->pairing_context->public_key,
                context->server->pairing_context->public_key_size
            );
            if (r) {
                CLIENT_ERROR(context, "Failed to compute SRP shared secret (code %d)", r);
                send_tlv_error_response(context, 4, TLVError_Authentication);
                break;
            }

            free(context->server->pairing_context->public_key);
            context->server->pairing_context->public_key = NULL;
            context->server->pairing_context->public_key_size = 0;

            CLIENT_DEBUG(context, "Verifying peer's proof");
            DEBUG_HEAP();
            r = crypto_srp_verify(context->server->pairing_context->srp, proof->value, proof->size);
            if (r) {
                CLIENT_ERROR(context, "Failed to verify peer's proof (code %d)", r);
                send_tlv_error_response(context, 4, TLVError_Authentication);
                break;
            }

            CLIENT_DEBUG(context, "Generating own proof");
            size_t server_proof_size = 0;
            crypto_srp_get_proof(context->server->pairing_context->srp, NULL, &server_proof_size);

            byte *server_proof = malloc(server_proof_size);
            r = crypto_srp_get_proof(context->server->pairing_context->srp, server_proof, &server_proof_size);

            tlv_values_t *response = tlv_new();
            tlv_add_integer_value(response, TLVType_State, 1, 4);
            tlv_add_value(response, TLVType_Proof, server_proof, server_proof_size);

            free(server_proof);

            send_tlv_response(context, response);
            break;
        }
        case 5: {
            CLIENT_INFO(context, "Pair Setup Step 3/3");
            DEBUG_HEAP();

            int r;

            byte shared_secret[HKDF_HASH_SIZE];
            size_t shared_secret_size = sizeof(shared_secret);

            CLIENT_DEBUG(context, "Calculating shared secret");
            const char salt1[] = "Pair-Setup-Encrypt-Salt";
            const char info1[] = "Pair-Setup-Encrypt-Info";
            r = crypto_srp_hkdf(
                context->server->pairing_context->srp,
                (byte *)salt1, sizeof(salt1)-1,
                (byte *)info1, sizeof(info1)-1,
                shared_secret, &shared_secret_size
            );
            if (r) {
                CLIENT_ERROR(context, "Failed to generate shared secret (code %d)", r);
                send_tlv_error_response(context, 6, TLVError_Authentication);
                break;
            }

            tlv_t *tlv_encrypted_data = tlv_get_value(message, TLVType_EncryptedData);
            if (!tlv_encrypted_data) {
                CLIENT_ERROR(context, "Invalid payload: no encrypted data");
                send_tlv_error_response(context, 6, TLVError_Authentication);
                break;
            }

            CLIENT_DEBUG(context, "Decrypting payload");
            size_t decrypted_data_size = 0;
            crypto_chacha20poly1305_decrypt(
                shared_secret, (byte *)"\x0\x0\x0\x0PS-Msg05", NULL, 0,
                tlv_encrypted_data->value, tlv_encrypted_data->size,
                NULL, &decrypted_data_size
            );

            byte *decrypted_data = malloc(decrypted_data_size);
            // TODO: check malloc result
            r = crypto_chacha20poly1305_decrypt(
                shared_secret, (byte *)"\x0\x0\x0\x0PS-Msg05", NULL, 0,
                tlv_encrypted_data->value, tlv_encrypted_data->size,
                decrypted_data, &decrypted_data_size
            );
            if (r) {
                CLIENT_ERROR(context, "Failed to decrypt data (code %d)", r);

                free(decrypted_data);

                send_tlv_error_response(context, 6, TLVError_Authentication);
                break;
            }

            tlv_values_t *decrypted_message = tlv_new();
            r = tlv_parse(decrypted_data, decrypted_data_size, decrypted_message);
            if (r) {
                CLIENT_ERROR(context, "Failed to parse decrypted TLV (code %d)", r);

                tlv_free(decrypted_message);
                free(decrypted_data);

                send_tlv_error_response(context, 6, TLVError_Authentication);
                break;
            }

            free(decrypted_data);

            tlv_t *tlv_device_id = tlv_get_value(decrypted_message, TLVType_Identifier);
            if (!tlv_device_id) {
                CLIENT_ERROR(context, "Invalid encrypted payload: no device identifier");

                tlv_free(decrypted_message);

                send_tlv_error_response(context, 6, TLVError_Authentication);
                break;
            }

            tlv_t *tlv_device_public_key = tlv_get_value(decrypted_message, TLVType_PublicKey);
            if (!tlv_device_public_key) {
                CLIENT_ERROR(context, "Invalid encrypted payload: no device public key");

                tlv_free(decrypted_message);

                send_tlv_error_response(context, 6, TLVError_Authentication);
                break;
            }

            tlv_t *tlv_device_signature = tlv_get_value(decrypted_message, TLVType_Signature);
            if (!tlv_device_signature) {
                CLIENT_ERROR(context, "Invalid encrypted payload: no device signature");

                tlv_free(decrypted_message);

                send_tlv_error_response(context, 6, TLVError_Authentication);
                break;
            }

            CLIENT_DEBUG(context, "Importing device public key");
            ed25519_key *device_key = crypto_ed25519_new();
            r = crypto_ed25519_import_public_key(
                device_key,
                tlv_device_public_key->value, tlv_device_public_key->size
            );
            if (r) {
                CLIENT_ERROR(context, "Failed to import device public Key (code %d)", r);

                crypto_ed25519_free(device_key);
                tlv_free(decrypted_message);

                send_tlv_error_response(context, 6, TLVError_Authentication);
                break;
            }

            byte device_x[HKDF_HASH_SIZE];
            size_t device_x_size = sizeof(device_x);

            CLIENT_DEBUG(context, "Calculating DeviceX");
            const char salt2[] = "Pair-Setup-Controller-Sign-Salt";
            const char info2[] = "Pair-Setup-Controller-Sign-Info";
            r = crypto_srp_hkdf(
                context->server->pairing_context->srp,
                (byte *)salt2, sizeof(salt2)-1,
                (byte *)info2, sizeof(info2)-1,
                device_x, &device_x_size
            );
            if (r) {
                CLIENT_ERROR(context, "Failed to generate DeviceX (code %d)", r);

                crypto_ed25519_free(device_key);
                tlv_free(decrypted_message);

                send_tlv_error_response(context, 6, TLVError_Authentication);
                break;
            }

            size_t device_info_size = device_x_size + tlv_device_id->size + tlv_device_public_key->size;
            byte *device_info = malloc(device_info_size);
            memcpy(device_info,
                   device_x,
                   device_x_size);
            memcpy(device_info + device_x_size,
                   tlv_device_id->value,
                   tlv_device_id->size);
            memcpy(device_info + device_x_size + tlv_device_id->size,
                   tlv_device_public_key->value,
                   tlv_device_public_key->size);

            CLIENT_DEBUG(context, "Verifying device signature");
            r = crypto_ed25519_verify(
                device_key,
                device_info, device_info_size,
                tlv_device_signature->value, tlv_device_signature->size
            );
            if (r) {
                CLIENT_ERROR(context, "Failed to generate DeviceX (code %d)", r);

                free(device_info);
                crypto_ed25519_free(device_key);
                tlv_free(decrypted_message);

                send_tlv_error_response(context, 6, TLVError_Authentication);
                break;
            }

            free(device_info);

            char *device_id = strndup((const char *)tlv_device_id->value, tlv_device_id->size);

            r = homekit_storage_add_pairing(device_id, device_key, pairing_permissions_admin);
            if (r) {
                CLIENT_ERROR(context, "Failed to store pairing (code %d)", r);

                free(device_id);
                crypto_ed25519_free(device_key);
                tlv_free(decrypted_message);
                send_tlv_error_response(context, 6, TLVError_Unknown);
                break;
            }

            INFO("Added pairing with %s", device_id);

            HOMEKIT_NOTIFY_EVENT(context->server, HOMEKIT_EVENT_PAIRING_ADDED);

            free(device_id);

            crypto_ed25519_free(device_key);
            tlv_free(decrypted_message);

            CLIENT_DEBUG(context, "Exporting accessory public key");
            size_t accessory_public_key_size = 0;
            crypto_ed25519_export_public_key(context->server->accessory_key, NULL, &accessory_public_key_size);

            byte *accessory_public_key = malloc(accessory_public_key_size);
            r = crypto_ed25519_export_public_key(context->server->accessory_key, accessory_public_key, &accessory_public_key_size);
            if (r) {
                CLIENT_ERROR(context, "Failed to export accessory public key (code %d)", r);

                free(accessory_public_key);

                send_tlv_error_response(context, 6, TLVError_Authentication);
                break;
            }

            size_t accessory_id_size = strlen(context->server->accessory_id);
            size_t accessory_info_size = HKDF_HASH_SIZE + accessory_id_size + accessory_public_key_size;
            byte *accessory_info = malloc(accessory_info_size);

            CLIENT_DEBUG(context, "Calculating AccessoryX");
            size_t accessory_x_size = accessory_info_size;
            const char salt3[] = "Pair-Setup-Accessory-Sign-Salt";
            const char info3[] = "Pair-Setup-Accessory-Sign-Info";
            r = crypto_srp_hkdf(
                context->server->pairing_context->srp,
                (byte *)salt3, sizeof(salt3)-1,
                (byte *)info3, sizeof(info3)-1,
                accessory_info, &accessory_x_size
            );
            if (r) {
                CLIENT_ERROR(context, "Failed to generate AccessoryX (code %d)", r);

                free(accessory_info);
                free(accessory_public_key);

                send_tlv_error_response(context, 6, TLVError_Unknown);
                break;
            }

            memcpy(accessory_info + accessory_x_size,
                   context->server->accessory_id, accessory_id_size);
            memcpy(accessory_info + accessory_x_size + accessory_id_size,
                   accessory_public_key, accessory_public_key_size);

            CLIENT_DEBUG(context, "Generating accessory signature");
            DEBUG_HEAP();
            size_t accessory_signature_size = 0;
            crypto_ed25519_sign(
                context->server->accessory_key,
                accessory_info, accessory_info_size,
                NULL, &accessory_signature_size
            );

            byte *accessory_signature = malloc(accessory_signature_size);
            r = crypto_ed25519_sign(
                context->server->accessory_key,
                accessory_info, accessory_info_size,
                accessory_signature, &accessory_signature_size
            );
            if (r) {
                CLIENT_ERROR(context, "Failed to generate accessory signature (code %d)", r);

                free(accessory_signature);
                free(accessory_public_key);
                free(accessory_info);

                send_tlv_error_response(context, 6, TLVError_Unknown);
                break;
            }

            free(accessory_info);

            tlv_values_t *response_message = tlv_new();
            tlv_add_value(response_message, TLVType_Identifier,
                          (byte *)context->server->accessory_id, accessory_id_size);
            tlv_add_value(response_message, TLVType_PublicKey,
                          accessory_public_key, accessory_public_key_size);
            tlv_add_value(response_message, TLVType_Signature,
                          accessory_signature, accessory_signature_size);

            free(accessory_public_key);
            free(accessory_signature);

            size_t response_data_size = 0;
            TLV_DEBUG(response_message);

            tlv_format(response_message, NULL, &response_data_size);

            byte *response_data = malloc(response_data_size);
            r = tlv_format(response_message, response_data, &response_data_size);
            if (r) {
                CLIENT_ERROR(context, "Failed to format TLV response (code %d)", r);

                free(response_data);
                tlv_free(response_message);

                send_tlv_error_response(context, 6, TLVError_Unknown);
                break;
            }

            tlv_free(response_message);

            CLIENT_DEBUG(context, "Encrypting response");
            size_t encrypted_response_data_size = 0;
            crypto_chacha20poly1305_encrypt(
                shared_secret, (byte *)"\x0\x0\x0\x0PS-Msg06", NULL, 0,
                response_data, response_data_size,
                NULL, &encrypted_response_data_size
            );

            byte *encrypted_response_data = malloc(encrypted_response_data_size);
            r = crypto_chacha20poly1305_encrypt(
                shared_secret, (byte *)"\x0\x0\x0\x0PS-Msg06", NULL, 0,
                response_data, response_data_size,
                encrypted_response_data, &encrypted_response_data_size
            );

            free(response_data);

            if (r) {
                CLIENT_ERROR(context, "Failed to encrypt response data (code %d)", r);

                free(encrypted_response_data);

                send_tlv_error_response(context, 6, TLVError_Unknown);
                break;
            }

            tlv_values_t *response = tlv_new();
            tlv_add_integer_value(response, TLVType_State, 1, 6);
            tlv_add_value(response, TLVType_EncryptedData,
                          encrypted_response_data, encrypted_response_data_size);

            free(encrypted_response_data);

            send_tlv_response(context, response);

            pairing_context_free(context->server->pairing_context);
            context->server->pairing_context = NULL;

            context->server->paired = 1;
            homekit_setup_mdns(context->server);

            CLIENT_INFO(context, "Successfully paired");

            break;
        }
        default: {
            CLIENT_ERROR(context, "Unknown state: %d",
                  tlv_get_integer_value(message, TLVType_State, -1));
        }
    }

    tlv_free(message);

#ifdef HOMEKIT_OVERCLOCK_PAIR_SETUP
    homekit_overclock_end();
#endif
}

void homekit_server_on_pair_verify(client_context_t *context, const byte *data, size_t size) {
    DEBUG("HomeKit Pair Verify");
    DEBUG_HEAP();

#ifdef HOMEKIT_OVERCLOCK_PAIR_VERIFY
    homekit_overclock_start();
#endif

    tlv_values_t *message = tlv_new();
    tlv_parse(data, size, message);

    TLV_DEBUG(message);

    int r;

    switch(tlv_get_integer_value(message, TLVType_State, -1)) {
        case 1: {
            CLIENT_INFO(context, "Pair Verify Step 1/2");

            CLIENT_DEBUG(context, "Importing device Curve25519 public key");
            tlv_t *tlv_device_public_key = tlv_get_value(message, TLVType_PublicKey);
            if (!tlv_device_public_key) {
                CLIENT_ERROR(context, "Device Curve25519 public key not found");
                send_tlv_error_response(context, 2, TLVError_Unknown);
                break;
            }
            curve25519_key *device_key = crypto_curve25519_new();
            r = crypto_curve25519_import_public(
                device_key,
                tlv_device_public_key->value, tlv_device_public_key->size
            );
            if (r) {
                CLIENT_ERROR(context, "Failed to import device Curve25519 public key (code %d)", r);
                crypto_curve25519_free(device_key);
                send_tlv_error_response(context, 2, TLVError_Unknown);
                break;
            }

            CLIENT_DEBUG(context, "Generating accessory Curve25519 key");
            curve25519_key *my_key = crypto_curve25519_generate();
            if (!my_key) {
                CLIENT_ERROR(context, "Failed to generate accessory Curve25519 key");
                crypto_curve25519_free(device_key);
                send_tlv_error_response(context, 2, TLVError_Unknown);
                break;
            }

            CLIENT_DEBUG(context, "Exporting accessory Curve25519 public key");
            size_t my_key_public_size = 0;
            crypto_curve25519_export_public(my_key, NULL, &my_key_public_size);

            byte *my_key_public = malloc(my_key_public_size);
            r = crypto_curve25519_export_public(my_key, my_key_public, &my_key_public_size);
            if (r) {
                CLIENT_ERROR(context, "Failed to export accessory Curve25519 public key (code %d)", r);
                free(my_key_public);
                crypto_curve25519_free(my_key);
                crypto_curve25519_free(device_key);
                send_tlv_error_response(context, 2, TLVError_Unknown);
                break;
            }

            CLIENT_DEBUG(context, "Generating Curve25519 shared secret");
            size_t shared_secret_size = 0;
            crypto_curve25519_shared_secret(my_key, device_key, NULL, &shared_secret_size);

            byte *shared_secret = malloc(shared_secret_size);
            r = crypto_curve25519_shared_secret(my_key, device_key, shared_secret, &shared_secret_size);
            crypto_curve25519_free(my_key);
            crypto_curve25519_free(device_key);

            if (r) {
                CLIENT_ERROR(context, "Failed to generate Curve25519 shared secret (code %d)", r);
                free(shared_secret);
                free(my_key_public);
                send_tlv_error_response(context, 2, TLVError_Unknown);
                break;
            }

            CLIENT_DEBUG(context, "Generating signature");
            size_t accessory_id_size = strlen(context->server->accessory_id);
            size_t accessory_info_size = my_key_public_size + accessory_id_size + tlv_device_public_key->size;

            byte *accessory_info = malloc(accessory_info_size);
            memcpy(accessory_info,
                   my_key_public, my_key_public_size);
            memcpy(accessory_info + my_key_public_size,
                   context->server->accessory_id, accessory_id_size);
            memcpy(accessory_info + my_key_public_size + accessory_id_size,
                   tlv_device_public_key->value, tlv_device_public_key->size);

            size_t accessory_signature_size = 0;
            crypto_ed25519_sign(
                context->server->accessory_key,
                accessory_info, accessory_info_size,
                NULL, &accessory_signature_size
            );

            byte *accessory_signature = malloc(accessory_signature_size);
            r = crypto_ed25519_sign(
                context->server->accessory_key,
                accessory_info, accessory_info_size,
                accessory_signature, &accessory_signature_size
            );
            free(accessory_info);
            if (r) {
                CLIENT_ERROR(context, "Failed to generate signature (code %d)", r);
                free(accessory_signature);
                free(shared_secret);
                free(my_key_public);
                send_tlv_error_response(context, 2, TLVError_Unknown);
                break;
            }

            tlv_values_t *sub_response = tlv_new();
            tlv_add_value(sub_response, TLVType_Identifier,
                          (const byte *)context->server->accessory_id, accessory_id_size);
            tlv_add_value(sub_response, TLVType_Signature,
                          accessory_signature, accessory_signature_size);

            free(accessory_signature);

            size_t sub_response_data_size = 0;
            tlv_format(sub_response, NULL, &sub_response_data_size);

            byte *sub_response_data = malloc(sub_response_data_size);
            r = tlv_format(sub_response, sub_response_data, &sub_response_data_size);
            tlv_free(sub_response);

            if (r) {
                CLIENT_ERROR(context, "Failed to format sub-TLV message (code %d)", r);
                free(sub_response_data);
                free(shared_secret);
                free(my_key_public);
                send_tlv_error_response(context, 2, TLVError_Unknown);
                break;
            }

            CLIENT_DEBUG(context, "Generating proof");
            size_t session_key_size = 0;
            const byte salt[] = "Pair-Verify-Encrypt-Salt";
            const byte info[] = "Pair-Verify-Encrypt-Info";
            crypto_hkdf(
                shared_secret, shared_secret_size,
                salt, sizeof(salt)-1,
                info, sizeof(info)-1,
                NULL, &session_key_size
            );

            byte *session_key = malloc(session_key_size);
            r = crypto_hkdf(
                shared_secret, shared_secret_size,
                salt, sizeof(salt)-1,
                info, sizeof(info)-1,
                session_key, &session_key_size
            );
            if (r) {
                CLIENT_ERROR(context, "Failed to derive session key (code %d)", r);
                free(session_key);
                free(sub_response_data);
                free(shared_secret);
                free(my_key_public);
                send_tlv_error_response(context, 2, TLVError_Unknown);
                break;
            }

            CLIENT_DEBUG(context, "Encrypting response");
            size_t encrypted_response_data_size = 0;
            crypto_chacha20poly1305_encrypt(
                session_key, (byte *)"\x0\x0\x0\x0PV-Msg02", NULL, 0,
                sub_response_data, sub_response_data_size,
                NULL, &encrypted_response_data_size
            );

            byte *encrypted_response_data = malloc(encrypted_response_data_size);
            r = crypto_chacha20poly1305_encrypt(
                session_key, (byte *)"\x0\x0\x0\x0PV-Msg02", NULL, 0,
                sub_response_data, sub_response_data_size,
                encrypted_response_data, &encrypted_response_data_size
            );
            free(sub_response_data);

            if (r) {
                CLIENT_ERROR(context, "Failed to encrypt sub response data (code %d)", r);
                free(encrypted_response_data);
                free(session_key);
                free(shared_secret);
                free(my_key_public);
                send_tlv_error_response(context, 2, TLVError_Unknown);
                break;
            }

            tlv_values_t *response = tlv_new();
            tlv_add_integer_value(response, TLVType_State, 1, 2);
            tlv_add_value(response, TLVType_PublicKey,
                          my_key_public, my_key_public_size);
            tlv_add_value(response, TLVType_EncryptedData,
                          encrypted_response_data, encrypted_response_data_size);

            free(encrypted_response_data);

            send_tlv_response(context, response);

            if (context->verify_context)
                pair_verify_context_free(context->verify_context);

            context->verify_context = pair_verify_context_new();
            context->verify_context->secret = shared_secret;
            context->verify_context->secret_size = shared_secret_size;

            context->verify_context->session_key = session_key;
            context->verify_context->session_key_size = session_key_size;

            context->verify_context->accessory_public_key = my_key_public;
            context->verify_context->accessory_public_key_size = my_key_public_size;

            context->verify_context->device_public_key = malloc(tlv_device_public_key->size);
            memcpy(context->verify_context->device_public_key,
                   tlv_device_public_key->value, tlv_device_public_key->size);
            context->verify_context->device_public_key_size = tlv_device_public_key->size;

            break;
        }
        case 3: {
            CLIENT_INFO(context, "Pair Verify Step 2/2");

            if (!context->verify_context) {
                CLIENT_ERROR(context, "Failed to verify: no state 1 data");
                send_tlv_error_response(context, 4, TLVError_Authentication);
                break;
            }

            tlv_t *tlv_encrypted_data = tlv_get_value(message, TLVType_EncryptedData);
            if (!tlv_encrypted_data) {
                CLIENT_ERROR(context, "Failed to verify: no encrypted data");

                pair_verify_context_free(context->verify_context);
                context->verify_context = NULL;

                send_tlv_error_response(context, 4, TLVError_Authentication);
                break;
            }

            CLIENT_DEBUG(context, "Decrypting payload");
            size_t decrypted_data_size = 0;
            crypto_chacha20poly1305_decrypt(
                context->verify_context->session_key, (byte *)"\x0\x0\x0\x0PV-Msg03", NULL, 0,
                tlv_encrypted_data->value, tlv_encrypted_data->size,
                NULL, &decrypted_data_size
            );

            byte *decrypted_data = malloc(decrypted_data_size);
            r = crypto_chacha20poly1305_decrypt(
                context->verify_context->session_key, (byte *)"\x0\x0\x0\x0PV-Msg03", NULL, 0,
                tlv_encrypted_data->value, tlv_encrypted_data->size,
                decrypted_data, &decrypted_data_size
            );
            if (r) {
                CLIENT_ERROR(context, "Failed to decrypt data (code %d)", r);

                free(decrypted_data);
                pair_verify_context_free(context->verify_context);
                context->verify_context = NULL;

                send_tlv_error_response(context, 4, TLVError_Authentication);
                break;
            }

            tlv_values_t *decrypted_message = tlv_new();
            r = tlv_parse(decrypted_data, decrypted_data_size, decrypted_message);
            free(decrypted_data);

            if (r) {
                CLIENT_ERROR(context, "Failed to parse decrypted TLV (code %d)", r);

                tlv_free(decrypted_message);
                pair_verify_context_free(context->verify_context);
                context->verify_context = NULL;

                send_tlv_error_response(context, 4, TLVError_Authentication);
                break;
            }

            tlv_t *tlv_device_id = tlv_get_value(decrypted_message, TLVType_Identifier);
            if (!tlv_device_id) {
                CLIENT_ERROR(context, "Invalid encrypted payload: no device identifier");

                tlv_free(decrypted_message);
                pair_verify_context_free(context->verify_context);
                context->verify_context = NULL;

                send_tlv_error_response(context, 4, TLVError_Authentication);
                break;
            }

            tlv_t *tlv_device_signature = tlv_get_value(decrypted_message, TLVType_Signature);
            if (!tlv_device_signature) {
                CLIENT_ERROR(context, "Invalid encrypted payload: no device identifier");

                tlv_free(decrypted_message);
                pair_verify_context_free(context->verify_context);
                context->verify_context = NULL;

                send_tlv_error_response(context, 4, TLVError_Authentication);
                break;
            }

            char *device_id = strndup((const char *)tlv_device_id->value, tlv_device_id->size);
            CLIENT_DEBUG(context, "Searching pairing with %s", device_id);
            pairing_t *pairing = homekit_storage_find_pairing(device_id);
            if (!pairing) {
                CLIENT_ERROR(context, "No pairing for %s found", device_id);

                free(device_id);
                tlv_free(decrypted_message);
                pair_verify_context_free(context->verify_context);
                context->verify_context = NULL;

                send_tlv_error_response(context, 4, TLVError_Authentication);
                break;
            }

            CLIENT_INFO(context, "Found pairing with %s", device_id);
            free(device_id);

            byte permissions = pairing->permissions;
            int pairing_id = pairing->id;

            size_t device_info_size =
                context->verify_context->device_public_key_size +
                context->verify_context->accessory_public_key_size +
                tlv_device_id->size;

            byte *device_info = malloc(device_info_size);
            memcpy(device_info,
                   context->verify_context->device_public_key, context->verify_context->device_public_key_size);
            memcpy(device_info + context->verify_context->device_public_key_size,
                   tlv_device_id->value, tlv_device_id->size);
            memcpy(device_info + context->verify_context->device_public_key_size + tlv_device_id->size,
                   context->verify_context->accessory_public_key, context->verify_context->accessory_public_key_size);

            CLIENT_DEBUG(context, "Verifying device signature");
            r = crypto_ed25519_verify(
                pairing->device_key,
                device_info, device_info_size,
                tlv_device_signature->value, tlv_device_signature->size
            );
            free(device_info);
            pairing_free(pairing);
            tlv_free(decrypted_message);

            if (r) {
                CLIENT_ERROR(context, "Failed to verify device signature (code %d)", r);

                pair_verify_context_free(context->verify_context);
                context->verify_context = NULL;

                send_tlv_error_response(context, 4, TLVError_Authentication);
                break;
            }

            const byte salt[] = "Control-Salt";

            size_t read_key_size = 32;
            context->read_key = malloc(read_key_size);
            const byte read_info[] = "Control-Read-Encryption-Key";
            r = crypto_hkdf(
                context->verify_context->secret, context->verify_context->secret_size,
                salt, sizeof(salt)-1,
                read_info, sizeof(read_info)-1,
                context->read_key, &read_key_size
            );

            if (r) {
                CLIENT_ERROR(context, "Failed to derive read encryption key (code %d)", r);

                free(context->read_key);
                context->read_key = NULL;
                pair_verify_context_free(context->verify_context);
                context->verify_context = NULL;

                send_tlv_error_response(context, 4, TLVError_Unknown);
                break;
            }

            size_t write_key_size = 32;
            context->write_key = malloc(write_key_size);
            const byte write_info[] = "Control-Write-Encryption-Key";
            r = crypto_hkdf(
                context->verify_context->secret, context->verify_context->secret_size,
                salt, sizeof(salt)-1,
                write_info, sizeof(write_info)-1,
                context->write_key, &write_key_size
            );

            pair_verify_context_free(context->verify_context);
            context->verify_context = NULL;

            if (r) {
                CLIENT_ERROR(context, "Failed to derive write encryption key (code %d)", r);

                free(context->write_key);
                context->write_key = NULL;
                free(context->read_key);
                context->read_key = NULL;

                send_tlv_error_response(context, 4, TLVError_Unknown);
                break;
            }

            tlv_values_t *response = tlv_new();
            tlv_add_integer_value(response, TLVType_State, 1, 4);

            send_tlv_response(context, response);

            context->pairing_id = pairing_id;
            context->permissions = permissions;
            context->encrypted = true;

            HOMEKIT_NOTIFY_EVENT(context->server, HOMEKIT_EVENT_CLIENT_VERIFIED);

            CLIENT_INFO(context, "Verification successful, secure session established");

            break;
        }
        default: {
            CLIENT_ERROR(context, "Unknown state: %d",
                  tlv_get_integer_value(message, TLVType_State, -1));
        }
    }

    tlv_free(message);

#ifdef HOMEKIT_OVERCLOCK_PAIR_VERIFY
    homekit_overclock_end();
#endif
}


void homekit_server_on_get_accessories(client_context_t *context) {
    CLIENT_INFO(context, "Get Accessories");
    DEBUG_HEAP();

    client_send(context, json_200_response_headers, sizeof(json_200_response_headers)-1);

    json_stream *json = json_new(1024, client_send_chunk, context);
    json_object_start(json);
    json_string(json, "accessories"); json_array_start(json);

    for (homekit_accessory_t **accessory_it = context->server->config->accessories; *accessory_it; accessory_it++) {
        homekit_accessory_t *accessory = *accessory_it;

        json_object_start(json);

        json_string(json, "aid"); json_uint32(json, accessory->id);
        json_string(json, "services"); json_array_start(json);

        for (homekit_service_t **service_it = accessory->services; *service_it; service_it++) {
            homekit_service_t *service = *service_it;

            json_object_start(json);

            json_string(json, "iid"); json_uint32(json, service->id);
            json_string(json, "type"); json_string(json, service->type);
            json_string(json, "hidden"); json_boolean(json, service->hidden);
            json_string(json, "primary"); json_boolean(json, service->primary);
            if (service->linked) {
                json_string(json, "linked"); json_array_start(json);
                for (homekit_service_t **linked=service->linked; *linked; linked++) {
                    json_uint32(json, (*linked)->id);
                }
                json_array_end(json);
            }

            json_string(json, "characteristics"); json_array_start(json);

            for (homekit_characteristic_t **ch_it = service->characteristics; *ch_it; ch_it++) {
                homekit_characteristic_t *ch = *ch_it;

                json_object_start(json);
                write_characteristic_json(
                    json, context, ch,
                      characteristic_format_type
                    | characteristic_format_meta
                    | characteristic_format_perms
                    | characteristic_format_events,
                    NULL
                );
                json_object_end(json);
            }

            json_array_end(json);
            json_object_end(json); // service
        }

        json_array_end(json);
        json_object_end(json); // accessory
    }

    json_array_end(json);
    json_object_end(json); // response

    json_flush(json);
    json_free(json);

    client_send_chunk(NULL, 0, context);
}

void homekit_server_on_get_characteristics(client_context_t *context) {
    CLIENT_INFO(context, "Get Characteristics");
    DEBUG_HEAP();

    query_param_t *qp = context->endpoint_params;
    while (qp) {
        CLIENT_DEBUG(context, "Query paramter %s = %s", qp->name, qp->value);
        qp = qp->next;
    }

    query_param_t *id_param = query_params_find(context->endpoint_params, "id");
    if (!id_param) {
        CLIENT_ERROR(context, "Invalid get characteristics request: missing ID parameter");
        send_json_error_response(context, 400, HAPStatus_InvalidValue);
        return;
    }

    bool bool_endpoint_param(const char *name) {
        query_param_t *param = query_params_find(context->endpoint_params, name);
        return param && param->value && !strcmp(param->value, "1");
    }

    characteristic_format_t format = 0;
    if (bool_endpoint_param("meta"))
        format |= characteristic_format_meta;

    if (bool_endpoint_param("perms"))
        format |= characteristic_format_perms;

    if (bool_endpoint_param("type"))
        format |= characteristic_format_type;

    if (bool_endpoint_param("ev"))
        format |= characteristic_format_events;

    bool success = true;

    char *id = strdup(id_param->value);
    char *ch_id;
    char *_id = id;
    while ((ch_id = strsep(&_id, ","))) {
        char *dot = strstr(ch_id, ".");
        if (!dot) {
            send_json_error_response(context, 400, HAPStatus_InvalidValue);
            free(id);
            return;
        }

        *dot = 0;
        int aid = atoi(ch_id);
        int iid = atoi(dot+1);

        CLIENT_DEBUG(context, "Requested characteristic info for %d.%d", aid, iid);
        homekit_characteristic_t *ch = homekit_characteristic_by_aid_and_iid(context->server->config->accessories, aid, iid);
        if (!ch) {
            success = false;
            continue;
        }

        if (!(ch->permissions & homekit_permissions_paired_read)) {
            success = false;
            continue;
        }
    }

    free(id);
    id = strdup(id_param->value);

    if (success) {
        client_send(context, json_200_response_headers, sizeof(json_200_response_headers)-1);
    } else {
        client_send(context, json_207_response_headers, sizeof(json_207_response_headers)-1);
    }

    json_stream *json = json_new(256, client_send_chunk, context);
    json_object_start(json);
    json_string(json, "characteristics"); json_array_start(json);

    void write_characteristic_error(json_stream *json, int aid, int iid, int status) {
        json_object_start(json);
        json_string(json, "aid"); json_uint32(json, aid);
        json_string(json, "iid"); json_uint32(json, iid);
        json_string(json, "status"); json_uint8(json, status);
        json_object_end(json);
    }

    _id = id;
    while ((ch_id = strsep(&_id, ","))) {
        char *dot = strstr(ch_id, ".");
        *dot = 0;
        int aid = atoi(ch_id);
        int iid = atoi(dot+1);

        CLIENT_DEBUG(context, "Requested characteristic info for %d.%d", aid, iid);
        homekit_characteristic_t *ch = homekit_characteristic_by_aid_and_iid(context->server->config->accessories, aid, iid);
        if (!ch) {
            write_characteristic_error(json, aid, iid, HAPStatus_NoResource);
            continue;
        }

        if (!(ch->permissions & homekit_permissions_paired_read)) {
            write_characteristic_error(json, aid, iid, HAPStatus_WriteOnly);
            continue;
        }

        json_object_start(json);
        write_characteristic_json(json, context, ch, format, NULL);
        if (!success) {
            json_string(json, "status"); json_uint8(json, HAPStatus_Success);
        }
        json_object_end(json);
    }

    json_array_end(json);
    json_object_end(json); // response

    json_flush(json);
    json_free(json);

    client_send_chunk(NULL, 0, context);

    free(id);
}

void homekit_server_on_update_characteristics(client_context_t *context, const byte *data, size_t size) {
    CLIENT_INFO(context, "Update Characteristics");
    DEBUG_HEAP();

    char *data1 = strndup((char *)data, size);
    cJSON *json = cJSON_Parse(data1);
    free(data1);

    if (!json) {
        CLIENT_ERROR(context, "Failed to parse request JSON");
        send_json_error_response(context, 400, HAPStatus_InvalidValue);
        return;
    }

    cJSON *characteristics = cJSON_GetObjectItem(json, "characteristics");
    if (!characteristics) {
        CLIENT_ERROR(context, "Failed to parse request: no \"characteristics\" field");
        cJSON_Delete(json);
        send_json_error_response(context, 400, HAPStatus_InvalidValue);
        return;
    }
    if (characteristics->type != cJSON_Array) {
        CLIENT_ERROR(context, "Failed to parse request: \"characteristics\" field is not an list");
        cJSON_Delete(json);
        send_json_error_response(context, 400, HAPStatus_InvalidValue);
        return;
    }

    HAPStatus process_characteristics_update(const cJSON *j_ch) {
        cJSON *j_aid = cJSON_GetObjectItem(j_ch, "aid");
        if (!j_aid) {
            CLIENT_ERROR(context, "Failed to process request: no \"aid\" field");
            return HAPStatus_NoResource;
        }
        if (j_aid->type != cJSON_Number) {
            CLIENT_ERROR(context, "Failed to process request: \"aid\" field is not a number");
            return HAPStatus_NoResource;
        }

        cJSON *j_iid = cJSON_GetObjectItem(j_ch, "iid");
        if (!j_iid) {
            CLIENT_ERROR(context, "Failed to process request: no \"iid\" field");
            return HAPStatus_NoResource;
        }
        if (j_iid->type != cJSON_Number) {
            CLIENT_ERROR(context, "Failed to process request: \"iid\" field is not a number");
            return HAPStatus_NoResource;
        }

        int aid = j_aid->valueint;
        int iid = j_iid->valueint;

        homekit_characteristic_t *ch = homekit_characteristic_by_aid_and_iid(
            context->server->config->accessories, aid, iid
        );
        if (!ch) {
            CLIENT_ERROR(context, "Failed to process request to update %d.%d: "
                  "no such characteristic", aid, iid);
            return HAPStatus_NoResource;
        }

        cJSON *j_value = cJSON_GetObjectItem(j_ch, "value");
        if (j_value) {
            homekit_value_t h_value = HOMEKIT_NULL();

            if (!(ch->permissions & homekit_permissions_paired_write)) {
                CLIENT_ERROR(context, "Failed to update %d.%d: no write permission", aid, iid);
                return HAPStatus_ReadOnly;
            }

            switch (ch->format) {
                case homekit_format_bool: {
                    bool value = false;
                    if (j_value->type == cJSON_True) {
                        value = true;
                    } else if (j_value->type == cJSON_False) {
                        value = false;
                    } else if (j_value->type == cJSON_Number &&
                            (j_value->valueint == 0 || j_value->valueint == 1)) {
                        value = j_value->valueint == 1;
                    } else {
                        CLIENT_ERROR(context, "Failed to update %d.%d: value is not a boolean or 0/1", aid, iid);
                        return HAPStatus_InvalidValue;
                    }

                    CLIENT_DEBUG(context, "Updating characteristic %d.%d with boolean %s", aid, iid, value ? "true" : "false");

                    h_value = HOMEKIT_BOOL(value);
                    if (ch->setter_ex) {
                        ch->setter_ex(ch, h_value);
                    } else {
                        ch->value = h_value;
                    }
                    break;
                }
                case homekit_format_uint8:
                case homekit_format_uint16:
                case homekit_format_uint32:
                case homekit_format_uint64:
                case homekit_format_int: {
                    // We accept boolean values here in order to fix a bug in HomeKit. HomeKit sometimes sends a boolean instead of an integer of value 0 or 1.
                    if (j_value->type != cJSON_Number && j_value->type != cJSON_False && j_value->type != cJSON_True) {
                        CLIENT_ERROR(context, "Failed to update %d.%d: value is not a number", aid, iid);
                        return HAPStatus_InvalidValue;
                    }

                    double min_value = 0;
                    double max_value = 0;

                    switch (ch->format) {
                        case homekit_format_uint8: {
                            min_value = 0;
                            max_value = 255;
                            break;
                        }
                        case homekit_format_uint16: {
                            min_value = 0;
                            max_value = 65535;
                            break;
                        }
                        case homekit_format_uint32: {
                            min_value = 0;
                            max_value = 4294967295;
                            break;
                        }
                        case homekit_format_uint64: {
                            min_value = 0;
                            max_value = 18446744073709551615ULL;
                            break;
                        }
                        case homekit_format_int: {
                            min_value = -2147483648;
                            max_value = 2147483647;
                            break;
                        }
                        default: {
                            // Impossible, keeping to make compiler happy
                            break;
                        }
                    }

                    if (ch->min_value)
                        min_value = *ch->min_value;
                    if (ch->max_value)
                        max_value = *ch->max_value;

                    double value = j_value->valuedouble;
                    if (value < min_value || value > max_value) {
                        CLIENT_ERROR(context, "Failed to update %d.%d: value %g is not in range %g..%g",
                                     aid, iid, value, min_value, max_value);
                        return HAPStatus_InvalidValue;
                    }

                    if (ch->valid_values.count) {
                        bool matches = false;
                        int v = (int)value;
                        for (int i=0; i<ch->valid_values.count; i++) {
                            if (v == ch->valid_values.values[i]) {
                                matches = true;
                                break;
                            }
                        }

                        if (!matches) {
                            CLIENT_ERROR(context, "Failed to update %d.%d: value is not one of valid values", aid, iid);
                            return HAPStatus_InvalidValue;
                        }
                    }

                    if (ch->valid_values_ranges.count) {
                        bool matches = false;
                        for (int i=0; i<ch->valid_values_ranges.count; i++) {
                            if (value >= ch->valid_values_ranges.ranges[i].start &&
                                    value <= ch->valid_values_ranges.ranges[i].end) {
                                matches = true;
                                break;
                            }
                        }

                        if (!matches) {
                            CLIENT_ERROR(context, "Failed to update %d.%d: value is not in valid values range", aid, iid);
                            return HAPStatus_InvalidValue;
                        }
                    }

                    CLIENT_DEBUG(context, "Updating characteristic %d.%d with integer %g", aid, iid, value);

                    switch (ch->format) {
                        case homekit_format_uint8:
                            h_value = HOMEKIT_UINT8(value);
                            break;
                        case homekit_format_uint16:
                            h_value = HOMEKIT_UINT16(value);
                            break;
                        case homekit_format_uint32:
                            h_value = HOMEKIT_UINT32(value);
                            break;
                        case homekit_format_uint64:
                            h_value = HOMEKIT_UINT64(value);
                            break;
                        case homekit_format_int:
                            h_value = HOMEKIT_INT(value);
                            break;

                        default:
                            CLIENT_ERROR(context, "Unexpected format when updating numeric value: %d", ch->format);
                            return HAPStatus_InvalidValue;
                    }

                    if (ch->setter_ex) {
                        ch->setter_ex(ch, h_value);
                    } else {
                        ch->value = h_value;
                    }
                    break;
                }
                case homekit_format_float: {
                    if (j_value->type != cJSON_Number) {
                        CLIENT_ERROR(context, "Failed to update %d.%d: value is not a number", aid, iid);
                        return HAPStatus_InvalidValue;
                    }

                    float value = j_value->valuedouble;
                    if ((ch->min_value && value < *ch->min_value) ||
                            (ch->max_value && value > *ch->max_value)) {
                        CLIENT_ERROR(context, "Failed to update %d.%d: value is not in range", aid, iid);
                        return HAPStatus_InvalidValue;
                    }

                    CLIENT_DEBUG(context, "Updating characteristic %d.%d with %g", aid, iid, value);

                    h_value = HOMEKIT_FLOAT(value);
                    if (ch->setter_ex) {
                        ch->setter_ex(ch, h_value);
                    } else {
                        ch->value = h_value;
                    }
                    break;
                }
                case homekit_format_string: {
                    if (j_value->type != cJSON_String) {
                        CLIENT_ERROR(context, "Failed to update %d.%d: value is not a string", aid, iid);
                        return HAPStatus_InvalidValue;
                    }

                    int max_len = (ch->max_len) ? *ch->max_len : 64;

                    char *value = j_value->valuestring;
                    if (strlen(value) > max_len) {
                        CLIENT_ERROR(context, "Failed to update %d.%d: value is too long", aid, iid);
                        return HAPStatus_InvalidValue;
                    }

                    CLIENT_DEBUG(context, "Updating characteristic %d.%d with \"%s\"", aid, iid, value);

                    h_value = HOMEKIT_STRING(value);
                    if (ch->setter_ex) {
                        ch->setter_ex(ch, h_value);
                    } else {
                        homekit_value_destruct(&ch->value);
                        homekit_value_copy(&ch->value, &h_value);
                    }
                    break;
                }
                case homekit_format_tlv: {
                    if (j_value->type != cJSON_String) {
                        CLIENT_ERROR(context, "Failed to update %d.%d: value is not a string", aid, iid);
                        return HAPStatus_InvalidValue;
                    }

                    int max_len = (ch->max_len) ? *ch->max_len : 256;

                    char *value = j_value->valuestring;
                    size_t value_len = strlen(value);
                    if (value_len > max_len) {
                        CLIENT_ERROR(context, "Failed to update %d.%d: value is too long", aid, iid);
                        return HAPStatus_InvalidValue;
                    }

                    size_t tlv_size = base64_decoded_size((unsigned char*)value, value_len);
                    byte *tlv_data = malloc(tlv_size);
                    if (base64_decode((byte*) value, value_len, tlv_data) < 0) {
                        free(tlv_data);
                        CLIENT_ERROR(context, "Failed to update %d.%d: error Base64 decoding", aid, iid);
                        return HAPStatus_InvalidValue;
                    }

                    tlv_values_t *tlv_values = tlv_new();
                    int r = tlv_parse(tlv_data, tlv_size, tlv_values);
                    free(tlv_data);

                    if (r) {
                        CLIENT_ERROR(context, "Failed to update %d.%d: error parsing TLV", aid, iid);
                        return HAPStatus_InvalidValue;
                    }

                    CLIENT_DEBUG(context, "Updating characteristic %d.%d with TLV:", aid, iid);
                    for (tlv_t *t=tlv_values->head; t; t=t->next) {
                        char *escaped_payload = binary_to_string(t->value, t->size);
                        CLIENT_DEBUG(context, "  Type %d value (%d bytes): %s", t->type, t->size, escaped_payload);
                        free(escaped_payload);
                    }

                    h_value = HOMEKIT_TLV(tlv_values);
                    if (ch->setter_ex) {
                        ch->setter_ex(ch, h_value);
                    } else {
                        homekit_value_destruct(&ch->value);
                        homekit_value_copy(&ch->value, &h_value);
                    }

                    tlv_free(tlv_values);
                    break;
                }
                case homekit_format_data: {
                    // TODO:
                    break;
                }
            }

            if (!h_value.is_null) {
                context->current_characteristic = ch;
                context->current_value = &h_value;

                homekit_characteristic_notify(ch, h_value);

                context->current_characteristic = NULL;
                context->current_value = NULL;
            }
        }

        cJSON *j_events = cJSON_GetObjectItem(j_ch, "ev");
        if (j_events) {
            if (!(ch->permissions && homekit_permissions_notify)) {
                CLIENT_ERROR(context, "Failed to set notification state for %d.%d: "
                      "notifications are not supported", aid, iid);
                return HAPStatus_NotificationsUnsupported;
            }

            if ((j_events->type != cJSON_True) && (j_events->type != cJSON_False)) {
                CLIENT_ERROR(context, "Failed to set notification state for %d.%d: "
                      "invalid state value", aid, iid);
            }

            if (j_events->type == cJSON_True) {
                homekit_characteristic_add_notify_callback(ch, client_notify_characteristic, context);
            } else {
                homekit_characteristic_remove_notify_callback(ch, client_notify_characteristic, context);
            }
        }

        return HAPStatus_Success;
    }

    HAPStatus *statuses = malloc(sizeof(HAPStatus) * cJSON_GetArraySize(characteristics));
    bool has_errors = false;
    for (int i=0; i < cJSON_GetArraySize(characteristics); i++) {
        cJSON *j_ch = cJSON_GetArrayItem(characteristics, i);

        char *s = cJSON_Print(j_ch);
        CLIENT_DEBUG(context, "Processing element %s", s);
        free(s);

        statuses[i] = process_characteristics_update(j_ch);

        if (statuses[i] != HAPStatus_Success)
            has_errors = true;
    }

    if (!has_errors) {
        CLIENT_DEBUG(context, "There were no processing errors, sending No Content response");

        send_204_response(context);
    } else {
        CLIENT_DEBUG(context, "There were processing errors, sending Multi-Status response");
        client_send(context, json_207_response_headers, sizeof(json_207_response_headers)-1);

        json_stream *json1 = json_new(1024, client_send_chunk, context);
        json_object_start(json1);
        json_string(json1, "characteristics"); json_array_start(json1);

        for (int i=0; i < cJSON_GetArraySize(characteristics); i++) {
            cJSON *j_ch = cJSON_GetArrayItem(characteristics, i);

            json_object_start(json1);
            json_string(json1, "aid"); json_uint32(json1, cJSON_GetObjectItem(j_ch, "aid")->valueint);
            json_string(json1, "iid"); json_uint32(json1, cJSON_GetObjectItem(j_ch, "iid")->valueint);
            json_string(json1, "status"); json_uint8(json1, statuses[i]);
            json_object_end(json1);
        }

        json_array_end(json1);
        json_object_end(json1); // response

        json_flush(json1);
        json_free(json1);

        client_send_chunk(NULL, 0, context);
    }

    free(statuses);
    cJSON_Delete(json);
}

void homekit_server_on_pairings(client_context_t *context, const byte *data, size_t size) {
    DEBUG("HomeKit Pairings");
    DEBUG_HEAP();

    tlv_values_t *message = tlv_new();
    tlv_parse(data, size, message);

    TLV_DEBUG(message);

    int r;

    if (tlv_get_integer_value(message, TLVType_State, -1) != 1) {
        send_tlv_error_response(context, 2, TLVError_Unknown);
        tlv_free(message);
        return;
    }

    switch(tlv_get_integer_value(message, TLVType_Method, -1)) {
        case TLVMethod_AddPairing: {
            CLIENT_INFO(context, "Add Pairing");

            if (!(context->permissions & pairing_permissions_admin)) {
                CLIENT_ERROR(context, "Refusing to add pairing to non-admin controller");
                send_tlv_error_response(context, 2, TLVError_Authentication);
                break;
            }

            tlv_t *tlv_device_identifier = tlv_get_value(message, TLVType_Identifier);
            if (!tlv_device_identifier) {
                CLIENT_ERROR(context, "Invalid add pairing request: no device identifier");
                send_tlv_error_response(context, 2, TLVError_Unknown);
                break;
            }
            tlv_t *tlv_device_public_key = tlv_get_value(message, TLVType_PublicKey);
            if (!tlv_device_public_key) {
                CLIENT_ERROR(context, "Invalid add pairing request: no device public key");
                send_tlv_error_response(context, 2, TLVError_Unknown);
                break;
            }
            int device_permissions = tlv_get_integer_value(message, TLVType_Permissions, -1);
            if (device_permissions == -1) {
                CLIENT_ERROR(context, "Invalid add pairing request: no device permissions");
                send_tlv_error_response(context, 2, TLVError_Unknown);
                break;
            }

            ed25519_key *device_key = crypto_ed25519_new();
            r = crypto_ed25519_import_public_key(
                device_key, tlv_device_public_key->value, tlv_device_public_key->size
            );
            if (r) {
                CLIENT_ERROR(context, "Failed to import device public key");
                crypto_ed25519_free(device_key);
                send_tlv_error_response(context, 2, TLVError_Unknown);
                break;
            }

            char *device_identifier = strndup(
                (const char *)tlv_device_identifier->value,
                tlv_device_identifier->size
            );

            pairing_t *pairing = homekit_storage_find_pairing(device_identifier);
            if (pairing) {
                size_t pairing_public_key_size = 0;
                crypto_ed25519_export_public_key(pairing->device_key, NULL, &pairing_public_key_size);

                byte *pairing_public_key = malloc(pairing_public_key_size);
                r = crypto_ed25519_export_public_key(pairing->device_key, pairing_public_key, &pairing_public_key_size);
                if (r) {
                    CLIENT_ERROR(context, "Failed to add pairing: error exporting pairing public key (code %d)", r);
                    free(pairing_public_key);
                    pairing_free(pairing);
                    free(device_identifier);
                    crypto_ed25519_free(device_key);
                    send_tlv_error_response(context, 2, TLVError_Unknown);
                }

                pairing_free(pairing);

                if (pairing_public_key_size != tlv_device_public_key->size ||
                        memcmp(tlv_device_public_key->value, pairing_public_key, pairing_public_key_size)) {
                    CLIENT_ERROR(context, "Failed to add pairing: pairing public key differs from given one");
                    free(pairing_public_key);
                    free(device_identifier);
                    crypto_ed25519_free(device_key);
                    send_tlv_error_response(context, 2, TLVError_Unknown);
                }

                free(pairing_public_key);

                r = homekit_storage_update_pairing(device_identifier, device_permissions);
                if (r) {
                    CLIENT_ERROR(context, "Failed to add pairing: storage error (code %d)", r);
                    free(device_identifier);
                    crypto_ed25519_free(device_key);
                    send_tlv_error_response(context, 2, TLVError_Unknown);
                    break;
                }

                INFO("Updated pairing with %s", device_identifier);
            } else {
                if (!homekit_storage_can_add_pairing()) {
                    CLIENT_ERROR(context, "Failed to add pairing: max peers");
                    free(device_identifier);
                    crypto_ed25519_free(device_key);
                    send_tlv_error_response(context, 2, TLVError_MaxPeers);
                    break;
                }

                r = homekit_storage_add_pairing(
                    device_identifier, device_key, device_permissions
                );
                if (r) {
                    CLIENT_ERROR(context, "Failed to add pairing: storage error (code %d)", r);
                    free(device_identifier);
                    crypto_ed25519_free(device_key);
                    send_tlv_error_response(context, 2, TLVError_Unknown);
                    break;
                }

                INFO("Added pairing with %s", device_identifier);

                HOMEKIT_NOTIFY_EVENT(context->server, HOMEKIT_EVENT_PAIRING_ADDED);
            }

            free(device_identifier);
            crypto_ed25519_free(device_key);

            tlv_values_t *response = tlv_new();
            tlv_add_integer_value(response, TLVType_State, 1, 2);

            send_tlv_response(context, response);

            break;
        }
        case TLVMethod_RemovePairing: {
            CLIENT_INFO(context, "Remove Pairing");

            if (!(context->permissions & pairing_permissions_admin)) {
                CLIENT_ERROR(context, "Refusing to remove pairing to non-admin controller");
                send_tlv_error_response(context, 2, TLVError_Authentication);
                break;
            }

            tlv_t *tlv_device_identifier = tlv_get_value(message, TLVType_Identifier);
            if (!tlv_device_identifier) {
                CLIENT_ERROR(context, "Invalid remove pairing request: no device identifier");
                send_tlv_error_response(context, 2, TLVError_Unknown);
                break;
            }

            char *device_identifier = strndup(
                (const char *)tlv_device_identifier->value,
                tlv_device_identifier->size
            );

            pairing_t *pairing = homekit_storage_find_pairing(device_identifier);

            if (pairing) {
                bool is_admin = pairing->permissions & pairing_permissions_admin;
                pairing_free(pairing);

                r = homekit_storage_remove_pairing(device_identifier);
                if (r) {
                    CLIENT_ERROR(context, "Failed to remove pairing: storage error (code %d)", r);
                    free(device_identifier);
                    send_tlv_error_response(context, 2, TLVError_Unknown);
                    break;
                }

                INFO("Removed pairing with %s", device_identifier);

                HOMEKIT_NOTIFY_EVENT(context->server, HOMEKIT_EVENT_PAIRING_REMOVED);

                client_context_t *c = context->server->clients;
                while (c) {
                    if (c->pairing_id == pairing->id)
                        c->disconnect = true;
                    c = c->next;
                }

                if (is_admin) {
                    // Removed pairing was admin,
                    // check if there any other admins left.
                    // If no admins left, enable pairing again
                    pairing_iterator_t *pairing_it = homekit_storage_pairing_iterator();
                    pairing_t *pairing;
                    while ((pairing = homekit_storage_next_pairing(pairing_it))) {
                        if (pairing->permissions & pairing_permissions_admin) {
                            break;
                        }
                        pairing_free(pairing);
                    };
                    homekit_storage_pairing_iterator_free(pairing_it);

                    if (!pairing) {
                        // No admins left, enable pairing again
                        INFO("Last admin pairing was removed, enabling pair setup");

                        context->server->paired = false;
                        homekit_setup_mdns(context->server);
                    } else {
                        pairing_free(pairing);
                    }
                }
            }

            free(device_identifier);

            tlv_values_t *response = tlv_new();
            tlv_add_integer_value(response, TLVType_State, 1, 2);

            send_tlv_response(context, response);
            break;
        }
        case TLVMethod_ListPairings: {
            CLIENT_INFO(context, "List Pairings");

            if (!(context->permissions & pairing_permissions_admin)) {
                CLIENT_INFO(context, "Refusing to list pairings to non-admin controller");
                send_tlv_error_response(context, 2, TLVError_Authentication);
                break;
            }

            tlv_values_t *response = tlv_new();
            tlv_add_integer_value(response, TLVType_State, 1, 2);

            bool first = true;

            pairing_iterator_t *it = homekit_storage_pairing_iterator();
            pairing_t *pairing = NULL;

            size_t public_key_size = 32;
            byte *public_key = malloc(public_key_size);

            while ((pairing = homekit_storage_next_pairing(it))) {
                if (!first) {
                    tlv_add_value(response, TLVType_Separator, NULL, 0);
                }
                r = crypto_ed25519_export_public_key(pairing->device_key, public_key, &public_key_size);

                tlv_add_string_value(response, TLVType_Identifier, pairing->device_id);
                tlv_add_value(response, TLVType_PublicKey, public_key, public_key_size);
                tlv_add_integer_value(response, TLVType_Permissions, 1, pairing->permissions);

                first = false;

                pairing_free(pairing);
            }
            homekit_storage_pairing_iterator_free(it);

            free(public_key);

            send_tlv_response(context, response);
            break;
        }
        default: {
            send_tlv_error_response(context, 2, TLVError_Unknown);
            break;
        }
    }

    tlv_free(message);
}

void homekit_server_on_reset(client_context_t *context) {
    INFO("Reset");

    homekit_server_reset();
    send_204_response(context);

    sleep(3000 / portTICK_PERIOD_MS);

    homekit_system_restart();
}

void homekit_server_on_resource(client_context_t *context) {
    CLIENT_INFO(context, "Resource");
    DEBUG_HEAP();

    if (!context->server->config->on_resource) {
        send_404_response(context);
        return;
    }

    context->server->config->on_resource(context->body, context->body_length);
}


int homekit_server_on_url(http_parser *parser, const char *data, size_t length) {
    client_context_t *context = (client_context_t*) parser->data;

    context->endpoint = HOMEKIT_ENDPOINT_UNKNOWN;
    if (parser->method == HTTP_GET) {
        if (!strncmp(data, "/accessories", length)) {
            context->endpoint = HOMEKIT_ENDPOINT_GET_ACCESSORIES;
        } else {
            static const char url[] = "/characteristics";
            size_t url_len = sizeof(url)-1;

            if (length >= url_len && !strncmp(data, url, url_len) &&
                    (data[url_len] == 0 || data[url_len] == '?'))
            {
                context->endpoint = HOMEKIT_ENDPOINT_GET_CHARACTERISTICS;
                if (data[url_len] == '?') {
                    char *query = strndup(data+url_len+1, length-url_len-1);
                    context->endpoint_params = query_params_parse(query);
                    free(query);
                }
            }
        }
    } else if (parser->method == HTTP_POST) {
        if (!strncmp(data, "/identify", length)) {
            context->endpoint = HOMEKIT_ENDPOINT_IDENTIFY;
        } else if (!strncmp(data, "/pair-setup", length)) {
            context->endpoint = HOMEKIT_ENDPOINT_PAIR_SETUP;
        } else if (!strncmp(data, "/pair-verify", length)) {
            context->endpoint = HOMEKIT_ENDPOINT_PAIR_VERIFY;
        } else if (!strncmp(data, "/pairings", length)) {
            context->endpoint = HOMEKIT_ENDPOINT_PAIRINGS;
        } else if (!strncmp(data, "/reset", length)) {
            context->endpoint = HOMEKIT_ENDPOINT_RESET;
        } else if (!strncmp(data, "/resource", length)) {
            context->endpoint = HOMEKIT_ENDPOINT_RESOURCE;
        }
    } else if (parser->method == HTTP_PUT) {
        if (!strncmp(data, "/characteristics", length)) {
            context->endpoint = HOMEKIT_ENDPOINT_UPDATE_CHARACTERISTICS;
        }
    }

    if (context->endpoint == HOMEKIT_ENDPOINT_UNKNOWN) {
        char *url = strndup(data, length);
        ERROR("Unknown endpoint: %s %s", http_method_str(parser->method), url);
        free(url);
    }

    return 0;
}

int homekit_server_on_body(http_parser *parser, const char *data, size_t length) {
    client_context_t *context = parser->data;
    context->body = realloc(context->body, context->body_length + length + 1);
    memcpy(context->body + context->body_length, data, length);
    context->body_length += length;
    context->body[context->body_length] = 0;

    return 0;
}

int homekit_server_on_message_complete(http_parser *parser) {
    client_context_t *context = parser->data;

    switch(context->endpoint) {
        case HOMEKIT_ENDPOINT_PAIR_SETUP: {
            homekit_server_on_pair_setup(context, (const byte *)context->body, context->body_length);
            break;
        }
        case HOMEKIT_ENDPOINT_PAIR_VERIFY: {
            homekit_server_on_pair_verify(context, (const byte *)context->body, context->body_length);
            break;
        }
        case HOMEKIT_ENDPOINT_IDENTIFY: {
            homekit_server_on_identify(context);
            break;
        }
        case HOMEKIT_ENDPOINT_GET_ACCESSORIES: {
            homekit_server_on_get_accessories(context);
            break;
        }
        case HOMEKIT_ENDPOINT_GET_CHARACTERISTICS: {
            homekit_server_on_get_characteristics(context);
            break;
        }
        case HOMEKIT_ENDPOINT_UPDATE_CHARACTERISTICS: {
            homekit_server_on_update_characteristics(context, (const byte *)context->body, context->body_length);
            break;
        }
        case HOMEKIT_ENDPOINT_PAIRINGS: {
            homekit_server_on_pairings(context, (const byte *)context->body, context->body_length);
            break;
        }
        case HOMEKIT_ENDPOINT_RESET: {
            homekit_server_on_reset(context);
            break;
        }
        case HOMEKIT_ENDPOINT_RESOURCE: {
            homekit_server_on_resource(context);
            break;
        }
        case HOMEKIT_ENDPOINT_UNKNOWN: {
            DEBUG("Unknown endpoint");
            send_404_response(context);
            break;
        }
    }

    if (context->endpoint_params) {
        query_params_free(context->endpoint_params);
        context->endpoint_params = NULL;
    }

    if (context->body) {
        free(context->body);
        context->body = NULL;
        context->body_length = 0;
    }

    return 0;
}


static http_parser_settings homekit_http_parser_settings = {
    .on_url = homekit_server_on_url,
    .on_body = homekit_server_on_body,
    .on_message_complete = homekit_server_on_message_complete,
};


static void homekit_client_process(client_context_t *context) {
    int data_len = read(
        context->socket,
        context->data+context->data_available,
        context->data_size-context->data_available
    );
    if (data_len == 0) {
        context->disconnect = true;
        return;
    }

    if (data_len < 0) {
        if (errno != EAGAIN) {
            CLIENT_ERROR(context, "Error reading data from socket (code %d). Disconnecting", errno);
            context->disconnect = true;
        }
        return;
    }

    CLIENT_DEBUG(context, "Got %d incomming data", data_len);
    byte *payload = (byte *)context->data;
    size_t payload_size = (size_t)data_len;

    byte *decrypted = NULL;
    size_t decrypted_size = 0;

    if (context->encrypted) {
        CLIENT_DEBUG(context, "Decrypting data");

        client_decrypt(context, context->data, data_len, NULL, &decrypted_size);

        decrypted = malloc(decrypted_size);
        int r = client_decrypt(context, context->data, data_len, decrypted, &decrypted_size);
        if (r < 0) {
            CLIENT_ERROR(context, "Invalid client data");
            free(decrypted);
            return;
        }
        context->data_available = data_len - r;
        if (r && context->data_available) {
            memmove(context->data, &context->data[r], context->data_available);
        }
        CLIENT_DEBUG(context, "Decrypted %d bytes, available %d", decrypted_size, context->data_available);

        payload = decrypted;
        payload_size = decrypted_size;
        if (payload_size)
            print_binary("Decrypted data", payload, payload_size);
    } else {
        context->data_available = 0;
    }

    current_client_context = context;

    http_parser_execute(
        context->parser, &homekit_http_parser_settings,
        (char *)payload, payload_size
    );

    current_client_context = NULL;

    CLIENT_DEBUG(context, "Finished processing");

    if (decrypted) {
        free(decrypted);
    }
}


void homekit_server_close_client(homekit_server_t *server, client_context_t *context) {
    CLIENT_INFO(context, "Closing client connection");

    FD_CLR(context->socket, &server->fds);
    // TODO: recalc server->max_fd ?
    server->nfds--;

    close(context->socket);

    if (context->server->pairing_context && context->server->pairing_context->client == context) {
        pairing_context_free(context->server->pairing_context);
        context->server->pairing_context = NULL;
    }

    if (context->server->clients == context) {
        context->server->clients = context->next;
    } else {
        client_context_t *c = context->server->clients;
        while (c->next && c->next != context)
            c = c->next;
        if (c->next)
            c->next = c->next->next;
    }

    homekit_accessories_clear_notify_callbacks(
        context->server->config->accessories,
        client_notify_characteristic,
        context
    );

    HOMEKIT_NOTIFY_EVENT(server, HOMEKIT_EVENT_CLIENT_DISCONNECTED);

    client_context_free(context);
}


client_context_t *homekit_server_accept_client(homekit_server_t *server) {
    int s = accept(server->listen_fd, (struct sockaddr *)NULL, (socklen_t *)NULL);
    if (s < 0)
        return NULL;

    if (server->nfds > HOMEKIT_MAX_CLIENTS) {
        INFO("No more room for client connections (max %d)", HOMEKIT_MAX_CLIENTS);
        close(s);
        return NULL;
    }

    INFO("Got new client connection: %d", s);

#if 0
    const struct timeval rcvtimeout = { 10, 0 }; /* 10 second timeout */
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &rcvtimeout, sizeof(rcvtimeout));

    const int yes = 1; /* enable sending keepalive probes for socket */
    setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes));

    const int idle = 180; /* 180 sec idle before start sending probes */
    setsockopt(s, IPPROTO_TCP, TCP_KEEPIDLE, &idle, sizeof(idle));

    const int interval = 30; /* 30 sec between probes */
    setsockopt(s, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));

    const int maxpkt = 4; /* Drop connection after 4 probes without response */
    setsockopt(s, IPPROTO_TCP, TCP_KEEPCNT, &maxpkt, sizeof(maxpkt));
#endif

    client_context_t *context = client_context_new();
    context->server = server;
    context->socket = s;
    context->next = server->clients;

    server->clients = context;

    FD_SET(s, &server->fds);
    server->nfds++;
    if (s > server->max_fd)
        server->max_fd = s;

    HOMEKIT_NOTIFY_EVENT(server, HOMEKIT_EVENT_CLIENT_CONNECTED);

    return context;
}


client_context_t *homekit_server_find_client_by_fd(homekit_server_t *server, int fd) {
    client_context_t *context = server->clients;
    while (context) {
        if (context->socket == fd)
            return context;
        context = context->next;
    }

    return NULL;
}


void homekit_server_process_notifications(homekit_server_t *server) {
    client_context_t *context = server->clients;
    while (context) {
        characteristic_event_t *event = NULL;
#if 0
        if (xQueueReceive(context->event_queue, &event, 0)) {
            // Get and coalesce all client events
            client_event_t *events_head = malloc(sizeof(client_event_t));
            events_head->characteristic = event->characteristic;
            homekit_value_copy(&events_head->value, &event->value);
            events_head->next = NULL;

            homekit_value_destruct(&event->value);
            free(event);

            client_event_t *events_tail = events_head;

            while (xQueueReceive(context->event_queue, &event, 0)) {
                client_event_t *e = events_head;
                while (e) {
                    if (e->characteristic == event->characteristic)
                        break;

                    e = e->next;
                }

                if (e) {
                    homekit_value_destruct(&e->value);
                } else {
                    e = malloc(sizeof(client_event_t));
                    e->characteristic = event->characteristic;
                    e->next = NULL;

                    events_tail->next = e;
                    events_tail = e;
                }

                homekit_value_copy(&e->value, &event->value);

                homekit_value_destruct(&event->value);
                free(event);
            }

            send_client_events(context, events_head);

            client_event_t *e = events_head;
            while (e) {
                client_event_t *next = e->next;

                homekit_value_destruct(&e->value);
                free(e);

                e = next;
            }
        }
#endif

        context = context->next;
    }
}


void homekit_server_close_clients(homekit_server_t *server) {
    client_context_t *context = server->clients;
    while (context) {
        client_context_t *next = context->next;

        if (context->disconnect)
            homekit_server_close_client(server, context);

        context = next;
    }
}


static void homekit_run_server(homekit_server_t *server)
{
    DEBUG("Staring HTTP server");

    struct sockaddr_in serv_addr;
    server->listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(PORT);
    bind(server->listen_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    listen(server->listen_fd, 10);

    FD_SET(server->listen_fd, &server->fds);
    server->max_fd = server->listen_fd;
    server->nfds = 1;

    for (;;) {
        fd_set read_fds;
        memcpy(&read_fds, &server->fds, sizeof(read_fds));

        struct timeval timeout = { 1, 0 }; /* 1 second timeout */
        int triggered_nfds = select(server->max_fd + 1, &read_fds, NULL, NULL, &timeout);
        if (triggered_nfds > 0) {
            if (FD_ISSET(server->listen_fd, &read_fds)) {
                homekit_server_accept_client(server);
                triggered_nfds--;
            }

            client_context_t *context = server->clients;
            while (context && triggered_nfds) {
                if (FD_ISSET(context->socket, &read_fds)) {
                    homekit_client_process(context);
                    triggered_nfds--;
                }

                context = context->next;
            }

            homekit_server_close_clients(server);
        }

        homekit_server_process_notifications(server);
    }

    server_free(server);
}


void homekit_setup_mdns(homekit_server_t *server) {
    INFO("Configuring mDNS");

    homekit_accessory_t *accessory = server->config->accessories[0];
    homekit_service_t *accessory_info =
        homekit_service_by_type(accessory, HOMEKIT_SERVICE_ACCESSORY_INFORMATION);
    if (!accessory_info) {
        ERROR("Invalid accessory declaration: no Accessory Information service");
        return;
    }

    homekit_characteristic_t *name =
        homekit_service_characteristic_by_type(accessory_info, HOMEKIT_CHARACTERISTIC_NAME);
    if (!name) {
        ERROR("Invalid accessory declaration: "
              "no Name characteristic in AccessoryInfo service");
        return;
    }

    homekit_characteristic_t *model =
        homekit_service_characteristic_by_type(accessory_info, HOMEKIT_CHARACTERISTIC_MODEL);
    if (!model) {
        ERROR("Invalid accessory declaration: "
              "no Model characteristic in AccessoryInfo service");
        return;
    }

    char unique_name[65]={0};
    strncpy(unique_name, name->value.string_value, sizeof(unique_name)-6);
    unique_name[strlen(unique_name)]='-';
    unique_name[strlen(unique_name)]=server->accessory_id[0];
    unique_name[strlen(unique_name)]=server->accessory_id[1];
    unique_name[strlen(unique_name)]=server->accessory_id[3];
    unique_name[strlen(unique_name)]=server->accessory_id[4];
    homekit_mdns_configure_init(unique_name, PORT);

    // accessory model name (required)
    homekit_mdns_add_txt("md", "%s", model->value.string_value);
    // protocol version (required)
    homekit_mdns_add_txt("pv", "1.0");
    // device ID (required)
    // should be in format XX:XX:XX:XX:XX:XX, otherwise devices will ignore it
    homekit_mdns_add_txt("id", "%s", server->accessory_id);
    // current configuration number (required)
    homekit_mdns_add_txt("c#", "%d", server->config->config_number);
    // current state number (required)
    homekit_mdns_add_txt("s#", "1");
    // feature flags (required if non-zero)
    //   bit 0 - supports HAP pairing. required for all HomeKit accessories
    //   bits 1-7 - reserved
    homekit_mdns_add_txt("ff", "0");
    // status flags
    //   bit 0 - not paired
    //   bit 1 - not configured to join WiFi
    //   bit 2 - problem detected on accessory
    //   bits 3-7 - reserved
    homekit_mdns_add_txt("sf", "%d", (server->paired) ? 0 : 1);
    // accessory category identifier
    homekit_mdns_add_txt("ci", "%d", server->config->category);

    if (server->config->setupId) {
        DEBUG("Accessory Setup ID = %s", server->config->setupId);

        size_t data_size = strlen(server->config->setupId) + strlen(server->accessory_id) + 1;
        char *data = malloc(data_size);
        snprintf(data, data_size, "%s%s", server->config->setupId, server->accessory_id);
        data[data_size-1] = 0;

        unsigned char shaHash[SHA512_DIGEST_SIZE];
        wc_Sha512Hash((const unsigned char *)data, data_size-1, shaHash);

        free(data);

        unsigned char encodedHash[9];
        memset(encodedHash, 0, sizeof(encodedHash));

        word32 len = sizeof(encodedHash);
        Base64_Encode_NoNl((const unsigned char *)shaHash, 4, encodedHash, &len);

        homekit_mdns_add_txt("sh", "%s", encodedHash);
    }

    homekit_mdns_configure_finalize();
}

char *homekit_accessory_id_generate() {
    char *accessory_id = malloc(18);

    byte buf[6];
    homekit_random_fill(buf, sizeof(buf));

    snprintf(accessory_id, 18, "%02X:%02X:%02X:%02X:%02X:%02X",
             buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);

    INFO("Generated new accessory ID: %s", accessory_id);
    return accessory_id;
}

ed25519_key *homekit_accessory_key_generate() {
    ed25519_key *key = crypto_ed25519_generate();
    if (!key) {
        ERROR("Failed to generate accessory key");
        return NULL;
    }

    INFO("Generated new accessory key");

    return key;
}

void homekit_server_task(void *args) {
    homekit_server_t *server = args;
    INFO("Starting server");

    int r = homekit_storage_init();

    if (r == 0) {
        server->accessory_id = homekit_storage_load_accessory_id();
        server->accessory_key = homekit_storage_load_accessory_key();
    }
    if (!server->accessory_id || !server->accessory_key) {
        server->accessory_id = homekit_accessory_id_generate();
        homekit_storage_save_accessory_id(server->accessory_id);

        server->accessory_key = homekit_accessory_key_generate();
        homekit_storage_save_accessory_key(server->accessory_key);
    } else {
        INFO("Using existing accessory ID: %s", server->accessory_id);
    }

    pairing_iterator_t *pairing_it = homekit_storage_pairing_iterator();
    pairing_t *pairing;
    while ((pairing = homekit_storage_next_pairing(pairing_it))) {
        if (pairing->permissions & pairing_permissions_admin) {
            break;
        }
        pairing_free(pairing);
    }
    homekit_storage_pairing_iterator_free(pairing_it);

    if (pairing) {
        INFO("Found admin pairing with %s, disabling pair setup", pairing->device_id);
        pairing_free(pairing);
        server->paired = true;
    }

    homekit_mdns_init();
    homekit_setup_mdns(server);

    HOMEKIT_NOTIFY_EVENT(server, HOMEKIT_EVENT_SERVER_INITIALIZED);

    homekit_run_server(server);

#if 0
    vTaskDelete(NULL);
#endif
}

#define ISDIGIT(x) isdigit((unsigned char)(x))
#define ISBASE36(x) (isdigit((unsigned char)(x)) || (x >= 'A' && x <= 'Z'))

void homekit_server_init(homekit_server_config_t *config) {
    if (!config->accessories) {
        ERROR("Error initializing HomeKit accessory server: "
              "accessories are not specified");
        return;
    }

    if (!config->password && !config->password_callback) {
        ERROR("Error initializing HomeKit accessory server: "
              "neither password nor password callback is specified");
        return;
    }

    if (config->password) {
        const char *p = config->password;
        if (strlen(p) != 10 ||
                !(ISDIGIT(p[0]) && ISDIGIT(p[1]) && ISDIGIT(p[2]) && p[3] == '-' &&
                    ISDIGIT(p[4]) && ISDIGIT(p[5]) && p[6] == '-' &&
                    ISDIGIT(p[7]) && ISDIGIT(p[8]) && ISDIGIT(p[9]))) {
            ERROR("Error initializing HomeKit accessory server: "
                  "invalid password format");
            return;
        }
    }

    if (config->setupId) {
        const char *p = config->setupId;
        if (strlen(p) != 4 ||
                !(ISBASE36(p[0]) && ISBASE36(p[1]) && ISBASE36(p[2]) && ISBASE36(p[3]))) {
            ERROR("Error initializing HomeKit accessory server: "
                  "invalid setup ID format");
            return;
        }
    }

    homekit_accessories_init(config->accessories);

    if (!config->config_number) {
        config->config_number = config->accessories[0]->config_number;
        if (!config->config_number) {
            config->config_number = 1;
        }
    }

    if (!config->category) {
        config->category = config->accessories[0]->category;
    }

    homekit_server_t *server = server_new();
    server->config = config;

#if 0
    if (NULL != xTaskCreate(homekit_server_task, "HomeKit Server",
                              SERVER_TASK_STACK, server, 1, NULL)) {
        ERROR("Error initializing HomeKit accessory server: "
              "failed to start a server task");
        server_free(server);
    }
#else
    pthread_t tid;
    pthread_create(&tid, NULL, homekit_server_task, server);

#endif
    ERROR("homekit_server_init done");
}

void homekit_server_reset() {
    homekit_storage_reset();
}

bool homekit_is_paired() {
    pairing_iterator_t *pairing_it = homekit_storage_pairing_iterator();
    pairing_t *pairing;
    while ((pairing = homekit_storage_next_pairing(pairing_it))) {
        if (pairing->permissions & pairing_permissions_admin) {
            break;
        }
        pairing_free(pairing);
    };
    homekit_storage_pairing_iterator_free(pairing_it);

    bool paired = false;
    if (pairing) {
        paired = true;
        pairing_free(pairing);
    }

    return paired;
}

int homekit_get_accessory_id(char *buffer, size_t size) {
    char *accessory_id = homekit_storage_load_accessory_id();
    if (!accessory_id)
        return -2;

    if (size < strlen(accessory_id) + 1)
        return -1;

    strncpy(buffer, accessory_id, size);

    free(accessory_id);

    return 0;
}

int homekit_get_setup_uri(const homekit_server_config_t *config, char *buffer, size_t buffer_size) {
    static const char base36Table[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    if (buffer_size < 20)
        return -1;

    if (!config->password)
        return -1;
    // TODO: validate password in case it is run beffore server is started

    if (!config->setupId)
        return -1;
    // TODO: validate setupID in case it is run beffore server is started

    homekit_accessory_t *accessory = homekit_accessory_by_id(config->accessories, 1);
    if (!accessory)
        return -1;

    uint32_t setup_code = 0;
    for (const char *s = config->password; *s; s++)
        if ISDIGIT(*s)
            setup_code = setup_code * 10 + *s - '0';

    uint64_t payload = 0;

    payload <<= 4;  // reserved 4 bits

    payload <<= 8;
    payload |= accessory->category & 0xff;

    payload <<= 4;
    payload |= 2;  // flags (2=IP, 4=BLE, 8=IP_WAC)

    payload <<= 27;
    payload |= setup_code & 0x7fffffff;

    strcpy(buffer, "X-HM://");
    buffer += 7;
    for (int i=8; i >= 0; i--) {
        buffer[i] = base36Table[payload % 36];
        payload /= 36;
    }
    buffer += 9;

    strcpy(buffer, config->setupId);
    buffer += 4;

    buffer[0] = 0;

    return 0;
}
