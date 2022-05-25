#pragma once

#include <stddef.h>

size_t base64_encoded_size(const unsigned char* data, size_t size);
size_t base64_decoded_size(const unsigned char* encoded_data, size_t encoded_size);

int base64_encode(const unsigned char* data, size_t data_size, unsigned char *encoded_data);
int base64_decode(const unsigned char* encoded_data, size_t encoded_size, unsigned char *data);

