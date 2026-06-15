#pragma once
#include <Arduino.h>

struct Plant  // General Structure of Plants
{
  const char* name;
  const char* sciname;
  const float tmin;
  const float tmax;
  const float lmin;
  const float lmax;
  const float hmin;
  const float hmax;
  const int suntarget;
  const int watmin;
  const int watmax;
  const char* note;
};


const Plant PLANTS[] = {
  { "Xương rồng",
    "Desert Cactus",
    10,
    40,
    5000,
    99999,
    30,
    60,
    5,
    240,
    336,
    "Quy tắc ngón tay, nếu ngón tay cảm giác khô hoàn toàn hãy tưới cây" },
  { "Lưỡi hổ",
    "Snake Plant",
    13,
    35,
    1000,
    10000,
    40,
    70,
    3,
    168,
    240,
    "Lá cây rủ xuống khi thiếu nước, tưới đầy đủ thì lá cây thẳng bình thường" },
  { "Lan ý",
    "Peace Lily",
    15,
    32,
    1500,
    4000,
    50,
    80,
    0,
    72,
    120,
    "" },
  { "Sen đá",
    "Succulents",
    10,
    35,
    4000,
    25000,
    40,
    60,
    5,
    168,
    240,
    "Nên tưới vào gốc, tránh đọng nước ở kẽ lá, trồng ở loại đất thoát nước nhanh như Perlite, Pumice, xỉ than chiếm 60-70% và 30% đất thịt/phân hữu cơ." },
  { "Trầu bà",
    "Epipremnum aureum",
    18,
    30,
    500,
    10000,
    40,
    80,
    0,
    72,
    168,
    "Ưa sáng gián tiếp, tránh nắng gắt trực tiếp. Để đất khô khoảng 1/3–1/2 chậu rồi mới tưới. Nhiệt độ dưới 10°C có thể gây tổn thương" },
  { "Dương Xỉ",
    "Boston Nephrolepis exaltata",
    16,
    28,
    500,
    5000,
    60,
    95,
    0,
    48,
    96,
    "Tưới nước thường xuyên (khoảng 2 - 3 ngày/lần tùy thời tiết), kết hợp phun sương để duy trì độ ẩm lý tưởng" },
  { "Cây Cọ Nhật",
    "Livistona chinensis",
    15,
    32,
    1000,
    20000,
    40,
    80,
    2,
    72,
    168,
    "đất tơi xốp thoát nước, ánh sáng bán phần và lượng nước vừa phải" },
};

const int PLANTS_COUNT = 7;
