# About

An open-source reimplementation of the MCU system module.

The MCU module originally ships with the MCU firmware and upgrades the MCU firmware if it detects that it is out of date. The original proprietary Nintendo MCU firmware is not included in this repository.

In the future, I may try to reimplement the firmware. Until then, this sysmodule will be built with firmware upgrades disabled by default.

**IF YOU USE THE FIRMWARE UPGRADE FEATURE, YOU USE IT AT YOUR OWN RISK. UPLOADING INVALID FIRMWARE TO THE MCU CAN CAUSE A BRICK THAT CANNOT BE RECOVERED FROM BY SIMPLE MEANS (INCLUDING NTRBOOT). UNLESS YOU ARE PREPARED TO SOLDER WIRES TO YOUR CONSOLE'S MOTHERBOARD TO FLASH THE MCU FIRMWARE USING EXTERNAL HARDWARE IN THE EVENT OF A BRICK, DO NOT USE THE FIRMWARE UPLOAD FEATURE.**

With all of that out of the way, it is possible to enable firmware uploads by configuring using the environment variables listed below. Place your MCU firmware in the same directory as the Makefile and name it `mcu_firm.bin`. **This firmware blob must be exactly 0x4000 bytes in size.**

# Compiling

Requirements:
- Latest version of devkitPro/devkitARM
- makerom on PATH

The following environment variables are available to configure the build process:

| Variable             | Description                                                                                                                                                                       |
|----------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `DEBUG`              | When set, all optimization is disabled and debug symbols are included in the output ELF. When not set, the ELF will be optimized for size and will not include any debug symbols. |
| `DEBUG_PRINTS`       | Debug messages (IPC logs) will be shown in the GDB console when attached.                                                                                                         |
| `N3DS`               | Build the New3DS-specific variation of the MCU module (with the New3DS bit set in the title ID).                                                                                  |
| `ENABLE_FIRM_UPLOAD` | Enables MCU firmware upgrades. **USE WITH CAUTION. PLEASE READ THE ABOVE.**                                                                                                       |
| `MCU_FIRM_VER_HIGH`  | When building with firmware upgrade support, specifies the major version of the MCU firmware blob. **USE WITH CAUTION. PLEASE READ THE ABOVE.**                                   |
| `MCU_FIRM_VER_LOW`   | When building with firmware upgrade support, specifies the minor version of the MCU firmware blob. **USE WITH CAUTION. PLEASE READ THE ABOVE.**                                   |

# Licensing

The project itself is using the Unlicense.

Parts of [libctru](https://github.com/devkitPro/libctru) were taken and modified (function argument order, names, etc.). The license for libctru can be seen here: [LICENSE.libctru.md](/LICENSE.libctru.md)
