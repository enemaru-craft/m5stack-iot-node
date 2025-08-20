#include <M5Stack.h>
#include <DHT.h>

#define DHTPIN 22
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

void setup(){
  M5.begin();
  M5.Lcd.setTextSize(2);
  M5.Lcd.setTextColor(WHITE, BLACK);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(50, 100);
  M5.Lcd.println("Hello");
  delay(2000);

  M5.Lcd.fillScreen(BLACK); // 画面をリセット
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("DHT11 Sensor");
  dht.begin();
  delay(2000);
}

void loop(){
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 20);

  if (isnan(temperature) || isnan(humidity)){
    M5.Lcd.setTextColor(WHITE, RED);
    M5.Lcd.println("Can't receive Data from Sensor");
  } else {
    M5.Lcd.setTextSize(3);
    M5.Lcd.setTextColor(GREEN);
    M5.Lcd.printf("TEMP: %.1f C\n", temperature);
    M5.Lcd.printf("HUMI: %.1f %%\n", humidity);
  }

  delay(2000);
}