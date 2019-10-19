#pragma once

#include <stdint.h>

uint32_t homekit_random();
void homekit_random_fill(uint8_t *data, size_t size);

void homekit_system_restart();
void homekit_overclock_start();
void homekit_overclock_end();

#ifdef ESP_OPEN_RTOS
#include <spiflash.h>
#define ESP_OK 0
#endif

#ifdef ESP_IDF
#include <esp_system.h>
#include <esp_spi_flash.h>
#define SPI_FLASH_SECTOR_SIZE SPI_FLASH_SEC_SIZE
#define spiflash_read(addr, buffer, size) (spi_flash_read((addr), (buffer), (size)) == ESP_OK)
#define spiflash_write(addr, data, size) (spi_flash_write((addr), (data), (size)) == ESP_OK)
#define spiflash_erase_sector(addr) (spi_flash_erase_sector((addr) / SPI_FLASH_SECTOR_SIZE) == ESP_OK)
#endif

#define SPI_FLASH_SEC_SIZE  4096    /**< SPI Flash sector size */
#define SPI_FLASH_SECTOR_SIZE SPI_FLASH_SEC_SIZE


#ifdef ESP_IDF
#define SERVER_TASK_STACK 12288
#else
#define SERVER_TASK_STACK 2048
#endif


void homekit_mdns_init();
void homekit_mdns_configure_init(const char *instance_name, int port);
void homekit_mdns_add_txt(const char *key, const char *format, ...);
void homekit_mdns_configure_finalize();
