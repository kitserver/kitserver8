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
        LOG(L"buffers deallocated.");
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

// VARIABLES
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
bool CreatePipeForKitBin(DWORD binId, HANDLE& handle, DWORD& size);
bool CreatePipeForFontBin(DWORD binId, HANDLE& handle, DWORD& size);
bool CreatePipeForNumbersBin(DWORD binId, HANDLE& handle, DWORD& size);
int GetBinType(DWORD id);

DWORD LoadPNGTexture(BITMAPINFO** tex, const wchar_t* filename);
void ApplyAlphaChunk(RGBQUAD* palette, BYTE* memblk, DWORD size);
static int read_file_to_mem(const wchar_t *fn,unsigned char **ppfiledata, int *pfilesize);
void ApplyDIBTexture(TEXTURE_ENTRY* tex, BITMAPINFO* bitmap);
void FreePNGTexture(BITMAPINFO* bitmap);
void ReplaceTexturesInBin(UNPACKED_BIN* bin, wstring files[], size_t n);
void DumpData(void* data, size_t size);

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
        mov [eax+0x2884], 0x144  // execute replaced code
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
    if (afsId == 6)
    {
        if (XBIN_KIT_FIRST <= binId && binId <= XBIN_KIT_LAST) 
            return CreatePipeForKitBin(binId, hfile, fsize);
        else if (XBIN_NUMBER_FIRST <= binId && binId <= XBIN_NUMBER_LAST) 
            return CreatePipeForNumbersBin(binId, hfile, fsize);
        else if (XBIN_FONT_FIRST <= binId && binId <= XBIN_FONT_LAST) 
            return CreatePipeForFontBin(binId, hfile, fsize);
    }
    return false;
}

int GetBinType(DWORD id)
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

/**
 * Create a pipe and write a dynamically created BIN into it.
 */
bool CreatePipeForKitBin(DWORD binId, HANDLE& handle, DWORD& size)
{
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

    int gkShift = (binId % 2)*2;
    wstring files[2];
    wstring kitFolder[] = {L"ga",L"gb",L"pa",L"pb"};
    for (int i=0; i<2; i++)
    {
        wstring filename(getPesInfo()->gdbDir);
        filename += L"GDB\\uni\\Russia\\"+kitFolder[i+gkShift]+L"\\kit.png";
        files[i] = filename;
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
bool CreatePipeForFontBin(DWORD binId, HANDLE& handle, DWORD& size)
{
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
    switch (GetBinType(binId))
    {
        case BIN_FONT_GA:
            filename += L"GDB\\uni\\Russia\\ga\\font.png";
            break;
        case BIN_FONT_GB:
            filename += L"GDB\\uni\\Russia\\gb\\font.png";
            break;
        case BIN_FONT_PA:
            filename += L"GDB\\uni\\Russia\\pa\\font.png";
            break;
        case BIN_FONT_PB:
            filename += L"GDB\\uni\\Russia\\pb\\font.png";
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
bool CreatePipeForNumbersBin(DWORD binId, HANDLE& handle, DWORD& size)
{
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
    switch (GetBinType(binId))
    {
        case BIN_NUMS_GA:
            dirname += L"GDB\\uni\\Russia\\ga\\";
            break;
        case BIN_NUMS_GB:
            dirname += L"GDB\\uni\\Russia\\gb\\";
            break;
        case BIN_NUMS_PA:
            dirname += L"GDB\\uni\\Russia\\pa\\";
            break;
        case BIN_NUMS_PB:
            dirname += L"GDB\\uni\\Russia\\pb\\";
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
        for (k=0, m=bih->biHeight-1; k<tex->header.height, m>=0; k++, m--)
        {
            for (j=0; j<bih->biWidth/2; j+=1) {
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

