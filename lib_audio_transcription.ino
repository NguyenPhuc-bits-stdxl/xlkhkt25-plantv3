// --- user preferences --------  
#define TIMEOUT_STT         8     // max. waiting time [sec] for STT Server transcription response (after wav sent), e.g. 8 sec. 
                                  // typical response latency is much faster. TIMEOUT is primary used if server connection lost
                                  
String SpeechToText_ElevenLabs( String audio_filename, uint8_t* PSRAM, long PSRAM_length, String language, const char* API_Key )
{ 
  static WiFiClientSecure client;                                        
  uint32_t t_start = millis(); 
  
  
  // ---------- Connect to ElevenLabs Server (only if needed, e.g. on INIT or after .close() and after lost connection)

  const char* STT_ENDPOINT = "api.elevenlabs.io";
  
  if ( !client.connected() )
  { DebugPrintln("> Initialize ELEVENLABS Server connection ... ");
    client.setInsecure();
    if (!client.connect(STT_ENDPOINT, 443)) 
    { Serial.println("\n* ERROR - WifiClientSecure connection to Server failed!");
      client.stop(); /* might not have any effect, similar with client.clear() */
      return ("");   // in rare cases: WiFiClientSecure freezed (with older libraries) 
    }
    DebugPrintln("Done. Connected to ELEVENLABS Server.");
  }
  client.setNoDelay(true);            // NEW: immediately flush after each write(), means: disable TCP nagle buffering.
                                      // Idea: might increase performance a bit (20-50ms per write) [default is false]
                                  
  uint32_t t_connected = millis();  
        
  size_t audio_size;
  if ( PSRAM != NULL && PSRAM_length > 0)                
  {  audio_size = PSRAM_length; 
  }
  else
  {  
     DebugPrintln( "> Audio File: SD card not supported"); 
  }

  if (audio_size == 0)   // error handling: if nothing found at all .. then return empty string. DONE.
  {  DebugPrintln( "* ERROR - No AUDIO data for transcription found!");
     return ("");
  }

  
  // ---------- PAYLOAD - HEADER: Building multipart/form-data HEADER and sending via HTTPS request to ElevenLabs Server
  
  // API Reference (Create Transcript): https://elevenlabs.io/docs/api-reference/speech-to-text/convert
  // [model] (mandatory):            >> using model 'scribe_v1'
  // [tag_audio_events] (optional):  if true (default) adding audio events into transcript, e.g. (laughter), (footsteps)
  // [language_code] (optional):     ISO-639-1/ISO-639-3 language_code. Recommended to keep empty in Elevnlabs (see above)
  
  String payload_header;
  String payload_end;
  String boundary =  "---011000010111000001101001";
  String bond     =  "--" + boundary + "\r\n" + "Content-Disposition: form-data; ";  
   
  payload_header  =  bond + "name=\"model_id\"\r\n\r\n" + "scribe_v1\r\n";                             
  payload_header  += bond + "name=\"tag_audio_events\"\r\n\r\n" + "false\r\n";                         
  payload_header  += (language != "")? (bond + "name=\"language_code\"\r\n\r\n" + language + "\r\n") : ("");  
  payload_header  += bond + "name=\"file\"; filename=\"audio.wav\"\r\n" + "Content-Type: audio/wav\r\n\r\n"; 
  /* <= ... Hint: Binary audio data will be send here (between payload_header and payload_end) */
  payload_end     =  "\r\n--" + boundary + "--\r\n";
  
  /* DebugPrintln( "\n> PAYLOAD (multipart/form-data:\n" + payload_header + "... AUDIO DATA ..." + payload_end ); */
  
  size_t total_length = payload_header.length() + audio_size + payload_end.length();    // total PAYLOAD length
  
  // ---------- Sending  HTTPS POST request header ---
  client.println("POST /v1/speech-to-text HTTP/1.1");
  client.println("Host: " + (String) STT_ENDPOINT);
  client.println("xi-api-key: " + String (API_Key));
  client.println("Content-Type: multipart/form-data; boundary=" + boundary);
  client.println("Content-Length: " + String(total_length));
  client.println();
  
  // ---------- Sending PAYLOAD
  client.print(payload_header);


  // ---------- PAYLOAD - DATA: Sending binary AUDIO data (from SD Card or PSRAM) to ElevenLabs Server
  
  DebugPrintln("> POST Request to ELEVENLABS Server started, sending WAV data now ..." );
  
  // ---------- Option 1: Sending WAV from PSRAM buffer
  
  if (PSRAM != NULL && PSRAM_length > 0)
  {
     // observation: client.write( PSRAM, PSRAM_length ); .. unfortunately NOT working -> smaller CHUNKS needed !
     // Writing binary data above 16383 bytes in 1 block will be rejected (maybe a handshake WiFiClientSecure.h issue ?)
     // workaraound: sending WAV in chunks
     
     size_t blocksize   = 16383;     // found by try&error, 16383 (14 bit max value) seems a hard coded max. limit
     size_t chunk_start = 0;
     size_t result_bytes_sent;
  
     while ( chunk_start < PSRAM_length ) 
     { if ( (chunk_start + blocksize) > PSRAM_length )   
       {  blocksize = PSRAM_length - chunk_start;  // at eof buffer (rest)
       }
       result_bytes_sent = client.write( PSRAM + chunk_start, blocksize );  // sending in chunks
       DebugPrintln( "  Send Bytes: " + (String) blocksize + ", Confirmed: " + (String) result_bytes_sent );  
       chunk_start += blocksize;
     }
     DebugPrintln( "> All WAV data in PSRAM sent [" + (String) PSRAM_length + " bytes], waiting transcription" );    
  }
 
  // ---------- Option 2: Sending WAV from SD CARD (only if Option 1 not launched)
  
  if ( audio_filename != "" && !(PSRAM != NULL && PSRAM_length > 0) )
  {  
     DebugPrintln( "> Sending bytes failed: SD Card functionality removed");    
  }    
  
  // ---------- PAYLOAD - END:  Sending final boundery to complete multipart/form-data
  client.print( payload_end );  

  uint32_t t_wavbodysent = millis();  
  

  // ---------- Waiting (!) to ElevenLabs Server response (stop waiting latest after TIMEOUT_STT [secs])
 
  String response = "";   // waiting until available() true and all data completely received
  while ( response == "" && millis() < (t_wavbodysent + TIMEOUT_STT*1000) )   
  { while (client.available())                         
    { char c = client.read();
      response += String(c);      
    }
    response.trim();  // in rare cases server returns ' ' and  pauses (so we ignore, waiting next char)
    // printing dots '.' e.g. each 250ms while waiting (always, also if DEBUG false)
    Serial.print(".");                   
    delay(250);                                 
  } 
  Serial.print(" ");  // final space at end (for better readability)
  
  if (millis() >= (t_wavbodysent + TIMEOUT_STT*1000))
  { Serial.print("\n*** TIMEOUT ERROR - forced TIMEOUT after " + (String) TIMEOUT_STT + " seconds");
    Serial.println(" (is your ElevenLabs API Key valid ?) ***\n");    
  } 
  uint32_t t_response = millis();  


  // ---------- closing connection to ElevenLabs 
  // ElevenLabs supports a 'nice' feature: websockets will NOT be closed automatically (at least for several minutes),
  // - allows keeping connection OPEN for next requests. Result: increased performance, no connect needed (~0.5 sec faster)
  // - side effect: other WiFiClientSecure sockets might be less reliable (e.g. on Open AI TTS or Open AI LLM) without PSRAM
  // - we use this performance trick only in case PSRAM is available, Tipp for best reliability: remove the 'if' and close always
    
  if ( ESP.getPsramSize() == 0 ) { client.stop(); } 
                     
    
  // ---------- Parsing json response, extracting transcription etc.
  
  /* Comment to return value 'detected languages' in older Nova-2 model:
  MULTI lingual (not used here): tag is labeled with 'detected_language' (instead 'languages')
  MONO  lingual mode: no language tag at all */
  
  int    response_len     = response.length();
  
  String transcription    = json_object( response, "\"text\":" );
  String detect_language  = json_object( response, "\"language_code\":" );    
  /* String wavduration   = json_object( response, "\"duration\":" ); */

  DebugPrintln( "\n---------------------------------------------------" );
  /* DebugPrintln( "-> Total Response: \n" + response + "\n");  // ## uncomment to see the complete server response ##   */
  DebugPrintln( "ELEVENLABS: [scribe_v1] - " + ((language == "")? ("MULTI lingual") : ("MONO lingual [" + language +"]")) )
  DebugPrintln( "-> Latency Server (Re)CONNECT [t_connected]:   " + (String) ((float)((t_connected-t_start))/1000) );   
  DebugPrintln( "-> Latency sending WAV file [t_wavbodysent]:   " + (String) ((float)((t_wavbodysent-t_connected))/1000) );   
  DebugPrintln( "-> Latency ELEVENLABS response [t_response]:   " + (String) ((float)((t_response-t_wavbodysent))/1000) );   
  DebugPrintln( "=> TOTAL Duration [sec]: ..................... " + (String) ((float)((t_response-t_start))/1000) ); 
  /*DebugPrintln( "=> Server detected audio length [sec]: " + wavduration ); // json object does not exist in ElevenLabs */
  DebugPrintln( "=> Server response length [bytes]: " + (String) response_len );
  DebugPrintln( "=> Detected language (optional): [" + detect_language + "]" );  
  DebugPrintln( "=> Transcription: [" + transcription + "]" );
  DebugPrintln( "---------------------------------------------------\n" );

  
  // ---------- return transcription String 
  return transcription;    
}

String json_object( String input, String element )
{ String content = "";
  int pos_start = input.indexOf(element);      
  if (pos_start > 0)                                      // if element found:
  {  pos_start += element.length();                       // pos_start points now to begin of element content     
     int pos_end = input.indexOf( ",\"", pos_start);      // pos_end points to ," (start of next element)  
     if (pos_end > pos_start)                             // memo: "garden".substring(from3,to4) is 1 char "d" ..
     { content = input.substring(pos_start,pos_end);      // .. thats why we use for 'to' the pos_end (on ," ):
     } content.trim();                                    // remove optional spaces between the json objects
     if (content.startsWith("\""))                        // String objects typically start & end with quotation marks "    
     { content=content.substring(1,content.length()-1);   // remove both existing quotation marks (if exist)
     }     
  }  
  return (content);
}
