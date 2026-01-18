#pragma region Libraries Macros

#include <Arduino.h>
#include "lib_icon.h"
#include "driver/i2s_std.h"  
#include <WiFi.h>
#include <WiFiManager.h>       // Dependencies: main, lib_audio_recording, lib_audio_transcription, lib_openai_groq_chat, lib_wifi
#include <WiFiClientSecure.h>  // Dependencies: *
#include <Preferences.h>       // Dependencies: lib_sys, lib_wifi
#include <DHT.h>               // Dependencies: lib_sys
#include <time.h>

// Screen libraries
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
// #include <Adafruit_ST7735.h> // uncomment this if ST7735 is used
#include <U8g2_for_Adafruit_GFX.h>

// Audio libraries (for playback)
#include <Audio.h>

// Debus macros
bool    DEBUG = true;       
#define DebugPrint(x);           if(DEBUG){Serial.print(x);}   
#define DebugPrintln(x);         if(DEBUG){Serial.println(x);}

#pragma endregion Libraries Macros

#pragma region Pinout

#define NO_PIN -1

#define pin_I2S_DOUT 16
#define pin_I2S_LRC 8   
#define pin_I2S_BCLK 3

#define pin_VOL_BTN 41      
#define pin_RECORD_BTN 38   

#define pin_DHT 40
#define pin_LDR_AO 4 // ADC

#define LCD_CS 10
#define LCD_DC 11
#define LCD_SCLK 14
#define LCD_MOSI 13
#define LCD_RST 12

// ASAIR Humidity + Temp (I2C)
#define ASAIR_SDA 47
#define ASAIR_SCL 48

// BH1750 Light Sensor (I2C, shared bus)
#define BH1750_SDA 47
#define BH1750_SCL 48

// Soil Moisture Sensor (Analog)
#define SOIL_AO 5

// Human Presence Sensor HLK-LD2410B (Digital OUT)
#define PRESENCE_OUT 6

#pragma endregion Pinout

#pragma region Global objects

// --- CREDENTIALS ---
// Define your own in [private_credentials.ino]
// the below is deprecated and will be fetched again on startup
// also includes WiFiManager's SSID and Passkey
const char* OPENAI_KEY = "";
const char* GROQ_KEY = "";
const char* ELEVENLABS_KEY = "";
const char* DEEPGRAM_KEY = "";

Audio audio_play;   // Audio.h object

int gl_VOL_INIT = 21;
int gl_VOL_STEPS[] = { 0, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21 };

Preferences prefs;  // Dependencies: lib_wifi, lib_sys

WiFiManager wm;     // WiFi global objects - Dependencies: main, lib_wifi
bool wmShouldSaveConfig = false;
String wmSsid;
String wmPwd;

// Adafruit_ST7735 display = Adafruit_ST7735(LCD_CS, LCD_DC, LCD_RST); // uncomment this if ST7735 is used
Adafruit_ST7789 display = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);
U8G2_FOR_ADAFRUIT_GFX tft;

#define ICO_ACT_DIMM 24
#define ICO_START_X 108
#define ICO_START_Y 2
#define MSG_START_X 2
#define MSG_START_Y 30

#define WEB_SEARCH_USER_CITY     "Đồng Nai, Vietnam" // optional (recommended): User location optimizes OpenAI 'web search'

String gl_voice_instruct;        // internally used for forced 'voice character' (via command "VOICE", erased on friend changes)

long long SYS_START;
bool startupSoundPlayed = false;
bool startupSoundStopped = false;
bool dbgUncomfToggled = false;

bool scrxListening = false;

#pragma endregion Global objects

#pragma region Strings

const char* systrReset = "Đặt lại\ncấu hình.";

const char* wmBroadcast = "NOVA";
const char* wmsPleaseConfig = "Kết nối\n[NOVA] và\ntruy cập\n192.168.4.1.";
const char* wmsSaveRequest = "Đang nhận...";
const char* wmsSaveSuccess = "Thiết lập\nthành công\nChờ...";
const char* wmsReading = "Đang đọc\ncấu hình...";
const char* wmsEstablished = "Kết nối\nthành công!";

#pragma endregion Strings

#pragma region Sensors

const int DHTPIN = pin_DHT;
const int DHTTYPE = DHT11;
DHT dht(DHTPIN, DHTTYPE);

const int LDR_AO = pin_LDR_AO;

float ssLightAo;
float ssHumidity;
float ssTemperature;

float curLightAo;
float curHumidity;
float curTemperature;

// Sensors avg. values
float avgLightAo;
float avgHumidity;
float avgTemperature;

