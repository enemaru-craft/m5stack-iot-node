#include <Avatar.h>
#include <M5Unified.h>
#include <faces/FaceTemplates.hpp>
#include <DHT.h>

using namespace m5avatar;

// ==== DHT11 設定 ====
#define DHTPIN 22
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);
unsigned long lastUpdate = 0;  // 更新タイミング管理
const unsigned long updateInterval = 2000; // 2秒おき

Avatar avatar;

Face* faces[6];
const int num_faces = sizeof(faces) / sizeof(Face*);
int face_idx = 0;

const Expression expressions[] = {Expression::Angry, Expression::Sleepy,
                                  Expression::Happy, Expression::Sad,
                                  Expression::Doubt, Expression::Neutral};
const int num_expressions = sizeof(expressions) / sizeof(Expression);
int idx = 0;

ColorPalette* color_palettes[5];
const int num_palettes = sizeof(color_palettes) / sizeof(ColorPalette*);
int palette_idx = 0;

bool isShowingQR = false;

// ==== カスタムフェイス ====
class MyCustomFace : public Face {
   public:
    MyCustomFace()
        : Face(new UShapeMouth(44, 44, 0, 16), new BoundingRect(222, 160),
               new EllipseEye(32, 32, false), new BoundingRect(163, 64),
               new EllipseEye(32, 32, true), new BoundingRect(163, 256),
               new BowEyebrow(64, 20, false), new BoundingRect(163, 64),
               new BowEyebrow(64, 20, true), new BoundingRect(163, 256)) {}
};

void setup() {
    M5.begin();
    M5.Lcd.setBrightness(30);
    M5.Lcd.clear();

    dht.begin();  // DHT初期化

    faces[0] = avatar.getFace();
    faces[1] = new DoggyFace();
    faces[2] = new OmegaFace();
    faces[3] = new GirlyFace();
    faces[4] = new PinkDemonFace();
    faces[5] = new MyCustomFace();

    color_palettes[0] = new ColorPalette();
    color_palettes[1] = new ColorPalette();
    color_palettes[2] = new ColorPalette();
    color_palettes[3] = new ColorPalette();
    color_palettes[4] = new ColorPalette();
    color_palettes[1]->set(COLOR_PRIMARY, M5.Lcd.color24to16(0x383838));
    color_palettes[1]->set(COLOR_BACKGROUND, M5.Lcd.color24to16(0xfac2a8));
    color_palettes[1]->set(COLOR_SECONDARY, TFT_PINK);
    color_palettes[2]->set(COLOR_PRIMARY, TFT_YELLOW);
    color_palettes[2]->set(COLOR_BACKGROUND, TFT_DARKCYAN);
    color_palettes[3]->set(COLOR_PRIMARY, TFT_DARKGREY);
    color_palettes[3]->set(COLOR_BACKGROUND, TFT_WHITE);
    color_palettes[4]->set(COLOR_PRIMARY, TFT_RED);
    color_palettes[4]->set(COLOR_BACKGROUND, TFT_PINK);

    avatar.init(8);
    avatar.setColorPalette(*color_palettes[0]);

    // 温湿度表示用フォント設定
    M5.Lcd.setTextSize(2);
    M5.Lcd.setTextColor(GREEN);  // 白文字・黒背景
}

void loop() {
    M5.update();

    // ボタン処理
    if (M5.BtnA.wasPressed()) {
        avatar.setFace(faces[face_idx]);
        face_idx = (face_idx + 1) % num_faces;
    }
    if (M5.BtnB.wasPressed()) {
        avatar.setColorPalette(*color_palettes[palette_idx]);
        palette_idx = (palette_idx + 1) % num_palettes;
    }
    if (M5.BtnC.wasPressed()) {
        avatar.setExpression(expressions[idx]);
        idx = (idx + 1) % num_expressions;
    }

    // ==== 温湿度表示 ====
    if (millis() - lastUpdate > updateInterval) {
        float temp = dht.readTemperature();
        float humi = dht.readHumidity();

        if (!isnan(temp) && !isnan(humi)) {
            char buf[32];
            snprintf(buf, sizeof(buf), "Temp: %.1f C", temp);
            M5.Lcd.fillRect(0, 0, 160, 20, BLACK); // 前の表示をクリア
            M5.Lcd.setCursor(0, 0);
            M5.Lcd.print(buf);

            snprintf(buf, sizeof(buf), "Humi: %.1f %%", humi);
            M5.Lcd.fillRect(0, 20, 160, 20, BLACK); // 前の表示をクリア
            M5.Lcd.setCursor(0, 20);
            M5.Lcd.print(buf);
        }

        lastUpdate = millis();
    }

    //avatar.update();
}
