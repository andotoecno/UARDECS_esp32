/*
  Uardecs.h - Library for UECS 


  Ken-ichiro Yasuba 2013.
  updated by Hideto Kurosaki 2022.4.

*/

#ifndef Uardecs_h
#define Uardecs_h


#include <Ethernet2.h>
#include <EthernetUdp2.h>
#include <WiFi.h>

#include <Arduino.h>
#include <SPI.h>   
#include <EEPROM.h>
#include <stdio.h>
// #include <avr/pgmspace.h>


#define CHOICES(s) sizeof(s)/sizeof(s[0])


//############################################################
//############################################################

#define MAX_TYPE_CHAR 20
#define MAX_CCMTYPESIZE 19
	
#define MAX_IPADDRESS_NUMBER 4
#define MAX_ATTR_NUMBER 4
#define AT_ROOM		0
#define AT_REGI		1
#define AT_ORDE		2
#define AT_PRIO		3

#define MAX_DIGIT		16
	
	
/********************************/
/* ROM CHAR *********************/
/********************************/
const char UECSdefaultIPAddress[] PROGMEM="192.168.1.7";

const char UECSccm_XMLHEADER[] PROGMEM="<?xml version=\"1.0\"?><UECS";
const char UECSccm_UECSVER_E10[] PROGMEM=" ver=\"1.00-E10\">";
const char UECSccm_DATATYPE[] PROGMEM = "<DATA type=\""; //12 words
const char UECSccm_ROOMTXT[] PROGMEM="\" room=\""; // 8 words
const char UECSccm_REGIONTXT[] PROGMEM="\" region=\"";  // 10 words    
const char UECSccm_ORDERTXT[] PROGMEM="\" order=\""; // 9 words
const char UECSccm_PRIORITYTXT[] PROGMEM="\" priority=\""; //12 words 
const char UECSccm_CLOSETAG[] PROGMEM="\">";
const char UECSccm_IPTAG[] PROGMEM="</DATA><IP>"; // 11 words
const char UECSccm_FOOTER0[] PROGMEM="</UECS>"; // 12 words
const char UECSccm_FOOTER1[] PROGMEM="</IP></UECS>"; // 12 words


const char UECS_NONE[] PROGMEM="NONE";
const char UECS_A1S0[] PROGMEM="A_1S_0";
const char UECS_A1S1[] PROGMEM="A_1S_1";
const char UECS_A10S0[] PROGMEM="A_10S_0";
const char UECS_A10S1[] PROGMEM="A_10S_1";
const char UECS_A1M0[] PROGMEM="A_1M_0";
const char UECS_A1M1[] PROGMEM="A_1M_1";
const char UECS_S1S[] PROGMEM="S_1S_0";
const char UECS_S1M[] PROGMEM="S_1M_0";
const char UECS_B0_[] PROGMEM="B_0";
const char UECS_B1_[] PROGMEM="B_1";

const char UECSccm_NODESCAN1[] PROGMEM="<NODESCAN/></UECS>";
const char UECSccm_NODESCAN2[] PROGMEM="></NODESCAN></UECS>";

const char UECSccm_NODENAME[] PROGMEM="<NODE><NAME>"; // 12 words
const char UECSccm_VENDER[] PROGMEM="</NAME><VENDER>"; // 15 words
const char UECSccm_UECSID[] PROGMEM="</VENDER><UECSID>"; // 17 words 
const char UECSccm_UECSID_IP[] PROGMEM="</UECSID><IP>"; // 13 words
const char UECSccm_MAC[] PROGMEM="</IP><MAC>"; // 10 words
const char UECSccm_NODECLOSE[] PROGMEM="</MAC></NODE></UECS>"; // 20 words


const char UECSccm_CCMSCAN[] PROGMEM = "<CCMSCAN";  // 8 words
const char UECSccm_PAGE[] PROGMEM = " page=\""; // 7 words
const char UECSccm_CCMSCANCLOSE0[] PROGMEM = "/></UECS>";  // 9 words
const char UECSccm_CCMSCANCLOSE2[] PROGMEM="\"/></UECS>"; // 10 words
const char UECSccm_CCMSCANCLOSE1[] PROGMEM = "></CCMSCAN></UECS>"; //18words

const char UECSccm_CCMNUM[] PROGMEM="<CCMNUM page=\""; // 14 words
const char UECSccm_TOTAL[] PROGMEM="\" total=\""; // 9 words
const char UECSccm_CCMNO[] PROGMEM="</CCMNUM><CCM No=\""; // 18 words 
const char UECSccm_CAST[] PROGMEM="\" cast=\""; // 8 words 
const char UECSccm_UNIT[] PROGMEM="\" unit=\""; // 8 words
const char UECSccm_SR[] PROGMEM="\" SR=\""; // 6 words
const char UECSccm_LV[] PROGMEM="\" LV=\""; // 6 words
const char UECSccm_CCMRESCLOSE[] PROGMEM="</CCM></UECS>";


  
/********************************/
/* Original Attribute ***********/
/********************************/
struct UECSOriginalAttribute {
  char nodename[21]; // fix
  char vender[21];   // fix
  char uecsid[21];   // fix
  IPAddress ip;
  byte subnet[4];
  byte gateway[4];
  byte dns[4];

