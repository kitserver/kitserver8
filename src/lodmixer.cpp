// lodmixer.cpp
#define UNICODE
#define THISMOD &k_lodmixer

#include <windows.h>
#include <stdio.h>
#include "kload_exp.h"
#include "lodmixer.h"
#include "lodmixer_addr.h"
#include "dllinit.h"

#define FLOAT_ZERO 0.0001f

KMOD k_lodmixer={MODID,NAMELONG,NAMESHORT,DEFAULT_DEBUG};

HINSTANCE hInst;
LMCONFIG _lmconfig = {
    {DEFAULT_WIDTH, DEFAULT_HEIGHT, DEFAULT_ASPECT_RATIO}, 
    {DEFAULT_LOD_SWITCH1, DEFAULT_LOD_SWITCH2},
    DEFAULT_ASPECT_RATIO_CORRECTION_ENABLED,
    DEFAULT_CONTROLLER_CHECK_ENABLED,
    DEFAULT_LODCHECK1,
};

EXTERN_C BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved);

void initLodMixer();
void lodmixerConfig(char* pName, const void* pValue, DWORD a);
void modifySettings();
void getResolution(DWORD& width, DWORD& height);
void setResolution(DWORD width, DWORD height);
void setAspectRatio(float aspectRatio, bool manual);
void setSwitchesForLOD(float switch1, float switch2);
void lodAtModeCheckCallPoint();
void lodAtSettingsReadPoint();
void lodAtSettingsResetPoint();
KEXPORT DWORD lodAtModeCheck(DWORD mode);

EXTERN_C BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		hInst=hInstance;
		RegisterKModule(&k_lodmixer);

		if (!checkGameVersion()) {
			LOG(L"Sorry, your game version isn't supported!");
			return false;
		}

		copyAdresses();
		hookFunction(hk_D3D_Create, initLodMixer);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		LOG(L"Detaching dll...");
	}
	return true;
}

void modifySettings()
{
    setResolution(_lmconfig.screen.width, _lmconfig.screen.height);
    if (_lmconfig.aspectRatioCorrectionEnabled)
    {
        DWORD width = 0, height = 0;
        getResolution(width,height);
        float ar = (_lmconfig.screen.aspectRatio > FLOAT_ZERO) ?
            _lmconfig.screen.aspectRatio :  // manual
            float(width) / float(height);   // automatic

        setAspectRatio(ar, _lmconfig.screen.aspectRatio > FLOAT_ZERO);
    }
    setSwitchesForLOD(_lmconfig.lod.switch1, _lmconfig.lod.switch2);
}

