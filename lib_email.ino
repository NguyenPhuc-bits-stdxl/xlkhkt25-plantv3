void emlInit()
{
    ssl_client.setInsecure();
    Serial.println("ReadyMail, version " + String(READYMAIL_VERSION));

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

void emlStart()
{
    // sync time
    sysSyncNTP();

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
    
    msg.text.transferEncoding("quoted-printable");
    msg.html.transferEncoding("quoted-printable");
    
    if (EMBED_MESSAGE)
        msg.html.embedFile(true, "msg.html", embed_message_type_attachment);

    // HTML Header và chèn Logo
    bodyHtml = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"></head>";
    bodyHtml += "<body style=\"margin:0;padding:0;font-family:Arial,sans-serif;background-color:#ffffff;\">";
    bodyHtml += "<div style=\"text-align:center;padding:30px 0;\">";
    // Link logo 
    bodyHtml += "<img src=\"https://raw.githubusercontent.com/NguyenPhuc-bits-stdxl/xlkhkt25-plantv3/refs/heads/main/logoNovaEmail.png\" width=\"120\" alt=\"Nova Logo\">";
    bodyHtml += "</div>";
}

// Hàm báo không thoải mái
void emlBodyUncomfortable(String smsg, int slight, float stemp, float shumid, int signore, String swhere, String stime)
{
    // Fallback text
    bodyText += "Bạn ơi, tớ đang cảm thấy khó chịu!\n";
    //bodyText += sysGetSensorsString();

    // HTML Content
    bodyHtml += "<div style=\"max-width:600px;margin:0 auto;padding:0 20px;color:#333;\">";
    bodyHtml += "<h2 style=\"text-align:center;font-size:22px;margin-bottom:20px;\">Bạn ơi, tớ đang cảm thấy khó chịu!</h2>";
    bodyHtml += "<p>Thư này được gửi đến bạn bởi hệ thống điện tử của <b>cây xanh Nova</b>, cây xanh hiện đang cảm thấy không ổn và cần nhờ đến sự trợ giúp của bạn.</p>";
    bodyHtml += "<p><b>Lời nhắn từ cây xanh:</b><br><span style=\"color:#d35400;font-style:italic;\">\"" + smsg + "\"</span></p>";
    
    bodyHtml += "<p><b>Thông tin chi tiết:</b></p>";
    bodyHtml += "<ul style=\"list-style:none;padding-left:0;line-height:1.8;\">";
    bodyHtml += "<li>- <b>Thời gian:</b> " + stime + "</li>";
    bodyHtml += "<li>- <b>Địa điểm:</b> " + swhere + "</li>";
    bodyHtml += "<li>- <b>Chỉ số cảm biến ánh sáng (từ 0-4095):</b> " + String(slight) + "</li>";
    bodyHtml += "<li>- <b>Nhiệt độ:</b> " + String(stemp, 1) + " °C</li>";
    bodyHtml += "<li>- <b>Độ ẩm:</b> " + String(shumid, 1) + " %</li>";
    bodyHtml += "<li>- <b>Đã yêu cầu trợ giúp:</b> " + String(signore) + " lần liên tiếp kể từ thời điểm tương tác gần nhất.</li>";
    bodyHtml += "</ul></div>";
}

// Hàm gửi lịch sử trò chuyện
void emlBodyConversation(String slog, int slight, float stemp, float shumid, int signore, String swhere, String stime) {
    bodyText += "LỊCH SỬ TRÒ CHUYỆN\n";
    bodyText += "Thư này được gửi đến bạn bởi hệ thống điện tử của cây xanh Nova.\n";
    bodyText += "Dưới đây là lịch sử trò chuyện giữa bạn và Nova.\n\n";
    bodyText += slog + "\n\n";
    bodyText += "Thông tin chi tiết:\n";
    bodyText += "- Thời gian: " + stime + "\n";
    bodyText += "- Địa điểm: " + swhere + "\n";


    bodyHtml += "<div style=\"max-width:600px;margin:0 auto;padding:0 20px;color:#333;\">";
    bodyHtml += "<h2 style=\"text-align:center;font-size:22px;margin-bottom:8px;\">";
    bodyHtml += "Lịch sử trò chuyện</h2>";

    // Dòng mô tả nhỏ
    bodyHtml += "<p style=\"text-align:center;font-size:14px;color:#666;margin-top:0;\">";
    bodyHtml += "Thư này được gửi đến bạn bởi hệ thống điện tử của <b>cây xanh Nova</b>.";
    bodyHtml += "<br>Dưới đây là lịch sử trò chuyện giữa bạn và Nova.";
    bodyHtml += "</p>";

    // Khối lịch sử trò chuyện
    bodyHtml += "<div style=\"white-space:pre-wrap;";
    bodyHtml += "font-family:monospace;";
    bodyHtml += "font-size:13px;";
    bodyHtml += "color:#555;";
    bodyHtml += "margin:25px 0;\">";
    bodyHtml += slog;
    bodyHtml += "</div>";

    // Thông tin chi tiết
    bodyHtml += "<h3 style=\"margin-top:30px;\">Thông tin chi tiết</h3>";
    bodyHtml += "<ul style=\"list-style:none;padding-left:0;line-height:1.8;\">";
    bodyHtml += "<li>- <b>Thời gian:</b> " + stime + "</li>";
    bodyHtml += "<li>- <b>Địa điểm:</b> " + swhere + "</li>";
    bodyHtml += "<li>- <b>Chỉ số cảm biến ánh sáng (từ 0-4095):</b> " + String(slight) + "</li>";
    bodyHtml += "<li>- <b>Nhiệt độ:</b> " + String(stemp, 1) + " °C</li>";
    bodyHtml += "<li>- <b>Độ ẩm:</b> " + String(shumid, 1) + " %</li>";
    bodyHtml += "<li>- <b>Đã yêu cầu trợ giúp:</b> " + String(signore) + " lần liên tiếp kể từ thời điểm tương tác gần nhất.</li>";
    bodyHtml += "</ul>";

    bodyHtml += "</div>";
}

// Chèn chân trang và finalize
void emlFinalize() {
    bodyHtml += "<div style=\"max-width:600px;margin:40px auto 20px auto;text-align:center;font-size:13px;color:#666;border-top:1px solid #eee;padding-top:20px;\">";
    bodyHtml += "<p>Cây xanh Nova là một dự án khoa học kỹ thuật của một nhóm học sinh tại trường Trung học Phổ thông Xuân Lộc.<br>";
    bodyHtml += "Ghé thăm <a href=\"#\" style=\"color:#007bff;text-decoration:none;\">trang GitHub của nhóm chúng mình!</a></p>";
    bodyHtml += "<p style=\"font-size:11px;color:#aaa;margin-top:20px;\">Nếu bạn không phải là người nhận của email này, bạn có thể an toàn bỏ qua nó.<br>";
    bodyHtml += "<i>If you are not the intended recipient of this email, you can safely discard it.</i></p>";
    bodyHtml += "</div></body></html>";

    // Tránh bị Gmail flag là spam, dùng CRLF thay vì LF thôi
    // bodyText.replace("\n", "\r\n");
    // bodyHtml.replace("\n", "\r\n");

    // Gắn nội dung
    msg.text.body(bodyText);
    msg.html.body(bodyHtml);
    
    // Lấy timestamp
    // Serial.println("GET timestamp via NTP Pool");
    // configTime(7 * 3600, 0, "pool.ntp.org");
    // msg.timestamp = time(nullptr);
    // Serial.println("GET timestamp done!");

    // Check định dạng email người nhận
    if (RECIPIENT_EMAIL == "") {
        Serial.println("RM Email can't be null, task cancelled.");
        return;
    }
    if (RECIPIENT_EMAIL.indexOf("@") == std::string::npos) {
        Serial.println("RM Email doesn't sastify the format, task aborted.");
        return;
    }
    if (RECIPIENT_EMAIL.length() < 7) { //a@a.com minlength = 7
        Serial.println("RM Email too short, aborted");
        return;
    }

    // Gửi
    if (smtp.send(msg)) {
        Serial.println("SENT! Check your inbox!");
    } else {
        Serial.println("SEND FAILED! Check connection.");
    }
}