// MQ2HUD.cpp : Defines the entry point for the DLL application.
//
// Cr4zyb4rd patch: 1/7/2005
// Updated Sep 09 2017 by eqmule to take undeclared macro variables into account

#include "../MQ2Plugin.h"
HANDLE hHudLock = 0;
bool bEQHasFocus=true;

typedef struct _HUDELEMENT {
    DWORD Type;
    DWORD Size;
    LONG X;
    LONG Y;
    DWORD Color;
    CHAR Text[MAX_STRING];
    CHAR PreParsed[MAX_STRING];
    struct _HUDELEMENT *pNext;
} HUDELEMENT, *PHUDELEMENT;

#define HUDTYPE_NORMAL      1
#define HUDTYPE_FULLSCREEN  2
#define HUDTYPE_CURSOR      4
#define HUDTYPE_CHARSELECT  8
#define HUDTYPE_MACRO      16

PreSetup("MQ2HUD");

PHUDELEMENT pHud=0;
struct _stat LastRead;

char HUDNames[MAX_STRING]="Elements";
char HUDSection[MAX_STRING]="MQ2HUD";
int SkipParse=1;
int CheckINI=10;
bool bBGUpdate = true;
bool bClassHUD = true;
bool bZoneHUD = true;
bool bUseFontSize = false;

BOOL Stat(PCHAR Filename, struct _stat &Dest)
{
	int client = 0;
	errno_t err = _sopen_s(&client, Filename, _O_RDONLY,_SH_DENYNO, _S_IREAD | _S_IWRITE);
    if (err) {
        return FALSE;
    }
    _fstat(client,&Dest);
    _close(client);
    return TRUE;
}

VOID ClearElements()
{
	lockit lk(hHudLock,"HudLock");
    while(pHud)
    {
        PHUDELEMENT pNext=pHud->pNext;
        delete pHud;
        pHud=pNext;
    }
}

VOID AddElement(PCHAR IniString)
{
	lockit lk(hHudLock,"HudLock");
    LONG X;
    LONG Y;
    DWORD Type;
    ARGBCOLOR Color;
    Color.A=0xFF;
    // x,y,color,string
    PCHAR pComma;
    DWORD Size;

    pComma=strchr(IniString,',');
    if (!pComma)
        return;
    *pComma=0;
    Type=atoi(IniString);
    IniString=&pComma[1];

    if(bUseFontSize)
    {
        pComma=strchr(IniString,',');
        if (!pComma)
            return;
        *pComma=0;
        Size=atoi(IniString);
        IniString=&pComma[1];
    }
    else
    {
        Size=2;
    }

    pComma=strchr(IniString,',');
    if (!pComma)
        return;
    *pComma=0;
    X=atoi(IniString);
    IniString=&pComma[1];

    //y
    pComma=strchr(IniString,',');
    if (!pComma)
        return;
    *pComma=0;
    Y=atoi(IniString);
    IniString=&pComma[1];

    //color R
    pComma=strchr(IniString,',');
    if (!pComma)
        return;
    *pComma=0;
    Color.R=atoi(IniString);
    IniString=&pComma[1];

    //color G
    pComma=strchr(IniString,',');
    if (!pComma)
        return;
    *pComma=0;
    Color.G=atoi(IniString);
    IniString=&pComma[1];

    //color B
    pComma=strchr(IniString,',');
    if (!pComma)
        return;
    *pComma=0;
    Color.B=atoi(IniString);
    IniString=&pComma[1];

    //string
    if (!IniString[0])
        return;
    PHUDELEMENT pElement=new HUDELEMENT;
    pElement->pNext=pHud;
    pHud=pElement;
    pElement->Type=Type;
    pElement->Color=Color.ARGB;
    pElement->X=X;
    pElement->Y=Y;
    strcpy_s(pElement->Text,IniString);
	ZeroMemory(pElement->PreParsed,sizeof(pElement->PreParsed));
    pElement->Size=Size;

    DebugSpew("New element '%s' in color %X",pElement->Text,pElement->Color);
}