  byte mac[6];
  unsigned short room;
  unsigned short region;
  unsigned short order;

  unsigned char flags;
  unsigned char status;
  
};

//for U_orgAttribute.flags
//#define ATTRFLAG_ALLOW_ABRIDGE_TYPE	1	//In recieved CCM type,Ignore attribute code after dot like ".mIC"
#define ATTRFLAG_LOOSELY_VERCHECK	2	//loose version judgment of CCM packet


//for U_orgAttribute.status
#define STATUS_SAFEMODE				1		//safemode jumper was enabled when startup
#define STATUS_MEMORY_LEAK			2		//Memory leak alert
#define STATUS_NEEDRESET			4		//Please push reset button

#define STATUS_SAFEMODE_MASK		254		//safemode Release

/********************************/
/* CCM    ***********************/
/********************************/
struct UECSCCM{
  boolean sender;              // fix
  const char * name;    // fix
  const char * type;    // fix 
  const char * unit;    // fix
  unsigned char decimal;       // fix   
  char ccmLevel;        // A_1S_0 etc. fix
  signed short attribute[4];
  signed short baseAttribute[3];
  signed long value;
//  signed long old_value;
  boolean validity;
  unsigned long recmillis;
  IPAddress address;
  boolean flagStimeRfirst;
  unsigned char flags;
};


/********************************/
/* TEMPCCM    *******************/
/********************************/
struct UECSTEMPCCM{
  char type[MAX_TYPE_CHAR];     
  unsigned char decimal;       // fix   
  signed short attribute[4];
  signed short baseAttribute[3];
  signed long value;
  IPAddress address;
};

boolean UECSparseRec(struct UECSTEMPCCM *_tempCCM,int *matchCCMID);
void UECSCreateCCMPacketAndSend(struct UECSCCM* _ccm);
void UECSupRecCCM(UECSCCM* _ccm, UECSTEMPCCM* _ccmRec);
void UECScheckUpDate(UECSTEMPCCM* _tempCCM, unsigned long _time,int startid);
boolean UECSresNodeScan();
void UECSautomaticSendManager();
void UECSautomaticValidManager(unsigned long td);


//############################################################
//############################################################

#define UECSSHOWDATA 0
#define UECSINPUTDATA 1
#define UECSSELECTDATA 2
#define UECSSHOWSTRING 3

#define NONE -1
#define A_1S_0 0
#define A_1S_1 1
#define A_10S_0 2
#define A_10S_1 3
#define A_1M_0 4
#define A_1M_1 5
#define S_1S_0 6
#define S_1M_0 7
#define B_0 8
#define B_1 9
	
#define BUF_SIZE 300
#define BUF_HTTP_REFRESH_SIZE 279  //BUF_SIZE-(MAX_TYPE_CHAR+1)

//EEPROM 1k
	#define EEPROM_OFFSET_DATATOP	800
	#define EEPROM_OFFSET_IP		800
	#define EEPROM_OFFSET_SUBNET	804
	#define EEPROM_OFFSET_GATEWAY	808
	#define EEPROM_OFFSET_DNS		812
	#define EEPROM_OFFSET_ROOM		816
	#define EEPROM_OFFSET_REGION	817
	#define EEPROM_OFFSET_ORDER_L	818
	#define EEPROM_OFFSET_ORDER_H	819
	#define EEPROM_OFFSET_NODENAME	825
	#define EEPROM_OFFSET_WEBDATA	850
	#define EEPROM_OFFSET_DATAEND	1023



const char UECShttpHead200_OK[] PROGMEM="HTTP/1.1 200 OK\r\n";
const char UECShttpHeadContentType[] PROGMEM="Content-Type: text/html\r\n";
const char UECShttpHeadConnection[] PROGMEM="Connection: close\r\n\r\n";

