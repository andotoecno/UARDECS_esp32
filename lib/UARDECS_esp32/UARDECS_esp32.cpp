#include <UARDECS_esp32.h>

char UECSbuffer[BUF_SIZE];		   // main buffer
char UECStempStr20[MAX_TYPE_CHAR]; // sub buffer
AsyncUDP UECS_UDP16520;
AsyncUDP UECS_UDP16529;
WiFiServer UECSlogserver(80);
WiFiClient UECSclient;

struct UECSTEMPCCM UECStempCCM;

unsigned char UECSsyscounter60s;
unsigned long UECSsyscounter1s;
unsigned long UECSnowmillis;
unsigned long UECSlastmillis;

// Form CCM.cpp
// ##############################################################################
// ##############################################################################

const char *UECSattrChar[] = {
	UECSccm_ROOMTXT,
	UECSccm_REGIONTXT,
	UECSccm_ORDERTXT,
	UECSccm_PRIORITYTXT,
};
const char *UECSCCMLEVEL[] = {
	UECS_A1S0,
	UECS_A1S1,
	UECS_A10S0,
	UECS_A10S1,
	UECS_A1M0,
	UECS_A1M1,
	UECS_S1S,
	UECS_S1M,
	UECS_B0_,
	UECS_B1_,
};

boolean UECSparseRec(struct UECSTEMPCCM *_tempCCM, int *matchCCMID)
{

	int i;
	int progPos = 0;
	int startPos = 0;
	short shortValue = 0;

	if (!UECSFindPGMChar(UECSbuffer, &UECSccm_XMLHEADER[0], &progPos))
	{
		return false;
	}
	startPos += progPos;

	if (UECSFindPGMChar(&UECSbuffer[startPos], &UECSccm_UECSVER_E10[0], &progPos))
	{
		startPos += progPos;
		// E10 packet
	}
	else
	{
		// other ver packet
		if (!(U_orgAttribute.flags & ATTRFLAG_LOOSELY_VERCHECK))
		{
			return false;
		}
	}

	if (!UECSFindPGMChar(&UECSbuffer[startPos], &UECSccm_DATATYPE[0], &progPos))
	{
		return false;
	}
	startPos += progPos;

	// copy ccm type string
	for (i = 0; i < MAX_TYPE_CHAR; i++)
	{
		_tempCCM->type[i] = UECSbuffer[startPos + i];
		if (_tempCCM->type[i] == ASCIICODE_DQUOT || _tempCCM->type[i] == '\0')
		{
			_tempCCM->type[i] = '\0';
			break;
		}
	}
	_tempCCM->type[MAX_CCMTYPESIZE] = '\0';
	startPos = startPos + i;

	// In a practical environment ,packets of 99% are irrelevant CCM.
	// matching top 3 chars for acceleration
	*matchCCMID = -1;
	for (i = 0; i < U_MAX_CCM; i++)
	{
		if (U_ccmList[i].ccmLevel != NONE && U_ccmList[i].sender == false) // check receive ccm
		{
			if (_tempCCM->type[0] == pgm_read_byte(U_ccmList[i].type) &&
				_tempCCM->type[1] == pgm_read_byte(U_ccmList[i].type + 1) &&
				_tempCCM->type[2] == pgm_read_byte(U_ccmList[i].type + 2))
			{
				*matchCCMID = i;
				break;
			}
		}
	}
	if (*matchCCMID < 0)
	{
		return false;
	} // my ccm was not match ->cancel packet reading

	for (i = 0; i < MAX_ATTR_NUMBER; i++)
	{
		if (!UECSGetValPGMStrAndChr(&UECSbuffer[startPos], UECSattrChar[i], '\"', &(_tempCCM->attribute[i]), &progPos))
		{
			return false;
		}
		startPos += progPos;
	}

	// find tag end
	if (!UECSFindPGMChar(&UECSbuffer[startPos], &UECSccm_CLOSETAG[0], &progPos))
	{
		return false;
	}
	startPos += progPos;

	// get value
	if (!UECSGetFixedFloatValue(&UECSbuffer[startPos], &(_tempCCM->value), &(_tempCCM->decimal), &progPos))
	{
		return false;
	}
	startPos += progPos;

	// ip tag
	if (!UECSFindPGMChar(&UECSbuffer[startPos], &UECSccm_IPTAG[0], &progPos))
	{
		// ip tag not found(old type packet)
		// ip address is already filled
		if (!UECSFindPGMChar(&UECSbuffer[startPos], &UECSccm_FOOTER0[0], &progPos))
		{
			return false;
		}
		return true;
	}
	startPos += progPos;

	unsigned char ip[4];
	if (!UECSGetIPAddress(&UECSbuffer[startPos], ip, &progPos))
	{
		return false;
	}

	_tempCCM->address[0] = ip[0];
	_tempCCM->address[1] = ip[1];
	_tempCCM->address[2] = ip[2];
	_tempCCM->address[3] = ip[3];

	startPos += progPos;

	if (U_orgAttribute.flags & ATTRFLAG_LOOSELY_VERCHECK)
	{
		return true;
	} // Ignore information after ip

	// check footer
	if (!UECSFindPGMChar(&UECSbuffer[startPos], &UECSccm_FOOTER1[0], &progPos))
	{
		return false;
	}
	/*
	Serial.println("type");
	Serial.println(_tempCCM->type);
	Serial.println("attribute[AT_ROOM]");
	Serial.println(_tempCCM->attribute[AT_ROOM]);
	Serial.println("attribute[AT_REGI]");
	Serial.println(_tempCCM->attribute[AT_REGI]);
	Serial.println("attribute[AT_ORDE]");
	Serial.println(_tempCCM->attribute[AT_ORDE]);
	Serial.println("attribute[AT_PRIO]");
	Serial.println(_tempCCM->attribute[AT_PRIO]);
	Serial.println("value");
	Serial.println(_tempCCM->value);
	Serial.println("decimal");
	Serial.println(_tempCCM->decimal);
	Serial.println("ip[0]");
	Serial.println(_tempCCM->address[0]);
	Serial.println("ip[1]");
	Serial.println(_tempCCM->address[1]);
	Serial.println("ip[2]");
	Serial.println(_tempCCM->address[2]);
	Serial.println("ip[3]");
	Serial.println(_tempCCM->address[3]);
	*/
	return true;
}
//----------------------------------------------------------------------------
void UECSCreateCCMPacketAndSend(struct UECSCCM *_ccm)
{
	ClearMainBuffer();
	UDPAddPGMCharToBuffer(&(UECSccm_XMLHEADER[0]));
	UDPAddPGMCharToBuffer(&(UECSccm_UECSVER_E10[0]));
	UDPAddPGMCharToBuffer(&(UECSccm_DATATYPE[0]));
	UDPAddPGMCharToBuffer(_ccm->type);

	for (int i = 0; i < 3; i++)
	{
		UDPAddPGMCharToBuffer(UECSattrChar[i]);
		UDPAddValueToBuffer(_ccm->baseAttribute[i]);
	}

	UDPAddPGMCharToBuffer(UECSattrChar[AT_PRIO]);
	UDPAddValueToBuffer(_ccm->attribute[AT_PRIO]);

	UDPAddPGMCharToBuffer(&(UECSccm_CLOSETAG[0]));
	dtostrf(((double)_ccm->value) / pow(10, _ccm->decimal), -1, _ccm->decimal, UECStempStr20);
	UDPAddCharToBuffer(UECStempStr20);
	UDPAddPGMCharToBuffer(&(UECSccm_IPTAG[0]));

	if (U_orgAttribute.status & STATUS_SAFEMODE)
	{
		UDPAddPGMCharToBuffer(&(UECSdefaultIPAddress[0]));
	}
	else
	{
		sprintf(UECStempStr20, "%d.%d.%d.%d", U_orgAttribute.ip[0], U_orgAttribute.ip[1], U_orgAttribute.ip[2], U_orgAttribute.ip[3]);
		UDPAddCharToBuffer(UECStempStr20);
	}

	UDPAddPGMCharToBuffer(&(UECSccm_FOOTER1[0]));

	// send ccm
	UECS_UDP16520.writeTo((const uint8_t *)UECSbuffer, sizeof(UECSbuffer), _ccm->address, 16520);
}

void UECSupRecCCM(UECSCCM *_ccm, UECSTEMPCCM *_ccmRec)
{
	boolean success = false;

	int i;
	for (i = 0; i < MAX_ATTR_NUMBER; i++)
	{
		_ccm->attribute[i] = _ccmRec->attribute[i];
	}

	for (i = 0; i < MAX_IPADDRESS_NUMBER; i++)
	{
		_ccm->address[i] = _ccmRec->address[i];
	}

	int dif_decimal = _ccm->decimal - _ccmRec->decimal;
	if (dif_decimal >= 0)
	{
		_ccm->value = _ccmRec->value * pow(10, dif_decimal);
	}
	else
	{
		_ccm->value = _ccmRec->value / pow(10, -dif_decimal);
	}

	_ccm->recmillis = 0;
	_ccm->validity = true;
	_ccm->flagStimeRfirst = true;
}
//---------------------------------------------------------------

