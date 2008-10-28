/* Kitserver module */
#define UNICODE
#define THISMOD &k_kserv

#include <windows.h>
#include <stdio.h>
#include "kload_exp.h"
#include "kserv.h"
#include "kserv_addr.h"
#include "dllinit.h"
#include "gdb.h"
#include "pngdib.h"
#include "utf8.h"
#include "commctrl.h"
#include "afsio.h"

#if _CPPLIB_VER < 503
#define  __in
#define  __out 
#define  __in
#define  __out_opt
#define  __inout_opt
#endif

#define lang(s) getTransl("kserv",s)

#include <map>
#include <hash_map>
#include <wchar.h>

#define NUM_TEAMS 258
#define NUM_TEAMS_TOTAL 314

// VARIABLES
HINSTANCE hInst = NULL;
KMOD k_kserv = {MODID, NAMELONG, NAMESHORT, DEFAULT_DEBUG};

INIT_NEW_KIT origInitNewKit = NULL;

#define NUM_INITED_KITS 10
KIT_OBJECT* initedKits[NUM_INITED_KITS];
int nextInitedKitsIdx = 0;

// FUNCTIONS
HRESULT STDMETHODCALLTYPE initModule(IDirect3D9* self, UINT Adapter,
    D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags,
    D3DPRESENT_PARAMETERS *pPresentationParameters, 
    IDirect3DDevice9** ppReturnedDeviceInterface);
	
DWORD STDMETHODCALLTYPE kservInitNewKit(DWORD p1);
DWORD kservAfterCreateTexture(DWORD p1);

WORD GetTeamIdByIndex(int index);
char* GetTeamNameByIndex(int index);
char* GetTeamNameById(WORD id);
void kservAfterReadNamesCallPoint();
KEXPORT void kservAfterReadNames();


/*******************/
/* DLL Entry Point */
/*******************/
EXTERN_C BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		hInst = hInstance;

		RegisterKModule(THISMOD);
		
		if (!checkGameVersion()) {
			LOG(L"Sorry, your game version isn't supported!");
			return false;
		}
		
		copyAdresses();
		hookFunction(hk_D3D_CreateDevice, initModule);
	}
	
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		LOG(L"Shutting down this module...");
	}
	
	return true;
}

WORD GetTeamIdByIndex(int index)
{
    if (index < 0 || index >= NUM_TEAMS_TOTAL)
        return 0xffff; // invalid index
    TEAM_NAME* teamName = **(TEAM_NAME***)data[TEAM_NAMES];
    return teamName[index].teamId;
}

char* GetTeamNameByIndex(int index)
{
    if (index < 0 || index >= NUM_TEAMS_TOTAL)
        return NULL; // invalid index
    TEAM_NAME* teamName = **(TEAM_NAME***)data[TEAM_NAMES];
    return (char*)&(teamName[index].name);
}

char* GetTeamNameById(WORD id)
{
    TEAM_NAME* teamName = **(TEAM_NAME***)data[TEAM_NAMES];
    for (int i=0; i<NUM_TEAMS_TOTAL; i++)
    {
        if (teamName[i].teamId == id)
            return (char*)&(teamName[i].name);
    }
    return NULL;
}

HRESULT STDMETHODCALLTYPE initModule(IDirect3D9* self, UINT Adapter,
    D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags,
    D3DPRESENT_PARAMETERS *pPresentationParameters, 
    IDirect3DDevice9** ppReturnedDeviceInterface)
{
	unhookFunction(hk_D3D_CreateDevice, initModule);

    LOG(L"Initializing Kserv Module");
	
	// hooks
	origInitNewKit = (INIT_NEW_KIT)hookVtableFunction(code[C_KIT_VTABLE], 1, kservInitNewKit);
	MasterHookFunction(code[C_AFTER_CREATE_TEXTURE], 1, kservAfterCreateTexture);
    HookCallPoint(code[C_AFTER_READ_NAMES], 
            kservAfterReadNamesCallPoint, 6, 5);
	
	TRACE(L"Initialization complete.");
	return D3D_OK;
}

KEXPORT DWORD hookVtableFunction(DWORD vtableAddr, DWORD offset, void* func)
{
	DWORD* vtableEntry = (DWORD*)(vtableAddr + offset*4);
	DWORD orig = *vtableEntry;
	
	DWORD protection = 0;
    DWORD newProtection = PAGE_READWRITE;
    if (VirtualProtect(vtableEntry, 4, newProtection, &protection)) 
		*vtableEntry = (DWORD)func;
	return orig;
}



DWORD STDMETHODCALLTYPE kservInitNewKit(DWORD p1)
{
	KIT_OBJECT* kitObject;
	DWORD result;
	__asm mov kitObject, ecx
	
	TRACE1N(L"initKit: %08x", (DWORD)kitObject);
	initedKits[nextInitedKitsIdx] = kitObject;
	nextInitedKitsIdx = (nextInitedKitsIdx + 1) % NUM_INITED_KITS;
	
	__asm {
		push p1
		mov ecx, kitObject
		call origInitNewKit
		mov result, eax
	}
	return result;
}

VOID WINAPI ColorFill (D3DXVECTOR4* pOut, const D3DXVECTOR2* pTexCoord, 
const D3DXVECTOR2* pTexelSize, LPVOID pData)
{
	// red
    *pOut = D3DXVECTOR4(1.0f, 0.0f, 0.0f, 1.0f);
}

DWORD kservAfterCreateTexture(DWORD p1)
{
	PES_TEXTURE* tex;
	PES_TEXTURE_PACKAGE* texPackage;
	__asm mov tex, ebx
	__asm mov eax, [ebp]
	__asm mov eax, [eax]
	__asm mov eax, [eax+8]
	__asm mov texPackage, eax
	
	for (int i = 0; i < NUM_INITED_KITS; i++) {
		KIT_OBJECT* kitObject = initedKits[i];
		if (!kitObject || kitObject->kitTex != texPackage)
			continue;
			
		// this is a kit
		TRACE1N(L"kit at %08x was loaded", (DWORD)kitObject);
		// fill the texture for testing
		D3DXFillTexture(tex->tex, ColorFill, NULL);
	}
	
	return MasterCallNext(p1);
}

void kservAfterReadNamesCallPoint()
{
    __asm {
        pushfd 
        push ebp
        push eax
        push ebx
        push ecx
        push edx
        push esi
        push edi
        mov [eax+0x2884], ecx  // execute replaced code
        call kservAfterReadNames
        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        pop eax
        pop ebp
        popfd
        retn
    }
}

KEXPORT void kservAfterReadNames()
{
    // Dump slot information
    
    TEAM_KIT_INFO* teamInfo = (TEAM_KIT_INFO*)(*(DWORD*)data[PLAYERS_DATA] + data[TEAM_KIT_INFO_OFFSET]);
    LOG1N(L"teamInfo = %08x", (DWORD)teamInfo);

    // team names are stored in Utf-8, so we
    // write the bytes as is. (Names are original: before edit data is loaded)
    wstring filename(getPesInfo()->myDir);
    filename += L"\\slots.txt";
    FILE* f = _wfopen(filename.c_str(),L"wt");
    for (int i=0; i<NUM_TEAMS; i++)
    {
        WORD teamId = GetTeamIdByIndex(i);
        if (teamId == 0xffff)
            continue;
        fprintf(f, "slot: %6d\tteam: %3d (%04x) %s\n", 
                (short)teamInfo[i].ga.slot, i, teamId, GetTeamNameByIndex(i));
    }
    fclose(f);
}

