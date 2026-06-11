#pragma region OpenAIChatModels

#define RSTR_ASSISTANT "assistant"
#define RSTR_USER "user"
#define RSTR_DEV "developer"
#define RSTR_SYS "system"

#define TIMEOUT_LLM  10          // preferred max. waiting time [sec] for LMM AI response     

String OpenAI_Handle_UserRequest(String urequest) {
  String urequest_cap = urequest;
  urequest.toUpperCase();
  
  bool shouldUseWebSearch = false;

  if (urequest_cap.indexOf("GOOGLE") > -1) shouldUseWebSearch = true;  
  if (urequest_cap.indexOf("TÌM KIẾM") > -1) shouldUseWebSearch = true;  
  if (urequest_cap.indexOf("TRA CỨU") > -1) shouldUseWebSearch = true;  
  if (urequest_cap.indexOf("SEARCH") > -1) shouldUseWebSearch = true;

  LLM_Append(RSTR_USER, urequest);
  return OpenAI_Groq_LLM (OPENAI_KEY, shouldUseWebSearch, "");
}

// newft1: Xây dựng các hàm append
void LLM_Append(String role, String content) {
    content.replace( "\"", "\\\"" );  // to avoid any ERROR (if user enters any " -> convert to \")

    MESSAGES += ((MESSAGES == "") ? "{" : ",\n\n{");
    MESSAGES += "\"role\": \""; 
    MESSAGES += role;
    MESSAGES += "\", \"content\": \"";
    MESSAGES += content;
    MESSAGES += "\"}";

    Serial.print("LLM New prompt appended (role ");
    Serial.print(role);
    Serial.println(")");
    
    Serial.println( ">> MESSAGES LENGTH: " + (String) MESSAGES.length() );   
    Serial.println( ">> FREE HEAP: " + (String) ESP.getFreeHeap() );             
}

