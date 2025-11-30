const char* wmBroadcast = "CAY XANH LILY";
const char* wmsPleaseConfig = "Connect to\n'CAY XANH LILY'\nto configure";
const char* wmsSaveRequest = "Receiving data\nfrom WiFiManager...";
const char* wmsSaveSuccess = "WiFi credentials\nare saved\nsuccessfully. Wait...";
const char* wmsReading = "Reading WiFi\nconfiguration...";

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
}

void wmInit() {
  wm.setSaveConfigCallback(wmSaveConfigCallback);
}
