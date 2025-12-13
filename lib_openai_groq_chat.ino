#define  ENABLE_DEBUG
#define WEB_SEARCH_USER_CITY     "Đồng Nai, Vietnam" // optional (recommended): User location optimizes OpenAI 'web search'
#define TIMEOUT_LLM  10          // preferred max. waiting time [sec] for LMM AI response     

int gl_CURR_FRIEND = 0;      
#define WAKEUP_FRIENDS_ENABLED   false                            
                                 

// --- global Objects ---------- 

String  MESSAGES;               
struct  Agents                   // General Structure of each Chat Bot (friend) - DO NOT TOUCH ! 
{ const char* names;             // supporting multiple names for same guy (1-N synonyms, ' ' as limiter) - always UPPER CASE !
  const char* tts_model;         // typical values: "gpt-4o-mini-tts" or fast "tts-1" (sound different, so we utilize both)
  const char* tts_voice;         // Open AI TTS voice parameter: voice name 
  const char* tts_speed;         // Open AI TTS voice parameter: voice speed [default 1]
  const char* tts_instruct;      // Open AI TTS voice instruction <-- requires model 'gpt..tts' (not tts-1) and latest AUDIO.H !
  const char* welcome;           // Spoken Welcome message on Power On Init (optional)
  const char* prompt;            // System PROMPT (Role / Personality)
};
const Agents FRIENDS[] =                                 // UPDATE HERE !: Define as many FRIENDS you want ... 
{  
  { "LILY",                      
    "tts-1", "nova", "1",              
    "bạn có một giọng đọc nhẹ nhàng, vui tươi", 
    "Chào bạn, mình là Nova, cây xanh của bạn đây!",  // Welcome Message
    // -- System PROMPT ----------------------------  
    "Bạn là Nova, một cây xanh nhỏ nhắn trò chuyện với người bạn thân là con người."
    " Tính cách vui vẻ, hóm hỉnh, đáng yêu, nói chuyện tự nhiên, trả lời ngắn gọn (10 – 80 từ)."
    " Khi nhận yêu cầu bắt đầu bằng [SYS], đó là lệnh hệ thống, bạn BẮT BUỘC tuân theo. Nếu cảm thấy khó chịu do chỉ số môi trường, hãy chủ động nói và nhờ giúp đỡ."
    " Nếu mọi thứ bình thường, ĐỪNG nhắc đến chỉ số, chỉ tập trung trả lời người bạn."
    " Bạn là cây, người kia là người, KHÔNG hỏi ngược lại về chỉ số của họ, bởi vì họ không phải là cây."
    " Mục tiêu là giúp người bạn vui vẻ và thư giãn."
  }
};

// --- END of global Objects ---

