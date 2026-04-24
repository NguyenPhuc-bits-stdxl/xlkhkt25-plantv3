void wmSaveConfigCallback() {
  scrClear();
  scrDrawIcon(ICO_START_X, ICO_START_Y, ICO_ACT_DIMM, ICO_ACT_DIMM, epd_bitmap_icoWifi, ST77XX_RED);
  scrDrawMessageFixed(MSG_START_X, MSG_START_Y, wmsSaveRequest);

  wmShouldSaveConfig = true;
}

void wmSaveCreds(String newSsid, String newPwd, String wmstPlantName, String wmstEmail, String wmstCall, String wmstName, String wmstDesc, String wmstAge) {
  prefs.putString("ssid", newSsid);
  prefs.putString("pwd", newPwd);
  prefs.putString("wmstPlantName", wmstPlantName);
  prefs.putString("wmstEmail", wmstEmail);
  prefs.putString("wmstCall", wmstCall);
  prefs.putString("wmstName", wmstName);
  prefs.putString("wmstDesc", wmstDesc);
  prefs.putString("wmstAge", wmstAge);
  
  // sang Phase 2 của setup - lấy thông tin cây - set flag FALSE
  prefs.putBool("wmstSetupDone", false);

  scrClear();
  scrDrawIcon(ICO_START_X, ICO_START_Y, ICO_ACT_DIMM, ICO_ACT_DIMM, epd_bitmap_icoSetup, ST77XX_RED);
  scrDrawMessageFixed(MSG_START_X, MSG_START_Y, wmsSaveSuccess);
  
  ESP.restart();
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
  
  WiFiManagerParameter plnameField("plname", "Cây bạn đang trồng là? (tùy chọn)", "", 128);
  wm.addParameter(&plnameField);
  
  WiFiManagerParameter emailField("email", "Địa chỉ nhận email thông báo? (tùy chọn)", "", 128);
  wm.addParameter(&emailField);

  WiFiManagerParameter callField("callU", "Mình nên gọi bạn bằng (anh, chị, em, chú, cô, bác...)? (tùy chọn)", "", 16);
  wm.addParameter(&callField);

  WiFiManagerParameter nameField("nameU", "Tên của bạn là gì? (tùy chọn)", "", 32);
  wm.addParameter(&nameField);

  WiFiManagerParameter describeField("describeU", "Mô tả tính cách của bạn bằng 3 đến 5 từ? (tùy chọn)", "", 128);
  wm.addParameter(&describeField);
  
  WiFiManagerParameter ageField("ageU", "Tuổi của bạn? (tùy chọn)", "", 32);
  wm.addParameter(&ageField);
    
  wm.startConfigPortal(wmBroadcast);
  if (wmShouldSaveConfig) {
    wmSaveCreds(wm.getWiFiSSID(), wm.getWiFiPass(), plnameField.getValue(), emailField.getValue(), callField.getValue(), nameField.getValue(), describeField.getValue(), ageField.getValue());
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
