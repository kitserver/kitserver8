// ADDRESSES for kserv.cpp
BYTE allowedGames[] = {
	gvPES2009,
    gvPES2009v110,
    gvPES2009v120,
};

#define CODELEN 7
enum {
	C_KIT_VTABLE, C_AFTER_CREATE_TEXTURE, C_AFTER_READ_NAMES,
    C_READ_NUM_SLOTS1, C_READ_NUM_SLOTS2, C_READ_NUM_SLOTS3, C_READ_NUM_SLOTS4,
};

#define NOCODEADDR {0,0,0,0,0,0,0},
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
        0, 0, 0, 0,
    },
    // PES2009 v1.10
    {
        0x1015f4c, 0x913eea, 0x8b6f05,
        0, 0, 0, 0,
    },
    // PES2009 v1.20
    {
        0x1030498, 0xdd722a, 0xe10255,

        0x806bdb, //shr2
        0x806c7e, //shr2
        0x80728c, //shr2
        0xec2782, //sar1
    },
};

#define DATALEN 5 
enum {
	NEXT_MATCH_DATA_PTR, PLAYERS_DATA,
    TEAM_NAMES, TEAM_KIT_INFO_OFFSET,
    NUM_SLOTS_PTR,
};

#define NODATAADDR {0,0,0,0,0},
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
        0x163f9ec, 0x163f9e4,
        0x163f9f0, 0x16a308,
        0x12a8d74,
    },
    // PES2009 v1.10
    {
        0x163f9ec, 0x163f9e8,
        0x163f9f0, 0x16a308,
        0x12a8d74,
    },
    // PES2009 v1.20
    {
        0x166ff14, 0x166ff04,
        0x166ff34, 0x16a308,
        0x13550ec,
    },
};

DWORD code[CODELEN];
DWORD data[DATALEN];
