#define RECORD_PSRAM      true   
#define RECORD_SDCARD     false
#define AUDIO_FILE        "/record.wav"   // mandatory if RECORD_SDCARD is true: filename for the AUDIO recording                                                
#define SAMPLE_RATE       16000
#define BITS_PER_SAMPLE   16
#define GAIN_BOOSTER_I2S  1

uint8_t* PSRAM_BUFFER;            // global array for RECORDED .wav (50% of PSRAM via ps_malloc() in I2S_Recording_Init()
                                  // (using 50% only to allow other functions using PSRAM too, e.g. AUDIO.H openai_speech() 
size_t PSRAM_BUFFER_max_usage;    // size of used buffer (50% of PSRAM)
size_t PSRAM_BUFFER_counter = 0;  // current pointer offset position to last recorded byte

i2s_std_config_t  std_cfg = 
{ .clk_cfg  =   // instead of macro 'I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),'
  { .sample_rate_hz = SAMPLE_RATE,
    .clk_src = I2S_CLK_SRC_DEFAULT,
    .mclk_multiple = I2S_MCLK_MULTIPLE_256,
  },  
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
   
  Serial.println( "> I2S_Recording_Init - Initializing Recording Setup:"     );
  delay(1000);

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
          Serial.println("> ps_malloc(): " + (String) PSRAM_BUFFER_max_usage + " (allocating 50% of free PSRAM for Audio)" );  
          Serial.println("> PSRAM maximum audio recording length [sec]: " + (String) max_seconds + "\n");           
       }     
    }
    else
    {  Serial.println("* ERROR - No PSRAM found!. Stopped."); 
       while(true);  // END (waiting forever) 
    }     
  }
  
  if (RECORD_SDCARD)
  { 
    // changed v4 11/06: we do nothing here because lib_sd already init'ed it!  
  }  
  
  Serial.println("I2S recording init success!");
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
       Serial.println("> WAV Header in PSRAM generated, Audio Recording started ... ");
    }
    if (RECORD_SDCARD)     
    {  if (SD.exists(AUDIO_FILE)) 
       {  SD.remove(AUDIO_FILE);  // because we start with a net new file
       }     
       File audio_file = SD.open(AUDIO_FILE, FILE_WRITE);
       
       if (!audio_file) {
         Serial.println("SD something went nuked Loop removal");
       }

       audio_file.write((uint8_t *) &myWAV_Header, 44);
       audio_file.close();        
       Serial.println("> WAV Header stored on SD card, Audio Recording started ... ");
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
    i2s_channel_read(
      rx_handle,
      audio_buffer,
      sizeof(audio_buffer),
      &bytes_read,
      pdMS_TO_TICKS(1000)
    );

    size_t values_recorded = bytes_read / 2;   // 1024 (also if 8bit, because I2S 'waste' 16 bit always, see below)
    
    // Optionally: Boostering the very low I2S Microphone INMP44 amplitude (multiplying values with factor GAIN_BOOSTER_I2S)  
    if ( GAIN_BOOSTER_I2S > 1 && GAIN_BOOSTER_I2S <= 64 )    // check your own best values, recommended range: 1-64
    for (int16_t i = 0; i < values_recorded; ++i)             // all 1024 values, 16bit (bytes_read/2) 
    {   audio_buffer[i] = audio_buffer[i] * GAIN_BOOSTER_I2S;  
    }

    if (BITS_PER_SAMPLE == 8) // in case we store a 8bit WAV file we fill the 2nd array with converted values
    { for (int16_t i = 0; i < ( values_recorded ); ++i)        
      { audio_buffer_8bit[i] = (uint8_t) ((( audio_buffer[i] + 32768 ) >>8 ) & 0xFF); 
      }
    }

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
    if (RECORD_SDCARD)
    {  // Save audio data to SD card (appending chunk array to file end)
       File audio_file = SD.open(AUDIO_FILE, FILE_APPEND);

       if (!audio_file) {
         Serial.println("SD something went nuked Append");
       }

       if (BITS_PER_SAMPLE == 16) // 16 bit default: appending original I2S chunks (e.g. 1014 values, 2048 bytes)
       {  audio_file.write((uint8_t*)audio_buffer, values_recorded * 2);   // for each value 2 bytes needed
       }        
       if (BITS_PER_SAMPLE == 8)  // 8bit mode: appending calculated 1014 values instead (1024 bytes, 2048/2) 
       {  audio_file.write((uint8_t*)audio_buffer_8bit, values_recorded);
       }  
       audio_file.close();                 
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

       Serial.println("> ... Done. Audio Recording into PSRAM finished.");
       Serial.println("> Bytes recorded: " + (String) *audiolength_bytes + ", audio length [sec]: " + (String) *audiolength_sec );

       delay(500);
       audio_play.stopSong(); // just to be sure

       // Optional for debugging: Writing the PSRAM content to a 2nd file "AudioPSRAM.wav", printing first chunks
       sdfoolproofTest();

       File control_file = SD.open("/recps.wav", FILE_WRITE);
       if (!control_file) {
        Serial.println("Something was very wrong in the creation of PSRAM aud!");
       }
       control_file.write( PSRAM_BUFFER, PSRAM_BUFFER_counter );
       control_file.close(); 

       Serial.println( "\n# DEBUG: PSRAM content mirrored on SD card [AudioPSRAM.wav]" );
       Serial.println(   "# DEBUG: PSRAM extract [220 bytes, 44 byte wav header in first 2 rows]:\n ");
       for (int i=0; i<220; i++) 
       { Serial.print( PSRAM_BUFFER[i], HEX); Serial.print( "\t"); 
         if ( (i+1)%22 == 0) {Serial.println();}
       } Serial.println();
    }    
    if (RECORD_SDCARD)
    {  
       File audio_file = SD.open(AUDIO_FILE, FILE_APPEND);   // Do NOT use 'FILE_WRITE' we need a 'r+' !
       
       if (!audio_file) {
         Serial.println("SD something went nuked Finalize");
       }

       long filesize = audio_file.size();
       /* bug fix: earlier version was wrrong: .flength = filesize; .dlength = (filesize-8) */
       audio_file.seek(0); myWAV_Header.flength = (filesize-8);  myWAV_Header.dlength = (filesize-44); 
       audio_file.write((uint8_t *) &myWAV_Header, 44);
       audio_file.close(); 

       // return updated values via REFERENCE (pointer):
       *audio_filename    = AUDIO_FILE;
       *audiolength_bytes = filesize;
       *audiolength_sec   = (float) (filesize-44) / (SAMPLE_RATE * BITS_PER_SAMPLE/8);   

       Serial.println("> ... Done. Audio Recording finished, stored as '" + (String) AUDIO_FILE + "' on SD Card.");
       Serial.println("> Bytes recorded: " + (String) *audiolength_bytes + ", audio length [sec]: " + (String) *audiolength_sec ); 
    }
    
    // Record is done (stored either in PSRAM or on SD card)
    flg_is_recording = false;  // important: this is done only here (any next Recording_Stop() calls have no action)
    return true;               // means: telling the main loop that new record is available now 
  }    
}