VOID LoadElements()
{
    ClearElements();
	lockit lk(hHudLock,"HudLock");
    CHAR ElementList[MAX_STRING*10] = {0};
    CHAR szBuffer[MAX_STRING], CurrentHUD[MAX_STRING] = {0};
    CHAR ClassDesc[MAX_STRING], ZoneName[MAX_STRING] = {0};
    int argn=1;
    GetArg(CurrentHUD,HUDNames,argn,0,0,0,',');
    while (*CurrentHUD) {
        GetPrivateProfileString(CurrentHUD,NULL,"",ElementList,MAX_STRING*10,INIFileName);
        PCHAR pElementList = ElementList;
        while (pElementList[0]!=0) {
            GetPrivateProfileString(CurrentHUD,pElementList,"",szBuffer,MAX_STRING,INIFileName);
            if (szBuffer[0]!=0) {
                AddElement(szBuffer);
            }
            pElementList+=strlen(pElementList)+1;
        }
        GetArg(CurrentHUD,HUDNames,++argn,0,0,0,',');
    }
    if (gGameState==GAMESTATE_INGAME) {
        if (bClassHUD && ((ppCharData) && (pCharData))) {
			if (PCHARINFO2 pChar2 = GetCharInfo2()) {
				sprintf_s(ClassDesc, "%s", GetClassDesc(pChar2->Class));
				GetPrivateProfileString(ClassDesc, NULL, "", ElementList, MAX_STRING * 10, INIFileName);
				PCHAR pElementList = ElementList;
				while (pElementList[0] != 0) {
					GetPrivateProfileString(ClassDesc, pElementList, "", szBuffer, MAX_STRING, INIFileName);
					if (szBuffer[0] != 0) {
						AddElement(szBuffer);
					}
					pElementList += strlen(pElementList) + 1;
				}
			}
        }
        if (bZoneHUD && (pZoneInfo)) {
            sprintf_s(ZoneName,"%s",((PZONEINFO)pZoneInfo)->LongName);
            GetPrivateProfileString(ZoneName,NULL,"",ElementList,MAX_STRING*10,INIFileName);
            PCHAR pElementList = ElementList;
            while (pElementList[0]!=0) {
                GetPrivateProfileString(ZoneName,pElementList,"",szBuffer,MAX_STRING,INIFileName);
                if (szBuffer[0]!=0) {
                    AddElement(szBuffer);
                }
                pElementList+=strlen(pElementList)+1;
            }
        }
    }

    if (!Stat(INIFileName,LastRead))
        ZeroMemory(&LastRead,sizeof(struct _stat));
}
template <unsigned int _Size>LPSTR SafeItoa(int _Value,char(&_Buffer)[_Size], int _Radix)
{
	errno_t err = _itoa_s(_Value, _Buffer, _Radix);
	if (!err) {
		return _Buffer;
	}
	return "";
}
VOID HandleINI()
{
	lockit lk(hHudLock,"HudLock");
    CHAR szBuffer[MAX_STRING] = {0};
    WritePrivateProfileString(HUDSection,"Last",HUDNames,INIFileName);
    SkipParse = GetPrivateProfileInt(HUDSection,"SkipParse",1,INIFileName);
    SkipParse = SkipParse < 1 ? 1 : SkipParse;
    CheckINI = GetPrivateProfileInt(HUDSection,"CheckINI",10,INIFileName);
    CheckINI = CheckINI < 10 ? 10 : CheckINI;
    GetPrivateProfileString(HUDSection,"UpdateInBackground","on",szBuffer,MAX_STRING,INIFileName);
    bBGUpdate = _strnicmp(szBuffer,"on",2)?false:true;
    GetPrivateProfileString(HUDSection,"ClassHUD","on",szBuffer,MAX_STRING,INIFileName);
    bClassHUD = _strnicmp(szBuffer,"on",2)?false:true;
    GetPrivateProfileString(HUDSection,"ZoneHUD","on",szBuffer,MAX_STRING,INIFileName);
    bZoneHUD = _strnicmp(szBuffer,"on",2)?false:true;
    GetPrivateProfileString("MQ2HUD","UseFontSize","off",szBuffer,MAX_STRING,INIFileName);
    bUseFontSize = _strnicmp(szBuffer,"on",2)?false:true;
    // Write the SkipParse and CheckINI section, in case they didn't have one
    WritePrivateProfileString(HUDSection,"SkipParse",SafeItoa(SkipParse,szBuffer,10),INIFileName);
    WritePrivateProfileString(HUDSection,"CheckINI",SafeItoa(CheckINI,szBuffer,10),INIFileName);
    WritePrivateProfileString(HUDSection,"UpdateInBackground",bBGUpdate?"on":"off",INIFileName);
    WritePrivateProfileString(HUDSection,"ClassHUD",bClassHUD?"on":"off",INIFileName);
    WritePrivateProfileString(HUDSection,"ZoneHUD",bZoneHUD?"on":"off",INIFileName);
    WritePrivateProfileString("MQ2HUD","UseFontSize",bUseFontSize?"on":"off",INIFileName);

    LoadElements();
}

