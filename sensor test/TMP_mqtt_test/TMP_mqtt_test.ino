#include <M5Stack.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <Wire.h>

// DS18B20のデータピン
#define ONE_WIRE_BUS 26

// 最大保存数
#define MAX_DATA_POINTS 100

// 温度・湿度履歴保存用
float tempHistory[MAX_DATA_POINTS];
int dataCount = 0;

// WiFI接続関連
const char* ssid = "TsUki"; //WiFIのSSIDを入力
const char* pass = "yuki2995"; // WiFiのパスワードを入力

// AWS IoTエンドポイント
const char* mqtt_server = "an18mjyngp4x1-ats.iot.us-east-1.amazonaws.com";
const int mqtt_port = 8883;
const char* mqtt_topic = "mqtt_test/test";

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// 表示モード
enum DisplayMode {
  MODE_NORMAL,
  MODE_TEMP_GRAPH
};
DisplayMode mode = MODE_NORMAL;

//エラーカウント
int ErrorCount = 0;

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

const char* client_id = "device-M5-001";

void connectAWS() {
  net.setCACert(ca_cert);
  net.setCertificate(client_cert);
  net.setPrivateKey(client_key);

  while (!client.connected()){
    M5.Lcd.print("Connecting to MQTT...");
    if (client.connect("M5StackClient")) {
      M5.Lcd.println("connected");
    } else {
      M5.Lcd.print("failed, rc=");
      M5.Lcd.println(client.state());
      delay(2000);
    }
  }
}


void setup() {
  M5.begin();
  Wire.begin();
  Serial.begin(115200);
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(0, 0);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED){
    delay(500);
    M5.Lcd.print(".");
  }
  M5.Lcd.println("WiFi connected");

  client.setServer(mqtt_server, mqtt_port);

  connectAWS();

  delay(5000);
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(0, 0);
  M5.Lcd.println("DS18B20 Temp");

  sensors.begin(); // センサ初期化
}

void loop() {
  if (!client.connected()){
    connectAWS();
  }
  client.loop();

  sensors.requestTemperatures(); // 温度リクエスト
  float tempC = sensors.getTempCByIndex(0); // 最初のセンサの温度を取得

  time_t now = time(nullptr); //UNIXタイム取得

  // 表示更新
  if (tempC == DEVICE_DISCONNECTED_C) {
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.fillRect(0, 40, 320, 40, BLACK);
    M5.Lcd.println("Sensor Error!");
  } else {
    M5.Lcd.setCursor(0, 40);
    M5.Lcd.fillRect(0, 40, 320, 40, BLACK);
    M5.Lcd.printf("Temp: %.2f C", tempC);

    // 温度をJSON形式で送信
    char payload[128];
    snprintf(payload, sizeof(payload), "{\"client_id\":\"%s\",\"timestamp\":%ld,\"temp\":%.2f}",
      client_id, now, tempC);
    client.publish(mqtt_topic, payload);

    Serial.println(payload);
}

  delay(10000);
}
