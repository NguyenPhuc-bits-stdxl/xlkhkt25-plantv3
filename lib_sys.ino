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

  // Time
  configTime(0, 0, "pool.ntp.org");
  Serial.println("SYS NTP Time configured");
}

void sysReset() {
  scrClear();
  scrDrawIcon(ICO_START_X, ICO_START_Y, ICO_ACT_DIMM, ICO_ACT_DIMM, epd_bitmap_icoReplay, ST77XX_BLUE);
  scrDrawMessage(MSG_START_X, MSG_START_Y, systrReset, true, true);

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

String sysGetDateTimeString() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return String("00/00/0000 00:00:00");
  }

  char buffer[25];
  snprintf(
    buffer,
    sizeof(buffer),
    "%02d/%02d/%04d %02d:%02d:%02d",
    timeinfo.tm_mday,
    timeinfo.tm_mon + 1,
    timeinfo.tm_year + 1900,
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec
  );

  return String(buffer);
}

String sysGetSensorsString() {
  sysReadSensors();

  char buf[1024];
  snprintf(buf, sizeof(buf),
    "Bạn là một cây xanh đang trò chuyện với một người bạn thân."
    " Nếu bạn cảm thấy khó chịu, bạn có thể nói với họ khi các chỉ số này không ổn."
    " Nhiệt độ (từ 40 độ trở lên thì khá là khó chịu): %d độ C."
    " Độ ẩm (dưới 40% khá là khát nước): %d%%."
    " Ánh sáng (theo thang từ 0-4096, số càng lớn nghĩa là càng tối): %d."
    " Bạn đang ở %s, thời gian hiện tại theo UTC là %s.",
    (int)ssTemperature, (int)ssHumidity, ssLightAo, WEB_SEARCH_USER_CITY, sysGetDateTimeString()
  );
  return String(buf);
}

String sysGetUncomfortableString() {
  return String("[SYS] NOVA à! Đây là yêu cầu từ HỆ THỐNG. Hiện tại hệ thống đã phát hiện được các chỉ số thất thường, bạn hãy báo cáo với bạn thân của bạn và yêu cầu họ giúp đỡ bạn NGAY LẬP TỨC. Dưới đây là bản tóm tắt chỉ số của bạn")
    + sysGetSensorsString();
}

bool sysIsUncomfortable() {
  // Check các điều kiện...

  // if (ssHumidity < 40.0f) return true;
  // if (ssTemperature > 40.0f) return true;

  // struct tm timeinfo;
  // int h = timeinfo.tm_hour;
  // // 06:00 đến 14:00 trời tối => kêu than tối quá
  // // note 3800 là giá trị ngẫu nhiên để test, chưa calibrate nên chưa rõ
  // if ((h >= 13) && (h <= 21) && (ssLight > 3800)) { // (lệch 7h do utc+7)
  //   return true;
  // }
  // return false;
  return false;
}