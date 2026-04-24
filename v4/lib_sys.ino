void sysSensorsRead() {
  // Gán giá trị vào biến ss-
  ssLightAo = GetLightValue(analogRead(LDR_AO));
  ssHumidity = dht.readHumidity();
  ssTemperature = dht.readTemperature();

  // Soil
  soilValue = digitalRead(SOIL_DO);
  if (soilValueLast == 1 && soilValue == 0) {
    soilLastWaterTime = millis(); 
    Serial.println("SYS Soil registered");
  }
  soilValueLast = soilValue;

  // Sun
  if (ssLightAo > THRESH_LIGHT_MIN) {
    sunMillis += SS_UPDATE;
  }

  NV_OKAY = sysIsOkay();
  if (NV_OKAY) {
    MAIL_UPDATE_LAST = millis();
  }
}



#pragma region Time

void sysSyncNTP() {
  configTime(7*3600, 0, "pool.ntp.org", "time.nist.gov");
  getLocalTime(&timeinfo);
  Serial.println("SYS NTP Time configured");
}

// Lấy string DateTime
String sysGetDateTimeString() {
  // struct tm timeinfo;
  // if (!getLocalTime(&timeinfo)) {
  //   return String("lỗi không đọc được");
  // }

  // KHÁ LÀ LÂU NẾU DÙNG CÁI Ở TRÊN :)
  // lý do: call NTP liên tục, nên thay vào đó lấy internal clock của ESP32 (đã sync từ startup) (")>

  time_t now = time(nullptr);    
  localtime_r(&now, &timeinfo); 

  char buffer[64];
  snprintf(
    buffer,
    sizeof(buffer),
    "ngày %02d tháng %02d, %04d %02d:%02d:%02d",
    timeinfo.tm_mday,
    timeinfo.tm_mon + 1,
    timeinfo.tm_year + 1900,
    timeinfo.tm_hour,
    timeinfo.tm_min,
    timeinfo.tm_sec
  );

  return String(buffer);
}

// Lấy string DateTime shortver
String sysGetDateTimeStringShort() {
  time_t now = time(nullptr);    
  localtime_r(&now, &timeinfo); 

  char buffer[40];
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

// mode 0 d/h:m:s
// mode 1 h:m:s
String sysConvertMillisToTimeString(unsigned long value, uint8_t mode) {
  unsigned long vt = value;

  unsigned long d = vt / 86400000;
  vt -= d * 86400000;

  unsigned long h = vt / 3600000;
  vt -= d * 3600000;

  unsigned long m = vt / 60000;
  vt -= m * 60000;

  unsigned long s = vt / 1000;

  char buffer[20];
  snprintf(
    buffer,
    sizeof(buffer),
    "%04d / %02d:%02d:%02d",
    d,h,m,s
  );

  if (mode == 1) {
    snprintf(
      buffer,
      sizeof(buffer),
      "%02d:%02d:%02d",
      d*24+h,m,s
    );
  }
  return String(buffer);

}

#pragma endregion Time

bool sysValidRange(float value, float min, float max) {
    return (value >= min && value <= max);
}

// Hàm kiểm tra Nhiệt độ
bool isTemperatureValid() {
    return sysValidRange(ssTemperature, THRESH_TEMP_MIN, THRESH_TEMP_MAX);
}

// Hàm kiểm tra Độ ẩm
bool isHumidityValid() {
    return sysValidRange(ssHumidity, THRESH_HUMID_MIN, THRESH_HUMID_MAX);
}

// Hàm kiểm tra Ánh sáng
bool isLightValid() {
    return sysValidRange(ssLightAo, THRESH_LIGHT_MIN, THRESH_LIGHT_MAX);
}

// Kiểm tra all
bool sysIsOkay() {
  return ((isLightValid() == true) && (isHumidityValid() == true) && (isTemperatureValid() == true));
}

String getPlantStatus() {
    // 1. Tính toán thời gian phơi sáng (sunMillis)
    unsigned long totalSecs = sunMillis / 1000;
    int sunH = totalSecs / 3600;
    int sunM = (totalSecs % 3600) / 60;
    int sunS = totalSecs % 60;

    // 2. Tính toán thời gian từ lần tưới cuối (soilLastWaterTime)
    unsigned long diffMillis = millis() - soilLastWaterTime;
    unsigned long diffSecs = diffMillis / 1000;
    
    int days = diffSecs / 86400;
    int hours = (diffSecs % 86400) / 3600;
    int mins = (diffSecs % 3600) / 60;
    int secs = diffSecs % 60;

    // 3. Xử lý trạng thái đất (0: Ẩm, 1: Khô)
    String soilStatus = (soilValue == 0) ? "Đang đủ ẩm" : "Đang bị khô";

    // 4. Xây dựng nội dung String
    String message = "Chào " + YOU_CALL + " " + YOU_NAME + " yêu dấu! ";
    message += "\nMình là cây " + PLANT_NAME + " nhỏ của " + YOU_CALL + " nè!. ";
    
    if (!NV_OKAY) {
        message += "\nMình đang cảm thấy không ổn lắm, " + YOU_CALL + " giúp mình nhé. ";
    } else {
        message += "\nMình đang cảm thấy rất tuyệt vời! ";
    }
    
    message += "Cảm ơn " + YOU_CALL + " " + YOU_NAME + " nhiều nè :3\n";
    
    message += "\n--------------------------\n";
    message += "☀️ Ánh sáng: " + String(ssLightAo) + "\n";
    message += "💧 Độ ẩm không khí: " + String(ssHumidity) + "%\n";
    message += "🌡️ Nhiệt độ: " + String(ssTemperature) + "°C\n";
    message += "🌱 Đất hiện tại: " + soilStatus + "\n";
    
    message += "⏱️ Thời gian phơi sáng hôm nay: ";
    message += String(sunH) + " giờ " + String(sunM) + " phút " + String(sunS) + " giây\n";
    
    message += "🚿 Lần tưới gần nhất: ";
    message += String(days) + " ngày " + String(hours) + " giờ " + String(mins) + " phút " + String(secs) + " giây trước";

    return message;
}