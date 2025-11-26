# MPU Setting
## ESP32 - 3.5 inch LVGL
### Board Info
https://www.lcdwiki.com/3.5inch_ESP32-32E_Display

### Library  Manager

1. Arduino_GFX_Library by moon on our nation
2. XPT2046_TouchScreen by Paul Stoffregen

### Board Manager

1. ESP32 by Espressif Systems

### Arduino IDE upload setting

1. Tool -> Board -> 'ESP32 Dev Module'
2. Flash Size: 4mb
3. PSRAM: Disabled
4. Upload Speed: 115200 (921600 might break the upload)
5. Flash Mode: DIO
6. Flash Frequency: 40MHz
7. Partition Scheme: Default

[Press 'BOOT' on the board after press upload on IDE, like normal way to upload on ESP32]