void UECScheckUpDate(UECSTEMPCCM *_tempCCM, unsigned long _time, int startid)
{

	for (int i = startid; i < U_MAX_CCM; i++)
	{
		if (U_ccmList[i].ccmLevel == NONE || U_ccmList[i].sender == true)
		{
			continue;
		}

		if (!((_tempCCM->attribute[AT_ROOM] == 0 || U_ccmList[i].baseAttribute[AT_ROOM] == 0 || _tempCCM->attribute[AT_ROOM] == U_ccmList[i].baseAttribute[AT_ROOM]) &&
			  (_tempCCM->attribute[AT_REGI] == 0 || U_ccmList[i].baseAttribute[AT_REGI] == 0 || _tempCCM->attribute[AT_REGI] == U_ccmList[i].baseAttribute[AT_REGI]) &&
			  (_tempCCM->attribute[AT_ORDE] == 0 || U_ccmList[i].baseAttribute[AT_ORDE] == 0 || _tempCCM->attribute[AT_ORDE] == U_ccmList[i].baseAttribute[AT_ORDE])))
		{
			continue;
		}

		// type
		strcpy_P(UECStempStr20, U_ccmList[i].type);

		/*
		if(U_orgAttribute.flags&ATTRFLAG_ALLOW_ABRIDGE_TYPE)
			{
			strtok(UECStempStr20,".");
			strtok(_tempCCM->type,".");
			}
			*/
		if (strcmp(UECStempStr20, _tempCCM->type) != 0)
		{
			continue;
		}

		boolean up = false;
		if (U_ccmList[i].ccmLevel == B_0 || U_ccmList[i].ccmLevel == B_1)
		{
			up = true;
		}
		else if (!U_ccmList[i].validity)
		{
			up = true;
		} // fresh ccm
		else if (_tempCCM->attribute[AT_PRIO] < U_ccmList[i].attribute[AT_PRIO])
		{
			up = true;
		} // lower priority number is strong
		else
		{

			if (_tempCCM->attribute[AT_ROOM] == U_ccmList[i].attribute[AT_ROOM])
			{
				if (_tempCCM->attribute[AT_REGI] == U_ccmList[i].attribute[AT_REGI])
				{
					if (_tempCCM->attribute[AT_ORDE] == U_ccmList[i].attribute[AT_ORDE])
					{

						// if(_tempCCM->address <= U_ccmList[i].address)
						// convert big endian
						unsigned long address_t = _tempCCM->address[0];
						address_t = (address_t << 8) | _tempCCM->address[1];
						address_t = (address_t << 8) | _tempCCM->address[2];
						address_t = (address_t << 8) | _tempCCM->address[3];
						unsigned long address_b = U_ccmList[i].address[0];
						address_b = (address_b << 8) | U_ccmList[i].address[1];
						address_b = (address_b << 8) | U_ccmList[i].address[2];
						address_b = (address_b << 8) | U_ccmList[i].address[3];

						if (address_t <= address_b)
						{
							up = true;
						}
					}
					else if (_tempCCM->attribute[AT_ORDE] == U_ccmList[i].baseAttribute[AT_ORDE] || U_ccmList[i].baseAttribute[AT_ORDE] == 0)
					{
						up = true;
					}
				}
				else if (_tempCCM->attribute[AT_REGI] == U_ccmList[i].baseAttribute[AT_REGI] || U_ccmList[i].baseAttribute[AT_REGI] == 0)
				{
					up = true;
				}
			}
			else if (_tempCCM->attribute[AT_ROOM] == U_ccmList[i].baseAttribute[AT_ROOM] || U_ccmList[i].baseAttribute[AT_ROOM] == 0)
			{
				up = true;
			}
		}

		if (up)
		{
			UECSupRecCCM(&U_ccmList[i], _tempCCM);
		}
	}
}

/********************************/
/* 16529 Response   *************/
/********************************/

boolean UECSresNodeScan()
{
	int i;
	int progPos = 0;
	int startPos = 0;

	if (!UECSFindPGMChar(UECSbuffer, &UECSccm_XMLHEADER[0], &progPos))
	{
		return false;
	}
	startPos += progPos;

	if (!UECSFindPGMChar(&UECSbuffer[startPos], &UECSccm_UECSVER_E10[0], &progPos))
	{
		// other ver
		return false;
	}
	startPos += progPos;

	// NODESCAN
	if (UECSFindPGMChar(&UECSbuffer[startPos], &UECSccm_NODESCAN1[0], &progPos) || UECSFindPGMChar(&UECSbuffer[startPos], &UECSccm_NODESCAN2[0], &progPos))
	{

		// NODESCAN response
		ClearMainBuffer();
		UDPAddPGMCharToBuffer(&(UECSccm_XMLHEADER[0]));
		UDPAddPGMCharToBuffer(&(UECSccm_UECSVER_E10[0]));
		UDPAddPGMCharToBuffer(&(UECSccm_NODENAME[0]));
		UDPAddPGMCharToBuffer(&(U_name[0]));
		UDPAddPGMCharToBuffer(&(UECSccm_VENDER[0]));
		UDPAddPGMCharToBuffer(&(U_vender[0]));
		UDPAddPGMCharToBuffer(&(UECSccm_UECSID[0]));
		UDPAddPGMCharToBuffer(&(U_uecsid[0]));
		UDPAddPGMCharToBuffer(&(UECSccm_UECSID_IP[0]));

		if (U_orgAttribute.status & STATUS_SAFEMODE)
		{
			UDPAddPGMCharToBuffer(&(UECSdefaultIPAddress[0]));
		}
		else
		{
			sprintf(UECStempStr20, "%d.%d.%d.%d", U_orgAttribute.ip[0], U_orgAttribute.ip[1], U_orgAttribute.ip[2], U_orgAttribute.ip[3]);
			UDPAddCharToBuffer(UECStempStr20);
		}

		UDPAddPGMCharToBuffer(&(UECSccm_MAC[0]));
		sprintf(UECStempStr20, "%02X%02X%02X%02X%02X%02X", U_orgAttribute.mac[0], U_orgAttribute.mac[1], U_orgAttribute.mac[2], U_orgAttribute.mac[3], U_orgAttribute.mac[4], U_orgAttribute.mac[5]);
		UDPAddCharToBuffer(UECStempStr20);
		UDPAddPGMCharToBuffer(&(UECSccm_NODECLOSE[0]));

		return true;
	}

	// CCMSCAN
	if (UECSFindPGMChar(&UECSbuffer[startPos], &UECSccm_CCMSCAN[0], &progPos))
	{

		short pageNum = 0;
		startPos += progPos;

		if (UECSGetValPGMStrAndChr(&UECSbuffer[startPos], &UECSccm_PAGE[0], '\"', &pageNum, &progPos)) // format of page number written type
		{
			startPos += progPos;
			// check close tag
			if (!(UECSFindPGMChar(&UECSbuffer[startPos], &UECSccm_CCMSCANCLOSE2[0], &progPos)))
			{
				return false;
			}
		}
		else if (UECSFindPGMChar(&UECSbuffer[startPos], &UECSccm_CCMSCANCLOSE0[0], &progPos)) // format of page number abridged type
		{
			pageNum = 1;
		}
		else
		{
			return false;
		}

		// CCMSCAN response
		ClearMainBuffer();
		UDPAddPGMCharToBuffer(&(UECSccm_XMLHEADER[0]));
		UDPAddPGMCharToBuffer(&(UECSccm_UECSVER_E10[0]));
		UDPAddPGMCharToBuffer(&(UECSccm_CCMNUM[0]));

		// count enabled ccm
		short enabledCCMNum = 0;
		short returnCCMID = -1;
		for (i = 0; i < U_MAX_CCM; i++)
		{
			if (U_ccmList[i].ccmLevel != NONE)
			{
				enabledCCMNum++;
				if (enabledCCMNum == pageNum)
				{
					returnCCMID = i;
				}
			}
		}

		if (enabledCCMNum == 0 || returnCCMID < 0)
		{
			return false;
		} // page num over

		UDPAddValueToBuffer(pageNum);
		UDPAddPGMCharToBuffer(&(UECSccm_TOTAL[0]));
		UDPAddValueToBuffer(enabledCCMNum);
		UDPAddPGMCharToBuffer(&(UECSccm_CLOSETAG[0]));
		UDPAddValueToBuffer(1); // Column number is always 1
		UDPAddPGMCharToBuffer(&(UECSccm_CCMNO[0]));
		UDPAddValueToBuffer(pageNum); // page number = ccm number

		for (i = 0; i < 3; i++)
		{
			UDPAddPGMCharToBuffer(UECSattrChar[i]);
			UDPAddValueToBuffer(U_ccmList[returnCCMID].baseAttribute[i]);
		}
		UDPAddPGMCharToBuffer(UECSattrChar[AT_PRIO]);
		UDPAddValueToBuffer(U_ccmList[returnCCMID].attribute[AT_PRIO]);

		UDPAddPGMCharToBuffer(&(UECSccm_CAST[0]));
		UDPAddValueToBuffer(U_ccmList[returnCCMID].decimal);
		UDPAddPGMCharToBuffer(&(UECSccm_UNIT[0]));
		UDPAddPGMCharToBuffer((U_ccmList[returnCCMID].unit));
		UDPAddPGMCharToBuffer(&(UECSccm_SR[0]));
		if (U_ccmList[returnCCMID].sender)
		{
			UDPAddPGMCharToBuffer(UECSTxtPartS);
		}
		else
		{
			UDPAddPGMCharToBuffer(UECSTxtPartR);
		}
		UDPAddPGMCharToBuffer(&(UECSccm_LV[0]));
		UDPAddPGMCharToBuffer((UECSCCMLEVEL[U_ccmList[returnCCMID].ccmLevel]));
		UDPAddPGMCharToBuffer(&(UECSccm_CLOSETAG[0]));
		UDPAddPGMCharToBuffer((U_ccmList[returnCCMID].type));
		UDPAddPGMCharToBuffer(&(UECSccm_CCMRESCLOSE[0]));

		return true;
	}

	return false;
}

