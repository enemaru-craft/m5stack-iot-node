#include <M5Stack.h>

// Port A のピンをUARTに利用
// SDA(GPIO21) → RX, SCL(GPIO22) → TX
HardwareSerial SensorSerial(2);

void setup() {
  M5.begin();
  M5.Lcd.setTextSize(2);
  M5.Lcd.println("Wind Sensor Reader (Port A)");

  // UART初期化 (センサの仕様に合わせる)
  SensorSerial.begin(19200, SERIAL_8N1, 21, 22); 
}

void loop() {
  if (SensorSerial.available()) {
    String data1 = SensorSerial.readStringUntil('\n'); // 改行まで取得
    String data2 = SensorSerial.readStringUntil('\n'); // 改行まで取得
    String data3 = SensorSerial.readStringUntil('\n'); // 改行まで取得
    String data4 = SensorSerial.readStringUntil('\n'); // 改行まで取得
    data3.trim();

    // 画面に表示
    M5.Lcd.fillScreen(BLACK);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.setTextColor(GREEN, BLACK);
    M5.Lcd.println("Wind Sensor Data:");
    M5.Lcd.setTextSize(3);
    M5.Lcd.println(data1);
    M5.Lcd.println(data2);
    M5.Lcd.println(data3);
    M5.Lcd.println(data4);
  }

  delay(100);
}