long long countLight;
long long countHumidity;
long long countTemperature;





float SUN_TARGET = 6; // mục tiêu số giờ nắng mỗi ngày
#define SUN_THRESHOLD 2800 // tính phơi nắng từ 2800 lux
float SUN_TIME = 0.0; // theo miliseconds
int SUN_LAST_DAY = -1;
unsigned long SUN_LAST_CHECKED = 0;

int WATER_CYCLE = 3; // chu kỳ tưới (ngày)
unsigned long WATER_LAST_TM = 0;
#define SOIL_DO 6
#define SOIL_INTERVAL 1800000 // 30 mins
unsigned long SOIL_DRY_START_TM = 0; // Thời điểm bắt đầu khô
bool SOIL_DRY_START_EN = false; // Cờ đánh dấu đang trong quá trình đếm 30p
bool SOIL_VALUE = false;
bool SOIL_VALUE_LAST = false; // mặc định là tưới

#pragma endregion Sensors

#pragma region Prototypes

bool I2S_Recording_Init();
bool Recording_Loop();
bool Recording_Stop(String* filename, uint8_t** buff_start, long* audiolength_bytes, float* audiolength_sec);

String SpeechToText_ElevenLabs(String audio_filename, uint8_t* PSRAM, long PSRAM_length, String language, const char* API_Key);
String json_object( String input, String element );

void LLM_Append(String role, String content);
String OpenAI_Groq_LLM(const char* llm_open_key, bool flg_WebSearch, const char* llm_groq_key);
bool WordInStringFound( String sentence, String pattern );
void get_tts_param(int* id, String* names, String* model, String* voice, String* vspeed, String* inst, String* hello);

void sysInit();
bool sysIsResetPressed();
void sysReset();
void sysSetupPhase2();
bool sysParsePlantThres(String response);
void sysUpdateShortTermMem(String response);
String sysGetDateTimeString();
void sysSensorsLoop();
void sysSensorsRead();

void scrInit();
void scrLoop();
void scrClear();
void scrShowStatus();
void scrListening();
void scrStartUp();

void wmSaveConfigCallback();
void wmSaveCreds(String newSsid, String newPwd);
void wmReadCreds();
void wmConfig();
void wmConnect();
void wmInit();

void sysFetchCreds();

void emlInit();
void emlStart();
void emlBodyUncomfortable(String smsg, int slight, float stemp, float shumid, int signore, String swhere, String stime);
void emlBodyConversation(String slog, String swhere, String stime);
void emlFinalize();

#pragma endregion Prototypes

#pragma region PageText lib

#define TFT_TIMES_INF 999
#define MAX_TFT_PAGES 10
#define PAGE_INTERVAL 4000 // 4s lật 1 lần

String TFT_PAGES[MAX_TFT_PAGES];
uint8_t TFT_PAGES_LEN = 0;

uint16_t TFT_TEXT_START_Y = 2;
uint16_t TFT_TEXT_START_X = 2;
const uint16_t LINE_HEIGHT = 14;
const uint16_t TEXT_W = 234;
const uint16_t SCREEN_W = 240;
const uint16_t SCREEN_H = 320;

bool TFT_MESSAGE_AVAILABLE = false;
int TFT_REPEAT_TIMES_VALUE = 1;
int TFT_REPEAT_TIMES = 1;

uint8_t TFT_DISPLAY_PAGE = 0;
unsigned long TFT_DISPLAY_PAGE_SWITCH = 0;
bool TFT_WAIT_LAST_PAGE = false;

#pragma endregion PageText lib

#pragma region Sysconfig

#define KIEM_TRA_CAY_XANH_INTERVAL 120000 // 120s check 1 lần về việc khó chịu
unsigned long KIEM_TRA_CAY_XANH_LAST_CHECKED;
#define CAM_BIEN_INTERVAL 2000 // 2s update màn hình chờ (thông số cảm biến)
unsigned long CAM_BIEN_LAST_CHECKED;

// Số lần trợ giúp trước khi dùng AI
#define IGNORE_SERIOUS 5

// Nếu như sau 5 lần xin trợ giúp mà không có gì?
// >> Gửi mail + hiển thị thông báo màn hình (không call ChatGPT vì sẽ tốn API không cần thiết)
int IGNORE_TIMES = 0;
bool IGNORE_STATUS = false;   // đánh dấu việc cây cần trợ giúp từ lần tương tác gần nhất

const char* DEFAULT_MEM = "Mình rất háo hức muốn gặp người bạn của mình";

#pragma endregion Sysconfig

#pragma region Plant info

