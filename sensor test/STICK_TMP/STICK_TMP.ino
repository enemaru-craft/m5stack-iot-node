#include <M5Stack.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// DS18B20のデータピン
#define ONE_WIRE_BUS 22

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
  M5.begin();
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("DS18B20 Temp");

  sensors.begin(); // センサ初期化
}

void loop() {
  sensors.requestTemperatures(); // 温度リクエスト
  float tempC = sensors.getTempCByIndex(0); // 最初のセンサの温度を取得

  // 表示更新
  if (tempC == DEVICE_DISCONNECTED_C) {
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.fillRect(0, 40, 320, 40, BLACK);
    M5.Lcd.println("Sensor Error!");
  } else {
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.fillRect(0, 40, 320, 40, BLACK);
    M5.Lcd.printf("Temp: %.2f C", tempC);
}

  delay(1000);
}