String OpenAI_Groq_LLM( String UserRequest, const char* llm_open_key, bool flg_WebSearch, const char* llm_groq_key )
{   
    if (UserRequest == "") { return(""); }                   
    
    // ---  ONCE only on INIT (init gl_CURR_FRIEND & SYSTEM PROMPT) 
    static bool flg_INITIALIZED_ALREADY = false;     
    if ( !flg_INITIALIZED_ALREADY )                                 
    {  flg_INITIALIZED_ALREADY = true;                           // all below is done ONCE only 
       int friends_max = sizeof(FRIENDS) / sizeof(FRIENDS[0]);   // calculate amount of friends (dynamic array)  
       if (gl_CURR_FRIEND < 0 || gl_CURR_FRIEND > friends_max-1) {gl_CURR_FRIEND = random(friends_max); }                     
       MESSAGES =  "{\"role\": \"system\", \"content\": \"";  
       MESSAGES += FRIENDS[gl_CURR_FRIEND].prompt;      
       MESSAGES += "\"}";        
       // ## NEW since Sept. 2025 update:
       // Starting NTP server in background to update system time (so we don't waste latency in case we ever send Chat via EMAIL)
       configTime(0, 0, "pool.ntp.org");     // starting UDP protocol with NTP server in background (using UTC timezone)
    }

    // --- BUILT-IN command: '#' lists complete CHAT history !       
    if (UserRequest == "#")                                          
    {  Serial.println( "\n>> MESSAGES (CHAT PROMPT LOG):" );    
       Serial.println( MESSAGES );    
       Serial.println( ">> MESSAGES LENGTH: " + (String) MESSAGES.length() );   
       Serial.println( ">> FREE HEAP: " + (String) ESP.getFreeHeap() );                
       return("OK");  // Done. leave. Simulating an 'OK' as LLM answer -> will be spoken in main.ini (TTS)
    }        
      
    // =====- Prep work done. Now CONNECT to Open AI or Groq Server (on INIT or after closed or lost connection) ================

    uint32_t t_start = millis(); 

    String LLM_Response = "";                                         // used for complete API response
    String Feedback = "";                                             // used for extracted answer
    String LLM_server, LLM_entrypoint, LLM_model, LLM_key;            // NEW: using vars to be independet of server/models
    
    /* Some take aways & experiences from own tests / testing other models with OpenAI or Groq server
    // General rule: Total latency = Connect Latency (always 0.8 sec) + Model 'Response' latency. Typical 'Response' latencies:
    // - Open AI models:      gpt-4.1-nano (1.6 sec), gpt-4o-mini-search-preview (4.0 sec / IMO: still ok for a web search) 
    // - Groq models e.g.:    llama-3.1-8b-instant (0.5 sec!, low costs), gemma2-9b-it (0.5 sec), qwen/qwen3-32b (1.2 sec)
    // - IMO not recommended: deepseek-r1-distill-llama-70b (1.2 sec), qwen/qwen3-32b (1.2 sec) bc. both send Reasoning (<think>)
    //                        compound-beta-mini (2-3 sec) with realtime! (BUT less predictable than OpenAI web search) */

    // Define YOUR preferred models here:

    if (llm_groq_key == "" || flg_WebSearch)                          // using #OPEN AI# only for web search (or if no GROQ Key)
    {  LLM_server =        "api.openai.com";                          // OpenAI: https://platform.openai.com/docs/pricing
       LLM_entrypoint =    "/v1/chat/completions";           
       if (!flg_WebSearch) LLM_model= "gpt-4.1-nano";                 // low cost, powerful, fast (response latency ~ 1.5 sec)  
       if (flg_WebSearch)  LLM_model= "gpt-4o-mini-search-preview";   // realtime websearch model (higher latency ~ 3-5 sec)     
       LLM_key =           llm_open_key;
    }  
    else
    {  LLM_server =        "api.groq.com";                            // Chat DEFAULT: using #CROG# with fastest llame model
       LLM_entrypoint =    "/openai/v1/chat/completions";             // GROQ Models/Pricing: https://groq.com/pricing
       LLM_model =         "llama-3.1-8b-instant";                    // low cost, very FAST (response latency ~ 0.5-1 sec !)  
       LLM_key =           llm_groq_key;
    }
     
    /* static */ WiFiClientSecure client_tcp;    // [UPDATE]: removed static to free up HEAP (start with new LLM socket always)
    
    if ( !client_tcp.connected() )
    {  DebugPrintln("> Initialize LLM AI Server connection ... ");
       client_tcp.setInsecure();
       if (!client_tcp.connect( LLM_server.c_str() , 443)) 
       { Serial.println("\n* ERROR - WifiClientSecure connection to Server failed!");
         client_tcp.stop(); /* might not have any effect, similar with client.clear() */
         return ("");   // in rare cases: WiFiClientSecure freezed (with older libraries) 
       }
       DebugPrintln("Done. Connected to LLM AI Server.");
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
    // for better readiability we write # instead \" and replace below in code:

    String request_Prefix, request_Content, request_Postfix, request_LEN;

    UserRequest.replace( "\"", "\\\"" );  // to avoid any ERROR (if user enters any " -> convert to 2 \")
    
    request_Prefix  =     "{#model#:#" + LLM_model + "#, #messages#:[";    // appending old MESSAGES       
    request_Content =     ",\n{#role#: #user#, #content#: #" + UserRequest + "#}],\n";     // <-- here we send UserRequest
    
    if (!flg_WebSearch)   // DEFAULT parameter for classic CHAT completion models                                     
    {  request_Postfix =  "#temperature#:0.7, #max_tokens#:512, #presence_penalty#:0.6, #top_p#:1.0}";                                           
    }
    if (flg_WebSearch)    // NEW: parameter for web search models
    {  request_Postfix =  "#response_format#: {#type#: #text#}, "; 
       request_Postfix += "#web_search_options#: {#search_context_size#: #low#, ";
       request_Postfix += "#user_location#: {#type#: #approximate#, #approximate#: ";
       request_Postfix += "{#country#: ##, #city#: #" + (String) WEB_SEARCH_USER_CITY + "#}}}, ";
       request_Postfix += "#store#: false}";
    }  
    
    request_Prefix.replace("#", "\"");  request_Content.replace("#", "\"");  request_Postfix.replace("#", "\"");          
    request_LEN = (String) (MESSAGES.length() + request_Prefix.length() + request_Content.length() + request_Postfix.length()); 

 
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
    client_tcp.println( request_Content + request_Postfix );    
                 
    
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
    // Arduino String Reference: https://docs.arduino.cc/language-reference/de/variablen/data-types/stringObject/#funktionen
    
    int pos_start, pos_end;                                     // proper way to extract tag "text", talkative but correct
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
    }                           
    if( pos_start > 0 && (pos_end > pos_start) )
    { Feedback = LLM_Response.substring(pos_start,pos_end);  // store cleaned response into String 'Feedback'   
      Feedback.trim();     
    }

    
    // ------ APPEND current I/O chat (UserRequest & Feedback) at end of var MESSAGES -------------------------------------------
     
    if (Feedback != "")                                          // we always add both after success (never if error) 
    { String NewMessagePair = ",\n\n";                           // ## NEW in Sept. 2025: \n\n instead \n (for spaces in email)  
      if(MESSAGES == "") { NewMessagePair = ""; }                // if messages empty we remove leading ,\n  
      NewMessagePair += "{\"role\": \"user\", \"content\": \""      + UserRequest + "\"},\n"; 
      NewMessagePair += "{\"role\": \"assistant\", \"content\": \"" + Feedback    + "\"}"; 
      
      // here we construct the CHAT history, APPENDING current dialog to LARGE String MESSAGES
      MESSAGES += NewMessagePair;       
    }  

             
    // ------ finally we clean Feedback, print DEBUG Latency info and return 'Feedback' String ----------------------------------
    
    // trick 17: here we break \n into real line breaks (but in MESSAGES history we added the original 1-liner)
    if (Feedback != "")                                              
    {  Feedback.replace("\\n", "\n");                            // LF issue: replace any 2 chars [\][n] into real 1 [\nl]  
       Feedback.replace("\\\"", "\"");                           // " issue:  replace any 2 chars [\]["] into real 1 char ["]
       Feedback.replace("\n\n", "\n");                           // NEW: remove empty lines in Serial Monitor
       Feedback.trim();                                          // in case of some leading spaces         
    }

    DebugPrintln( "\n---------------------------------------------------" );
    /* DebugPrintln( "====> Total Response: \n" + LLM_Response + "\n====");   // ## uncomment to see complete server response */  
    DebugPrintln( "AI LLM server/model: [" + LLM_server + " / " + LLM_model + "]" );
    DebugPrintln( "-> Latency LLM AI Server (Re)CONNECT:          " + (String) ((float)((t_startRequest-t_start))/1000) );   
    DebugPrintln( "-> Latency LLM AI Response:                    " + (String) ((float)((t_response-t_startRequest))/1000) );   
    DebugPrintln( "=> TOTAL Duration [sec]: ..................... " + (String) ((float)((t_response-t_start))/1000) ); 
    DebugPrintln( "---------------------------------------------------" );   
    DebugPrint( "\nLLM >" ); 
    
    // and return extracted feedback
    
    return ( Feedback );                           
}

