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

  // Load Prefs config - Email - default NULL
  RECIPIENT_EMAIL = prefs.getString("wmstEmail", "");
  // Load Prefs config - GetThresh - default TRUE - FALSE only when after First Config
  SETUP_GET_THRESH = prefs.getBool("wmstGetThresh", true);

  // (")> Dùng biện pháp cực kỳ cực đoan, gán giá trị mặc định 2 lần
  PLANT_NAME = prefs.getString("wmstPlantName", DEF_PLANT_NAME); 
  YOU_CALL = prefs.getString("wmstCall", DEF_YOU_CALL);
  YOU_NAME = prefs.getString("wmstName", DEF_YOU_NAME);
  YOU_DESC = prefs.getString("wmstDesc", DEF_YOU_DESC);
  YOU_AGE  = prefs.getString("wmstAge", DEF_YOU_AGE);

  // Check string rỗng
  if (PLANT_NAME == "") PLANT_NAME = DEF_PLANT_NAME;
  if (YOU_CALL == "") YOU_CALL = DEF_YOU_CALL;
  if (YOU_NAME == "") YOU_NAME = DEF_YOU_NAME;
  if (YOU_DESC == "") YOU_DESC = DEF_YOU_DESC;
  if (YOU_AGE == "") YOU_AGE = DEF_YOU_AGE;

  THRESH_TEMP_MIN  = prefs.getFloat("wmstTmin", 10.0f);
  THRESH_TEMP_MAX  = prefs.getFloat("wmstTmax", 40.0f);
  THRESH_HUMID_MIN = prefs.getFloat("wmstHmin", 60.0f);
  THRESH_HUMID_MAX = prefs.getFloat("wmstHmax", 90.0f);
  THRESH_LIGHT_MIN = prefs.getFloat("wmstLmin", 0.0f);
  THRESH_LIGHT_MAX = prefs.getFloat("wmstLmax", 3800.0f);

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

String sysBuildInfoPrompt() {
    char buf[1024];

    snprintf(
        buf,
        sizeof(buf),

        "Được biết thêm, bạn là một %s. "
        "Hãy dựa vào đặc điểm sinh học và nhu cầu sinh trưởng phổ biến của loài này "
        "(nếu không xác định rõ, coi như cây xanh thông thường). "

        "Đánh giá môi trường theo cấu hình hệ thống (chỉ dùng các ngưỡng dưới đây): "
        "- Nhiệt độ: dưới %.1f°C là lạnh; từ %.1f°C trở lên là khó chịu. "
        "- Độ ẩm: dưới %.1f%% là khô; trên %.1f%% có nguy cơ úng. "
        "- Ánh sáng: đo từ 0–4095 (giá trị lớn hơn là tối hơn). "
        "Từ %.1f trở lên là rất tối; %.1f trở xuống là rất sáng. "

        "Thông tin người dùng (nếu thông tin bị thiếu, rỗng hoặc không hợp lệ, "
        "hãy dùng giá trị mặc định, KHÔNG suy diễn): "
        "- Xưng hô: %s (nếu không cung cấp, dùng 'bạn'). "
        "- Tên: %s (nếu không cung cấp hoặc rỗng, gọi là 'bạn thân'). "
        "- Tuổi: %s (nếu không cung cấp hoặc không hợp lệ, coi là người trưởng thành bình thường). "
        "- Tính cách: %s (nếu không cung cấp, mặc định vui vẻ, đơn giản, lạc quan, hơi hướng nội). "

        "Hãy sử dụng các thông tin hợp lệ nêu trên để phản hồi tự nhiên và phù hợp.",

        PLANT_NAME.c_str(),
        THRESH_TEMP_MIN, THRESH_TEMP_MAX,
        THRESH_HUMID_MIN, THRESH_HUMID_MAX,
        THRESH_LIGHT_MIN, THRESH_LIGHT_MAX,
        YOU_CALL.length() ? YOU_CALL.c_str() : "bạn",
        YOU_NAME.length() ? YOU_NAME.c_str() : "bạn thân",
        YOU_AGE.length() ? YOU_AGE.c_str() : "25",
        YOU_DESC.length() ? YOU_DESC.c_str() : "vui vẻ, đơn giản, lạc quan, hơi hướng nội"
    );

    return String(buf);
}

String sysBuildPlantThresholdPrompt() {
    char buf[1024];

    snprintf(
        buf,
        sizeof(buf),
        "[SYS] Đây là yêu cầu THIẾT LẬP LẠI các CHỈ SỐ MÔI TRƯỜNG."
        "Dựa vào kiến thức của bạn về loài cây %s, hãy mô tả thật ngắn gọn môi trường phát triển phù hợp. "
        "Sau đó, BẮT BUỘC xuất ra MỘT code-block duy nhất (```), "
        "chứa CHÍNH XÁC 6 số nguyên trên CÙNG MỘT DÒNG, cách nhau bởi dấu cách: "
        "t_min t_max h_min h_max l_min l_max. "
        "Nhiệt độ (°C), độ ẩm RH (%%), ánh sáng (0-4095). "
        "Nếu không chắc chắn, dùng mặc định: 10 40 60 90 0 3800. "
        "Không được thêm chữ hay ký hiệu nào trong code-block. "
        "Các giá trị có thể xấp xỉ, sai số trong khoảng 3 đơn vị. "
        "KHÔNG dùng liệt kê, ngắt dòng hay liên kết đến các trang web!",
        PLANT_NAME.c_str()
    );

    return String(buf);
}

bool sysParsePlantThres(String response) {
    int start = response.indexOf("```");
    if (start < 0) return false;

    start += 3;
    int end = response.indexOf("```", start);
    if (end < 0) return false;

    String payload = response.substring(start, end);
    payload.trim();

    float tmin, tmax, hmin, hmax, lmin, lmax;

    int count = sscanf(
        payload.c_str(),
        "%f %f %f %f %f %f",
        &tmin, &tmax,
        &hmin, &hmax,
        &lmin, &lmax
    );

    if (count != 6) return false;

    // Nhiệt độ: 5 – 45 °C
    tmin = max(5.0f,  min(tmin, 45.0f));
    tmax = max(5.0f,  min(tmax, 45.0f));

    // Độ ẩm: 30 – 100 %
    hmin = max(30.0f, min(hmin, 100.0f));
    hmax = max(30.0f, min(hmax, 100.0f));

    // Ánh sáng: 0 – 4095
    lmin = max(0.0f,  min(lmin, 4095.0f));
    lmax = max(0.0f,  min(lmax, 4095.0f));

    // Đảm bảo min ≤ max
    float tempValue;
    if (tmin > tmax) {
      tempValue = tmin;
      tmin = tmax;
      tmax = tempValue;
    }
    if (hmin > hmax) {
      tempValue = hmin;
      hmin = hmax;
      hmax = tempValue;
    }
    if (lmin > lmax) {
      tempValue = lmin;
      lmin = lmax;
      lmax = tempValue;
    }

    prefs.putFloat("wmstTmin", tmin);
    prefs.putFloat("wmstTmax", tmax);
    prefs.putFloat("wmstHmin", hmin);
    prefs.putFloat("wmstHmax", hmax);
    prefs.putFloat("wmstLmin", lmin);
    prefs.putFloat("wmstLmax", lmax);

    return true;
}
