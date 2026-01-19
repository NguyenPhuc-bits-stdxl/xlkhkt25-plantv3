#pragma region Essentials

// Init khởi động
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

  // Lấy giá trị cấu hình
  PLANT_NAME = prefs.getString("wmstPlantName", DEF_PLANT_NAME); 
  YOU_CALL = prefs.getString("wmstCall", DEF_YOU_CALL);
  YOU_NAME = prefs.getString("wmstName", DEF_YOU_NAME);
  YOU_DESC = prefs.getString("wmstDesc", DEF_YOU_DESC);
  YOU_AGE  = prefs.getString("wmstAge", DEF_YOU_AGE);

  // Lấy giá trị cấu hình (LẦN 2) Check string rỗng
  if (PLANT_NAME == "") PLANT_NAME = DEF_PLANT_NAME;
  if (YOU_CALL == "") YOU_CALL = DEF_YOU_CALL;
  if (YOU_NAME == "") YOU_NAME = DEF_YOU_NAME;
  if (YOU_DESC == "") YOU_DESC = DEF_YOU_DESC;
  if (YOU_AGE == "") YOU_AGE = DEF_YOU_AGE;

  // Lấy giá trị Thresh (NA = default)
  THRESH_TEMP_MIN  = prefs.getFloat("wmstTmin", 15.0f);
  THRESH_TEMP_MAX  = prefs.getFloat("wmstTmax", 45.0f);
  THRESH_HUMID_MIN = prefs.getFloat("wmstHmin", 60.0f);
  THRESH_HUMID_MAX = prefs.getFloat("wmstHmax", 90.0f);
  THRESH_LIGHT_MIN = prefs.getFloat("wmstLmin", 800.0f); // 8klux - 20klux +- 2klux bù lệch cảm biến
  THRESH_LIGHT_MAX = prefs.getFloat("wmstLmax", 22000.0f);

  // Lấy giá trị chăm sóc (NA = default)
  WATER_CYCLE      = prefs.getInt("wmstWcycle", 3);
  SUN_TARGET       = prefs.getFloat("wmstLDuration", 6);
  
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
  // Lấy thời gian +7 (VN)
  sysSyncNTP();

  // Reset UC
  sysUncomfReset();

  // CareObj
  sunInit();
  soilInit();

  // Email
  emlInit();
}

// ĐÈ VOL + REC để RESET khi khởi động => cấu hình lại WF
bool sysIsResetPressed() {
  if ((digitalRead(pin_RECORD_BTN) == LOW) && (digitalRead(pin_VOL_BTN) == LOW))
  {
    return true;
  }
  return false;
}

// Hàm RESET
void sysReset() {
  scrClear();
  scrDrawIcon(ICO_START_X, ICO_START_Y, ICO_ACT_DIMM, ICO_ACT_DIMM, epd_bitmap_icoReplay, ST77XX_BLUE);
  scrDrawMessageFixed(MSG_START_X, MSG_START_Y, systrReset);

  delay(500);

  wmConfig();
}

