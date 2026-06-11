// Nova Plant Gamification Project
// Thiên Phúc and Thế Trung - 2026 - Xuân Lộc High School
// Version 04 - 24/04/2026

#pragma region Pinout

#define NO_PIN -1

#define pin_I2S_DOUT 16       // I2S Speaker (thru. MAX98357A amp)
#define pin_I2S_LRC 8   
#define pin_I2S_BCLK 3

#define pin_NEXT 41           // Next PushButton (Digital)
#define pin_SELECT 38         // Select PushBustton (Digital)

#define pin_DHT 7            // DHT kxn sensor (Digital - 40bit packets)

#define pin_LDR_AO 4          // AO pin of photodiode sensor/LDR sensor

#define LCD_CS 10             // LCD pins
#define LCD_DC 11
#define LCD_SCLK 14
#define LCD_MOSI 13
#define LCD_RST 12

// Soil Moisture Sensor (Digital)
#define SOIL_DO 5

// SD card
bool flg_SD_found = false;
#define SD_SCK 14
#define SD_MISO 9
#define SD_MOSI 13
#define SD_CS 17

// INMP Mic
#define I2S_LR            LOW                     
#define I2S_WS            17            
#define I2S_SD            15        
#define I2S_SCK           18    

#pragma endregion Pinout

#pragma region Libs

#include <Arduino.h>
#include "icons.h"
#include "driver/i2s_std.h"  
#include <WiFi.h>
#include <WiFiManager.h>       // Dependencies: main, lib_audio_recording, lib_audio_transcription, lib_openai_groq_chat, lib_wifi
#include <Preferences.h>       // Dependencies: lib_sys, lib_wifi
#include <DHT.h>               // Dependencies: lib_sys
#include <time.h>
#include <Audio.h>             // Audio library (for playback)
#include <SD.h>
#include <WiFiClientSecure.h> 

#pragma endregion Libs

#pragma region Misc.

// // SYSTEM PAGES // //
// // 0Home 1Vol 2Mail 3Info 4Reset
int currentScreenPage = 0;

#define SYSINT_PINF 2147483640 // very large number

long long SYS_START; // Timestamp of system's startup

// (DEPENDENCIES REQUIRED)

struct tm timeinfo; // from lib_sys.ino (timekeeping)

Audio audio_play;   // Audio.h object
int gl_VOL_INIT = 21; // Default volume at init (max = 21)
int gl_VOL_STEPS[] = { 0, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21 }; // Volume values array
int volume_level = 10;
int volume_steps = 11;

Preferences prefs;  // Dependencies: lib_wifi, lib_sys

// Screen global
#define SCR_LINE_HEIGHT 14
#define SCR_W 240
#define SCR_H 320

// AI Chat Globals
bool isRecordingVoice = false;
String MESSAGES = ""; // Lưu trữ ngữ cảnh hội thoại
const char* AI_PROMPT = "Bạn là Nova, một mầm cây thông minh và là người bạn đồng hành chăm sóc sức khỏe tinh thần cho người trồng. Hãy trả lời ngắn gọn, thân thiện, mang tính khích lệ và luôn xưng là 'Nova', gọi người đối diện là 'bạn'.";

#pragma endregion Misc.

#pragma region Screen

#include <Adafruit_GFX.h>
#include <U8g2_for_Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

Adafruit_ST7789 display = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);
U8G2_FOR_ADAFRUIT_GFX tft;

// // If ST7735 is used:
// // 1. Uncomment the code block in lib_screen.ino - scrInit()
// // 2. Uncomment the block below

// #include <Adafruit_ST7735.h> // uncomment this if ST7735 is used
// Adafruit_ST7735 display = Adafruit_ST7735(LCD_CS, LCD_DC, LCD_RST); // uncomment this if ST7735 is used

#define ICO_ACT_DIMM 24
#define ICO_START_X 108
#define ICO_START_Y 2
#define MSG_START_X 2
#define MSG_START_Y 30

#pragma endregion Screen

#pragma region WiFi

WiFiManager wm;     // WiFi global objects - Dependencies: main, lib_wifi
bool wmShouldSaveConfig = false;
String wmSsid;
String wmPwd;

#pragma endregion WiFi

#pragma region Email

#include "Networks.h"

#define ENABLE_SMTP  // Allows SMTP class and data
#define ENABLE_DEBUG // Allows debugging
#define READYMAIL_DEBUG_PORT Serial

// If message timestamp and/or Date header was not set,
// the message timestamp will be taken from this source, otherwise
// the default timestamp will be used.
#if defined(ESP32) || defined(ESP8266)
#define READYMAIL_TIME_SOURCE time(nullptr); // Or using WiFi.getTime() in WiFiNINA and WiFi101 firmwares.
#endif

#include <ReadyMail.h>

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465 // SSL or 587 for STARTTLS

String AUTHOR_EMAIL = "";
String AUTHOR_PASSWORD = "";

// Config qua WM
String RECIPIENT_EMAIL = "";

#define SSL_MODE true
#define AUTHENTICATION true
#define NOTIFY "SUCCESS,FAILURE,DELAY" // Delivery Status Notification (if SMTP server supports this DSN extension)
#define PRIORITY "Normal"                // High, Normal, Low
#define PRIORITY_NUM "1"               // 1 = high, 3, 5 = low
#define EMBED_MESSAGE false            // To send the html or text content as attachment

WiFiClientSecure ssl_client;
SMTPClient smtp(ssl_client);

SMTPMessage msg;
String bodyText;
String bodyHtml;

// For more information, see http://bit.ly/474niML
void smtpCb(SMTPStatus status)
{
    if (status.progress.available)
        ReadyMail.printf("ReadyMail[smtp][%d] Uploading file %s, %d %% completed\n", status.state,
                         status.progress.filename.c_str(), status.progress.value);
    else
        ReadyMail.printf("ReadyMail[smtp][%d]%s\n", status.state, status.text.c_str());
}

#pragma endregion Email

#pragma region Plant tracking

// // TRACKING // //
const int DHTPIN = pin_DHT;
const int DHTTYPE = DHT11;
DHT dht(DHTPIN, DHTTYPE);

const int LDR_AO = pin_LDR_AO;

float ssLightAo;
float ssHumidity;
float ssTemperature;

// --- PHƠI NẮNG (Prefix: sun) ---
unsigned long sunMillis = 0;      // Tổng giây nắng tích lũy
const unsigned long sunTarget = 3 * 3600; // Mục tiêu 3 tiếng (10800 giây)

// --- TƯỚI NƯỚC (Prefix: soil) ---
unsigned long soilLastWaterTime = 0; // Lần cuối tưới (millis)
bool soilValue = false;              // Giá trị cảm biến hiện tại
bool soilValueLast = false;          // Giá trị cảm biến vòng loop trước

// Timer trang HOME
#define HOME_UPDATE 2000
unsigned long HOME_UPDATE_LAST = 0;

// Timer cập nhật sensors
#define SS_UPDATE 300
unsigned long SS_UPDATE_LAST = 0;

