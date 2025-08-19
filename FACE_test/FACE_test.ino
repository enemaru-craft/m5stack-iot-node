#include <M5Stack.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TJpg_Decoder.h>

// JPEGをC配列化したヘッダーファイル
#include "pic1.h"
#include "pic2.h"
#include "pic3.h"

// 描画コールバック関数
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  M5.Lcd.pushImage(x, y, w, h, bitmap);
  return true;
}

// DS18B20のデータピン
#define ONE_WIRE_BUS 26

// 最大保存数
#define MAX_DATA_POINTS 100

// 温度・湿度履歴保存用
float tempHistory[MAX_DATA_POINTS];
int dataCount = 0;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// 表示モード
enum DisplayMode {
  MODE_NORMAL,
  MODE_TEMP_GRAPH
};
DisplayMode mode = MODE_NORMAL;

//エラーカウント
int ErrorCount = 0;

//顔制御
int face = 0;

//温度制御
float temperature = 30.0;
int pm = 0;

void setup() {
  M5.begin();
  randomSeed(analogRead(0));
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(50, 100);
  M5.Lcd.println("Starting...");
  delay(3000);

  // TJpg_Decoder 初期化
  TJpgDec.setJpgScale(2);                  // 拡大縮小なし
  TJpgDec.setSwapBytes(true);               // エンディアン調整
  TJpgDec.setCallback(tft_output);          // 出力関数登録
}

void loop() {
  M5.update();

  int randNum = random(0, 21); //ランダムナンバー生成

  if(temperature > 38.0){
    pm = 1;
  } else if (temperature < 22.0){
    pm = 0;
  }

  if(pm == 0) {
    temperature = temperature + (randNum / 10.0);
  } else{
    temperature = temperature - (randNum / 10.0);
  }

  // ボタン処理
  if (M5.BtnA.wasPressed()) {
    mode = MODE_NORMAL;
    M5.Lcd.fillScreen(BLACK);
  } else if (M5.BtnB.wasPressed()) {
    mode = MODE_TEMP_GRAPH;
    M5.Lcd.fillScreen(BLACK);
  }

  // データ記録（有効なときのみ）
  if (temperature > 0) {
    if (dataCount < MAX_DATA_POINTS) {
      tempHistory[dataCount] = temperature;
      dataCount++;
    } else {
      for (int i = 1; i < MAX_DATA_POINTS; i++) {
        tempHistory[i - 1] = tempHistory[i];
      }
      tempHistory[MAX_DATA_POINTS - 1] = temperature;
    }
  }
  else
    mode = MODE_NORMAL;

  // 表示モードに応じた描画
  switch (mode) {
    case MODE_NORMAL:
      showTemp(temperature);
      break;
    case MODE_TEMP_GRAPH:
      drawGraph(tempHistory, dataCount, "Temp Graph", "C", PINK);
      break;
  }

  delay(1000);
}

void showTemp(float temperature) {
  M5.Lcd.fillScreen(BLACK);
  if (temperature <= 0) {
    M5.Lcd.setTextSize(3);
    M5.Lcd.fillScreen(RED);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(5, 50);
    M5.Lcd.println("Unable to receive data");
    M5.Lcd.setCursor(5, 100);
    M5.Lcd.println("Please Check the Sensor");
    ErrorCount++;
    if(ErrorCount > 20){
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.fillScreen(RED);
      M5.Lcd.setCursor(50, 10);
      M5.Lcd.println(" ");
      M5.Lcd.println("Sensor Error");
      M5.Lcd.println("Reboot...");
      delay(1000);
      ESP.restart();
    }
  } else {
    M5.Lcd.fillRect(0, 240-120, 320, 90, 0x8410);
    M5.Lcd.fillRect(0, 240-122, 320, 4, WHITE);
    M5.Lcd.fillRect(0, 240-38, 320, 4, WHITE);
    M5.Lcd.setCursor(5,140);
    M5.Lcd.setTextSize(5);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.printf("%.1f C\n", temperature);
    drawFace(temperature);
    drawUI();
  }
}

