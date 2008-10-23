// ADDRESSES for kserv.cpp
BYTE allowedGames[] = {
	gvPES2009,
};

#define CODELEN 2
enum {
	C_KIT_VTABLE, C_AFTER_CREATE_TEXTURE,
};

#define NOCODEADDR {0,0},
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
        0x1015ebc, 0x91407a,
    },
    // PES2009 v1.10
	NOCODEADDR
};

#define DATALEN 1 
enum {
	DUMMY,
};

#define NODATAADDR {0},
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
        0,
    },
    // PES2009 v1.10
	NODATAADDR
};

DWORD code[CODELEN];
DWORD data[DATALEN];
