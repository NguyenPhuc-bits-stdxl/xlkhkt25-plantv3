// --- includes ----------------
#include "driver/i2s_std.h"       // important: older legacy #include <driver/i2s.h> no longer supported 

// === recording settings ====== 
#define RECORD_PSRAM      true   // true: store Audio Recording .wav in PSRAM (ESP32 with PSRAM needed, e.g. ESP32 Wrover)
                                  
// --- PCB: ESP32-S3 Dev ------------
#define I2S_LR            LOW    // LOW because L/R pin of INMP441 is connected to GND (default LEFT channel)                                 
#define I2S_WS            4            
#define I2S_SD            6        
#define I2S_SCK           5       
                        
                                  
// --- audio settings ----------                         
#define SAMPLE_RATE       16000  // typical values: 8000 .. 44100, use e.g 8K (and 8 bit mono) for smallest .wav files  
                                 // hint: best quality with 16000 or 24000 (above 24000: random dropouts and distortions)
                                 // recommendation in case the STT service produces lot of wrong words: try 16000 

#define BITS_PER_SAMPLE   8      // 16 bit and 8bit supported (24 or 32 bits not supported)
                                 // hint: 8bit is less critical for STT services than a low 8kHz sample rate
                                 // for fastest STT: combine 8kHz and 8 bit. 

#define GAIN_BOOSTER_I2S  32     // original I2S streams is VERY silent, so we added an optional GAIN booster for INMP441
                                 // multiplier, values: 1-64 (32 seems best value for INMP441)
                                 // 64: high background noise but still working well for STT on quiet human conversations
  
// --- global vars -------------

uint8_t* PSRAM_BUFFER;            // global array for RECORDED .wav (50% of PSRAM via ps_malloc() in I2S_Recording_Init()
                                  // (using 50% only to allow other functions using PSRAM too, e.g. AUDIO.H openai_speech() 
size_t PSRAM_BUFFER_max_usage;    // size of used buffer (50% of PSRAM)
size_t PSRAM_BUFFER_counter = 0;  // current pointer offset position to last recorded byte


// [std_cfg]: KALO I2S_std configuration for I2S Input device (Microphone INMP441), Details see esp32-core file 'i2s_std.h' 

i2s_std_config_t  std_cfg = 
{ .clk_cfg  =   // instead of macro 'I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),'
  { .sample_rate_hz = SAMPLE_RATE,
    .clk_src = I2S_CLK_SRC_DEFAULT,
    .mclk_multiple = I2S_MCLK_MULTIPLE_256,
  },
  // IMPORTANT (NEW): for .slot_cfg we use MACRO in 'i2s_std.h' (to support all ESP32 variants, ESP32-S3 supported too)
  // Datasheet INMP441: Microphone uses PHILIPS format (bc. Data signal has a 1-bit shift AND signal WS is NOT pulse lasting)
  
  .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG( I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO ), 
  
  .gpio_cfg =   
  { .mclk = I2S_GPIO_UNUSED,
    .bclk = (gpio_num_t) I2S_SCK,
    .ws   = (gpio_num_t) I2S_WS,
    .dout = I2S_GPIO_UNUSED,
    .din  = (gpio_num_t) I2S_SD,
    .invert_flags = 
    { .mclk_inv = false,
      .bclk_inv = false,
      .ws_inv = false,
    },
  },
};

// [re_handle]: global handle to the RX channel with channel configuration [std_cfg]
i2s_chan_handle_t  rx_handle;


