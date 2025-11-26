#include <U8g2lib.h>
#include <Arduino_GFX_Library.h>
#include <SPI.h>
#include <XPT2046_Touchscreen.h>
#include <WiFi.h>

//Managing personal infos
#include "secrets.h"

// const char* WIFI_SSID = "YOUR_SSID";
// const char* WIFI_PASS = "YOUR_PASSWORD";

// ===== LCDWiki 핀(ESP32-32E 3.5" 보드) =====
#define TFT_CS   15
#define TFT_DC   2
#define TFT_SCK  14
#define TFT_MOSI 13
#define TFT_MISO 12
#define TFT_BL   27
#define TFT_RST  GFX_NOT_DEFINED   // EN과 reset 공유

// Touch (XPT2046)
#define TP_CS    33
#define TP_IRQ   36

// ===== 터치 캘리브레이션(초기값: 대개 동작, 안 맞으면 수정) =====
#define TS_MINX  200
#define TS_MAXX  3800
#define TS_MINY  200
#define TS_MAXY  3800

// 회전/방향 보정 스위치 (터치가 뒤집히면 여기만 바꾸면 됨)
#define TOUCH_SWAP_XY   true
#define TOUCH_INVERT_X  true
#define TOUCH_INVERT_Y  false

// 디바운스
const uint32_t debounceMs = 180;

// HSPI 사용 (이 보드의 SPI 라인에 맞춤)
SPIClass hspi(HSPI);

// GFX (ST7796 SPI)
Arduino_DataBus *bus = new Arduino_ESP32SPI(
  TFT_DC, TFT_CS,
  TFT_SCK, TFT_MOSI, TFT_MISO,
  HSPI
);
Arduino_GFX *gfx = new Arduino_ST7796(bus, TFT_RST, 0);

// Touch
XPT2046_Touchscreen ts(TP_CS, TP_IRQ);

// 색
static const uint16_t COLOR_RED   = 0xF800;
static const uint16_t COLOR_WHITE = 0xFFFF;
static const uint16_t COLOR_BLACK = 0x0000;

// UI
const char *MSG = "Hello world";
int textSize = 3;

struct Rect { int16_t x, y, w, h; };
Rect touchBox;

// 상태(토글)
bool toggled = false;

// 터치 edge 감지
bool lastTouched = false;
uint32_t lastToggleMs = 0;

// 캘리브레이션 도움(관측 min/max)
int16_t obsMinX = 4095, obsMaxX = 0, obsMinY = 4095, obsMaxY = 0;

int16_t clamp16(int16_t v, int16_t lo, int16_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

bool pointInRect(int16_t x, int16_t y, const Rect &r) {
  return (x >= r.x) && (x < (r.x + r.w)) && (y >= r.y) && (y < (r.y + r.h));
}




void computeLayout() {
  int16_t W = gfx->width();
  int16_t H = gfx->height();
  touchBox = { (int16_t)(W / 4), (int16_t)(H / 4), (int16_t)(W / 2), (int16_t)(H / 2) };
}

void drawBoxWithChrome(uint16_t fillColor) {
  // 그림자(아래/오른쪽으로 살짝)
  gfx->fillRect(touchBox.x + 6, touchBox.y + 6, touchBox.w, touchBox.h, 0x2104); // 짙은 회색(대충)
  // 본체
  gfx->fillRect(touchBox.x, touchBox.y, touchBox.w, touchBox.h, fillColor);
  // 테두리(대비색)
  uint16_t border = (fillColor == COLOR_RED) ? COLOR_WHITE : COLOR_RED;
  gfx->drawRect(touchBox.x, touchBox.y, touchBox.w, touchBox.h, border);
  gfx->drawRect(touchBox.x + 1, touchBox.y + 1, touchBox.w - 2, touchBox.h - 2, border);
}

void drawCenteredText(uint16_t textColor) {
  // gfx->setTextColor(textColor, COLOR_BLACK);
  gfx->setTextColor(textColor);

  gfx->setFont(u8g2_font_helvB14_te);
  // gfx->setUTF8Print(true);           // UTF-8 쓰면 켜기

  int16_t x1, y1;
  uint16_t w, h;

  // 중앙정렬: 가능하면 getTextBounds 사용 (라이브러리 버전에 따라 제공)
  gfx->getTextBounds(MSG, 0, 0, &x1, &y1, &w, &h);

  int16_t tx = (gfx->width()  - (int16_t)w) / 2 - x1;
  int16_t ty = (gfx->height() - (int16_t)h) / 2 - y1;

  gfx->setCursor(tx, ty);
  gfx->print(MSG);

  gfx->setFont(); // (선택) 다시 기본 폰트로 돌리고 싶으면
}

void renderNormal() {
  computeLayout();

  uint16_t boxColor  = toggled ? COLOR_WHITE : COLOR_RED;
  uint16_t textColor = toggled ? COLOR_RED   : COLOR_WHITE;

  gfx->fillScreen(COLOR_BLACK);
  drawBoxWithChrome(boxColor);
  drawCenteredText(textColor);
}



bool mapTouchToScreen(const TS_Point &p, int16_t &sx, int16_t &sy) {
  int16_t rx = p.x;
  int16_t ry = p.y;

  // 관측값 업데이트(캘리브레이션 도움)
  obsMinX = min(obsMinX, rx); obsMaxX = max(obsMaxX, rx);
  obsMinY = min(obsMinY, ry); obsMaxY = max(obsMaxY, ry);

  // swap/invert(회전/방향 보정)
  if (TOUCH_SWAP_XY) { int16_t t = rx; rx = ry; ry = t; }
  if (TOUCH_INVERT_X) rx = (TS_MAXX + TS_MINX) - rx;
  if (TOUCH_INVERT_Y) ry = (TS_MAXY + TS_MINY) - ry;

  rx = clamp16(rx, TS_MINX, TS_MAXX);
  ry = clamp16(ry, TS_MINY, TS_MAXY);

  int16_t W = gfx->width();
  int16_t H = gfx->height();

  sx = map(rx, TS_MINX, TS_MAXX, 0, W - 1);
  sy = map(ry, TS_MINY, TS_MAXY, 0, H - 1);
  return true;
}

void printTouchDebug(const TS_Point &p, int16_t sx, int16_t sy, bool inBox) {
  Serial.printf("RAW(%d,%d) -> SCREEN(%d,%d) inBox=%d | OBS min/max X[%d..%d] Y[%d..%d]\n",
                p.x, p.y, sx, sy, inBox ? 1 : 0,
                obsMinX, obsMaxX, obsMinY, obsMaxY);
}
// void connectWiFi() {
//   WiFi.mode(WIFI_STA);
//   WiFi.setSleep(false);
//   WiFi.begin(WIFI_SSID, WIFI_PASS);

//   Serial.print("WiFi connecting");
//   int tries = 0;
//   while (WiFi.status() != WL_CONNECTED && tries < 60) {
//     delay(500);
//     Serial.print(".");
//     tries++;
//   }
//   Serial.println();

//   if (WiFi.status() == WL_CONNECTED) {
//     Serial.print("WiFi connected! IP: ");
//     Serial.println(WiFi.localIP());
//   } else {
//     Serial.println("WiFi connect FAILED");
//   }
// }
void connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
}

