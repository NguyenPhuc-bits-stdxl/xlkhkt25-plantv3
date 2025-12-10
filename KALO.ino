// -- DEFINE YOUR OWN CREDENTIALS IN private_credentials.ino --

#pragma region DECLARATION

#define VERSION "\n== KALO_ESP32_Voice_Chat_AI_Friends (last update: Sept. 22, 2025) ======================\n"
#define VERSION_DATE "20250922"

// --- includes ----------------

#include <WiFi.h>
#include <WiFiManager.h>       // Dependencies: main, lib_audio_recording, lib_audio_transcription, lib_openai_groq_chat, lib_wifi
#include <WiFiClientSecure.h>  // only needed in other tabs (.ino)
#include <Preferences.h>       // Dependencies: lib_sys
#include <DHT.h>               // Dependencies: lib_sys

// Screen libraries
// #include <Adafruit_GFX.h>
// #include <Adafruit_ST7735.h>
// #include <SPI.h>

#include <Audio.h>  // @Schreibfaul1 library, used for PLAYING Audio (via I2S Amplifier) -> mandatory for TTS only
                    // [not needed for: Audio Recording - Audio STT Transcription - AI LLM Chat]

// --- defines & macros -------- // DEBUG Toggle: 'true' enables, 'false' disables printing additional details in Serial Monitor
bool DEBUG = true;
#define DebugPrint(x) \
  ; \
  if (DEBUG) { Serial.print(x); }
#define DebugPrintln(x) \
  ; \
  if (DEBUG) { Serial.println(x); }

// === PRIVATE credentials =====
// -- DEFINE YOUR OWN CREDENTIALS IN private_credentials.ino -- //
// THIS SECTION BELOW IS DEPRECATED //

const char* OPENAI_KEY = "";
const char* GROQ_KEY = "";
const char* ELEVENLABS_KEY = "";
const char* DEEPGRAM_KEY = "";
// also includes wmSsid, wmPwd

// === user settings ==========
int gl_VOL_INIT = 21;
int gl_VOL_STEPS[] = { 0, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21 };

// --- PCB: ESP32-S3 Dev
#define pin_I2S_DOUT 16  // I2S_DOUT + I2S_LRC + I2S_BCLK are the I2S Audio OUT pins (e.g. for I2S Amplifier MAX98357)
#define pin_I2S_LRC 8    // (Hint 1: I2S INPUT pins via Microphone INMP441: see header in 'lib_audio_recording.ino')
#define pin_I2S_BCLK 3
#define pin_TOUCH NO_PIN     // # KALO mod # - self soldered RECORD TOUCH (free: ESP32 12,14,32 / ESP32-S3: 3,12,14)
#define NO_PIN -1            // 'not connected' value for optional controls: RECORD_BTN | TOUCH | VOL_POTI | VOL_BTN
#define pin_VOL_POTI NO_PIN  // no analogue wheel POTI available, using VOL_BTN for audio volume control
#define pin_VOL_BTN 41       // # KALO mod # - using original RECORD side button used as audio volume control
#define pin_RECORD_BTN 38    // # KALO mod # - no 2nd button available (original RECORD side btn on pin 2 used as VOL_BTN)

// --- global Objects ----------

Audio audio_play;  // AUDIO.H object for I2S stream

uint32_t gl_TOUCH_RELEASED;  // idle value (untouched), read once on Init, used internally as reference in loop()
String gl_voice_instruct;    // internally used for forced 'voice character' (via command "VOICE", erased on friend changes)

Preferences prefs;  // Dependencies: lib_wifi, lib_sys

WiFiManager wm;  // WiFi global objects - Dependencies: main, lib_wifi
bool wmShouldSaveConfig = false;
String wmSsid;
String wmPwd;

// -- SYSTEM STRINGS ---

const char* systrReset = "RESET button\nis pressed!";

const char* wmBroadcast = "CAY XANH LILY";
const char* wmsPleaseConfig = "Connect to\n'CAY XANH LILY'\nto configure";
const char* wmsSaveRequest = "Receiving data\nfrom WiFiManager...";
const char* wmsSaveSuccess = "WiFi credentials\nare saved\nsuccessfully. Wait...";
const char* wmsReading = "Reading WiFi\nconfiguration...";
const char* wmsEstablished = "Connection established. Ready!";

// --- SENSORS CONFIGURATION ---

