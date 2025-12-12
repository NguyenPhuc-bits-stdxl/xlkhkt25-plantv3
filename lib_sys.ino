void sysInit() {
  sysFetchCreds();

  // Sensors
  pinMode(DHTPIN, INPUT_PULLUP);

  // Preferences, Sensors, WiFiManager
  prefs.begin("config", false);
  dht.begin();
  wmInit();
  wmConnect();
  scrInit();     
  scrStartUp();

  if (sysIsResetPressed()) {
    delay(1000);
    sysReset();
  }
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
    ssTemperature, ssHumidity, ssLightAo
  );
  return String(buf);
}