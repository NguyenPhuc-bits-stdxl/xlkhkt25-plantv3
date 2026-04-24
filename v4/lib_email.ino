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

void emlBodyWelcome() {
    bodyText += "Chào bạn, mình là Nova!";

    bodyHtml += "<div style=\"max-width:600px;margin:0 auto;padding:0 20px;color:#333;\">";
    bodyHtml += "<h2 style=\"text-align:center;font-size:22px;margin-bottom:8px;\">";
    bodyHtml += "Chào bạn, mình là Nova</h2>";

    // Dòng mô tả nhỏ
    bodyHtml += "<p style=\"text-align:center;font-size:14px;color:#666;margin-top:0;\">";
    bodyHtml += "Xin chúc mừng, bạn vừa cấu hình thành công cây xanh yêu dấu của mình.\n";
    bodyHtml += "<br>Hãy cùng nhau xây dựng thói quen chăm sóc cây xanh nào!.";
    bodyHtml += "</p>";
    
    bodyHtml += "</div>";
}

// Hàm báo không thoải mái
void emlBody(String smsg)
{
    // Fallback text
    bodyText += "Thông báo đến bạn iu!\n";
    //bodyText += sysGetSensorsString();

    // HTML Content
    bodyHtml += "<div style=\"max-width:600px;margin:0 auto;padding:0 20px;color:#333;white-space: pre-wrap;\">";
    bodyHtml += "<h2 style=\"text-align:center;font-size:22px;margin-bottom:20px;\">Thông báo đến bạn iu! :3</h2>";
    bodyHtml += "<p>Thư này được gửi đến bạn bởi hệ thống điện tử của <b>cây xanh Nova</b>.</p>";
    bodyHtml += "<div>\"" + smsg + "\"</div>";

    bodyHtml += "</div>";
}

// Chèn chân trang và finalize
void emlFinalize() {
    bodyHtml += "<div style=\"max-width:600px;margin:40px auto 20px auto;text-align:center;font-size:13px;color:#666;border-top:1px solid #eee;padding-top:20px;\">";
    bodyHtml += "<p>Cây xanh Nova là một dự án khoa học kỹ thuật của một nhóm học sinh tại trường Trung học Phổ thông Xuân Lộc.<br>";
    bodyHtml += "Ghé thăm <a href=\"https://github.com/NguyenPhuc-bits-stdxl/xlkhkt25-plantv3\" style=\"color:#007bff;text-decoration:none;\">trang GitHub của nhóm chúng mình!</a></p>";
    bodyHtml += "<p style=\"font-size:11px;color:#aaa;margin-top:20px;\">Nếu bạn không phải là người nhận của email này, bạn có thể an toàn bỏ qua nó.<br>";
    bodyHtml += "<i>If you are not the intended recipient of this email, you can safely discard it.</i></p>";
    bodyHtml += "</div></body></html>";

    // Tránh bị Gmail flag là spam, dùng CRLF thay vì LF thôi
    bodyText.replace("\n", "\r\n");
    bodyHtml.replace("\n", "\r\n");

    // Gắn nội dung
    msg.text.body(bodyText);
    msg.html.body(bodyHtml);
    
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