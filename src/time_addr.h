// ADDRESSES for time module
BYTE allowedGames[] = {
    //gvPES2008demo,
    gvPES2008,
    gvPES2008v110,
    gvPES2008v120,
    gvPES2009,
    gvPES2009v110,
    gvPES2009v120,
};

#define CODELEN 3
enum { 
    C_SET_EXHIB_TIME, C_SET_CUP_ENDURANCE, C_SET_CUP_TIME,
};

#define NOCODEADDR {0,0,0}
DWORD codeArray[][CODELEN] = { 
    // PES2008 DEMO
    NOCODEADDR,
    // [Settings] PES2008 PC DEMO
    NOCODEADDR,
    // PES2008
    {
        0xaa6ed2, 0x8ff332, 0x90498d,
    },
    // [Settings] PES2008 PC
    NOCODEADDR,
    NOCODEADDR,
    // PES2008 1.10
    {
        0xaa7f72, 0x900262, 0x90475d,
    },
    NOCODEADDR,
    // PES2008 1.20
    { 
        0xaaa552, 0x905722, 0x9025cd,
    },
    // PES2009 Demo
    NOCODEADDR,
    // PES2009 
    { 
        0xdc1a89, 0x4f1358, 0x4ec90d,
    },
    // PES2009 1.10
    { 
        0xdd34b9, 0x4f13d8, 0x4ec98d,
    },
    // PES2009 1.20
    { 
        0xc745d9, 0x5273d8, 0x51f3dd,
    },
};

#define DATALEN 1 
enum {
    DUMMY
};

#define NODATAADDR {0}
DWORD dataArray[][DATALEN] = {
    // PES2008 DEMO
    NODATAADDR,
    // [Settings] PES2008 PC DEMO
    NODATAADDR,
    // PES2008
    NODATAADDR,
    // [Settings] PES2008 PC
    NODATAADDR,
    NODATAADDR,
    // PES2008 1.10
    NODATAADDR,
    NODATAADDR,
    // PES2008 1.20
    NODATAADDR,
    // PES2009 Demo
    NODATAADDR,
    // PES2009 
    NODATAADDR,
    // PES2009 1.10
    NODATAADDR,
    // PES2009 1.20
    NODATAADDR,
};

DWORD code[CODELEN];
DWORD data[DATALEN];