// グラフ描画関数
void drawGraph(float data[], int count, const char* title, const char* unit, uint16_t color) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println(title);

  M5.Lcd.drawRect(10, 40, 300, 150, WHITE);  // グラフ枠

  if (count < 2) return;

  // 最小・最大値を求める
  float minVal = data[0];
  float maxVal = data[0];
  for (int i = 1; i < count; i++) {
    if (data[i] < minVal) minVal = data[i];
    if (data[i] > maxVal) maxVal = data[i];
  }

  maxVal = maxVal + 7; maxVal = roundToNerest5(maxVal);
  minVal = minVal - 5; minVal = roundToNerest5(minVal);

  if (minVal == maxVal) maxVal += 1;

  // ★ グラフの線を描画
  for (int i = 1; i < count; i++) {
    int x0 = map(i - 1, 0, MAX_DATA_POINTS - 1, 10, 310);
    int y0 = map(data[i - 1], minVal, maxVal, 190, 40);
    int x1 = map(i, 0, MAX_DATA_POINTS - 1, 10, 310);
    int y1 = map(data[i], minVal, maxVal, 190, 40);
    M5.Lcd.drawLine(x0, y0, x1, y1, color);
  }

  // ★ 最新のデータ値を表示（点＋数値）
  int lastIndex = count - 1;
  int x = map(lastIndex, 0, MAX_DATA_POINTS - 1, 10, 310);
  int y = map(data[lastIndex], minVal, maxVal, 190, 40);
  M5.Lcd.fillCircle(x, y, 3, RED);  // 最新点を赤丸で表示
  if(x > 230){
    x = 230;
    y = 30;
  }

  M5.Lcd.setTextColor(RED);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(x + 5, y - 10);  // 点の横に表示
  M5.Lcd.printf("%.1f%s", data[lastIndex], unit);

  // 上下の最大・最小値のラベル
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(220, 190);
  M5.Lcd.printf("%.1f%s", minVal, unit);
  M5.Lcd.setCursor(220, 40);
  M5.Lcd.printf("%.1f%s", maxVal, unit);

  drawUI();
}

//グラフの最大最小点を出す
float roundToNerest5(float value) {
  return round(value / 5.0) * 5.0;
}

// ボタンUIを描画する関数
void drawUI() {
  int y = 240 - 30;  // 画面下部に表示

  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE, 0x8410);
  M5.Lcd.fillRect(0, y, 320, 30, 0x8410);

  // Aボタン（温度表示）
  M5.Lcd.setCursor(40, y + 5);
  M5.Lcd.print("TEMP");

  // Bボタン（グラフ表示）
  M5.Lcd.setCursor(130, y + 5);
  M5.Lcd.print("GRAPH");

  // Cボタン（未使用）
  M5.Lcd.setCursor(222, y + 5);
  M5.Lcd.print("     ");
}

void drawFace(float tmp) {
  int centerX = M5.Lcd.width() / 2;
  int centerY = (M5.Lcd.height() / 2) - 20;

  if (tmp < 25.0){
    M5.Lcd.fillRect(0, 0, 320, 120, 0x000F);
    // 口（弧状の線で再現）
    for (int i = -30; i <= 30; i++) {
      int y = (int)(0.02 * i * i);  // 放物線（口のカーブ）
      M5.Lcd.drawPixel(centerX + i, centerY - 20 + y, WHITE);
    }
    // 配列からJPEGを描画
    TJpgDec.drawJpg(220, 122, pic1, sizeof(pic1));
  } else if (tmp < 32.0) {
    M5.Lcd.fillRect(0, 0, 320, 120, 0x03E0);
    // 口（弧状の線で再現）
    for (int i = -30; i <= 30; i++) {
      int y = (int)(0.0 * i * i);  // 放物線（口のカーブ）
      M5.Lcd.drawPixel(centerX + i, centerY - 10 + y, WHITE);
    }
    TJpgDec.drawJpg(220, 122, pic2, sizeof(pic2));
  } else {
    M5.Lcd.fillRect(0, 0, 320, 120, 0xFA20);
    // 口（弧状の線で再現）
    for (int i = -30; i <= 30; i++) {
      int y = (int)(-0.02 * i * i);  // 放物線（口のカーブ）
      M5.Lcd.drawPixel(centerX + i, centerY - 10 + y, WHITE);
    }
    TJpgDec.drawJpg(220, 122, pic3, sizeof(pic3));
  }

  if (face == 0){
    // 目
    M5.Lcd.fillCircle(centerX - 40, centerY - 70, 10, WHITE);
    M5.Lcd.fillCircle(centerX + 40, centerY - 70, 10, WHITE);
  } else {
    // 目
    M5.Lcd.fillRect(centerX - 50, centerY - 70, 20, 5, WHITE);
    M5.Lcd.fillRect(centerX + 30, centerY - 70, 20, 5, WHITE);
  }

  

  if(face == 0)
    face = 1;
  else
    face = 0;
}