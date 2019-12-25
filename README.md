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

## Player

The player code is based on the [stm32 wav player](https://github.com/OBauck/stm32_wav_player) by [Ole Bauck](https://github.com/OBauck).
TIM1 works as DMA at 32000Hz (mono) 16-bit samples a second.
TIM2 supposedly fires every time when buffer is empty.
LD2 pin is the analog audio output.

The player is capable of playing high quality audio (not just beeping), I am still figuring out the details.
It only plays mono sound now, but mind that the TS80 tip only uses two outermost pins (with the K-type thermocouple in series).

## Display

The ssd1306 display code is based on the [stm32-ssd1306](https://github.com/afiskon/stm32-ssd1306) library by [Aleksander Alekseev](https://github.com/afiskon/).
The TS80 OLED display (96x16) appears to work in 128x32 mode just fine, so you can just crop the output.

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

## Building

To build that you need [STM32CubeF1](https://github.com/STMicroelectronics/STM32CubeF1). Add path to the unpacked archive to the Makefile, run make. In my case:

```
FIRMWARE = /mnt/c/SDK/STM32CubeF1
```

## References
* https://imgur.com/a/xO4b91r (Picture gallery)
* https://github.com/joric/ts100tris
* https://github.com/OBauck/stm32_wav_player
* https://github.com/afiskon/stm32-ssd1306
* https://github.com/STMicroelectronics/STM32CubeF1