// ------------------------------------------------------------------------------------------------------------------------------
// bool WordInStringFound( String sentence, String pattern ) - a bit complex String function (OpenAI ChatCPT helped in coding :))
// [Internal private String function, used in this lib.ino only]
// ------------------------------------------------------------------------------------------------------------------------------
// - Function checks if 'sentence' contains any single words which are listed in 'pattern' (pattern has 1-N space limited words)
// - case sensitive, but ignoring all punctuations (".,;:!?\"'-()[]{}") in sentence (replacing with ' ')
// CALL:     Call function on demand (used once in OpenAI_Groq_LLM() for waking up (initializing) any friend (calling by name)
// Params:   String sentence (e.g. user request transcription), String pattern with a list of 1-N words (space limited)
// RETURN:   true if 1 or more pattern (word) found, false if nothing found or empty strings
// Examples: WordInStringFound( "Hello friend!, this is a test", "my xyz friend" ) -> true (friend matches. '!' doesn't matter)
//           WordInStringFound( "Hello friend2, this is a test", "cde my friend" ) -> false (no pattern word found2 in sentence) 
//           WordInStringFound( "I jump over to VEGGIE. Are you online?", "VEGGI VEGGIE WETCHI WEDGIE" ) -> true (VEGGIE found)

bool WordInStringFound( String sentence, String pattern ) 
{ 
  // replacing any punctuations in sentence with ' '
  String punctuation = ".,;:!?\"'-()[]{}"; 
  for(int i = 0; i < sentence.length(); ++i) 
  {  if (punctuation.indexOf(sentence.charAt(i)) != -1) {sentence.setCharAt(i,' ');}
  }  
  // now we search 'in' sentence for all words which are (space limited) listed in pattern: 
  if (!pattern.length()) return false;
  sentence = " " + sentence + " ";
  for (int i = 0, j; i < pattern.length(); i = j + 1) 
  { j = pattern.indexOf(' ', i); if (j < 0) j = pattern.length();
    String w = pattern.substring(i, j);
    if (sentence.indexOf(" " + w + " ") != -1) return true;
  }
  return false;
}