const int DHTPIN = 40;
const int DHTTYPE = DHT11;
DHT dht(DHTPIN, DHTTYPE);

const int LDR_AO = 4;

int ssLightAo;
float ssHumidity;
float ssTemperature;

bool flg_RECORD_BTN;
bool flg_RECORD_TOUCH = false;

// --- TFT SCREEN ---
#define LCD_CS 10
#define LCD_DC 11
#define LCD_SCLK 14
#define LCD_MOSI 13
#define LCD_RST 12
//Adafruit_ST7735 tft = Adafruit_ST7735(LCD_CS, LCD_DC, LCD_RST);

// --- last not least: declaration of functions in other modules (not mandatory but ensures compiler checks correctly)
// splitting Sketch into multiple tabs see e.g. here: https://www.youtube.com/watch?v=HtYlQXt14zU

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

void scrInit();
void scrShowMessage(const char* msg);
void scrShowStatus();
void scrStartUp();

void wmSaveConfigCallback();
void wmSaveCreds(String newSsid, String newPwd);
void wmReadCreds();
void wmConfig();
void wmConnect();
void wmInit();

void sysFetchCreds();

#pragma endregion DECLARATION

// ******************************************************************************************************************************

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  Serial.println("SERIAL INITIALIZED");

  // Hello World
  Serial.println(VERSION);

  Serial.println("INITIALIZING SYSTEM COMPONENTS (WF + SCR + SENSORS)...");
  sysInit();
  Serial.println("SYSTEM COMPONENTS INITIALIZED");

  // Digital INPUT pin assignments (not needed for analogue pin_VOL_POTI & pin_TOUCH)
  // Detail: Some ESP32 pins do NOT support INPUT_PULLUP (e.g. pins 34-39), external resistor still needed
  if (pin_RECORD_BTN != NO_PIN) { pinMode(pin_RECORD_BTN, INPUT_PULLUP); }
  if (pin_VOL_BTN != NO_PIN) { pinMode(pin_VOL_BTN, INPUT_PULLUP); }

  I2S_Recording_Init();
  Serial.println("I2S RECORDING INITIALIZED");

  // INIT Audio Output (via Audio.h, see here: https://github.com/schreibfaul1/ESP32-audioI2S)
  audio_play.setPinout(pin_I2S_BCLK, pin_I2S_LRC, pin_I2S_DOUT);
  audio_play.setVolume(gl_VOL_INIT);
  Serial.println("I2S PLAYBACK INITIALIZED");

  // INIT done, starting user interaction
  Serial.println("\nWorkflow:\n> Hold or Touch button during recording voice -OR- enter request in Serial Monitor");
  Serial.println("> Key word [RADIO | DAILY NEWS | TAGESSCHAU] inside request: start audio url streaming");
  Serial.println("========================================================================================\n");

  Serial.println("INITIALIZATION COMPLETED");
}

// ******************************************************************************************************************************

void loop() {
  // Code logic chính (đẩy xuống cho đỡ rối)
  MainLoop();
  
  // ----------------------------------------------------------------------------------------------------------------------------
  // Play AUDIO (Schreibfaul1 loop for Play Audio (details here: https://github.com/schreibfaul1/ESP32-audioI2S))
  // and updating LED status
  
  audio_play.loop();
  vTaskDelay(1);
  delay(100);

  if (flg_RECORD_BTN == LOW || flg_RECORD_TOUCH) {
    //scrShowMessage("LISTENING...");
  } else if (audio_play.isRunning()) {
    //scrShowMessage("SPEAKING...");
  } else {
    sysReadSensors();
    Serial.println("---");
    Serial.println(ssLightAo);
    Serial.println(ssTemperature);
    Serial.println(ssHumidity);
    //scrShowStatus();
    //scrShowMessage("REQ READY");
  }
}

// end of LOOP() ****************************************************************************************************************