VOID DefaultHUD(PSPAWNINFO pChar, PCHAR szLine)
{
    strcpy_s(HUDNames, "Elements");
    HandleINI();
}

VOID LoadHUD(PSPAWNINFO pChar, PCHAR szLine)
{
    CHAR HUDTemp[MAX_STRING] = {0};
    CHAR CurrentHUD[MAX_STRING];
    int argn=1;
    GetArg(CurrentHUD,HUDNames,argn,0,0,0,',');
    while (*CurrentHUD) {
        if (!strcmp(CurrentHUD,szLine)) {
            WriteChatf("Hud \"%s\" already loaded",szLine);
            return;
        }
        if (HUDTemp[0]) strcat_s(HUDTemp,",");
        strcat_s(HUDTemp,CurrentHUD);
        GetArg(CurrentHUD,HUDNames,++argn,0,0,0,',');
    }
    if (HUDTemp[0]) strcat_s(HUDTemp,",");
    strcat_s(HUDTemp,szLine);
    strcpy_s(HUDNames,HUDTemp);
    HandleINI();
}

VOID UnLoadHUD(PSPAWNINFO pChar, PCHAR szLine)
{
    CHAR HUDTemp[MAX_STRING] = {0};
    CHAR CurrentHUD[MAX_STRING];
    bool found=false;
    int argn=1;
    GetArg(CurrentHUD,HUDNames,argn,0,0,0,',');
    while (*CurrentHUD) {
        if (!strcmp(CurrentHUD,szLine)) {
            found=true;
        } else {
            if (HUDTemp[0]) strcat_s(HUDTemp,",");
            strcat_s(HUDTemp,CurrentHUD);
        }
        GetArg(CurrentHUD,HUDNames,++argn,0,0,0,',');
    }
    strcpy_s(HUDNames,HUDTemp);

    if (!found) WriteChatf("Hud \"%s\" not loaded",szLine);

    HandleINI();
}

VOID BackgroundHUD(PSPAWNINFO pChar, PCHAR szLine)
{
    if (!_stricmp(szLine,"off"))
    {
        WritePrivateProfileString(HUDSection,"UpdateInBackground","off",INIFileName);
        WriteChatColor("MQ2HUD::Background updates are OFF");
    }
    else if (!_stricmp(szLine,"on"))
    {
        WritePrivateProfileString(HUDSection,"UpdateInBackground","on",INIFileName);
        WriteChatColor("MQ2HUD::Background updates are ON");
    }
    else
    {
        WriteChatColor("Usage: /backgroundhud [on|off]");
        return;
    }
    HandleINI();
}

VOID ClassHUD(PSPAWNINFO pChar, PCHAR szLine)
{
    if (!_stricmp(szLine,"off"))
    {
        WritePrivateProfileString(HUDSection,"ClassHUD","off",INIFileName);
        WriteChatColor("MQ2HUD::Auto-include HUD per class description is OFF");
    }
    else if (!_stricmp(szLine,"on"))
    {
        WritePrivateProfileString(HUDSection,"ClassHUD","on",INIFileName);
        WriteChatColor("MQ2HUD::Auto-include HUD per class description is ON");
    }
    else
    {
        WriteChatColor("Usage: /classhud [on|off]");
        return;
    }
    HandleINI();
}

VOID ZoneHUD(PSPAWNINFO pChar, PCHAR szLine)
{
    if (!_stricmp(szLine,"off"))
    {
        WritePrivateProfileString(HUDSection,"ZoneHUD","off",INIFileName);
        WriteChatColor("MQ2HUD::Auto-include HUD per zone name is OFF");
    }
    else if (!_stricmp(szLine,"on"))
    {
        WritePrivateProfileString(HUDSection,"ZoneHUD","on",INIFileName);
        WriteChatColor("MQ2HUD::Auto-include HUD per zone name is ON");
    }
    else
    {
        WriteChatColor("Usage: /zonehud [on|off]");
        return;
    }
    HandleINI();
}

