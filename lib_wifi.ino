void wmSaveConfigCallback() {
  scrClear();
  scrDrawIcon(ICO_START_X, ICO_START_Y, ICO_ACT_DIMM, ICO_ACT_DIMM, epd_bitmap_icoWifi, ST77XX_RED);
  scrDrawMessageFixed(MSG_START_X, MSG_START_Y, wmsSaveRequest);

  wmShouldSaveConfig = true;
}

void wmSaveCreds(String newSsid, String newPwd) {
  prefs.putString("ssid", newSsid);
  prefs.putString("pwd", newPwd);

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

  wm.startConfigPortal(wmBroadcast);
  if (wmShouldSaveConfig) {
    wmSaveCreds(wm.getWiFiSSID(), wm.getWiFiPass());
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