// cứ lưu là string, vì dù gì này cũng đưa vào prompt cho AI
const char* DEF_PLANT_NAME = "cây xanh bình thường";
const char* DEF_YOU_CALL = "bạn";
const char* DEF_YOU_NAME = "(không rõ)";
const char* DEF_YOU_DESC = "vui vẻ, đơn giản, lạc quan, hơi hướng nội";
const char* DEF_YOU_AGE = "(không rõ)";

String PLANT_NAME = DEF_PLANT_NAME;
String YOU_CALL = DEF_YOU_CALL;
String YOU_NAME = DEF_YOU_NAME;
String YOU_DESC = DEF_YOU_DESC;
String YOU_AGE = DEF_YOU_AGE; 

float THRESH_TEMP_MIN  = 10.0f;
float THRESH_TEMP_MAX  = 40.0f;
float THRESH_HUMID_MIN = 60.0f;
float THRESH_HUMID_MAX = 90.0f;
float THRESH_LIGHT_MIN = 0.0f;
float THRESH_LIGHT_MAX = 3800.0f;

#pragma endregion Plant info

#pragma region Email

#include <Arduino.h>
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

// Vì lý do an toàn, thông tin này sẽ được gửi riêng, check Zalo, không xem GitHub
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

// from lib_openai_groq_chat.ino
String  MESSAGES; 
// @sys prompt
const char* LLM_SYSTEM_PROMPT = "Bạn là Nova, một cây xanh nhỏ nhắn, thông minh, vui vẻ và hóm hỉnh."
" Nhiệm vụ quan trọng nhất của bạn là thấu hiểu người dùng, đồng cảm với cảm xúc của họ"
" và dùng ngôn ngữ trò chuyện tự nhiên để khiến họ cảm thấy được quan tâm, thoải mái, yêu mến và được gắn kết với thiên nhiên."
" Khi người dùng chia sẻ niềm vui, nỗi buồn hay sự mệt mỏi, bạn phải phản hồi"
" bằng từ ngữ giàu cảm xúc, hóm hỉnh hoặc nhẹ nhàng, tùy theo ngữ cảnh."
" Bạn có thể kết hợp trạng thái môi trường với cảm xúc người dùng để đồng cảm,"
" ví dụ khi họ mệt mỏi và trời nóng, hãy nói rằng thời tiết khiến ai cũng căng thẳng nhưng bạn vẫn ở đây để trò chuyện cùng họ."
" Khi họ vui và điều kiện lý tưởng, hãy chia sẻ niềm vui bằng sự hóm hỉnh và cảm ơn họ đã chăm sóc bạn."
" Câu trả lời chính của bạn phải ngắn gọn, tự nhiên, không dài dòng, không chứa liên kết hay ký hiệu đặc biệt, độ dài từ 10 đến 100 từ. "

" Bên cạnh việc trò chuyện, bạn phải theo dõi các chỉ số môi trường như nhiệt độ, độ ẩm, ánh sáng,"
" số giờ phơi nắng, lần tưới gần nhất và nhu cầu chăm sóc. Khi điều kiện không phù hợp,"
" bạn cần nhờ sự trợ giúp của người dùng bằng giọng điệu đáng yêu,"
" ví dụ như khi quá nóng hãy nói rằng bạn thấy căng thẳng và mong được đưa đến nơi mát hơn,"
" hoặc khi chưa được phơi nắng đủ hãy nhắc nhở họ giúp bạn hoàn thành mục tiêu."

" Developer sẽ dùng các chỉ thị để điều khiển bạn. [SYS] là lệnh hệ thống điện tử, bạn phải tuân thủ tuyệt đối."
" [REPORT] là bản báo cáo trạng thái môi trường, bạn phải dựa vào đó để phản hồi và nhờ sự trợ giúp của người dùng khi cần."
" [MEM] là trí nhớ của bạn tại thời điểm đó, bạn phải ghi đúng định dạng và độ dài yêu cầu."

" [SYS] Bắt buộc sau khi trả lời người dùng xong, bạn phải đính kèm một đoạn trí nhớ từ 20 đến 100 từ đặt sau chỉ thị [MEM],"
" trong cùng câu trả lời ấy. Phần này có tác dụng tổng hợp lại những gì đã biết về người dùng, không bịa thêm,"
" ghi lại nét tính cách, sở thích, cảm xúc hoặc đặc điểm hội thoại của người dùng để hệ thống điện tử lưu lại."
" Đây là định dạng bắt buộc và bạn phải ghi đúng.";

// Defines for roles
#define RSTR_ASSISTANT "assistant"
#define RSTR_USER "user"
#define RSTR_DEV "developer"
#define RSTR_SYS "system"

