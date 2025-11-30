#define LCD_CS   9
#define LCD_DC   10
#define LCD_SCLK 13
#define LCD_MOSI 12
#define LCD_RST  11
Adafruit_ST7735 tft = Adafruit_ST7735(LCD_CS, LCD_DC, LCD_RST);

void scrInit() {
  SPI.begin(LCD_SCLK, -1, LCD_MOSI, LCD_CS);
  delay(100);
  tft.initR(INITR_144GREENTAB);
  tft.setRotation(0);
}

void scrShowMessage(const char* msg) {
  Serial.print("diag ");
  Serial.println(msg);
}

void scrShowStatus(int vLight, float vTemp, float vHumid, int vBat) {
  tft.setCursor(0, 0);
  tft.fillScreen(ST77XX_WHITE);
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(1);

  tft.print("Light: ");
  tft.println(vLight);
  tft.print("Temp: ");
  tft.println(vTemp);
  tft.print("Humidity: ");
  tft.println(vHumid);
  tft.print("Battery: ");
  tft.println(vBat);
}

void scrStartUp() {
  tft.setCursor(0, 0);
  tft.fillScreen(ST77XX_WHITE);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.print("LILY");
}