//------------------------------------------------------------------
void UECSautomaticSendManager()
{
	for (int id = 0; id < U_MAX_CCM; id++)
	{
		if (U_ccmList[id].ccmLevel == NONE || !U_ccmList[id].sender)
		{
			continue;
		}

		if (U_ccmList[id].ccmLevel == A_1S_0 || U_ccmList[id].ccmLevel == A_1S_1 || U_ccmList[id].ccmLevel == S_1S_0)
		{
			U_ccmList[id].flagStimeRfirst = true;
		}
		else if ((UECSsyscounter60s % 10 == id % 10) && (U_ccmList[id].ccmLevel == A_10S_0 || U_ccmList[id].ccmLevel == A_10S_1))
		{
			U_ccmList[id].flagStimeRfirst = true;
		}
		else if (UECSsyscounter60s == id % 10 && (U_ccmList[id].ccmLevel == A_1M_0 || U_ccmList[id].ccmLevel == A_1M_1 || U_ccmList[id].ccmLevel == S_1M_0))
		{
			U_ccmList[id].flagStimeRfirst = true;
		}
		else
		{
			U_ccmList[id].flagStimeRfirst = false;
		}
	}
}
//----------------------------------------------------------------------
void UECSautomaticValidManager(unsigned long td)
{
	for (int id = 0; id < U_MAX_CCM; id++)
	{
		if (U_ccmList[id].ccmLevel == NONE || U_ccmList[id].sender)
		{
			continue;
		}

		if (U_ccmList[id].recmillis > 86400000) // over 24hour
		{
			continue; // stop count
		}

		U_ccmList[id].recmillis += td; // count time(ms) since last recieved

		unsigned long validmillis = 0;
		if (U_ccmList[id].ccmLevel == A_1S_0 || U_ccmList[id].ccmLevel == A_1S_1 || U_ccmList[id].ccmLevel == S_1S_0)
		{
			validmillis = 3000;
		}
		else if (U_ccmList[id].ccmLevel == A_10S_0 || U_ccmList[id].ccmLevel == A_10S_1)
		{
			validmillis = 30000;
		}
		else if (U_ccmList[id].ccmLevel == A_1M_0 || U_ccmList[id].ccmLevel == A_1M_1 || U_ccmList[id].ccmLevel == S_1M_0)
		{
			validmillis = 180000;
		}

		if (U_ccmList[id].recmillis > validmillis || U_ccmList[id].flagStimeRfirst == false)
		{
			U_ccmList[id].validity = false;
		}
	}
}

// ##############################################################################
// ##############################################################################

void UECS_EEPROM_writeLong(int ee, long value)
{
	int skip_counter = 0;
	byte *p = (byte *)(void *)&value;
	for (unsigned int i = 0; i < sizeof(value); i++)
	{
		if (EEPROM.read(ee) != p[i]) // same value skip
		{
			EEPROM.write(ee, p[i]);
		}
		else
			skip_counter++;
		ee++;
	}
	// writing EEPROM, ESP32 specification
	if (skip_counter < sizeof(value))
	{
		EEPROM.commit();
	}
}

long UECS_EEPROM_readLong(int ee)
{
	long value = 0;
	byte *p = (byte *)(void *)&value;
	for (unsigned int i = 0; i < sizeof(value); i++)
		*p++ = EEPROM.read(ee++);
	return value;
}
//-----------------------------------------------------------new
void HTTPsetInput(short _value)
{
	HTTPAddPGMCharToBuffer(&(UECShtmlInputText[0]));
	HTTPAddValueToBuffer(_value);
	HTTPAddPGMCharToBuffer(&(UECShtmlINPUTCLOSE3[0]));
}
//-----------------------------------------------------------new
void HTTPprintIPtoHtml(byte address[])
{
	for (int i = 0; i < 3; i++)
	{
		HTTPsetInput(address[i]);
		HTTPAddPGMCharToBuffer(UECSTxtPartColon);
	}
	HTTPsetInput(address[3]);
	HTTPAddPGMCharToBuffer(&(UECSbtrag[0]));
}
//-----------------------------------------------------------new
//---------------------------------------------------------------
void HTTPPrintRedirectP1()
{
	ClearMainBuffer();
	HTTPAddPGMCharToBuffer(&(UECShttpHead200_OK[0]));
	HTTPAddPGMCharToBuffer(&(UECShttpHeadContentType[0]));
	HTTPAddPGMCharToBuffer(&(UECShttpHeadConnection[0]));
	HTTPAddPGMCharToBuffer(&(UECShtmlHEADER[0]));
	HTTPAddPGMCharToBuffer(&(UECShtmlREDIRECT[0]));
	HTTPCloseBuffer();
}
//-----------------------------------------------------------
void HTTPPrintHeader()
{
	ClearMainBuffer();

	HTTPAddPGMCharToBuffer(&(UECShttpHead200_OK[0]));
	HTTPAddPGMCharToBuffer(&(UECShttpHeadContentType[0]));
	HTTPAddPGMCharToBuffer(&(UECShttpHeadConnection[0]));

	HTTPAddPGMCharToBuffer(&(UECShtmlHEADER[0]));
	HTTPAddPGMCharToBuffer(&(UECShtmlNORMAL[0]));

	if (U_orgAttribute.status & STATUS_MEMORY_LEAK)
	{
		HTTPAddPGMCharToBuffer(&(UECShtmlATTENTION_INTERR[0]));
	}

	if (U_orgAttribute.status & STATUS_SAFEMODE)
	{
		HTTPAddPGMCharToBuffer(&(UECShtmlATTENTION_SAFEMODE[0]));
	}

	HTTPAddPGMCharToBuffer(&(U_name[0]));

	HTTPAddPGMCharToBuffer(&(UECShtmlTITLECLOSE[0]));

	HTTPAddPGMCharToBuffer(&(UECShtml1A[0])); // </script></HEAD><BODY><CENTER><H1>
}
//-----------------------------------------------------------
void HTTPsendPageError()
{
	HTTPPrintHeader();
	HTTPAddPGMCharToBuffer(&(UECSpageError[0]));
	HTTPAddPGMCharToBuffer(&(UECShtmlH1CLOSE[0]));
	HTTPAddPGMCharToBuffer(&(UECShtmlHTMLCLOSE[0]));
	HTTPCloseBuffer();
}
//-------------------------------------------------------------
void HTTPsendPageIndex()
{

	HTTPPrintHeader();
	HTTPAddCharToBuffer(U_nodename);
	HTTPAddPGMCharToBuffer(&(UECShtmlH1CLOSE[0]));
	HTTPAddPGMCharToBuffer(&(UECShtmlIndex[0]));
	HTTPAddPGMCharToBuffer(&(UECShtmlHR[0]));
	HTTPAddPGMCharToBuffer(&(U_footnote[0]));
	HTTPAddPGMCharToBuffer(&(UECShtmlHTMLCLOSE[0]));
	HTTPCloseBuffer();
}
//--------------------------------------------------
void HTTPsendPageLANSetting()
{
	UECSinitOrgAttribute();

	HTTPPrintHeader();
	HTTPAddCharToBuffer(U_nodename);
	HTTPAddPGMCharToBuffer(UECShtmlH1CLOSE);
	HTTPAddPGMCharToBuffer(UECShtmlH2TAG);
	HTTPAddPGMCharToBuffer(UECShtmlLANTITLE);
	HTTPAddPGMCharToBuffer(UECShtmlH2CLOSE);
	HTTPAddPGMCharToBuffer(&(UECShtmlLAN2[0]));	 // <form action=\"./h2.htm\" name=\"f\"><p>
	HTTPAddPGMCharToBuffer(&(UECShtmlLAN3A[0])); // address:
	byte UECStempByte[4];
	for (int i = 0; i < 4; i++)
	{
		UECStempByte[i] = U_orgAttribute.ip[i];
	}
	HTTPprintIPtoHtml(UECStempByte); // XXX:XXX:XXX:XXX <br>

	HTTPAddPGMCharToBuffer(&(UECShtmlLAN3B[0])); // subnet:
	HTTPprintIPtoHtml(U_orgAttribute.subnet);	 // XXX:XXX:XXX:XXX <br>

	HTTPAddPGMCharToBuffer(&(UECShtmlLAN3C[0])); // gateway:
	HTTPprintIPtoHtml(U_orgAttribute.gateway);	 // XXX:XXX:XXX:XXX <br>

	HTTPAddPGMCharToBuffer(&(UECShtmlLAN3D[0])); // dns:
	HTTPprintIPtoHtml(U_orgAttribute.dns);		 // XXX:XXX:XXX:XXX <br>

	HTTPAddPGMCharToBuffer(&(UECShtmlLAN3E[0])); // mac:
	sprintf(UECStempStr20, "%02X%02X%02X%02X%02X%02X", U_orgAttribute.mac[0], U_orgAttribute.mac[1], U_orgAttribute.mac[2], U_orgAttribute.mac[3], U_orgAttribute.mac[4], U_orgAttribute.mac[5]);
	UDPAddCharToBuffer(UECStempStr20);

	HTTPAddPGMCharToBuffer(UECShtmlH2TAG);	   // <H2>
	HTTPAddPGMCharToBuffer(UECShtmlUECSTITLE); // UECS
	HTTPAddPGMCharToBuffer(UECShtmlH2CLOSE);   // </H2>

	HTTPAddPGMCharToBuffer(UECShtmlRoom);
	HTTPsetInput(U_orgAttribute.room);
	HTTPAddPGMCharToBuffer(UECShtmlRegion);
	HTTPsetInput(U_orgAttribute.region);
	HTTPAddPGMCharToBuffer(UECShtmlOrder);
	HTTPsetInput(U_orgAttribute.order);
	HTTPAddPGMCharToBuffer(&(UECSbtrag[0]));

	HTTPAddPGMCharToBuffer(&(UECShtmlUECSID[0])); // uecsid:
	UDPAddPGMCharToBuffer(&(U_uecsid[0]));

	HTTPAddPGMCharToBuffer(UECShtmlH2TAG);	   // <H2>
	HTTPAddPGMCharToBuffer(UECShtmlNAMETITLE); // Node Name
	HTTPAddPGMCharToBuffer(UECShtmlH2CLOSE);   // </H2>

	HTTPAddPGMCharToBuffer(&(UECShtmlInputText[0]));
	HTTPAddCharToBuffer(U_nodename);
	HTTPAddPGMCharToBuffer(UECShtmlINPUTCLOSE19);
	HTTPAddPGMCharToBuffer(&(UECSbtrag[0]));

	HTTPAddPGMCharToBuffer(&(UECShtmlSUBMIT[0])); // <input name=\"b\" type=\"submit\" value=\"send\">
	HTTPAddPGMCharToBuffer(&(UECSformend[0]));	  //</form>

	if (U_orgAttribute.status & STATUS_NEEDRESET)
	{
		HTTPAddPGMCharToBuffer(&(UECShtmlATTENTION_RESET[0]));
	}

	HTTPAddPGMCharToBuffer(&(UECShtmlRETURNINDEX[0]));
	HTTPAddPGMCharToBuffer(&(UECShtmlHTMLCLOSE[0])); //</BODY></HTML>
	HTTPCloseBuffer();
}