void MainLoop()
{  
  flg_RECORD_BTN = false;
  flg_RECORD_TOUCH = false;

  String UserRequest = "";            // user request, initialized new each loop pass
  String LLM_Feedback;                // LLM AI response
  static String LLM_Feedback_before;  // static var to keep information from last request (as alternative to global var)
  static bool flg_UserStoppedAudio;   // both static vars are used for 'Press button' actions (stop Audio a/o repeat LLM)

  String record_SDfile;  // 4 vars are used for receiving recording details
  uint8_t* record_buffer;
  long record_bytes;
  float record_seconds;

  // ------ Read USER INPUT via Serial Monitor (fill String UserRequest and ECHO after CR entered) ------------------------------
  // ESP32 as TEXT Chat device: this allows to use the Serial Monitor as an LLM AI text chat device
  // HINT: hidden feature (covered in lib_openai_groq.ino): Keyword '#' in Serial Monitor prints the history of complete dialog

  while (Serial.available() > 0)  // definition: returns numbers ob chars after CR done
  {                               // we end up here only after Input done
                                  // this 'while loop' is a NOT blocking loop script :)
    UserRequest = Serial.readStringUntil('\n');

    // Clean the input line first:
    UserRequest.replace("\r", "");
    UserRequest.replace("\n", "");
    UserRequest.trim();

    // then ECHO in monitor in [brackets] (in case the user entered more than spaces or CR only):
    if (UserRequest != "") {
      Serial.println("\nYou> [" + UserRequest + "]");
    }
  }

  // ------ Check status of AUDIO RECORDING controls (supporting PUSH buttons and TOUCH buttons) --------------------------------
  if (pin_RECORD_BTN != NO_PIN) {
    flg_RECORD_BTN = digitalRead(pin_RECORD_BTN);
  } else flg_RECORD_BTN = HIGH;

  // ------ Read USER INPUT via Voice recording & launch Deepgram transcription -------------------------------------------------
  // ESP32 as VOICE chat device: Recording (as long pressing or touching RECORD) & Transcription on Release (result: UserRequest)
  // 3 different BTN actions:  PRESS & HOLD for recording || STOP (Interrupt) LLM AI speaking || REPEAT last LLM AI answer

  if (flg_RECORD_BTN == LOW || flg_RECORD_TOUCH)  // # Recording started, supporting btn and touch sensor
  {
    delay(30);                   // unbouncing & suppressing finger button 'click' noise
    if (audio_play.isRunning())  // Before we start any recording: always stop earlier Audio
    {
      audio_play.stopSong();  // [bug fix]: previous audio_play.connecttohost() won't work
      Serial.println("\n< STOP AUDIO >");
      flg_UserStoppedAudio = true;  // to remember later that user stopped (distinguish to REPEAT)
    }
    // Now Starting Recording (no blocking, not waiting)
    Recording_Loop();  // that's the main task: Recording AUDIO (ongoing)
  }

  if (flg_RECORD_BTN == HIGH && !flg_RECORD_TOUCH)  // Recording not started yet OR stopped (on release button)
  {
    // now we check if RECORDING is done, we receive recording details (length etc..) via &pointer
    // hint: Recording_Stop() is true ONCE when recording finalized and .wav is available

    if (Recording_Stop(&record_SDfile, &record_buffer, &record_bytes, &record_seconds)) {
      if (record_seconds > 0.4)  // using short btn TOUCH (<0.4 secs) for other actions
      {
        Serial.println("ELEVENLABS STARTING...");
        Serial.println("\nYou {STT}> ");

        // Action happens here! -> Launching SpeechToText (STT) transcription (WAITING until done)
        // using ElevenLabs STT as default (best performance, multi-lingual, high accuracy (language & word detection)
        // Reminder: as longer the spoken sentence, as better the results in language and word detection ;)

        UserRequest = SpeechToText_ElevenLabs(record_SDfile, record_buffer, record_bytes, "", ELEVENLABS_KEY);
        if (UserRequest != "")  // Done!. In case we got a valid spoken transcription:
        {
          Serial.println("TRANSCRIPT WAS SUCCESSFUL");
        }
        Serial.println("[" + UserRequest + "]");  // printing result in Serial Monitor always
      } else                                      // 2 additional Actions on short button PRESS (< 0.4 secs):
      {
        if (!flg_UserStoppedAudio)  // - STOP AUDIO when playing (done above, if Btn == LOW)
        {
          Serial.println("< REPEAT TTS >");    // - REPEAT last LLM answer (if audio currently not playing)
          LLM_Feedback = LLM_Feedback_before;  // hint: REPEAT is also helpful in the rare cases when Open AI
        }                                      // TTS 'missed' speaking: just tip btn again for triggering
        else {                                 // Trick: allow <REPEAT TTS> on next BTN release (LOW->HIGH) after next short recording
          flg_UserStoppedAudio = false;
        }
      }
    }
  }

  // ------ USER REQUEST found -> Checking KEYWORDS first -----------------------------------------------------------------------

  String cmd = UserRequest;
  cmd.toUpperCase();
  cmd.replace(".", "");

  // 1. keyword 'RADIO' inside the user request -> Playing German RADIO Live Stream: SWR3
  // Use case example (Recording request): "Please play radio for me, thanks" -> Streaming launched

  if (cmd.indexOf("RADIO") >= 0) {
    Serial.println("< Streaming German RADIO: SWR3 >");
    // HINT !: the streaming can fail on some ESP32 without PSRAM (AUDIO.H issue!), in this case: deactivate/remove next line:
    audio_play.connecttohost("https://liveradio.swr.de/sw282p3/swr3/play.mp3");
    UserRequest = "";  // do NOT start LLM
  }

  // ------ USER REQUEST found -> Call OpenAI_Groq_LLM() ------------------------------------------------------------------------
  // #NEW: Update: Utilizing new 'real-time' web search feature in case user request contains the keyword 'GOOGLE'
  // [using same 'OpenAI_Groq_LLM(..)' function with additional parameter (function switches to dedicated LLM search model]
  // Recap: OpenAI_Groq_LLM() remembers complete history (appending prompts) to support ongoing dialogs (web searches included;)

  if (UserRequest != "") {
   // Thêm string Sensors vào cho prompt
   UserRequest = sysGetSensorsString() + UserRequest;

    // [bugfix/new]: ensure that all TTS websockets are closed prior open LLM websockets (otherwise LLM connection fails)
    //audio_play.stopSong();    // stop potential audio (closing AUDIO.H TTS sockets to free up the HEAP)

    // CASE 1: launch Open AI WEB SEARCH feature if user request includes the keyword 'Google'
    // supporting User requests like 'Will it rain tomorrow in my region?, please ask Google'

    if (UserRequest.indexOf("Google") >= 0 || UserRequest.indexOf("GOOGLE") >= 0) {
      Serial.println("WEB SEARCH ONGOING...");
      Serial.print("LLM AI WEB SEARCH> ");  // function OpenAI_Groq_LLM() will Serial.print '...'

      // 2 workarounds are recommended to utilize the new Open AI Web Search for TTS (speaking the response).
      // Background: New Open AI Web Search models are not intended for TTS (much too detailed, including lists & links etc.)
      // and they are also 'less' prompt sensitive, means ignoring earlier instructions (e.g. 'shorten please!') quite often
      // so we use 2 tricks: 1. adding a 'default' instruction each time (forcing 'short answers') + 2. removing remaining links
      // KEEP in mind: SEARCH models are slower (response delayed) than CHAT models, so we use on (GOOGLE) demand only !

      // 1. forcing a short answer via appending prompt postfix - hard coded GOAL:
      String Postfix = ". Tóm tắt trong vài câu, tương tự như nói chuyện, KHÔNG dùng liệt kê, ngắt dòng hay liên kết đến các trang web!";
      String Prompt_Enhanced = UserRequest + Postfix;

      // Action happens here! (WAITING until Open AI web search is done)
      LLM_Feedback = OpenAI_Groq_LLM(Prompt_Enhanced, OPENAI_KEY, true, GROQ_KEY);  // 'true' means: launch WEB SEARCH model

      // 2. even in case WEB_SEARCH_ADDON contains a instruction 'no links please!: there are still rare situation that
      // links are included (eof search results). So we cut them manually  (prior sending to TTS / Audio speaking)

      int any_links = LLM_Feedback.indexOf("([");  // Trick 2: searching for potential links at the end
      if (any_links > 0)                           // (they typically start with '([..'
      {
        Serial.println("\n>>> RAW: [" + LLM_Feedback + "]");        // Serial Monitor: printing both, TTS: uses cutted only
        LLM_Feedback = LLM_Feedback.substring(0, any_links) + "|";  // ('|' just as 'cut' indicator for Serial Monitor)
        Serial.print(">>> CUTTED for TTS:");
      }
    }

    else  // CASE 2 [DEFAULT]: LLM chat completion model (for human like conversations)

    {
      Serial.print("LLM AI CHAT> ");  // function OpenAI_Groq_LLM() will Serial.print '...'

      // Action happens here! (WAITING until Open AI or Groq done)
      LLM_Feedback = OpenAI_Groq_LLM(UserRequest, OPENAI_KEY, false, GROQ_KEY);  // 'false' means: default CHAT model
    }

    // final tasks (always):

    if (LLM_Feedback != "")  // in case we got any valid feedback ..
    {
      int id;
      String names, model, voice, vspeed, instruction, welcome;  // to get name of current LLM agent (friend)
      get_tts_param(&id, &names, &model, &voice, &vspeed, &instruction, &welcome);
      Serial.println(" [" + names + "]" + " [" + LLM_Feedback + "]");

      LLM_Feedback_before = LLM_Feedback;
    } else Serial.print("\n");
  }

  // ------ Speak LLM answer (using Open AI voices by default, all voice settings done in TextToSpeech() ------------------------

  if (LLM_Feedback != "") {
    Serial.println("TTS PENDING");

    // simple TTS call: TextToSpeech() manages all voice parameter for current active AI agent (FRIEND[x])
    TextToSpeech(LLM_Feedback);
  }

  if (pin_VOL_BTN != NO_PIN)  // --- Alternative: using a VOL_BTN to toggle thru N values (e.g. TECHISMS or Elato AI pcb)
  {
    static bool flg_volume_updated = false;
    static int volume_level = -1;
    if (digitalRead(pin_VOL_BTN) == LOW && !flg_volume_updated) {
      int steps = sizeof(gl_VOL_STEPS) / sizeof(gl_VOL_STEPS[0]);  // typically: 3 steps (any arrays supported)
      volume_level = ((volume_level + 1) % steps);                 // walking in circle (starting with 0): e.g. 0 -> 1 -> 2 -> 0 ..
      Serial.println("New Audio Volume: [" + (String)volume_level + "] = " + (String)gl_VOL_STEPS[volume_level]);
      flg_volume_updated = true;
      audio_play.setVolume(gl_VOL_STEPS[volume_level]);
    }
    if (digitalRead(pin_VOL_BTN) == HIGH && flg_volume_updated) {
      flg_volume_updated = false;
    }
  }
}