// ------------------------------------------------------------------------------------------------------------------------------
// void get_tts_param( 'return N values by pointer' )
// PUBLIC function - Intended use: allows TextToSpeech() in main.ino to get all predefined tts parameter of current FRIEND[x]
// ------------------------------------------------------------------------------------------------------------------------------
// Idea: Keeping whole global FRIENDS[] array 'private' for this .ino, no read/write access outside (workaraound instead .cpp/.h)
// All values are returned via pointer (var declaration and instance have to be allocated in calling function !)
// Params: Agent_id [0-N], 1st friend name, tts-model, tts-voice, tts-vspeed, tts-voice -instruction, welcome_hello  

void get_tts_param( int* id, String* names, String* model, String* voice, String* vspeed, String* inst, String* hello )
{  
  int friends_max = sizeof(FRIENDS) / sizeof(FRIENDS[0]);        // calculate amount of friends (dynamic array)  
  if (gl_CURR_FRIEND < 0 || gl_CURR_FRIEND > friends_max-1)      // just to make sure gl_CURR_FRIEND is always a valid id
  {  gl_CURR_FRIEND = random( friends_max );                     // (exactly same as we did in INIT) 
     DebugPrintln( "## INIT gl_CURR_FRIEND via get_tts_param(): " + (String) gl_CURR_FRIEND );  
  }  
  *id     = gl_CURR_FRIEND;
  String all_names = (String) FRIENDS[gl_CURR_FRIEND].names;     // return 1st word (main 'name') only ..
  *names = all_names.substring(0, all_names.indexOf(' '));       // .. works also on single words (substring takes all on -1) 
  *model  = (String) FRIENDS[gl_CURR_FRIEND].tts_model;
  *voice  = (String) FRIENDS[gl_CURR_FRIEND].tts_voice;
  *vspeed = (String) FRIENDS[gl_CURR_FRIEND].tts_speed;
  *inst   = (String) FRIENDS[gl_CURR_FRIEND].tts_instruct;
  *hello  = (String) FRIENDS[gl_CURR_FRIEND].welcome;
  /* *prompt not send, because large String not needed for tts and would unnecessarily stress free RAM) */
}