#pragma region Libraries Macros

#include "driver/i2s_std.h"  
#include <WiFi.h>
#include <WiFiManager.h>       // Dependencies: main, lib_audio_recording, lib_audio_transcription, lib_openai_groq_chat, lib_wifi
#include <WiFiClientSecure.h>  // Dependencies: *
#include <Preferences.h>       // Dependencies: lib_sys, lib_wifi
#include <DHT.h>               // Dependencies: lib_sys
#include <time.h>

// Screen libraries
#include <Adafruit_ST7735.h>
#include <U8g2_for_Adafruit_GFX.h>

// Audio libraries (for playback)
#include <Audio.h>

// Debus macros
bool    DEBUG = true;       
#define DebugPrint(x);           if(DEBUG){Serial.print(x);}   
#define DebugPrintln(x);         if(DEBUG){Serial.println(x);}

#pragma endregion Libraries Macros

#pragma region Pinout

#define pin_I2S_DOUT 16  
#define pin_I2S_LRC 8   
#define pin_I2S_BCLK 3
#define pin_VOL_BTN 41      
#define pin_RECORD_BTN 38   

#define pin_DHT 40
#define pin_LDR_AO 4

#define LCD_CS 10
#define LCD_DC 11
#define LCD_SCLK 14
#define LCD_MOSI 13
#define LCD_RST 12

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

// --- SENSORS CONFIGURATION ---
const int DHTPIN = pin_DHT;
const int DHTTYPE = DHT11;
DHT dht(DHTPIN, DHTTYPE);

const int LDR_AO = pin_LDR_AO;

int ssLightAo;
float ssHumidity;
float ssTemperature;
int ssBattery;

Adafruit_ST7735 display = Adafruit_ST7735(LCD_CS, LCD_DC, LCD_RST);
U8G2_FOR_ADAFRUIT_GFX tft;

#pragma endregion Global objects

#pragma region Prototypes

bool I2S_Recording_Init();
bool Recording_Loop();
bool Recording_Stop(String* filename, uint8_t** buff_start, long* audiolength_bytes, float* audiolength_sec);

String SpeechToText_ElevenLabs(String audio_filename, uint8_t* PSRAM, long PSRAM_length, String language, const char* API_Key);

String OpenAI_Groq_LLM(String UserRequest, const char* llm_open_key, bool flg_WebSearch, const char* llm_groq_key);
void get_tts_param(int* id, String* names, String* model, String* voice, String* vspeed, String* inst, String* hello);

void sysInit();
void sysReset();
bool sysIsResetPressed();
void sysReadSensors();
String sysGetSensorsString();
bool sysIsUncomfortable();
String sysGetDateTimeString();
String sysGetUncomfortableString();

void scrInit();
void scrDrawIcon(const uint8_t x, const uint8_t y, const uint8_t w, const uint8_t h, const uint8_t* icon, const uint16_t color);
void scrDrawMessage(const uint8_t x, const uint8_t y, const char* msg, bool doNotClearAfterDisplayEnds, bool repeatMessageOnLoop);
void scrClear();
void scrShowStatus();
void scrStartUp();

void wmSaveConfigCallback();
void wmSaveCreds(String newSsid, String newPwd);
void wmReadCreds();
void wmConfig();
void wmConnect();
void wmInit();

void sysFetchCreds();

#pragma endregion Prototypes

#pragma region Strings

const char* systrReset = "Đặt lại\ncấu hình.";

const char* wmBroadcast = "NOVA";
const char* wmsPleaseConfig = "Kết nối\n[NOVA] và\ntruy cập\n192.168.4.1.";
const char* wmsSaveRequest = "Đang nhận...";
const char* wmsSaveSuccess = "Thiết lập\nthành công\nChờ...";
const char* wmsReading = "Đang đọc\ncấu hình...";
const char* wmsEstablished = "Kết nối\nthành công!";

#pragma endregion Strings

#define ICO_BRAND_DIMM 32
#define ICO_ACT_DIMM 24
#define MSG_START_X 2
#define MSG_START_Y 36
#define ICO_START_X 52
#define ICO_START_Y 2

#define KIEM_TRA_CAY_XANH_INTERVAL 60000 // 60s check 1 lần về việc khó chịu
long long KIEM_TRA_CAY_XANH_LAST_CHECKED;

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
  Serial.println("SYS Components e.g. Screen + Prefs + WiFi + sensors")

  I2S_Recording_Init();    
  Serial.println("SYS I2S Recording initialized!");
    
  // INIT Audio Output (via Audio.h, see here: https://github.com/schreibfaul1/ESP32-audioI2S)
  audio_play.setPinout( pin_I2S_BCLK, pin_I2S_LRC, pin_I2S_DOUT );
  audio_play.setVolume( gl_VOL_INIT );  
  Serial.println("SYS I2S Playback initialized!");
  
  KIEM_TRA_CAY_XANH_LAST_CHECKED = millis() + 300000; // postpone 5p để cho người dùng config hoặc là yên tĩnh lúc đầu

  Serial.println("SYS All set! READY!");
}

