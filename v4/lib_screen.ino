void scrInit() {
  SPI.begin(LCD_SCLK, -1, LCD_MOSI, LCD_CS);
  delay(100);
  
  // // ST7735
  // display.initR(INITR_144GREENTAB);
  // display.setRotation(0);

  // For ST7789, use init(width, height)
  display.init(240, 320);   // adjust resolution (240x240, 240x320, etc.)
  display.setRotation(0);   // optional, set orientation
  display.invertDisplay(false);
  
  tft.begin(display);

  scrClear();
     
  tft.setFontMode(1);          
  tft.setFontDirection(0);   
  tft.setBackgroundColor(ST77XX_WHITE);        
  tft.setForegroundColor(ST77XX_BLACK);
  tft.setFont(u8g2_font_unifont_t_vietnamese1);  

  scrClear();

  // Vẽ màn hình khởi động
  display.drawXBitmap(56, 32, epd_bitmap_lgNovaEsp, 128, 48, ST77XX_GREEN); 
  scrDrawMessageFixed(0, 96, "  Đang khởi động hệ thống...");
  delay(50);
}

void scrClear() {
  display.fillScreen(ST77XX_WHITE); 
}

void scrDrawIcon(const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h, const uint8_t* icon, const uint16_t color) {
  display.fillRect(x, y, w, h, ST77XX_WHITE);
  display.drawXBitmap(x, y, icon, w, h, color);
}

void scrDrawMessageFixed(const uint16_t x, const uint16_t y, String msg)
{ 
  tft.setCursor(x, y + SCR_LINE_HEIGHT);
  tft.print(msg);
}

void scrDrawHomeScreen()
{
  uint16_t x = 8;  
  uint16_t y = 32;

  tft.setCursor(0, 92);
  display.fillRect(0, 20, SCR_W, 240, ST77XX_WHITE);
  display.drawXBitmap(108, 32, NV_OKAY ? epd_bitmap_icoSmile : epd_bitmap_icoStressed , 24, 24, NV_OKAY ? ST77XX_GREEN : ST77XX_RED); 

  tft.print("  Nhiệt độ: ");
  tft.print(ssTemperature);
  tft.print("°C");
  tft.println();

  tft.print("  Độ ẩm: ");
  tft.print(ssHumidity);
  tft.print(" %RH");
  tft.println();

  tft.print("  Ánh sáng: ");
  tft.print(ssLightAo);
  tft.print(" lux");
  tft.println();

  tft.print("  Đất: ");
  tft.print(soilValue ? "Khô" : "Ẩm");
  tft.println();

  tft.print("  Lần tưới gần đây:");
  tft.print("\n      ");
  tft.print(sysConvertMillisToTimeString(millis() - soilLastWaterTime, 0));
  tft.println();

  tft.print("  Thời gian phơi nắng:");
  tft.print("\n      ");
  tft.print(sysConvertMillisToTimeString(sunMillis, 1));
  tft.println();
}

void scrDrawVolScreen() {
  
  tft.setCursor(0, 92);
  display.fillRect(0, 20, SCR_W, 240, ST77XX_WHITE);
  display.drawXBitmap(108, 32, epd_bitmap_icoVol, 24, 24, ST77XX_BLUE); 
  
  tft.println("  Âm lượng: ");
  tft.print("  ");
  tft.print(volume_level * 10);
  tft.print(" %");
  tft.println();
}

void scrDrawMailScreen() {
  tft.setCursor(0, 92);
  display.fillRect(0, 20, SCR_W, 240, ST77XX_WHITE);
  display.drawXBitmap(108, 32, epd_bitmap_icoMail, 24, 24, ST77XX_GREEN); 
  
  tft.println("  Hãy bấm SELECT để gửi\n  thư điện tử trạng thái.");
}

void scrDrawMailTaskScreen() {
  tft.setCursor(0, 92);
  display.fillRect(0, 20, SCR_W, 240, ST77XX_WHITE);
  display.drawXBitmap(108, 32, epd_bitmap_icoMail, 24, 24, ST77XX_RED); 
  
  tft.println("  Chờ tí nhé!\n  Mail đang tới nè...");
}

void scrDrawInfoScreen() {
  tft.setCursor(0, 92);
  display.fillRect(0, 20, SCR_W, 240, ST77XX_WHITE);
  display.drawXBitmap(108, 32, epd_bitmap_icoSetup, 24, 24, ST77XX_BLUE); 
  
  tft.println("  Thông tin đăng ký của bạn:");
  tft.println("  Tên cây:");
  tft.print  ("    ");
  tft.println(PLANT_NAME);
  tft.println("  Email:");
  tft.print  ("    ");
  tft.println(RECIPIENT_EMAIL);
  tft.println("  Tên người dùng:");
  tft.print  ("    ");
  tft.println(YOU_NAME);
  tft.println("  Xưng hô:");
  tft.print  ("    ");
  tft.println(YOU_CALL);
  tft.println("  Kết nối Wi-Fi:");
  tft.print  ("    ");
  tft.println(wmSsid);
}

void scrDrawResetScreen() {
  tft.setCursor(0, 92);
  display.fillRect(0, 20, SCR_W, 240, ST77XX_WHITE);
  display.drawXBitmap(108, 32, epd_bitmap_icoReplay, 24, 24, ST77XX_RED); 
  
  tft.println("  Bạn hãy bấm SELECT\n  để khởi động lại hệ thống");
}

void scrDrawStatusBar() {
  tft.setCursor(48, 14);
  display.fillRect(0, 0, SCR_W, 14, ST77XX_WHITE);
  
  tft.print(sysGetDateTimeStringShort());
}

void scrDrawPageNumber() {
  tft.setCursor(72, 304);
  display.fillRect(0, 290, SCR_W, 40, ST77XX_WHITE);
  
  tft.print("TRANG ");
  tft.print(currentScreenPage + 1);
  tft.print("/5");
}

void scrDrawFullMsg(String msg) {
  scrClear();
  tft.setCursor(0, 20);  
  tft.print(msg);
}