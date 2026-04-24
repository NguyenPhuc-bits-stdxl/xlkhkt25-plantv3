void wmSaveConfigCallback() {
  wmShouldSaveConfig = true;
}

void wmSaveCreds(String newSsid, String newPwd, String wmstPlantName, String wmstEmail, String wmstCall, String wmstName) {
  prefs.putString("ssid", newSsid);
  prefs.putString("pwd", newPwd);
  prefs.putString("wmstPlantName", wmstPlantName);
  prefs.putString("wmstEmail", wmstEmail);
  prefs.putString("wmstCall", wmstCall);
  prefs.putString("wmstName", wmstName);
  prefs.putString("wmstFirstRun", "Y");

  scrDrawFullMsg("  Đã cấu hình xong,\n  đang khởi động lại...");

  ESP.restart();
}

void wmReadCreds() {
  Serial.println("Wifi Reading creds...");

  wmSsid = prefs.getString("ssid", "");
  wmPwd = prefs.getString("pwd", "");
  
  delay(200);
}

void wmConfig() {
  scrDrawFullMsg("  Mở cài đặt mạng\n  Kết nối đến [NOVA]\n  và truy cập 192.168.1.4\n  để cấu hình");

  WiFiManagerParameter plnameField("plname", "Cây bạn đang trồng là? (tùy chọn)", "", 128);
  wm.addParameter(&plnameField);
  
  WiFiManagerParameter emailField("email", "Địa chỉ nhận email thông báo? (tùy chọn)", "", 128);
  wm.addParameter(&emailField);

  WiFiManagerParameter callField("callU", "Mình nên gọi bạn bằng (anh, chị, em, chú, cô, bác...)? (tùy chọn)", "", 16);
  wm.addParameter(&callField);

  WiFiManagerParameter nameField("nameU", "Tên của bạn là gì? (tùy chọn)", "", 32);
  wm.addParameter(&nameField);

  wm.startConfigPortal("NOVA");
  if (wmShouldSaveConfig) {
    wmSaveCreds(wm.getWiFiSSID(), wm.getWiFiPass(), plnameField.getValue(), emailField.getValue(), callField.getValue(), nameField.getValue());
  }
}

void wmConnect() {
  wmReadCreds();
  wm.autoConnect(wmSsid.c_str(), wmPwd.c_str());
    
  scrDrawFullMsg("  Kết nối thành công!");
}

void wmInit() {
  wm.setSaveConfigCallback(wmSaveConfigCallback);
}