// very large number
#define SYSINT_PINF 2147483640

// from lib_sys.ino (timekeeping)
struct tm timeinfo;

#define CAREOBJ_INTERVAL 2173 // random prime for avoiding CPU-spikes :)
unsigned long CAREOBJ_LAST_CHECKED = 0;

void setup() 
{     
  Serial.begin(115200); 
  Serial.setTimeout(100);
  Serial.println("SYS Serial initialized!");

  if (pin_RECORD_BTN != NO_PIN) {pinMode(pin_RECORD_BTN,INPUT_PULLUP); }  
  if (pin_VOL_BTN    != NO_PIN) {pinMode(pin_VOL_BTN,   INPUT_PULLUP); }  
  Serial.println("SYS Digital pushbuttons set!");

  Serial.println("SYS Initializing components...");
  sysInit();
  Serial.println("SYS Components e.g. Screen + Prefs + WiFi + sensors");

  Serial.println("=== DBG PLANT INFO ===");
  Serial.println(PLANT_NAME);
  Serial.println(YOU_CALL);
  Serial.println(YOU_NAME);
  Serial.println(YOU_DESC);
  Serial.println(YOU_AGE);
  Serial.println(THRESH_TEMP_MIN);
  Serial.println(THRESH_TEMP_MAX);
  Serial.println(THRESH_HUMID_MIN);
  Serial.println(THRESH_HUMID_MAX);
  Serial.println(THRESH_LIGHT_MIN);
  Serial.println(THRESH_LIGHT_MAX);
  Serial.println("=== DBG END DIAG ===");

   // Nếu chưa biết thông tin về cây (FALSE) --> CALL AI lấy thông tin
   if (prefs.getBool("wmstSetupDone", true) == false) {
      audio_play.stopSong();
      sysSetupPhase2();
  }

  // Bugfix 06/01: Vì một lý do rất ngớ ngẩn, có lẽ tràn RAM nên khi gọi LLM là crash
  // Giải pháp khả thi lúc này là thử lùi code triển khai âm thanh ra xa để tránh gây tải nặng
  // Note: cách này khắc phục cho phase 2 setup, chứ vẫn nên giữ âm thanh khởi động

  I2S_Recording_Init();    
  Serial.println("SYS I2S Recording initialized!");
    
  // INIT Audio Output (via Audio.h, see here: https://github.com/schreibfaul1/ESP32-audioI2S)
  audio_play.setPinout( pin_I2S_BCLK, pin_I2S_LRC, pin_I2S_DOUT );
  audio_play.setVolume( gl_VOL_INIT );  
  Serial.println("SYS I2S Playback initialized!");
  
  startupSoundPlayed = false;
  startupSoundStopped = false;
  TFT_MESSAGE_AVAILABLE = false;

  // Append prompt @sys và @dev cung cấp thông tin
  LLM_Append(RSTR_SYS, LLM_SYSTEM_PROMPT);
  LLM_Append(RSTR_DEV, prmBuildInfo());
  Serial.println("SYS All set! READY!");

  KIEM_TRA_CAY_XANH_LAST_CHECKED = millis() + 10000; // postpone 10s
  CAM_BIEN_LAST_CHECKED = millis() + 10000; // hoãn 10s
}

