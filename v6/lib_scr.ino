#define SCR_W 240
#define SCR_H 320
#define TFT_CONTENT_ZONE_Y 64
#define LINE_HEIGHT 16
#define LINE_BEGIN 16

#pragma region Text Wrap Logic
#define MAX_TFT_PAGES 10
#define TEXT_W 230

String TFT_PAGES[MAX_TFT_PAGES];
uint16_t TFT_PAGES_LEN = 0;
uint16_t TFT_PAGES_REPEAT = 1;

unsigned long TFT_LAST = 0;

bool tftHasLongMsg = false;
int tftCurrentShowingMsgPage = 0;
#define TFT_PAGES_DELAY 5000

void scrPrepareTftPages(const char* msg, uint16_t startY) {
  TFT_PAGES_LEN = 0;
  for (uint16_t i = 0; i < MAX_TFT_PAGES; i++) TFT_PAGES[i] = "";

  const uint16_t MAX_LINES_PER_PAGE = (SCR_H - startY) / LINE_HEIGHT;
  uint16_t currentLineCount = 0;
  String word = ""; String line = "";

  int i = 0;
  while (true) {
    if (TFT_PAGES_LEN >= MAX_TFT_PAGES) {
      if (TFT_PAGES_LEN > 0) TFT_PAGES[MAX_TFT_PAGES - 1] += "...\n";
      break;
    }
    char c = msg[i];
    if (c != ' ' && c != '\0') {
      word += c; i++; continue;
    }

    if (tft.getUTF8Width(word.c_str()) > TEXT_W) {
      for (uint16_t k = 0; k < word.length(); k++) {
        String test = line + word[k];
        if (tft.getUTF8Width(test.c_str()) <= TEXT_W) {
          line = test;
        } else {
          TFT_PAGES[TFT_PAGES_LEN] += line + "\n";
          currentLineCount++;
          line = word[k];
          if (currentLineCount >= MAX_LINES_PER_PAGE) {
            TFT_PAGES_LEN++;
            if (TFT_PAGES_LEN >= MAX_TFT_PAGES) {
              TFT_PAGES[MAX_TFT_PAGES - 1] += "...\n"; return;
            }
            currentLineCount = 0;
          }
        }
      }
      word = ""; if (c == '\0') break; i++; continue;
    }

    String testLine = line + (line.length() ? " " : "") + word;
    if (tft.getUTF8Width(testLine.c_str()) <= TEXT_W) {
      line = testLine;
    } else {
      TFT_PAGES[TFT_PAGES_LEN] += line + "\n";
      currentLineCount++;
      line = word;
      if (currentLineCount >= MAX_LINES_PER_PAGE) {
        TFT_PAGES_LEN++;
        if (TFT_PAGES_LEN >= MAX_TFT_PAGES) {
          TFT_PAGES[MAX_TFT_PAGES - 1] += "...\n"; return;
        }
        currentLineCount = 0;
      }
    }
    word = ""; if (c == '\0') break; i++;
  }
  if (TFT_PAGES_LEN < MAX_TFT_PAGES && line.length()) {
    TFT_PAGES[TFT_PAGES_LEN] += line + "\n";
    TFT_PAGES_LEN++;
  }
  if (TFT_PAGES_LEN > MAX_TFT_PAGES) TFT_PAGES_LEN = MAX_TFT_PAGES;
}

void scrWrapLoopKickstart() {
  tftHasLongMsg = true;
  tftCurrentShowingMsgPage = 0;
  
  TFT_LAST = millis();

  // Clear content area
  display.fillRect(0, 48, SCR_W, 240, ST77XX_WHITE);
  scrDrawTextSpaced(TFT_PAGES[tftCurrentShowingMsgPage], TFT_CONTENT_ZONE_Y);
  tftCurrentShowingMsgPage++;
}

void scrWrapLoopShutdown() {
  tftHasLongMsg = false;
  tftCurrentShowingMsgPage = 0;
}

