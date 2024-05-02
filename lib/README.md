## 前提条件
- ここで参考にしたライブラリおよび説明書はUARDECS ver2.0.0です。
- Megaじゃない方なので、CCMサーチの応答は未実装です
- 以下移植にあたってのメモ書きです

## Ethernetライブラリの解説
**IPアドレスの設定**
  - 基本はノードのweb pageにブラウザでアクセスして書き換える
    - 書き換えた値は内臓eepromに保存
  - 初期状態だとip分からなくてアクセスできないので、あらかじめ設定しておいたジャンパピンをHighにするとIP:192.168.1.7, subnet:255.255.255.0に強制設定される(ライブラリではSafeModeと呼ぶ)のでそこにアクセスして書き換え
  - 詳細は説明書の33pなど参照

**Ethernetクラスについて**
- ノードのwebpage表示(ステータスとip等の設定)で使われている
- 基本はWiFi.config()とWiFi.begin()で置き換え
Ethernetライブラリ参照コード
```
if(U_orgAttribute.status&STATUS_SAFEMODE)
  	{
  	byte defip[]     = {192,168,1,7};
  	//byte defsubnet[] = {255,255,255,0};
  	byte defdummy[] = {0,0,0,0};
  	Ethernet.begin(U_orgAttribute.mac, defip, defdummy,defdummy,defdummy);
  	}
  	else
    {
    Ethernet.begin(U_orgAttribute.mac, U_orgAttribute.ip, U_orgAttribute.dns, U_orgAttribute.gateway, U_orgAttribute.subnet);
    }
```
  - 説明書によると、DNS、gatewayは使っていないとのこと
    - DNSはローカルネットワークだから不要だし、gatewayは直接IP指定して通信するため未使用だと思われる
  - safeモードで起動した時(if内)にsubnetが0.0.0.0になっているけど大丈夫なのだろうか？
    - defsubnetの255.255.255.0使うべきでは？
    - サブネットマスクとは(復習)
      - https://ascii.jp/elem/000/000/562/562310/

**復習：ブロードキャスト、ユニキャスト、マルチキャスト**
  - 参考：https://lang-ship.com/blog/work/esp32-asyncudp/
  - ユニキャスト：ipとポートを指定して送信する
  - ブロードキャスト：ポートだけを指定して送信する。ipはホスト部を255にしたブロードキャストipにする。また、MACアドレスはFF:FF:FF:FFにする(Bluetoothの場合のみでWiFiやEthernetはこれいらないかも)。
    - 実装上は普通のread writeがユニキャストで、送信側のみブロードキャスト用関数が実装されている場合がある(ブロードキャスト用関数がない場合はホスト部のみ255にして送れば同じこと)
  - マルチキャスト
    - ある特定のIPでlistenしている相手だけに送信する
    - 事前にマルチキャスト用のIPを決めておいて、そのIPを受信側に伝えておく必要がある
      - ブロードキャストと違って、無差別に送らないけどマルチキャストIPを知っている受信側には一斉送信できる点で便利

**復習：Etheret UDPの送信/受信手順**
- 事前準備
  - Ethernet.begin(ip,port)
  - UDP.begin(port)
  - 以上のインスタンスを作成
- 送信
  - UDP.beginPacket(ip,port) -> UDP.write(data) -> UDP.endPacket()返り値が通信成功のtrue/false
- 受信
  - UDP.paesePacket()で受信データのバイト数を確認 -> UDP.read(buffer,size)で中身読む
    - UDP.paesePacket()での受信確認後であれば、UDP.remoteIP()で接続先のIPも確認可能

**Ethernet UDPを使った内部実装について**
- 16520と16529の2つのポートについての実装
  - 16520(データポート)
    - UECSupdate16520portReceive()
      - PC等から送ったCCMの受信とweb pageでのステータス更新
      - 受信時にリモートのIPの指定なし
    - UECSCreateCCMPacketAndSend()
      - PC等へのセンサー値などの送信
      - 送信時はブロードキャスト(らしい)
        - EthernetUDPライブラリはブロードキャスト用の関数がないので、ホスト部を255に変更していると思われるが、実装を読み解いても正直分からなかった
        - 説明書などを読むと、ノードからのCCMの送信がブロードキャストっぽいことがほのめかされている
  - 16529(スキャンポート)
    - UECSupdate16529port()
      - ノード情報の受信からの送信
        - 受信時にリモートのIPの指定なし
        - 送信時はユニキャスト