// Hàm này giờ chỉ có chức năng gửi, các vấn đề append prompt dùng qua hàm LLM_Append()
String OpenAI_Groq_LLM( const char* llm_open_key, bool flg_WebSearch, const char* llm_groq_key )
{   
    // =====- Prep work done. Now CONNECT to Open AI or Groq Server (on INIT or after closed or lost connection) ================

    uint32_t t_start = millis(); 

    String LLM_Response = "";                                         // used for complete API response
    String Feedback = "";                                             // used for extracted answer
    String LLM_server, LLM_entrypoint, LLM_model, LLM_key;            // NEW: using vars to be independet of server/models
    
    // Dùng LLM của OpenAI
    LLM_server =        "api.openai.com";                          // OpenAI: https://platform.openai.com/docs/pricing
    LLM_entrypoint =    "/v1/chat/completions";           
    if (!flg_WebSearch) LLM_model= "gpt-4.1-nano";                 // low cost, powerful, fast (response latency ~ 1.5 sec)  
    if (flg_WebSearch)  LLM_model= "gpt-4o-mini-search-preview";   // realtime websearch model (higher latency ~ 3-5 sec)     
    LLM_key =           llm_open_key;
    
    /*static*/ WiFiClientSecure client_tcp;    // [UPDATE]: removed static to free up HEAP (start with new LLM socket always)
    
    if ( !client_tcp.connected() )
    {  
       Serial.println("> Initialize LLM AI Server connection ... ");
       client_tcp.setInsecure();
       if (!client_tcp.connect( LLM_server.c_str() , 443)) 
       { Serial.println("\n* ERROR - WifiClientSecure connection to Server failed!");
         client_tcp.stop(); /* might not have any effect, similar with client.clear() */
         return ("");   // in rare cases: WiFiClientSecure freezed (with older libraries) 
       }
       Serial.println("Done. Connected to LLM AI Server.");
    }
    client_tcp.setNoDelay(true);     // NEW: immediately flush after each write(), means disable TCP nagle buffering.
                                     // Idea: might increase performance a bit (20-50ms per write) [default is false]
 
    
    // ------ Creating the Payload: ---------------------------------------------------------------------------------------------
    
    // == model CHAT: creating a user prompt in format:  >"messages": [MESSAGES], {"role":"user", "content":"what means AI?"}]<
    // recap: Syntax of entries in global var MESSAGES [e.g.100K]: 
    // > {"role": "system", "content": "you are a helpful assistant"},\n
    //   {"role": "user", "content": "how are you doing?"},\n
    //   {"role": "assistant", "content": "Thanks for asking, as an AI bot I do not have any feelings"} <
    //
    // for better readiability we write \" instead \" and replace below in code:

    String request_Prefix, request_Postfix, request_LEN;

    request_Prefix  =     "{\"model\":\"" + LLM_model + "\", \"messages\":[";    // appending old MESSAGES       
    
    if (!flg_WebSearch)   // DEFAULT parameter for classic CHAT completion models                                     
    {  request_Postfix =  "],\n\"temperature\":0.7, \"max_tokens\":512, \"presence_penalty\":0.6, \"top_p\":1.0}";                                           
    }
    if (flg_WebSearch)    // NEW: parameter for web search models
    {  request_Postfix =  "],\n\"response_format\": {\"type\": \"text\"}, "; 
       request_Postfix += "\"web_search_options\": {\"search_context_size\": \"low\", ";
       request_Postfix += "\"user_location\": {\"type\": \"approximate\", \"approximate\": ";
       request_Postfix += "{\"country\": \"\", \"city\": \"Việt Nam\"}}}, ";
       request_Postfix += "\"store\": false}";
    }  
          
    request_LEN = (String) (MESSAGES.length() + request_Prefix.length() + request_Postfix.length()); 

 
    // ------ Sending the request: ----------------------------------------------------------------------------------------------

    uint32_t t_startRequest = millis(); 
    
    client_tcp.println( "POST " + LLM_entrypoint + " HTTP/1.1" );   
    client_tcp.println( "Connection: close" ); 
    client_tcp.println( "Host: " + LLM_server );                   
    client_tcp.println( "Authorization: Bearer " + LLM_key );  
    client_tcp.println( "Content-Type: application/json; charset=utf-8" ); 
    client_tcp.println( "Content-Length: " + request_LEN ); 
    client_tcp.println(); 
    client_tcp.print( request_Prefix );    // detail: no 'ln' because Content + Postfix will follow)  
   
    // Now sending the complete MESSAGES chat history (String) .. 2 options (from own experiences in testing & user feedback): 
    // 1. either with one single 'client_tcp.print( MESSAGES );' .. works well on my ESP32 (even a 100 KB String works flawless)
    // 2. or sending in chunks (background: some user had issues if MESSAGES size exceeds 8K .. so we use option (2) here:  

    /* client_tcp.print( MESSAGES );       // Option 1: sending complete MESSAGES history once (works well on my ESP32)  */    
    
    // Option 2 (NEW): sending MESSAGES (text) in chunks (prevents the TLS layer from choking on big payloads)
    const size_t CHUNK_SIZE = 1024;        // 1K just as example (all below 8K should work also on older ESP32)        
    for (size_t i = 0; i < MESSAGES.length();  i += CHUNK_SIZE) 
    {   client_tcp.print(MESSAGES.substring(i, i +  CHUNK_SIZE)); 
    }

    // final Postfix (then all is done):
    client_tcp.println( request_Postfix );    
                 
    
    // ------ Waiting the server response: --------------------------------------------------------------------------------------

    LLM_Response = "";
    while ( millis() < (t_startRequest + (TIMEOUT_LLM*1000)) && LLM_Response == "" )  
    { Serial.print(".");                   // printed in Serial Monitor always    
      delay(250);                          // waiting until tcp sends data 
      while (client_tcp.available())       // available means: if a char received then read char and add to String
      { char c = client_tcp.read();
        LLM_Response += String(c);
      }       
    } 
    if ( millis() >= t_startRequest + (TIMEOUT_LLM*1000) ) 
    {  Serial.print("\n*** LLM AI TIMEOUT ERROR - forced TIMEOUT after " + (String) TIMEOUT_LLM + " seconds");      
    } 
    client_tcp.stop();                     // closing LLM connection always (observation: otherwise OpenAI TTS won't work)
        
    uint32_t t_response = millis();  

  
    // ------ Now extracting clean message for return value 'Feedback': --------------------------------------------------------- 
    // 'talkative code below' but want to make sure that also complex cases (e.g. " chars inside the response are working well)
    
    int pos_start, pos_end, pos_mem;                            // proper way to extract tag "text", talkative but correct
    bool found = false;                                         // supports also complex tags, e.g.  > "What means \"RGB\"?" < 
    
    pos_start = LLM_Response.indexOf("\"content\":");           // search tag ["content": "Answer..."]   
    if (pos_start > 0)                                         
    { pos_start = LLM_Response.indexOf("\"", pos_start + strlen("\"content\":")) + 1;   // search next " -> now points to 'A'
      pos_end = pos_start + 1;

      while (!found)                                        
      { found = true;                                           // avoid endless loop in case no " found (won't happen anyhow)  
        pos_end = LLM_Response.indexOf("\"", pos_end);          // search the final " ... but ignore any rare \" inside the text!  
        if (pos_end > 0)                                        // " found -> Done.   but:  
        {  // in case we find a \ before the " then proceed with next search (because it was a \marked " inside the text)    
           if (LLM_Response.substring(pos_end -1, pos_end) == "\\") { found = false; pos_end++; }
        }           
      }            

      // 06/01: tách MEM
      // pos_mem = LLM_Response.indexOf("[MEM]", pos_start);
      // if (pos_mem == -1) pos_mem = SYSINT_PINF; // đặt thành số cực lớn để tránh lỗi tách bậy
      pos_mem = SYSINT_PINF;
    }                           
    if( pos_start > 0 && (pos_end > pos_start) )
    { 
      // 06/01: lấy pos_mem nếu có, còn không thì mặc định pos_end
      Feedback = LLM_Response.substring(pos_start, pos_mem < pos_end ? pos_mem : pos_end);  // store cleaned response into String 'Feedback'   

      Feedback.trim();     
    }

    
    // ------ APPEND current I/O chat (UserRequest & Feedback) at end of var MESSAGES -------------------------------------------
    // Chèn vào lịch sử MESSAGES

    if (Feedback != "")                                          // we always add both after success (never if error) 
    { 
      // String NewMessagePair = ",\n\n";                           // NEW in Sept. 2025: \n\n instead \n (for spaces in email)  
      // if(MESSAGES == "") { NewMessagePair = ""; }                // if messages empty we remove leading ,\n  
      // NewMessagePair += "{\"role\": \"user\", \"content\": \""      + UserRequest + "\"},\n"; 
      // NewMessagePair += "{\"role\": \"assistant\", \"content\": \"" + Feedback    + "\"}"; 
      
      // newft1: thay bằng hàm LLM_Append
      LLM_Append(RSTR_ASSISTANT, Feedback); // phản hồi chiều lại luôn là assistant

      // here we construct the CHAT history, APPENDING current dialog to LARGE String MESSAGES
      // MESSAGES += NewMessagePair;       
    }  
             
    // ------ finally we clean Feedback, print DEBUG Latency info and return 'Feedback' String ----------------------------------
    
    // trick 17: here we break \n into real line breaks (but in MESSAGES history we added the original 1-liner)
    if (Feedback != "")                                              
    {  Feedback.replace("\\n", "\n");                            // LF issue: replace any 2 chars [\][n] into real 1 [\nl]  
       Feedback.replace("\\\"", "\"");                           // " issue:  replace any 2 chars [\]["] into real 1 char ["]
       Feedback.replace("\n\n", "\n");                           // NEW: remove empty lines in Serial Monitor
       Feedback.trim();                                          // in case of some leading spaces         
    }

    Serial.println( "\n---------------------------------------------------" );
    //DebugPrintln( "====> Total Response: \n" + LLM_Response + "\n====");   // ## uncomment to see complete server response */  
    Serial.println( "AI LLM server/model: [" + LLM_server + " / " + LLM_model + "]" );
    Serial.println( "-> Latency LLM AI Server (Re)CONNECT:          " + (String) ((float)((t_startRequest-t_start))/1000) );   
    Serial.println( "-> Latency LLM AI Response:                    " + (String) ((float)((t_response-t_startRequest))/1000) );   
    Serial.println( "=> TOTAL Duration [sec]: ..................... " + (String) ((float)((t_response-t_start))/1000) ); 
    Serial.println( "---------------------------------------------------" );   
    Serial.println( "\nLLM >" ); 
    
    // 06/01: update TRÍ NHỚ NGẮN HẠN
    // sysUpdateShortTermMem(pos_mem == SYSINT_PINF ? "Mình rất háo hức muốn gặp người bạn của mình." : LLM_Response.substring(pos_mem, pos_end));

    Serial.println(Feedback);
    return ( Feedback );                           
} 

