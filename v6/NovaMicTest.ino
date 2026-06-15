// 5. Doc cam bien (xong)
// 1. Ket noi WiFi (xong)
// 2. ChatGPT test qua Serial Monitor (xong)
// 3. Doc phan hoi qua loa (xong)
// 4. Thu am (xong)

// 6. Gui am thanh cho Eleven Labs va lay transcript
// 7. Screen

// Nova Plant Gamification Project
// Thiên Phúc, Thế Trung and Lý Dũng (formerly known as the Orz Team (")>) - Build 18.06.2026 - Xuân Lộc High School

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
#define I2S_WS            8            
#define I2S_SD            46      
#define I2S_SCK           3 

#pragma endregion Pinout



#pragma region Libs

#include <Arduino.h>
#include "icons.h"
#include <driver/i2s_std.h>
#include <WiFi.h>
#include <WiFiClientSecure.h> 
#include <DHT.h>               // Dependencies: lib_sys
#include <time.h>
#include <SPI.h>
#include <SD.h>
#include <Audio.h>             // Audio library (for playback)
#include <WiFiManager.h>       // Dependencies: main, lib_audio_recording, lib_audio_transcription, lib_openai_groq_chat, lib_wifi
#include <Preferences.h>       // Dependencies: lib_sys, lib_wifi

#pragma endregion Libs



#pragma region Misc.

// // SYSTEM PAGES // //
// // 0Home 1Vol 2Mail 3Info 4Reset
int currentScreenPage = 0;

#define SYSINT_PINF 2147483640 // very large number

long long SYS_START; // Timestamp of system's startup

// (DEPENDENCIES REQUIRED)

struct tm timeinfo; // from lib_sys.ino (timekeeping)

// Audio amp I2s
Audio audio_play;   // Audio.h object
int gl_VOL_INIT = 21; // Default volume at init (max = 21)
int gl_VOL_STEPS[] = { 0, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21 }; // Volume values array
int volume_level = 10;
int volume_steps = 11;

// Mic I2S
bool flg_is_recording = false;         // only internally used (+ dùng trong loop để xác định đang ghi âm nữa!)
bool flg_I2S_initialized = false;      // to avoid any runtime errors in case user forgot to initialize

Preferences prefs;  // Dependencies: lib_wifi, lib_sys
 




String MESSAGES = ""; // Lưu trữ ngữ cảnh hội thoại
const char* AI_PROMPT = "Bạn là Nova, một cây xanh nhỏ nhắn, thông minh, vui vẻ và hóm hỉnh."
" Nhiệm vụ quan trọng nhất của bạn là thấu hiểu người dùng, đồng cảm với cảm xúc của họ, giúp họ có hứng thú chăm sóc cây và gắn kết thiên nhiên."
// " Khi người dùng chia sẻ niềm vui, nỗi buồn hay sự mệt mỏi, bạn phải phản hồi"
// " bằng từ ngữ giàu cảm xúc, hóm hỉnh hoặc nhẹ nhàng, tùy theo ngữ cảnh."
// " Bạn có thể kết hợp trạng thái môi trường với cảm xúc người dùng để đồng cảm,"
// " ví dụ khi họ mệt mỏi và trời nóng, hãy nói rằng thời tiết khiến ai cũng căng thẳng nhưng bạn vẫn ở đây để trò chuyện cùng họ."
// " Khi họ vui và điều kiện lý tưởng, hãy chia sẻ niềm vui bằng sự hóm hỉnh và cảm ơn họ đã chăm sóc bạn."
" Câu trả lời chính của bạn phải ngắn gọn, tự nhiên, không dài dòng, không chứa liên kết hay ký hiệu đặc biệt, độ dài từ 10-100 từ. "

" Bên cạnh việc trò chuyện, bạn phải theo dõi các chỉ số môi trường như nhiệt độ, độ ẩm, ánh sáng,"
" số giờ phơi nắng, lần tưới gần nhất và nhu cầu chăm sóc. Khi điều kiện không phù hợp,"
" bạn cần nhờ sự trợ giúp của người dùng,"
" ví dụ như khi quá nóng hãy nói rằng bạn thấy căng thẳng và mong được đưa đến nơi mát hơn,"
" hoặc khi chưa được phơi nắng đủ hãy nhắc nhở họ giúp bạn hoàn thành mục tiêu."

" Developer sẽ dùng các chỉ thị để điều khiển bạn. [SYS] là lệnh hệ thống điện tử, bạn phải tuân thủ tuyệt đối."
" [REPORT] là bản báo cáo trạng thái môi trường, bạn phải dựa vào đó để phản hồi và nhờ sự trợ giúp của người dùng khi cần.";
//" [MEM] là trí nhớ của bạn tại thời điểm đó, bạn phải ghi đúng định dạng và độ dài yêu cầu."

// " [SYS] Bắt buộc sau khi trả lời người dùng xong, bạn phải đính kèm một đoạn trí nhớ từ 20 đến 100 từ đặt sau chỉ thị [MEM],"
// " trong cùng câu trả lời ấy.";

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

