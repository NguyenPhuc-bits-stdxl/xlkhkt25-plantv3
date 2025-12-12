// // ===================== CONFIG =====================
// const int TEXT_W = 124;      // vùng width text (padding trái 2 px)
// const int X_PAD  = 2;        // padding trái
// const int SCREEN_H = 128;
// const int LINE_HEIGHT = 12;  // đổi theo font
// // ==================================================

// // --------------------------------------------------
// // Cắt một từ dài thành nhiều phần vừa với chiều rộng TEXT_W
// // --------------------------------------------------
// void splitLongWord(const String &word, String parts[]) {
//     int partCount = 0;
//     String current = "";

//     for (int i = 0; i < word.length(); i++) {
//         String test = current + word[i];
//         if (u8g2.getUTF8Width(test.c_str()) <= TEXT_W) {
//             current += word[i];
//         } else {
//             parts[partCount++] = current;
//             current = word[i];
//         }
//     }

//     if (current.length() > 0) {
//         parts[partCount++] = current;
//     }

//     parts[partCount] = ""; // kết thúc
// }

// // --------------------------------------------------
// // Hiển thị một trang text (không xoá phần trên startY)
// // --------------------------------------------------
// void showPage(String lines[], int count, int startY) {
//     tft.fillRect(0, startY, 128, SCREEN_H - startY, ST77XX_BLACK);

//     u8g2.setForegroundColor(ST77XX_WHITE);

//     int y = startY + LINE_HEIGHT;
//     for (int i = 0; i < count; i++) {
//         u8g2.setCursor(X_PAD, y);
//         u8g2.print(lines[i]);
//         y += LINE_HEIGHT;
//     }

//     delay(2000);
// }


// // --------------------------------------------------
// // Chia text thành từng trang, tự xuống dòng theo pixel
// // --------------------------------------------------
// void displayPagedText(const char *text, int startY) {
//     String word = "";
//     String line = "";

//     const int MAX_LINES = (SCREEN_H - startY) / LINE_HEIGHT;
//     String pageLines[MAX_LINES];
//     int lineCount = 0;

//     // Bạn đổi font tại đây
//     u8g2.setFont(u8g2_font_6x12_tf);


//     int i = 0;
//     while (true) {
//         char c = text[i];

//         // gom từ
//         if (c != ' ' && c != '\0') {
//             word += c;
//             i++;
//             continue;
//         }

//         // --------- Xử lý trường hợp từ quá dài ---------
//         if (u8g2.getUTF8Width(word.c_str()) > TEXT_W) {
//             String parts[20];
//             splitLongWord(word, parts);

//             for (int p = 0; parts[p] != ""; p++) {
//                 if (line.length() > 0) {
//                     pageLines[lineCount++] = line;
//                     line = "";
//                 }

//                 pageLines[lineCount++] = parts[p];

//                 if (lineCount == MAX_LINES) {
//                     showPage(pageLines, lineCount, startY);
//                     lineCount = 0;
//                 }
//             }

//             word = "";
//             if (c == '\0') break;
//             i++;
//             continue;
//         }
//         // ------------------------------------------------


//         // Test thử nếu ghép word vào line có vượt width?
//         String testLine = line + (line.length() > 0 ? " " : "") + word;
//         int w = u8g2.getUTF8Width(testLine.c_str());

//         if (w <= TEXT_W) {
//             line = testLine;
//         } else {
//             pageLines[lineCount++] = line;
//             line = word;

//             if (lineCount == MAX_LINES) {
//                 showPage(pageLines, lineCount, startY);
//                 lineCount = 0;
//             }
//         }

//         word = "";

//         if (c == '\0') break;
//         i++;
//     }


//     // Thêm dòng cuối cùng
//     if (line.length() > 0) {
//         pageLines[lineCount++] = line;
//     }

//     if (lineCount > 0) {
//         showPage(pageLines, lineCount, startY);
//     }
// }
