# TS80player

Audio player for the TS80 soldering iron

## Video

[![](http://img.youtube.com/vi/aose7zWV1fM/0.jpg)](https://youtu.be/aose7zWV1fM)

## Player

* Example: https://github.com/OBauck/stm32_wav_player
* TIM1 works as DMA at 32000Hz (mono) 16-bit samples a second,
* TIM2 fires every time when buffer is empty.
* LD2 pin - analog audio

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
Upload [TS80-Bootloader.hex](https://github.com/Ralim/ts100/blob/master/Development%20Resources/TS80-Bootloader.hex) to the Bluepill the usual way, via e.g. UART.
Connect B1 to GND for the USB Mass Storage bootloader, use pins B6/B7 for I2C display.
You can upload your own firmware in hex format (use 0x08004000 offset and 0x4000 start vector) and file extension will change to .RDY.
You can press reset instead of unplugging USB each time.
Stock TS80 firmware works as well.

![](https://i.imgur.com/KVUJhPQ.jpg)


## Building

To build that you need [STM32CubeF1](https://github.com/STMicroelectronics/STM32CubeF1). Put path to the unpacked archive into your Makefile. Then just run make.

```
FIRMWARE = /mnt/c/SDK/STM32CubeF1
```

## Pictures

* [Headphones attached](https://i.imgur.com/UsYAqPm.jpg)
* [Example setup with PWM display](https://i.imgur.com/RBE9rfq.jpg)
* [TS80 tip pinout](https://i.imgur.com/17CBbJx.jpg)
* [TS100 schematic](https://i.imgur.com/3bzm3LW.jpg)

## References
* https://imgur.com/a/xO4b91r (Picture gallery)
* https://github.com/Ralim/ts100 (TS100/TS80 firmware)
* https://github.com/Ralim/ts100/wiki/TS-80 (TS80 wiki)
* https://github.com/joric/ts100tris
* https://github.com/OBauck/stm32_wav_player
* https://github.com/rogerclarkmelbourne/STM32duino-bootloader/tree/master/binaries
* http://www.avislab.com/blog/stm32-bootloader_ru/
* https://github.com/avislab/STM32F103/tree/master/Example_Bootloader/Example_Bootloader/Debug/bin
* https://github.com/afiskon/stm32-ssd1306
* https://github.com/STMicroelectronics/STM32CubeF1

