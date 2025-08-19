#include <M5Stack.h>
#include <Wire.h>
#include "Adafruit_SHT31.h"

Adafruit_SHT31 sht31 = Adafruit_SHT31();

void setup() {
  M5.begin();
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("SEN23292P 温度センサ");

  // SHT31初期化
  if (!sht31.begin(0x44)) { // ENV UnitのI2Cアドレスは0x44
    M5.Lcd.println("SHT31が見つかりません！");
    while (1) delay(1);
  }
}

void loop() {
  float temp = sht31.readTemperature();

  if (!isnan(temp)) {
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.fillRect(0, 40, 320, 30, BLACK);  // 前の表示を消す
    M5.Lcd.printf("温度: %.2f C", temp);
  } else {
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.println("温度読み取り失敗");
  }

  delay(1000); // 1秒ごとに更新
}
