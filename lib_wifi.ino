void wmSaveConfigCallback() {
  scrClear();
  scrDrawIcon(ICO_START_X, ICO_START_Y, ICO_ACT_DIMM, ICO_ACT_DIMM, epd_bitmap_icoWifi, ST77XX_RED);
  scrDrawMessageFixed(MSG_START_X, MSG_START_Y, wmsSaveRequest);

  wmShouldSaveConfig = true;
}

void wmSaveCreds(String newSsid, String newPwd, String wmstEmail, String wmstPlantName) {
  prefs.putString("ssid", newSsid);
  prefs.putString("pwd", newPwd);
  prefs.putString("wmstEmail", wmstEmail);
  prefs.putString("wmstPlantName", wmstPlantName);
  
  scrClear();
  scrDrawIcon(ICO_START_X, ICO_START_Y, ICO_ACT_DIMM, ICO_ACT_DIMM, epd_bitmap_icoSetup, ST77XX_RED);
  scrDrawMessageFixed(MSG_START_X, MSG_START_Y, wmsSaveSuccess);
}

void wmReadCreds() {
  scrClear();
  scrDrawIcon(ICO_START_X, ICO_START_Y, ICO_ACT_DIMM, ICO_ACT_DIMM, epd_bitmap_icoWifi, ST77XX_YELLOW);
  scrDrawMessageFixed(MSG_START_X, MSG_START_Y, wmsReading);

  wmSsid = prefs.getString("ssid", "");
  wmPwd = prefs.getString("pwd", "");
  
  delay(500);
}

void wmConfig() {
  scrClear();

  scrDrawIcon(ICO_START_X, ICO_START_Y, ICO_ACT_DIMM, ICO_ACT_DIMM, epd_bitmap_icoSetup, ST77XX_BLUE);
  scrDrawMessageFixed(MSG_START_X, MSG_START_Y, wmsPleaseConfig);
  
  WiFiManagerParameter emailField("email", "Địa chỉ nhận email thông báo? (tùy chọn)", "", 128);
  wm.addParameter(&emailField);

  WiFiManagerParameter plnameField("plant name", "Cây bạn đang trồng là cây gì? (tùy chọn)", "", 128);
  wm.addParameter(&plnameField);
  
  wm.startConfigPortal(wmBroadcast);
  if (wmShouldSaveConfig) {
    wmSaveCreds(wm.getWiFiSSID(), wm.getWiFiPass(), emailField.getValue(), plnameField.getValue());
    ESP.restart();
  }
}

void wmConnect() {
  wmReadCreds();
  wm.autoConnect(wmSsid.c_str(), wmPwd.c_str());
  
  scrClear();
  scrDrawIcon(ICO_START_X, ICO_START_Y, ICO_ACT_DIMM, ICO_ACT_DIMM, epd_bitmap_icoWifi, ST77XX_GREEN);
  scrDrawMessageFixed(MSG_START_X, MSG_START_Y, wmsEstablished);
}

void wmInit() {
  wm.setSaveConfigCallback(wmSaveConfigCallback);
}
