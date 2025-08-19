#include <M5Stack.h>
#include <TJpg_Decoder.h>
#include "test.h"  // JPEGをC配列化したヘッダーファイル

// 描画コールバック関数
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  M5.Lcd.pushImage(x, y, w, h, bitmap);
  return true;
}

void setup() {
  M5.begin();
  M5.Lcd.fillScreen(WHITE);

  // TJpg_Decoder 初期化
  TJpgDec.setJpgScale(1);                  // 拡大縮小なし
  TJpgDec.setSwapBytes(true);               // エンディアン調整
  TJpgDec.setCallback(tft_output);          // 出力関数登録

  // 配列からJPEGを描画
  TJpgDec.drawJpg(0, 0, testpic, sizeof(testpic));
}

void loop() {
}
