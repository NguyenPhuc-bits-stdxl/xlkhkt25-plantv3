# Giới thiệu
Chậu cây thông minh phiên bản 3 (Sản phẩm dự thi giai đoạn tái cấu trúc lần thứ 2)

# Big thanks to kaloprojects
Chân thành cảm ơn đến tác giả kaloprojects đã truyền cảm hứng cho dự án Cây xanh AI Nova của nhóm chúng em!
Bạn, người xem repo này, có thể muốn xem thử repo của tác giả kaloprojects qua link ở dưới.

Thank you for inspiring our ESP32 AI Plant Friend (Nova Plant) project!
You, the viewer, may wanna have a check of his repository at the link down below.
[kaloprojects' project](https://github.com/kaloprojects/KALO-ESP32-Voice-Chat-AI-Friends/tree/main/KALO_ESP32_Voice_Chat_AI_Friends)

## Flash configuration
**Board: "ESP32S3 Dev Module"**

USB CDC On Boot: "Disabled"

CPU Frequency: "240MHz (WiFi)"

Core Debug Level: "None"

USB DFU On Boot: "Disabled"

Erase All Flash Before Sketch Upload: "Disabled"

Events Run On: "Core 1"

Flash Mode: "QIO 80MHz"

**Flash Size: "16MB (128Mb)"**

JTAG Adapter: "Disabled"

Arduino Runs On: "Core 1"

USB Firmware MSC On Boot: "Disabled"

**Partition Scheme: "16M Flash (3MB APP/9.9MB FATFS)"**

**PSRAM: "OPI PSRAM"**

Upload Mode: "UARTO / Hardware CDC"

Upload Speed: "921600"

USB Mode: "Hardware CDC and JTAG"

Zigbee Mode: "Disabled"

## Changes made:
- SD card functionality removed
- Prompts reconfigured to Vietnamese
- Supported reading sensors (DHT + LDR)
- Display added (ST7735 + U8g2 for AdaGFX)
- Reset buton is introduced
- Added functionality to configure WiFi by sending RESET command at startup
- Added the ability to sense uncomfortable conditions and ask for help if needed

# Dependencies
## Via Boards Manager
- ESP32 boards by Adafruit version 3.x
## Via Library Manager
- Audio by schreibfaul1 3.4.2
- WiFiManager by tzapu 2.0.17
- Adafruit ST7735 and ST7789 library by Adafruit 1.11.0
- [U8g2_for_Adafruit_GFX by olikraus 1.8.0](https://github.com/olikraus/U8g2_for_Adafruit_GFX)
- Adafruit GFX/SPI/GLCD fonts libraries
- Adafruit Unified Sensor by Adafruit
- DHT kxn by Adafruit
- DHT sensor library by Adafruit

# Diagram
```cpp
// --- SENSORS ---
const int DHTPIN = 40;
const int LDR_AO = 4;

// --- TFT SCREEN ---
#define LCD_CS 10
#define LCD_DC 11
#define LCD_SCLK 14
#define LCD_MOSI 13
#define LCD_RST 12

// --- I2S AMP ---
#define pin_I2S_DOUT 16 // trên mạch là DIN
#define pin_I2S_LRC 8  
#define pin_I2S_BCLK 3

// --- BUTTONS ---
#define pin_VOL_BTN 41      
#define pin_RECORD_BTN 38   

// --- I2S RECORDING ---
#define I2S_LR            LOW // nối với GND
#define I2S_WS            17            
#define I2S_SD            15        
#define I2S_SCK           18          
```

# Audio settings                      
```cpp    
#define SAMPLE_RATE       16000
#define BITS_PER_SAMPLE   16    
#define GAIN_BOOSTER_I2S  6   
```