// Timer gửi mail
#define MAIL_UPDATE 120000 // 2p
unsigned long MAIL_UPDATE_LAST = 0;

// Trạng thái bất mãn
bool NV_OKAY = true;

// // PLANT INFO // //

const char* DEF_PLANT_NAME = "cây xanh bình thường";
const char* DEF_YOU_NAME = "(không rõ)";

String PLANT_NAME = DEF_PLANT_NAME;
String YOU_NAME = DEF_YOU_NAME;

float THRESH_TEMP_MIN  = 10.0f;
float THRESH_TEMP_MAX  = 40.0f;
float THRESH_HUMID_MIN = 55.0f;
float THRESH_HUMID_MAX = 90.0f;
float THRESH_LIGHT_MIN = 400.0f;
float THRESH_LIGHT_MAX = 22000.0f;

#define THRESH_LIGHT 2000

#pragma endregion Plant tracking

#pragma region API Keys

const char* OPENAI_KEY     = "SK_YOUR_OPENAI_KEY";
const char* ELEVENLABS_KEY = "SK_YOUR_ELEVENLABS_KEY";

#pragma endregion API Keys

#pragma region Function prototypes

void handleButtons();
void handleSelectAction();

// Prototypes
void scrPrepareTftPages(const char* msg, uint16_t startY);

void sdInit();
void sdPlaySound(char* fileName);

// Prototypes
void sysFetchCreds();
void sysSensorsRead();

void sysSyncNTP();
String sysGetDateTimeString();
String sysGetDateTimeStringShort();
String sysConvertMillisToTimeString(unsigned long value, uint8_t mode);

bool sysValidRange(float value, float min, float max);
bool isTemperatureValid();
bool isHumidityValid();
bool isLightValid();
bool sysIsOkay();
String getPlantStatus();
void sysUpdateShortTermMem(String memString);

void wmSaveConfigCallback();
void wmSaveCreds(String newSsid, String newPwd, String wmstPlantName, String wmstEmail, String wmstCall, String wmstName);
void wmReadCreds();
void wmConfig();
void wmConnect();
void wmInit();

void scrInit();
void scrClear();
void scrDrawIcon(const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h, const uint8_t* icon, const uint16_t color);
void scrDrawMessageFixed(const uint16_t x, const uint16_t y, String msg);
void scrDrawHomeScreen()
void scrDrawVolScreen();
void scrDrawMailScreen();
void scrDrawMailTaskScreen();
void scrDrawInfoScreen();
void scrDrawResetScreen();
void scrDrawStatusBar();
void scrDrawPageNumber();
void scrDrawFullMsg(String msg);

void emlInit();
void emlStart();
void emlBodyWelcome();
void emlBody(String smsg);
void emlFinalize();

bool I2S_Recording_Init();
bool Recording_Loop();
bool Recording_Stop(String* audio_filename, uint8_t** buff_start, long* audiolength_bytes, float* audiolength_sec);

String SpeechToText_ElevenLabs(String audio_filename, uint8_t* PSRAM, long PSRAM_length, String language, const char* API_Key);

void LLM_Append(String role, String content);
String OpenAI_Groq_LLM( const char* llm_open_key, bool flg_WebSearch, const char* llm_groq_key );

#pragma endregion Function prototypes





void setup() 
{     
  Serial.begin(115200); 
  Serial.setTimeout(100);
  Serial.println("SYS Serial initialized!");

  // Pushbuttons
  if (pin_SELECT  != NO_PIN) {pinMode(pin_SELECT, INPUT_PULLUP); }  
  if (pin_NEXT    != NO_PIN) {pinMode(pin_NEXT,   INPUT_PULLUP); }  
  Serial.println("SYS Digital pushbuttons set!");

  // Sensors DHT
  pinMode(DHTPIN, INPUT_PULLUP);
  dht.begin();
  Serial.println("SYS DHT initialized");

  // Sensors Soil
  pinMode(SOIL_DO, INPUT);
  Serial.println("SYS Soil initialized");

  // Sensors ALL
  Serial.println("SYS ALL SENSORS INITIALIZED");

  // Screen
  scrInit();

  // Prefs
  prefs.begin("config", false);
  sysFetchCreds();
  Serial.println("SYS Credentials fetched");
  
  // RESET
  if ((digitalRead(pin_NEXT) == LOW) && (digitalRead(pin_SELECT) == LOW))
  {
    scrDrawFullMsg("  Đang đặt lại mọi thứ...");
    wmConfig();
  }

  // Load Prefs config - Email - default NULL
  RECIPIENT_EMAIL = prefs.getString("wmstEmail", "");

  // Lấy giá trị cấu hình
  PLANT_NAME = prefs.getString("wmstPlantName", DEF_PLANT_NAME); 
  YOU_NAME = prefs.getString("wmstName", DEF_YOU_NAME);

  // Lấy giá trị cấu hình (LẦN 2) Check string rỗng
  if (PLANT_NAME == "") PLANT_NAME = DEF_PLANT_NAME;
  if (YOU_NAME == "") YOU_NAME = DEF_YOU_NAME;

  Serial.println("SYS Prefs initialized");

  // WiFiManager Init & Connect
  wmInit();
  Serial.println("SYS WF initialized");
  wmConnect();

  // Sync thời gian
  sysSyncNTP();
  Serial.println("SYS NTP sync'ed");

  // Email
  emlInit();
  Serial.println("SYS RM initialized");

  // // INIT Audio Output (via Audio.h, see here: https://github.com/schreibfaul1/ESP32-audioI2S)
  audio_play.setPinout( pin_I2S_BCLK, pin_I2S_LRC, pin_I2S_DOUT );
  audio_play.setVolume( gl_VOL_INIT );  
  Serial.println("SYS I2S Playback initialized!");

  // // Play startup sound
  sdPlaySound("STARTUP.WAV");

  // Misc. values
  SYS_START = millis();
  HOME_UPDATE_LAST = millis();
  SS_UPDATE_LAST = millis();

  if (prefs.getString("wmstFirstRun", "N") == "Y") 
  {    
    scrDrawFullMsg("  Chờ tí nhe, lên liền á...");
    prefs.putString("wmstFirstRun", "N");
    emlStart();
    emlBodyWelcome();
    emlFinalize();
  }
  
  Serial.println("SYS All set! READY!");
  scrClear();
  
  scrDrawHomeScreen();
  scrDrawPageNumber();
  scrDrawStatusBar();
}