## Ethernet to Wifiに伴う書き換え
- 抽象化されているstrictなどは残して、Ethernetに関する部分だけ書き換えたい

- Ethernet server,client → WiFi server,clientの移行
  - ログ用のサーバ
  - ノードに直接アクセスしたときにHTML等表示
  - WiFiライブラリに置き換えてメソッドはそのまま使用できた
  
- EthernetUDP → AsyncUDPの移行
  - reference
    - Ethernetリファレンス
      - https://garretlab.web.fc2.com/arduino_reference/libraries/standard_libraries/Ethernet/EthernetUDP/
    - EthernetUDP.persePacket()
      - https://nobita-rx7.hatenablog.com/entry/28471040
    - Async UDPリファレンス(M5Stack用を参照)
      - https://lang-ship.com/reference/unofficial/M5StickC/Class/ESP32/AsyncUDP/
    - Async UDP 実装
      - https://lang-ship.com/blog/work/esp32-asyncudp/#toc24
      - https://qiita.com/yomori/items/3e02bec35ad1262b7b94
      - https://program4ptotat.livedoor.blog/archives/16398801.html

  - ライブラリ内で使用しているUDP   
    - UDP16520→データ送信用のポート、メイン
    - UDP16529→スキャンポート？
  - メソッドの対応関係
    - Ethernet：begin(port)
      - Async UDPだと不要なので削除
    - 送信
      - Ethernet：beginPacket(ip,port) -> write(buf) -> endPacket()
      - Async UDP ：writeTo(buf,buf_size,ip,port)
        - UECSbufferとwriteToの型が合わないのでcastした
    - 受信
      - Ethernet：persePacket() ->  read(buf,buf_size),remoyeIP()
        - 恐らくEthernetライブラリだとportをbeginで指定している
      - Async UDP：listen(port) ->  onPacket(callback func)
        - ここでコールバック関数を書く必要があるので、コールバック関数内で外部の変数をとりこむのどうやるのか困った
          - Arduinoはラムダ式使えるので、ラムダ式実装して取り込んだ
          - 参考：https://qiita.com/dojyorin/items/bce720509be0486b9b79
        - また、UECSbufferとreadの型が合わないのでcastした
    - (stop)
      - 元実装だとコメントアウトで未使用だけど、一応Async UDPに存在するメソッドのcloseに置き換え
  - Ethenet.begin() → WiFi.config(), WiFi.begin()

## Arduino to ESP32に伴う仕様変更
- EEPROM
  - EEPROMの保存領域確保にbegin(確保したいバイト数)が必要
  - EEPROMのread/write関数は同じであるが、ESP32はwriteの後にcommit関数の実行が必要


## web pageの仕組み
**HTTPcheckRequest()**
- アクセス時(server.available())に、クエリに応じて表示するページを選択している
- 存在しないページであれば `Error!`を表示


##　Debug
- 2023/3/21
- webページ上の`send`アイコンのクリック時に`GET/1HTTP/1.1`が送られるはずが、`GET/favicon.icoHTTP/1.1`が送られている
  - faviconはお気に入り登録を押した時のクエリのようで、ウェブページ上のアイコンを押した時の挙動として、ESP32のライブラリの仕様なのか連続で押すとなぜかfaviconになっている
  - これは現在のブラウザの仕様で、何度かクリックするとfaviconがでる仕様らしい
  - ESP32での回避についてはここが参考になった
    - https://www.mgo-tec.com/blog-entry-esp32-mjpeg-bmp.html/3

##　ToDo
- WIFiのSSID,passもノードのwebpageのアクセスして設定
- 受診失敗時のリセット
- デフォルトipはユーザーが設定できるように