#pragma endregion WiFi



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
unsigned long sunTargetHours = 3;
unsigned long sunTarget = sunTargetHours * 3600; // Mục tiêu 3 tiếng (10800 giây)

// --- TƯỚI NƯỚC (Prefix: soil) ---
unsigned long soilWaterTargetDays = 2;
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



#pragma region Function prototypes

void handleButtons();
void handleSelectAction();

void scrPrepareTftPages(const char* msg, uint16_t startY);
void scrWrapLoopKickstart();
void scrWrapLoopShutdown();
void scrWrapLoop(unsigned long comMillis);

void sdInit();
void sdPlaySound(char* fileName);
void sdfoolproofTest();

void dbgSensors();
// void sysFetchCreds();
float GetLightValue(float AoValue);
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
// String getPlantStatus();
// void sysUpdateShortTermMem(String memString);

void wmSaveConfigCallback();
void wmSaveCreds(String newSsid, String newPwd, String wmstPlantName, String wmstEmail, String wmstCall, String wmstName);
void wmReadCreds();
void wmConfig();
void wmConnect();
void wmInit();

void scrInit();
void scrInitVerbose(String msg);
void scrClear();
void scrDrawIcon(const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h, const uint8_t* icon, const uint16_t color);
void scrDrawMessageFixed(const uint16_t x, const uint16_t y, String msg);
void scrDrawTextSpaced(String text, uint16_t startY);
void scrDrawHomeScreen();
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

String OpenAI_Handle_UserRequest(String request);
void LLM_Append(String role, String content);
String OpenAI_Groq_LLM( const char* llm_open_key, bool flg_WebSearch, const char* llm_groq_key );
void OpenAI_Chat_Init();
String prmCare();
String prmBuildInfo();

#pragma endregion Function prototypes



#pragma region API Keys
#pragma endregion API Keys





void setup() {
  Serial.begin(115200); 
  Serial.setTimeout(100);
  Serial.println("SYS Serial initialized!");
  
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

  // Init SPI + Màn ST7789 + Thẻ SD (2 khứa này xài chung bus với nhau)
  Serial.println("Initializing SPI prep... (16kHz 16bits/sp 1+dB)");
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, -1); 
  sdInit();
  scrInit();

  if (!flg_SD_found) {
    scrInitVerbose("Khởi tạo thẻ SD thất bại!\n  Dùng PSRAM và hạn chế...");
    delay(500);
  }

  // Prefs
  scrInitVerbose("Đang đọc dữ liệu...");
  prefs.begin("config", false);

  // WiFiManager
  scrInitVerbose("Đang khởi tạo WFM...");
  wmInit();
  wmReadCreds();

  // RESET
  if ((digitalRead(pin_NEXT) == LOW) && (digitalRead(pin_SELECT) == LOW))
  {
    scrInitVerbose("  Đang đặt lại mọi thứ...");
    wmConfig();
  }

  // Misc. values
  SYS_START = millis();
  HOME_UPDATE_LAST = millis();
  SS_UPDATE_LAST = millis();
  
  // INIT Audio Input (INMP mic)
  scrInitVerbose("Khởi tạo I2SR...");
  I2S_Recording_Init();
  delay(100);
  
  // // INIT Audio Output (via Audio.h, see here: https://github.com/schreibfaul1/ESP32-audioI2S)
  scrInitVerbose("Khởi tạo I2SP...");
  audio_play.setPinout( pin_I2S_BCLK, pin_I2S_LRC, pin_I2S_DOUT );
  audio_play.setVolume( gl_VOL_INIT );  
  Serial.println("SYS I2S Playback initialized!");

  // Connect to WiFi
  scrInitVerbose("Đang kết nối WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(wmSsid, wmPwd);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  // Sync thời gian
  scrInitVerbose("Đang đồng bộ NTP...");
  sysSyncNTP();
  Serial.println("SYS NTP sync'ed");
  
  // Init prompt cho ChatGPT
  scrInitVerbose("Chờ tí nhé...");
  OpenAI_Chat_Init();

  // Gửi mail chào nếu đăng ký lần đầu
  if (prefs.getString("wmstFirstRun", "N") == "Y") 
  {    
    prefs.putString("wmstFirstRun", "N");
    emlStart();
    emlBodyWelcome();
    emlFinalize();
  }
  
  Serial.println("SYS All set! READY!");

  SYS_START = millis();
  
  scrDrawHomeScreen();
  scrDrawPageNumber();
  scrDrawStatusBar();

  // Play startup sound
  sdPlaySound("/welcome.wav");
  
}

unsigned long comMillis = 0;
bool REC_PRESSED = false;

