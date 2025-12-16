/**
 * The example to send simple text message.
 * For proper network/SSL client and port selection, please see http://bit.ly/46Xu9Yk
 */
#include <Arduino.h>
#include "Networks.h"

#define ENABLE_SMTP  // Allows SMTP class and data
#define ENABLE_DEBUG // Allows debugging
#define READYMAIL_DEBUG_PORT Serial

// If message timestamp and/or Date header was not set,
// the message timestamp will be taken from this source, otherwise
// the default timestamp will be used.
#if defined(ESP32) || defined(ESP8266)
#define READYMAIL_TIME_SOURCE time(nullptr); // Or using WiFi.getTime() in WiFiNINA and WiFi101 firmwares.
#endif

#include <ReadyMail.h>

#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465 // SSL or 587 for STARTTLS

// Vì lý do an toàn, thông tin này sẽ được gửi riêng, check Zalo, không xem GitHub
String AUTHOR_EMAIL = "";
String AUTHOR_PASSWORD = "";
String RECIPIENT_EMAIL = "";

// Cũng tự điền trong private_credentials.ino
String WIFI_SSID = "";
String WIFI_PASSWORD = "";

#define SSL_MODE true
#define AUTHENTICATION true
#define NOTIFY "SUCCESS,FAILURE,DELAY" // Delivery Status Notification (if SMTP server supports this DSN extension)
#define PRIORITY "Normal"                // High, Normal, Low
#define PRIORITY_NUM "1"               // 1 = high, 3, 5 = low
#define EMBED_MESSAGE false            // To send the html or text content as attachment

WiFiClientSecure ssl_client;
SMTPClient smtp(ssl_client);

void lbgetCreds(); // prototype

// Để này ra global vì sẽ dùng full scope trong code
// Mình gửi email theo multipart (html và richtext)
// Code này tuy là full-HTML-oriented nhưng mà ông nên viết thêm vài câu bằng text cho fallback nha
// Vd. email html nó chi tiết thì text thuần nó chỉ cần là cây xanh của bạn đang... thông tin...
SMTPMessage msg;
String bodyText;
String bodyHtml;

// For more information, see http://bit.ly/474niML
void smtpCb(SMTPStatus status)
{
    if (status.progress.available)
        ReadyMail.printf("ReadyMail[smtp][%d] Uploading file %s, %d %% completed\n", status.state,
                         status.progress.filename.c_str(), status.progress.value);
    else
        ReadyMail.printf("ReadyMail[smtp][%d]%s\n", status.state, status.text.c_str());
}

void setup()
{
    Serial.begin(115200);
    Serial.println();

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    // If server SSL certificate verification was ignored for this ESP32 WiFiClientSecure.
    // To verify root CA or server SSL cerificate,
    // please consult your SSL client documentation.
    ssl_client.setInsecure();

    lbgetCreds();

    Serial.println("ReadyMail, version " + String(READYMAIL_VERSION));

    // In case ESP8266 crashes, please see https://bit.ly/48r4wSe

    smtp.connect(SMTP_HOST, SMTP_PORT, smtpCb, SSL_MODE);
    if (!smtp.isConnected())
        return;

    if (AUTHENTICATION)
    {
        smtp.authenticate(AUTHOR_EMAIL, AUTHOR_PASSWORD, readymail_auth_password);
        if (!smtp.isAuthenticated())
            return;
    }
}

// --------------------------------------------------
// VIẾT CHO TÔI CÁC HÀM NHƯ SAU

// Vì cả 2 loại cảnh báo và gửi lịch sử chat nó giống nhau về cái logo ở trên, nên ông chèn vào cái logo và vài cái head,title,meta,style
// Tui đã cấu hình vài thứ cơ bản rồi, ông tham khảo và chèn giúp tui html cho phù hợp, xem cái hình với hỏi AI cho rõ hơn
// You might wanna check this: https://theorycircuit.com/esp32-projects/simple-way-to-send-email-using-esp32/ 
// [2] https://randomnerdtutorials.com/esp32-send-email-smtp-server-arduino-ide/
void emlStart()
{
    // Clear lại object trước khi làm cái gì nha, vì có thể còn dữ liệu cũ từ mail trước, do đang đặt globalscope
    msg = SMTPMessage();
    bodyText = "";
    bodyHtml = "";

    // Prep work và config, đừng động vào đây nhé!
    msg.headers.add(rfc822_subject, "Nova system notification");
    msg.headers.add(rfc822_from, "Nova <" + String(AUTHOR_EMAIL) + ">");
    msg.headers.add(rfc822_to, "Bạn <" + String(RECIPIENT_EMAIL) + ">"); 
    msg.headers.addCustom("X-Priority", "1");
    msg.headers.addCustom("Importance", "High");
    
    // content transfer encoding e.g. 7bit, base64, quoted-printable
    // 7bit is default for ascii text and quoted-printable is default set for non-ascii text.
    // Dùng QP + UTF8 vì có tiếng Việt
    // msg.text.charSet = "UTF-8";
    msg.text.transferEncoding("quoted-printable");
    // msg.html.charSet = "UTF-8";
    msg.html.transferEncoding("quoted-printable");
    
    // Tui chưa rõ cái EMBED_MESSAGE này là gì lắm, chắc để false cũng được,
    // nhưng nếu ông add HTML vào mà bị lỗi thì cân nhắc đổi sang true ở trên phần #define
    // With embedFile function, the html message will send as attachment.
    if (EMBED_MESSAGE)
        msg.html.embedFile(true, "msg.html", embed_message_type_attachment);

    // Viết code chèn logo vào html
    // Bắt đầu viết từ đây
}

// Hàm báo không thoải mái, nhận vào các giá trị tui để trong params của hàm này
// msg - lời cây xanh nói
// slight, stemp, shumid - giá trị cảm biến
// signore - số lần yêu cầu trợ giúp
void emlBodyUncomfortable(String smsg, int slight, float stemp, float shumid, int signore)
{

}

// Hàm gửi lịch sử trò chuyện, nhận vào 1 string duy nhất
void emlBodyConversation(String slog) {

}

// Chèn mấy cái ở dưới (Cây xanh Nova... safely discard it.) và finalize gửi đi
void emlFinalize() {

    // ông nên ghi vào đây
    // ...
    //

    // Tránh bị Gmail flag là spam, dùng CRLF thay vì LF thôi
    bodyText.replace("\n", "\r\n");
    bodyHtml.replace("\n", "\r\n");

    // Gắn nội dung
    msg.text.body(bodyText);
    msg.html.body(bodyHtml);
    
    // Lấy timestamp
    Serial.println("GET timestamp via NTP Pool");
    configTime(0, 0, "pool.ntp.org");
    while (time(nullptr) < 100000) delay(100);
    msg.timestamp = time(nullptr);
    Serial.println("GET timestamp done!");

    // Gửi
    smtp.send(msg);
    Serial.println("SENT! Check your inbox!");
}

// RỒI HẾT
// --------------------------------------------------



// DANGER
// KHÔNG GHI CODE NÀO VÀO VÒNG LOOP NÀY HẾT
void loop()
{
  // CẢNH BÁO: KHÔNG GHI CODE NÀO VÀO VÒNG LOOP NÀY HẾT
  // VÌ GHI BẬY, GỬI MAIL SPAM LÀ GMAIL KHÓA
  // TUYỆT ĐỐI KHÔNG ĐỘNG VÀO LOOP
}