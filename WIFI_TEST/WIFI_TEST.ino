#include <WiFi.h>
#include <WiFiUDP.h>
#include <M5Stack.h>

// arp -aでIPアドレスをすべて取得
// ipconfigで自身のIPアドレスを取得

const char ssid[] = "TsUki"; //WiFIのSSIDを入力
const char pass[] = "yuki2995"; // WiFiのパスワードを入力

WiFiUDP wifiUdp; 
const char *pc_addr = "10.177.251.164";  //"192.168.0.6";
const int pc_port = 50007; //送信先のポート
const int my_port = 50008;  //自身のポート


void setup() {
  M5.begin();
  M5.Power.begin();
  M5.Lcd.setTextSize(2);

  WiFi.begin(ssid, pass);
  while( WiFi.status() != WL_CONNECTED) {
    delay(500); 
    M5.Lcd.print("."); 
  }  
  M5.Lcd.println("WiFi connected");
  M5.Lcd.print("IP address = ");
  M5.Lcd.println(WiFi.localIP());
  
  wifiUdp.begin(my_port);
}

void loop(){
  for(int i=0;i<100;i++){
  // データ送信
  wifiUdp.beginPacket(pc_addr, pc_port);
  wifiUdp.write('t');
  wifiUdp.write('e');
  wifiUdp.write('s');
  wifiUdp.write('t');
  wifiUdp.endPacket();
  M5.Lcd.print(".");


  delay(2000); //2秒待つ
  }

  //終了コマンド送信
  wifiUdp.beginPacket(pc_addr, pc_port);
  wifiUdp.write('&');
  wifiUdp.endPacket();
  delay(2000);
}