#pragma region LibSD

// Khởi tạo thẻ SD
void sdInit() {
  // không init SPI tại đây vì trong setup() init rồi nhé!
  // SPI.begin(SD_SCK, SD_MISO, SD_MOSI, -1); 
  if (!SD.begin(SD_CS, SPI, 8000000)) { //!SD.begin(SD_CS)) {
    Serial.println("SYS SD card not found");
    flg_SD_found = false;
  } else {
    Serial.println("SYS SD Card initialized");
    flg_SD_found = true;
  }
}

// Phát âm thanh trên thẻ SD
void sdPlaySound(char* fileName) {
  File f = SD.open("/welcome.wav");

  if (f) {
      Serial.printf("Size = %u\n", f.size());
      f.close();
  }
  else {
      Serial.println("Cannot open welcome.wav");
  }

  if (flg_SD_found && SD.exists(fileName)) {
    Serial.println(String("SYS Playing audio file: ") + fileName);
    audio_play.connecttoFS(SD, fileName);
  } else {
    Serial.println("SYS Playing audio failed, file not found");
  }
}

void sdfoolproofTest() {  
  // SD test
  File f = SD.open("/append.txt", FILE_WRITE);
  f.println("line1");
  f.close();
  f = SD.open("/append.txt", FILE_APPEND);
  if (!f) {
    Serial.println("APPEND FAILED");
  }
  else {
    Serial.println("APPEND WORKS");
    f.println("line2");
    f.close();
  }
}
#pragma endregion LibSD
