// MQ2EQBugFix.cpp : Defines the entry point for the DLL application.
//

// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup, do NOT do it in DllMain.

#include "../MQ2Plugin.h"

PreSetup("MQ2EQBugFix");
DWORD p2DPrimitiveManager = 0;
DWORD pRender = 0;

//A1B408 ; int __cdecl startworddisplayexceptionhandler()
int __cdecl startworddisplayexceptionhandler_Trampoline(struct EHExceptionRecord *, struct EHRegistrationNode *, struct _CONTEXT *, void *);   
int __cdecl startworddisplayexceptionhandler_Detour(struct EHExceptionRecord *A, struct EHRegistrationNode *B, struct _CONTEXT *C, void *D)
{
	MessageBox(NULL, "WHY!!?", "Crashed in startworddisplay", MB_SYSTEMMODAL | MB_OK);
	//DebugBreak();
	int ret = startworddisplayexceptionhandler_Trampoline(A, B, C, D);
	return ret;
}
DETOUR_TRAMPOLINE_EMPTY(int __cdecl startworddisplayexceptionhandler_Trampoline(struct EHExceptionRecord *, struct EHRegistrationNode *, struct _CONTEXT *, void *));
DWORD g_bDeviceReady = 0;
#include <d3d9.h>
class CDisplay_Hook
{
public:
    int is_3dON_Trampoline();   
    int is_3dON_Detour()
    {
        if(!this)
        {
            DebugSpew("MQ2EQBugFix::Crash avoided!");
            return 0;
        }
        return is_3dON_Trampoline();
    }
	HRESULT Reset_Tramp(D3DPRESENT_PARAMETERS *);
	HRESULT Reset_Detour(D3DPRESENT_PARAMETERS *pPresentationParameters)
	{
		MessageBox(NULL, "Reset_Detour", "Reset_Detour", MB_SYSTEMMODAL | MB_OK);
		HRESULT ret = Reset_Tramp(pPresentationParameters);
		if (ret != D3D_OK)
		{
			Sleep(0);
		}
		return ret;
	}
	void UpdateDisplay_Tramp();
	void UpdateDisplay_Detour()
	{
		//DWORD TH = (DWORD)this;
		if (pRender) {
			if (DWORD addr = *(DWORD*)pRender) {
				if (DWORD D3DDevice = *(DWORD*)(addr + 0xec8)) {
					if (*(DWORD*)D3DDevice) {
						return UpdateDisplay_Tramp();
					}
				}
			}
		}
		*(BYTE*)g_bDeviceReady = 0;
		return;
	}
};
DETOUR_TRAMPOLINE_EMPTY(int CDisplay_Hook::is_3dON_Trampoline());
DETOUR_TRAMPOLINE_EMPTY(void CDisplay_Hook::UpdateDisplay_Tramp());
DETOUR_TRAMPOLINE_EMPTY(HRESULT CDisplay_Hook::Reset_Tramp(D3DPRESENT_PARAMETERS *));

#ifdef EQMULETESTINGSTUFF
#define startworlddisplayexceptionhandler_x 0xA1B408
#define INITIALIZE_EQGAME_OFFSET(var) DWORD var = (((DWORD)var##_x - 0x400000) + (DWORD)GetModuleHandle(NULL))
INITIALIZE_EQGAME_OFFSET(startworlddisplayexceptionhandler);
#endif
inline bool _DataCompare(const unsigned char* pData, const unsigned char* bMask, const char* szMask)
{
    for(;*szMask;++szMask,++pData,++bMask)
        if(*szMask=='x' && *pData!=*bMask )
            return false;
    return (*szMask) == 0;
}

unsigned long _FindPattern(unsigned long dwAddress,unsigned long dwLen,unsigned char *bPattern,char * szMask)
{
    for(unsigned long i=0; i < dwLen; i++)
        if(_DataCompare( (unsigned char*)( dwAddress+i ),bPattern,szMask) )
            return (unsigned long)(dwAddress+i);
   
    return 0;
}
unsigned long _GetDWordAt(unsigned long address, unsigned long numBytes)
{
    if(address)
    {
        address += numBytes;
        if(*(unsigned long*)address)
            return *(unsigned long*)address;
    }
    return 0;
}
PBYTE lcPattern = (PBYTE)"\x8b\x44\x24\x04\x8b\x0d\x00\x00\x00\x00\x50\xe8\x00\xe8\xff\xff";
char lcMask[] = "xxxxxx????xx?xxx";