void OpenAI_Chat_Init() {
  LLM_Append(RSTR_SYS, AI_PROMPT);
  LLM_Append(RSTR_DEV, prmBuildInfo());
}

// ROLE: DEVELOPER
// GLOBAL PARAMS: #[CARE_PARAMS]
// Gửi tự động, yêu cầu LLM trả lời cảm ơn/nhắc nhở người dùng chăm sóc
String prmCare() {
  String prompt = "";
  prompt += "[REPORT] Dưới đây là dữ liệu chăm sóc: Nhiệt độ là " + String(ssTemperature, 1) + " độ C. ";
  prompt += "Độ ẩm là " + String(ssHumidity, 1) + " %. ";
  prompt += "Ánh sáng là " + String(ssLightAo, 0) + " lux. "; // Ánh sáng thường là số nguyên lớn nên để 0 chữ số thập phân
  prompt += String((float)sunMillis / 3600000.0f, 4) + " = số giờ nắng đã nhận hôm nay, ";
  prompt += String(sunTargetHours) + " = mục tiêu giờ nắng, ";
  prompt += String(((float)(millis() - soilLastWaterTime) / 86400000.0f), 4) + " = lần tưới gần nhất (ngày), ";
  prompt += String(soilWaterTargetDays) + " = chu kỳ tưới (ngày/lần). ";
  prompt += "Thời gian hiện giờ là " + sysGetDateTimeString() + ". ";
  prompt += "[SYS] Hãy phản hồi ngắn gọn, thân thiện và khích lệ: nếu người dùng đã chăm sóc đúng thì cảm ơn họ vì đã dành thời gian, ";
  prompt += "nếu chưa thì nhắc nhở nhẹ nhàng để hoàn thành việc tưới hoặc phơi nắng theo khuyến nghị.";

  return prompt;
}

