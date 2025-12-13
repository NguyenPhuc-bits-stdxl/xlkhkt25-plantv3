void wmSaveConfigCallback() {
  scrDrawMessage(MSG_START_X, MSG_START_Y, wmsSaveRequest, true, true);
  wmShouldSaveConfig = true;
}

void wmSaveCreds(String newSsid, String newPwd) {
  prefs.putString("ssid", newSsid);
  prefs.putString("pwd", newPwd);

  scrDrawMessage(MSG_START_X, MSG_START_Y, wmsSaveSuccess, true, true);
}

void wmReadCreds() {
  scrDrawMessage(MSG_START_X, MSG_START_Y, wmsReading, true, true);

  wmSsid = prefs.getString("ssid", "");
  wmPwd = prefs.getString("pwd", "");
  
  delay(500);
}

void wmConfig() {
  scrDrawMessage(MSG_START_X, MSG_START_Y, wmsPleaseConfig, true, true);

  wm.startConfigPortal(wmBroadcast);
  if (wmShouldSaveConfig) {
    wmSaveCreds(wm.getWiFiSSID(), wm.getWiFiPass());
    ESP.restart();
  }
}

void wmConnect() {
  wmReadCreds();
  wm.autoConnect(wmSsid.c_str(), wmPwd.c_str());
  
  scrDrawMessage(MSG_START_X, MSG_START_Y, wmsEstablished, true, true);
}

void wmInit() {
  wm.setSaveConfigCallback(wmSaveConfigCallback);
}