//--------------------------------------------------

void HTTPsendPageCCM()
{

	HTTPPrintHeader();
	HTTPAddCharToBuffer(U_nodename);
	HTTPAddPGMCharToBuffer(&(UECShtmlCCMRecRes0[0])); // </H1><H2>CCM Status</H2><TABLE border=\"1\"><TBODY align=\"center\"><TR><TH>Info</TH><TH>S/R</TH><TH>Type</TH><TH>SR Lev</TH><TH>Value</TH><TH>Valid</TH><TH>Sec</TH><TH>Atr</TH><TH>IP</TH></TR>

	for (int i = 0; i < U_MAX_CCM; i++)
	{
		if (U_ccmList[i].ccmLevel != NONE)
		{

			HTTPAddPGMCharToBuffer(&(UECStrtd[0])); //<tr><td>
			HTTPAddPGMCharToBuffer(U_ccmList[i].name);
			HTTPAddPGMCharToBuffer(&(UECStdtd[0])); //</td><td>
			if (U_ccmList[i].sender)
			{
				HTTPAddPGMCharToBuffer(UECSTxtPartS);
			}
			else
			{
				HTTPAddPGMCharToBuffer(UECSTxtPartR);
			}

			HTTPAddPGMCharToBuffer(&(UECStdtd[0])); //</td><td>
			HTTPAddPGMCharToBuffer(U_ccmList[i].type);
			HTTPAddPGMCharToBuffer(&(UECStdtd[0])); //</td><td>

			HTTPAddPGMCharToBuffer((UECSCCMLEVEL[U_ccmList[i].ccmLevel])); //******

			HTTPAddPGMCharToBuffer(&(UECStdtd[0])); //</td><td>

			dtostrf(((double)U_ccmList[i].value) / pow(10, U_ccmList[i].decimal), -1, U_ccmList[i].decimal, UECStempStr20);
			HTTPAddCharToBuffer(UECStempStr20);
			HTTPAddPGMCharToBuffer(&(UECStdtd[0])); //</td><td>
			if (U_ccmList[i].sender)
			{
			}
			else
			{
				if (U_ccmList[i].validity)
				{
					HTTPAddPGMCharToBuffer(UECSTxtPartOK);
				}
				else
				{
					HTTPAddPGMCharToBuffer(UECSTxtPartHyphen);
				}
			}

			HTTPAddPGMCharToBuffer(&(UECStdtd[0])); //</td><td>

			if (U_ccmList[i].flagStimeRfirst && U_ccmList[i].sender == false)
			{
				if (U_ccmList[i].recmillis < 36000000)
				{
					HTTPAddValueToBuffer(U_ccmList[i].recmillis / 1000);
				}
			}
			else
			{
				// over 10hour
			}

			HTTPAddPGMCharToBuffer(&(UECStdtd[0])); //</td><td>

			if (U_ccmList[i].flagStimeRfirst && U_ccmList[i].sender == false)
			{
				sprintf(UECStempStr20, "%d-%d-%d-%d", U_ccmList[i].attribute[AT_ROOM], U_ccmList[i].attribute[AT_REGI], U_ccmList[i].attribute[AT_ORDE], U_ccmList[i].attribute[AT_PRIO]);
				HTTPAddCharToBuffer(UECStempStr20);
			}
			sprintf(UECStempStr20, "(%d-%d-%d)", U_ccmList[i].baseAttribute[AT_ROOM], U_ccmList[i].baseAttribute[AT_REGI], U_ccmList[i].baseAttribute[AT_ORDE]);
			HTTPAddCharToBuffer(UECStempStr20);
			HTTPAddPGMCharToBuffer(&(UECStdtd[0])); //</td><td>

			HTTPAddPGMCharToBuffer(&(UECSAHREF[0]));
			sprintf(UECStempStr20, "%d.%d.%d.%d", U_ccmList[i].address[0], U_ccmList[i].address[1], U_ccmList[i].address[2], U_ccmList[i].address[3]);
			HTTPAddCharToBuffer(UECStempStr20);
			HTTPAddPGMCharToBuffer(&(UECSTagClose[0]));
			HTTPAddCharToBuffer(UECStempStr20);
			HTTPAddPGMCharToBuffer(&(UECSSlashA[0]));

			HTTPAddPGMCharToBuffer(&(UECStdtr[0])); //</td></tr>

		} // NONE route
	}

	HTTPAddPGMCharToBuffer(&(UECShtmlTABLECLOSE[0])); //</TBODY></TABLE>

	if (U_HtmlLine > 0)
	{
		HTTPAddPGMCharToBuffer(&(UECShtmlUserRes0[0])); // </H1><H2>Status</H2><TABLE border=\"1\"><TBODY align=\"center\"><TR><TH>Name</TH><TH>Val</TH><TH>Unit</TH><TH>Detail</TH></TR>

		for (int i = 0; i < U_HtmlLine; i++)
		{

			HTTPAddPGMCharToBuffer(&(UECStrtd[0]));
			HTTPAddPGMCharToBuffer(U_html[i].name);
			HTTPAddPGMCharToBuffer(&(UECStdtd[0]));
			// only value

			if (U_html[i].pattern == UECSSHOWDATA)
			{
				if (U_html[i].decimal != 0)
				{
					dtostrf(((double)*U_html[i].data) / pow(10, U_html[i].decimal), -1, U_html[i].decimal, UECStempStr20);
				}
				else
				{
					sprintf(UECStempStr20, "%ld", *(U_html[i].data));
				}
				HTTPAddCharToBuffer(UECStempStr20);
				HTTPAddPGMCharToBuffer(&(UECShtmlInputHidden[0]));
			}
			else if (U_html[i].pattern == UECSINPUTDATA)
			{
				HTTPAddPGMCharToBuffer(&(UECShtmlInputText[0]));
				dtostrf(((double)*U_html[i].data) / pow(10, U_html[i].decimal), -1, U_html[i].decimal, UECStempStr20);
				HTTPAddCharToBuffer(UECStempStr20);
				HTTPAddPGMCharToBuffer(UECSSlashTagClose);
			}
			else if (U_html[i].pattern == UECSSELECTDATA)
			{
				HTTPAddPGMCharToBuffer(&(UECShtmlSelect[0]));

				for (int j = 0; j < U_html[i].selectnum; j++)
				{

					HTTPAddPGMCharToBuffer(&(UECShtmlOption[0]));

					HTTPAddValueToBuffer(j);
					if ((int)(*U_html[i].data) == j)
					{
						HTTPAddPGMCharToBuffer(UECSTxtPartSelected);
					}
					else
					{
						HTTPAddPGMCharToBuffer(UECSTagClose);
					}
					HTTPAddPGMCharToBuffer(U_html[i].selectname[j]); //************
				}

				HTTPAddPGMCharToBuffer(&(UECShtmlSelectEnd[0]));
			}
			else if (U_html[i].pattern == UECSSHOWSTRING)
			{
				HTTPAddPGMCharToBuffer(U_html[i].selectname[(int)*(U_html[i].data)]); //************
				HTTPAddPGMCharToBuffer(&(UECShtmlInputHidden[0]));
			}

			HTTPAddPGMCharToBuffer(&(UECStdtd[0]));
			HTTPAddPGMCharToBuffer(U_html[i].unit);
			HTTPAddPGMCharToBuffer(&(UECStdtd[0]));
			HTTPAddPGMCharToBuffer(U_html[i].detail);
			HTTPAddPGMCharToBuffer(&(UECStdtr[0])); //</td></tr>
		}

		HTTPAddPGMCharToBuffer(&(UECShtmlTABLECLOSE[0])); //</TBODY></TABLE>
		HTTPAddPGMCharToBuffer(&(UECShtmlSUBMIT[0]));	  // <input name=\"b\" type=\"submit\" value=\"send\">
		HTTPAddPGMCharToBuffer(&(UECSformend[0]));		  //</form>
	}
	HTTPAddPGMCharToBuffer(&(UECShtmlRETURNINDEX[0])); //<P align=\"center\">return <A href=\"index.htm\">Top</A></P>
	HTTPAddPGMCharToBuffer(&(UECShtmlHTMLCLOSE[0]));

	HTTPCloseBuffer();
}

