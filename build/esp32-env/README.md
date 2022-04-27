# esp32 support

As embeded IOT library, gear-lib support build and run on esp32 board

you can develop IOT product, like Apple Homekit IP camera.

Based on [esp-homekit](https://github.com/maximkulkin/esp-homekit).

## Configuration

Before compiling, you need to alter several settings in menuconfig (`make
menuconfig`):
* Partition Table
    * Partition Table = **Custom partition table CSV**
    * Custom partition CSV file = **partitions.csv**
* Component config
    * ESP32-specific
        * Support for external, SPI-connected RAM = **check**
        * SPI RAM config
            * Initialize SPI RAM when booting the ESP32 = **check**
            * SPI RAM access method = **Make RAM allocatable using malloc() as well**
    * Gear Lib
        * Using libposix
        * Using libavcap
        * Using libmedia-io
        * Using librtmpc
      ....
make erase_flash