void initLodMixer()
{
    // skip the settings-check call
    LOG(L"Initializing LOD mixer...");

    getConfig("lodmixer", "screen.width", DT_DWORD, 1, lodmixerConfig);
    getConfig("lodmixer", "screen.height", DT_DWORD, 2, lodmixerConfig);
    getConfig("lodmixer", "screen.aspect-ratio", DT_FLOAT, 3, lodmixerConfig);
    getConfig("lodmixer", "lod.switch1", DT_FLOAT, 4, lodmixerConfig);
    getConfig("lodmixer", "lod.switch2", DT_FLOAT, 5, lodmixerConfig);
    getConfig("lodmixer", "aspect-ratio.correction.enabled", DT_DWORD, 6, lodmixerConfig);
    getConfig("lodmixer", "controller.check.disabled", DT_DWORD, 7, lodmixerConfig);
    getConfig("lodmixer", "lod.check1", DT_DWORD, 8, lodmixerConfig);
    LOG2N(L"Screen resolution to force: %dx%d", 
            _lmconfig.screen.width, _lmconfig.screen.height);

    BYTE* bptr = (BYTE*)code[C_SETTINGS_CHECK];
    DWORD protection;
    DWORD newProtection = PAGE_EXECUTE_READWRITE;
    if (bptr)
    {
        if (VirtualProtect(bptr, 6, newProtection, &protection)) {
            /* CALL */
            bptr[0] = 0xe8;
            DWORD* ptr = (DWORD*)(code[C_SETTINGS_CHECK] + 1);
            ptr[0] = (DWORD)modifySettings - (DWORD)(code[C_SETTINGS_CHECK] + 5);
            /* NOP */ 
            bptr[5] = 0x90;
            LOG(L"Settings check disabled. Settings overwrite enabled.");
        } 
    }

    HookCallPoint(code[C_SETTINGS_READ], lodAtSettingsReadPoint, 6, 0, true);
    HookCallPoint(code[C_SETTINGS_RESET], lodAtSettingsResetPoint, 6, 0, true);
    if (code[C_VIDEO_CHECK1]!=0)
    {
        bptr = (BYTE*)code[C_VIDEO_CHECK1];
        if (VirtualProtect(bptr, 6, newProtection, &protection)) {
            /* jmp */  memcpy(bptr,"\xe9\x5d\x01\x00\x00",5);
            /* nop */  bptr[5] = 0x90;
        }
    }
    if (code[C_VIDEO_CHECK2]!=0)
    {
        bptr = (BYTE*)code[C_VIDEO_CHECK2];
        if (VirtualProtect(bptr, 4, newProtection, &protection)) {
            // /* jmp to end */  memcpy(bptr,"\xeb\x5b",2);
            /* jmp */  memcpy(bptr,"\xe9\xaf\x00\x00\x00",5);
            /* nop */  bptr[5] = 0x90;
        }
    }

    if (_lmconfig.controllerCheckEnabled)
    {
        bptr = (BYTE*)code[C_MODE_CHECK];
        if (bptr && getPesInfo()->gameVersion < gvPES2009demo)
        {
            // need to insert a bit of code, to handle the special
            // case of Exhibition Mode. 61 bytes before C_MODE_CHECK is a good place.
            BYTE* codeInsert = bptr - 61;
            if (VirtualProtect(bptr, 4, newProtection, &protection)) {
                BYTE patch[] = {0xeb,0xc1}; // jmp short "TO_TEST_ECX"
                memcpy(bptr, patch, sizeof(patch));
                if (VirtualProtect(codeInsert, 12, newProtection, &protection)) {
                    BYTE patch2[] = {
                        0x85,0xc9,  // test ecx,ecx
                        0x74,0x03,  // je short "TO_XOR_EBX"
                        0x33,0xc9,  // xor ecx,ecx
                        0x41,       // inc ecx
                        0x33,0xdb,  // xor ebx,ebx  - the command that was replaced with jmp
                        0xeb,0x34,  // jmp short "BACK_TO_AFTER_JMP"
                    };
                    memcpy(codeInsert, patch2, sizeof(patch2));
                    LOG(L"Mode check disabled for controller selection.");
                }
            } 
        }
        else if (bptr)
        {
            // PES2009: use callpoint
            HookCallPoint(code[C_MODE_CHECK], lodAtModeCheckCallPoint, 6, 1);
            LOG(L"Mode check disabled for controller selection.");
        }
    }

    if (!_lmconfig.lodCheck1)
    {
        bptr = (BYTE*)code[C_LODCHECK_1];
        if (bptr && VirtualProtect(bptr, 4, newProtection, &protection)) {
            bptr[0] = 0x90;  // nop
            bptr[1] = 0x90;  // nop
            LOG(L"LOD check: lowest lod level disabled.");
        }
    }

    LOG(L"Initialization complete.");
    unhookFunction(hk_D3D_Create, initLodMixer);
}

void lodmixerConfig(char* pName, const void* pValue, DWORD a)
{
	switch (a) {
		case 1:	// width
			_lmconfig.screen.width = *(DWORD*)pValue;
			break;
		case 2: // height
			_lmconfig.screen.height = *(DWORD*)pValue;
			break;
		case 3: // aspect ratio
			_lmconfig.screen.aspectRatio = *(float*)pValue;
			break;
		case 4: // LOD 
			_lmconfig.lod.switch1 = *(float*)pValue;
			break;
		case 5: // LOD
			_lmconfig.lod.switch2 = *(float*)pValue;
			break;
		case 6: // LOD
			_lmconfig.aspectRatioCorrectionEnabled = *(DWORD*)pValue != 0;
			break;
		case 7: // Controller check
			_lmconfig.controllerCheckEnabled = *(DWORD*)pValue != 0;
			break;
        case 8: // lod-check
            _lmconfig.lodCheck1 = *(DWORD*)pValue != 0;
            break;
	}
}