// [myWAV_Header]: selfmade WAV Header:
struct WAV_HEADER 
{ char  riff[4] = {'R','I','F','F'};                        /* "RIFF"                                   */
  long  flength = 0;                                        /* file length in bytes - 8 [bug fix]       <= calc at end */ 
  char  wave[4] = {'W','A','V','E'};                        /* "WAVE"                                   */
  char  fmt[4]  = {'f','m','t',' '};                        /* "fmt "                                   */
  long  chunk_size = 16;                                    /* size of FMT chunk in bytes (usually 16)  */
  short format_tag = 1;                                     /* 1=PCM, 257=Mu-Law, 258=A-Law, 259=ADPCM  */
  short num_chans = 1;                                      /* 1=mono, 2=stereo                         */
  long  srate = SAMPLE_RATE;                                /* samples per second, e.g. 44100           */
  long  bytes_per_sec = SAMPLE_RATE * (BITS_PER_SAMPLE/8);  /* srate * bytes_per_samp, e.g. 88200       */ 
  short bytes_per_samp = (BITS_PER_SAMPLE/8);               /* 2=16-bit mono, 4=16-bit stereo (byte 34) */
  short bits_per_samp = BITS_PER_SAMPLE;                    /* Number of bits per sample, e.g. 16       */
  char  dat[4] = {'d','a','t','a'};                         /* "data"                                   */
  long  dlength = 0;                                        /* data length (filelength - 44) [bug fix]  <= calc at end */
} myWAV_Header;


bool flg_is_recording = false;         // only internally used

bool flg_I2S_initialized = false;      // to avoid any runtime errors in case user forgot to initialize

bool I2S_Recording_Init() 
{  
  // NEW: Updating I2S structure to the correct channel (LEFT and RIGHT supported)
  if (I2S_LR == HIGH) {std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_RIGHT;}  // manually updated, not supported via MACRO (LUCA)
  if (I2S_LR == LOW)  {std_cfg.slot_cfg.slot_mask = I2S_STD_SLOT_LEFT; }  // I2S default in MONO (STEREO creates wrong 'BOTH')
  
  // Get the default channel configuration by helper macro (defined in 'i2s_common.h')
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  
  i2s_new_channel(&chan_cfg, NULL, &rx_handle);     // Allocate a new RX channel and get the handle of this channel
  i2s_channel_init_std_mode(rx_handle, &std_cfg);   // Initialize the channel
  i2s_channel_enable(rx_handle);                    // Before reading data, start the RX channel first

  DebugPrintln( "> I2S_Recording_Init - Initializing Recording Setup:"     );
  DebugPrintln( "- Sketch Size: " + (String) ESP.getSketchSize()           );
  DebugPrintln( "- Total Heap:  " + (String) ESP.getHeapSize()             );
  DebugPrintln( "- Free Heap:   " + (String) ESP.getFreeHeap()             );
  DebugPrintln( "- STACK Size:  " + (String) getArduinoLoopTaskStackSize() ); 
  DebugPrintln( "- Flash Size:  " + (String) ESP.getFlashChipSize()        );
  DebugPrintln( "- Total PSRAM: " + (String) ESP.getPsramSize()            );
  DebugPrintln( "- Free PSRAM:  " + (String) ESP.getFreePsram()            );     

  if (RECORD_PSRAM)
  { // check if PSRAM exists -> if yes .. ACTION: allocating 50% of available PSRAM for recording buffer
    if (ESP.getFreePsram() > 0 )
    {  PSRAM_BUFFER_max_usage = ESP.getFreePsram() / 2;  
       int max_seconds = (PSRAM_BUFFER_max_usage / (SAMPLE_RATE * BITS_PER_SAMPLE/8));        
       if ( max_seconds < 5 )  // all below ~5 seconds for AUDIO Recording makes less sense
       {  Serial.println("* ERROR - Not enough free PSRAM found!. Stopped."); 
          while(true);   // END (waiting forever)
       }    
       else // all fine, so we allocate PSRAM for recording  
       {  PSRAM_BUFFER = (uint8_t*) ps_malloc(PSRAM_BUFFER_max_usage);      
          DebugPrintln("> ps_malloc(): " + (String) PSRAM_BUFFER_max_usage + " (allocating 50% of free PSRAM for Audio)" );  
          DebugPrintln("> PSRAM maximum audio recording length [sec]: " + (String) max_seconds + "\n");           
       }     
    }
    else
    {  Serial.println("* ERROR - No PSRAM found!. Stopped."); 
       while(true);  // END (waiting forever) 
    }     
  }
  
  flg_I2S_initialized = true;                       // all is initialized, checked in procedure Recording_Loop()
 
  return flg_I2S_initialized;  
}