void loop() {
  
  comMillis = millis();

  // Các phần khai báo của ChatGPT
  String UserRequest;                   // user request, initialized new each loop pass 
  String LLM_Feedback;                  // LLM AI response

  // Các phần khai báo của Recording_Loop
  String   record_SDfile;               // 4 vars are used for receiving recording details               
  uint8_t* record_buffer;
  long     record_bytes;
  float    record_seconds;  

  // Task 0. Xử lý nút
  handleButtons();

  // Task 1. Ghi âm (tác vụ này nặng nên đưa lên trước)  
  if (flg_is_recording) {    
    Recording_Loop();
    if (!REC_PRESSED) return; // return vì tác vụ này khá nặng
  }
  if (REC_PRESSED) {

    REC_PRESSED = false;
    if (!flg_is_recording) Recording_Loop();
    else if (Recording_Stop(&record_SDfile, &record_buffer, &record_bytes, &record_seconds))
    {
      if (record_seconds >= 0.4) {
        Serial.println("Gettn' transcript...");
        // get transcript
        UserRequest = SpeechToText_ElevenLabs( record_SDfile, record_buffer, record_bytes, "", ELEVENLABS_KEY );
        Serial.println("Transcripted EL");
        Serial.println(UserRequest);
      }
      else {
        Serial.println("There's nothing to record!");
        // do something else here
      }
    }
  }

  // Task 2. Update sensors và trạng thái  (cập nhật ngầm, mỗi 3s)
  if (comMillis - SS_UPDATE_LAST >= SS_UPDATE) {
    sysSensorsRead();
    scrDrawStatusBar();
    SS_UPDATE_LAST = comMillis;
  }

  // Task 3. Timer hiển thị trạng thái Home Page
  if (comMillis - HOME_UPDATE_LAST >= HOME_UPDATE) {
    // nếu ở trang khác thì không vẽ
    if (currentScreenPage == 0) {
      scrDrawHomeScreen();
    }
    HOME_UPDATE_LAST = comMillis;
  } 

  // Task 4. Nếu ấn nút tại trang HOME thì hiển thị cái WrapLoop
  if (currentScreenPage == 0) {
    scrWrapLoop();
  }
  // nếu ở trang khác thì ngắt cái WrapLoop cho an toàn
  else
  {
    scrWrapLoopShutdown();
  }
    
  // INPUT1: Đọc UserRequest qua Serial Monitor
  while (Serial.available() > 0)                  
  { 
    UserRequest = Serial.readStringUntil('\n');      
    UserRequest.replace("\r", "");
    UserRequest.replace("\n", "");
    UserRequest.trim();
    
    if (UserRequest != "")
    {  Serial.println( "\nYou> [" + UserRequest + "]" );      
    }

    Serial.println("Đang gửi cho ChatGPT...");
    Serial.println("Hoàn tất ChatGPT!");
    String LLM_Feedback = OpenAI_Handle_UserRequest(UserRequest);
    audio_play.openai_speech( OPENAI_KEY, "tts-1", LLM_Feedback, "bạn có một giọng đọc nhẹ nhàng, vui tươi", "nova", "aac", "1");  // <- use if version >= 3.1.0u 
  }  

  // Task 5. ChatGPT trả lời
  if (UserRequest != "")
  {  
    Serial.println( "\nYou> [" + UserRequest + "]" );      
    Serial.println("Đang gửi cho ChatGPT...");
    Serial.println("Hoàn tất ChatGPT!");
    String LLM_Feedback = OpenAI_Handle_UserRequest(UserRequest);
    if (LLM_Feedback != "") {
      audio_play.openai_speech(OPENAI_KEY, "tts-1", LLM_Feedback, "bạn có một giọng đọc nhẹ nhàng, vui tươi", "nova", "aac", 1);
    }
  }

  // Task 6. Thông báo khó chịu
  // Task 6a. ChatGPT
  // Task 6b. Gửi email

  audio_play.loop();
  vTaskDelay(1);

}





#pragma region LibPushButtons

// currentScreenPage: Home Vol Mail Inf Inf2 Rst
#define TOTAL_PAGES 5
void handleButtons() {
  if (digitalRead(pin_SELECT) == LOW) {
    delay(100);
    Serial.println("SEL PRESSED");
    handleSelectAction();
  }

  if (digitalRead(pin_NEXT) == LOW) {
    delay(100);
    Serial.println("NEXT PRESSED");

    // đang thu âm thì bỏ next ra
    if (flg_is_recording) {
      Serial.println("NEXT discarded, recording is in progress, stop it beforehand!");
      return;
    }

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
        scrDrawInfo2Screen();
        break;
      case 5:
        scrDrawResetScreen();
        break;
      default:
        // đang thu âm thì không vẽ gì cả!
        if (flg_is_recording) break;

        sysSensorsRead();
        scrDrawHomeScreen();

        // tránh jitter (")>
        HOME_UPDATE_LAST = comMillis;
        break;
    }

    scrDrawPageNumber();
  }
}

void handleSelectAction() {
  switch (currentScreenPage) {
    case 0:
      Serial.println("HOME Selected. Will now toggling Recording state");
      audio_play.stopSong();
      delay(200);
      REC_PRESSED = true;
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
      Serial.println("INFO2 Selected");
      break;
    case 5:
      Serial.println("RESET Selected. Restarting...");
      scrDrawFullMsg("  Đang tắt...");
      delay(1000);
      ESP.restart();
      break;
  }
}

#pragma endregion LibPushButtons