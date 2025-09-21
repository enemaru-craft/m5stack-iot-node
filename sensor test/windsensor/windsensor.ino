#include <M5Stack.h>

// Port A のピンをUARTに利用
// SDA(GPIO21) → RX, SCL(GPIO22) → TX
HardwareSerial SensorSerial(2);

int sigcount = 0;

void setup() {
  M5.begin();
  M5.Lcd.setTextSize(2);
  M5.Lcd.println("Wind Sensor Reader (Port A)");

  // UART初期化 (センサの仕様に合わせる)
  SensorSerial.begin(19200, SERIAL_8N1, 21, 22); 
}

void loop() {
  if (SensorSerial.available()) {
    // センサ値を取得
    String data1 = SensorSerial.readStringUntil('\n'); // フラグ
    String data2 = SensorSerial.readStringUntil('\n'); // 風力
    String data3 = SensorSerial.readStringUntil('\n'); // 気圧
    String data4 = SensorSerial.readStringUntil('\n'); // 温度
    data1.trim(); data2.trim(); data3.trim(); data4.trim();

    // アルファベットと空白をなくす処理
    int spaceIndex = data2.indexOf(' ');  // 空白の位置を探す
    String numStr = data2.substring(spaceIndex + 1);  
    float wind = numStr.toFloat(); 

    sigcount++;


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
    M5.Lcd.printf("%.2fm/s\n", wind);
    M5.Lcd.printf("\n%d\n", sigcount);
  }

  delay(100);
}