//---------------------------------------------####################
//---------------------------------------------####################
//---------------------------------------------####################
void HTTPGetFormDataCCMPage()
{
	if (U_HtmlLine == 0)
	{
		return;
	}

	int i;
	int startPos = 0;
	int progPos = 0;
	long tempValue;
	unsigned char tempDecimal;

	for (i = 0; i < U_HtmlLine; i++)
	{
		if (!UECSFindPGMChar(&UECSbuffer[startPos], UECSaccess_LEQUAL, &progPos))
		{
			continue;
		}
		startPos += progPos;

		if (U_html[i].pattern == UECSINPUTDATA)
		{

			if (!UECSGetFixedFloatValue(&UECSbuffer[startPos], &tempValue, &tempDecimal, &progPos))
			{
				return;
			}
			startPos += progPos;
			if (UECSbuffer[startPos] != '&')
			{
				return;
			} // last '&' not found

			int dif_decimal = U_html[i].decimal - tempDecimal;
			if (dif_decimal >= 0)
			{
				*U_html[i].data = tempValue * pow(10, dif_decimal);
			}
			else
			{
				*U_html[i].data = tempValue / pow(10, -dif_decimal);
			}

			if (*U_html[i].data < U_html[i].minValue)
			{
				*U_html[i].data = U_html[i].minValue;
			}
			if (*U_html[i].data > U_html[i].maxValue)
			{
				*U_html[i].data = U_html[i].maxValue;
			}
		}
		else if (U_html[i].pattern == UECSSELECTDATA)
		{

			if (!UECSGetFixedFloatValue(&UECSbuffer[startPos], &tempValue, &tempDecimal, &progPos))
			{
				return;
			}
			startPos += progPos;

			if (UECSbuffer[startPos] != '&')
			{
				return;
			} // last '&' not found

			if (tempDecimal != 0)
			{
				return;
			}
			*U_html[i].data = tempValue % U_html[i].selectnum; // cut too big value
		}
	}

	OnWebFormRecieved();

	for (i = 0; i < U_HtmlLine; i++)
	{
		UECS_EEPROM_writeLong(EEPROM_OFFSET_WEBDATA + i * 4, *(U_html[i].data));
	}
}
//--------------------------------------------------------------------------
void HTTPGetFormDataLANSettingPage()
{
	int i;
	int startPos = 0;
	int progPos = 0;
	long UECStempValue[20];
	unsigned char tempDecimal;
	int skip_counter = 0;
	//
	// MYIP      4
	// SUBNET    4
	// GATEWAY   4
	// DNS       4
	// room      1
	// region    1
	// order     1
	//-------------
	// total    19

	// get value
	for (i = 0; i < 19; i++)
	{
		if (!UECSFindPGMChar(&UECSbuffer[startPos], UECSaccess_LEQUAL, &progPos))
		{
			return;
		}
		startPos += progPos;
		if (!UECSGetFixedFloatValue(&UECSbuffer[startPos], &UECStempValue[i], &tempDecimal, &progPos))
		{
			return;
		}
		startPos += progPos;
		if (UECSbuffer[startPos] != '&')
		{
			return;
		} // last '&' not found
		if (tempDecimal != 0)
		{
			return;
		}

		// check value and write
		if (i < 18 && (UECStempValue[i] < 0 || UECStempValue[i] > 255))
		{
			return;
		} // IP address,room,region
		else if (UECStempValue[i] < 0 || UECStempValue[i] > 30000)
		{
			return;
		} // order
	}

	// order requires 2byte
	// UECStempValue[18]	order low
	// UECStempValue[19]	order high
	// set "order" high-byte
	UECStempValue[19] = (UECStempValue[18] / 256) & 127;

	skip_counter = 0;
	for (int i = 0; i < 20; i++)
	{
		if (EEPROM.read(EEPROM_OFFSET_DATATOP + i) != (unsigned char)UECStempValue[i]) // skip same value
		{
			EEPROM.write(EEPROM_OFFSET_DATATOP + i, (unsigned char)UECStempValue[i]);
			U_orgAttribute.status |= STATUS_NEEDRESET;

			// EEPROM.commit();
			Serial.println("save UECS main buf");
			// delay(100);
		}
		else skip_counter++;
	}
	// writing EEPROM, ESP32 specification
	Serial.println(skip_counter);
	if (skip_counter < 20)
	{
EEPROM.commit();
	}

	//---------------------------NODE NAME
	if (!UECSFindPGMChar(&UECSbuffer[startPos], UECSaccess_LEQUAL, &progPos))
	{
		return;
	}
	startPos += progPos;

	// copy node name
	for (i = 0; i < 20; i++)
	{

		if (UECSbuffer[startPos + i] == '<' || UECSbuffer[startPos + i] == '>') // eliminate tag
		{
			UECSbuffer[startPos + i] = '*';
		}

		if (UECSbuffer[startPos + i] == '&')
		{
			break;
		}
		if (UECSbuffer[startPos + i] == '\0' || i == 19)
		{
			return;
		} // �I�[�������̂Ŗ���
		// prevention of Cutting multibyte UTF-8 code
		if (i >= 16 && (unsigned char)UECSbuffer[startPos + i] >= 0xC0) // UTF-8 multibyte code header
		{

			break;
		}

		UECStempStr20[i] = UECSbuffer[startPos + i];
	}

	UECStempStr20[i] = '\0'; // set end code

	for (int i = 0; i < 20; i++)
	{
		U_nodename[i] = UECStempStr20[i];

		if (EEPROM.read(EEPROM_OFFSET_NODENAME + i) != U_nodename[i]) // skip same value
		{
			EEPROM.write(EEPROM_OFFSET_NODENAME + i, U_nodename[i]);
			// EEPROM.commit();
			// delay(100);
			Serial.println("save UECS Node Name");
		}
		else skip_counter++;
	}
	// writing EEPROM, ESP32 specification
	Serial.println(skip_counter);
	if (skip_counter < 20)
	{
EEPROM.commit();
	}

	return;
}