void scrWrapLoop() {  
  if (!tftHasLongMsg) {
    return; 
  }
  if (tftCurrentShowingMsgPage < 0) {
    scrWrapLoopShutdown();
    return;
  }  
  // đang còn loop; set về 0 và trừ repeat
  if (tftCurrentShowingMsgPage >= TFT_PAGES_LEN) {
    tftCurrentShowingMsgPage = 0;
    TFT_PAGES_REPEAT--;
  }
  // hết repeat; dừng luôn
  if (TFT_PAGES_REPEAT <= 0) {
    scrWrapLoopShutdown();
  }

  if (millis() - TFT_LAST >= TFT_PAGES_DELAY)
  {
    // Clear content area
    display.fillRect(0, 48, SCR_W, 240, ST77XX_WHITE);
    tft.setCursor(20, 62);

    scrDrawTextSpaced(TFT_PAGES[tftCurrentShowingMsgPage], TFT_CONTENT_ZONE_Y);
    tftCurrentShowingMsgPage++;
    TFT_LAST = millis();
  }
}
#pragma endregion Text Wrap Logic


#pragma region LibScreen

void scrInit() {
  // không init SPI tại đây vì trong setup() init rồi nhé!
  delay(100);
  
  // // ST7735
  // display.initR(INITR_144GREENTAB);
  // display.setRotation(0);

  // For ST7789, use init(width, height)
  display.init(240, 320);   // adjust resolution (240x240, 240x320, etc.)
  display.setRotation(0);   // optional, set orientation
  display.invertDisplay(false);
  
  tft.begin(display);

  scrClear();
     
  tft.setFontMode(1);          
  tft.setFontDirection(0);   
  tft.setBackgroundColor(ST77XX_WHITE);        
  tft.setForegroundColor(ST77XX_BLACK);
  tft.setFont(u8g2_font_unifont_t_vietnamese1);  

  scrClear();

  // Vẽ màn hình khởi động
  display.drawXBitmap(56, 32, epd_bitmap_lgNovaEsp, 128, 48, ST77XX_GREEN); 
  scrDrawMessageFixed(0, 96, "  Đang khởi động hệ thống...");
  delay(50);
}

void scrInitVerbose(String msg) {
  display.fillRect(0, 116, SCR_W, 14, ST77XX_WHITE);
  tft.setCursor(20, 102);
  tft.print(msg);
}
void scrClear() {
  display.fillScreen(ST77XX_WHITE); 
}

void scrDrawIcon(const uint16_t x, const uint16_t y, const uint16_t w, const uint16_t h, const uint8_t* icon, const uint16_t color) {
  display.fillRect(x, y, w, h, ST77XX_WHITE);
  display.drawXBitmap(x, y, icon, w, h, color);
}

void scrDrawMessageFixed(const uint16_t x, const uint16_t y, String msg)
{ 
  tft.setCursor(x, y + LINE_HEIGHT);
  tft.print(msg);
}

// Hiển thị text có cách dòng LINE_HEIGHT
void scrDrawTextSpaced(String text, uint16_t startY) { 
  uint16_t y = startY;

  tft.setCursor(LINE_BEGIN, y);
  for (int i=0; i<text.length(); i++) {
    char ci = text[i];
    
    if (ci == '\n') {
      y += LINE_HEIGHT;
      tft.setCursor(LINE_BEGIN, y);
      continue;
    }
    
    tft.print(ci);
  }
}

void scrDrawHomeScreen()
{
  uint16_t x = 8;  
  uint16_t y = 32;

  tft.setCursor(0, 92);
  display.fillRect(0, 20, SCR_W, 240, ST77XX_WHITE);
  display.drawXBitmap(108, 32, NV_OKAY ? epd_bitmap_icoSmile : epd_bitmap_icoStressed , 24, 24, NV_OKAY ? ST77XX_GREEN : ST77XX_RED); 

  tft.print("  Nhiệt độ: ");
  tft.print(ssTemperature);
  tft.print("°C");
  tft.println();

  tft.print("  Độ ẩm: ");
  tft.print(ssHumidity);
  tft.print(" %RH");
  tft.println();

  tft.print("  Ánh sáng: ");
  tft.print(ssLightAo);
  tft.print(" lux");
  tft.println();

  tft.print("  Đất: ");
  tft.print(soilValue ? "Khô" : "Ẩm");
  tft.println();

  tft.print("  Lần tưới gần đây:");
  tft.print("\n      ");
  tft.print(sysConvertMillisToTimeString(millis() - soilLastWaterTime, 0));
  tft.println();

  tft.print("  Thời gian phơi nắng:");
  tft.print("\n      ");
  tft.print(sysConvertMillisToTimeString(sunMillis, 1));
  tft.println();
}