String wifiLine() {
  wl_status_t st = WiFi.status();
  if (st == WL_CONNECTED) {
    return "WiFi:OK " + WiFi.localIP().toString() + " RSSI:" + String(WiFi.RSSI());
  }
  if (st == WL_IDLE_STATUS) return "WiFi:IDLE";
  if (st == WL_NO_SSID_AVAIL) return "WiFi:NO_SSID";
  if (st == WL_CONNECT_FAILED) return "WiFi:FAIL";
  if (st == WL_DISCONNECTED) return "WiFi:DISC";
  return "WiFi:UNK";
}

void drawWifiStatusBar(bool force = false) {
  static String last = "";
  static uint32_t lastMs = 0;

  // 너무 자주 갱신하지 않기(깜빡임 방지)
  if (!force && (millis() - lastMs) < 500) return;
  lastMs = millis();

  String s = wifiLine();
  if (!force && s == last) return;
  last = s;

  // 상단 바 영역만 지우고 다시 그리기
  const int barH = 24;
  gfx->fillRect(0, 0, gfx->width(), barH, COLOR_BLACK);

  // 폰트(원하면 U8g2 폰트 사용 가능)
  gfx->setFont(u8g2_font_6x10_tr);   // 작고 가독성 좋음
  gfx->setTextSize(1);
  gfx->setTextColor(COLOR_WHITE);    // 배경 투명 (이미 bar를 검정으로 지웠음)
  gfx->setCursor(4, 16);             // y는 폰트 baseline 느낌이라 16 정도가 무난
  gfx->print(s);

  gfx->setFont(); // (선택) 기본 폰트로 복귀
}

void setup() {
  Serial.begin(115200);
  delay(200);
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH); // 백라이트 ON

  gfx->begin(40000000);
  gfx->setRotation(3); // 90도 회전 -> landscape

  hspi.begin(TFT_SCK, TFT_MISO, TFT_MOSI);
  ts.begin(hspi);
  // ts.setRotation(0); // 여기서는 우리 코드에서 mapping 처리하므로 기본 0으로 두는 게 안전

  renderNormal();
  connectWiFi();
  drawWifiStatusBar(true);  // 처음 1회 강제 출력

  Serial.println("Ready. Touch the CENTER box to toggle. Watch RAW/SCREEN coords for calibration.");
}

void loop() {
drawWifiStatusBar(); // 주기적으로 상태 표시 업데이트

if (WiFi.status() != WL_CONNECTED) {
  static uint32_t lastTry = 0;
  if (millis() - lastTry > 3000) {   // 3초마다 재시도
    lastTry = millis();
    connectWiFi();
    drawWifiStatusBar(true);
  }
}  bool touched = ts.touched();

  if (touched && !lastTouched && (millis() - lastToggleMs) > debounceMs) {
    TS_Point p = ts.getPoint();
    int16_t sx, sy;
    mapTouchToScreen(p, sx, sy);

    computeLayout();
    bool inBox = pointInRect(sx, sy, touchBox);
    printTouchDebug(p, sx, sy, inBox);

    if (inBox) {
      toggled = !toggled;   // ✅ 토글
      renderNormal();       // ✅ 최종 렌더
      drawWifiStatusBar(true);
    }

    lastToggleMs = millis();
  }

  lastTouched = touched;
  delay(10);
}