PBYTE drPattern = (PBYTE)"\x80\x3d\x00\x00\x00\x00\x00\x74\x00\x8b\x44\x24\x10\x56\x8b\x70\x08\x85\xf6\x74";
char drMask[] = "xx????xx?xxxxxxxxxxx";

DWORD switchbug = 0;

int __cdecl CachedTextBug_Tramp(class CTextObject *);
int __cdecl CachedTextBug_Detour(class CTextObject *obj)
{
	if (p2DPrimitiveManager && *(DWORD*)p2DPrimitiveManager) {
		return CachedTextBug_Tramp(obj);
	}
	return 0;
}
DETOUR_TRAMPOLINE_EMPTY(int __cdecl CachedTextBug_Tramp(class CTextObject *));
DWORD __UpdateDisplay = 0;
DWORD __Reset = 0;
PLUGIN_API VOID InitializePlugin(VOID)
{
	//dont mess with this its work in progress, im sick of the crash that happens if you invoke the UAC dialog while eq is loading
	//basically it makes the render device get lost and then we crash
	//the proper handling would be to detect UAC or device lost and then reset it, but its not done
	//and if it is, its in the wrong place or it misses it
	//also, if the device is lost then we cant simply do d3device->Reset because well... d3device is NULL so that would crash us...
	//something to think more on...
	//8B 44 24 04 8B 0D ?? ?? ?? 10 50 E8 ?? E8 FF FF
	/*MessageBox(NULL, "break in", "eqbugfix", MB_SYSTEMMODAL | MB_OK);
	if (HANDLE eqghandle = GetModuleHandle("EQGraphicsDX9.dll")) {
		
		if (DWORD DeviceReady = _FindPattern((DWORD)eqghandle, 0x150000, drPattern, drMask)) {
			g_bDeviceReady = _GetDWordAt(DeviceReady, 2);
		}
		if (switchbug = _FindPattern((DWORD)eqghandle, 0x150000, lcPattern, lcMask))
		{
			p2DPrimitiveManager = _GetDWordAt(switchbug, 6);
			if (g_ppDrawHandler) {
				if (DWORD addr = *(DWORD*)g_ppDrawHandler) {
					if (DWORD addr2 = *(DWORD*)addr) {
						if (__UpdateDisplay = *(DWORD*)(addr2 + 0xb4)) {
							pRender = _GetDWordAt(__UpdateDisplay, 1);
							//EzDetourwName(__UpdateDisplay, &CDisplay_Hook::UpdateDisplay_Detour , &CDisplay_Hook::UpdateDisplay_Tramp, "UpdateDisplay_Detour");
						}
						if (__Reset = *(DWORD*)(addr2 + 0x40)) {
							EzDetourwName(__Reset, &CDisplay_Hook::Reset_Detour , &CDisplay_Hook::Reset_Tramp, "Reset_Detour");
						}
					}
				}
			}
			//EzDetourwName(switchbug, CachedTextBug_Detour, CachedTextBug_Tramp,"CachedTextBug_Detour");
		}
	}*/
    DebugSpewAlways("Initializing MQ2EQBugFix");
    EzDetourwName(CDisplay__is3dON, &CDisplay_Hook::is_3dON_Detour, &CDisplay_Hook::is_3dON_Trampoline,"CDisplay__is3dON");
    #ifdef EQMULETESTINGSTUFF
	//EzDetourwName(startworlddisplayexceptionhandler, startworddisplayexceptionhandler_Detour, startworddisplayexceptionhandler_Trampoline,"startworlddisplayexceptionhandler");
	#endif
}

PLUGIN_API VOID ShutdownPlugin(VOID)
{
    DebugSpewAlways("Shutting down MQ2EQBugFix");
	RemoveDetour(CDisplay__is3dON);
	if (switchbug) {
		//RemoveDetour(switchbug);
	}
	if (__UpdateDisplay) {
		//RemoveDetour(__UpdateDisplay);
	}
	if (__Reset) {
		RemoveDetour(__Reset);
	}
	#ifdef EQMULETESTINGSTUFF
    //RemoveDetour(startworlddisplayexceptionhandler);
	#endif
}