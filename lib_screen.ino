void scrInit() {
  SPI.begin(LCD_SCLK, -1, LCD_MOSI, LCD_CS);
  delay(100);
  display.initR(INITR_144GREENTAB);
  display.setRotation(0);
  tft.begin(display);

  display.fillScreen(ST77XX_WHITE);      
  tft.setFontMode(1);          
  tft.setFontDirection(0);   
  tft.setBackgroundColor(ST77XX_WHITE);        
  tft.setForegroundColor(ST77XX_BLACK);
  tft.setFont(u8g2_font_courR08_tf);  
}

void scrDrawIcon(const uint8_t x, const uint8_t y, const unsigned char icon[]) {

}

void scrDrawMessage(const uint8_t x, const uint8_t y, const char* msg)
{
  tft.setCursor(x, y);
  tft.println(msg);
}

void scrShowMessageWithIcon(const char* msg, const unsigned char icon[]) {  
  display.fillScreen(ST77XX_WHITE);    
  scrDrawIcon(52, 8, icon);
  scrDrawMessage(2, 60, msg);
}

void scrShowMessage(const char* msg) {
  display.fillScreen(ST77XX_WHITE);    
  scrDrawMessage(2, 2, msg);
}

void scrShowStatus() {
}

void scrStartUp() {
  display.fillScreen(ST77XX_WHITE);    
  tft.setCursor(0, 0);
  tft.print("LILY");
}