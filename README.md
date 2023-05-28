## 概要
- UARDECSをESP32のWiFiで使えるように移植したライブラリです
- 2022/3/21時点でまだ作成途中です
- 実装の詳細はlib内のREADMEから

## 進捗状況(2022/5/28)
- とりあえず、ノードとして接続できることを確認しましたが、ESP32側から送信処理などを行うと落ちてしまいます
  - 処理落ちがESP32のライブラリ固有の問題か確認するために、"web_server_test.cpp"を追加しました。これはUECSと関係ないserverをたてるので、ESP32にアクセスしてgetのテストとかできます。
- MACアドレスの入力が必要なので、ESP32のWiFiのMACアドレス取得用スクリプトを別ファイルで追加しました

## ToDo

## 開発メモ
- ラズパイのUECSの確認
  - 192.168.