#include <Arduino_GFX_Library.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>

// LCD (ST7796U, 4-line SPI)
#define TFT_CS   15
#define TFT_DC   2
#define TFT_SCK  14
#define TFT_MOSI 13
#define TFT_MISO 12
#define TFT_BL   27
#define TFT_RST  GFX_NOT_DEFINED

// Touch (XPT2046, SPI )
#define TP_CS    33
#define TP_IRQ   36   // 터치 시 LOW 들어오는 인터럽트 핀(입력 전용 핀)  [oai_citation:1‡LCD Wiki](https://www.lcdwiki.com/3.5inch_ESP32-32E_Display)

// 색(565)
static const uint16_t COLOR_RED   = 0xF800;
static const uint16_t COLOR_BLACK = 0x0000;

// HSPI를 명시적으로 사용 (이 보드의 LCD/Touch SPI가 14/13/12 라인에 붙어있음)  [oai_citation:2‡LCD Wiki](https://www.lcdwiki.com/3.5inch_ESP32-32E_Display)
SPIClass hspi(HSPI);

// Arduino_GFX (ESP32 SPI bus → ST7796)
Arduino_DataBus *bus = new Arduino_ESP32SPI(
  TFT_DC /* DC */, TFT_CS /* CS */,
  TFT_SCK /* SCK */, TFT_MOSI /* MOSI */, TFT_MISO /* MISO */,
  HSPI /* spi_num */
); //  [oai_citation:3‡GitHub](https://github.com/moononournation/Arduino_GFX/wiki/Dev-Device-Declaration)
Arduino_GFX *gfx = new Arduino_ST7796(bus, TFT_RST, 0 /* rotation */); //  [oai_citation:4‡GitHub](https://github.com/moononournation/Arduino_GFX/wiki/Dev-Device-Declaration)

// Touch
XPT2046_Touchscreen ts(TP_CS, TP_IRQ);

bool isRed = true;
bool lastTouched = false;
uint32_t lastToggleMs = 0;
const uint32_t debounceMs = 150;

void drawSolid()
{
  gfx->fillScreen(isRed ? COLOR_RED : COLOR_BLACK);
}

void setup()
{
  // 백라이트 ON (HIGH가 켜짐)  [oai_citation:5‡LCD Wiki](https://www.lcdwiki.com/3.5inch_ESP32-32E_Display)
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // LCD 시작
  gfx->begin(40000000); // 40MHz
  gfx->setRotation(0);
  drawSolid();

  // Touch 시작: XPT2046 라이브러리는 begin(SPIClass&) 형태로 SPI를 넘길 수 있음  [oai_citation:6‡Arduino Forum](https://forum.arduino.cc/t/xpt2046-touchscreen-esp32-spi-issue-solved/1155074?utm_source=chatgpt.com)
  hspi.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  ts.begin(hspi);
}

void loop()
{
  bool touched = ts.touched();

  // "새로 눌린 순간"만 감지해서 토글 + 디바운스
  if (touched && !lastTouched && (millis() - lastToggleMs) > debounceMs)
  {
    isRed = !isRed;
    drawSolid();
    lastToggleMs = millis();
  }

  lastTouched = touched;
  delay(10);
}