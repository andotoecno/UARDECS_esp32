## 概要
- [UARDECS](https://uecs.org/arduino/uardecs.html)をESP32のWiFiで使えるように移植したライブラリです。使い方はUARDECSと同じです。
- 実装の詳細はlib内のREADMEから
- 2024/6/1時点で以下の動作確認済みです。
    - ブラウザからノードの設定を見に行き、IPアドレス等を更新。
    - UECSパケット送受信支援ツールでUDPが来ているのを確認。
    - UECS-GEARでテストデータの受信を確認。(サンプルのデータ受信は、RoomEst1.iniを使って確認できます。)
- MACアドレスの入力が必要なので、ESP32のWiFiのMACアドレス取得用スクリプトを別ファイルで追加しました(2023/5/28)
