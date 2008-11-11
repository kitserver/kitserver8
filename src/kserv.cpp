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
#include "afsreader.h"
#include "teaminfo.h"
#include "soft\zlib123-dll\include\zlib.h"

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
#define SWAPBYTES(dw) \
    ((dw<<24 & 0xff000000) | (dw<<8  & 0x00ff0000) | \
    (dw>>8  & 0x0000ff00) | (dw>>24 & 0x000000ff))

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

#define XSLOT_FIRST 0x5ef
#define XSLOT_LAST  0x6ee

#define NUM_SLOTS_CV0F 128
#define CV0F_BIN_FONT_FIRST   134
#define CV0F_BIN_FONT_LAST    645
#define CV0F_BIN_NUMBER_FIRST 646
#define CV0F_BIN_NUMBER_LAST  1157
#define CV0F_BIN_KIT_FIRST    1172
#define CV0F_BIN_KIT_LAST     1427

HINSTANCE hInst = NULL;
KMOD k_kserv = {MODID, NAMELONG, NAMESHORT, DEFAULT_DEBUG};

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

/**
 * Utility class to keep track of buffers
 * and memory allocations/deallocations
 */
class kserv_buffer_manager_t
{
public:
    kserv_buffer_manager_t() : _unpacked(NULL), _packed(NULL) {}
    ~kserv_buffer_manager_t() 
    {
        if (_unpacked) { HeapFree(GetProcessHeap(),0,_unpacked); }
        if (_packed) { HeapFree(GetProcessHeap(),0,_packed); }
        for (list<BYTE*>::iterator bit = _buffers.begin();
                bit != _buffers.end();
                bit++)
            HeapFree(GetProcessHeap(),0,*bit);
        TRACE(L"buffers deallocated.");
    }
    UNPACKED_BIN* new_unpacked(size_t size)
    {
        _unpacked = (UNPACKED_BIN*)HeapAlloc(GetProcessHeap(),
                HEAP_ZERO_MEMORY, size);
        return _unpacked;
    }
    PACKED_BIN* new_packed(size_t size)
    {
        _packed = (PACKED_BIN*)HeapAlloc(GetProcessHeap(),
                HEAP_ZERO_MEMORY, size);
        return _packed;
    }
    BYTE* new_buffer(size_t size)
    {
        BYTE* _buffer = (BYTE*)HeapAlloc(GetProcessHeap(),
                HEAP_ZERO_MEMORY, size);
        if (_buffer) _buffers.push_back(_buffer);
        return _buffer;
    }

    UNPACKED_BIN* _unpacked;
    PACKED_BIN* _packed;
    list<BYTE*> _buffers;
};

typedef struct _ORG_TEAM_KIT_INFO
{
    TEAM_KIT_INFO tki;
    bool ga;
    bool pa;
    bool gb;
    bool pb;
} ORG_TEAM_KIT_INFO;

// VARIABLES
INIT_NEW_KIT origInitNewKit = NULL;

#define NUM_INITED_KITS 10
KIT_OBJECT* initedKits[NUM_INITED_KITS];
int nextInitedKitsIdx = 0;

GDB* _gdb;
kserv_config_t _kserv_config;
hash_map<int,ORG_TEAM_KIT_INFO> _orgTeamKitInfo;
hash_map<WORD,WORD> _slotMap;
hash_map<WORD,WORD> _reverseSlotMap;
typedef hash_map<WORD,KitCollection>::iterator kc_iter_t;
CRITICAL_SECTION _cs;

// FUNCTIONS
HRESULT STDMETHODCALLTYPE initModule(IDirect3D9* self, UINT Adapter,
    D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags,
    D3DPRESENT_PARAMETERS *pPresentationParameters, 
    IDirect3DDevice9** ppReturnedDeviceInterface);
void kservConfig(char* pName, const void* pValue, DWORD a);
	
DWORD STDMETHODCALLTYPE kservInitNewKit(DWORD p1);
DWORD kservAfterCreateTexture(DWORD p1);

WORD GetTeamIdByIndex(int index);
WORD GetTeamIndexById(WORD id);
WORD GetTeamIndexBySlot(WORD slot);
KitCollection* FindTeamInGDB(WORD teamIndex);
char* GetTeamNameByIndex(int index);
char* GetTeamNameById(WORD id);
void kservAfterReadNamesCallPoint();
KEXPORT void kservAfterReadNames();
void DumpSlotsInfo();
void kservReadEditData(LPCVOID data, DWORD size);
void kservWriteEditData(LPCVOID data, DWORD size);
void kservReadReplayData(LPCVOID data, DWORD size);
void kservWriteReplayData(LPCVOID data, DWORD size);
void InitSlotMapInThread(TEAM_KIT_INFO* teamKitInfo=NULL);
DWORD WINAPI InitSlotMap(LPCVOID param=NULL);
void RelinkKit(WORD teamIndex, WORD slot, KIT_INFO& kitInfo);
void UndoRelinks();
bool kservGetFileInfo(DWORD afsId, DWORD binId, HANDLE& hfile, DWORD& fsize);
bool CreatePipeForKitBin(DWORD afsId, DWORD binId, HANDLE& handle, DWORD& size);
bool CreatePipeForFontBin(DWORD afsId, DWORD binId, HANDLE& handle, DWORD& size);
bool CreatePipeForNumbersBin(DWORD afsId, DWORD binId, HANDLE& handle, DWORD& size);
int GetBinType(DWORD afsId, DWORD id);
void ApplyKitAttributes(map<wstring,Kit>& m, const wchar_t* kitKey, KIT_INFO& ki);
KEXPORT void ApplyKitAttributes(const map<wstring,Kit>::iterator kiter, KIT_INFO& ki);
void RGBAColor2KCOLOR(const RGBAColor& color, KCOLOR& kcolor);
void KCOLOR2RGBAColor(const KCOLOR kcolor, RGBAColor& color);

DWORD LoadPNGTexture(BITMAPINFO** tex, const wchar_t* filename);
void ApplyAlphaChunk(RGBQUAD* palette, BYTE* memblk, DWORD size);
static int read_file_to_mem(const wchar_t *fn,unsigned char **ppfiledata, int *pfilesize);
void ApplyDIBTexture(TEXTURE_ENTRY* tex, BITMAPINFO* bitmap);
void FreePNGTexture(BITMAPINFO* bitmap);
void ReplaceTexturesInBin(UNPACKED_BIN* bin, wstring files[], size_t n);
void DumpData(void* data, size_t size);

