#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef ESP_OPEN_RTOS

#include <string.h>
#include <esp/hwrand.h>
#include <espressif/esp_common.h>
#include <esplibs/libmain.h>
#include "mdnsresponder.h"

#ifndef MDNS_TTL
#define MDNS_TTL 4500
#endif

uint32_t homekit_random() {
    return hwrand();
}

void homekit_random_fill(uint8_t *data, size_t size) {
    hwrand_fill(data, size);
}

void homekit_system_restart() {
    sdk_system_restart();
}

void homekit_overclock_start() {
    sdk_system_overclock();
}

void homekit_overclock_end() {
    sdk_system_restoreclock();
}

static char mdns_instance_name[65] = {0};
static char mdns_txt_rec[128] = {0};
static int mdns_port = 80;

void homekit_mdns_init() {
    mdns_init();
}

void homekit_mdns_configure_init(const char *instance_name, int port) {
    strncpy(mdns_instance_name, instance_name, sizeof(mdns_instance_name));
    mdns_txt_rec[0] = 0;
    mdns_port = port;
}

void homekit_mdns_add_txt(const char *key, const char *format, ...) {
    va_list arg_ptr;
    va_start(arg_ptr, format);

    char value[128];
    int value_len = vsnprintf(value, sizeof(value), format, arg_ptr);

    va_end(arg_ptr);

    if (value_len && value_len < sizeof(value)-1) {
        char buffer[128];
        int buffer_len = snprintf(buffer, sizeof(buffer), "%s=%s", key, value);

        if (buffer_len < sizeof(buffer)-1)
            mdns_TXT_append(mdns_txt_rec, sizeof(mdns_txt_rec), buffer, buffer_len);
    }
}

void homekit_mdns_configure_finalize() {
    mdns_clear();
    mdns_add_facility(mdns_instance_name, "_hap", mdns_txt_rec, mdns_TCP, mdns_port, MDNS_TTL);

    printf("mDNS announcement: Name=%s %s Port=%d TTL=%d\n",
           mdns_instance_name, mdns_txt_rec, mdns_port, MDNS_TTL);
}

#endif

#ifdef ESP_IDF

#include <string.h>
#include <mdns.h>

uint32_t homekit_random() {
    return esp_random();
}

void homekit_random_fill(uint8_t *data, size_t size) {
    uint32_t x;
    for (int i=0; i<size; i+=sizeof(x)) {
        x = esp_random();
        memcpy(data+i, &x, (size-i >= sizeof(x)) ? sizeof(x) : size-i);
    }
}

void homekit_system_restart() {
    esp_restart();
}

void homekit_overclock_start() {
}

void homekit_overclock_end() {
}

void homekit_mdns_init() {
    mdns_init();
}

void homekit_mdns_configure_init(const char *instance_name, int port) {
    mdns_hostname_set(instance_name);
    mdns_instance_name_set(instance_name);
    mdns_service_add(instance_name, "_hap", "_tcp", port, NULL, 0);
}

void homekit_mdns_add_txt(const char *key, const char *format, ...) {
    va_list arg_ptr;
    va_start(arg_ptr, format);

    char value[128];
    int value_len = vsnprintf(value, sizeof(value), format, arg_ptr);

    va_end(arg_ptr);

    if (value_len && value_len < sizeof(value)-1) {
        mdns_service_txt_item_set("_hap", "_tcp", key, value);
    }
}

void homekit_mdns_configure_finalize() {
    /*
    printf("mDNS announcement: Name=%s %s Port=%d TTL=%d\n",
           name->value.string_value, txt_rec, PORT, 0);
    */
}

#endif

uint32_t homekit_random() {
    return random();
}

void homekit_random_fill(uint8_t *data, size_t size) {
    uint32_t x;
    for (int i=0; i<size; i+=sizeof(x)) {
        x = random();
        memcpy(data+i, &x, (size-i >= sizeof(x)) ? sizeof(x) : size-i);
    }
}

void homekit_system_restart() {
}

void homekit_mdns_configure_init(const char *instance_name, int port) {
}

void homekit_mdns_add_txt(const char *key, const char *format, ...) {
}
void homekit_mdns_configure_finalize() {
}
void homekit_mdns_init() {
}

#define SPIFLASH_FILE    "spiflash"

int spiflash_read(int addr, void *buf, size_t size)
{
    int ret, fd;
    fd = open(SPIFLASH_FILE, O_RDWR|O_CREAT|O_APPEND, 0644);
    if (fd == -1) {
        printf("open %s failed!\n", SPIFLASH_FILE);
        return -1;
    }
    lseek(fd, addr, SEEK_SET);
    ret = read(fd, buf, size);
    close(fd);
    return ret;
}

int spiflash_write(int addr, void *buf, size_t size)
{
    int ret, fd;
    fd = open(SPIFLASH_FILE, O_RDWR|O_CREAT|O_APPEND, 0644);
    if (fd == -1) {
        printf("open %s failed!\n", SPIFLASH_FILE);
        return -1;
    }
    lseek(fd, addr, SEEK_SET);
    ret = write(fd, buf, size);
    close(fd);
    return ret;
}

int spiflash_erase_sector(int addr)
{
    int ret, fd;
    struct stat buf;
    void *zero;
    if (stat(SPIFLASH_FILE, &buf) < 0) {
        return -1;
    }
    fd = open(SPIFLASH_FILE, O_RDWR|O_CREAT|O_APPEND, 0644);
    if (fd == -1) {
        printf("open %s failed!\n", SPIFLASH_FILE);
        return -1;
    }
    zero = calloc(1, buf.st_size);
    ret = write(fd, zero, buf.st_size);
    close(fd);
    free(zero);
    return ret;
}
