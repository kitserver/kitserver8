// ADDRESSES for kserv.cpp
BYTE allowedGames[] = {
	gvPES2009,
    gvPES2009v110,
};

#define CODELEN 3
enum {
	C_KIT_VTABLE, C_AFTER_CREATE_TEXTURE, C_AFTER_READ_NAMES,
};

#define NOCODEADDR {0,0,0},
DWORD codeArray[][CODELEN] = { 
	// PES2008
    NOCODEADDR
    NOCODEADDR
    NOCODEADDR
    NOCODEADDR
    NOCODEADDR
    NOCODEADDR
    NOCODEADDR
	NOCODEADDR
	// PES2009 demo
	NOCODEADDR
    // PES2009
    {
        0x1015ebc, 0x91407a, 0x8b73e5,
    },
    // PES2009 v1.10
    {
        0x1015f4c, 0x913eea, 0x8b6f05,
    },
};

#define DATALEN 4 
enum {
	NEXT_MATCH_DATA_PTR, PLAYERS_DATA,
    TEAM_NAMES, TEAM_KIT_INFO_OFFSET
};

#define NODATAADDR {0,0,0,0},
DWORD dataArray[][DATALEN] = {
	// PES2008
    NODATAADDR
    NODATAADDR
    NODATAADDR
    NODATAADDR
    NODATAADDR
    NODATAADDR
    NODATAADDR
	NODATAADDR
	// PES2009 demo
	NODATAADDR
    // PES2009
    {
        0x163f9ec, 0x163f9e8,
        0x163f9f0, 0x16a308
    },
    // PES2009 v1.10
    {
        0x163f9ec, 0x163f9e8,
        0x163f9f0, 0x16a308
    }
};

DWORD code[CODELEN];
DWORD data[DATALEN];