//---------------------------------------------####################
void HTTPcheckRequest()
{
	UECSclient = UECSlogserver.available();
	// Caution! This function can not receive only up to the top 299 bytes.
	// Dropped string will be ignored.

	if (UECSclient)
	{

		// Add null termination
		UECSbuffer[UECSclient.read((uint8_t *)UECSbuffer, BUF_SIZE - 1)] = '\0';

		HTTPFilterToBuffer(); // Get first line before "&S=" and eliminate unnecessary code
		Serial.println(UECSbuffer);
		int progPos = 0;
		// Top Page
		if (UECSFindPGMChar(UECSbuffer, &(UECSaccess_NOSPC_GETP0[0]), &progPos))
		{
			HTTPsendPageIndex();
		}
		// CCM page
		else if (UECSFindPGMChar(UECSbuffer, &(UECSaccess_NOSPC_GETP1[0]), &progPos))
		{
			HTTPsendPageCCM();
		}
		// LAN setting page
		else if (UECSFindPGMChar(UECSbuffer, &(UECSaccess_NOSPC_GETP2[0]), &progPos))
		{
			HTTPsendPageLANSetting();
		}
		// send CCM
		else if (UECSFindPGMChar(UECSbuffer, &(UECSaccess_NOSPC_GETP1A[0]), &progPos)) // include form data
		{
			HTTPGetFormDataCCMPage();
			// HTTPsendPageCCM();
			HTTPPrintRedirectP1();
		}
		// send LAN setting
		else if (UECSFindPGMChar(UECSbuffer, &(UECSaccess_NOSPC_GETP2A[0]), &progPos)) // include form data
		{
			HTTPGetFormDataLANSettingPage(); // save setting
			HTTPsendPageLANSetting();		 // reload LAN setting page
		}
		else
		{
			HTTPsendPageError();
		}
	}
	UECSclient.stop();
}

//////////////////
//// html ////////
//////////////////

void UDPReadCallback(AsyncUDPPacket packet)
{
}

void UECSupdate16520portReceive(UECSTEMPCCM *_tempCCM, unsigned long _millis)
{
	int packetSize = UECS_UDP16520.listen(16520);
	int matchCCMID = -1;
	if (packetSize)
	{
		ClearMainBuffer();
		UECS_UDP16520.onPacket([_tempCCM](AsyncUDPPacket packet)
							   { 
			_tempCCM->address = packet.remoteIP();
			UECSbuffer[packet.read((uint8_t *)UECSbuffer, BUF_SIZE - 1)] = '\0'; });
		UDPFilterToBuffer();

		if (UECSparseRec(_tempCCM, &matchCCMID))
		{
			UECScheckUpDate(_tempCCM, _millis, matchCCMID);
		}
	}
}

void UECSupdate16529port(UECSTEMPCCM *_tempCCM)
{
	int packetSize = UECS_UDP16529.listen(16529);
	if (packetSize)
	{
		ClearMainBuffer();
		UECS_UDP16529.onPacket([_tempCCM](AsyncUDPPacket packet)
							   { 
			_tempCCM->address = packet.remoteIP();
			UECSbuffer[packet.read((uint8_t *)UECSbuffer, BUF_SIZE - 1)] = '\0'; });
		UDPFilterToBuffer();

		if (UECSresNodeScan())
		{
			UECS_UDP16529.writeTo((const uint8_t *)UECSbuffer, sizeof(UECSbuffer), _tempCCM->address, 16529);
		}
	}
}
//----------------------------------

void UECSsetup()
{
	delay(500);
	pinMode(U_InitPin, INPUT_PULLUP);


	EEPROM.begin(1024);
	Serial.begin(115200);

	if (digitalRead(U_InitPin) == U_InitPin_Sense || UECS_EEPROM_readLong(EEPROM_OFFSET_IP) == -1)
	{
		U_orgAttribute.status |= STATUS_SAFEMODE;
		Serial.println("Wakeup with safe mode");
	}
	else
	{
		U_orgAttribute.status &= STATUS_SAFEMODE_MASK; // Release safemode
		Serial.println("Wakeup with CCM mode");
	}

	UECSinitOrgAttribute();
	UECSinitCCMList();
	UserInit();
	UECSstartEthernet();
}

void UECSstartEthernet()
{

	if (U_orgAttribute.status & STATUS_SAFEMODE)
	{
		if (!WiFi.config(defip, defip, defsubnet, defdummy))
		{ // ip,gateway,subnet,dns
			Serial.println("Failed to configure!");
		}
	}
	else
	{
		if (!WiFi.config(U_orgAttribute.ip, U_orgAttribute.gateway, U_orgAttribute.subnet, U_orgAttribute.dns))
		{ // ip,gateway,subnet,dns
			Serial.println("Failed to configure of WiFi");
		}
	}

	WiFi.begin(WiFi_SSID, WiFi_PASS);
	while (WiFi.status() != WL_CONNECTED)
	{
		delay(2000);
		Serial.println("WiFi connecting");
	}
	Serial.println("WiFi connected");
	Serial.println(WiFi.localIP());
	char ip[20], gateway[20], subnet[20];
	sprintf(ip, "%d.%d.%d.%d", U_orgAttribute.ip[0], U_orgAttribute.ip[1], U_orgAttribute.ip[2], U_orgAttribute.ip[3]);
	sprintf(gateway, "%d.%d.%d.%d", U_orgAttribute.gateway[0], U_orgAttribute.gateway[1], U_orgAttribute.gateway[2], U_orgAttribute.gateway[3]);
	sprintf(subnet, "%d.%d.%d.%d", U_orgAttribute.subnet[0], U_orgAttribute.subnet[1], U_orgAttribute.subnet[2], U_orgAttribute.subnet[3]);
	Serial.println(ip);
	Serial.println(gateway);
	Serial.println(subnet);
	UECSlogserver.begin();
}

//---------------------------------------------------------
void UECSresetEthernet()
{
	// UECS_UDP16520.close();
	// UECS_UDP16529.close();
	// SPI.end();
	UECSstartEthernet();
}
//------------------------------------------------------------------------
// Software reset command
// call 0
typedef int (*CALLADDR)(void);
void SoftReset(void)
{
	CALLADDR resetAddr = 0;
	(resetAddr)();
}
//---------------------------------------------------------
//---------------------------------------------------------
//----------------------------------------------------------------------

void UECSloop()
{

	// network Check
	// 1. http request
	// 2. udp16520Receive
	// 3. udp16529Receive and Send
	// << USER MANAGEMENT >>
	// 4. udp16520Send
	HTTPcheckRequest();
	UECSupdate16520portReceive(&UECStempCCM, UECSnowmillis);
	UECSupdate16529port(&UECStempCCM);
	UserEveryLoop();

	UECSnowmillis = millis();
	if (UECSnowmillis == UECSlastmillis)
	{
		return;
	}

	// how long elapsed?
	unsigned long td = UECSnowmillis - UECSlastmillis;
	// check overflow
	if (UECSnowmillis < UECSlastmillis)
	{
		td = (4294967295 - UECSlastmillis) + UECSnowmillis;
	}

	// over 1msec
	UECSsyscounter1s += td;
	UECSlastmillis = UECSnowmillis;
	UECSautomaticValidManager(td);

	if (UECSsyscounter1s < 999)
	{
		return;
	}
	// over 1000msec
	UECSsyscounter1s = 0;
	UECSsyscounter60s++;

	if (UECSsyscounter60s >= 60)
	{
		UserEveryMinute();
		UECSsyscounter60s = 0;
	}

	UECSautomaticSendManager();
	UserEverySecond();

	for (int i = 0; i < U_MAX_CCM; i++)
	{
		if (U_ccmList[i].sender && U_ccmList[i].flagStimeRfirst && U_ccmList[i].ccmLevel != NONE)
		{
			UECSCreateCCMPacketAndSend(&U_ccmList[i]);
			// U_ccmList[i].old_value=U_ccmList[i].value;
		}
	}
}
//------------------------------------------------------
void UECSinitOrgAttribute()
{

	for (int i = 0; i < 4; i++)
	{
		U_orgAttribute.ip[i] = EEPROM.read(i + EEPROM_OFFSET_IP);
		U_orgAttribute.subnet[i] = EEPROM.read(i + EEPROM_OFFSET_SUBNET);
		U_orgAttribute.gateway[i] = EEPROM.read(i + EEPROM_OFFSET_GATEWAY);
		U_orgAttribute.dns[i] = EEPROM.read(i + EEPROM_OFFSET_DNS);
	}

	// reset web form
	for (int i = 0; i < U_HtmlLine; i++)
	{
		*(U_html[i].data) = U_html[i].minValue;
	}

	U_orgAttribute.room = EEPROM.read(EEPROM_OFFSET_ROOM);
	U_orgAttribute.region = EEPROM.read(EEPROM_OFFSET_REGION);
	U_orgAttribute.order = EEPROM.read(EEPROM_OFFSET_ORDER_L) + (unsigned short)(EEPROM.read(EEPROM_OFFSET_ORDER_H)) * 256;
	if (U_orgAttribute.order > 30000)
	{
		U_orgAttribute.order = 30000;
	}

	if (U_orgAttribute.status & STATUS_SAFEMODE)
	{
		return;
	}

	for (int i = 0; i < 20; i++)
	{
		U_nodename[i] = EEPROM.read(EEPROM_OFFSET_NODENAME + i);
	}

	for (int i = 0; i < U_HtmlLine; i++)
	{
		*(U_html[i].data) = UECS_EEPROM_readLong(EEPROM_OFFSET_WEBDATA + i * 4);
	}
}
//------------------------------------------------------
void UECSinitCCMList()
{
	for (int i = 0; i < U_MAX_CCM; i++)
	{
		U_ccmList[i].ccmLevel = NONE;
		U_ccmList[i].validity = false;
		U_ccmList[i].flagStimeRfirst = false;
		U_ccmList[i].recmillis = 0;

		/*
		U_ccmList[i].address[0] = 255;
		U_ccmList[i].address[1] = 255;
		U_ccmList[i].address[2] = 255;
		U_ccmList[i].address[3] = 255;
		U_ccmList[i].typeTxt[0]='\0';
		U_ccmList[i].attribute[AT_ROOM] = 1;
		U_ccmList[i].attribute[AT_REGI] = 1;
		U_ccmList[i].attribute[AT_ORDE] = 1;
		U_ccmList[i].attribute[AT_PRIO] = 29;
		U_ccmList[i].baseAttribute[AT_ROOM] = U_orgAttribute.room;
		U_ccmList[i].baseAttribute[AT_REGI] = U_orgAttribute.region;
		U_ccmList[i].baseAttribute[AT_ORDE] = U_orgAttribute.order;
		U_ccmList[i].baseAttribute[AT_PRIO] = 29;
		*/
	}
}

