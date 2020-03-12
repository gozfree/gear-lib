#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdlib.h>
#include <stdio.h>

typedef unsigned char byte;

#ifdef HOMEKIT_DEBUG

#define DEBUG(message, ...) printf(">>> %s: " message "\n", __func__, ##__VA_ARGS__)

#else

#define DEBUG(message, ...)

#endif

#define INFO(message, ...) printf(">>> HomeKit: %s:%d " message "\n", __func__, __LINE__,  ##__VA_ARGS__)
#define ERROR(message, ...) printf("!!! HomeKit: %s:%d " message "\n", __func__, __LINE__,  ##__VA_ARGS__)

#ifdef ESP_IDF

#define DEBUG_HEAP() DEBUG("Free heap: %d", esp_get_free_heap_size());

#else

#define DEBUG_HEAP() DEBUG("Free heap: %d", xPortGetFreeHeapSize());

#endif

char *binary_to_string(const byte *data, size_t size);
void print_binary(const char *prompt, const byte *data, size_t size);

#endif // __DEBUG_H__
