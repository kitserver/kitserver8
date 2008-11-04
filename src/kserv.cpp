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

#define CREATE_FLAGS 0

#define NUM_TEAMS 258
#define NUM_TEAMS_TOTAL 314

#define NUM_SLOTS 256
#define BIN_FONT_FIRST   2271 
#define BIN_FONT_LAST    3294 
#define BIN_NUMBER_FIRST 3295
#define BIN_NUMBER_LAST  4318
#define BIN_KIT_FIRST    7832
#define BIN_KIT_LAST     8343
#define XBIN_FONT_FIRST   8347 
#define XBIN_FONT_LAST    9370 
#define XBIN_NUMBER_FIRST 9371
#define XBIN_NUMBER_LAST  10394
#define XBIN_KIT_FIRST    10870
#define XBIN_KIT_LAST     11381

enum
{
    BIN_KIT_GK,
    BIN_KIT_PL,
    BIN_FONT_GA,
    BIN_FONT_GB,
    BIN_FONT_PA,
    BIN_FONT_PB,
    BIN_NUMS_GA,
    BIN_NUMS_GB,
    BIN_NUMS_PA,
    BIN_NUMS_PB,
};

class kserv_config_t 
{
public:
    kserv_config_t() : _use_description(true) {}
    bool _use_description;
};

// VARIABLES
HINSTANCE hInst = NULL;
KMOD k_kserv = {MODID, NAMELONG, NAMESHORT, DEFAULT_DEBUG};

INIT_NEW_KIT origInitNewKit = NULL;

#define NUM_INITED_KITS 10
KIT_OBJECT* initedKits[NUM_INITED_KITS];
int nextInitedKitsIdx = 0;

GDB* _gdb;
kserv_config_t _kserv_config;
hash_map<int,TEAM_KIT_INFO> _orgTeamKitInfo;

// FUNCTIONS
HRESULT STDMETHODCALLTYPE initModule(IDirect3D9* self, UINT Adapter,
    D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags,
    D3DPRESENT_PARAMETERS *pPresentationParameters, 
    IDirect3DDevice9** ppReturnedDeviceInterface);
void kservConfig(char* pName, const void* pValue, DWORD a);
	
DWORD STDMETHODCALLTYPE kservInitNewKit(DWORD p1);
DWORD kservAfterCreateTexture(DWORD p1);

WORD GetTeamIdByIndex(int index);
char* GetTeamNameByIndex(int index);
char* GetTeamNameById(WORD id);
void kservAfterReadNamesCallPoint();
KEXPORT void kservAfterReadNames();
void DumpSlotsInfo();
void kservReadEditData(LPCVOID data, DWORD size);
void kservWriteEditData(LPCVOID data, DWORD size);
void RelinkTeam(int teamIndex, TEAM_KIT_INFO* teamKitInfo=NULL);
void UndoRelinks();
bool kservGetFileInfo(DWORD afsId, DWORD binId, HANDLE& hfile, DWORD& fsize);
bool OpenFileIfExists(const wchar_t* filename, HANDLE& handle, DWORD& size);


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
        if (_gdb) delete _gdb;
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
	//origInitNewKit = (INIT_NEW_KIT)hookVtableFunction(code[C_KIT_VTABLE], 1, kservInitNewKit);
	//MasterHookFunction(code[C_AFTER_CREATE_TEXTURE], 1, kservAfterCreateTexture);
    HookCallPoint(code[C_AFTER_READ_NAMES], 
            kservAfterReadNamesCallPoint, 6, 5);

    // Load GDB
    LOG1S(L"pesDir: {%s}",getPesInfo()->pesDir);
    LOG1S(L"myDir : {%s}",getPesInfo()->myDir);
    LOG1S(L"gdbDir: {%sGDB}",getPesInfo()->gdbDir);
    _gdb = new GDB(getPesInfo()->gdbDir, false);
    LOG1N(L"Teams in GDB map: %d", _gdb->uni.size());

    getConfig("kserv", "debug", DT_DWORD, 1, kservConfig);
    getConfig("kserv", "use.description", DT_DWORD, 2, kservConfig);
    LOG1N(L"debug = %d", k_kserv.debug);
    LOG1N(L"use.description = %d", _kserv_config._use_description);

    // add callbacks
    addReadEditDataCallback(kservReadEditData);
    addWriteEditDataCallback(kservWriteEditData);
    afsioAddCallback(kservGetFileInfo);

	TRACE(L"Initialization complete.");
	return D3D_OK;
}

void kservConfig(char* pName, const void* pValue, DWORD a)
{
	switch (a) {
		case 1: // debug
			k_kserv.debug = *(DWORD*)pValue;
			break;
        case 2: // use-description
            _kserv_config._use_description = *(DWORD*)pValue == 1;
            break;
	}
	return;
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
    // dump slot information
    DumpSlotsInfo();

    // re-link Russia
    RelinkTeam(21);
}

void DumpSlotsInfo()
{
    // team names are stored in Utf-8, so we write the bytes as is.
    TEAM_KIT_INFO* teamKitInfo = (TEAM_KIT_INFO*)(*(DWORD*)data[PLAYERS_DATA] 
            + data[TEAM_KIT_INFO_OFFSET]);
    LOG1N(L"teamKitInfo = %08x", (DWORD)teamKitInfo);

    wstring filename(getPesInfo()->myDir);
    filename += L"\\slots.txt";
    FILE* f = _wfopen(filename.c_str(),L"wt");
    for (int i=0; i<NUM_TEAMS; i++)
    {
        WORD teamId = GetTeamIdByIndex(i);
        if (teamId == 0xffff)
            continue;
        fprintf(f, "slot: %6d\tteam: %3d (%04x) %s\n", 
                (short)teamKitInfo[i].ga.slot, 
                i, teamId, GetTeamNameByIndex(i));
    }
    fclose(f);
}