void loop() 
{
  unsigned long comMillis = millis();

  handleButtons();

  // Update sensors
  if (comMillis - SS_UPDATE_LAST >= SS_UPDATE) {
    sysSensorsRead();
    scrDrawStatusBar();
    SS_UPDATE_LAST = comMillis;
  }

  // Timer HOME PAGE
  if (comMillis - HOME_UPDATE_LAST >= HOME_UPDATE) {
    if (currentScreenPage == 0) {
      scrDrawHomeScreen();
    }
    HOME_UPDATE_LAST = comMillis;
  } 
  
  // khó chịu 2p liền - gửi mail
  if ((comMillis - MAIL_UPDATE_LAST >= MAIL_UPDATE) && (NV_OKAY == false)) {
    MAIL_UPDATE_LAST = millis();
    
    scrDrawMailTaskScreen();
    emlStart();
    emlBody(String("Chú ý! Chú ý!!!\n") + getPlantStatus());
    emlFinalize();
    
    MAIL_UPDATE_LAST = millis();
  }

  vTaskDelay(1);
}





#pragma region Text Wrap Logic
#define MAX_TFT_PAGES 10
#define TEXT_W 230
#define LINE_HEIGHT 16

String TFT_PAGES[MAX_TFT_PAGES];
uint16_t TFT_PAGES_LEN = 0;

void scrPrepareTftPages(const char* msg, uint16_t startY) {
  TFT_PAGES_LEN = 0;
  for (uint16_t i = 0; i < MAX_TFT_PAGES; i++) TFT_PAGES[i] = "";

  const uint16_t MAX_LINES_PER_PAGE = (SCR_H - startY) / LINE_HEIGHT;
  uint16_t currentLineCount = 0;
  String word = ""; String line = "";

  int i = 0;
  while (true) {
    if (TFT_PAGES_LEN >= MAX_TFT_PAGES) {
      if (TFT_PAGES_LEN > 0) TFT_PAGES[MAX_TFT_PAGES - 1] += "...\n";
      break;
    }
    char c = msg[i];
    if (c != ' ' && c != '\0') {
      word += c; i++; continue;
    }

    if (tft.getUTF8Width(word.c_str()) > TEXT_W) {
      for (uint16_t k = 0; k < word.length(); k++) {
        String test = line + word[k];
        if (tft.getUTF8Width(test.c_str()) <= TEXT_W) {
          line = test;
        } else {
          TFT_PAGES[TFT_PAGES_LEN] += line + "\n";
          currentLineCount++;
          line = word[k];
          if (currentLineCount >= MAX_LINES_PER_PAGE) {
            TFT_PAGES_LEN++;
            if (TFT_PAGES_LEN >= MAX_TFT_PAGES) {
              TFT_PAGES[MAX_TFT_PAGES - 1] += "...\n"; return;
            }
            currentLineCount = 0;
          }
        }
      }
      word = ""; if (c == '\0') break; i++; continue;
    }

    String testLine = line + (line.length() ? " " : "") + word;
    if (tft.getUTF8Width(testLine.c_str()) <= TEXT_W) {
      line = testLine;
    } else {
      TFT_PAGES[TFT_PAGES_LEN] += line + "\n";
      currentLineCount++;
      line = word;
      if (currentLineCount >= MAX_LINES_PER_PAGE) {
        TFT_PAGES_LEN++;
        if (TFT_PAGES_LEN >= MAX_TFT_PAGES) {
          TFT_PAGES[MAX_TFT_PAGES - 1] += "...\n"; return;
        }
        currentLineCount = 0;
      }
    }
    word = ""; if (c == '\0') break; i++;
  }
  if (TFT_PAGES_LEN < MAX_TFT_PAGES && line.length()) TFT_PAGES[TFT_PAGES_LEN] += line + "\n";
  TFT_PAGES_LEN++;
  if (TFT_PAGES_LEN > MAX_TFT_PAGES) TFT_PAGES_LEN = MAX_TFT_PAGES;
}
#pragma endregion Text Wrap Logic





#pragma region LibPushButtons

// currentScreenPage: Home Vol Mail Inf Rst
#define TOTAL_PAGES 5
void handleButtons() {

  if (digitalRead(pin_NEXT) == LOW) {
    delay(500);
    Serial.println("NEXT PRESSED");
    currentScreenPage = (currentScreenPage + 1) % TOTAL_PAGES;

    switch (currentScreenPage) {
      case 1:
        scrDrawVolScreen();
        break;
      case 2:
        scrDrawMailScreen();
        break;
      case 3:
        scrDrawInfoScreen();
        break;
      case 4:
        scrDrawResetScreen();
        break;
      default:
        sysSensorsRead();
        scrDrawHomeScreen();
        break;
    }

    scrDrawPageNumber();
  }

  if (digitalRead(pin_SELECT) == LOW) {
    delay(500);
    Serial.println("SEL PRESSED");
    handleSelectAction();
  }

}

void handleSelectAction() {
  switch (currentScreenPage) {
    case 0:
      Serial.println("HOME Selected");
      break;
    case 1:
      Serial.println("VOL Selected. Volume changed");
      
      volume_level = (volume_level + 1) % volume_steps;
      audio_play.setVolume(gl_VOL_STEPS[volume_level]);
      scrDrawVolScreen();
      break;
    case 2:
      Serial.println("EMAIL Selected. Sending Email...");

      scrDrawMailTaskScreen();
      emlStart();
      emlBody(String("Dưới đây là bản báo cáo mà bạn đã yêu cầu qua lệnh SELECT nè ^^\n") + getPlantStatus());
      emlFinalize();
      scrDrawMailScreen();

      break;
    case 3:
      Serial.println("INFO Selected");
      break;
    case 4:
      Serial.println("RESET Selected. Restarting...");
      scrDrawFullMsg("  Đang tắt...");
      delay(1000);
      ESP.restart();
      break;
  }
}

#pragma endregion LibPushButtons





#pragma region LibSD

// Khởi tạo thẻ SD
void sdInit() {
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, -1); 
  if (!SD.begin(SD_CS)) {
    Serial.println("SYS SD card not found");
    flg_SD_found = false;
  } else {
    Serial.println("SYS SD Card initialized");
    flg_SD_found = true;
  }
}

// Phát âm thanh trên thẻ SD
void sdPlaySound(char* fileName) {
  if (flg_SD_found && SD.exists(fileName)) {
    Serial.println(String("SYS Playing audio file: ") + fileName);
    audio_play.connecttoFS(SD, fileName);
  } else {
    Serial.println("SYS Playing audio failed, file not found");
  }
}

#pragma endregion LibSD





