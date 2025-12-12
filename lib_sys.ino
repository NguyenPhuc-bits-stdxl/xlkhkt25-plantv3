void sysInit() {
  sysFetchCreds();
  Serial.println("SYS Creds fetched");

  // Sensors
  pinMode(DHTPIN, INPUT_PULLUP);
  dht.begin();
  Serial.println("SYS DHT initialized");

  // TFT
  scrInit();     
  scrStartUp();  
  delay(1000);
  Serial.println("SYS TFT initialized");

  // Prefs
  prefs.begin("config", false);
  Serial.println("SYS Prefs initialized");

  // WiFiManager
  wmInit();
  Serial.println("SYS WF initialized");

  // RESET check
  if (sysIsResetPressed()) {
    sysReset();
    delay(1000);
  }

  // Connect to WiFi
  wmConnect();
  Serial.println("SYS WF connected");
}

void sysReset() {
  scrShowMessage(systrReset);
  wmConfig();
}

// ĐÈ VOL + REC để RESET khi khởi động => cấu hình lại WF
bool sysIsResetPressed() {
  if ((digitalRead(pin_RECORD_BTN) == LOW) && (digitalRead(pin_VOL_BTN) == LOW))
  {
    return true;
  }
  return false;
}

void sysReadSensors() {
  ssLightAo = analogRead(LDR_AO);
  ssHumidity = dht.readHumidity();    
  ssTemperature = dht.readTemperature(); 

  if (isnan(ssHumidity) || isnan(ssTemperature)) {
    ssHumidity = -1;
    ssTemperature = -1;
  }
}

String sysGetSensorsString() {
  char buf[320];
  snprintf(buf, sizeof(buf),
    "Bạn là một cây xanh đang trò chuyện với một người bạn thân."
    " Nếu bạn cảm thấy khó chịu, bạn có thể nói với họ khi các chỉ số này không ổn."
    " Nhiệt độ: %d°C\nĐộ ẩm: %d%%\nÁnh sáng (theo thang từ 0-4096, số càng lớn nghĩa là càng tối): %d \n Lời nói đến từ người bạn thân: \n",
    (int)ssTemperature, (int)ssHumidity, ssLightAo
  );
  return String(buf);
}