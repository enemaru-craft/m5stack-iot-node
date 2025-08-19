#include <M5Stack.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WiFiUDP.h>

// WiFI接続関連
const char ssid[] = "TsUki"; //WiFIのSSIDを入力
const char pass[] = "yuki2995"; // WiFiのパスワードを入力
WiFiUDP wifiUdp; 
const char *pc_addr = "10.177.251.164";  //"192.168.0.6";
const int pc_port = 50007; //送信先のポート
const int my_port = 50008;  //自身のポート

// DS18B20のデータピン
#define ONE_WIRE_BUS 22

// 最大保存数
#define MAX_DATA_POINTS 100

// 温度・湿度履歴保存用
float tempHistory[MAX_DATA_POINTS];
int dataCount = 0;

//エラーカウント
int ErrorCount = 0;

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// 表示モード
enum DisplayMode {
  MODE_NORMAL,
  MODE_TEMP_GRAPH
};
DisplayMode mode = MODE_NORMAL;

void setup() {
  M5.begin();
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(50, 10);
  M5.Lcd.println("Starting...");
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(10, 50);
  WiFi.begin(ssid, pass);
  while( WiFi.status() != WL_CONNECTED) {
    delay(500); 
    M5.Lcd.print(".");
    ErrorCount++;
    if(ErrorCount > 30){
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.fillScreen(RED);
      M5.Lcd.setCursor(50, 10);
      M5.Lcd.println("Connection Error");
      M5.Lcd.println("Reboot...");
      delay(1000); // 少し待ってから
      ESP.restart();
    }
  }
  ErrorCount = 0;
  M5.Lcd.println(" ");
  M5.Lcd.setTextColor(BLACK, GREEN);
  M5.Lcd.println("WiFi connected");
  M5.Lcd.print("IP address = ");
  M5.Lcd.println(WiFi.localIP());
  
  wifiUdp.begin(my_port);

  delay(3000);

  sensors.begin(); // センサ初期化
}

void loop() {
  M5.update();
  sensors.requestTemperatures(); // 温度リクエスト
  float temperature = sensors.getTempCByIndex(0); // センサの温度を取得
  wifiUdp.beginPacket(pc_addr, pc_port); // WiFi設定

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

  delay(1000); // データ更新頻度
}

void showTemp(float temperature) {
  if (temperature <= 0) {
    M5.Lcd.setTextSize(3);
    M5.Lcd.fillScreen(RED);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(5, 50);
    M5.Lcd.println("Unable to receive data");
    M5.Lcd.setCursor(5, 100);
    M5.Lcd.println("Please Check the Sensor");
    M5.Lcd.print("ErrorCount: ");
    M5.Lcd.println(ErrorCount);
    // WiFi
    wifiUdp.write((const uint8_t *)"ERROR", strlen("ERROR"));
    wifiUdp.endPacket();
    ErrorCount++;
    if(ErrorCount > 20){
      M5.Lcd.setTextColor(WHITE);
      M5.Lcd.fillScreen(RED);
      M5.Lcd.setCursor(50, 10);
      M5.Lcd.println(" ");
      M5.Lcd.println("Sensor Error");
      M5.Lcd.println("Reboot...");
      wifiUdp.write((const uint8_t *)"REBOOT", strlen("REBOOT"));
      wifiUdp.endPacket();
      delay(1000);
      ESP.restart();
    }
  } else {
    // 以下WiFIでデータをPCに送信
    char buf[16];
    sprintf(buf, "%.1f", temperature);
    wifiUdp.write((const uint8_t*)buf, strlen(buf));
    wifiUdp.endPacket();
    ErrorCount = 0; //エラーカウント
    // 画面表示
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(5, 10);
    M5.Lcd.setTextSize(4);
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.println("Enemaru Craft");
    M5.Lcd.setCursor(40,120);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setTextColor(PINK);
    M5.Lcd.printf("TEMP: %.1f C\n", temperature);
  }
}

// グラフ描画関数
void drawGraph(float data[], int count, const char* title, const char* unit, uint16_t color) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println(title);

  M5.Lcd.drawRect(10, 40, 300, 180, WHITE);  // グラフ枠

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
    int y0 = map(data[i - 1], minVal, maxVal, 220, 40);
    int x1 = map(i, 0, MAX_DATA_POINTS - 1, 10, 310);
    int y1 = map(data[i], minVal, maxVal, 220, 40);
    M5.Lcd.drawLine(x0, y0, x1, y1, color);
  }

  // ★ 最新のデータ値を表示（点＋数値）
  int lastIndex = count - 1;
  int x = map(lastIndex, 0, MAX_DATA_POINTS - 1, 10, 310);
  int y = map(data[lastIndex], minVal, maxVal, 220, 40);
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
  M5.Lcd.setCursor(220, 220);
  M5.Lcd.printf("%.1f%s", minVal, unit);
  M5.Lcd.setCursor(220, 40);
  M5.Lcd.printf("%.1f%s", maxVal, unit);
}

//グラフの最大最小点を出す
float roundToNerest5(float value) {
  return round(value / 5.0) * 5.0;
}