const char UECShtmlTABLECLOSE[] PROGMEM="</TBODY></TABLE>"; // 16 words
const char UECShtmlRETURNINDEX[] PROGMEM="<P align=\"center\">return<A href=\".\">Top</A></P>";
const char UECSbtrag[] PROGMEM = "<br>";
const char UECShtmlLAN2[] PROGMEM = "<form action=\"./2\"><p>"; // 37words
const char UECShtmlLAN3A[] PROGMEM ="address:"; //9 words
const char UECShtmlLAN3B[] PROGMEM ="subnet:";
const char UECShtmlLAN3C[] PROGMEM ="gateway:";
const char UECShtmlLAN3D[] PROGMEM ="dns:";
const char UECShtmlLAN3E[] PROGMEM ="mac:";
const char UECShtmlRoom[] PROGMEM ="room:"; //8 words
const char UECShtmlRegion[] PROGMEM ="region:";
const char UECShtmlOrder[] PROGMEM ="order:";
const char UECShtmlUECSID[] PROGMEM ="uecsid:";
const char UECShtmlLANTITLE[] PROGMEM = "LAN";
const char UECShtmlUECSTITLE[] PROGMEM = "UECS";
const char UECShtmlNAMETITLE[] PROGMEM = "Node Name";

const char UECShtmlATTENTION_RESET[] PROGMEM = "Please push reset button.";

const char UECShtmlATTENTION_SAFEMODE[] PROGMEM = "[SafeMode]";
const char UECShtmlATTENTION_INTERR[] PROGMEM = "[ERR!]";
const char UECShtmlInputText[] PROGMEM = "<INPUT name=\"L\" value=\"";

const char UECShtmlINPUTCLOSE3[] PROGMEM = "\" size=\"3\">";
const char UECShtmlINPUTCLOSE19[] PROGMEM = "\" size=\"19\">";
const char UECShtmlSUBMIT[] PROGMEM = "<input type=\"submit\" value=\"send\" name=\"S\">";

const char UECShtmlHEADER[] PROGMEM="<!DOCTYPE html><HTML><HEAD><META http-equiv=\"";
const char UECShtmlNORMAL[] PROGMEM    ="Content-Type\" content=\"text/html; charset=utf-8\"><TITLE>";
const char UECShtmlREDIRECT[] PROGMEM   ="refresh\" content=\"0;URL=./1\"></HEAD></HTML>";

const char UECShtmlTITLECLOSE[] PROGMEM="</TITLE>";
const char UECShtml1A[] PROGMEM="</HEAD><BODY><CENTER><H1>";
const char UECShtmlH1CLOSE[] PROGMEM="</H1>";
const char UECShtmlH2TAG[] PROGMEM="<H2>";
const char UECShtmlH2CLOSE[] PROGMEM="</H2>";
const char UECShtmlHTMLCLOSE[] PROGMEM="</CENTER></BODY></HTML>";
const char UECShtmlCCMRecRes0[] PROGMEM="</H1><H2>CCM Status</H2><TABLE border=\"1\"><TBODY align=\"center\"><TR><TH>Info</TH><TH>S/R</TH><TH>Type</TH><TH>SR Lev</TH><TH>Value</TH><TH>Valid</TH><TH>Sec</TH><TH>Atr</TH><TH>IP</TH></TR>";
const char UECStdtd[] PROGMEM="</TD><TD>";
const char UECStrtd[] PROGMEM="<TR><TD>";
const char UECStdtr[] PROGMEM="</TD></TR>";
const char UECShtmlHR[] PROGMEM="<HR>";
const char UECSformend[] PROGMEM = "</form>";

const char UECSpageError[] PROGMEM = "Error!";

const char UECShtmlIndex[] PROGMEM = "<a href=\"./1\">Node Status</a><br><br><a href=\"./2\">Network Config</a><br>";

const char UECShtmlUserRes0[] PROGMEM="<H2>Status & SetValue</H2><form action=\"./1\"><TABLE border=\"1\"><TBODY align=\"center\"><TR><TH>Name</TH><TH>Val</TH><TH>Unit</TH><TH>Detail</TH></TR>";
const char UECShtmlInputHidden[] PROGMEM = "<INPUT type=\"hidden\" name=\"L\" value=\"0\"/>";
	
const char UECShtmlSelect[] PROGMEM =  "<Select name=\"L\">";
const char UECShtmlOption[] PROGMEM = "<Option value=\"";
const char UECShtmlSelectEnd[] PROGMEM = "</SELECT>";

const char UECSaccess_NOSPC_GETP0[] PROGMEM = "GET/HTTP/1.1";
const char UECSaccess_NOSPC_GETP1[] PROGMEM = "GET/1HTTP/1.1";
const char UECSaccess_NOSPC_GETP1A[] PROGMEM = "GET/1?L=";
const char UECSaccess_NOSPC_GETP2[] PROGMEM = "GET/2HTTP/1.1";
const char UECSaccess_NOSPC_GETP2A[] PROGMEM = "GET/2?L=";
const char UECSaccess_LEQUAL[] PROGMEM = "L=";


const char UECSAHREF[] PROGMEM = "<a href=\"http://";
const char UECSTagClose[] PROGMEM = "\">";
const char UECSSlashTagClose[] PROGMEM = "\"/>";
const char UECSSlashA[] PROGMEM = "</a>";