void getResolution(DWORD& width, DWORD& height)
{
	DWORD protection;
    DWORD newProtection = PAGE_EXECUTE_READWRITE;

    // get resolution
    if (VirtualProtect((BYTE*)data[SCREEN_WIDTH], 4, newProtection, &protection))
        width = *(DWORD*)data[SCREEN_WIDTH]; 
    if (VirtualProtect((BYTE*)data[SCREEN_HEIGHT], 4, newProtection, &protection))
        height = *(DWORD*)data[SCREEN_HEIGHT];
}

void setResolution(DWORD width, DWORD height)
{
	DWORD protection;
    DWORD newProtection = PAGE_EXECUTE_READWRITE;

    // set resolution
    if (width>0 && height>0)
    {
        if (VirtualProtect((BYTE*)data[SCREEN_WIDTH], 4, newProtection, &protection))
            *(DWORD*)data[SCREEN_WIDTH] = width;
        if (VirtualProtect((BYTE*)data[SCREEN_HEIGHT], 4, newProtection, &protection))
            *(DWORD*)data[SCREEN_HEIGHT] = height;

        LOG2N(L"Resolution set: %dx%d", width, height);
    }
}

void setAspectRatio(float aspectRatio, bool manual)
{
	DWORD protection;
    DWORD newProtection = PAGE_EXECUTE_READWRITE;

    if (aspectRatio <= FLOAT_ZERO) // safety-check
        return;

    if (fabs(aspectRatio - 1.33333) < fabs(aspectRatio - 1.77777)) {
        // closer to 4:3
        *(DWORD*)data[WIDESCREEN_FLAG] = 0;
        if (VirtualProtect((BYTE*)data[RATIO_4on3], 4, newProtection, &protection)) {
            *(float*)data[RATIO_4on3] = aspectRatio;
        }
        LOG(L"Widescreen mode: no");
    } else {
        // closer to 16:9
        *(DWORD*)data[WIDESCREEN_FLAG] = 1;
        if (VirtualProtect((BYTE*)data[RATIO_16on9], 4, newProtection, &protection)) {
            *(float*)data[RATIO_16on9] = aspectRatio;
        }
        LOG(L"Widescreen mode: yes");
    }
    LOG1F(L"Aspect ratio: %0.5f", aspectRatio);
    LOG1S(L"Aspect ratio type: %s", (manual)?L"manual":L"automatic");
}

void setSwitchesForLOD(float switch1, float switch2)
{
	DWORD protection;
    DWORD newProtection = PAGE_EXECUTE_READWRITE;
    if (_lmconfig.lod.switch1 > FLOAT_ZERO && data[LOD_SWITCH1]) 
    {
        if (VirtualProtect((BYTE*)data[LOD_SWITCH1], 4, newProtection, &protection)) {
            *(float*)data[LOD_SWITCH1] = switch1;
            LOG1F(L"LOD: 1st switch (0->1) set to: %0.5f", switch1);
        }
    }
    if (_lmconfig.lod.switch2 > FLOAT_ZERO && data[LOD_SWITCH2]) 
    {
        if (VirtualProtect((BYTE*)data[LOD_SWITCH2], 4, newProtection, &protection)) {
            *(float*)data[LOD_SWITCH2] = switch2;
            LOG1F(L"LOD: 2nd switch (1->2) set to: %0.5f", switch2);
        }
    }
}

void lodAtModeCheckCallPoint()
{
    __asm {
        pushfd 
        push ebp
        push eax
        push ebx
        push edx
        push esi
        push edi
        mov [edi+0x13c], eax  // execute replaced code
        push ecx
        call lodAtModeCheck
        mov ecx,eax
        add esp,4     // pop parameters
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

KEXPORT DWORD lodAtModeCheck(DWORD mode)
{
    if (mode != 0)
        return 1;
    return 0;
}

void lodAtSettingsReadPoint()
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
        call modifySettings
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

void lodAtSettingsResetPoint()
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
        call modifySettings
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

