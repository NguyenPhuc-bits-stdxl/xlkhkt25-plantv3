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
    return String("lỗi không đọc được thời gian");
  }

  char buffer[256];
  snprintf(
    buffer,
    sizeof(buffer),
    "ngày %02d tháng %02d năm %04d, %02d giờ %02d phút %02d giây",
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
    " Nhiệt độ (từ 40 độ trở lên thì khá là khó chịu): %d độ C."
    " Độ ẩm (dưới 40 phần trăm khá là khát nước): %d phần trăm."
    " Ánh sáng (theo thang từ 0-4096, số càng lớn nghĩa là càng tối): %d."
    " Bạn đang ở %s, thời gian hiện tại theo UTC là %s.",
    (int)ssTemperature, (int)ssHumidity, ssLightAo, WEB_SEARCH_USER_CITY, sysGetDateTimeString().c_str()
  );
  return String(buf);
}

String sysGetUncomfortableString() {
  return String("[SYS] Nova, hệ thống phát hiện chỉ số bất thường. Hãy báo ngay cho bạn thân và yêu cầu hỗ trợ khẩn cấp. Tóm tắt chỉ số: ")
    + sysGetSensorsString();
}

bool sysIsUncomfortable() {
  // Check các điều kiện...

  // if (ssHumidity < 40.0f) return true;
  // if (ssTemperature > 40.0f) return true;

  // struct tm timeinfo;
  // int h = timeinfo.tm_hour + 7; // (lệch 7h do utc+7)
  // // 06:00 đến 14:00 trời tối => kêu than tối quá
  // // note 3800 là giá trị ngẫu nhiên để test, chưa calibrate nên chưa rõ
  // if ((h >= 6) && (h <= 14) && (ssLight > 3800)) { 
  //   return true;
  // }
  // return false;
  return false;
}