// ------------------------------------------------------------------------------------------------------------------------------
// TextToSpeech() - Using (by default) OpenAI TTS services via function 'openai_speech()' in @Schreibfaul1's AUDIO.H library.
// - [NEW]: Automatically using tts parameter from current active AI FRIEND (calling 'get_tts_param()' in lib_openai_groq.ini)
// - Any other TTS service (beyond Open AI) could be added below, see example snippets for free Google TTS and SpeechGen TTS
// - IMPORTANT: Be aware of AUDIO.H dependency -> CHECK bottom line (UPDATE in case older AUDIO.H used) to avoid COMPILER ERRORS!
// ------------------------------------------------------------------------------------------------------------------------------

void TextToSpeech(String p_request) {
  // OpenAI voices are multi-lingual :) .. 9 tts-1 voices (August 2025): alloy|ash|coral|echo|fable|onyx|nova|sage|shimmer
  // Supported audio formats (response): aac | mp3 | wav (sample rate issue) | flac (PSRAM needed)
  // Known AUDIO.H issue: Latency delay (~1 sec) before voice starts speaking (performance improved with latest AUDIO.H)

  // Params of AUDIO.H .openai_speech():
  // - model:          Available TTS models: tts-1 | tts-1-hd | gpt-4o-mini-tts (gpt models needed for 'voice instruct' !)
  // - request:        The text to generate audio for. The maximum length is 4096 characters.
  // - voice_instruct: (Optional) forcing voice style (e.g. "you are whispering"). Needs AUDIO.H >= 3.1.0 & gpt-xy-tts !
  // - voice:          Voice name (multilingual), May 2025: alloy|ash|coral|echo|fable|onyx|nova|sage|shimmer
  // - format:         supported audio formats: aac | mp3 | wav (sample rate issue) | flac (PSRAM needed)
  // - vspeed:         The speed of the generated voice. Select a value from 0.25 to 4.0. DEFAULT: 1.0

  static int id_before;
  int id;
  String names, model, voice, vspeed, instruction, welcome;
  get_tts_param(&id, &names, &model, &voice, &vspeed, &instruction, &welcome);  // requesting tts values for current FRIEND

  Serial.print("TTS Audio [" + names + "|" + voice + "|" + model + "]");                      // e.g. TTS AUDIO [SUSAN | nova/gpt-4o-mini-tts]
  audio_play.openai_speech(OPENAI_KEY, model, p_request, instruction, voice, "aac", vspeed);  // <- use if version >= 3.1.0u
}