BOOL dataHUD(PCHAR szIndex, MQ2TYPEVAR &Ret)
{
    Ret.Ptr=HUDNames;
    Ret.Type=pStringType;
    return true;
}

// Called once, when the plugin is to initialize
PLUGIN_API VOID InitializePlugin(VOID)
{
	hHudLock = CreateMutex(NULL, FALSE, NULL);
	
    CHAR szBuffer[MAX_STRING] = {0};
    DebugSpewAlways("Initializing MQ2HUD");

    GetPrivateProfileString(HUDSection,"Last","Elements",HUDNames,MAX_STRING,INIFileName);
    HandleINI();

    AddCommand("/defaulthud",DefaultHUD);
    AddCommand("/loadhud",LoadHUD);
    AddCommand("/unloadhud",UnLoadHUD);
    AddCommand("/backgroundhud",BackgroundHUD);
    AddCommand("/classhud",ClassHUD);
    AddCommand("/zonehud",ZoneHUD);
    AddMQ2Data("HUD",dataHUD);
}

// Called once, when the plugin is to shutdown
PLUGIN_API VOID ShutdownPlugin(VOID)
{
    DebugSpewAlways("Shutting down MQ2HUD");
    ClearElements();
 	lockit lk(hHudLock,"HudLock");
	RemoveCommand("/loadhud");
    RemoveCommand("/unloadhud");
    RemoveCommand("/defaulthud");
    RemoveCommand("/backgroundhud");
    RemoveCommand("/classhud");
    RemoveCommand("/zonehud");
    RemoveMQ2Data("HUD");
	if (hHudLock) {
		ReleaseMutex(hHudLock);
		CloseHandle(hHudLock);
		hHudLock = 0;
	}
}

PLUGIN_API VOID SetGameState(DWORD GameState)
{
    if (GameState==GAMESTATE_INGAME)
        sprintf_s(HUDSection,"%s_%s",GetCharInfo()->Name,EQADDR_SERVERNAME);
    else 
        strcpy_s(HUDSection,"MQ2HUD");
    GetPrivateProfileString(HUDSection,"Last","Elements",HUDNames,MAX_STRING,INIFileName);
    HandleINI();
}

// Called after entering a new zone
PLUGIN_API VOID OnZoned(VOID)
{
    if (bZoneHUD) HandleINI();
}

