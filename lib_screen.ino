void scrInit() {
  SPI.begin(LCD_SCLK, -1, LCD_MOSI, LCD_CS);
  delay(100);
  
  // // ST7735
  // display.initR(INITR_144GREENTAB);
  // display.setRotation(0);

  // For ST7789, use init(width, height)
  display.init(240, 320);   // adjust resolution (240x240, 240x320, etc.)
  display.setRotation(0);   // optional, set orientation
  display.invertDisplay(false);   // or false
  
  tft.begin(display);

  scrClear();
     
  tft.setFontMode(1);          
  tft.setFontDirection(0);   
  tft.setBackgroundColor(ST77XX_WHITE);        
  tft.setForegroundColor(ST77XX_BLACK);
  tft.setFont(u8g2_font_unifont_t_vietnamese1);  
}

void scrLoop() {
  long long now = millis();

  // ================== KHÔNG CÓ TIN NHẮN ==================
  if (!TFT_MESSAGE_AVAILABLE) return;

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
  display.fillRect(0, TFT_TEXT_START_Y, SCREEN_W, SCREEN_H, ST77XX_WHITE);

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

void scrClear() {
  display.fillScreen(ST77XX_WHITE); 
}

void scrDrawIcon(const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h, const uint8_t* icon, const uint16_t color) {
  display.fillRect(0, 0, SCREEN_W, 30, ST77XX_WHITE);
  display.drawXBitmap(x, y, icon, w, h, color);
}

void scrDrawMessageFixed(const uint16_t x, const uint16_t y, const char* msg)
{ 
  scrxListening = false;
  display.fillRect(0, y, SCREEN_W, SCREEN_H, ST77XX_WHITE);
  tft.setCursor(x, y + LINE_HEIGHT);
  tft.print(msg);
}

void scrDrawMessage(const uint16_t x, const uint16_t y, const char* msg, int repeatTimes)
{
  scrxListening = false;
  scrPrepareTftPages(msg, y);

  TFT_MESSAGE_AVAILABLE = true;
  TFT_DISPLAY_PAGE = 0;
  TFT_DISPLAY_PAGE_SWITCH = millis()-PAGE_INTERVAL;
  TFT_REPEAT_TIMES = (repeatTimes < 1) ? 1: repeatTimes;
  TFT_REPEAT_TIMES_VALUE = 1;
  TFT_TEXT_START_X = x;
  TFT_TEXT_START_Y = y;

  // ... loop will handle
}

void scrShowStatus() {
  scrClear();
  // display.fillRect(64, 0, 1, 128, ST77XX_BLACK);
  // display.fillRect(0, 64, 128, 1, ST77XX_BLACK);

  // // Ánh sáng, nhiệt, ẩm, trạng thái
  // display.drawXBitmap(20, 8, epd_bitmap_icosLight0, 24, 24, ST77XX_YELLOW);
  // if (ssLightAo >= 3800) 
  // {
  //   display.fillRect(0, 0, 63, 63, ST77XX_WHITE);
  //   display.drawXBitmap(20, 8, epd_bitmap_icosLight1, 24, 24, ST77XX_BLACK);
  // }

  // display.drawXBitmap(84, 8, epd_bitmap_icosTemp, 24, 24, ST77XX_RED);
  // display.drawXBitmap(20, 72, epd_bitmap_icosHumid, 24, 24, ST77XX_BLUE);
  
  // display.drawXBitmap(84, 72, epd_bitmap_icoSmile, 24, 24, ST77XX_GREEN);
  // tft.setCursor(68, 116);
  // tft.print("Vui vẻ");

  // if (IGNORE_STATUS) 
  // { display.fillRect(65, 65, 128, 128, ST77XX_WHITE);
  //   display.drawXBitmap(84, 72, epd_bitmap_icoSad, 24, 24, ST77XX_ORANGE);
  //   tft.setCursor(68, 116);
  //   tft.print("Giúp!");
  // }

  // tft.setCursor(4, 52);
  // tft.print(curLightAo);
  // tft.setCursor(4, 116);
  // tft.print(curHumidity);
  // tft.setCursor(68, 52);
  // tft.print(curTemperature);

  tft.setCursor(4, 20);
  tft.println("Trạng thái cây xanh của bạn");
  tft.print("Thời gian: ");
  tft.println(sysGetDateTimeString());
  tft.print("Ánh sáng (lux): ");
  tft.println(curLightAo);
  tft.print("Nhiệt độ: ");
  tft.println(curTemperature);
  tft.print("Độ ẩm (%RH): ");
  tft.println(curHumidity);
  tft.print("Đất ẩm? ");
  tft.println(SOIL_VALUE ? "Khô" : "Ẩm"); // ẩm 0 khô 1
  tft.print("Lần tưới gần nhất (m): ");
  tft.println((float)(millis() - WATER_LAST_TM)/(60000.0f));
  tft.print("Lần tưới gần nhất (h): ");
  tft.println((float)(millis() - WATER_LAST_TM)/(3600000.0f));
  tft.print("Thời gian phơi sáng (m): ");
  tft.println( (float)SUN_TIME / 60000.0f  );
  tft.print("Thời gian phơi sáng (h): ");
  tft.println( (float)SUN_TIME / 3600000.0f );
  tft.print("Trạng thái cây: ");
  tft.print(IGNORE_STATUS ? "Giúp!" : "Vui vẻ, bình thường");
  tft.print(" (");
  tft.print(IGNORE_TIMES);
  tft.println(" lần)");
}

void scrListening() {
  if (scrxListening) return;

  scrxListening = true;
  scrDrawIcon(ICO_START_X, ICO_START_Y, ICO_ACT_DIMM, ICO_ACT_DIMM, epd_bitmap_icoMic, ST77XX_RED);  
  display.fillRect(0, 28, 128, 128, ST77XX_WHITE);
  tft.setCursor(MSG_START_X, MSG_START_Y + LINE_HEIGHT);
  tft.print("Đang nghe...");
}

void scrStartUp() {   
  scrClear();
  display.drawXBitmap(0, 32, epd_bitmap_lgNovaEsp, 128, 48, ST77XX_GREEN);
 
  tft.setCursor(20, 96);
  tft.print("Đang tải...");
}