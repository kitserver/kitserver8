// ADDRESSES for lodmixer.cpp
BYTE allowedGames[] = {
	gvPES2008,
	gvPES2008v110,
	gvPES2008v120,
	gvPES2009demo,
	gvPES2009,
	gvPES2009v110,
	gvPES2009v120,
};

#define CODELEN 7
enum {
	C_SETTINGS_CHECK, C_MODE_CHECK,
    C_SETTINGS_READ, C_SETTINGS_RESET, C_VIDEO_CHECK1, C_VIDEO_CHECK2, 
    C_LODCHECK_1,
};

#define NOCODEADDR {0,0,0,0,0,0,0},
DWORD codeArray[][CODELEN] = { 
    // PES2008 DEMO
    NOCODEADDR
	// [Settings] PES2008 PC DEMO
    NOCODEADDR
    // PES2008
	{
        0xbb8be3, 0xaa9542,
        0, 0, 0, 0,
        0,
	},
	// [Settings] PES2008 PC
    NOCODEADDR
    NOCODEADDR
    // PES2008 1.10
	{
        0xbb89d3, 0xaaa5e2,
        0, 0, 0, 0,
        0,
	},
    NOCODEADDR
    // PES2008 1.20
    {
        0xbbd083, 0xaacbc2,
        0, 0, 0, 0,
        0,
    },
    // PES2009 Demo
    {
        0, 0,
        0xc7c730, 0xc7c3a4, 0xc79741, 0xc79a34,
        0xc2225f,
    },
    // PES2009 
    {
        0, 0xdbffce,
        0xe19460, 0xe190d4, 0xe15e51, 0xe16144,
        0x92a2ef,
    },
    // PES2009 1.10
    {
        0, 0xdd19be,
        0xe19f80, 0xe19bf4, 0xe16971, 0xe16c64,
        0x9297ff,
    },
    // PES2009 1.20
    {
        0, 0xc72b0e,
        0xe2d760, 0xe2d3d4, 0xe2a0d1, 0xe2a3c4,
        0xdcc44f,
    },
};

#define DATALEN 7
enum {
    SCREEN_WIDTH, SCREEN_HEIGHT, WIDESCREEN_FLAG,
    RATIO_4on3, RATIO_16on9,
    LOD_SWITCH1, LOD_SWITCH2,
};

#define NODATAADDR {0,0,0,0,0,0,0},
DWORD dataArray[][DATALEN] = {
    // PES2008 DEMO
	NODATAADDR
	// [Settings] PES2008 PC DEMO
	NODATAADDR
    // PES2008
    {
        0x12509c8, 0x12509cc,0x12509d0,
        0xdb43dc, 0xdb4754,
        0xdb1fe4, 0xdb1fe0,
    },
	// [Settings] PES2008 PC
	NODATAADDR
	NODATAADDR
    // PES2008 1.10
    {
        0x12519f0, 0x12519f4,0x12519f8,
        0xdb5484, 0xdb57fc,
        0xdb3044, 0xdb3040,
    },
    NODATAADDR
    // PES2008 1.20
    {
        0x1252a08, 0x1252a0c,0x1252a10, 
        0xdb6434, 0xdb67b0,
        0xdb4024, 0xdb4084,
    },
    // PES2009 DEMO
    {
        0x1217068, 0x121706c, 0x1217070,
        0xe86ae0, 0xe897d8,
        0, 0,
    },
    // PES2009 
    {
        0x163df18, 0x163df1c, 0x163df20,
        0x1024b9c, 0x1027784,
        0, 0, //0x1024c54, 0x13c6c70 ???
    },
    // PES2009 1.10
    {
        0x163df18, 0x163df1c, 0x163df20,
        0x1024c04, 0x1027820,
        0, 0,
    },
    // PES2009 1.20
    {
        0x166fb88, 0x166fb8c, 0x166fb90,
        0x1058bf8, 0x105b9f0, 
        0, 0,
    },
};

DWORD code[CODELEN];
DWORD data[DATALEN];