void kservReadNumSlotsCallPoint1();
void kservReadNumSlotsCallPoint2();
void kservReadNumSlotsCallPoint3();
void kservReadNumSlotsCallPoint4();
KEXPORT DWORD kservReadNumSlots(DWORD slot);

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
        InitializeCriticalSection(&_cs);
	}
	
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		LOG(L"Shutting down this module...");
        if (_gdb) delete _gdb;
        DeleteCriticalSection(&_cs);
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

WORD GetTeamIndexById(WORD id)
{
    TEAM_NAME* teamName = **(TEAM_NAME***)data[TEAM_NAMES];
    for (int i=0; i<NUM_TEAMS_TOTAL; i++)
    {
        if (teamName[i].teamId == id)
            return i;
    }
    return 0xffff;
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

    HookCallPoint(code[C_READ_NUM_SLOTS1], kservReadNumSlotsCallPoint1, 6, 1);
    HookCallPoint(code[C_READ_NUM_SLOTS2], kservReadNumSlotsCallPoint2, 6, 1);
    HookCallPoint(code[C_READ_NUM_SLOTS3], kservReadNumSlotsCallPoint3, 6, 1);
    HookCallPoint(code[C_READ_NUM_SLOTS4], kservReadNumSlotsCallPoint4, 6, 0);

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
    addReadReplayDataCallback(kservReadReplayData);
    addWriteReplayDataCallback(kservWriteReplayData);
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
        mov dword ptr ds:[eax+0x2884], 0x144  // execute replaced code
        pushfd 
        push ebp
        push eax
        push ebx
        push ecx
        push edx
        push esi
        push edi
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
    //DumpSlotsInfo();

    // initialize kit slots
    InitSlotMapInThread();
}