void loop() 
{
  String UserRequest;                   // user request, initialized new each loop pass 
  String LLM_Feedback;                  // LLM AI response
  static String LLM_Feedback_before;    // static var to keep information from last request (as alternative to global var)
  static bool flg_UserStoppedAudio;     // both static vars are used for 'Press button' actions (stop Audio a/o repeat LLM)

  String   record_SDfile;               // 4 vars are used for receiving recording details               
  uint8_t* record_buffer;
  long     record_bytes;
  float    record_seconds;  
  
  // --- ĐỌC REQUEST QUA SERIAL MONITOR ---
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

  // ------ Read USER INPUT via Voice recording & launch transcription ----------------------------------------------------------
  // ESP32 as VOICE chat device: Recording (as long pressing or touching RECORD) & Transcription on Release (result: UserRequest)
  // 3 different BTN actions:  PRESS & HOLD for recording || STOP (Interrupt) LLM AI speaking || REPEAT last LLM AI answer  
  bool flg_RECORD_BTN = digitalRead(pin_RECORD_BTN);       
  if ( flg_RECORD_BTN == LOW ) {
     delay(30);                                                   // unbouncing & suppressing finger button 'click' noise 
     if (audio_play.isRunning())                                  // Before we start any recording: always stop earlier Audio 
     {  audio_play.stopSong();                                    // [bug fix]: previous audio_play.connecttohost() won't work
        Serial.println( "\n< STOP AUDIO >" );
        flg_UserStoppedAudio = true;                              // to remember later that user stopped (distinguish to REPEAT)    
     }   
     // Now Starting Recording (no blocking, not waiting)
     Recording_Loop();                                            // that's the main task: Recording AUDIO (ongoing)  
  }

  // Nếu bấm RECORD hơn 0.4s  >> ghi âm
  // Nếu không                >> lặp lại Feedback trước
  if ( flg_RECORD_BTN == HIGH ) 
  {  
     if (Recording_Stop( &record_SDfile, &record_buffer, &record_bytes, &record_seconds )) 
     {  if (record_seconds > 0.4)                                 // using short btn TOUCH (<0.4 secs) for other actions
        { 
           Serial.print( "\nYou {STT}> " );                       // function SpeechToText_Deepgram will append '...'
           UserRequest = SpeechToText_ElevenLabs( record_SDfile, record_buffer, record_bytes, "", ELEVENLABS_KEY );
           Serial.println( "[" + UserRequest + "]" );            
           
           Serial.println("ELEVENLABS Transcript was successful.")           
        }
        else                                                      // 2 additional Actions on short button PRESS (< 0.4 secs):
        { if (!flg_UserStoppedAudio)                              // - STOP AUDIO when playing (done above, if Btn == LOW)
          {  Serial.println( "< REPEAT TTS >" );                  // - REPEAT last LLM answer (if audio currently not playing)
             LLM_Feedback = LLM_Feedback_before;                  // hint: REPEAT is also helpful in the rare cases when Open AI
          }                                                       // TTS 'missed' speaking: just tip btn again for triggering    
          else 
          {  // Trick: allow <REPEAT TTS> on next BTN release (LOW->HIGH) after next short recording
             flg_UserStoppedAudio = false;                        
          }
        }
     }      
  }  
  
  // ------ USER REQUEST found -> Checking KEYWORDS first -----------------------------------------------------------------------
  String cmd = UserRequest;
  cmd.toUpperCase();
  cmd.replace(".", "");

  // RADIO
  if (cmd.indexOf("RADIO") >=0 )
  {  Serial.println( "< Streaming German RADIO: SWR3 >" );  
     audio_play.connecttohost( "https://liveradio.swr.de/sw282p3/swr3/play.mp3" ); 
     UserRequest = "";
  } 

  // ------ USER REQUEST found -> Call OpenAI_Groq_LLM() ------------------------------------------------------------------------
  if (UserRequest != "" ) 
  { 
    // Chèn string sensor vào để báo cho cây biết thông số của nó
    UserRequest = sysGetSensorsString() + UserRequest;

    // [bugfix/new]: ensure that all TTS websockets are closed prior open LLM websockets (otherwise LLM connection fails)
    audio_play.stopSong();    // stop potential audio (closing AUDIO.H TTS sockets to free up the HEAP) 
    
    // CASE 1: launch Open AI WEB SEARCH feature if user request includes the keyword 'Google'
    if ( UserRequest.indexOf("Google") >= 0 || UserRequest.indexOf("GOOGLE") >= 0 )                          
    {  
       Serial.print( "LLM AI WEB SEARCH> " );                     // function OpenAI_Groq_LLM() will Serial.print '...'
       
       // 1. forcing a short answer via appending prompt postfix - hard coded GOAL:
       String Postfix = ". Tóm tắt trong vài câu, tương tự như nói chuyện, KHÔNG dùng liệt kê, ngắt dòng hay liên kết đến các trang web!"; 
       String Prompt_Enhanced = UserRequest + Postfix;   
                                                                            
       // Action happens here! (WAITING until Open AI web search is done)        
       LLM_Feedback = OpenAI_Groq_LLM( Prompt_Enhanced, OPENAI_KEY, true, GROQ_KEY );   // 'true' means: launch WEB SEARCH model

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
       LLM_Feedback = OpenAI_Groq_LLM( UserRequest, OPENAI_KEY, false, GROQ_KEY );    // 'false' means: default CHAT model                                 
    }
          
    // ------ FINAL TASKS -------------------------------------------------------------------------------------------------------
    if (LLM_Feedback != "")                                       // in case we got any valid feedback ..  
    {      
      int id;  String names, model, voice, vspeed, instruction, welcome;     // to get name of current LLM agent (friend)                  
      get_tts_param( &id, &names, &model, &voice, &vspeed, &instruction, &welcome );      
      Serial.println( " [" + names + "]" + " [" + LLM_Feedback + "]" );  
          
      LLM_Feedback_before = LLM_Feedback;                           
    }           
    else Serial.print("\n");   
  }

  // ------ Speak LLM answer (using Open AI voices by default, all voice settings done in TextToSpeech() ------------------------
  if (LLM_Feedback != "") 
  {  
     Serial.println("OPENAI TTS Pending...");
     TextToSpeech( LLM_Feedback );         
  }
   
  // ------ Điều chỉnh âm lượng -------------------------------------------------------------------------------------------------    
  if (pin_VOL_BTN != NO_PIN)      // --- Alternative: using a VOL_BTN to toggle thru N values (e.g. TECHISMS or Elato AI pcb)
  {  static bool flg_volume_updated = false; 
     static int volume_level = -1; 
     if (digitalRead(pin_VOL_BTN) == LOW && !flg_volume_updated)          
     {  int steps = sizeof(gl_VOL_STEPS) / sizeof(gl_VOL_STEPS[0]);
        volume_level = ((volume_level+1) % steps);  
        Serial.println( "New Audio Volume: [" + (String) volume_level + "] = " + (String) gl_VOL_STEPS[volume_level] );
        flg_volume_updated = true;
        audio_play.setVolume( gl_VOL_STEPS[volume_level] );  
    }
    if (digitalRead(pin_VOL_BTN) == HIGH && flg_volume_updated)         
    {  flg_volume_updated = false;    
    }
  }
  
  // ----------------------------------------------------------------------------------------------------------------------------
  // Play AUDIO (Schreibfaul1 loop for Play Audio (details here: https://github.com/schreibfaul1/ESP32-audioI2S))
  audio_play.loop();  
  vTaskDelay(1); 

  // update LED status always (in addition to WHITE flashes + YELLOW on STT + BLUE on LLM + CYAN on TTS)
  if (flg_RECORD_BTN==LOW)
    { /*Đang nghe*/ }  
  else if (audio_play.isRunning())
    { /*Đang nói*/ }  
  else if (millis() - KIEM_TRA_CAY_XANH_LAST_CHECKED >= KIEM_TRA_CAY_XANH_INTERVAL)
    { /* Wishlist: Check điều kiện không thoải mái (tạm toggle qua flag UNCOMFORTABLE)*/ 
      if (sysIsUncomfortable()) {
         // Dừng STREAM Audio để tránh tràn heap và xung đột
         audio_play.stopSong();
         
         LLM_Feedback = OpenAI_Groq_LLM( sysGetUncomfortableString() + sysGetSensorsString(), OPENAI_KEY, false, GROQ_KEY );
         if (LLM_Feedback != "")                              
         {    
            int id;  String names, model, voice, vspeed, instruction, welcome;                  
            get_tts_param( &id, &names, &model, &voice, &vspeed, &instruction, &welcome );      
            Serial.println( " [" + names + "]" + " [" + LLM_Feedback + "]" );  
          
            LLM_Feedback_before = LLM_Feedback;        

            Serial.println("OPENAI TTS Pending...");
            TextToSpeech( LLM_Feedback );
         }         
      }
      
      // Reset timer
      KIEM_TRA_CAY_XANH_LAST_CHECKED = millis();
    }  
  else 
    { /*Màn hình chờ*/ }
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