// Hàm lấy credentials (ok - 09/06/2026)
void sysFetchCreds() {  
  wmSsid =             "";      // WiFi credentials are optional, declare as default for debugging, if not then configure through WM later on.
  wmPwd =              ""; 
  AUTHOR_EMAIL = "";
  AUTHOR_PASSWORD = "";
  RECIPIENT_EMAIL = "";
  OPENAI_KEY     = "SK_YOUR_OPENAI_KEY";
  ELEVENLABS_KEY = "SK_YOUR_ELEVENLABS_KEY";
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

// Lấy string để gửi email
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
    String message = "Chào bạn " + YOU_NAME + " yêu dấu! ";
    message += "\nMình là cây " + PLANT_NAME + " nhỏ của bạn nè!. ";
    
    if (!NV_OKAY) {
        message += "\nMình đang cảm thấy không ổn lắm, bạn giúp mình nhé. ";
    } else {
        message += "\nMình đang cảm thấy rất tuyệt vời! ";
    }
    
    message += "Cảm ơn bạn " + YOU_NAME + " nhiều nè :3\n";
    
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

// Trí nhớ ngắn hạn
void sysUpdateShortTermMem(String memString) {

}




#pragma region LibWifi

void wmSaveConfigCallback() {
  wmShouldSaveConfig = true;
}

void wmSaveCreds(String newSsid, String newPwd, String wmstPlantName, String wmstEmail, String wmstCall, String wmstName) {
  prefs.putString("ssid", newSsid);
  prefs.putString("pwd", newPwd);
  prefs.putString("wmstPlantName", wmstPlantName);
  prefs.putString("wmstEmail", wmstEmail);
  prefs.putString("wmstCall", wmstCall);
  prefs.putString("wmstName", wmstName);
  prefs.putString("wmstFirstRun", "Y");

  scrDrawFullMsg("  Đã cấu hình xong,\n  đang khởi động lại...");

  ESP.restart();
}

void wmReadCreds() {
  Serial.println("Wifi Reading creds...");

  wmSsid = prefs.getString("ssid", "");
  wmPwd = prefs.getString("pwd", "");
  
  delay(200);
}

void wmConfig() {
  scrDrawFullMsg("  Mở cài đặt mạng\n  Kết nối đến [NOVA]\n  và truy cập 192.168.1.4\n  để cấu hình");

  WiFiManagerParameter plnameField("plname", "Cây bạn đang trồng là? (tùy chọn)", "", 128);
  wm.addParameter(&plnameField);
  
  WiFiManagerParameter emailField("email", "Địa chỉ nhận email thông báo? (tùy chọn)", "", 128);
  wm.addParameter(&emailField);

  WiFiManagerParameter callField("callU", "Mình nên gọi bạn bằng (anh, chị, em, chú, cô, bác...)? (tùy chọn)", "", 16);
  wm.addParameter(&callField);

  WiFiManagerParameter nameField("nameU", "Tên của bạn là gì? (tùy chọn)", "", 32);
  wm.addParameter(&nameField);

  wm.startConfigPortal("NOVA");
  if (wmShouldSaveConfig) {
    wmSaveCreds(wm.getWiFiSSID(), wm.getWiFiPass(), plnameField.getValue(), emailField.getValue(), callField.getValue(), nameField.getValue());
  }
}

void wmConnect() {
  wmReadCreds();
  wm.autoConnect(wmSsid.c_str(), wmPwd.c_str());
    
  scrDrawFullMsg("  Kết nối thành công!");
}

void wmInit() {
  wm.setSaveConfigCallback(wmSaveConfigCallback);
}

#pragma endregion LibWifi





#pragma region LibScreen

void scrInit() {
  SPI.begin(LCD_SCLK, -1, LCD_MOSI, LCD_CS);
  delay(100);
  
  // // ST7735
  // display.initR(INITR_144GREENTAB);
  // display.setRotation(0);

  // For ST7789, use init(width, height)
  display.init(240, 320);   // adjust resolution (240x240, 240x320, etc.)
  display.setRotation(0);   // optional, set orientation
  display.invertDisplay(false);
  
  tft.begin(display);

  scrClear();
     
  tft.setFontMode(1);          
  tft.setFontDirection(0);   
  tft.setBackgroundColor(ST77XX_WHITE);        
  tft.setForegroundColor(ST77XX_BLACK);
  tft.setFont(u8g2_font_unifont_t_vietnamese1);  

  scrClear();

  // Vẽ màn hình khởi động
  display.drawXBitmap(56, 32, epd_bitmap_lgNovaEsp, 128, 48, ST77XX_GREEN); 
  scrDrawMessageFixed(0, 96, "  Đang khởi động hệ thống...");
  delay(50);
}

void scrClear() {
  display.fillScreen(ST77XX_WHITE); 
}

void scrDrawIcon(const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h, const uint8_t* icon, const uint16_t color) {
  display.fillRect(x, y, w, h, ST77XX_WHITE);
  display.drawXBitmap(x, y, icon, w, h, color);
}

void scrDrawMessageFixed(const uint16_t x, const uint16_t y, String msg)
{ 
  tft.setCursor(x, y + SCR_LINE_HEIGHT);
  tft.print(msg);
}

void scrDrawHomeScreen()
{
  uint16_t x = 8;  
  uint16_t y = 32;

  tft.setCursor(0, 92);
  display.fillRect(0, 20, SCR_W, 240, ST77XX_WHITE);
  display.drawXBitmap(108, 32, NV_OKAY ? epd_bitmap_icoSmile : epd_bitmap_icoStressed , 24, 24, NV_OKAY ? ST77XX_GREEN : ST77XX_RED); 

  tft.print("  Nhiệt độ: ");
  tft.print(ssTemperature);
  tft.print("°C");
  tft.println();

  tft.print("  Độ ẩm: ");
  tft.print(ssHumidity);
  tft.print(" %RH");
  tft.println();

  tft.print("  Ánh sáng: ");
  tft.print(ssLightAo);
  tft.print(" lux");
  tft.println();

  tft.print("  Đất: ");
  tft.print(soilValue ? "Khô" : "Ẩm");
  tft.println();

  tft.print("  Lần tưới gần đây:");
  tft.print("\n      ");
  tft.print(sysConvertMillisToTimeString(millis() - soilLastWaterTime, 0));
  tft.println();

  tft.print("  Thời gian phơi nắng:");
  tft.print("\n      ");
  tft.print(sysConvertMillisToTimeString(sunMillis, 1));
  tft.println();
}

void scrDrawVolScreen() {
  
  tft.setCursor(0, 92);
  display.fillRect(0, 20, SCR_W, 240, ST77XX_WHITE);
  display.drawXBitmap(108, 32, epd_bitmap_icoVol, 24, 24, ST77XX_BLUE); 
  
  tft.println("  Âm lượng: ");
  tft.print("  ");
  tft.print(volume_level * 10);
  tft.print(" %");
  tft.println();
}

void scrDrawMailScreen() {
  tft.setCursor(0, 92);
  display.fillRect(0, 20, SCR_W, 240, ST77XX_WHITE);
  display.drawXBitmap(108, 32, epd_bitmap_icoMail, 24, 24, ST77XX_GREEN); 
  
  tft.println("  Hãy bấm SELECT để gửi\n  thư điện tử trạng thái.");
}

void scrDrawMailTaskScreen() {
  tft.setCursor(0, 92);
  display.fillRect(0, 20, SCR_W, 240, ST77XX_WHITE);
  display.drawXBitmap(108, 32, epd_bitmap_icoMail, 24, 24, ST77XX_RED); 
  
  tft.println("  Chờ tí nhé!\n  Mail đang tới nè...");
}

void scrDrawInfoScreen() {
  tft.setCursor(0, 92);
  display.fillRect(0, 20, SCR_W, 240, ST77XX_WHITE);
  display.drawXBitmap(108, 32, epd_bitmap_icoSetup, 24, 24, ST77XX_BLUE); 
  
  tft.println("  Thông tin đăng ký của bạn:");
  tft.println("  Tên cây:");
  tft.print  ("    ");
  tft.println(PLANT_NAME);
  tft.println("  Email:");
  tft.print  ("    ");
  tft.println(RECIPIENT_EMAIL);
  tft.println("  Tên người dùng:");
  tft.print  ("    ");
  tft.println(YOU_NAME);
  tft.println("  Kết nối Wi-Fi:");
  tft.print  ("    ");
  tft.println(wmSsid);
}

void scrDrawResetScreen() {
  tft.setCursor(0, 92);
  display.fillRect(0, 20, SCR_W, 240, ST77XX_WHITE);
  display.drawXBitmap(108, 32, epd_bitmap_icoReplay, 24, 24, ST77XX_RED); 
  
  tft.println("  Bạn hãy bấm SELECT\n  để khởi động lại hệ thống");
}

void scrDrawStatusBar() {
  tft.setCursor(48, 14);
  display.fillRect(0, 0, SCR_W, 14, ST77XX_WHITE);
  
  tft.print(sysGetDateTimeStringShort());
}

void scrDrawPageNumber() {
  tft.setCursor(72, 304);
  display.fillRect(0, 290, SCR_W, 40, ST77XX_WHITE);
  
  tft.print("TRANG ");
  tft.print(currentScreenPage + 1);
  tft.print("/5");
}

void scrDrawFullMsg(String msg) {
  scrClear();
  tft.setCursor(0, 20);  
  tft.print(msg);
}

#pragma endregion LibScreen





#pragma region LibEmail

void emlInit()
{
    ssl_client.setInsecure();
    Serial.println("ReadyMail, version " + String(READYMAIL_VERSION));

    smtp.connect(SMTP_HOST, SMTP_PORT, smtpCb, SSL_MODE);
    if (!smtp.isConnected())
        return;

    if (AUTHENTICATION)
    {
        smtp.authenticate(AUTHOR_EMAIL, AUTHOR_PASSWORD, readymail_auth_password);
        if (!smtp.isAuthenticated())
            return;
    }
}

void emlStart()
{
    // sync time
    sysSyncNTP();

    // Clear lại object trước khi làm cái gì nha, vì có thể còn dữ liệu cũ từ mail trước, do đang đặt globalscope
    msg = SMTPMessage();
    bodyText = "";
    bodyHtml = "";

    // Prep work và config, đừng động vào đây nhé!
    msg.headers.add(rfc822_subject, "Nova system notification");
    msg.headers.add(rfc822_from, "Nova <" + String(AUTHOR_EMAIL) + ">");
    msg.headers.add(rfc822_to, "Bạn <" + String(RECIPIENT_EMAIL) + ">"); 
    msg.headers.addCustom("X-Priority", "1");
    msg.headers.addCustom("Importance", "High");
    
    msg.text.transferEncoding("quoted-printable");
    msg.html.transferEncoding("quoted-printable");
    
    if (EMBED_MESSAGE)
        msg.html.embedFile(true, "msg.html", embed_message_type_attachment);

    // HTML Header và chèn Logo
    bodyHtml = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"></head>";
    bodyHtml += "<body style=\"margin:0;padding:0;font-family:Arial,sans-serif;background-color:#ffffff;\">";
    bodyHtml += "<div style=\"text-align:center;padding:30px 0;\">";
    // Link logo 
    bodyHtml += "<img src=\"https://raw.githubusercontent.com/NguyenPhuc-bits-stdxl/xlkhkt25-plantv3/refs/heads/main/logoNovaEmail.png\" width=\"120\" alt=\"Nova Logo\">";
    bodyHtml += "</div>";
}

void emlBodyWelcome() {
    bodyText += "Chào bạn, mình là Nova!";

    bodyHtml += "<div style=\"max-width:600px;margin:0 auto;padding:0 20px;color:#333;\">";
    bodyHtml += "<h2 style=\"text-align:center;font-size:22px;margin-bottom:8px;\">";
    bodyHtml += "Chào bạn, mình là Nova</h2>";

    // Dòng mô tả nhỏ
    bodyHtml += "<p style=\"text-align:center;font-size:14px;color:#666;margin-top:0;\">";
    bodyHtml += "Xin chúc mừng, bạn vừa cấu hình thành công cây xanh yêu dấu của mình.\n";
    bodyHtml += "<br>Hãy cùng nhau xây dựng thói quen chăm sóc cây xanh nào!";
    bodyHtml += "</p>";
    
    bodyHtml += "</div>";
}

// Hàm báo không thoải mái
void emlBody(String smsg)
{
    // Fallback text
    bodyText += "Thông báo đến bạn iu!\n";
    //bodyText += sysGetSensorsString();

    // HTML Content
    bodyHtml += "<div style=\"max-width:600px;margin:0 auto;padding:0 20px;color:#333;white-space: pre-wrap;\">";
    bodyHtml += "<h2 style=\"text-align:center;font-size:22px;margin-bottom:20px;\">Thông báo đến bạn iu! :3</h2>";
    bodyHtml += "<p>Thư này được gửi đến bạn bởi hệ thống điện tử của <b>cây xanh Nova</b>.</p>";
    bodyHtml += "<div>\"" + smsg + "\"</div>";

    bodyHtml += "</div>";
}

// Chèn chân trang và finalize
void emlFinalize() {
    bodyHtml += "<div style=\"max-width:600px;margin:40px auto 20px auto;text-align:center;font-size:13px;color:#666;border-top:1px solid #eee;padding-top:20px;\">";
    bodyHtml += "<p>Cây xanh Nova là một dự án khoa học kỹ thuật của một nhóm học sinh tại trường Trung học Phổ thông Xuân Lộc.<br>";
    bodyHtml += "Ghé thăm <a href=\"https://github.com/NguyenPhuc-bits-stdxl/xlkhkt25-plantv3\" style=\"color:#007bff;text-decoration:none;\">trang GitHub của nhóm chúng mình!</a></p>";
    bodyHtml += "<p style=\"font-size:11px;color:#aaa;margin-top:20px;\">Nếu bạn không phải là người nhận của email này, bạn có thể an toàn bỏ qua nó.<br>";
    bodyHtml += "<i>If you are not the intended recipient of this email, you can safely discard it.</i></p>";
    bodyHtml += "</div></body></html>";

    // Tránh bị Gmail flag là spam, dùng CRLF thay vì LF thôi
    bodyText.replace("\n", "\r\n");
    bodyHtml.replace("\n", "\r\n");

    // Gắn nội dung
    msg.text.body(bodyText);
    msg.html.body(bodyHtml);
    
    // Check định dạng email người nhận
    if (RECIPIENT_EMAIL == "") {
        Serial.println("RM Email can't be null, task cancelled.");
        return;
    }
    if (RECIPIENT_EMAIL.indexOf("@") == std::string::npos) {
        Serial.println("RM Email doesn't sastify the format, task aborted.");
        return;
    }
    if (RECIPIENT_EMAIL.length() < 7) { //a@a.com minlength = 7
        Serial.println("RM Email too short, aborted");
        return;
    }

    // Gửi
    if (smtp.send(msg)) {
        Serial.println("SENT! Check your inbox!");
    } else {
        Serial.println("SEND FAILED! Check connection.");
    }
}

#pragma endregion LibEmail





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






// I2S recording (src. thanks to Kaloproject)
#pragma region I2SR

bool flg_is_recording = false;
bool flg_I2S_initialized = false;
// --- global vars -------------

#define RECORD_PSRAM      true   
#define RECORD_SDCARD     false      
#define AUDIO_FILE        "/record.wav"   // mandatory if RECORD_SDCARD is true: filename for the AUDIO recording                                                
#define SAMPLE_RATE       16000 
#define BITS_PER_SAMPLE   16   
#define GAIN_BOOSTER_I2S  6    

// [std_cfg]: KALO I2S_std configuration for I2S Input device (Microphone INMP441), Details see esp32-core file 'i2s_std.h' 
i2s_std_config_t  std_cfg = 
{ .clk_cfg  =   // instead of macro 'I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),'
  { .sample_rate_hz = SAMPLE_RATE,
    .clk_src = I2S_CLK_SRC_DEFAULT,
    .mclk_multiple = I2S_MCLK_MULTIPLE_256,
  },
  .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG( I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO ), 
  .gpio_cfg =   
  { .mclk = I2S_GPIO_UNUSED,
    .bclk = (gpio_num_t) I2S_SCK,
    .ws   = (gpio_num_t) I2S_WS,
    .dout = I2S_GPIO_UNUSED,
    .din  = (gpio_num_t) I2S_SD,
    .invert_flags = 
    { .mclk_inv = false,
      .bclk_inv = false,
      .ws_inv = false,
    },
  },
};

// [re_handle]: global handle to the RX channel with channel configuration [std_cfg]
i2s_chan_handle_t  rx_handle;


// [myWAV_Header]: selfmade WAV Header:
struct WAV_HEADER 
{ char  riff[4] = {'R','I','F','F'};                        /* "RIFF"                                   */
  long  flength = 0;                                        /* file length in bytes - 8 [bug fix]       <= calc at end */ 
  char  wave[4] = {'W','A','V','E'};                        /* "WAVE"                                   */
  char  fmt[4]  = {'f','m','t',' '};                        /* "fmt "                                   */
  long  chunk_size = 16;                                    /* size of FMT chunk in bytes (usually 16)  */
  short format_tag = 1;                                     /* 1=PCM, 257=Mu-Law, 258=A-Law, 259=ADPCM  */
  short num_chans = 1;                                      /* 1=mono, 2=stereo                         */
  long  srate = SAMPLE_RATE;                                /* samples per second, e.g. 44100           */
  long  bytes_per_sec = SAMPLE_RATE * (BITS_PER_SAMPLE/8);  /* srate * bytes_per_samp, e.g. 88200       */ 
  short bytes_per_samp = (BITS_PER_SAMPLE/8);               /* 2=16-bit mono, 4=16-bit stereo (byte 34) */
  short bits_per_samp = BITS_PER_SAMPLE;                    /* Number of bits per sample, e.g. 16       */
  char  dat[4] = {'d','a','t','a'};                         /* "data"                                   */
  long  dlength = 0;                                        /* data length (filelength - 44) [bug fix]  <= calc at end */
} myWAV_Header;

bool I2S_Recording_Init() 
{  
  if (I2S_LR == HIGH) {std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT;}  // manually updated, not supported via MACRO (LUCA)
  if (I2S_LR == LOW)  {std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT; }  // I2S default in MONO (STEREO creates wrong 'BOTH')
  
  // Get the default channel configuration by helper macro (defined in 'i2s_common.h')
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  
  i2s_new_channel(&chan_cfg, NULL, &rx_handle);     // Allocate a new RX channel and get the handle of this channel
  i2s_channel_init_std_mode(rx_handle, &std_cfg);   // Initialize the channel
  i2s_channel_enable(rx_handle);                    // Before reading data, start the RX channel first

  if (RECORD_SDCARD)
  { // check if SD card reader and SD card found 
    if (SD.begin()) 
    {  
       Serial.println("SD detected, used for AUDIO recording\n");       
    }
    else
    {  Serial.println("SD Card initialization failed!. Halt'ed."); 
       while(true);  // END (waiting forever) 
    }     
  }  

  flg_I2S_initialized = true;                       // all is initialized, checked in procedure Recording_Loop() 
  return flg_I2S_initialized;  
}

bool Recording_Loop() 
{
  if (!flg_I2S_initialized) return false;
  if (!flg_is_recording) {
    flg_is_recording = true;
    if (SD.exists(AUDIO_FILE)) SD.remove(AUDIO_FILE);
    File audio_file = SD.open(AUDIO_FILE, FILE_WRITE);
    audio_file.write((uint8_t*)&myWAV_Header, 44);
    audio_file.close();
  }
  
  int16_t audio_buffer[1024];
  size_t bytes_read = 0;
  i2s_channel_read(rx_handle, audio_buffer, sizeof(audio_buffer), &bytes_read, portMAX_DELAY);
  size_t values_recorded = bytes_read / 2;
  
  for (int16_t i = 0; i < values_recorded; ++i) {
    audio_buffer[i] = audio_buffer[i] * GAIN_BOOSTER_I2S;
  }
  
  File audio_file = SD.open(AUDIO_FILE, FILE_APPEND);
  audio_file.write((uint8_t*)audio_buffer, values_recorded * 2);
  audio_file.close();
  return true;
}

bool Recording_Stop(String* audio_filename, uint8_t** buff_start, long* audiolength_bytes, float* audiolength_sec) {
  if (!flg_is_recording || !flg_I2S_initialized) return false;
  flg_is_recording = false;

  File audio_file = SD.open(AUDIO_FILE, "r+");
  long filesize = audio_file.size();
  audio_file.seek(0); 
  myWAV_Header.flength = (filesize - 8); 
  myWAV_Header.dlength = (filesize - 44); 
  audio_file.write((uint8_t*)&myWAV_Header, 44);
  audio_file.close(); 

  *audio_filename = AUDIO_FILE;
  *buff_start = NULL;
  *audiolength_bytes = filesize;
  *audiolength_sec = (float)(filesize - 44) / (SAMPLE_RATE * BITS_PER_SAMPLE / 8);   
  return true;               
}

#pragma endregion I2SR





#pragma region ELTranscript

String SpeechToText_ElevenLabs(String audio_filename, uint8_t* PSRAM, long PSRAM_length, String language, const char* API_Key) {
  WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect("api.elevenlabs.io", 443)) return "";
  
  File file = SD.open(audio_filename, FILE_READ);
  size_t audio_size = file.size();
  if (audio_size == 0) { file.close(); return ""; }

  String boundary = "---011000010111000001101001";
  String bond = "--" + boundary + "\r\nContent-Disposition: form-data; ";  
  String payload_header = bond + "name=\"model_id\"\r\n\r\nscribe_v1\r\n" +
                          bond + "name=\"tag_audio_events\"\r\n\r\nfalse\r\n" +
                          bond + "name=\"file\"; filename=\"audio.wav\"\r\nContent-Type: audio/wav\r\n\r\n"; 
  String payload_end = "\r\n--" + boundary + "--\r\n";
  size_t total_length = payload_header.length() + audio_size + payload_end.length();

  client.println("POST /v1/speech-to-text HTTP/1.1");
  client.println("Host: api.elevenlabs.io");
  client.println("xi-api-key: " + String(API_Key));
  client.println("Content-Type: multipart/form-data; boundary=" + boundary);
  client.println("Content-Length: " + String(total_length));
  client.println();
  client.print(payload_header);

  uint8_t buffer[1024];
  while (file.available()) {
    size_t bytesRead = file.read(buffer, sizeof(buffer));
    if (bytesRead > 0) client.write(buffer, bytesRead);
  }
  file.close();
  client.print(payload_end);

  String response = "";
  uint32_t t_timeout = millis();
  while (response == "" && millis() - t_timeout < 8000) {
    while (client.available()) response += (char)client.read();
    delay(100);
  }
  client.stop();

  int pos_start = response.indexOf("\"text\":");
  if (pos_start > 0) {
    pos_start += 7;
    int pos_end = response.indexOf(",\"", pos_start);
    String transcript = response.substring(pos_start, pos_end);
    transcript.replace("\"", ""); transcript.trim();
    return transcript;
  }
  return "";
}