const char UECSTxtPartR[] PROGMEM = "R";
const char UECSTxtPartS[] PROGMEM = "S";
const char UECSTxtPartColon[] PROGMEM = ":";
const char UECSTxtPartOK[] PROGMEM = "OK";
const char UECSTxtPartHyphen[] PROGMEM = "-";
const char UECSTxtPartSelected[] PROGMEM = "\" selected>";



struct UECSUserHtml{
  const char* name;
  byte pattern;
  const char* unit;
  const char* detail;
  const char** selectname;
  unsigned char selectnum;
  signed long* data;
  signed long minValue;
  signed long maxValue;
  unsigned char decimal;
};

void UECSinitOrgAttribute();
void UECSinitCCMList();
void UECSstartEthernet();
void UECSresetEthernet();
void SoftReset(void);

void UECS_EEPROM_writeLong(int ee, long value);
long UECS_EEPROM_readLong(int ee);
void HTTPcheckRequest();
void HTTPPrintHeader();
void HTTPsendPageIndex();
void HTTPsendPageLANSetting();
void HTTPsendPageCCM();

void HTTPGetFormDataCCMPage();
void HTTPGetFormDataLANSettingPage();

void UECSupdate16520portReceive(UECSTEMPCCM* _tempCCM, unsigned long _millis);
void UECSupdate16529port(UECSTEMPCCM* _tempCCM);
void UECSsetup();
void UECSloop();

/*
void UECSsetCCM(boolean _sender,
signed char _num,
const char* _name,
const char* _type,
const char* _unit,
unsigned short _room,
unsigned short _region,
unsigned short _order,
unsigned short _priority,
unsigned char _decimal,
char _ccmLevel);
*/

void UECSsetCCM(boolean _sender, signed char _num, const char* _name, const char* _type, const char* _unit, unsigned short _priority, unsigned char _decimal,  char _ccmLevel);

extern const int U_MAX_CCM;
extern const int U_HtmlLine;
extern struct UECSUserHtml U_html[];
extern char buffer[];
extern const byte U_InitPin;
extern const byte U_InitPin_Sense;
extern const char U_name[] PROGMEM;
extern const char U_vender[] PROGMEM;
extern const char U_uecsid[] PROGMEM;
extern const char U_footnote[] PROGMEM;
extern char U_nodename[];
extern UECSOriginalAttribute U_orgAttribute;
extern const char *UECSCCMLEVEL[];

extern UECSCCM U_ccmList[];
extern void OnWebFormRecieved();
extern void UserEveryMinute();
extern void UserEverySecond();
extern void UserEveryLoop();
extern void UserInit();

extern struct UECSTEMPCCM UECStempCCM;
extern unsigned char UECSsyscounter60s;
extern unsigned long UECSsyscounter1s;
extern unsigned long UECSnowmillis;
extern unsigned long UECSlastmillis;
extern EthernetUDP UECS_UDP16520;
extern EthernetUDP UECS_UDP16529;


//Form UecsLetterCheck.h
//############################################################
//############################################################



#define MAXIMUM_STRINGLENGTH 32767
#define ASCIICODE_SPACE	32
#define ASCIICODE_DQUOT	34
#define ASCIICODE_LF	10
#define ASCIICODE_CR	13


//######################################new
bool UECSFindPGMChar(char* targetBuffer,const char *_romword_startStr,int *lastPos);
bool UECSGetValPGMStrAndChr(char* targetBuffer,const char *_romword_startStr, char end_asciiCode, short *shortValue,int *lastPos);
//bool UECSGetValueBetweenChr(char* targetBuffer, char start_asciiCode, char end_asciiCode, short *shortValue,int *lastPos);
bool UECSGetFixedFloatValue(char* targetBuffer,long *longVal,unsigned char *decimal,int *lastPos);
bool UECSGetIPAddress(char *targetBuffer,unsigned char *,int *lastPos);
bool UECSAtoI_UChar(char *targetBuffer,unsigned char *ucharValue,int *lastPos);

void ClearMainBuffer(void);
void UDPAddPGMCharToBuffer(const char* _romword);
void UDPAddCharToBuffer(char* word);
void UDPAddValueToBuffer(long value);
void UDPFilterToBuffer(void);


void HTTPFilterToBuffer(void);
void HTTPAddPGMCharToBuffer(const char* _romword);
void HTTPAddCharToBuffer(char* word);
void HTTPAddValueToBuffer(long value);
void HTTPRefreshBuffer(void);
void HTTPCloseBuffer(void);


void HTTPprintIPtoHtml(byte address[]);
void HTTPsetInput(short _value);
void MemoryWatching(void);

bool ChangeWebValue(signed long* data,signed long value);


#endif