bool Recording_Loop() 
{
  if (!flg_I2S_initialized)     // to avoid any runtime error in case user missed to initialize
  {  Serial.println( "* ERROR in Recording_Loop() - I2S not initialized, call 'I2S_Recording_Init()' missed" );    
     return false;
  }
  
  if (!flg_is_recording)  // entering 1st time -> remove old AUDIO file, create new file with WAV header
  { 
    flg_is_recording = true;
    
    if (RECORD_PSRAM)    
    {  PSRAM_BUFFER_counter = 44;   
       for (int i = 0; i < PSRAM_BUFFER_counter; i++)                 
       {   PSRAM_BUFFER[i] = ((uint8_t*)&myWAV_Header)[i];  // copying each byte of a struct var
       } 
       DebugPrintln("> WAV Header in PSRAM generated, Audio Recording started ... ");
    }
    
    // now proceed below (flg_is_recording is true) ....
  }
  
  if (flg_is_recording)  // here we land when recording started already -> task: append record buffer to file
  { 
    // Array to store Original audio I2S input stream (reading in chunks, e.g. 1024 values) 
    int16_t audio_buffer[1024];                // max. 1024 values [2048 bytes] <- for the original I2S signed 16bit stream 
    uint8_t audio_buffer_8bit[1024];           // max. 1024 values [1024 bytes] <- self calculated if BITS_PER_SAMPLE == 8

    // now reading the I2S input stream (with NEW <I2S_std.h>)
    size_t bytes_read = 0;
    i2s_channel_read(rx_handle, audio_buffer, sizeof(audio_buffer), &bytes_read, portMAX_DELAY);
    
    size_t values_recorded = bytes_read / 2;   // 1024 (also if 8bit, because I2S 'waste' 16 bit always, see below)
    
    // Optionally: Boostering the very low I2S Microphone INMP44 amplitude (multiplying values with factor GAIN_BOOSTER_I2S)  
    if ( GAIN_BOOSTER_I2S > 1 && GAIN_BOOSTER_I2S <= 64 );    // check your own best values, recommended range: 1-64
    for (int16_t i = 0; i < values_recorded; ++i)             // all 1024 values, 16bit (bytes_read/2) 
    {   audio_buffer[i] = audio_buffer[i] * GAIN_BOOSTER_I2S;  
    }

    // If 8bit requested: Calculate 8bit Mono files (self made because any I2S _8BIT settings in I2S would still waste 16bits)
    // use case: reduce resolution 16->8bit to archive smallest .wav size (e.g. for sending to SpeechToText services) 
    // details WAV 8bit: https://stackoverflow.com/questions/44415863/what-is-the-byte-format-of-an-8-bit-monaural-wav-file 
    // details Convert:  https://stackoverflow.com/questions/5717447/convert-16-bit-pcm-to-8-bit
    // 16-bit signed to 8-bit WAV conversion rule: FROM -32768...0(silence)...+32767 -> TO: 0...128(silence)...256 

    if (BITS_PER_SAMPLE == 8) // in case we store a 8bit WAV file we fill the 2nd array with converted values
    { for (int16_t i = 0; i < ( values_recorded ); ++i)        
      { audio_buffer_8bit[i] = (uint8_t) ((( audio_buffer[i] + 32768 ) >>8 ) & 0xFF); 
      }
    }
    
    /* // Optional (to visualize and validate I2S Microphone INMP44 stream): displaying first 16 samples of each chunk
    DebugPrint("> I2S Rx Samples [Original, 16bit signed]:    ");
    for (int i=0; i<16; i++) { DebugPrint( (String) (int) audio_buffer[i] + "\t"); } 
    DebugPrintln();
    if (BITS_PER_SAMPLE == 8)    
    {   DebugPrint("> I2S Rx Samples [Converted, 8bit unsigned]:  ");
        for (int i=0; i<16; i++) { DebugPrint( (String) (int) audio_buffer_8bit[i] + "\t"); } 
        DebugPrintln("\n");
    } */      

    if (RECORD_PSRAM)    
    {  // Append audio data to PSRAM 
       if (BITS_PER_SAMPLE == 16)  // for each value 2 bytes needed
       {  if (PSRAM_BUFFER_counter + values_recorded * 2 < PSRAM_BUFFER_max_usage)
          {  memcpy ( (PSRAM_BUFFER + PSRAM_BUFFER_counter), audio_buffer, values_recorded * 2 ); 
             PSRAM_BUFFER_counter += values_recorded * 2; 
          }  else { Serial.println("* WARNING - PSRAM full, Recording stopped."); }
       }        
       if (BITS_PER_SAMPLE == 8)  
       {  if (PSRAM_BUFFER_counter + values_recorded < PSRAM_BUFFER_max_usage)
          {  memcpy ( (PSRAM_BUFFER + PSRAM_BUFFER_counter), audio_buffer_8bit, values_recorded ); 
             PSRAM_BUFFER_counter += values_recorded;    
          }  else { Serial.println("* WARNING - PSRAM full, Recording stopped."); }
       }  
    }
  }  
  return true;
}

