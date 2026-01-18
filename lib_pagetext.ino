void scrPrepareTftPages(const char* msg, uint16_t startY)
{
  TFT_PAGES_LEN = 0;

  for (uint16_t i = 0; i < MAX_TFT_PAGES; i++)
    TFT_PAGES[i] = "";

  const uint16_t MAX_LINES_PER_PAGE =
      (SCREEN_H - startY) / LINE_HEIGHT;

  uint16_t currentLineCount = 0;

  String word = "";
  String line = "";

  int i = 0;
  while (true)
  {
    if (TFT_PAGES_LEN >= MAX_TFT_PAGES)
    {
      // thêm dấu ... vào trang cuối
      if (TFT_PAGES_LEN > 0)
      {
        TFT_PAGES[MAX_TFT_PAGES - 1] += "...\n";
      }
      break;
    }

    char c = msg[i];

    if (c != ' ' && c != '\0') {
      word += c;
      i++;
      continue;
    }

    // ---------- từ quá dài ----------
    if (tft.getUTF8Width(word.c_str()) > TEXT_W)
    {
      for (uint16_t k = 0; k < word.length(); k++)
      {
        String test = line + word[k];
        if (tft.getUTF8Width(test.c_str()) <= TEXT_W)
        {
          line = test;
        }
        else
        {
          TFT_PAGES[TFT_PAGES_LEN] += line + "\n";
          currentLineCount++;
          line = word[k];

          if (currentLineCount >= MAX_LINES_PER_PAGE)
          {
            TFT_PAGES_LEN++;
            if (TFT_PAGES_LEN >= MAX_TFT_PAGES)
            {
              TFT_PAGES[MAX_TFT_PAGES - 1] += "...\n";
              return;
            }
            TFT_PAGES[TFT_PAGES_LEN] = "";
            currentLineCount = 0;
          }
        }
      }
      word = "";
      if (c == '\0') break;
      i++;
      continue;
    }

    // ---------- test ghép word ----------
    String testLine = line + (line.length() ? " " : "") + word;

    if (tft.getUTF8Width(testLine.c_str()) <= TEXT_W)
    {
      line = testLine;
    }
    else
    {
      TFT_PAGES[TFT_PAGES_LEN] += line + "\n";
      currentLineCount++;
      line = word;

      if (currentLineCount >= MAX_LINES_PER_PAGE)
      {
        TFT_PAGES_LEN++;
        if (TFT_PAGES_LEN >= MAX_TFT_PAGES)
        {
          TFT_PAGES[MAX_TFT_PAGES - 1] += "...\n";
          return;
        }
        TFT_PAGES[TFT_PAGES_LEN] = "";
        currentLineCount = 0;
      }
    }

    word = "";
    if (c == '\0') break;
    i++;
  }

  // flush dòng cuối nếu còn chỗ
  if (TFT_PAGES_LEN < MAX_TFT_PAGES && line.length())
  {
    TFT_PAGES[TFT_PAGES_LEN] += line + "\n";
  }

  TFT_PAGES_LEN++;
  if (TFT_PAGES_LEN > MAX_TFT_PAGES)
    TFT_PAGES_LEN = MAX_TFT_PAGES;
}