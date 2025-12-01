void sysInit() {
  sysFetchCreds();

  // Sensors
  //pinMode(DHTPIN, INPUT_PULLUP);
  //pinMode(LDR_DO, INPUT);

  // Preferences, Sensors, WiFiManager
  prefs.begin("config", false);
  //dht.begin();
  wmInit();
  wmConnect();
  //scrInit();   // under construction

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