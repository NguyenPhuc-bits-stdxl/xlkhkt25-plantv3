#include <Adafruit_ST7735.h>
#include <U8g2_for_Adafruit_GFX.h>

// 'icoSetup', 24x24px
const unsigned char epd_bitmap_icoSetup [] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x66, 0x00, 0x70, 
	0xe7, 0x0e, 0xf8, 0xe7, 0x1f, 0xd8, 0x81, 0x1b, 0x1c, 0x00, 0x38, 0x3c, 0x3c, 0x3c, 0x78, 0x7e, 
	0x1e, 0x70, 0x7e, 0x0e, 0x70, 0x7e, 0x0e, 0x78, 0x7e, 0x1e, 0x3c, 0x3c, 0x3c, 0x1c, 0x00, 0x38, 
	0xd8, 0x81, 0x1b, 0xf8, 0xe7, 0x1f, 0x70, 0xe7, 0x0e, 0x00, 0x66, 0x00, 0x00, 0x7e, 0x00, 0x00, 
	0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

#define LCD_CS 10
#define LCD_DC 11
#define LCD_SCLK 14
#define LCD_MOSI 13
#define LCD_RST 12

Adafruit_ST7735 display = Adafruit_ST7735(LCD_CS, LCD_DC, LCD_RST);
U8G2_FOR_ADAFRUIT_GFX tft;

#define CAM_BIEN_INTERVAL 5000 // 5s update màn hình chờ (thông số cảm biến)
long long CAM_BIEN_LAST_CHECKED;

#define MAX_TFT_PAGES 10
#define PAGE_INTERVAL 2000

String TFT_PAGES[MAX_TFT_PAGES];
uint8_t TFT_PAGES_LEN = 0;

uint8_t TFT_TEXT_START_Y = 2;
uint8_t TFT_TEXT_START_X = 2;
const uint8_t LINE_HEIGHT = 12;
const uint8_t TEXT_W = 124;
const uint8_t SCREEN_H = 128;

bool TFT_MESSAGE_AVAILABLE = false;
int TFT_REPEAT_TIMES_VALUE = 1;
int TFT_REPEAT_TIMES = 1;

uint8_t TFT_DISPLAY_PAGE = 0;
unsigned long TFT_DISPLAY_PAGE_SWITCH = 0;
bool TFT_WAIT_LAST_PAGE = false;

void setup() {
  Serial.begin(115200); 
  Serial.setTimeout(100);
  Serial.println("SYS Serial initialized!");

  SPI.begin(LCD_SCLK, -1, LCD_MOSI, LCD_CS);
  delay(100);
  display.initR(INITR_144GREENTAB);
  display.setRotation(0);
  tft.begin(display);

  scrClear();
     
  tft.setFontMode(1);          
  tft.setFontDirection(0);   
  tft.setBackgroundColor(ST77XX_WHITE);        
  tft.setForegroundColor(ST77XX_BLACK);
  tft.setFont(u8g2_font_unifont_t_vietnamese1);  
  Serial.println("TFT Init done!");

  tft.setCursor(2, 18);
  tft.print("Xin chào");
  delay(5000);
  
  scrDrawIcon(52, 2, 24, 24, epd_bitmap_icoSetup, ST77XX_GREEN);
  scrDrawMessage(2, 54,
  "Đây là tin nhắn cấu hình hệ thống, kiểm thử màn hình ST7735, đây sẽ là một tin nhắn khá dài. Chắc là sẽ viết thêm vài từ nữa cho nhiều vào. OK, dừng ở đây nhé!",
  2);
}

void loop() {
  scrLoop();
}

void scrDrawIcon(const uint8_t x, const uint8_t y, const uint8_t w, const uint8_t h, const uint8_t* icon, const uint16_t color) {
  display.drawXBitmap(x, y, icon, w, h, color);
}

void scrLoop()
{
  unsigned long now = millis();

  // ================== KHÔNG CÓ TIN NHẮN ==================
  if (!TFT_MESSAGE_AVAILABLE) {
    if (now - CAM_BIEN_LAST_CHECKED < CAM_BIEN_INTERVAL)
      return;

    scrShowStatus();
    CAM_BIEN_LAST_CHECKED = now;
    return;
  }

  // ================== ĐANG CHỜ GIỮ TRANG CUỐI ==================
  if (TFT_WAIT_LAST_PAGE) {
    if (now - TFT_DISPLAY_PAGE_SWITCH >= PAGE_INTERVAL) {
      // Đã giữ trang cuối đủ 2s → thoát
      TFT_WAIT_LAST_PAGE = false;
      TFT_MESSAGE_AVAILABLE = false;
    }
    return; // QUAN TRỌNG: không làm gì khác
  }

  // ================== CHƯA TỚI LÚC LẬT TRANG ==================
  if (now - TFT_DISPLAY_PAGE_SWITCH < PAGE_INTERVAL)
    return;

  // ================== HIỂN THỊ PAGE ==================
  display.fillRect(0, TFT_TEXT_START_Y - LINE_HEIGHT, 128, 128, ST77XX_WHITE);

  tft.setCursor(TFT_TEXT_START_X, TFT_TEXT_START_Y + LINE_HEIGHT);
  tft.print(TFT_PAGES[TFT_DISPLAY_PAGE]);

  TFT_DISPLAY_PAGE_SWITCH = now;
  TFT_DISPLAY_PAGE++;

  // ================== CHECK HẾT PAGE ==================
  if (TFT_DISPLAY_PAGE >= TFT_PAGES_LEN) {

    TFT_REPEAT_TIMES_VALUE++;

    // Còn vòng lặp → quay lại page 0
    if (TFT_REPEAT_TIMES_VALUE <= TFT_REPEAT_TIMES) {
      TFT_DISPLAY_PAGE = 0;
      return;
    }

    // LẦN CUỐI → giữ trang cuối
    TFT_WAIT_LAST_PAGE = true;
    // KHÔNG set TFT_MESSAGE_AVAILABLE = false ở đây
    // để trang cuối sống đủ PAGE_INTERVAL
  }
}

void scrDrawMessage(const uint8_t x, const uint8_t y, const char* msg, int repeatTimes)
{
  prepareTftPages(msg, y);

  TFT_MESSAGE_AVAILABLE = true;
  TFT_DISPLAY_PAGE = 0;
  TFT_DISPLAY_PAGE_SWITCH = millis()-PAGE_INTERVAL;
  TFT_REPEAT_TIMES = (repeatTimes < 1) ? 1: repeatTimes;
  TFT_REPEAT_TIMES_VALUE = 1;
  TFT_TEXT_START_X = x;
  TFT_TEXT_START_Y = y;

  // ... loop will handle
}

void scrClear() {
  display.fillScreen(ST77XX_WHITE); 
}

void scrShowStatus() {
  // Chia màn hình làm 4 ô, hiển thị các thông số
  // To do:
  // - Vẽ 2 đường thẳng (x=64 và y=64)
  // - Ghi số lên (bằng font u8g2)
  // - Vẽ icon (nhiệt đỏ, độ ẩm xanh dương, ánh sáng vàng, pin xanh lá) 24x24px
  scrClear();
  tft.setCursor(2, 14);
  tft.println("READY");
  tft.println(millis());
}

void scrStartUp() {   
  scrClear();
  //scrDrawIcon(48, 16, ICO_BRAND_DIMM, ICO_BRAND_DIMM, epd_bitmap_icoBranding32, ST77XX_GREEN);
  tft.setCursor(32, 96);
  tft.print("NOVA");
}