// ROLE: DEVELOPER
// GLOBAL PARAMS: #[YOU], #[THRESH], PLANT_NAME, #[CARE_PARAMS]
// Dùng khi khởi động, sau system prompt
String prmBuildInfo() {
  String prompt = "";
  prompt += "Dưới đây là thông tin: tên của người chăm sóc bạn là " + YOU_NAME;
  prompt += ". Bạn là loài cây " + PLANT_NAME;
  prompt += ", nếu không cung cấp thì hãy xem như một cây cảnh bình thường. ";
  prompt += "Developer nhận thấy đây là những chỉ số môi trường lý tưởng cho bạn: ";
  prompt += "nhiệt độ (độ C) từ " + String(THRESH_TEMP_MIN, 1) + " đến " + String(THRESH_TEMP_MAX, 1);
  prompt += ", độ ẩm RH% từ " + String(THRESH_HUMID_MIN, 0) + " đến " + String(THRESH_HUMID_MAX, 0);
  prompt += ", ánh sáng (lux) từ " + String(THRESH_LIGHT_MIN, 0) + " đến " + String(THRESH_LIGHT_MAX, 0);
  prompt += ", bạn cần được tưới nước " + String(soilWaterTargetDays) + " ngày/lần, ";
  prompt += "và bạn cần được phơi nắng " + String(sunTargetHours) + " giờ mỗi ngày. ";

  return prompt;
}

#pragma endregion OpenAIChatModels
