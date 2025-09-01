#include <M5Stack.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TJpg_Decoder.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "time.h"

// 以下は画像C配列をインクルード
#include "pic1.h"
#include "pic2.h"
#include "pic3.h"

// wifi接続用のヘッダファイル
#include "wifi.h"

// GPIOデータピン
#define ONE_WIRE_BUS 26

// 移動平均用
#define WINDOW_SIZE 5
float history[WINDOW_SIZE] = {0}; // 過去5回分の値を保存
int idx = 0;

// センサ
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// データ最大保存数
#define MAX_DATA_POINTS 100

// 温度保存用配列
float dataHistory[MAX_DATA_POINTS];
int dataCount = 0;

// 画像描画コールバック関数
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  M5.Lcd.pushImage(x, y, w, h, bitmap);
  return true;
}

// 表示モード
enum DisplayMode {
  MODE_NORMAL,
  MODE_TEMP_GRAPH
};
DisplayMode mode = MODE_NORMAL;

// エラーカウント
int ErrorCount = 0;

// 顔制御
int face = 0;

// 温度制御
int pm = 0;

// 画面モード確認用変数
int viewmode = 0; // 0がデータ,1がグラフ

// ルームID
int roomID = 0;
bool decided = false; // 決定されたかどうか

// NTPサーバ設定
const char* ntpServer = "ntp.nict.jp";
const long  gmtOffset_sec = 9 * 3600; // JST = UTC+9
const int   daylightOffset_sec = 0;

// =========================
// FreeRTOS タスク用ハンドル
// =========================
TaskHandle_t TaskSensorHandle;
TaskHandle_t TaskDisplayHandle;

// センサ値共有用
float latestData = 20.0;
float latestAvg  = 0.0;

// ==================
// センサ値取得タスク
// ==================
void TaskSensor(void *pvParameters) {
  while (1) {
    int randNum = random(0, 21);

    if(latestData > 38.0){
      pm = 1;
    } else if (latestData < 22.0){
      pm = 0;
    }

    if(pm == 0) {
      latestData = latestData + (randNum / 10.0);
    } else {
      latestData = latestData - (randNum / 10.0);
    }

    // 移動平均を更新
    history[idx] = latestData;
    idx = (idx + 1) % WINDOW_SIZE;

    float sum = 0;
    int count = 0;
    for (int i = 0; i < WINDOW_SIZE; i++) {
      if (history[i] != 0) {
        sum += history[i];
        count++;
      }
    }
    if (count == 0) { count = 1; sum = 0; } // エラー処理
    latestAvg = sum / count;

    // dataHistory に保存
    if (dataCount < MAX_DATA_POINTS) {
      dataHistory[dataCount++] = latestAvg;
    } else {
      for (int i = 1; i < MAX_DATA_POINTS; i++) {
        dataHistory[i - 1] = dataHistory[i];
      }
      dataHistory[MAX_DATA_POINTS - 1] = latestAvg;
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS); // 1秒ごと更新
  }
}

// ==============
// 画面描画タスク
// ==============
void TaskDisplay(void *pvParameters) {
  while(1) {
    M5.update();

    // ボタン処理
    if (M5.BtnA.wasPressed()) {
      mode = MODE_NORMAL;
    } else if (M5.BtnB.wasPressed()) {
      mode = MODE_TEMP_GRAPH;
    }

    // 表示モードに応じて描画
    if (latestAvg > 0) {
      switch (mode) {
        case MODE_NORMAL:
          showData(latestAvg);
          break;
        case MODE_TEMP_GRAPH:
          drawGraph(dataHistory, dataCount, "Temp Graph", "C", PINK);
          break;
      }
    } else {
      // エラー表示
      ErrorView();
    }

    vTaskDelay(1000 / portTICK_PERIOD_MS); // 1秒周期で描画更新
  }
}

