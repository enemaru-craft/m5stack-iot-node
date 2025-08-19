#include <M5Stack.h>

#define LIGHT_SENSOR_PIN 36  // GPIOPIN

void setup() {
  M5.begin();
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.print("Light Sensor");

  // アナログ読み取りの範囲が 0～4095
}

void loop() {
  int sensorValue = analogRead(LIGHT_SENSOR_PIN);  // アナログ値を読み取る（0～4095）
  
  M5.Lcd.fillRect(0, 30, 320, 30, BLACK);  // 前の値を消す（表示更新のため）
  M5.Lcd.setCursor(0, 30);
  M5.Lcd.print("Value: ");
  M5.Lcd.print(sensorValue);

  delay(500);  // 500msごとに更新
}