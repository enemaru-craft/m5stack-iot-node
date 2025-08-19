#include <M5Stack.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <TJpg_Decoder.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>

// 以下は画像C配列をインクルード
#include "pic1.h"
#include "pic2.h"
#include "pic3.h"

// GPIOデータピン
#define ONE_WIRE_BUS 26

// WiFI接続関連
const char* ssid = "TsUki"; //WiFIのSSIDを入力
const char* pass = "yuki2995"; // WiFiのパスワードを入力

// AWS IoTエンドポイント
const char* mqtt_server = "an18mjyngp4x1-ats.iot.us-east-1.amazonaws.com";
const int mqtt_port = 8883;
const char* mqtt_topic = "mqtt_test/test";

// センサ
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// データ最大保存数
#define MAX_DATA_POINTS 100

// 温度保存用配列
float tempHistory[MAX_DATA_POINTS];
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
float temperature = 30.0;
int pm = 0;

//通信カウント
int SigCount = 0;

const char* ca_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n" \
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n" \
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n" \
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n" \
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n" \
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n" \
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n" \
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n" \
"VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n" \
"93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n" \
"jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n" \
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n" \
"A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n" \
"U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n" \
"N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n" \
"o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n" \
"5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n" \
"rqXRfboQnoZsG4q5WTP468SQvvG5\n" \
"-----END CERTIFICATE-----\n";

const char* client_cert = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDWTCCAkGgAwIBAgIUS887WtTNSTuJiF1zltHKcxcs6+owDQYJKoZIhvcNAQEL\n" \
"BQAwTTFLMEkGA1UECwxCQW1hem9uIFdlYiBTZXJ2aWNlcyBPPUFtYXpvbi5jb20g\n" \
"SW5jLiBMPVNlYXR0bGUgU1Q9V2FzaGluZ3RvbiBDPVVTMB4XDTI1MDgwNDEyMzEy\n" \
"N1oXDTQ5MTIzMTIzNTk1OVowHjEcMBoGA1UEAwwTQVdTIElvVCBDZXJ0aWZpY2F0\n" \
"ZTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMhWW8fJ4vPhDar5/68J\n" \
"qBCcQv4nPB9GUqdjSWPK38jn94fsFO4llf+ZGbhTtXGLicBPlzOKw8Cljwh1d8cb\n" \
"Y4ME72NrG0stvDsAKAlFBm3395yjArms11eSdNdJFkt/p2yQDlclt0DKHWDe6T5i\n" \
"Q/G6jbGRLi1QVmIt0UGbGNbnwa0f4TY5JrqdVWX+ka/jr0QbUbAcfMm9eiZ6xRF4\n" \
"q45zy+qjOW2K5YZvrAdkAy98ryYDV6gR3UptLwVJzBofs+SmPXVDlkjX0ym08ETS\n" \
"X6Mx7/jP1K5JusRrhZ+lAcPGOaLEoh+HZvhWruiGxD6MCv10Mdow0BWy/EOqQIgi\n" \
"fW8CAwEAAaNgMF4wHwYDVR0jBBgwFoAUq6ltm+k7QLqSiMzbtfDobSiZbLAwHQYD\n" \
"VR0OBBYEFFpA9XdJ7jsnZF/aE3F+rslfeQEsMAwGA1UdEwEB/wQCMAAwDgYDVR0P\n" \
"AQH/BAQDAgeAMA0GCSqGSIb3DQEBCwUAA4IBAQCpWLMJwtMMyYh2wTlQEwj4dN9q\n" \
"x+jEmE9/tnSoq7Ddy7i5bBstIvHxj2oV/pO90az2gk7/j+OrqL7lnhSFRYMj6aKi\n" \
"U+4rr38//Z1seTbwvx32gax3ikN3P0J8/+kY83X4Utsrngc+YcyrRcOXlzUAKPrd\n" \
"0tETq7uq3GV3TZ2y3b5tPzYb8Z39J+9kpcpABmOi9j2PX9jKCbQfXkXqLNigHZEw\n" \
"HeljANoRRcf9UR8Si/PExDSVmLcyUVDsloD4HV6VHI8C4tJRHVQvN9dWYxf76za9\n" \
"PMD7j1+Wysuy0/KB1+XkmT5Nl4h2OdH/DnW86d2n75gLP6lBW9HU1jIwD/4a\n" \
"-----END CERTIFICATE-----\n";