void RelinkTeam(int teamIndex, TEAM_KIT_INFO* teamKitInfo)
{
    if (!teamKitInfo)
        teamKitInfo = (TEAM_KIT_INFO*)(*(DWORD*)data[PLAYERS_DATA] 
                + data[TEAM_KIT_INFO_OFFSET]);

    TEAM_KIT_INFO tki;
    tki.ga.slot = teamKitInfo[teamIndex].ga.slot;
    tki.pa.slot = teamKitInfo[teamIndex].pa.slot;
    tki.gb.slot = teamKitInfo[teamIndex].gb.slot;
    tki.pb.slot = teamKitInfo[teamIndex].pb.slot;

    pair<hash_map<int,TEAM_KIT_INFO>::iterator,bool> ires =
        _orgTeamKitInfo.insert(pair<int,TEAM_KIT_INFO>(teamIndex,tki));
    if (!ires.second)
    {
        // already saved in the map.
        TRACE1N(L"teamKitInfo for %d already saved.", teamIndex);
    }

    WORD slot = 0x5ef;
    teamKitInfo[teamIndex].ga.slot = slot;
    teamKitInfo[teamIndex].pa.slot = slot;
    teamKitInfo[teamIndex].gb.slot = slot;
    teamKitInfo[teamIndex].pb.slot = slot;

    LOG2N(L"team %d relinked to slot 0x%04x", teamIndex, slot); 

    // extend cv06.img
    afsioExtendSlots(6, XBIN_KIT_LAST+1);

    // rewrite memory location that holds the number of kit-slots
    DWORD* pNumSlots = (DWORD*)data[NUM_SLOTS_PTR];
    if (pNumSlots) 
    {
        DWORD protection = 0;
        DWORD newProtection = PAGE_READWRITE;
        if (VirtualProtect(pNumSlots, 4, newProtection, &protection)) 
            *pNumSlots = 0x0700;  
    }
}

void UndoRelinks(TEAM_KIT_INFO* teamKitInfo)
{
    for (hash_map<int,TEAM_KIT_INFO>::iterator it = _orgTeamKitInfo.begin();
            it != _orgTeamKitInfo.end();
            it++)
    {
        teamKitInfo[it->first].ga.slot = it->second.ga.slot;
        teamKitInfo[it->first].pa.slot = it->second.pa.slot;
        teamKitInfo[it->first].gb.slot = it->second.gb.slot;
        teamKitInfo[it->first].pb.slot = it->second.pb.slot;

        LOG1N(L"Team %d slot links restored", it->first);
    }
}

/**
 * edit data read callback
 */
void kservReadEditData(LPCVOID buf, DWORD size)
{
    // dump slot information again
    DumpSlotsInfo();

    // re-link Russia
    TEAM_KIT_INFO* teamKitInfo = (TEAM_KIT_INFO*)((BYTE*)buf 
            + 0x120 + data[TEAM_KIT_INFO_OFFSET] - 8);
    RelinkTeam(21, teamKitInfo);
}

/**
 * edit data write callback
 */
void kservWriteEditData(LPCVOID buf, DWORD size)
{
    // undo re-linking: we don't want dynamic relinking
    // to be saved in PES2009_EDIT01.bin
    TEAM_KIT_INFO* teamKitInfo = (TEAM_KIT_INFO*)((BYTE*)buf 
            + 0x120 + data[TEAM_KIT_INFO_OFFSET] - 8);
    UndoRelinks(teamKitInfo);
}

/**
 * AFSIO callback
 */
bool kservGetFileInfo(DWORD afsId, DWORD binId, HANDLE& hfile, DWORD& fsize)
{
    wchar_t* files[] = { L"font.bin", L"numbers.bin", L"kits.bin" };

    if (afsId == 6)
    {
        wchar_t* file = L"";
        if (XBIN_KIT_FIRST <= binId && binId <= XBIN_KIT_LAST) 
            file = files[2];
        else if (XBIN_NUMBER_FIRST <= binId && binId <= XBIN_NUMBER_LAST) 
            file = files[1];
        else if (XBIN_FONT_FIRST <= binId && binId <= XBIN_FONT_LAST) 
            file = files[0];
        else 
            return false;

        wchar_t filename[1024] = {0};
        swprintf(filename,L"%sGDB\\uni\\Russia\\%s", 
                getPesInfo()->gdbDir, file);
        LOG1S(L"using BIN file: %s", filename);

        return OpenFileIfExists(filename, hfile, fsize);
    }
    return false;
}

/**
 * Simple file-check routine.
 */
bool OpenFileIfExists(const wchar_t* filename, HANDLE& handle, DWORD& size)
{
    TRACE1S(L"OpenFileIfExists:: %s", filename);
    handle = CreateFile(filename,           // file to open
                       GENERIC_READ,          // open for reading
                       FILE_SHARE_READ,       // share for reading
                       NULL,                  // default security
                       OPEN_EXISTING,         // existing file only
                       FILE_ATTRIBUTE_NORMAL | CREATE_FLAGS, // normal file
                       NULL);                 // no attr. template

    if (handle != INVALID_HANDLE_VALUE)
    {
        size = GetFileSize(handle,NULL);
        return true;
    }
    return false;
}