// セットアップ
void setup() {
  M5.begin();
  randomSeed(analogRead(0)); //ランダムナンバーの種
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextSize(2);

  // WiFI接続
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while( WiFi.status() != WL_CONNECTED) {
    delay(500); 
    M5.Lcd.print("."); 
  }
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.println("WiFi connected");
  M5.Lcd.print("IP address = ");
  M5.Lcd.println(WiFi.localIP());

  // デバイス登録
  registerDevice("3030", "EBAA505B-6A54-4B27-AEAF-1432054DB130", "geothermal")

  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(50, 100);
  M5.Lcd.println("Starting...");
  // NTP初期化
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  delay(1000);

  M5.Lcd.clear(BLACK);
  while (!decided) {
    M5.update();
    IDUI(); // UI表示

    // ボタン操作
    if (M5.BtnA.wasPressed()) {
      if (roomID > 0) roomID--;   // マイナス防止
    }
    if (M5.BtnC.wasPressed()) {
      roomID++;
    }
    if (M5.BtnB.wasPressed()) {
      decided = true;
    }

    delay(150); // チラつき防止
  }

  // TJpg_Decoder 初期化
  TJpgDec.setJpgScale(2);          // 1/2倍
  TJpgDec.setSwapBytes(true);      // エンディアン調整
  TJpgDec.setCallback(tft_output); // 出力関数登録

  // ID決定後の表示
  M5.Lcd.clear(BLACK);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(CYAN, BLACK);
  M5.Lcd.setTextDatum(MC_DATUM);
  M5.Lcd.drawString("Room ID Decided!", M5.Lcd.width() / 2, 100);
  M5.Lcd.setTextSize(6);
  M5.Lcd.drawString(String(roomID), M5.Lcd.width() / 2, 160);
  M5.Lcd.setTextDatum(ML_DATUM);
  delay(2000);

  // データ表示の背景を描画
  M5.Lcd.clear(BLACK);
  M5.Lcd.fillRect(0, 240-120, 320, 120, 0x8410);
  M5.Lcd.fillRect(0, 240-122, 320, 4, WHITE);
  M5.Lcd.fillRect(0, 240-38, 320, 4, WHITE);

  // FreeRTOS タスク生成
  xTaskCreatePinnedToCore(
    TaskSensor,
    "TaskSensor",
    4096,
    NULL,
    1,
    &TaskSensorHandle,
    0   // Core0で実行
  );

  xTaskCreatePinnedToCore(
    TaskDisplay,
    "TaskDisplay",
    8192,
    NULL,
    1,
    &TaskDisplayHandle,
    1   // Core1で実行
  );
}

// メイン
void loop() {
  // 遅延時間
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}

//データ表示
void showData(float data) {
  if (viewmode == 1) { //変更後の最初のみ実行
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.fillRect(0, 240-120, 320, 120, 0x8410);
    M5.Lcd.fillRect(0, 240-122, 320, 4, WHITE);
    M5.Lcd.fillRect(0, 240-38, 320, 4, WHITE);
    viewmode = 0;
  }
  drawUI(data);
  M5.Lcd.setCursor(35,145);
  M5.Lcd.setTextSize(5);
  M5.Lcd.setTextColor(WHITE, 0x8410);
  M5.Lcd.printf("%.1f C\n", data);
}