const char* client_key = \
"-----BEGIN RSA PRIVATE KEY-----\n" \
"MIIEowIBAAKCAQEAyFZbx8ni8+ENqvn/rwmoEJxC/ic8H0ZSp2NJY8rfyOf3h+wU\n" \
"7iWV/5kZuFO1cYuJwE+XM4rDwKWPCHV3xxtjgwTvY2sbSy28OwAoCUUGbff3nKMC\n" \
"uazXV5J010kWS3+nbJAOVyW3QModYN7pPmJD8bqNsZEuLVBWYi3RQZsY1ufBrR/h\n" \
"Njkmup1VZf6Rr+OvRBtRsBx8yb16JnrFEXirjnPL6qM5bYrlhm+sB2QDL3yvJgNX\n" \
"qBHdSm0vBUnMGh+z5KY9dUOWSNfTKbTwRNJfozHv+M/Urkm6xGuFn6UBw8Y5osSi\n" \
"H4dm+Fau6IbEPowK/XQx2jDQFbL8Q6pAiCJ9bwIDAQABAoIBAC1jY2fgVYrrci1K\n" \
"dqMxjOI2hPcP7I8pX5CM7hOt4wWhiHNDCXrrCRDfnvQkAve2pX7siVUkGzWKTN2d\n" \
"v4Vcry7/7zyXB5gAxKSZZHi1Kr+/bloOlI98mU094TrlVCfGxCfUe2tDIEGNiRpp\n" \
"Zm671KjvmyqfudaKXQfhWYeDnB0T/bArHMue9ZlFHUOYugnKF6TxVjCm5GTgxB8E\n" \
"pY45+STlZBDJX839hdP0Cs7Gblrt8MESLatnGhIEyjRV4eUTqhl0o9NgHqVqQnQl\n" \
"bBajoF25h0medbjZcAnngorin+Fn3Y5qYK/mx5ico6CrIAgYZD27zsZitutbeNju\n" \
"lch18AECgYEA+d4wsXMJXxT+/MH/v8Xjv8yU5yo/9E2wwOtXavMZjppMJ87Lhwoj\n" \
"ePsV+EQuB5WthtBdVyJPv7xzXQpMGxGRsS+J99iNwPtgDK7/lB3HjdRLyLA1YttV\n" \
"f07yzolE4PBVcpibRweJMEK5WgrpgSzOEGjllJ+N1mhTigEJRlub+N0CgYEAzUD9\n" \
"TtGayxddZ/BOTxyv6SOCFhMnNGtotNIoylhPiamyhWGuCA3uznMuIymIynlSRDaz\n" \
"in7Hic+mkzPRK5pj0WC3565MJKfMDa3iC6svY3Co014FfY9h30NpDWQUWz087u5w\n" \
"RJJY19LIbkqZ8IfxE5QBKDnIifyBB2z7wg1tRLsCgYEApcAAqRXYd5xj8aN3Ve2d\n" \
"wNOjSx05w4Pt5mu3V5jfsTslnZqOZIwTL+PPlRsVNa7Sp4V9BZobMON/245DIgmr\n" \
"jdAtQ54Bmqyl8IP9wnFmuOzFQOGy6Fr7AjKK2vswF88Vv6kACEQ1ezAn6FUoxKnG\n" \
"IBiVgtu8qSgXkopRbrzCYHkCgYB84Jp9Ften0ppYv4LVbS21mY3u4k5K/UJiKS76\n" \
"FQqnReDAd/KLIfb2EmuypvV7wp6DtAg+Le428Wh2TONpNPCgFckuIwRelNx4Qh+y\n" \
"W79kL8n3d0+92buBO6ExrX8ZF8P31ofloGh/PtYFDExmZD8PW0/s6mMRWNgj7XGq\n" \
"2cJuOwKBgGOVlwnInBiOOq8Z+j+M1yT3slLuqgWhzhIpj9eNxHvONGBdPnZ4199L\n" \
"ZrJMUmQTgsSunIML6tUosA1jE3JOmpd7AFY3artwmGbXNybSNOvEiSviy+ZoVrsm\n" \
"bnZIQLB7QCj1S6v2PjjWJtyYrHEZ35C4qAjTfqanDAanwHyjMhv1\n" \
"-----END RSA PRIVATE KEY-----\n";

WiFiClientSecure net;
PubSubClient client(net);

// デバイスID
const char* client_id = "device-M5-";

