// Sys - Thời gian
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
    "%04d/%02d:%02d:%02d",
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


// Used for photodiode sensor to convert output value 0-4095 to LUX
float GetLightValue(float AoValue)
{
    float x = 4095.0f - AoValue;

    if (x <= 0.0)
    {
        return 6.0;   // điểm F
    }
    else if (x <= 1051.0)
    {
        // f: DuongThang(F, B)
        return 0.116079923882f * x + 6.0f;
    }
    else if (x <= 3715.0)
    {
        // g: DuongThang(B, C)
        return 0.2394894894895f * x - 123.7034534534535f;
    }
    else if (x <= 3918.68421052632f)
    {
        // h: DuongThang(C, E)
        return 85.935960591133f * x - 318486.09359605913f;
    }
    else if (x <= 3941.0)
    {
        // i: DuongThang(E, D)
        return 634.2608695652174f * x - 2466823.086956522f;
    }
    else
    {
        // j: DuongThang(D, A)
        return 1585.7428571428572f * x - 6216613.6f;
    }
}

// Sys - Đọc cảm biến và thông tin thoải mái
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

// Sys - Thoải mái
#pragma region Syscomf

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

#pragma endregion Syscomf

// For debug purposes only: In giá trị cảm biến ra SM
void dbgSensors() {
  
  Serial.print("  Nhiệt độ: ");
  Serial.print(ssTemperature);
  Serial.print("°C");
  Serial.println();

  Serial.print("  Độ ẩm: ");
  Serial.print(ssHumidity);
  Serial.print(" %RH");
  Serial.println();

  Serial.print("  Ánh sáng: ");
  Serial.print(ssLightAo);
  Serial.print(" lux");
  Serial.println();

  Serial.print("  Đất: ");
  Serial.print(soilValue ? "Khô" : "Ẩm");
  Serial.println();

  Serial.print("  Lần tưới gần đây:");
  Serial.print("\n      ");
  Serial.print(sysConvertMillisToTimeString(millis() - soilLastWaterTime, 0));
  Serial.println();

  Serial.print("  Thời gian phơi nắng:");
  Serial.print("\n      ");
  Serial.print(sysConvertMillisToTimeString(sunMillis, 1));
  Serial.println();
}