#pragma endregion ELTranscript





#pragma region OpenAIChatModels

#define RSTR_ASSISTANT "assistant"
#define RSTR_USER "user"
#define RSTR_DEV "developer"
#define RSTR_SYS "system"

#define TIMEOUT_LLM  10          // preferred max. waiting time [sec] for LMM AI response     

// newft1: Xây dựng các hàm append
void LLM_Append(String role, String content) {
    content.replace( "\"", "\\\"" );  // to avoid any ERROR (if user enters any " -> convert to \")

    MESSAGES += ((MESSAGES == "") ? "{" : ",\n\n{");
    MESSAGES += "\"role\": \""; 
    MESSAGES += role;
    MESSAGES += "\", \"content\": \"";
    MESSAGES += content;
    MESSAGES += "\"}";
    Serial.print("LLM New prompt appended (role ");
    Serial.print(role);
    Serial.println(")");
    
    Serial.println( ">> MESSAGES LENGTH: " + (String) MESSAGES.length() );   
    Serial.println( ">> FREE HEAP: " + (String) ESP.getFreeHeap() );             
}

// Hàm này giờ chỉ có chức năng gửi, các vấn đề append prompt dùng qua hàm LLM_Append()
String OpenAI_Groq_LLM( const char* llm_open_key, bool flg_WebSearch, const char* llm_groq_key )
{   
    // =====- Prep work done. Now CONNECT to Open AI or Groq Server (on INIT or after closed or lost connection) ================

    uint32_t t_start = millis(); 

    String LLM_Response = "";                                         // used for complete API response
    String Feedback = "";                                             // used for extracted answer
    String LLM_server, LLM_entrypoint, LLM_model, LLM_key;            // NEW: using vars to be independet of server/models
    
    // Dùng LLM của OpenAI
    LLM_server =        "api.openai.com";                          // OpenAI: https://platform.openai.com/docs/pricing
    LLM_entrypoint =    "/v1/chat/completions";           
    if (!flg_WebSearch) LLM_model= "gpt-4.1-nano";                 // low cost, powerful, fast (response latency ~ 1.5 sec)  
    if (flg_WebSearch)  LLM_model= "gpt-4o-mini-search-preview";   // realtime websearch model (higher latency ~ 3-5 sec)     
    LLM_key =           llm_open_key;
    
    /*static*/ WiFiClientSecure client_tcp;    // [UPDATE]: removed static to free up HEAP (start with new LLM socket always)
    
    if ( !client_tcp.connected() )
    {  
       Serial.println("> Initialize LLM AI Server connection ... ");
       client_tcp.setInsecure();
       if (!client_tcp.connect( LLM_server.c_str() , 443)) 
       { Serial.println("\n* ERROR - WifiClientSecure connection to Server failed!");
         client_tcp.stop(); /* might not have any effect, similar with client.clear() */
         return ("");   // in rare cases: WiFiClientSecure freezed (with older libraries) 
       }
       Serial.println("Done. Connected to LLM AI Server.");
    }
    client_tcp.setNoDelay(true);     // NEW: immediately flush after each write(), means disable TCP nagle buffering.
                                     // Idea: might increase performance a bit (20-50ms per write) [default is false]
 
    
    // ------ Creating the Payload: ---------------------------------------------------------------------------------------------
    
    // == model CHAT: creating a user prompt in format:  >"messages": [MESSAGES], {"role":"user", "content":"what means AI?"}]<
    // recap: Syntax of entries in global var MESSAGES [e.g.100K]: 
    // > {"role": "system", "content": "you are a helpful assistant"},\n
    //   {"role": "user", "content": "how are you doing?"},\n
    //   {"role": "assistant", "content": "Thanks for asking, as an AI bot I do not have any feelings"} <
    //
    // for better readiability we write \" instead \" and replace below in code:

    String request_Prefix, request_Postfix, request_LEN;

    request_Prefix  =     "{\"model\":\"" + LLM_model + "\", \"messages\":[";    // appending old MESSAGES       
    
    if (!flg_WebSearch)   // DEFAULT parameter for classic CHAT completion models                                     
    {  request_Postfix =  "],\n\"temperature\":0.7, \"max_tokens\":512, \"presence_penalty\":0.6, \"top_p\":1.0}";                                           
    }
    if (flg_WebSearch)    // NEW: parameter for web search models
    {  request_Postfix =  "],\n\"response_format\": {\"type\": \"text\"}, "; 
       request_Postfix += "\"web_search_options\": {\"search_context_size\": \"low\", ";
       request_Postfix += "\"user_location\": {\"type\": \"approximate\", \"approximate\": ";
       request_Postfix += "{\"country\": \"\", \"city\": \"Việt Nam\"}}}, ";
       request_Postfix += "\"store\": false}";
    }  
          
    request_LEN = (String) (MESSAGES.length() + request_Prefix.length() + request_Postfix.length()); 

 
    // ------ Sending the request: ----------------------------------------------------------------------------------------------

    uint32_t t_startRequest = millis(); 
    
    client_tcp.println( "POST " + LLM_entrypoint + " HTTP/1.1" );   
    client_tcp.println( "Connection: close" ); 
    client_tcp.println( "Host: " + LLM_server );                   
    client_tcp.println( "Authorization: Bearer " + LLM_key );  
    client_tcp.println( "Content-Type: application/json; charset=utf-8" ); 
    client_tcp.println( "Content-Length: " + request_LEN ); 
    client_tcp.println(); 
    client_tcp.print( request_Prefix );    // detail: no 'ln' because Content + Postfix will follow)  
   
    // Now sending the complete MESSAGES chat history (String) .. 2 options (from own experiences in testing & user feedback): 
    // 1. either with one single 'client_tcp.print( MESSAGES );' .. works well on my ESP32 (even a 100 KB String works flawless)
    // 2. or sending in chunks (background: some user had issues if MESSAGES size exceeds 8K .. so we use option (2) here:  

    /* client_tcp.print( MESSAGES );       // Option 1: sending complete MESSAGES history once (works well on my ESP32)  */    
    
    // Option 2 (NEW): sending MESSAGES (text) in chunks (prevents the TLS layer from choking on big payloads)
    const size_t CHUNK_SIZE = 1024;        // 1K just as example (all below 8K should work also on older ESP32)        
    for (size_t i = 0; i < MESSAGES.length();  i += CHUNK_SIZE) 
    {   client_tcp.print(MESSAGES.substring(i, i +  CHUNK_SIZE)); 
    }

    // final Postfix (then all is done):
    client_tcp.println( request_Postfix );    
                 
    
    // ------ Waiting the server response: --------------------------------------------------------------------------------------

    LLM_Response = "";
    while ( millis() < (t_startRequest + (TIMEOUT_LLM*1000)) && LLM_Response == "" )  
    { Serial.print(".");                   // printed in Serial Monitor always    
      delay(250);                          // waiting until tcp sends data 
      while (client_tcp.available())       // available means: if a char received then read char and add to String
      { char c = client_tcp.read();
        LLM_Response += String(c);
      }       
    } 
    if ( millis() >= t_startRequest + (TIMEOUT_LLM*1000) ) 
    {  Serial.print("\n*** LLM AI TIMEOUT ERROR - forced TIMEOUT after " + (String) TIMEOUT_LLM + " seconds");      
    } 
    client_tcp.stop();                     // closing LLM connection always (observation: otherwise OpenAI TTS won't work)
        
    uint32_t t_response = millis();  

  
    // ------ Now extracting clean message for return value 'Feedback': --------------------------------------------------------- 
    // 'talkative code below' but want to make sure that also complex cases (e.g. " chars inside the response are working well)
    
    int pos_start, pos_end, pos_mem;                                     // proper way to extract tag "text", talkative but correct
    bool found = false;                                         // supports also complex tags, e.g.  > "What means \"RGB\"?" < 
    pos_start = LLM_Response.indexOf("\"content\":");           // search tag ["content": "Answer..."]   
    if (pos_start > 0)                                         
    { pos_start = LLM_Response.indexOf("\"", pos_start + strlen("\"content\":")) + 1;   // search next " -> now points to 'A'
      pos_end = pos_start + 1;

      while (!found)                                        
      { found = true;                                           // avoid endless loop in case no " found (won't happen anyhow)  
        pos_end = LLM_Response.indexOf("\"", pos_end);          // search the final " ... but ignore any rare \" inside the text!  
        if (pos_end > 0)                                        // " found -> Done.   but:  
        {  // in case we find a \ before the " then proceed with next search (because it was a \marked " inside the text)    
           if (LLM_Response.substring(pos_end -1, pos_end) == "\\") { found = false; pos_end++; }
        }           
      }            

      // 06/01: tách MEM
      pos_mem = LLM_Response.indexOf("[MEM]", pos_start);
      if (pos_mem == -1) pos_mem = SYSINT_PINF; // đặt thành số cực lớn để tránh lỗi tách bậy
    }                           
    if( pos_start > 0 && (pos_end > pos_start) )
    { 
      // 06/01: lấy pos_mem nếu có, còn không thì mặc định pos_end
      Feedback = LLM_Response.substring(pos_start, pos_mem < pos_end ? pos_mem : pos_end);  // store cleaned response into String 'Feedback'   

      Feedback.trim();     
    }

    
    // ------ APPEND current I/O chat (UserRequest & Feedback) at end of var MESSAGES -------------------------------------------
    // Chèn vào lịch sử MESSAGES

    if (Feedback != "")                                          // we always add both after success (never if error) 
    { 
      // String NewMessagePair = ",\n\n";                           // NEW in Sept. 2025: \n\n instead \n (for spaces in email)  
      // if(MESSAGES == "") { NewMessagePair = ""; }                // if messages empty we remove leading ,\n  
      // NewMessagePair += "{\"role\": \"user\", \"content\": \""      + UserRequest + "\"},\n"; 
      // NewMessagePair += "{\"role\": \"assistant\", \"content\": \"" + Feedback    + "\"}"; 
      
      // newft1: thay bằng hàm LLM_Append
      LLM_Append(RSTR_ASSISTANT, Feedback); // phản hồi chiều lại luôn là assistant

      // here we construct the CHAT history, APPENDING current dialog to LARGE String MESSAGES
      // MESSAGES += NewMessagePair;       
    }  
             
    // ------ finally we clean Feedback, print DEBUG Latency info and return 'Feedback' String ----------------------------------
    
    // trick 17: here we break \n into real line breaks (but in MESSAGES history we added the original 1-liner)
    if (Feedback != "")                                              
    {  Feedback.replace("\\n", "\n");                            // LF issue: replace any 2 chars [\][n] into real 1 [\nl]  
       Feedback.replace("\\\"", "\"");                           // " issue:  replace any 2 chars [\]["] into real 1 char ["]
       Feedback.replace("\n\n", "\n");                           // NEW: remove empty lines in Serial Monitor
       Feedback.trim();                                          // in case of some leading spaces         
    }

    Serial.println( "\n---------------------------------------------------" );
    //DebugPrintln( "====> Total Response: \n" + LLM_Response + "\n====");   // ## uncomment to see complete server response */  
    Serial.println( "AI LLM server/model: [" + LLM_server + " / " + LLM_model + "]" );
    Serial.println( "-> Latency LLM AI Server (Re)CONNECT:          " + (String) ((float)((t_startRequest-t_start))/1000) );   
    Serial.println( "-> Latency LLM AI Response:                    " + (String) ((float)((t_response-t_startRequest))/1000) );   
    Serial.println( "=> TOTAL Duration [sec]: ..................... " + (String) ((float)((t_response-t_start))/1000) ); 
    Serial.println( "---------------------------------------------------" );   
    Serial.println( "\nLLM >" ); 
    
    // 06/01: update TRÍ NHỚ NGẮN HẠN
    sysUpdateShortTermMem(pos_mem == SYSINT_PINF ? "Mình rất háo hức muốn gặp người bạn của mình." : LLM_Response.substring(pos_mem, pos_end));

    Serial.println(Feedback);
    // and return extracted feedback
    return ( Feedback );                           
} 

#pragma endregion OpenAIChatModels