// AWS接続
void connectAWS() {
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 0);
  net.setCACert(ca_cert);
  net.setCertificate(client_cert);
  net.setPrivateKey(client_key);

  while (!client.connected()){
    M5.Lcd.println("Connecting to MQTT...");
    if (client.connect("M5StackClient")) {
      M5.Lcd.setTextColor(WHITE, GREEN);
      M5.Lcd.println("connected");
    } else {
      M5.Lcd.setTextColor(WHITE, BLACK);
      M5.Lcd.print("failed, rc=");
      M5.Lcd.println(client.state());
      delay(2000);
    }
  }
}

// セットアップ
void setup() {
  M5.begin();
  Wire.begin();
  Serial.begin(115200);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.setTextColor(WHITE, BLACK);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    M5.Lcd.print(".");
  }
  M5.Lcd.println(" ");
  M5.Lcd.setTextColor(WHITE, GREEN);
  M5.Lcd.println("WiFi connected");
  M5.Lcd.setTextColor(WHITE, BLACK);
  delay(1000);

  client.setServer(mqtt_server, mqtt_port);

  connectAWS();
  delay(2000);

  // センサ初期化
  sensors.begin();

  // TJpg_Decoder 初期化
  TJpgDec.setJpgScale(2);          // 1/2倍
  TJpgDec.setSwapBytes(true);      // エンディアン調整
  TJpgDec.setCallback(tft_output); // 出力関数登録
}

// メイン
void loop() {
  M5.update();

  sensors.requestTemperatures(); // 温度リクエスト
  float temperature = sensors.getTempCByIndex(0); // センサの温度を取得

  if (!client.connected()){
    connectAWS();
  }
  client.loop();

  time_t now = time(nullptr); // UNIXタイム取得

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

    if (SigCount < 15) //15秒に一回送信
      SigCount++;
    else{
      // 温度をJSON形式で送信
      char payload[128];
      snprintf(payload, sizeof(payload), "{\"client_id\":\"%s\",\"timestamp\":%ld,\"temp\":%.2f}",client_id, now, tempC);
      client.publish(mqtt_topic, payload);

      SigCount = 0; //SigCountをリセット
    }

    // 表示モードに応じた描画
    switch (mode) {
      case MODE_NORMAL:
        showTemp(temperature);
        break;
      case MODE_TEMP_GRAPH:
        drawGraph(tempHistory, dataCount, "Temp Graph", "C", PINK);
        break;
    }
  } else
    // エラー表示
    ErrorView();

  // 遅延時間
  delay(1000); // 1秒
}

//データ表示
void showTemp(float temperature) {
  M5.Lcd.fillScreen(BLACK);

  drawUI(temperature);
  M5.Lcd.setCursor(15,140);
  M5.Lcd.setTextSize(5);
  M5.Lcd.setTextColor(WHITE);
  M5.Lcd.printf("%.1f C\n", temperature);
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

  // グラフの線を描画
  for (int i = 1; i < count; i++) {
    int x0 = map(i - 1, 0, MAX_DATA_POINTS - 1, 10, 310);
    int y0 = map(data[i - 1], minVal, maxVal, 190, 40);
    int x1 = map(i, 0, MAX_DATA_POINTS - 1, 10, 310);
    int y1 = map(data[i], minVal, maxVal, 190, 40);
    M5.Lcd.drawLine(x0, y0, x1, y1, color);
  }

  // 最新のデータ値を表示（点＋数値）
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

  drawButton();
}

//グラフの最大最小点を出す
float roundToNerest5(float value) {
  return round(value / 5.0) * 5.0;
}

// 顔、線などのUIを描画
void drawUI(float tmp) {
  // データ表示の背景を描画
  M5.Lcd.fillRect(0, 240-120, 320, 90, 0x8410);
  M5.Lcd.fillRect(0, 240-122, 320, 4, WHITE);
  M5.Lcd.fillRect(0, 240-38, 320, 4, WHITE);

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
  M5.Lcd.fillRect(0, y, 320, 30, 0x8410);

  // Aボタン（温度表示）
  M5.Lcd.setCursor(40, y + 6);
  M5.Lcd.print("TEMP");

  // Bボタン（グラフ表示）
  M5.Lcd.setCursor(130, y + 6);
  M5.Lcd.print("GRAPH");

  // Cボタン（未使用）
  M5.Lcd.setCursor(222, y + 6);
  M5.Lcd.print("     ");
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