BOOL ParseMacroLine(PCHAR szOriginal, SIZE_T BufferSize,std::list<std::string>&out)
{
	// find each {}
	PCHAR pBrace = strstr(szOriginal, "${");
	if (!pBrace)
		return false;
	unsigned long NewLength;
	BOOL Changed = false;
	//PCHAR pPos;
	//PCHAR pStart;
	//PCHAR pIndex;
	CHAR szCurrent[MAX_STRING] = { 0 };
	//MQ2TYPEVAR Result = { 0 };
	do
	{
		// find this brace's end
		PCHAR pEnd = &pBrace[1];
		BOOL Quote = false;
		BOOL BeginParam = false;
		int nBrace = 1;
		while (nBrace)
		{
			++pEnd;
			if (BeginParam)
			{
				BeginParam = false;
				if (*pEnd == '\"')
				{
					Quote = true;
				}
				continue;
			}
			if (*pEnd == 0)
			{// unmatched brace or quote
				goto pmdbottom;
			}
			if (Quote)
			{
				if (*pEnd == '\"')
				{
					if (pEnd[1] == ']' || pEnd[1] == ',')
					{
						Quote = false;
					}
				}
			}
			else
			{
				if (*pEnd == '}')
				{
					nBrace--;
				}
				else if (*pEnd == '{')
				{
					nBrace++;
				}
				else if (*pEnd == '[' || *pEnd == ',')
					BeginParam = true;
			}

		}
		*pEnd = 0;
		strcpy_s(szCurrent, &pBrace[2]);
		if (szCurrent[0] == 0)
		{
			goto pmdbottom;
		}
		if (ParseMacroLine(szCurrent, sizeof(szCurrent),out))
		{
			unsigned long NewLength = strlen(szCurrent);
			memmove(&pBrace[NewLength + 1], &pEnd[1], strlen(&pEnd[1]) + 1);
			int addrlen = (int)(pBrace - szOriginal);
			memcpy_s(pBrace, BufferSize - addrlen, szCurrent, NewLength);
			pEnd = &pBrace[NewLength];
			*pEnd = 0;
		}
		if(!strchr(szCurrent,'[') && !strchr(szCurrent,'.'))
			out.push_back(szCurrent);

		NewLength = strlen(szCurrent);
		memmove(&pBrace[NewLength], &pEnd[1], strlen(&pEnd[1]) + 1);
		int addrlen = (int)(pBrace - szOriginal);
		memcpy_s(pBrace, BufferSize - addrlen, szCurrent, NewLength);
		if (bAllowCommandParse == false) {
			bAllowCommandParse = true;
			Changed = false;
			break;
		}
		else {
			Changed = true;
		}
	pmdbottom:;
	} while (pBrace = strstr(&pBrace[1], "${"));
	if (Changed)
		while (ParseMacroLine(szOriginal, BufferSize,out))
		{
			Sleep(0);
		}
	return Changed;
}
// Called every frame that the "HUD" is drawn -- e.g. net status / packet loss bar
PLUGIN_API VOID OnDrawHUD(VOID)
{
	if (hHudLock == 0)
		return;
	lockit lk(hHudLock,"HudLock");
	static bool bOkToCheck = true;
    static int N=0;
    CHAR szBuffer[MAX_STRING]={0};

    if (++N>CheckINI)
    {
        N=0;
        struct _stat now;
        if (Stat(INIFileName,now) && now.st_mtime!=LastRead.st_mtime)
            HandleINI();

        // check for EQ in foreground
		if (!bBGUpdate && !gbInForeground)
            bEQHasFocus = false;
        else
            bEQHasFocus = true;
    }
    if (!bEQHasFocus) return;

    DWORD SX=0;
    DWORD SY=0;
    if (pScreenX && pScreenY)
    {
        SX=ScreenX;
        SY=ScreenY;
    }   
    PHUDELEMENT pElement=pHud;
	bool bCheckParse = !(N % SkipParse);

    DWORD X,Y;
    while(pElement)
    {
        if ((gGameState==GAMESTATE_CHARSELECT && pElement->Type&HUDTYPE_CHARSELECT) ||
            (gGameState==GAMESTATE_INGAME && (
            (pElement->Type&HUDTYPE_NORMAL && ScreenMode!=3) ||
            (pElement->Type&HUDTYPE_FULLSCREEN && ScreenMode==3))))
        {
            if (pElement->Type&HUDTYPE_CURSOR)
            {
                PMOUSEINFO pMouse = (PMOUSEINFO)EQADDR_MOUSE;
                X=pMouse->X+pElement->X;
                Y=pMouse->Y+pElement->Y;
            }
            else
            {
                X=SX+pElement->X;
                Y=SX+pElement->Y;
            }
            //if (!(N%SkipParse)) {
			if (bCheckParse) {
				bOkToCheck = true;
                strcpy_s(pElement->PreParsed,pElement->Text);
				if (pElement->Type & HUDTYPE_MACRO) {
					if (gRunning) {
						CHAR szTemp[MAX_STRING] = { 0 };
						strcpy_s(szTemp, pElement->Text);
						std::list<std::string>out;
						ParseMacroLine(szTemp, MAX_STRING, out);
						if (out.size()) {
							for (std::list<std::string>::iterator i = out.begin(); i != out.end(); i++) {
								bOkToCheck = false;
								PCHAR pChar = (PCHAR)(*i).c_str();
								if (FindMQ2Data(pChar)) {
									bOkToCheck = true;
									continue;
								}
								if (!bOkToCheck)
								{
									//ok fine we didnt find it in the tlo map...
									//lets check variables
									if (FindMQ2DataVariable(pChar)) {
										bOkToCheck = true;
										continue;
									}
								}
								if (!bOkToCheck)
								{
									//still not found...
									break;
								}
							}
						}
					}
				}
				if(bOkToCheck) {
					if (PCHARINFO pChar = GetCharInfo()) {
						ParseMacroParameter(pChar->pSpawn, pElement->PreParsed);
					}
				}
				else {
					pElement->PreParsed[0] = '\0';
				}
            }
            strcpy_s(szBuffer,pElement->PreParsed);
            if (szBuffer[0] && strcmp(szBuffer,"NULL"))
            {
                DrawHUDText(szBuffer,X,Y,pElement->Color,pElement->Size);
            }
        }
        pElement=pElement->pNext;
    }
}