void scrDrawVolScreen() {
  
  tft.setCursor(0, 92);
  display.fillRect(0, 20, SCR_W, 240, ST77XX_WHITE);
  display.drawXBitmap(108, 32, epd_bitmap_icoVol, 24, 24, ST77XX_BLUE); 
  
  tft.println("  Âm lượng: ");
  tft.print("  ");
  tft.print(volume_level * 10);
  tft.print(" %");
  tft.println();
}

void scrDrawMailScreen() {
  tft.setCursor(0, 92);
  display.fillRect(0, 20, SCR_W, 240, ST77XX_WHITE);
  display.drawXBitmap(108, 32, epd_bitmap_icoMail, 24, 24, ST77XX_GREEN); 
  
  tft.println("  Hãy bấm SELECT để gửi\n  thư điện tử trạng thái.");
}

void scrDrawMailTaskScreen() {
  tft.setCursor(0, 92);
  display.fillRect(0, 20, SCR_W, 240, ST77XX_WHITE);
  display.drawXBitmap(108, 32, epd_bitmap_icoMail, 24, 24, ST77XX_RED); 
  
  tft.println("  Chờ tí nhé!\n  Mail đang tới nè...");
}

void scrDrawInfoScreen() {
  tft.setCursor(0, 92);
  display.fillRect(0, 20, SCR_W, 240, ST77XX_WHITE);
  display.drawXBitmap(108, 32, epd_bitmap_icoSetup, 24, 24, ST77XX_BLUE); 
  
  tft.println("  Thông tin đăng ký của bạn:");
  tft.println("  Tên cây:");
  tft.print  ("    ");
  tft.println(PLANT_NAME);
  tft.println("  Email:");
  tft.print  ("    ");
  tft.println(RECIPIENT_EMAIL);
  tft.println("  Tên người dùng:");
  tft.print  ("    ");
  tft.println(YOU_NAME);
  tft.println("  Kết nối Wi-Fi:");
  tft.print  ("    ");
  tft.println(wmSsid);
}

void scrDrawInfo2Screen() {
  tft.setCursor(0, 92);
  display.fillRect(0, 20, SCR_W, 240, ST77XX_WHITE);
  display.drawXBitmap(108, 32, epd_bitmap_icoSetup, 24, 24, ST77XX_GREEN); 
  
  tft.println("  Một vài thông số:");
  tft.println("  Bạn đã đồng hành cùng tớ:");
  tft.print  ("    ");
  tft.println(1);
  tft.println("  ngày.");
  tft.println("  Số lần trò chuyện:");
  tft.print  ("    ");
  tft.println("3 lần");
  tft.println("  Thông tin chăm sóc:");
  tft.print  ("    ");
  tft.println("Chi tiết tại trang 2");
}

void scrDrawResetScreen() {
  tft.setCursor(0, 92);
  display.fillRect(0, 20, SCR_W, 240, ST77XX_WHITE);
  display.drawXBitmap(108, 32, epd_bitmap_icoReplay, 24, 24, ST77XX_RED); 
  
  tft.println("  Bạn hãy bấm SELECT\n  để khởi động lại hệ thống\n  Để cấu hình lại tất cả,\n  hãy ấn giữ 2 nút khi khởi động.");
}

void scrDrawStatusBar() {
  tft.setCursor(48, 14);
  display.fillRect(0, 0, SCR_W, 14, ST77XX_WHITE);
  
  tft.print(sysGetDateTimeStringShort());
}

void scrDrawPageNumber() {
  tft.setCursor(72, 304);
  display.fillRect(0, 290, SCR_W, 40, ST77XX_WHITE);
  
  tft.print("TRANG ");
  tft.print(currentScreenPage + 1);
  tft.print("/5");
}

void scrDrawFullMsg(String msg) {
  scrClear();
  tft.setCursor(0, 20);  
  tft.print(msg);
}

#pragma endregion LibScreen
