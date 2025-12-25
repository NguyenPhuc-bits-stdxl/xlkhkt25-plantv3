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

  // Load Prefs config
  RECIPIENT_EMAIL = prefs.getString("wmstEmail", "");
  
  PLANT_NAME = prefs.getString("wmstPlantName", "cây xanh bình thường");
  if (PLANT_NAME == "") {
    PLANT_NAME = "cây xanh bình thường";
  }

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

  // Reset UC
  sysResetUncomfortableState();

  // Email
  emlInit();
}

void sysReset() {
  scrClear();
  scrDrawIcon(ICO_START_X, ICO_START_Y, ICO_ACT_DIMM, ICO_ACT_DIMM, epd_bitmap_icoReplay, ST77XX_BLUE);
  scrDrawMessageFixed(MSG_START_X, MSG_START_Y, systrReset);

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
  char buf[512];
  snprintf(buf, sizeof(buf),
    " [REPORT] Nhiệt độ: %d độ C. Độ ẩm: %d phần trăm. Ánh sáng: %d. Vị trí: %s (GMT+7). Thời gian hiện tại theo UTC: %s. ",
    (int)ssTemperature, (int)ssHumidity, ssLightAo, WEB_SEARCH_USER_CITY, sysGetDateTimeString().c_str()
  );
  return String(buf);
}

String sysGetUncomfortableString() {
  return String(" [SYS] Nova, hệ thống phát hiện chỉ số bất thường. Hãy báo ngay cho bạn thân và yêu cầu hỗ trợ khẩn cấp. ");
}

bool sysIsUncomfortable() {
  // Check các điều kiện...
  // ReadSensors đã chạy từ trước

  if (ssHumidity < 60.0f) return true;
  if (ssHumidity > 90.0f) return true;
  if (ssTemperature < 10.0f) return true;
  if (ssTemperature > 40.0f) return true;

  struct tm timeinfo;
  int h = timeinfo.tm_hour + 7; // (lệch 7h do utc+7)
  // 06:00 đến 15:00 trời tối => kêu than tối quá
  if ((h >= 6) && (h <= 15) && (ssLightAo > 3800)) { 
    return true;
  }
  return false;
}

void sysResetUncomfortableState() {
  IGNORE_STATUS = false;
  IGNORE_TIMES = 0;
}

void sysAccumulateUncomfortableState() {
  IGNORE_STATUS = true;
  IGNORE_TIMES++;
}

bool sysUncomfortableShouldUseAI() {
  if (!IGNORE_STATUS) return false;
  if (IGNORE_TIMES <= IGNORE_SERIOUS) return true;
  return false;
}

String sysGetUncomfortableStringIgnore() {
  char buf[196];
  snprintf(buf, sizeof(buf),
    "Bạn ơi, giúp mình với. Mình đã nhờ bạn %d lần rồi đấy!",
    IGNORE_TIMES
  );
  return String(buf);
}