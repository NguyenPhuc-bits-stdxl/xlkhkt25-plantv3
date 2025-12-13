void scrInit() {
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
}

void scrDrawIcon(const uint8_t x, const uint8_t y, const uint8_t w, const uint8_t h, const uint8_t* icon, const uint16_t color) {
  // tft.setForegroundColor(color);
  // tft.drawXBMP(x, y, w, h, icon);
}

void scrDrawMessage(const uint8_t x, const uint8_t y, const char* msg, bool doNotClearAfterDisplayEnds, bool repeatMessageOnLoop)
{
  /* Logic flow
    1. Chuẩn bị (trong lib_screen)
       Cắt trang và chuẩn bị sẵn tất cả vào array TFT_PAGES [const char*]
       TFT_MESSAGE_AVAILABLE = true;
       LOOP_DISPLAY_PAGE = 0;
       LOOP_DISPLAY_PAGE_SWITCH = milis()-9999; (Trigger chiếu page 0 khi vào loop)
       
    2. Trong LOOP
       Liên tục check [milis()-LOOP_DISPLAY_PAGE_SWITCH >= 2000] và [TFT_MESSAGE_AVAILABLE]
       > Có: 
             Clear khu vực thông báo
             tft.SetCursor(2, y);
             tft.print(TFT_PAGES[LOOP_DISPLAY_PAGE]);
             LOOP_DISPLAY_PAGE++;
             LOOP_DISPLAY_PAGE_SWITCH = milis();
             Check xem [LOOP_DISPLAY_PAGES >= TFT_PAGES.len()]
             > Có:
                   Có bật RMOL không?
                   > Có: LOOP_DISPLAY_PAGE = 0;
                   > Không:
                     Dừng tin nhắn vì chiếu hết => TFT_MESSAGE_AVAILABLE = false;
                     Quay về màn hình chờ => scrShowStatus()
       > Không:
                Chiếu hết chưa?
                Có > Kiểm tra điều kiện [DNCADE]
                     Có > Xóa màn       
  */

  tft.setForegroundColor(ST77XX_BLACK);
  tft.setCursor(x, y);
  tft.println(msg);
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
}

void scrStartUp() {   
  scrClear();
  scrDrawIcon(48, 16, ICO_BRAND_DIMM, ICO_BRAND_DIMM, epd_bitmap_icoBranding32, ST77XX_GREEN);
  tft.setCursor(32, 96);
  tft.print("NOVA");
}