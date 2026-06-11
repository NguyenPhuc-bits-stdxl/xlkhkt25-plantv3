#pragma region LibSD

// Khởi tạo thẻ SD
void sdInit() {
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, -1); 
  if (!SD.begin(SD_CS)) {
    Serial.println("SYS SD card not found");
    flg_SD_found = false;
  } else {
    Serial.println("SYS SD Card initialized");
    flg_SD_found = true;
  }
}

// Phát âm thanh trên thẻ SD
void sdPlaySound(char* fileName) {
  if (flg_SD_found && SD.exists(fileName)) {
    Serial.println(String("SYS Playing audio file: ") + fileName);
    audio_play.connecttoFS(SD, fileName);
  } else {
    Serial.println("SYS Playing audio failed, file not found");
  }
}

#pragma endregion LibSD
