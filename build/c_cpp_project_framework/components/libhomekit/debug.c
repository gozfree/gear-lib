#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

static char *binary_to_string(const uint8_t *data, size_t size) {
    int i;

    size_t buffer_size = size * 3 + 1; // 1 char for eos

    char *buffer = malloc(buffer_size);

    int pos = 0;
    for (i=0; i<size; i++) {
        size_t printed = snprintf(&buffer[pos], buffer_size - pos, "%02X ", data[i]);
        if (printed > 0) {
            pos += printed;
        }
    }
    buffer[pos] = 0;

    return buffer;
}


void dump_binary(const char *prompt, const uint8_t *data, size_t size) {
    char *buffer = binary_to_string(data, size);
    printf("%s (%d bytes): \"%s\"\n", prompt, (int)size, buffer);
    free(buffer);
}
