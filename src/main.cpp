/////////////////////////////////////////
//UARDECS Sample Program for ADT7410
//Ver1.2
//By H.kurosaki 2015/12/10
//////////////////////////////////////////
//[概要]
//温度センサADT7410から温度を読み出してCCMに出力します
//[使い方]
//想定する動作環境はAruduinoUNO + Ethernet Shield2を標準としています
//MEGAでも動きますが、ピン番号の違いに注意して下さい
//○ArduinoIDE
//Ver1.7.2以降を使用します
//○使用するライブラリ
//UARDECS
//http://uecs.org/arduino/uardecs.html
//説明書に従ってインストールしてください
//
//○ADT7410の接続
//センサ→Arduino
//VDD→5V
//SCL→SCL(UNOではA5、MEGAではD20も可)
//SDA→SDA(UNOではA4、MEGAではD21も可)
//GND→GND
//
//○その他
//D3ピンがIPアドレスのリセット用に使用されています(変更可)

#include <EEPROM.h>
#include "UARDECS_esp32.h"


////////////////////////////WiFi setting
const char * WiFi_SSID = "Sense6YN";
const char * WiFi_PASS = "pokemon405";


/////////////////////////////////////
//IP reset jupmer pin setting
/////////////////////////////////////
//Pin ID. This pin is pull-upped automatically.
const byte U_InitPin = 26;//このピンは変更可能です
const byte U_InitPin_Sense=HIGH;
////////////////////////////////////
//Node basic infomation
///////////////////////////////////
const char U_name[] PROGMEM= "UARDECS Node v.1.0";
const char U_vender[] PROGMEM= "XXXXXX Co.";
const char U_uecsid[] PROGMEM= "000000000000";
const char U_footnote[] PROGMEM= "Test node";
char U_nodename[20] = "TestNode";
UECSOriginalAttribute U_orgAttribute;
//////////////////////////////////
// html page1 setting
//////////////////////////////////
const int U_HtmlLine = 1; //Total number of HTML table rows.
const char NAME[] PROGMEM= "Sensor";
const char UNIT[] PROGMEM= "";
const char NONES[] PROGMEM= "";
const char note1[] PROGMEM= "test";
signed long web_value;
const char** dummy = NULL;
struct UECSUserHtml U_html[U_HtmlLine]={
  {NAME, UECSINPUTDATA, UNIT, note1, dummy, 0, &(web_value),0,0,0},
};

//////////////////////////////////
// UserCCM setting
//////////////////////////////////
//define CCMID for identify
//CCMID_dummy must put on last
enum {
CCMID_SenserValue,
CCMID_cnd,
CCMID_dummy,
};

const int U_MAX_CCM = CCMID_dummy;
UECSCCM U_ccmList[U_MAX_CCM];

const char ccmNameSensor[] PROGMEM= "Sensor";
const char ccmTypeSensor[] PROGMEM= "Value";
const char ccmUnitSensor[] PROGMEM= "";

const char ccmNameCnd[] PROGMEM= "NodeCondition";
const char ccmTypeCnd[] PROGMEM= "cnd.mIC";
const char ccmUnitCnd[] PROGMEM= "";

void UserInit(){
//ESP32のMACアドレスを入力
U_orgAttribute.mac[0] = 0x7C;
U_orgAttribute.mac[1] = 0x9E;
U_orgAttribute.mac[2] = 0xBD;
U_orgAttribute.mac[3] = 0x91;
U_orgAttribute.mac[4] = 0x5C;
U_orgAttribute.mac[5] = 0x50;

//Set ccm list
UECSsetCCM(true, CCMID_SenserValue, ccmNameSensor, ccmTypeSensor, ccmUnitSensor, 29, 1, A_1S_0);
UECSsetCCM(true,  CCMID_cnd      , ccmNameCnd , ccmTypeCnd , ccmUnitCnd , 29, 0, A_10S_0);
}
void OnWebFormRecieved(){
}
void UserEverySecond(){
  // add value
  U_ccmList[CCMID_SenserValue].value++;
  // nothing error -> cnd=0
  U_ccmList[CCMID_cnd].value=0;
  }
void UserEveryMinute(){
}
void UserEveryLoop(){
}
void loop(){
UECSloop();
}
void setup(){
UECSsetup();
U_ccmList[CCMID_SenserValue].value=100;
}