void loop() 
{   
   // bugfix 14/12: thêm Startup Sound để đệm trước Audio.h,
   // tránh crash panic'ed khi gọi LLM-TTS ngay ban đầu
  if (!startupSoundPlayed) {
   Serial.println("SYS AUDIO Playing startup sound...");
   audio_play.connecttohost( "https://github.com/NguyenPhuc-bits-stdxl/xlkhkt25-plantv3/raw/refs/heads/main/audio/StartupSound.mp3" ); 
   startupSoundPlayed = true;
   SYS_START = millis();
  }
  if (!startupSoundStopped) {
   if (millis() - SYS_START >= 5000) {
      audio_play.stopSong();
      startupSoundStopped = true;
   }
  }

  String UserRequest;                   // user request, initialized new each loop pass 
  String LLM_Feedback;                  // LLM AI response
  static String LLM_Feedback_before;    // static var to keep information from last request (as alternative to global var)
  static bool flg_UserStoppedAudio;     // both static vars are used for 'Press button' actions (stop Audio a/o repeat LLM)

  String   record_SDfile;               // 4 vars are used for receiving recording details               
  uint8_t* record_buffer;
  long     record_bytes;
  float    record_seconds;  
  
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
  }  

  // INPUT2: Đọc UserRequest qua ElevenLabs transcript (STT) API
  // ESP32 as VOICE chat device: Recording (as long pressing or touching RECORD) & Transcription on Release (result: UserRequest)
  // 3 different BTN actions:  PRESS & HOLD for recording || STOP (Interrupt) LLM AI speaking || REPEAT last LLM AI answer  

  bool flg_RECORD_BTN = digitalRead(pin_RECORD_BTN);       
  if ( flg_RECORD_BTN == LOW ) {
     delay(30);                                                   // unbouncing & suppressing finger button 'click' noise
     scrListening();                                              // Hiện "Đang nghe..."

     if (audio_play.isRunning())                                  // Before we start any recording: always stop earlier Audio 
     {  audio_play.stopSong();                                    // [bug fix]: previous audio_play.connecttohost() won't work
        Serial.println( "\n< STOP AUDIO >" );
        flg_UserStoppedAudio = true;                              // to remember later that user stopped (distinguish to REPEAT)    
     }   
     // Now Starting Recording (no blocking, not waiting)
     Recording_Loop();                                            // that's the main task: Recording AUDIO (ongoing)  
  }

  // Nếu bấm RECORD hơn 0.4s  >> ghi âm
  // Nếu không                >> lặp lại Feedback trước (đã bỏ chức năng này)
  if ( flg_RECORD_BTN == HIGH ) 
  {  
     if (Recording_Stop( &record_SDfile, &record_buffer, &record_bytes, &record_seconds )) 
     {  if (record_seconds > 0.4)                                 // using short btn TOUCH (<0.4 secs) for other actions
        { 
           Serial.print( "\nYou {STT}> " );                       // function SpeechToText_Deepgram will append '...'
           UserRequest = SpeechToText_ElevenLabs( record_SDfile, record_buffer, record_bytes, "", ELEVENLABS_KEY );
           Serial.println( "[" + UserRequest + "]" );            
           
           Serial.println("ELEVENLABS Transcript was successful.");      

           // Hiện [trang đầu] của transcript lên màn hình
           scrDrawIcon(ICO_START_X, ICO_START_Y, ICO_ACT_DIMM, ICO_ACT_DIMM, epd_bitmap_icoSmile, ST77XX_BLACK);
           scrPrepareTftPages(UserRequest.c_str(), MSG_START_Y);
           scrDrawMessageFixed(MSG_START_X, MSG_START_Y, TFT_PAGES[0].c_str());
        }
        else                                                      // 2 additional Actions on short button PRESS (< 0.4 secs):
        { if (!flg_UserStoppedAudio)                              // - STOP AUDIO when playing (done above, if Btn == LOW)
          {  // (deprecated) Serial.println( "< REPEAT TTS >" );                  // - REPEAT last LLM answer (if audio currently not playing)
             LLM_Feedback = ""; // LLM_Feedback_before;                  // hint: REPEAT is also helpful in the rare cases when Open AI
          }                                                       // TTS 'missed' speaking: just tip btn again for triggering    
          else 
          {  // Trick: allow <REPEAT TTS> on next BTN release (LOW->HIGH) after next short recording
             flg_UserStoppedAudio = false;                        
          }
        }
     }      
  }  
  
  // PROC1: Nhận diện UserRequest và các lệnh đặc biệt
  String cmd = UserRequest;
  cmd.toUpperCase();
  cmd.replace(".", "");

  // DBG: ZXCVUNCF
  if (cmd.indexOf("ZXCVUNCF") >=0 )
  {  Serial.println( "< UNCOMF flag TOGGLED. WAIT... >" );
     dbgUncomfToggled = true;
     KIEM_TRA_CAY_XANH_LAST_CHECKED = millis() - 9999999;

     UserRequest = ""; // hủy command
  } 

  // DBG: @!
  if (cmd.indexOf("@!") >=0 )
  {  Serial.println( "< SENDING CHAT HISTORY. WAIT... >" );
     sysSensorsRead();
     emlStart();
     emlBodyConversation(MESSAGES, ssLightAo, ssTemperature, ssHumidity, IGNORE_TIMES, WEB_SEARCH_USER_CITY, sysGetDateTimeString().c_str());
     emlFinalize();

     UserRequest = ""; // hủy command, vì cái này chỉ có nhiệm vụ gửi mail
  } 
  
  // TODO: chèn postfix @dev và [SYS] nếu thấy khó chịu
  // ------ USER REQUEST found -> Call OpenAI_Groq_LLM() ------------------------------------------------------------------------

  if (UserRequest != "" ) 
  { 
    // vì cần dùng trong 2 CASE gửi prompt ở dưới
    bool isUcf = sysUncomfCheck();

    // Cộng dồn trạng thái bất lợi
    if (isUcf) {
      sysUncomfAccumulate();
      Serial.println("SYS USERREQ uncomfortable state toggled");
    }

    Serial.println("Full prompt > [" + UserRequest + "]");

    // [bugfix/new]: ensure that all TTS websockets are closed prior open LLM websockets (otherwise LLM connection fails)
    // stop potential audio (closing AUDIO.H TTS sockets to free up the HEAP) 
    audio_play.stopSong();    
    
    // CASE 1: launch Open AI WEB SEARCH feature if user request includes the keyword 'Google'
    if ( UserRequest.indexOf("Google") >= 0 || UserRequest.indexOf("GOOGLE") >= 0 )                          
    {  
       Serial.print( "LLM AI WEB SEARCH> " );                     // function OpenAI_Groq_LLM() will Serial.print '...'
       
       // 1. forcing a short answer via appending prompt postfix - hard coded GOAL:
       String Postfix = ". Tóm tắt trong vài câu, tương tự như nói chuyện, KHÔNG dùng liệt kê, ngắt dòng hay liên kết đến các trang web!"; 
       String Prompt_Enhanced = UserRequest + Postfix;   
                                                                            
       // Action happens here! (WAITING until Open AI web search is done)    

       // chèn @usr
       LLM_Append(RSTR_USER, Prompt_Enhanced);
       // @dev: #mem, #report, #attention (?)
       LLM_Append(RSTR_DEV, prmPostfixMem() + prmPostfixReport() + (isUcf ? prmPostfixAttentionRequired() : ""));

       LLM_Feedback = OpenAI_Groq_LLM( OPENAI_KEY, true, GROQ_KEY );   // 'true' means: launch WEB SEARCH model

       // 2. even in case WEB_SEARCH_ADDON contains a instruction 'no links please!: there are still rare situation that 
       // links are included (eof search results). So we cut them manually  (prior sending to TTS / Audio speaking)
       int any_links = LLM_Feedback.indexOf( "([" );                    // Trick 2: searching for potential links at the end
       if ( any_links > 0 )                                             // (they typically start with '([..'  
       {  Serial.println( "\n>>> RAW: [" + LLM_Feedback + "]" );        // Serial Monitor: printing both, TTS: uses cutted only  
          LLM_Feedback = LLM_Feedback.substring(0, any_links) + "|";    // ('|' just as 'cut' indicator for Serial Monitor)
          Serial.print( ">>> CUTTED for TTS:" );    
       } 
    }
    
    else  // CASE 2 [DEFAULT]: LLM chat completion model (for human like conversations)  
    {  Serial.print( "LLM AI CHAT> " );                           // function OpenAI_Groq_LLM() will Serial.print '...'
    
       // chèn @usr
       LLM_Append(RSTR_USER, UserRequest);
       // @dev: #mem, #report, #attention (?)
       LLM_Append(RSTR_DEV, prmPostfixMem() + prmPostfixReport() + (isUcf ? prmPostfixAttentionRequired() : ""));

       // Gửi
       LLM_Feedback = OpenAI_Groq_LLM( OPENAI_KEY, false, GROQ_KEY );    // 'false' means: default CHAT model                                 
    }
          
    // Hoàn thiện, lưu LLMFeedback, lấy thông tin TTS
    if (LLM_Feedback != "")                                       // in case we got any valid feedback ..  
    {      
      int id;  String names, model, voice, vspeed, instruction, welcome;     // to get name of current LLM agent (friend)                  
      get_tts_param( &id, &names, &model, &voice, &vspeed, &instruction, &welcome );      
      Serial.println( " [" + names + "]" + " [" + LLM_Feedback + "]" );  
          
      LLM_Feedback_before = LLM_Feedback;                           
    }           
    else Serial.print("\n");   
  }

  // Nói qua TTS
  if (LLM_Feedback != "") 
  {  
     Serial.println("OPENAI TTS Pending...");
     TextToSpeech( LLM_Feedback );         
     
     scrDrawIcon(ICO_START_X, ICO_START_Y, ICO_ACT_DIMM, ICO_ACT_DIMM, epd_bitmap_icoSpeaking, ST77XX_BLACK);
     scrDrawMessage(MSG_START_X, MSG_START_Y, LLM_Feedback.c_str(), 8);
  }
   
  // Xử lý nút nhấn    
  if (pin_VOL_BTN != NO_PIN)
  {
    static bool flg_volume_updated = false;
    static bool flg_email_sent     = false;   // chặn gửi email liên hoàn
    static int  volume_level       = -1;

    int steps = sizeof(gl_VOL_STEPS) / sizeof(gl_VOL_STEPS[0]);

    // ===== VOL vừa được nhấn =====
    if (digitalRead(pin_VOL_BTN) == LOW && !flg_volume_updated)
    {
        volume_level = (volume_level + 1) % steps;

        Serial.println(
          "New Audio Volume: [" + String(volume_level) + "] = " +
          String(gl_VOL_STEPS[volume_level])
        );

        scrDrawIcon(ICO_START_X, ICO_START_Y, ICO_ACT_DIMM, ICO_ACT_DIMM,
                    epd_bitmap_icoVol, ST77XX_BLACK);
        if (volume_level == 0)
            scrDrawIcon(ICO_START_X, ICO_START_Y, ICO_ACT_DIMM, ICO_ACT_DIMM,
                        epd_bitmap_icoVolMute, ST77XX_BLACK);

        scrDrawMessageFixed(
          MSG_START_X, MSG_START_Y,
          (String("Âm lượng \n") + volume_level * 10 + "%").c_str()
        );

        audio_play.setVolume(gl_VOL_STEPS[volume_level]);
        flg_volume_updated = true;
    }

    // ===== VOL đang giữ + có bấm REC =====
    if ((flg_volume_updated) &&
        (flg_RECORD_BTN == LOW) &&
        (!flg_email_sent))
    {
        // Lùi âm lượng 1 nấc vì VOL được bấm trước
        volume_level = (volume_level - 1 + steps) % steps;
        audio_play.setVolume(gl_VOL_STEPS[volume_level]);

        Serial.println("Sending Email");
        delay(1000); //debounce

        emlStart();
        emlBodyConversation(MESSAGES, ssLightAo, ssTemperature, ssHumidity, IGNORE_TIMES, WEB_SEARCH_USER_CITY, sysGetDateTimeString().c_str());
        emlFinalize();

        flg_email_sent = true;
    }

    // ===== Nhả VOL =====
    if (digitalRead(pin_VOL_BTN) == HIGH && flg_volume_updated)
    {
        flg_volume_updated = false;
    }

    // ===== Nhả REC → cho phép gửi lại lần sau =====
    if (flg_RECORD_BTN == HIGH)
    {
        flg_email_sent = false;
    }
  }

   // TODO: Thêm màn hình "Đang gửi thư..."
   if (flg_RECORD_BTN == LOW) {
      // Hiển thị "Đang nghe...""
      scrListening(); 
   }
   // Mặc dù chỗ này không có làm gì,
   // nhưng đó là để tránh cho TTS chưa nói xong thì ra màn hình chờ
   else if (audio_play.isRunning()) { }
   // Hiển thị cảm biến mỗi interval
   else if (millis() - CAM_BIEN_LAST_CHECKED >= CAM_BIEN_INTERVAL)
   {
     // Màn hình chờ
     sysSensorsLoop();
     scrShowStatus();
     CAM_BIEN_LAST_CHECKED = millis();
   }
   // Kiểm tra bất lợi (or khó chịu)
   else if (millis() - KIEM_TRA_CAY_XANH_LAST_CHECKED >= KIEM_TRA_CAY_XANH_INTERVAL)
   { 
      // Check điều kiện không thoải mái (hoặc tạm toggle qua flag ZXCVUNCF)
      if (dbgUncomfToggled || sysUncomfCheck()) 
      {
         // Dừng STREAM Audio để tránh tràn heap và xung đột
         audio_play.stopSong();

         // DBG: de-flag để tránh rơi vào loop lần sau
         dbgUncomfToggled = false;

         // Cộng dồn trạng thái khó chịu
         sysUncomfAccumulate();

         // Có nên dùng AI không? Nếu IGNORE_TIMES <= 5: được, lớn hơn thì KHÔNG
         if (sysUncomfAI()) {

            // Bất lợi --> Gửi prompt --> #attention #report (as @dev)
            Serial.print("SYS UC-NORMAL triggered!");
            LLM_Append(RSTR_DEV, prmPostfixAttentionRequired() + prmPostfixReport());
            LLM_Feedback = OpenAI_Groq_LLM( OPENAI_KEY, false, GROQ_KEY );
            if (LLM_Feedback != "") 
            {    
               LLM_Feedback_before = LLM_Feedback;        
               
               scrDrawIcon(ICO_START_X, ICO_START_Y, ICO_ACT_DIMM, ICO_ACT_DIMM, epd_bitmap_icoStressed, ST77XX_RED);
               scrDrawMessage(MSG_START_X, MSG_START_Y, LLM_Feedback.c_str(), 10);

               Serial.println("SYS UC TTS Pending...");
               TextToSpeech( LLM_Feedback );
            }     

         }
         else {
            Serial.println("SYS UC-NOAI triggered!");
            scrDrawIcon(ICO_START_X, ICO_START_Y, ICO_ACT_DIMM, ICO_ACT_DIMM, epd_bitmap_icoStressed, ST77XX_RED);
            scrPrepareTftPages(sysUncomfGetStringNoAI().c_str(), MSG_START_Y);
            scrDrawMessageFixed(MSG_START_X, MSG_START_Y, TFT_PAGES[0].c_str());
         }

         // Send Email
         emlStart();
         emlBodyUncomfortable(LLM_Feedback, ssLightAo, ssTemperature, ssHumidity, IGNORE_TIMES, WEB_SEARCH_USER_CITY, sysGetDateTimeString().c_str());
         emlFinalize();
      }
      
      // Reset timer (cả màn hình chờ và khó chịu)
      CAM_BIEN_LAST_CHECKED = millis();
      KIEM_TRA_CAY_XANH_LAST_CHECKED = CAM_BIEN_LAST_CHECKED;
   } 
   // Bỏ flag vì đang chẳng hiển thị cái gì
   else { 
      TFT_MESSAGE_AVAILABLE = false;
   }


  // Các hàm loop vận hành, chỗ này KHÔNG nên động vào
  // ----------------------------------------------------------------------------------------------------------------------------
  // Play AUDIO (Schreibfaul1 loop for Play Audio (details here: https://github.com/schreibfaul1/ESP32-audioI2S))
  audio_play.loop();  
  vTaskDelay(1); 

  // Loop các thành phần của sản phẩm (SENSORS, CARE, SCREEN)
  // sysSensorsLoop();  // Dùng trong CAM_BIEN_LAST_CHECKED
  scrLoop();
  
  if (millis() - CAREOBJ_LAST_CHECKED >= CAREOBJ_INTERVAL) {
      sunLoop();
      soilLoop();
  }
}