void DumpSlotsInfo()
{
    // team names are stored in Utf-8, so we write the bytes as is.
    TEAM_KIT_INFO* teamKitInfo = (TEAM_KIT_INFO*)(*(DWORD*)data[PLAYERS_DATA] 
            + data[TEAM_KIT_INFO_OFFSET]);
    LOG1N(L"teamKitInfo = %08x", (DWORD)teamKitInfo);

    wstring filename(getPesInfo()->myDir);
    filename += L"\\uni.txt";
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

void InitSlotMapInThread(TEAM_KIT_INFO* teamKitInfo)
{
    if (!teamKitInfo)
        teamKitInfo = (TEAM_KIT_INFO*)(*(DWORD*)data[PLAYERS_DATA] 
                + data[TEAM_KIT_INFO_OFFSET]);

    DWORD threadId;
    HANDLE initThread = CreateThread( 
        NULL,                   // default security attributes
        0,                      // use default stack size  
        (LPTHREAD_START_ROUTINE)InitSlotMap, // thread function name
        teamKitInfo,            // argument to thread function 
        0,                      // use default creation flags 
        &threadId);             // returns the thread identifier 
}

DWORD WINAPI InitSlotMap(LPCVOID param)
{
    EnterCriticalSection(&_cs);
    TEAM_KIT_INFO* teamKitInfo = (TEAM_KIT_INFO*)param;
    if (!teamKitInfo)
        teamKitInfo = (TEAM_KIT_INFO*)(*(DWORD*)data[PLAYERS_DATA] 
                + data[TEAM_KIT_INFO_OFFSET]);

    _slotMap.clear();
    _reverseSlotMap.clear();
    _orgTeamKitInfo.clear();

    // linked (or re-linked teams)
    for (WORD i=0; i<NUM_TEAMS; i++)
    {
        short slot = (short)teamKitInfo[i].pa.slot;
        if (slot >= 0)
        {
            _slotMap.insert(pair<WORD,WORD>((WORD)slot,i));
            _reverseSlotMap.insert(pair<WORD,WORD>(i,(WORD)slot));
        }
    }
    LOG1N(L"Normal slots taken: %d", _slotMap.size());

    // GDB teams
    WORD nextSlot = 0x5ef;
    for (hash_map<WORD,KitCollection>::iterator git = _gdb->uni.begin();
            git != _gdb->uni.end();
            git++)
    {
        hash_map<WORD,WORD>::iterator rit = _reverseSlotMap.find(git->first);
        bool toRelink = (rit == _reverseSlotMap.end());

        // store original attributes
        WORD i = git->first;
        if (i >= NUM_TEAMS)
            continue; // safety check, until we implement support for
                      // add additional teams

        ORG_TEAM_KIT_INFO o;
        ZeroMemory(&o, sizeof(ORG_TEAM_KIT_INFO));
        memcpy(&o.tki, &teamKitInfo[i], sizeof(TEAM_KIT_INFO));
        if (git->second.goalkeepers.find(L"ga")!=git->second.goalkeepers.end())
        {
            o.ga = true;
            if (toRelink) RelinkKit(i, nextSlot, teamKitInfo[i].ga);
        }
        if (git->second.goalkeepers.find(L"gb")!=git->second.goalkeepers.end())
        {
            o.gb = true;
            if (toRelink) RelinkKit(i, nextSlot, teamKitInfo[i].gb);
        }
        if (git->second.players.find(L"pa")!=git->second.players.end())
        {
            o.pa = true;
            if (toRelink) RelinkKit(i, nextSlot, teamKitInfo[i].pa);
        }
        if (git->second.players.find(L"pb")!=git->second.players.end())
        {
            o.pb = true;
            if (toRelink) RelinkKit(i, nextSlot, teamKitInfo[i].pb);
        }
        _orgTeamKitInfo.insert(pair<int,ORG_TEAM_KIT_INFO>(i,o));

        // apply attributes
        ApplyKitAttributes(git->second.goalkeepers, 
                L"ga",teamKitInfo[git->first].ga);
        ApplyKitAttributes(git->second.players, 
                L"pa",teamKitInfo[git->first].pa);
        ApplyKitAttributes(git->second.goalkeepers, 
                L"gb",teamKitInfo[git->first].gb);
        ApplyKitAttributes(git->second.players, 
                L"pb",teamKitInfo[git->first].pb);

        // move to next slot
        if ((o.ga||o.gb||o.pa||o.pb) && toRelink)
        {
            LOG2N(L"team %d relinked to slot 0x%x", i, nextSlot); 
            nextSlot++;
        }
    }
    LOG1N(L"Total slots taken: %d", _slotMap.size());

    // extend cv06.img
    afsioExtendSlots(6, XBIN_KIT_LAST+1);

    LeaveCriticalSection(&_cs);
    return 0;
}

void RelinkKit(WORD teamIndex, WORD slot, KIT_INFO& kitInfo)
{
    kitInfo.slot = slot;
    _slotMap.insert(pair<WORD,WORD>(slot,teamIndex));
    _reverseSlotMap.insert(pair<WORD,WORD>(teamIndex,slot));
}

void ApplyKitAttributes(map<wstring,Kit>& m, const wchar_t* kitKey, KIT_INFO& ki)
{
    map<wstring,Kit>::iterator kiter = m.find(kitKey);
    if (kiter != m.end())
        ApplyKitAttributes(kiter, ki);
}

KEXPORT void ApplyKitAttributes(const map<wstring,Kit>::iterator kiter, KIT_INFO& ki)
{
    // load kit attributes from config.txt, if needed
    _gdb->loadConfig(kiter->second);

    // apply attributes
    if (kiter->second.attDefined & MODEL)
        ki.model = kiter->second.model;
    if (kiter->second.attDefined & COLLAR)
        ki.collar = kiter->second.collar;
    if (kiter->second.attDefined & SHIRT_NUMBER_LOCATION)
        ki.frontNumberPosition = kiter->second.shirtNumberLocation;
    if (kiter->second.attDefined & SHORTS_NUMBER_LOCATION)
        ki.shortsNumberPosition = kiter->second.shortsNumberLocation;
    if (kiter->second.attDefined & NAME_LOCATION)
        ki.fontPosition = kiter->second.nameLocation;
    if (kiter->second.attDefined & NAME_SHAPE)
        ki.fontShape = kiter->second.nameShape;
    if (kiter->second.attDefined & MAIN_COLOR) 
    {
        RGBAColor2KCOLOR(kiter->second.mainColor, ki.mainColor);
        // kit selection uses all 5 shirt colors - not only main (first one)
        for (int i=0; i<4; i++)
            RGBAColor2KCOLOR(kiter->second.mainColor, ki.editShirtColors[i]);
    }
    // shorts main color
    if (kiter->second.attDefined & SHORTS_MAIN_COLOR)
        RGBAColor2KCOLOR(kiter->second.shortsFirstColor, ki.shortsFirstColor);
}

void RGBAColor2KCOLOR(const RGBAColor& color, KCOLOR& kcolor)
{
    kcolor = 0x8000
        +((color.r>>3) & 31)
        +0x20*((color.g>>3) & 31)
        +0x400*((color.b>>3) & 31);
}

void KCOLOR2RGBAColor(const KCOLOR kcolor, RGBAColor& color)
{
    color.r = (kcolor & 31)<<3;
    color.g = (kcolor>>5 & 31)<<3;
    color.b = (kcolor>>10 & 31)<<3;
    color.a = 0xff;
}

void UndoRelinks(TEAM_KIT_INFO* teamKitInfo)
{
    for (hash_map<int,ORG_TEAM_KIT_INFO>::iterator it = _orgTeamKitInfo.begin();
            it != _orgTeamKitInfo.end();
            it++)
    {
        if (it->second.ga) 
            memcpy(&teamKitInfo[it->first].ga, 
                    &it->second.tki.ga, sizeof(KIT_INFO));
        if (it->second.pa) 
            memcpy(&teamKitInfo[it->first].pa, 
                    &it->second.tki.pa, sizeof(KIT_INFO));
        if (it->second.gb) 
            memcpy(&teamKitInfo[it->first].gb, 
                    &it->second.tki.gb, sizeof(KIT_INFO));
        if (it->second.pb) 
            memcpy(&teamKitInfo[it->first].pb, 
                    &it->second.tki.pb, sizeof(KIT_INFO));

        LOG1N(L"TeamKitInfo for %d restored", it->first);
    }
}

/**
 * edit data read callback
 */
void kservReadEditData(LPCVOID buf, DWORD size)
{
    // dump slot information again
    //DumpSlotsInfo();

    // initialize kit slots
    TEAM_KIT_INFO* teamKitInfo = (TEAM_KIT_INFO*)((BYTE*)buf 
            + 0x120 + data[TEAM_KIT_INFO_OFFSET] - 8);
    InitSlotMap(teamKitInfo);
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
 * replay data read callback
 */
void kservReadReplayData(LPCVOID buf, DWORD size)
{
    BYTE* replay = (BYTE*)buf;

    // adjust binId offsets so that we load appropriate
    // kit/font/numbers bins, for GDB teams
    WORD homeId = *(WORD*)(replay + 0x140);
    WORD homeIndex = GetTeamIndexById(homeId);
    hash_map<WORD,WORD>::iterator sit = _reverseSlotMap.find(homeIndex);
    if (sit != _reverseSlotMap.end() && (short)sit->second >= 0x5ef)
    {
        // this team is using x-slots
        if (replay[0x148]==0) // flag for edited vs. real kit
        {
            *(WORD*)(replay+0x14c) += sit->second*2;
            *(WORD*)(replay+0x178) += sit->second*4;
        }
        if (replay[0x1c8]==0) // flag for edited vs. real kit
        {
            *(WORD*)(replay+0x1cc) += sit->second*2;
            *(WORD*)(replay+0x1f8) += sit->second*4;
        }
    }

    WORD awayId = *(WORD*)(replay + 0x2ec);
    WORD awayIndex = GetTeamIndexById(awayId);
    sit = _reverseSlotMap.find(awayIndex);
    if (sit != _reverseSlotMap.end() && (short)sit->second >= 0x5ef)
    {
        if (replay[0x2f4]==0) // flag for edited vs. real kit
        {
            *(WORD*)(replay+0x2f8) += sit->second*2;
            *(WORD*)(replay+0x324) += sit->second*4;
        }
        if (replay[0x374]==0) // flag for edited vs. real kit
        {
            *(WORD*)(replay+0x378) += sit->second*2;
            *(WORD*)(replay+0x3a4) += sit->second*4;
        }
    }
}

/**
 * replay data write callback
 */
void kservWriteReplayData(LPCVOID buf, DWORD size)
{
    BYTE* replay = (BYTE*)buf;

    // make sure we don't write the binId offsets that
    // would cause the game to attempt loading a "extended" BIN.
    // With kitserver detached, this would cause the game to 
    // hang indefinitely.
    if (*(WORD*)(replay + 0x14c) > 0x1ff) *(WORD*)(replay+0x14c) %= 2;
    if (*(WORD*)(replay + 0x178) > 0x3ff) *(WORD*)(replay+0x178) %= 4;
    if (*(WORD*)(replay + 0x1cc) > 0x1ff) *(WORD*)(replay+0x1cc) %= 2;
    if (*(WORD*)(replay + 0x1f8) > 0x3ff) *(WORD*)(replay+0x1f8) %= 4;
    if (*(WORD*)(replay + 0x2f8) > 0x1ff) *(WORD*)(replay+0x2f8) %= 2;
    if (*(WORD*)(replay + 0x324) > 0x3ff) *(WORD*)(replay+0x324) %= 4;
    if (*(WORD*)(replay + 0x378) > 0x1ff) *(WORD*)(replay+0x378) %= 2;
    if (*(WORD*)(replay + 0x3a4) > 0x3ff) *(WORD*)(replay+0x3a4) %= 4;
}

/**
 * AFSIO callback
 */
bool kservGetFileInfo(DWORD afsId, DWORD binId, HANDLE& hfile, DWORD& fsize)
{
    if (afsId == 6)
    {
        // regular slots
        if (BIN_KIT_FIRST <= binId && binId <= BIN_KIT_LAST) 
            return CreatePipeForKitBin(afsId, binId, hfile, fsize);
        else if (BIN_NUMBER_FIRST <= binId && binId <= BIN_NUMBER_LAST) 
            return CreatePipeForNumbersBin(afsId, binId, hfile, fsize);
        else if (BIN_FONT_FIRST <= binId && binId <= BIN_FONT_LAST) 
            return CreatePipeForFontBin(afsId, binId, hfile, fsize);

        // x-slots
        else if (XBIN_KIT_FIRST <= binId && binId <= XBIN_KIT_LAST) 
            return CreatePipeForKitBin(afsId, binId, hfile, fsize);
        else if (XBIN_NUMBER_FIRST <= binId && binId <= XBIN_NUMBER_LAST) 
            return CreatePipeForNumbersBin(afsId, binId, hfile, fsize);
        else if (XBIN_FONT_FIRST <= binId && binId <= XBIN_FONT_LAST) 
            return CreatePipeForFontBin(afsId, binId, hfile, fsize);
    }
    else if (afsId == 15)
    {
        if (CV0F_BIN_KIT_FIRST<=binId && binId<=CV0F_BIN_KIT_LAST) 
            return CreatePipeForKitBin(afsId, binId, hfile, fsize);
        else if (CV0F_BIN_NUMBER_FIRST<=binId && binId<=CV0F_BIN_NUMBER_LAST) 
            return CreatePipeForNumbersBin(afsId, binId, hfile, fsize);
        else if (CV0F_BIN_FONT_FIRST<=binId && binId<=CV0F_BIN_FONT_LAST) 
            return CreatePipeForFontBin(afsId, binId, hfile, fsize);
    }

    return false;
}

int GetBinType(DWORD afsId, DWORD id)
{
    if (afsId == 6)
    {
        // normal slots
        if (BIN_KIT_FIRST <= id && id <= BIN_KIT_LAST)
        {
            return BIN_KIT_GK + ((id - BIN_KIT_FIRST)%2);
        }
        else if (BIN_FONT_FIRST <= id && id <= BIN_FONT_LAST)
        {
            return BIN_FONT_GA + ((id - BIN_FONT_FIRST)%4);
        }
        else if (BIN_NUMBER_FIRST <= id && id <= BIN_NUMBER_LAST)
        {
            return BIN_NUMS_GA + ((id - BIN_NUMBER_FIRST)%4);
        }

        // x-slots
        else if (XBIN_KIT_FIRST <= id && id <= XBIN_KIT_LAST)
        {
            return BIN_KIT_GK + ((id - XBIN_KIT_FIRST)%2);
        }
        else if (XBIN_FONT_FIRST <= id && id <= XBIN_FONT_LAST)
        {
            return BIN_FONT_GA + ((id - XBIN_FONT_FIRST)%4);
        }
        else if (XBIN_NUMBER_FIRST <= id && id <= XBIN_NUMBER_LAST)
        {
            return BIN_NUMS_GA + ((id - XBIN_NUMBER_FIRST)%4);
        }
    }
    else if (afsId == 15)
    {
        if (CV0F_BIN_KIT_FIRST <= id && id <= CV0F_BIN_KIT_LAST)
        {
            return BIN_KIT_GK + ((id - CV0F_BIN_KIT_FIRST)%2);
        }
        else if (CV0F_BIN_FONT_FIRST <= id && id <= CV0F_BIN_FONT_LAST)
        {
            return BIN_FONT_GA + ((id - CV0F_BIN_FONT_FIRST)%4);
        }
        else if (CV0F_BIN_NUMBER_FIRST <= id && id <= CV0F_BIN_NUMBER_LAST)
        {
            return BIN_NUMS_GA + ((id - CV0F_BIN_NUMBER_FIRST)%4);
        }
    }

    return -1;
}

void DumpData(void* data, size_t size)
{
    static int count = 0;
    char filename[256] = {0};
    sprintf(filename, "kitserver/dump%03d.bin", count);
    FILE* f = fopen(filename,"wb");
    if (f) 
    {
        fwrite(data, size, 1, f);
        fclose(f);
        LOG1N(L"Dumped file (count=%d)",count);
    }
    else
    {
        LOG2N(L"ERROR: unable to dump file (count=%d). Error code = %d",count, errno);
    }
    count++;
}

WORD GetTeamIndexBySlot(WORD slot)
{
    hash_map<WORD,WORD>::iterator sit = _slotMap.find(slot);
    if (sit != _slotMap.end())
        return sit->second;
    return 0xffff;
}

bool FindTeamInGDB(WORD teamIndex, KitCollection*& kcol)
{
    hash_map<WORD,KitCollection>::iterator it = _gdb->uni.find(teamIndex);
    if (it != _gdb->uni.end())
    {
        kcol = &it->second;
        return true;
    }
    kcol = &_gdb->dummyHome;
    return false;
}

/**
 * Create a pipe and write a dynamically created BIN into it.
 */
bool CreatePipeForKitBin(DWORD afsId, DWORD binId, HANDLE& handle, DWORD& size)
{
    // first step: determine the team, and see if we have
    // this team in the GDB
    WORD slot = (binId - BIN_KIT_FIRST) >> 1;
    if (afsId==15)
        slot = NUM_SLOTS + ((binId - CV0F_BIN_KIT_FIRST) >> 1);
    WORD teamIndex = GetTeamIndexBySlot(slot);
    KitCollection* kcol;
    if (!FindTeamInGDB(teamIndex, kcol) && 
            ((afsId==6 && binId <= BIN_KIT_LAST)||afsId==15))
        return false; // not in GDB: rely on afs kit

    // create the unpacked bin-data in memory
    kserv_buffer_manager_t bm;
    DWORD texSize = sizeof(TEXTURE_ENTRY_HEADER) + 256*sizeof(PALETTE_ENTRY)
        + 1024*512; /*data*/ 
    uLongf unpackedSize = sizeof(UNPACKED_BIN_HEADER) + sizeof(ENTRY_INFO)*2
        + 2*texSize;
    bm.new_unpacked(unpackedSize);
    if (!bm._unpacked) 
    {
        LOG1N(L"Unable to allocate buffer for binId=%d", binId);
        return false;
    }

    // initialize the bin structure
    bm._unpacked->header.numEntries = 2;
    bm._unpacked->header.unknown1 = 8;
    bm._unpacked->entryInfo[0].offset = 0x20;
    bm._unpacked->entryInfo[0].size = texSize;
    bm._unpacked->entryInfo[0].indexOffset = 0xffffffff;
    bm._unpacked->entryInfo[1].offset = 0x20 + texSize;
    bm._unpacked->entryInfo[1].size = texSize;
    bm._unpacked->entryInfo[1].indexOffset = 0xffffffff;
    for (int i=0; i<2; i++)
    {
        TEXTURE_ENTRY* te = (TEXTURE_ENTRY*)((BYTE*)bm._unpacked 
                + bm._unpacked->entryInfo[i].offset);
        memcpy(&(te->header.sig),"WE00",4);
        te->header.unknown1 = 0;
        te->header.unknown2 = 0;
        te->header.unknown3 = 3;
        te->header.bpp = 8;
        te->header.width = 1024;
        te->header.height = 512;
        te->header.paletteOffset = sizeof(TEXTURE_ENTRY_HEADER);
        te->header.dataOffset = sizeof(TEXTURE_ENTRY_HEADER) 
            + 256*sizeof(PALETTE_ENTRY);
        te->palette[0].r = 0x88; // set 1st color to grey.
        te->palette[0].g = 0x88; // as an indicator in-game that
        te->palette[0].b = 0x88; // the kit didn't load from GDB
        te->palette[0].a = 0xff;
    }

    wstring files[2];
    for (int i=0; i<2; i++)
    {
        wstring filename(getPesInfo()->gdbDir);
        filename += L"GDB\\uni\\" + kcol->foldername;
        switch (GetBinType(afsId, binId))
        {
            case BIN_KIT_GK:
                filename += (i==0)?L"\\ga":L"\\gb";
                break;
            case BIN_KIT_PL:
                filename += (i==0)?L"\\pa":L"\\pb";
                break;
        }
        files[i] = filename + L"\\kit.png";
    }
    ReplaceTexturesInBin(bm._unpacked, files, 2);
    //DumpData(bm._unpacked, unpackedSize);
    
    // pack
    uLongf packedSize = unpackedSize*2; // big buffer just in case;
    bm.new_packed(packedSize);
    int retval = compress((BYTE*)bm._packed->data,&packedSize,(BYTE*)bm._unpacked,unpackedSize);
    if (retval != Z_OK) {
        LOG1N(L"BIN re-compression failed. retval=%d", retval);
        return false;
    }
    memcpy(bm._packed->header.sig,"\x00\x01\x01WESYS",8);
    bm._packed->header.sizePacked = packedSize;
    bm._packed->header.sizeUnpacked = unpackedSize;
    size = packedSize + 0x10;

    // write the data to a pipe
    HANDLE pipeRead, pipeWrite;
    if (!CreatePipe(&pipeRead, &pipeWrite, NULL, size*2))
    {
        LOG(L"Unable to create a pipe");
        return false;
    }
    DWORD written = 0;
    if (!WriteFile(pipeWrite, bm._packed, size, &written, 0))
    {
        LOG(L"Unable to write to a pipe");
        return false;
    }
    handle = pipeRead;
    return true;
}

/**
 * Create a pipe and write a dynamically created BIN into it.
 */
bool CreatePipeForFontBin(DWORD afsId, DWORD binId, HANDLE& handle, DWORD& size)
{
    // first step: determine the team, and see if we have
    // this team in the GDB
    WORD slot = (binId - BIN_FONT_FIRST) >> 2;
    if (afsId==15)
        slot = NUM_SLOTS + ((binId - CV0F_BIN_FONT_FIRST) >> 2);
    WORD teamIndex = GetTeamIndexBySlot(slot);
    KitCollection* kcol;
    if (!FindTeamInGDB(teamIndex, kcol) && 
            ((afsId==6 && binId <= BIN_FONT_LAST)||afsId==15))
        return false; // not in GDB: rely on afs kit

    // create the unpacked bin-data in memory
    kserv_buffer_manager_t bm;
    DWORD texSize = sizeof(TEXTURE_ENTRY_HEADER) + 256*sizeof(PALETTE_ENTRY)
        + 256*64; /*data*/ 
    uLongf unpackedSize = sizeof(UNPACKED_BIN_HEADER) + sizeof(ENTRY_INFO)*2
        + texSize; // we only actually need 1 ENTRY_INFO, but let's have 2 
                   // for nice alignment (header-size then remains 8 DWORDs)
    bm.new_unpacked(unpackedSize);
    if (!bm._unpacked) 
    {
        LOG1N(L"Unable to allocate buffer for binId=%d", binId);
        return false;
    }

    // initialize the bin structure
    bm._unpacked->header.numEntries = 1;
    bm._unpacked->header.unknown1 = 8;
    bm._unpacked->entryInfo[0].offset = 0x20;
    bm._unpacked->entryInfo[0].size = texSize;
    bm._unpacked->entryInfo[0].indexOffset = 0xffffffff;
    TEXTURE_ENTRY* te = (TEXTURE_ENTRY*)((BYTE*)bm._unpacked 
            + bm._unpacked->entryInfo[0].offset);
    memcpy(&(te->header.sig),"WE00",4);
    te->header.unknown1 = 0;
    te->header.unknown2 = 0;
    te->header.unknown3 = 3;
    te->header.bpp = 8;
    te->header.width = 256;
    te->header.height = 64;
    te->header.paletteOffset = sizeof(TEXTURE_ENTRY_HEADER);
    te->header.dataOffset = sizeof(TEXTURE_ENTRY_HEADER) 
        + 256*sizeof(PALETTE_ENTRY);
    te->palette[0].r = 0xa8; // set 1st color to light grey.
    te->palette[0].g = 0xa8; // as an indicator in-game that
    te->palette[0].b = 0xa8; // the font didn't load from GDB
    te->palette[0].a = 0xff;

    wstring filename(getPesInfo()->gdbDir);
    filename += L"GDB\\uni\\"+kcol->foldername;
    switch (GetBinType(afsId, binId))
    {
        case BIN_FONT_GA:
            filename += L"\\ga\\font.png";
            break;
        case BIN_FONT_GB:
            filename += L"\\gb\\font.png";
            break;
        case BIN_FONT_PA:
            filename += L"\\pa\\font.png";
            break;
        case BIN_FONT_PB:
            filename += L"\\pb\\font.png";
            break;
    }
    wstring files[1];
    files[0] = filename;

    ReplaceTexturesInBin(bm._unpacked, files, 1);
    
    // pack
    uLongf packedSize = unpackedSize*2; // big buffer just in case;
    bm.new_packed(packedSize);
    int retval = compress((BYTE*)bm._packed->data,&packedSize,(BYTE*)bm._unpacked,unpackedSize);
    if (retval != Z_OK) {
        LOG1N(L"BIN re-compression failed. retval=%d", retval);
        return false;
    }
    memcpy(bm._packed->header.sig,"\x00\x01\x01WESYS",8);
    bm._packed->header.sizePacked = packedSize;
    bm._packed->header.sizeUnpacked = unpackedSize;
    size = packedSize + 0x10;
    //DumpData(bm._packed, size);

    // write the data to a pipe
    HANDLE pipeRead, pipeWrite;
    if (!CreatePipe(&pipeRead, &pipeWrite, NULL, size*2))
    {
        LOG(L"Unable to create a pipe");
        return false;
    }
    DWORD written = 0;
    if (!WriteFile(pipeWrite, bm._packed, size, &written, 0))
    {
        LOG(L"Unable to write to a pipe");
        return false;
    }
    handle = pipeRead;
    return true;
}

/**
 * Create a pipe and write a dynamically created BIN into it.
 */
bool CreatePipeForNumbersBin(DWORD afsId, DWORD binId, HANDLE& handle, DWORD& size)
{
    // first step: determine the team, and see if we have
    // this team in the GDB
    WORD slot = (binId - BIN_NUMBER_FIRST) >> 2;
    if (afsId==15)
        slot = NUM_SLOTS + ((binId - CV0F_BIN_NUMBER_FIRST) >> 2);
    WORD teamIndex = GetTeamIndexBySlot(slot);
    KitCollection* kcol;
    if (!FindTeamInGDB(teamIndex, kcol) && 
            ((afsId==6 && binId <= BIN_NUMBER_LAST)||afsId==15))
        return false; // not in GDB: rely on afs kit

    // create the unpacked bin-data in memory
    kserv_buffer_manager_t bm;
    DWORD texSize = sizeof(TEXTURE_ENTRY_HEADER) + 256*sizeof(PALETTE_ENTRY)
        + 512*256; /*data*/ 
    DWORD texSize2 = sizeof(TEXTURE_ENTRY_HEADER) + 256*sizeof(PALETTE_ENTRY)
        + 256*128; /*data*/ 
    uLongf unpackedSize = sizeof(UNPACKED_BIN_HEADER) + sizeof(ENTRY_INFO)*5
        + texSize + texSize2*3; // one extra ENTRY_INFO for header padding
    bm.new_unpacked(unpackedSize);
    if (!bm._unpacked) 
    {
        LOG1N(L"Unable to allocate buffer for binId=%d", binId);
        return false;
    }

    // initialize the bin structure
    bm._unpacked->header.numEntries = 4;
    bm._unpacked->header.unknown1 = 8;
    bm._unpacked->entryInfo[0].offset = 0x40;
    bm._unpacked->entryInfo[0].size = texSize;
    bm._unpacked->entryInfo[0].indexOffset = 0xffffffff;
    for (int i=1; i<4; i++)
    {
        bm._unpacked->entryInfo[i].offset = 0x40 + texSize + texSize2*(i-1);
        bm._unpacked->entryInfo[i].size = texSize2;
        bm._unpacked->entryInfo[i].indexOffset = 0xffffffff;
    }
    TEXTURE_ENTRY* te = (TEXTURE_ENTRY*)((BYTE*)bm._unpacked 
            + bm._unpacked->entryInfo[0].offset);
    memcpy(&(te->header.sig),"WE00",4);
    te->header.unknown1 = 0;
    te->header.unknown2 = 0;
    te->header.unknown3 = 3;
    te->header.bpp = 8;
    te->header.width = 512;
    te->header.height = 256;
    te->header.paletteOffset = sizeof(TEXTURE_ENTRY_HEADER);
    te->header.dataOffset = sizeof(TEXTURE_ENTRY_HEADER) 
        + 256*sizeof(PALETTE_ENTRY);
    te->palette[0].r = 0xc8; // set 1st color to light grey.
    te->palette[0].g = 0xc8; // as an indicator in-game that
    te->palette[0].b = 0xc8; // the numbers didn't load from GDB
    te->palette[0].a = 0xff;
    for (int i=1; i<4; i++)
    {
        te = (TEXTURE_ENTRY*)((BYTE*)bm._unpacked 
                + bm._unpacked->entryInfo[i].offset);
        memcpy(&(te->header.sig),"WE00",4);
        te->header.unknown1 = 0;
        te->header.unknown2 = 0;
        te->header.unknown3 = 3;
        te->header.bpp = 8;
        te->header.width = 256;
        te->header.height = 128;
        te->header.paletteOffset = sizeof(TEXTURE_ENTRY_HEADER);
        te->header.dataOffset = sizeof(TEXTURE_ENTRY_HEADER) 
            + 256*sizeof(PALETTE_ENTRY);
        te->palette[0].r = 0xc8; // set 1st color to light grey.
        te->palette[0].g = 0xc8; // as an indicator in-game that
        te->palette[0].b = 0xc8; // the numbers didn't load from GDB
        te->palette[0].a = 0xff;
    }

    wstring dirname(getPesInfo()->gdbDir);
    dirname += L"GDB\\uni\\"+kcol->foldername;
    switch (GetBinType(afsId, binId))
    {
        case BIN_NUMS_GA:
            dirname += L"\\ga\\";
            break;
        case BIN_NUMS_GB:
            dirname += L"\\gb\\";
            break;
        case BIN_NUMS_PA:
            dirname += L"\\pa\\";
            break;
        case BIN_NUMS_PB:
            dirname += L"\\pb\\";
            break;

    }
    wstring files[4];
    files[0] = dirname + L"numbers-back.png";
    files[1] = dirname + L"numbers-front.png";
    files[2] = dirname + L"numbers-shorts.png";
    files[3] = dirname + L"numbers-shorts.png";
    ReplaceTexturesInBin(bm._unpacked, files, 4);
    
    // pack
    uLongf packedSize = unpackedSize*2; // big buffer just in case;
    bm.new_packed(packedSize);
    int retval = compress((BYTE*)bm._packed->data,&packedSize,(BYTE*)bm._unpacked,unpackedSize);
    if (retval != Z_OK) {
        LOG1N(L"BIN re-compression failed. retval=%d", retval);
        return false;
    }
    memcpy(bm._packed->header.sig,"\x00\x01\x01WESYS",8);
    bm._packed->header.sizePacked = packedSize;
    bm._packed->header.sizeUnpacked = unpackedSize;
    size = packedSize + 0x10;
    //DumpData(bm._packed, size);

    // write the data to a pipe
    HANDLE pipeRead, pipeWrite;
    if (!CreatePipe(&pipeRead, &pipeWrite, NULL, size*2))
    {
        LOG(L"Unable to create a pipe");
        return false;
    }
    DWORD written = 0;
    if (!WriteFile(pipeWrite, bm._packed, size, &written, 0))
    {
        LOG(L"Unable to write to a pipe");
        return false;
    }
    handle = pipeRead;
    return true;
}

void ReplaceTexturesInBin(UNPACKED_BIN* bin, wstring files[], size_t n)
{
    for (int i=0; i<n; i++)
    {
        TEXTURE_ENTRY* tex = (TEXTURE_ENTRY*)((BYTE*)bin + bin->entryInfo[i].offset);
        BITMAPINFO* bmp = NULL;
        DWORD texSize = LoadPNGTexture(&bmp, files[i].c_str());
        if (texSize)
        {
            ApplyDIBTexture(tex, bmp);
            FreePNGTexture(bmp);
        }
    }
}

// Load texture from PNG file. Returns the size of loaded texture
DWORD LoadPNGTexture(BITMAPINFO** tex, const wchar_t* filename)
{
    if (k_kserv.debug)
        LOG1S(L"LoadPNGTexture: loading %s", (wchar_t*)filename);
    DWORD size = 0;

    PNGDIB *pngdib;
    LPBITMAPINFOHEADER* ppDIB = (LPBITMAPINFOHEADER*)tex;

    pngdib = pngdib_p2d_init();
	//TRACE(L"LoadPNGTexture: structure initialized");

    BYTE* memblk;
    int memblksize;
    if(read_file_to_mem(filename,&memblk, &memblksize)) {
        LOG1S(L"LoadPNGTexture: unable to read PNG file: %s", filename);
        return 0;
    }
    //TRACE(L"LoadPNGTexture: file read into memory");

    pngdib_p2d_set_png_memblk(pngdib,memblk,memblksize);
	pngdib_p2d_set_use_file_bg(pngdib,1);
	pngdib_p2d_run(pngdib);

	//TRACE(L"LoadPNGTexture: run done");
    pngdib_p2d_get_dib(pngdib, ppDIB, (int*)&size);
	//TRACE(L"LoadPNGTexture: get_dib done");

    pngdib_done(pngdib);
	TRACE(L"LoadPNGTexture: done done");

	TRACE1N(L"LoadPNGTexture: *ppDIB = %08x", (DWORD)*ppDIB);
    if (*ppDIB == NULL) {
        LOG(L"LoadPNGTexture: ERROR - unable to load PNG image.");
        return 0;
    }

    // read transparency values from tRNS chunk
    // and put them into DIB's RGBQUAD.rgbReserved fields
    ApplyAlphaChunk((RGBQUAD*)&((BITMAPINFO*)*ppDIB)->bmiColors, memblk, memblksize);

    HeapFree(GetProcessHeap(), 0, memblk);

	TRACE(L"LoadPNGTexture: done");
	return size;
}

/**
 * Extracts alpha values from tRNS chunk and applies stores
 * them in the RGBQUADs of the DIB
 */
void ApplyAlphaChunk(RGBQUAD* palette, BYTE* memblk, DWORD size)
{
    bool got_alpha = false;

    // find the tRNS chunk
    DWORD offset = 8;
    while (offset < size) {
        PNG_CHUNK_HEADER* chunk = (PNG_CHUNK_HEADER*)(memblk + offset);
        if (chunk->dwName == MAKEFOURCC('t','R','N','S')) {
            int numColors = SWAPBYTES(chunk->dwSize);
            BYTE* alphaValues = memblk + offset + sizeof(chunk->dwSize) + sizeof(chunk->dwName);
            for (int i=0; i<numColors; i++) {
                palette[i].rgbReserved = alphaValues[i];
            }
            got_alpha = true;
            break;
        }
        // move on to next chunk
        offset += sizeof(chunk->dwSize) + sizeof(chunk->dwName) + 
            SWAPBYTES(chunk->dwSize) + sizeof(DWORD); // last one is CRC
    }

    // initialize alpha to all-opaque, if haven't gotten it
    if (!got_alpha) {
        LOG(L"ApplyAlphaChunk::WARNING: no transparency.");
        for (int i=0; i<256; i++) {
            palette[i].rgbReserved = 0xff;
        }
    }
}

// Read a file into a memory block.
static int read_file_to_mem(const wchar_t *fn,unsigned char **ppfiledata, int *pfilesize)
{
	HANDLE hfile;
	DWORD fsize;
	//unsigned char *fbuf;
	BYTE* fbuf;
	DWORD bytesread;

	hfile=CreateFile(fn,GENERIC_READ,FILE_SHARE_READ,NULL,
		OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hfile==INVALID_HANDLE_VALUE) return 1;

	fsize=GetFileSize(hfile,NULL);
	if(fsize>0) {
		//fbuf=(unsigned char*)GlobalAlloc(GPTR,fsize);
		//fbuf=(unsigned char*)calloc(fsize,1);
        fbuf = (BYTE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, fsize);
		if(fbuf) {
			if(ReadFile(hfile,(void*)fbuf,fsize,&bytesread,NULL)) {
				if(bytesread==fsize) { 
					(*ppfiledata)  = fbuf;
					(*pfilesize) = (int)fsize;
					CloseHandle(hfile);
					return 0;   // success
				}
			}
			free((void*)fbuf);
		}
	}
	CloseHandle(hfile);
	return 1;  // error
}

// Substitute kit textures with data from DIB
// Currently supports only 4bit and 8bit paletted DIBs
void ApplyDIBTexture(TEXTURE_ENTRY* tex, BITMAPINFO* bitmap)
{
    TRACE(L"Applying DIB texture");

	BYTE* srcTex = (BYTE*)bitmap;
	BITMAPINFOHEADER* bih = &bitmap->bmiHeader;
	DWORD palOff = bih->biSize;
    DWORD numColors = bih->biClrUsed;
    if (numColors == 0)
        numColors = 1 << bih->biBitCount;

    DWORD palSize = numColors*4;
	DWORD bitsOff = palOff + palSize;

    if (bih->biBitCount!=4 && bih->biBitCount!=8)
    {
        LOG(L"ERROR: Unsupported bit-depth. Must be 4- or 8-bit paletted image.");
        return;
    }
    TRACE1N(L"Loading %d-bit image...", bih->biBitCount);

	// copy palette
	TRACE1N(L"bitsOff = %08x", bitsOff);
	TRACE1N(L"palOff  = %08x", palOff);
    memset((BYTE*)&tex->palette, 0, 0x400);
    memcpy((BYTE*)&tex->palette, srcTex + palOff, palSize);
	// swap R and B
	for (int i=0; i<numColors; i++) 
	{
		BYTE blue = tex->palette[i].b;
		BYTE red = tex->palette[i].r;
		tex->palette[i].b = red;
		tex->palette[i].r = blue;
	}
	TRACE(L"Palette copied.");

	int k, m, j, w;
    int width = min(tex->header.width, bih->biWidth); // be safe
    int height = min(tex->header.height, bih->biHeight); // be safe

	// copy pixel data
    if (bih->biBitCount == 8)
    {
        for (k=0, m=bih->biHeight-1; k<height, m>=bih->biHeight - height; k++, m--)
        {
            memcpy(tex->data + k*tex->header.width, srcTex + bitsOff + m*width, width);
        }
    }
    else if (bih->biBitCount == 4)
    {
        for (k=0, m=bih->biHeight-1; k<tex->header.height, m>=bih->biHeight - height; k++, m--)
        {
            for (j=0; j<width/2; j+=1) {
                // expand ech nibble into full byte
                tex->data[k*(tex->header.width) + j*2] = srcTex[bitsOff + m*(bih->biWidth/2) + j] >> 4 & 0x0f;
                tex->data[k*(tex->header.width) + j*2 + 1] = srcTex[bitsOff + m*(bih->biWidth/2) + j] & 0x0f;
            }
        }
    }
	TRACE(L"Texture replaced.");
}

void FreePNGTexture(BITMAPINFO* bitmap) 
{
	if (bitmap != NULL) {
        pngdib_p2d_free_dib(NULL, (BITMAPINFOHEADER*)bitmap);
	}
}

void kservReadNumSlotsCallPoint1()
{
    __asm {
        pushfd 
        push ebp
        push eax
        push ebx
        push edx
        push esi
        push edi
        shr eax,2
        push eax // slot
        call kservReadNumSlots
        add esp,4 // pop parameters
        mov ecx, eax
        pop edi
        pop esi
        pop edx
        pop ebx
        pop eax
        pop ebp
        popfd
        retn
    }
}

void kservReadNumSlotsCallPoint2()
{
    __asm {
        pushfd 
        push ebp
        push eax
        push ebx
        push ecx
        push esi
        push edi
        shr eax,2
        push eax // slot
        call kservReadNumSlots
        add esp,4 // pop parameters
        mov edx, eax
        pop edi
        pop esi
        pop ecx
        pop ebx
        pop eax
        pop ebp
        popfd
        retn
    }
}

void kservReadNumSlotsCallPoint3()
{
    __asm {
        pushfd 
        push ebp
        push eax
        push ebx
        push ecx
        push edx
        push edi
        shr eax,2
        push eax // slot
        call kservReadNumSlots
        add esp,4 // pop parameters
        mov esi, eax
        pop edi
        pop edx
        pop ecx
        pop ebx
        pop eax
        pop ebp
        popfd
        retn
    }
}

void kservReadNumSlotsCallPoint4()
{
    __asm {
        pushfd 
        push ebp
        push ebx
        push ecx
        push edx
        push esi
        push edi
        sar edx,1
        push edx // slot
        call kservReadNumSlots
        add esp,4 // pop parameters
        pop edi
        pop esi
        pop edx
        pop ecx
        pop ebx
        pop ebp
        popfd
        retn
    }
}

KEXPORT DWORD kservReadNumSlots(DWORD slot)
{
    DWORD* pNumSlots = (DWORD*)data[NUM_SLOTS_PTR];
    if (slot >= XSLOT_FIRST)
        return XSLOT_LAST+1;
    return *pNumSlots;
}

