# TS80player

Audio player for the TS80 soldering iron

## Video

[![](http://img.youtube.com/vi/aose7zWV1fM/0.jpg)](https://youtu.be/aose7zWV1fM)

## Installation

Download .hex file in the [releases section](https://github.com/joric/ts80player/releases).

Connect to USB while holding button A, copy .hex to USB drive, wait it renames to .rdy, unplug USB.

There's no firmware backup, just revert to the official firmware the same way.

## Hardware

* STM32F103T8U6 (ARM Cortex M3, clock frequency 72 MHz, 20K RAM, 64K FLASH)
* MMA8652FC (3-Axis, 12-bit Digital Accelerometer)
* SSD1306 (White 96x16 OLED Display)

Name    |TS100 | TS80
--------|:----:|:----:
KEY_A   |  A9  |  B1
KEY_B   |  A6  |  B0
PWM_OUT |  B4  |  A6
TIP_TEMP|  B0  |  A3
TMP36   |  A7  |  A4
OLED_RST|  A8  | A15
SCL     |  B6  |  B6
SDA     |  B7  |  B7

## Debugging

You could use a cheap Bluepill board for debugging.
Upload [TS80-Bootloader.hex](https://github.com/Ralim/ts100/blob/master/Development%20Resources/TS80-Bootloader.hex)
to the Bluepill the usual way, e.g. [via UART](https://github.com/joric/bluetosis/wiki/Uploading).
Connect B1 to GND for the USB Mass Storage bootloader, use pins B6/B7 for I2C display.
You can upload your own firmware in hex format (use 0x08004000 offset and 0x4000 start vector)
and file extension will change to .RDY.
You can press reset instead of unplugging USB each time.
Stock TS80 firmware works as well.

![](https://i.imgur.com/1jMHb4J.jpg)

## Documentation

* https://github.com/joric/ts80player/wiki

## References
* https://imgur.com/a/xO4b91r (Picture gallery)
* https://github.com/joric/ts100tris
* https://github.com/joric/es120tris
