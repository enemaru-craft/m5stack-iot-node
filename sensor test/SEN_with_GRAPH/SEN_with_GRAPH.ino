#include <M5Stack.h>
#include <DHT.h>

#define DHTPIN 22
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

// 最大保存数
#define MAX_DATA_POINTS 100

// 温度・湿度履歴保存用
float tempHistory[MAX_DATA_POINTS];
float humidHistory[MAX_DATA_POINTS];
int dataCount = 0;

// 表示モード
enum DisplayMode {
  MODE_NORMAL,
  MODE_TEMP_GRAPH,
  MODE_HUMID_GRAPH
};
DisplayMode mode = MODE_NORMAL;

void setup() {
  M5.begin();
  dht.begin();

  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(50, 100);
  M5.Lcd.println("Starting...");
  delay(3000);
}

void loop() {
  M5.update();

  // ボタン処理
  if (M5.BtnA.wasPressed()) {
    mode = MODE_NORMAL;
    M5.Lcd.fillScreen(BLACK);
  } else if (M5.BtnB.wasPressed()) {
    mode = MODE_TEMP_GRAPH;
    M5.Lcd.fillScreen(BLACK);
  } else if (M5.BtnC.wasPressed()) {
    mode = MODE_HUMID_GRAPH;
    M5.Lcd.fillScreen(BLACK);
  }

  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  // データ記録（有効なときのみ）
  if (!isnan(temperature) && !isnan(humidity)) {
    if (dataCount < MAX_DATA_POINTS) {
      tempHistory[dataCount] = temperature;
      humidHistory[dataCount] = humidity;
      dataCount++;
    } else {
      for (int i = 1; i < MAX_DATA_POINTS; i++) {
        tempHistory[i - 1] = tempHistory[i];
        humidHistory[i - 1] = humidHistory[i];
      }
      tempHistory[MAX_DATA_POINTS - 1] = temperature;
      humidHistory[MAX_DATA_POINTS - 1] = humidity;
    }
  }

  // 表示モードに応じた描画
  switch (mode) {
    case MODE_NORMAL:
      showTempHumidity(temperature, humidity);
      break;
    case MODE_TEMP_GRAPH:
      drawGraph(tempHistory, dataCount, "Temp Graph", "C", PINK);
      break;
    case MODE_HUMID_GRAPH:
      drawGraph(humidHistory, dataCount, "Humid Graph", "%", CYAN);
      break;
  }

  delay(2000);
}

// 通常表示関数
void showTempHumidity(float temperature, float humidity) {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(5, 10);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setTextColor(GREEN);
  M5.Lcd.println("Enemaru Craft");

  if (isnan(temperature) || isnan(humidity)) {
    M5.Lcd.setTextSize(3);
    M5.Lcd.fillScreen(RED);
    M5.Lcd.setTextColor(WHITE);
    M5.Lcd.setCursor(5, 50);
    M5.Lcd.println("Unable to receive deta");
    M5.Lcd.setCursor(5, 100);
    M5.Lcd.println("Please Check the Sensor");
  } else {
    M5.Lcd.setCursor(40,100);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setTextColor(PINK);
    M5.Lcd.printf("TEMP: %.1f C\n", temperature);
    M5.Lcd.setCursor(40,140);
    M5.Lcd.setTextColor(CYAN);
    M5.Lcd.printf("HUMI: %.1f %%\n", humidity);
  }
}

// グラフ描画関数（温度/湿度共通）
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