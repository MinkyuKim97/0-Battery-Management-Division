# MPU Setting
## ESP32 - 3.5 inch LVGL
### Board Info
https://www.lcdwiki.com/3.5inch_ESP32-32E_Display

### Library  Manager

1. Arduino_GFX_Library by moon on our nation
2. XPT2046_TouchScreen by Paul Stoffregen
3. U8g2 by oliver

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

### LVGL, Arduino GFX Library Font setting
[https://docs.rs/u8g2-fonts/latest/u8g2_fonts/fonts/index.html]

ex) gfx->setFont(u8g2_font_ncenB14_tr);