// グラフ描画関数
void drawGraph(float data[], int count, const char* title, const char* unit, uint16_t color) {
  if (viewmode == 0) { //変更後の最初のみ実行
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.fillRect(0, 207, 320, 37, 0x8410);
    viewmode = 1;
  }
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println(title);

  M5.Lcd.drawRect(9, 39, 301, 151, WHITE);  // グラフ枠
  M5.Lcd.fillRect(10, 40, 299, 149, BLACK);  // 中を埋める

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

  // グラフの線を描画
  for (int i = 1; i < count; i++) {
    int x0 = map(i - 1, 0, MAX_DATA_POINTS - 1, 10, 305);
    int y0 = map(data[i - 1], minVal, maxVal, 190, 35);
    int x1 = map(i, 0, MAX_DATA_POINTS - 1, 10, 305);
    int y1 = map(data[i], minVal, maxVal, 190, 35);
    M5.Lcd.drawLine(x0, y0, x1, y1, color);
  }

  // 最新のデータ値を表示（点＋数値）
  int lastIndex = count - 1;
  int x = map(lastIndex, 0, MAX_DATA_POINTS - 1, 10, 305);
  int y = map(data[lastIndex], minVal, maxVal, 190, 35);
  M5.Lcd.fillCircle(x, y, 3, RED);  // 最新点を赤丸で表示
  if(x > 230){
    x = 230;
    y = 30;
  }

  M5.Lcd.setTextColor(RED,BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(x + 5, y - 10);  // 点の横に表示
  M5.Lcd.printf("%.1f%s", data[lastIndex], unit);

  // 上下の最大・最小値のラベル
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(20, 191);
  M5.Lcd.printf("%.1f%s", minVal, unit);
  M5.Lcd.setCursor(20, 23);
  M5.Lcd.printf("%.1f%s", maxVal, unit);

  drawButton();
}

//グラフの最大最小点を出す
float roundToNerest5(float value) {
  return round(value / 5.0) * 5.0;
}

// 顔、線などのUIを描画
void drawUI(float tmp) {
  // 中心点
  int centerX = M5.Lcd.width() / 2;
  int centerY = (M5.Lcd.height() / 2) - 20;

  // 温度に応じた背景色、口、写真の表示
  if (tmp < 25.0){
    M5.Lcd.fillRect(0, 0, 320, 120, 0x000F);
    // 口（弧状の線で再現）
    for (int i = -30; i <= 30; i++) {
      int y = (int)(0.02 * i * i);  // 放物線（口のカーブ）
      M5.Lcd.drawPixel(centerX + i, centerY - 20 + y, WHITE);
    }
    // 配列からJPEGを描画
    TJpgDec.drawJpg(230, 122, pic1, sizeof(pic1));
  } else if (tmp < 35.0) {
    M5.Lcd.fillRect(0, 0, 320, 120, 0x03E0);
    // 口（弧状の線で再現）
    for (int i = -30; i <= 30; i++) {
      int y = (int)(0.0 * i * i);  // 放物線（口のカーブ）
      M5.Lcd.drawPixel(centerX + i, centerY - 10 + y, WHITE);
    }
    TJpgDec.drawJpg(230, 122, pic2, sizeof(pic2));
  } else {
    M5.Lcd.fillRect(0, 0, 320, 120, 0xFA20);
    // 口（弧状の線で再現）
    for (int i = -30; i <= 30; i++) {
      int y = (int)(-0.02 * i * i);  // 放物線（口のカーブ）
      M5.Lcd.drawPixel(centerX + i, centerY - 10 + y, WHITE);
    }
    TJpgDec.drawJpg(230, 122, pic3, sizeof(pic3));
  }

  // センサ情報が更新されるたびに目のかたちを変更
  if (face == 0){
    M5.Lcd.fillCircle(centerX - 40, centerY - 70, 10, WHITE);
    M5.Lcd.fillCircle(centerX + 40, centerY - 70, 10, WHITE);
    face = 1;
  } else {
    M5.Lcd.fillRect(centerX - 50, centerY - 70, 20, 5, WHITE);
    M5.Lcd.fillRect(centerX + 30, centerY - 70, 20, 5, WHITE);
    face = 0;
  }

  //ボタンUI表示
  drawButton();
}

// ボタンUI表示関数
void drawButton() {
  // ボタンUI表示用の座標
  int y = 240 - 30;

  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE, 0x8410);

  // Aボタン（温度表示）
  M5.Lcd.setCursor(40, y + 6);
  M5.Lcd.print("TEMP");

  // Bボタン（グラフ表示）
  M5.Lcd.setCursor(130, y + 6);
  M5.Lcd.print("GRAPH");

  // Cボタン（時計）
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    M5.Lcd.setCursor(222, y + 6);
    M5.Lcd.println("Error");
    return;
  }
  M5.Lcd.setCursor(220, y + 6);
  M5.Lcd.printf("%02d:%02d:%02d",
                timeinfo.tm_hour,
                timeinfo.tm_min,
                timeinfo.tm_sec);
}

// エラー表示
void ErrorView() {
  M5.Lcd.setTextSize(3);
  M5.Lcd.fillScreen(RED);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(5, 50);
  M5.Lcd.println("Unable to receive data");
  M5.Lcd.setCursor(5, 100);
  M5.Lcd.println("Please Check the Sensor");
  ErrorCount++;
  // 再起動
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
}

// ルームID決定用のUIを表示
void IDUI() {
  // 左ボタンの上に「-」
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(20, 20);
  M5.Lcd.print("-");

  // 右ボタンの上に「+」
  M5.Lcd.setCursor(280, 20);
  M5.Lcd.print("+");

  // 中央にルームID
  M5.Lcd.setTextSize(6);
  M5.Lcd.setTextColor(GREEN, BLACK);
  M5.Lcd.setCursor(110, 100);
  M5.Lcd.printf("%2d", roomID);

  // 下に「決定」ガイド
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(YELLOW);
  M5.Lcd.setCursor(110, 210);
  M5.Lcd.print("Press B to Decide");
}