void UECSsetCCM(boolean _sender, signed char _num, const char *_name, const char *_type, const char *_unit, unsigned short _priority, unsigned char _decimal, char _ccmLevel)
{
	if (_num > U_MAX_CCM || _num < 0)
	{
		return;
	}
	U_ccmList[_num].sender = _sender;
	U_ccmList[_num].ccmLevel = _ccmLevel;

	U_ccmList[_num].name = _name;
	U_ccmList[_num].type = _type;
	U_ccmList[_num].unit = _unit;
	U_ccmList[_num].baseAttribute[AT_ROOM] = U_orgAttribute.room;
	U_ccmList[_num].baseAttribute[AT_REGI] = U_orgAttribute.region;
	U_ccmList[_num].baseAttribute[AT_ORDE] = U_orgAttribute.order;
	U_ccmList[_num].attribute[AT_ROOM] = 0;
	U_ccmList[_num].attribute[AT_REGI] = 0;
	U_ccmList[_num].attribute[AT_ORDE] = 0;
	U_ccmList[_num].attribute[AT_PRIO] = _priority;
	U_ccmList[_num].decimal = _decimal;
	U_ccmList[_num].ccmLevel = _ccmLevel;
	U_ccmList[_num].address[0] = 255;
	U_ccmList[_num].address[1] = 255;
	U_ccmList[_num].address[2] = 255;
	U_ccmList[_num].address[3] = 255;
	return;
}

// ####################################String Buffer control
static int wp;
void ClearMainBuffer()
{
	UECSbuffer[0] = '\0';
	wp = 0;
}
//-----------------------------------
void UDPAddPGMCharToBuffer(const char *_romword)
{
	for (int i = 0; i <= MAXIMUM_STRINGLENGTH; i++)
	{
		UECSbuffer[wp] = pgm_read_byte(&_romword[i]);
		if (UECSbuffer[wp] == '\0')
		{
			break;
		}
		wp++;
	}

	MemoryWatching();
}
//-----------------------------------
void UDPAddCharToBuffer(char *word)
{
	strcat(UECSbuffer, word);
	wp = strlen(UECSbuffer);

	MemoryWatching();
}
//-----------------------------------
void UDPAddValueToBuffer(long value)
{
	sprintf(&UECSbuffer[wp], "%ld", value);
	wp = strlen(UECSbuffer);

	MemoryWatching();
}
//-----------------------------------
void HTTPAddPGMCharToBuffer(const char *_romword)
{
	for (int i = 0; i <= MAXIMUM_STRINGLENGTH; i++)
	{
		UECSbuffer[wp] = pgm_read_byte(&_romword[i]);
		if (UECSbuffer[wp] == '\0')
		{
			break;
		}
		wp++;
		// auto send
		if (wp > BUF_HTTP_REFRESH_SIZE)
		{
			HTTPRefreshBuffer();
		}
	}

	MemoryWatching();
}
//---------------------------------------------
void HTTPAddCharToBuffer(char *word)
{
	for (int i = 0; i <= MAXIMUM_STRINGLENGTH; i++)
	{
		UECSbuffer[wp] = word[i];
		if (UECSbuffer[wp] == '\0')
		{
			break;
		}
		wp++;
		// auto send
		if (wp > BUF_HTTP_REFRESH_SIZE)
		{
			HTTPRefreshBuffer();
		}
	}
	MemoryWatching();
}
//---------------------------------------------
void HTTPAddValueToBuffer(long value)
{
	sprintf(&UECSbuffer[wp], "%ld", value);
	wp = strlen(UECSbuffer);

	if (wp > BUF_HTTP_REFRESH_SIZE)
	{
		HTTPRefreshBuffer();
	}

	MemoryWatching();
}
void HTTPRefreshBuffer(void)
{

	UECSbuffer[wp] = '\0';
	UECSclient.print(UECSbuffer);
	ClearMainBuffer();
}
void HTTPCloseBuffer(void)
{

	if (strlen(UECSbuffer) > 0)
	{
		UECSclient.print(UECSbuffer);
	}
}
//------------------------------------
// delete \r,\n and contorol code
// replace the continuous space character to 1 space
// Attention:first one character is ignored without delete.
//------------------------------------
void UDPFilterToBuffer(void)
{

	int s_size = strlen(UECSbuffer);
	int write = 0, read = 0;
	int i, j;
	for (i = 1; i < s_size; i++)
	{
		if (UECSbuffer[i] == '\0')
		{
			break;
		}

		// find space
		if (UECSbuffer[i] < ASCIICODE_SPACE || (UECSbuffer[i] == ASCIICODE_SPACE && UECSbuffer[i - 1] == ASCIICODE_SPACE))
		{
			write = i;
			// find end of space
			for (j = write; j <= s_size; j++)
			{
				if (UECSbuffer[j] > ASCIICODE_SPACE || UECSbuffer[j] == '\0')
				{
					read = j;
					break;
				}
			}
			// copy str to fill space
			for (j = read; j <= s_size; j++)
			{
				UECSbuffer[write + (j - read)] = UECSbuffer[j];
			}
		}
	}
	MemoryWatching();
}
//------------------------------------
// delete space and contorol code for http response
//\r,\n is regarded to end
// Attention:first one character is ignored without delete.
// decode URL like %nn to char code
//------------------------------------
void HTTPFilterToBuffer(void)
{
	int s_size = strlen(UECSbuffer);
	int write = 0, read = 0;
	int i, j;

	// decode after %
	for (i = 1; i < s_size; i++)
	{
		if ((unsigned char)UECSbuffer[i] < ASCIICODE_SPACE || (UECSbuffer[i - 1] == '&' && UECSbuffer[i] == 'S' && UECSbuffer[i + 1] == '='))
		{
			UECSbuffer[i] = '\0';
			break;
		}

		if (UECSbuffer[i] == '%')
		{
			unsigned char decode = 0;
			if (UECSbuffer[i + 1] >= 'A' && UECSbuffer[i + 1] <= 'F')
			{
				decode = UECSbuffer[i + 1] + 10 - 'A';
			}
			else if (UECSbuffer[i + 1] >= 'a' && UECSbuffer[i + 1] <= 'f')
			{
				decode = UECSbuffer[i + 1] + 10 - 'a';
			}
			else if (UECSbuffer[i + 1] >= '0' && UECSbuffer[i + 1] <= '9')
			{
				decode = UECSbuffer[i + 1] - '0';
			}

			if (decode != 0)
			{
				decode = decode * 16;
				if (UECSbuffer[i + 2] >= 'A' && UECSbuffer[i + 2] <= 'F')
				{
					decode += UECSbuffer[i + 2] + 10 - 'A';
				}
				else if (UECSbuffer[i + 2] >= 'a' && UECSbuffer[i + 2] <= 'f')
				{
					decode += UECSbuffer[i + 2] + 10 - 'a';
				}
				else if (UECSbuffer[i + 2] >= '0' && UECSbuffer[i + 2] <= '9')
				{
					decode += UECSbuffer[i + 2] - '0';
				}
				else
				{
					decode = 0;
				}

				if (decode != 0)
				{
					if (decode == '&')
					{
						decode = '*';
					}
					UECSbuffer[i] = decode;
					UECSbuffer[i + 1] = ' ';
					UECSbuffer[i + 2] = ' ';
				}
			}
		}
	}

	s_size = strlen(UECSbuffer);

	for (i = 1; i < s_size; i++)
	{
		// eliminate tag
		// if(UECSbuffer[i]=='<' || UECSbuffer[i]=='>'){UECSbuffer[i]=' ';}

		// find space
		if (UECSbuffer[i] <= ASCIICODE_SPACE)
		{
			write = i;
			// find end of space
			for (j = write; j <= s_size; j++)
			{
				if ((unsigned char)(UECSbuffer[j]) > ASCIICODE_SPACE || UECSbuffer[j] == '\0')
				{
					read = j;
					break;
				}
			}
			// copy str to fill space
			for (j = read; j <= s_size; j++)
			{
				UECSbuffer[write + (j - read)] = UECSbuffer[j];
			}
		}
	}
	MemoryWatching();
}