void sysSyncNTP() {
  configTime(7*3600, 0, "pool.ntp.org", "time.nist.gov"); 
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

  char buffer[128];
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

  char buffer[128];
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

#pragma endregion Essentials

#pragma region SetupPhase2

// Hàm Setup Phase 2
void sysSetupPhase2() {
  scrSetupPhase2();

  Serial.println("SYS SP2");
  prefs.putString("sysStMem", DEFAULT_MEM);

  // append #setup2
  LLM_Append(RSTR_DEV, prmSetupPhase2());
  String ThreshFbk = OpenAI_Groq_LLM( OPENAI_KEY, true, GROQ_KEY );
  bool ThreshFbkParse = sysParsePlantThres(ThreshFbk);
  
  Serial.println();
  Serial.print("SYS PARSE RESULT ");
  Serial.println(ThreshFbkParse);

  prefs.putBool("wmstSetupDone", true); // set TRUE

  int tickPos = ThreshFbk.indexOf("```");
  if (tickPos < 0) tickPos = ThreshFbk.length();

  emlStart();
  emlBodyWelcome(ThreshFbk.substring(0, tickPos));
  emlFinalize();

  Serial.println("SYS SP2 DONE. RESTARTING...");
  ESP.restart(); // restart để áp dụng hiệu lực
}

// Parse Feedback LLM Setup Phase 2
bool sysParsePlantThres(String response) {
    int start = response.indexOf("```");
    if (start < 0) return false;

    start += 3;
    int end = response.indexOf("```", start);
    if (end < 0) return false;

    String payload = response.substring(start, end);
    payload.trim();

    float tmin, tmax, hmin, hmax, lmin, lmax, wcycle, sunhours;

    int count = sscanf(
        payload.c_str(),
        "%f %f %f %f %f %f %f %f",
        &tmin, &tmax,
        &hmin, &hmax,
        &lmin, &lmax,
        &wcycle, &sunhours
    );

    if (count != 8) return false;

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
  
    prefs.putInt("wmstWcycle", (int)wcycle);
    prefs.putFloat("wmstLDuration", sunhours);

    return true;
}

#pragma endregion SetupPhase2

#pragma region MEM

// Cập nhật [MEM]
void sysUpdateShortTermMem(String response) {
  prefs.putString("sysStMem", response);
  Serial.println();
  Serial.print("SYSMEM updated: ");
  Serial.println(response);
}

// Lấy [MEM]
String sysGetShortTermMemRaw()
{
  String result = prefs.getString("sysStMem", DEFAULT_MEM);
  
  if (result == "") {
    result = DEFAULT_MEM;
  }

  return result;
}

#pragma endregion MEM

#pragma region Uncomf

bool sysUncomfCheck() {
  // reset và lấy giá trị cảm biến
  sysSensorsRead();

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

void sysUncomfReset() {
  IGNORE_STATUS = false;
  IGNORE_TIMES = 0;
}

void sysUncomfAccumulate() {
  IGNORE_STATUS = true;
  IGNORE_TIMES++;
}

bool sysUncomfAI() {
  if (!IGNORE_STATUS) return false;
  if (IGNORE_TIMES <= IGNORE_SERIOUS) return true;
  return false;
}

String sysUncomfGetStringNoAI() {
  char buf[196];
  snprintf(buf, sizeof(buf),
    "Bạn ơi, giúp mình với. Mình đã nhờ bạn %d lần rồi đấy!",
    IGNORE_TIMES
  );
  return String(buf);
}

#pragma endregion Uncomf

#pragma region Sensors

// Hàm chạy liên tục trong loop()
void sysSensorsLoop() {
  // Đọc giá trị tức thời
  curLightAo = LDR_to_LUX(analogRead(LDR_AO));
  curHumidity = dht.readHumidity();
  curTemperature = dht.readTemperature();

  // Kiểm tra giá trị hợp lệ
  if (!isnan(curLightAo)) {
    countLight++;
    avgLightAo = avgLightAo + (curLightAo - avgLightAo) / countLight;
  }

  if (!isnan(curHumidity)) {
    countHumidity++;
    avgHumidity = avgHumidity + (curHumidity - avgHumidity) / countHumidity;
  }

  if (!isnan(curTemperature)) {
    countTemperature++;
    avgTemperature = avgTemperature + (curTemperature - avgTemperature) / countTemperature;
  }
}

// Hàm đọc giá trị trung bình tức thời
void sysSensorsRead() {
  // Gán giá trị trung bình vào biến ss-
  ssLightAo = avgLightAo;
  ssHumidity = avgHumidity;
  ssTemperature = avgTemperature;

  // Reset lại trung bình bằng giá trị hiện tại
  float curLightAo = LDR_to_LUX(analogRead(LDR_AO));
  float curHumidity = dht.readHumidity();
  float curTemperature = dht.readTemperature();

  if (isnan(curHumidity) || isnan(curTemperature)) {
    ssHumidity = -1;
    ssTemperature = -1;
  }

  avgLightAo = curLightAo;
  avgHumidity = curHumidity;
  avgTemperature = curTemperature;

  countLight = 1;
  countHumidity = 1;
  countTemperature = 1;
}

float LDR_to_LUX(float AoValue)
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

#pragma endregion Sensors

#pragma region Prompts

// ROLE: DEVELOPER
// GLOBAL PARAMS: #[YOU], #[THRESH], PLANT_NAME, #[CARE_PARAMS]
// Dùng khi khởi động, sau system prompt
String prmBuildInfo() {
  String prompt = "";
  prompt += "Dưới đây là thông tin về người dùng: cách xưng hô " + YOU_CALL;
  prompt += ", tên của họ là " + YOU_NAME;
  prompt += ", những từ thể hiện tính cách " + YOU_DESC;
  prompt += ", tuổi của họ là " + YOU_AGE;
  prompt += ". Nếu không có tên thì hãy gọi họ là bạn thân, nếu không có tuổi thì coi như một người trưởng thành khoảng 25 tuổi. ";
  prompt += "Nếu không có cách xưng hô thì hãy dùng bạn – mình, còn nếu người trò chuyện có vai vế lớn hơn thì hãy dựa theo ngữ cảnh mà dùng con, cháu hoặc em cho phù hợp. ";
  prompt += "Bạn hãy dựa vào các thông tin này để cá nhân hóa trò chuyện, tăng cường thấu hiểu người dùng và lựa chọn ngôn ngữ phù hợp. ";
  prompt += "Được biết thêm, bạn là loài cây " + PLANT_NAME;
  prompt += ", nếu không cung cấp thì hãy xem như một cây cảnh bình thường. ";
  prompt += "Developer nhận thấy đây là những chỉ số môi trường lý tưởng cho bạn: ";
  prompt += "nhiệt độ (độ C) từ " + String(THRESH_TEMP_MIN, 1) + " đến " + String(THRESH_TEMP_MAX, 1);
  prompt += ", độ ẩm RH% từ " + String(THRESH_HUMID_MIN, 0) + " đến " + String(THRESH_HUMID_MAX, 0);
  prompt += ", ánh sáng (lux) từ " + String(THRESH_LIGHT_MIN, 0) + " đến " + String(THRESH_LIGHT_MAX, 0);
  prompt += ", bạn cần được tưới nước " + String(WATER_CYCLE) + " ngày/lần, ";
  prompt += "và bạn cần được phơi nắng " + String(SUN_TARGET) + " giờ mỗi ngày. ";
  prompt += "Bạn phải dựa vào các thông tin này để phản hồi phù hợp, vừa đồng cảm với người dùng vừa nhờ họ hỗ trợ chăm sóc khi cần.";

  return prompt;
}

// ROLE: DEVELOPER
// GLOBAL PARAMS: PLANT_NAME
// Dùng khi thiết lập phase 2, sau system prompt để lấy thông tin cây
String prmSetupPhase2() {
  String prompt = "";  
  prompt += "[SYS] Đây là yêu cầu THIẾT LẬP LẠI các CHỈ SỐ MÔI TRƯỜNG. ";
  prompt += "Bạn phải dựa vào kiến thức của mình và các nguồn tài liệu uy tín trên internet về loài cây " + PLANT_NAME;
  prompt += " để mô tả thật ngắn gọn môi trường phát triển phù hợp và hướng dẫn chăm sóc bạn, tối đa là năm câu, không dài dòng. ";
  prompt += "Sau phần mô tả, bạn BẮT BUỘC phải xuất ra MỘT code-block duy nhất với ký hiệu mở và đóng là ``` ";
  prompt += "và không thêm bất kỳ chữ, ký hiệu hay ngôn ngữ chú thích nào khác. ";
  prompt += "Bên trong code-block phải có đúng 8 số nguyên, viết trên cùng một dòng, cách nhau bằng dấu cách, theo thứ tự: ";
  prompt += "t_min t_max h_min h_max l_min l_max w_cycle sun_hours. ";
  prompt += "Các giá trị lần lượt là: nhiệt độ tối thiểu và tối đa an toàn (°C), độ ẩm tối thiểu và tối đa an toàn (RH, %%), ";
  prompt += "ánh sáng tối thiểu và tối đa (lux), chu kỳ tưới (ngày/lần), và số giờ phơi nắng lý tưởng mỗi ngày. ";
  prompt += "Nếu thông tin về loài cây không rõ ràng, hãy sử dụng bộ mặc định sau để phán đoán: 10 40 60 90 800 2200 3 4. ";
  prompt += "Sai số nhỏ được chấp nhận nếu không ảnh hưởng đáng kể đến thực tế, ví dụ giữa 32°C và 35°C, ";
  prompt += "nhưng sai số lớn như giữa 25°C và 40°C là không được phép. ";
  prompt += "Bạn phải tuân thủ tuyệt đối định dạng này để hệ thống có thể đọc dữ liệu chính xác. Không chèn các liên kết web, danh sách liệt kê.";

  return prompt;
}

// ROLE: DEVELOPER
// GLOBAL PARAMS: #mem
// Chèn vào sau mỗi yêu cầu người dùng
String prmPostfixMem() {
  String prompt = "";
  
  prompt += "[MEM] " + sysGetShortTermMemRaw();
  prompt += " [SYS] Sau mỗi đoạn hội thoại với người dùng, bạn bắt buộc phải tạo một đoạn trí nhớ ngắn hạn mới, ";
  prompt += "đặt ngay sau chỉ thị [MEM] trong câu trả lời.";

  return prompt;
}

// ROLE: DEVELOPER
// LOCAL PARAMS (given)
// GLOBAL PARAMS: #report
// Prompt báo cáo, dùng trong khi cần gửi dữ liệu [REPORT] hoặc kèm postfix
String prmPostfixReport() {
  String report = "";
  report += "[REPORT] Nhiệt độ là " + String(ssTemperature, 1) + " độ C. ";
  report += "Độ ẩm là " + String(ssHumidity, 1) + " %. ";
  report += "Ánh sáng là " + String(ssLightAo, 0) + " lux. "; // Ánh sáng thường là số nguyên lớn nên để 0 chữ số thập phân
  report += "Thời gian hiện giờ là " + sysGetDateTimeString() + ".";
  
  return report;
}

// ROLE: DEVELOPER
// LOCAL PARAMS (given)
// GLOBAL PARAMS: #attention
// Prompt yêu cầu chú ý, gửi tự động, khi bất lợi
String prmPostfixAttentionRequired() {
  String attention = "";
  attention += "[SYS] Nova, hệ thống phát hiện chỉ số bất thường, hãy nhờ người dùng giúp đỡ bạn. ";
  return attention;
}

// ROLE: DEVELOPER
// Yêu cầu tạo MEM do LLM liên tục quên
String prmPostfixMemCre() {
  return "[SYS] Bắt buộc sau khi trả lời người dùng xong, bạn phải đính kèm một đoạn trí nhớ từ 20-100 từ đặt sau [MEM] trong cùng câu trả lời ấy, ghi đúng định dạng, KHÔNG phải là [NEM] hay [MEN], BẮT BUỘC là [MEM].";
}
// ROLE: DEVELOPER
// GLOBAL PARAMS: #[CARE_PARAMS]
// Gửi tự động, yêu cầu LLM trả lời cảm ơn/nhắc nhở người dùng chăm sóc
String prmCare() {
  String prompt = "";
  prompt += "[REPORT] Dưới đây là dữ liệu chăm sóc: ";
  prompt += String((float)SUN_TIME / 3600000.0f, 4) + " = số giờ nắng đã nhận hôm nay, ";
  prompt += String(SUN_TARGET) + " = mục tiêu giờ nắng, ";
  prompt += String(((float)(millis() - WATER_LAST_TM) / 86400000.0f), 4) + " = lần tưới gần nhất (ngày), ";
  prompt += String(WATER_CYCLE) + " = chu kỳ tưới (ngày). ";
  prompt += "Thời gian hiện giờ là " + sysGetDateTimeString() + ". ";
  prompt += "[SYS] Hãy phản hồi ngắn gọn, thân thiện và khích lệ: nếu người dùng đã chăm sóc đúng thì cảm ơn họ vì đã dành thời gian, ";
  prompt += "nếu chưa thì nhắc nhở nhẹ nhàng để hoàn thành việc tưới hoặc phơi nắng theo khuyến nghị.";

  return prompt;
}

#pragma endregion Prompts

#pragma region CareObjSun

// float SUN_TARGET = 6; // mục tiêu số giờ nắng mỗi ngày
// #define SUN_THRESHOLD 2800 // tính phơi nắng từ 2800 lux
// float SUN_TIME = 0.0; // theo miliseconds
// int SUN_LAST_DAY = -1;
// unsigned long SUN_LAST_CHECKED = 0;

// chạy ở #sysInit
void sunInit() {
  SUN_TIME = 0;
  SUN_LAST_CHECKED = millis();
}

// chạy trong #loop
// nếu bằng true => đã hoàn thành mục tiêu phơi nắng và ngược lại
bool sunLoop() {
  int currentDay = timeinfo.tm_mday;
  if (SUN_LAST_DAY == -1) SUN_LAST_DAY = currentDay;
  else if (currentDay != SUN_LAST_DAY) {
    Serial.printf("\nSYS Sun new day (%d, %d) \n", SUN_LAST_DAY, currentDay);
    SUN_TIME = 0;
    SUN_LAST_DAY = currentDay;
  }

  unsigned long currentTime = millis();
  float deltaTime = (currentTime - SUN_LAST_CHECKED);
  SUN_LAST_CHECKED = currentTime;

  if (curLightAo > SUN_THRESHOLD) SUN_TIME += deltaTime;

  return (SUN_TIME >= SUN_TARGET * 3600); // đạt mục tiêu chưa?
}

#pragma endregion CareObjSun

#pragma region CareObjSoil

// int WATER_CYCLE = 3; // chu kỳ tưới (ngày)
// unsigned long WATER_LAST_TM = 0;

// #define SOIL_DO 6
// #define SOIL_INTERVAL 1800000 // 30 mins

// unsigned long SOIL_DRY_START_TM = 0; // Thời điểm bắt đầu khô
// bool SOIL_DRY_START_EN = false; // Cờ đánh dấu đang trong quá trình đếm 30p
// bool SOIL_VALUE = false;
// bool SOIL_VALUE_LAST = false; // mặc định là tưới

void soilInit() {
  pinMode(SOIL_DO, INPUT);
}

// chạy trong #loop
// nếu bằng false => đất đã khô lâu hơn INTERVAL
bool soilLoop() {
  SOIL_VALUE = digitalRead(SOIL_DO);

  // KIỂM TRA KHOẢNH KHẮC VỪA ĐƯỢC TƯỚI
  // Nếu trước đó là Khô (1) mà bây giờ là Ẩm (0) -> Vừa có người tưới
  if (SOIL_VALUE_LAST == 1 && SOIL_VALUE == 0) {
    WATER_LAST_TM = millis(); 
    Serial.println("SYS Water event registered");
  }

  SOIL_VALUE_LAST = SOIL_VALUE;

  // Nếu khô (= 1)
  if (SOIL_VALUE) {
    
    // Vừa mới chuyển từ Ẩm sang Khô, bắt đầu tính giờ
    if (!SOIL_DRY_START_EN) {
      SOIL_DRY_START_TM = millis();
      SOIL_DRY_START_EN = true;
      Serial.println("SYS Soil is dry! Start timer!");
    } 
    // Đang khô và đã tính giờ, kiểm tra xem đã quá 30p chưa
    else {
      if (millis() - SOIL_DRY_START_TM >= SOIL_INTERVAL) {
        return false; // Đã khô >= 30 phút
      }
    }
  } 
  // Trạng thái ẨM (sensorValue == 0)
  else { 
    if (SOIL_DRY_START_EN) {
      Serial.println("SYS Soil is wet, timer has been reset!");
    }
    SOIL_DRY_START_EN = false;
    SOIL_DRY_START_TM = 0;
  }

  return true;
}
#pragma endregion CareObjSoil

String getMailStateString() {
  String msg;
  msg +=   "- Thời gian: " + sysGetDateTimeString();
  msg += "<br>- Địa điểm: " + String(WEB_SEARCH_USER_CITY);
  msg += "<br>- Ánh sáng: " + String(curLightAo, 2) + " lux";
  msg += "<br>- Nhiệt độ: " + String(curTemperature, 2) + " °C";
  msg += "<br>- Độ ẩm (%RH): " + String(curHumidity, 2) + " %";
  msg += "<br>- Trạng thái: ";
    msg += (IGNORE_STATUS ? "Cần trợ giúp (" : "Vui vẻ (");
    msg += String(IGNORE_TIMES) + " lần)";
  msg += "<br>- Trạng thái đất: ";
    msg += (SOIL_VALUE ? "Khô" : "Ẩm");
  msg += "<br>- Lần tưới cây gần nhất: ";
    msg += String((float)(millis() - WATER_LAST_TM)/(3600000.0f), 4) + " giờ trước";
  msg += "<br>- Thời gian phơi sáng hôm nay: ";
    msg += String((float)SUN_TIME / 3600000.0f, 4) + " giờ";
  msg += "<br>Mục tiêu chăm sóc:";
  msg += "<br>- Ánh sáng (lux): từ ";
    msg += String(THRESH_LIGHT_MIN, 1) + " đến " + String(THRESH_LIGHT_MAX, 1);
  msg += "<br>- Nhiệt độ (độ C): từ ";
    msg += String(THRESH_TEMP_MIN, 1) + " đến " + String(THRESH_TEMP_MAX, 1);
  msg += "<br>- Độ ẩm: từ ";
    msg += String(THRESH_HUMID_MIN, 1) + " đến " + String(THRESH_HUMID_MAX, 1);
  msg += "<br>- Chu kỳ tưới cây: ";
    msg += String(WATER_CYCLE) + " ngày/lần";
  msg += "<br>- Mỗi ngày cần ";
    msg += String(SUN_TARGET) + " giờ nắng";

  return msg;
}