bool Recording_Stop( String* audio_filename, uint8_t** buff_start, long* audiolength_bytes, float* audiolength_sec ) 
{
  // Action: STOP recording and finalize recorded wav
  // Do nothing to in case no Record was started, recap: 'false' means: 'nothing is stopped' -> no action at all
  // important because typically 'Record_Stop()' is called always in main loop()  
  
  if (!flg_is_recording) 
  {   return false;   
  }
  
  if (!flg_I2S_initialized)   // to avoid runtime errors: do nothing in case user missed to initialize at all
  {  return false;
  }
  
  // here we land when Recording is active .. 
  // Tasks: 1. finalize WAV file header (final length tags),   2. fill return values (via &pointer), 
  //        3. stop recording (with flg_is_recording = false), 4. return true/done to main loop()
  
  if (flg_is_recording)       
  { 
    flg_is_recording = false;  // important: this is done only here (means after wav finalized we are done)
    
    // init default values 
    *audio_filename = "";
    *buff_start = NULL;      
    *audiolength_bytes = 0;
    *audiolength_sec = 0;
    
    if (RECORD_PSRAM)   
    {  myWAV_Header.flength = (long) PSRAM_BUFFER_counter -  8;  
       myWAV_Header.dlength = (long) PSRAM_BUFFER_counter - 44;  
       // copy each byte of a struct var again:
       for (int i = 0; i < 44; i++)   // same as on init:  
       {   PSRAM_BUFFER[i] = ((uint8_t*)&myWAV_Header)[i];  
       } 

       // return updated values via REFERENCE (pointer):
       *buff_start        = PSRAM_BUFFER;           // comment: buff_start is a pointer TO the pointer of PSRAM ;)
       *audiolength_bytes = PSRAM_BUFFER_counter;
       *audiolength_sec   = (float) (PSRAM_BUFFER_counter-44) / (SAMPLE_RATE * BITS_PER_SAMPLE/8);   

       DebugPrintln("> ... Done. Audio Recording into PSRAM finished.");
       DebugPrintln("> Bytes recorded: " + (String) *audiolength_bytes + ", audio length [sec]: " + (String) *audiolength_sec );
    }
    
    // Record is done (stored either in PSRAM or on SD card)
    flg_is_recording = false;  // important: this is done only here (any next Recording_Stop() calls have no action)
    return true;               // means: telling the main loop that new record is available now 
  }    
}