//------------------------------------
bool UECSFindPGMChar(char *targetBuffer, const char *_romword_startStr, int *lastPos)
{
	*lastPos = 0;
	int startPos = -1;
	int _targetBuffersize = strlen(targetBuffer);
	int _startStrsize = strlen_P(_romword_startStr);
	if (_targetBuffersize < _startStrsize)
	{
		return false;
	}

	int i, j;

	//-------------start string check
	unsigned char startchr = pgm_read_byte(&_romword_startStr[0]);
	for (i = 0; i < _targetBuffersize - _startStrsize + 1; i++)
	{
		// not hit
		if (targetBuffer[i] != startchr)
		{
			continue;
		}

		// if hit 1 chr ,more check
		for (j = 0; j < _startStrsize; j++)
		{
			if (targetBuffer[i + j] != pgm_read_byte(&_romword_startStr[j]))
			{
				break;
			} // not hit!
		}
		// hit all chr
		if (j == _startStrsize)
		{
			startPos = i;
			break;
		}
	}

	if (startPos < 0)
	{
		return false;
	}
	*lastPos = startPos + _startStrsize;
	return true;
}

//------------------------------------
bool UECSGetValPGMStrAndChr(char *targetBuffer, const char *_romword_startStr, char end_asciiCode, short *shortValue, int *lastPos)
{
	int _targetBuffersize = strlen(targetBuffer);
	*lastPos = -1;
	int startPos = -1;
	int i;
	if (!UECSFindPGMChar(targetBuffer, _romword_startStr, &startPos))
	{
		false;
	}

	if (targetBuffer[startPos] < '0' || targetBuffer[startPos] > '9')
	{
		return false;
	}
	//------------end code check
	for (i = startPos; i < _targetBuffersize; i++)
	{
		if (targetBuffer[i] == '\0')
		{
			return false;
		} // no end code found
		if (targetBuffer[i] == end_asciiCode)
		{
			break;
		}
	}

	if (i >= _targetBuffersize)
	{
		return false;
	} // not found
	*lastPos = i;

	//*shortValue=UECSAtoI(&targetBuffer[startPos]);
	long longVal;
	unsigned char decimal;
	int progPos;
	if (!(UECSGetFixedFloatValue(&targetBuffer[startPos], &longVal, &decimal, &progPos)))
	{
		return false;
	}

	if (decimal != 0)
	{
		return false;
	}
	*shortValue = (short)longVal;
	if (*shortValue != longVal)
	{
		return false;
	} // over flow!

	return true;
}

/*
//------------------------------------
bool UECSGetValueBetweenChr(char* targetBuffer,char start_asciiCode, char end_asciiCode,short *shortValue,int *lastPos)
{
int _targetBuffersize=strlen(targetBuffer);
*lastPos=-1;
int startPos=-1;
int i;

//-------------start string check
for(i=0;i<_targetBuffersize;i++)
	{
	if(targetBuffer[i]==start_asciiCode){startPos=i+1;break;}
	}

*lastPos=i;
if(startPos<0){return false;}

if(targetBuffer[startPos]<'0' || targetBuffer[startPos]>'9'){return false;}

//------------end code check
for(i=startPos;i<_targetBuffersize;i++)
{
if(targetBuffer[i]=='\0'){return false;}//no end code found
if(targetBuffer[i]==end_asciiCode)
	{break;}
}

if(i>=_targetBuffersize){return false;}//not found
*lastPos=i;
*shortValue=atoi(&targetBuffer[startPos]);
return true;

}
*/
bool UECSGetIPAddress(char *targetBuffer, unsigned char *ip, int *lastPos)
{
	int _targetBuffersize = strlen(targetBuffer);
	int i;
	int progPos = 0;
	(*lastPos) = 0;

	// find first number
	for ((*lastPos); i < _targetBuffersize; (*lastPos)++)
	{
		if (targetBuffer[(*lastPos)] >= '0' && targetBuffer[(*lastPos)] <= '9')
		{
			break;
		}
	}
	if ((*lastPos) == _targetBuffersize)
	{
		return false;
	} // number not found

	// decode ip address
	for (i = 0; i <= 2; i++)
	{
		if (!UECSAtoI_UChar(&targetBuffer[(*lastPos)], &ip[i], &progPos))
		{
			return false;
		}
		(*lastPos) += progPos;
		if (targetBuffer[(*lastPos)] != '.')
		{
			return false;
		}
		(*lastPos)++;
	}

	// last is not '.'
	if (!UECSAtoI_UChar(&targetBuffer[(*lastPos)], &ip[3], &progPos))
	{
		return false;
	}
	(*lastPos) += progPos;

	return true;
}

//------------------------------------------------------------------------------
bool UECSAtoI_UChar(char *targetBuffer, unsigned char *ucharValue, int *lastPos)
{
	unsigned int newValue = 0;
	bool validFlag = false;

	int i;

	for (i = 0; i < MAX_DIGIT; i++)
	{
		if (targetBuffer[i] >= '0' && targetBuffer[i] <= '9')
		{
			validFlag = true;
			newValue = newValue * 10;
			newValue += (targetBuffer[i] - '0');
			if (newValue > 255)
			{
				return false;
			} // over flow!
			continue;
		}

		break;
	}

	*lastPos = i;
	*ucharValue = newValue;

	return validFlag;
}
//------------------------------------------------------------------------------
bool UECSGetFixedFloatValue(char *targetBuffer, long *longVal, unsigned char *decimal, int *lastPos)
{
	*longVal = 0;
	*decimal = 0;
	int i;
	int validFlag = 0;
	bool floatFlag = false;
	char signFlag = 1;
	unsigned long newValue = 0;
	unsigned long prvValue = 0;

	for (i = 0; i < MAX_DIGIT; i++)
	{
		if (targetBuffer[i] == '.')
		{
			if (floatFlag)
			{
				return false;
			} // Multiple symbols error
			floatFlag = true;
			continue;
		}
		else if (targetBuffer[i] == '-')
		{
			if (validFlag)
			{
				return false;
			} // Symbol is after number
			if (signFlag < 0)
			{
				return false;
			} // Multiple symbols error
			signFlag = -1;
			continue;
		}
		else if (targetBuffer[i] >= '0' && targetBuffer[i] <= '9')
		{

			validFlag = true;
			if (floatFlag)
			{
				(*decimal)++;
			}
			newValue = newValue * 10;
			newValue += (targetBuffer[i] - '0');
			if (prvValue > newValue || newValue > 2147483646)
			{
				return false;
			} // over flow!
			prvValue = newValue;
			continue;
		}

		break;
	}

	*longVal = ((long)newValue) * signFlag;
	*lastPos = i;

	return validFlag;
}

//------------------------------------
void MemoryWatching()
{
	if (UECStempStr20[MAX_TYPE_CHAR - 1] != 0 || UECSbuffer[BUF_SIZE - 1] != 0)
	{
		U_orgAttribute.status |= STATUS_MEMORY_LEAK;
	}
}
//------------------------------------
// In Safemode this function is not working properly because Web value will be reset.
bool ChangeWebValue(signed long *data, signed long value)
{
	for (int i = 0; i < U_HtmlLine; i++)
	{
		if (U_html[i].data == data)
		{
			*(U_html[i].data) = value;
			UECS_EEPROM_writeLong(EEPROM_OFFSET_WEBDATA + i * 4, *(U_html[i].data));
			return true;
		}
	}
	return false;
}
