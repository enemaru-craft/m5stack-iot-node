# m5stack-iot-node

# デバイスID命名規則
M5-"セッションID(ユーザが設定)"-"発電モジュール名"-"固有番号"
    
    セッションIDはM5起動時に設定

    発電モジュール名は以下の通り
        太陽光-solar
        風力-wind
        地熱-geo
        水力-hydro
        手回し-hand
    
    固有番号は各デバイス個別の連番とする

# 起動方法
sessionID決定
↓
wifi接続
↓
httpリクエスト
↓
mqtt接続
↓
メインループ