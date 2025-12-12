void wmSaveConfigCallback() {
  scrShowMessage(wmsSaveRequest);
  wmShouldSaveConfig = true;
}

void wmSaveCreds(String newSsid, String newPwd) {
  prefs.putString("ssid", newSsid);
  prefs.putString("pwd", newPwd);
  scrShowMessage(wmsSaveSuccess);
}

void wmReadCreds() {
  scrShowMessage(wmsReading);
  wmSsid = prefs.getString("ssid", "");
  wmPwd = prefs.getString("pwd", "");
  
  delay(1000);
}

void wmConfig() {
  wm.startConfigPortal(wmBroadcast);
  scrShowMessage(wmsPleaseConfig);
  if (wmShouldSaveConfig) {
    wmSaveCreds(wm.getWiFiSSID(), wm.getWiFiPass());
    ESP.restart();
  }
}

void wmConnect() {
  wmReadCreds();
  wm.autoConnect(wmSsid.c_str(), wmPwd.c_str());
  scrShowMessage(wmsEstablished);
}

void wmInit() {
  wm.setSaveConfigCallback(wmSaveConfigCallback);
}
