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
#define SD_SCK 8
#define SD_MISO 8
#define SD_MOSI 8
#define SD_CS 8

// (Unused) ASAIR Humidity + Temp (I2C)
#define ASAIR_SDA 47
#define ASAIR_SCL 48

// (Unused) BH1750 Light Sensor (I2C, shared bus)
#define BH1750_SDA 47
#define BH1750_SCL 48

// (Unused) Human Presence Sensor HLK-LD2410B (Digital OUT)
#define PRESENCE_OUT 6

#pragma endregion Pinout

#pragma region Libs

#include <Arduino.h>
#include "icons.h"
// #include "driver/i2s_std.h"  
#include <WiFi.h>
#include <WiFiManager.h>       // Dependencies: main, lib_audio_recording, lib_audio_transcription, lib_openai_groq_chat, lib_wifi
#include <Preferences.h>       // Dependencies: lib_sys, lib_wifi
#include <DHT.h>               // Dependencies: lib_sys
#include <time.h>
// #include <Audio.h>             // Audio library (for playback)
// #include <SD.h>

#pragma endregion Libs

#pragma region Misc.

#define SYSINT_PINF 2147483640 // very large number

long long SYS_START; // Timestamp of system's startup

// (DEPENDENCIES REQUIRED)

struct tm timeinfo; // from lib_sys.ino (timekeeping)

// Audio audio_play;   // Audio.h object
int gl_VOL_INIT = 21; // Default volume at init (max = 21)
int gl_VOL_STEPS[] = { 0, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21 }; // Volume values array
int volume_level = 10;
int volume_steps = 11;

Preferences prefs;  // Dependencies: lib_wifi, lib_sys

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

// // SYSTEM PAGES // //
// // 0Home 1Vol 2Mail 3Info 4Reset
int currentScreenPage = 0;

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
const char* DEF_YOU_CALL = "bạn";
const char* DEF_YOU_NAME = "(không rõ)";

String PLANT_NAME = DEF_PLANT_NAME;
String YOU_CALL = DEF_YOU_CALL;
String YOU_NAME = DEF_YOU_NAME;

float THRESH_TEMP_MIN  = 10.0f;
float THRESH_TEMP_MAX  = 40.0f;
float THRESH_HUMID_MIN = 55.0f;
float THRESH_HUMID_MAX = 90.0f;
float THRESH_LIGHT_MIN = 400.0f;
float THRESH_LIGHT_MAX = 22000.0f;

#define THRESH_LIGHT 2000

#define SCR_LINE_HEIGHT 14
#define SCR_W 240
#define SCR_H 320

#pragma region Signatures

void handleButtons();
void handleSelectAction();

void sysSensorsRead();
void sysSyncNTP();
String sysGetDateTimeString();
String sysGetDateTimeStringShort();
String sysConvertMillisToTimeString(unsigned long value, uint8_t mode);

float GetLightValue(float AoValue);

void scrInit();
void scrClear();
void scrDrawIcon(const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h, const uint8_t* icon, const uint16_t color);
void scrDrawMessageFixed(const uint16_t x, const uint16_t y, String msg);
void scrDrawHomeScreen();

void scrDrawHomeScreen();
void scrDrawVolScreen();
void scrDrawMailScreen();
void scrDrawInfoScreen();
void scrDrawResetScreen();
void scrDrawStatusBar();
void scrDrawPageNumber();
void scrDrawMailTaskScreen();

void wmSaveConfigCallback();
void wmSaveCreds(String newSsid, String newPwd, String wmstPlantName, String wmstEmail, String wmstCall, String wmstName);
void wmReadCreds();
void wmConfig();
void wmConnect();
void wmInit();

void sysFetchCreds();

#pragma endregion Signatures





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
  YOU_CALL = prefs.getString("wmstCall", DEF_YOU_CALL);
  YOU_NAME = prefs.getString("wmstName", DEF_YOU_NAME);

  // Lấy giá trị cấu hình (LẦN 2) Check string rỗng
  if (PLANT_NAME == "") PLANT_NAME = DEF_PLANT_NAME;
  if (YOU_CALL == "") YOU_CALL = DEF_YOU_CALL;
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
  // audio_play.setPinout( pin_I2S_BCLK, pin_I2S_LRC, pin_I2S_DOUT );
  // audio_play.setVolume( gl_VOL_INIT );  
  // Serial.println("SYS I2S Playback initialized!");

  // // Play startup sound
  // sdPlaySound("STARTUP.WAV")

  // Misc. values
  SYS_START = millis();
  HOME_UPDATE_LAST = millis();
  SS_UPDATE_LAST = millis();

  if (prefs.getString("wmstPlantName", "N") == "Y") 
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

// currentScreenPage: Home Vol Mail Inf Rst
#define TOTAL_PAGES 5
void handleButtons() {

  if (digitalRead(pin_NEXT) == LOW) {
    delay(500);
    Serial.println("NEXT PRESSED");
    currentScreenPage = (currentScreenPage + 1) % TOTAL_PAGES;

    switch (currentScreenPage) {
      case 0:
        sysSensorsRead();
        scrDrawHomeScreen();
        break;
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
      // audio_play.setVolume(gl_VOL_STEPS[volume_level]);
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