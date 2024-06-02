/////////////////////////////////////////
//UARDECS_ESP32 Sample Program
// by Yusaku Nakajima
// andotoecno@gmail.com

#include <EEPROM.h>
#include "UARDECS_esp32.h"

////////////////////////////WiFi setting
const char * WiFi_SSID = "";
const char * WiFi_PASS = "";

const char UECSdefaultIPAddress[] PROGMEM="192.168.102.7";
const byte default_ip[] = {192, 168, 102, 7};
const byte default_gateway[] = {192, 168, 102, 1};
const byte default_subnet[] = {255, 255, 255, 0};
const byte default_dns[] = {192, 168, 102, 1};

/////////////////////////////////////
//IP reset jupmer pin setting
/////////////////////////////////////
//Pin ID. This pin is pull-upped automatically.
const byte U_InitPin = 26;//このピンは変更可能です
const byte U_InitPin_Sense=LOW;
////////////////////////////////////
//Node basic infomation
///////////////////////////////////
const char U_name[] PROGMEM= "UARDECS-ESP32 Node v.1.0";
const char U_vender[] PROGMEM= "Andotoecno";
const char U_uecsid[] PROGMEM= "000000000000";
const char U_footnote[] PROGMEM= "UARDECS-ESP32 Sample Program";
char U_nodename[20] = "TestNode";
UECSOriginalAttribute U_orgAttribute;
//////////////////////////////////
// html page1 setting
//////////////////////////////////
const int U_HtmlLine = 1; //Total number of HTML table rows.
const char NAME[] PROGMEM= "Sensor";
const char UNIT[] PROGMEM= "";
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
CCMID_WaterVolume,
CCMID_EC,
CCMID_dummy,
};

const int U_MAX_CCM = CCMID_dummy;
UECSCCM U_ccmList[U_MAX_CCM];

const char ccmNameWaterVolume[] PROGMEM= "WaterVolumeNode";
const char ccmTypeWaterVolume[] PROGMEM= "WaterVolume.mMC";
const char ccmUnitWaterVolume[] PROGMEM= "";

const char ccmNameEC[] PROGMEM= "ECNode";
const char ccmTypeEC[] PROGMEM= "EC.mMC";
const char ccmUnitEC[] PROGMEM= "";

void UserInit(){
// ROOM,Region,Orderを設定
U_orgAttribute.room = 1;
U_orgAttribute.region = 1;
U_orgAttribute.order = 1;

//Set ccm list
//UECSsetCCM(送受信の区分,通し番号,CCM説明,Type,単位,priority,少数桁数,送信頻度設定[A_1S_0で1秒間隔])
UECSsetCCM(true, CCMID_WaterVolume, ccmNameWaterVolume, ccmTypeWaterVolume, ccmUnitWaterVolume, 29, 1, A_1S_0);
UECSsetCCM(true,  CCMID_EC      , ccmNameEC , ccmTypeEC , ccmUnitEC , 29, 2, A_1S_0);
}
void OnWebFormRecieved(){
}
void UserEverySecond(){
  // add value
  U_ccmList[CCMID_WaterVolume].value++;
  U_ccmList[CCMID_EC].value++;
  }
void UserEveryMinute(){
}
void UserEveryLoop(){
}
void loop(){
UECSloop();
}
void setup(){
EEPROM.begin(1024);// you need to call this function once
UECSsetup();
U_ccmList[CCMID_WaterVolume].value=100;
U_ccmList[CCMID_EC].value=1;

}