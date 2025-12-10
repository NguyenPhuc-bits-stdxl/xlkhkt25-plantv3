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

void scrShowStatus() {
  tft.setCursor(0, 0);
  tft.fillScreen(ST77XX_WHITE);
  tft.setTextColor(ST77XX_BLACK);
  tft.setTextSize(1);

  tft.print("Light AO: ");
  tft.println(ssLightAo);
  tft.print("Temp: ");
  tft.println(ssTemperature);
  tft.print("Humidity: ");
  tft.println(ssHumidity);
  tft.print("Battery: ");
  tft.println(90);
}

void scrStartUp() {
  tft.setCursor(0, 0);
  tft.fillScreen(ST77XX_WHITE);
  tft.setTextColor(ST77XX_GREEN);
  tft.setTextSize(2);
  tft.print("LILY");
}

void scrShow