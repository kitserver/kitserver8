// kserv.h

#define MODID 100
#define NAMELONG L"Kitserver Module 8.1.2.0"
#define NAMESHORT L"KSERV"
#define DEFAULT_DEBUG 0

#pragma pack(4)

typedef DWORD (*INIT_NEW_KIT)(DWORD);

KEXPORT DWORD hookVtableFunction(DWORD vtableAddr, DWORD offset, void* func);


typedef struct _PES_TEXTURE {
    DWORD* vtable;	// == 0xffab80 for PES2009
	BYTE unknown1[8];
	DWORD width;
	DWORD height;
	BYTE unknown2[0xc];
	IDirect3DTexture9* tex;
} PES_TEXTURE;

typedef struct _PES_TEXTURE_PACKAGE {
    DWORD* vtable;	// == 0x1009df8 for PES2009
	BYTE unknown1[8];
	PES_TEXTURE* tex;
} PES_TEXTURE_PACKAGE;

typedef struct _KIT_OBJECT {
    DWORD* vtable;	// == code[C_KIT_VTABLE]
	BYTE unknown1[0x40];
	PES_TEXTURE_PACKAGE* kitTex; 
	PES_TEXTURE_PACKAGE* flagTex; 
	BYTE unknown2[0x30];
	WORD teamId;
	BYTE unknown3[6];
	BYTE isAfsKit;
    BYTE unknown4[3];
	DWORD kitIdx;
} KIT_OBJECT;

typedef struct _TEAM_NAME {
    BYTE unknown1[0x46];
    char name[0x46];
    char abbr[4];
    WORD teamId;
    WORD teamId2;
    BYTE unknown2[8];
} TEAM_NAME; // size = 0x9c

typedef struct _PNG_CHUNK_HEADER {
    DWORD dwSize;
    DWORD dwName;
} PNG_CHUNK_HEADER;

typedef struct _TEXTURE_ENTRY_HEADER
{
    BYTE sig[4]; // "WE00"
    BYTE unknown1; // 0
    BYTE unknown2; // 0
    BYTE unknown3; // 03
    BYTE bpp;
    WORD width;
    WORD height;
    WORD paletteOffset;
    WORD dataOffset;
} TEXTURE_ENTRY_HEADER;

typedef struct _PALETTE_ENTRY
{
    BYTE b;
    BYTE g;
    BYTE r;
    BYTE a;
} PALETTE_ENTRY;

typedef struct _TEXTURE_ENTRY
{
    TEXTURE_ENTRY_HEADER header;
    PALETTE_ENTRY palette[256];
    BYTE data[1];
} TEXTURE_ENTRY;

