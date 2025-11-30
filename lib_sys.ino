// -- SYSTEM STRINGS ---

const char* systrReset = "RESET button\nis pressed!";
const char* systrHot = "Ở đây nóng quá bạn ơi!";
const char* systrCold = "Mình lạnh quá bạn ơi!";
const char* systrThirst = "Bạn ơi mình khát quá!";
const char* systrDark = "Trời tối quá bạn ơi, cho mình xin tí ánh sáng nhé!";

// --- SENSORS CONFIGURATION ---

const int DHTPIN = 17;      
const int DHTTYPE = DHT11;  
DHT dht(DHTPIN, DHTTYPE);

const int LDR_AO = 15;
const int LDR_DO = 16;

int ssLightAo;
bool ssLightDo; 
float ssHumidity;
float ssTemperature;

// ----- FUNCTIONS -----

void sysInit() {
  void sysFetchCreds();

  pinMode(DHTPIN, INPUT_PULLUP);
  pinMode(LDR_DO, INPUT);

  // Preferences, WiFiManager
  prefs.begin("config", false);
  dht.begin();
  wmInit();
  wmConnect();
  // scrInit();   // under construction

  if (sysIsResetPressed()) {
    delay(1000);
    sysReset();
  }
}

void sysReset() {
  scrShowMessage(systrReset);
  wmConfig();
}

bool sysIsResetPressed() {
  // --- code goes here ---
  return false;
}

void sysReadSensors() {
  ssLightAo = analogRead(LDR_AO);
  ssLightDo = digitalRead(LDR_DO);
  ssHumidity = dht.readHumidity();    
  ssTemperature = dht.readTemperature(); 

  if (isnan(ssHumidity) || isnan(ssTemperature)) {
    ssHumidity = -1;
    ssTemperature = -1;
  }
}