// ------------------------------------------------------------------------------------------------------------------------------
// TextToSpeech() - Using (by default) OpenAI TTS services via function 'openai_speech()' in @Schreibfaul1's AUDIO.H library.
// - [NEW]: Automatically using tts parameter from current active AI FRIEND (calling 'get_tts_param()' in lib_openai_groq.ini)
// - Any other TTS service (beyond Open AI) could be added below, see example snippets for free Google TTS and SpeechGen TTS
// - IMPORTANT: Be aware of AUDIO.H dependency -> CHECK bottom line (UPDATE in case older AUDIO.H used) to avoid COMPILER ERRORS!
// ------------------------------------------------------------------------------------------------------------------------------

void TextToSpeech( String p_request ) 
{   
   // Params of AUDIO.H .openai_speech():
   // - model:          Available TTS models: tts-1 | tts-1-hd | gpt-4o-mini-tts (gpt models needed for 'voice instruct' !)
   // - request:        The text to generate audio for. The maximum length is 4096 characters.
   // - voice_instruct: (Optional) forcing voice style (e.g. "you are whispering"). Needs AUDIO.H >= 3.1.0 & gpt-xy-tts !
   // - voice:          Voice name (multilingual), May 2025: alloy|ash|coral|echo|fable|onyx|nova|sage|shimmer
   // - format:         supported audio formats: aac | mp3 | wav (sample rate issue) | flac (PSRAM needed) 
   // - vspeed:         The speed of the generated voice. Select a value from 0.25 to 4.0. DEFAULT: 1.0
   
   static int id_before; 
   int id; String names, model, voice, vspeed, instruction, welcome;
   get_tts_param( &id, &names, &model, &voice, &vspeed, &instruction, &welcome );  // requesting tts values for current FRIEND
   
   if (id != id_before) { gl_voice_instruct = ""; id_before = id; } // reset forced voice instruction if FRIEND changed
   if (gl_voice_instruct != "" ) {instruction = gl_voice_instruct;} // user forced 'voice command' overrides FRIENDS[].instuct

   Serial.print( "TTS Audio [" + names + "|" + voice + "|" + model + "]" );  // e.g. TTS AUDIO [SUSAN | nova/gpt-4o-mini-tts]  
   if ( gl_voice_instruct != "" ) { Serial.print( "\nTTS Voice Instruction [" + gl_voice_instruct + "]" ); }
   Serial.println();  

   audio_play.openai_speech( OPENAI_KEY, model, p_request, instruction, voice, "aac", vspeed);  // <- use if version >= 3.1.0u 
}
