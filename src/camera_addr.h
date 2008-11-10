// ADDRESSES for camera module
BYTE allowedGames[] = {
    //gvPES2008demo,
    gvPES2008,
    gvPES2008v110,
    gvPES2008v120,
    gvPES2009demo,
    gvPES2009,
    gvPES2009v110,
    gvPES2009v120,
};

#define CODELEN 2
enum { 
    C_READ_CAMERA_ANGLE1, C_READ_CAMERA_ANGLE2,
};

#define NOCODEADDR {0,0}
DWORD codeArray[][CODELEN] = { 
    // PES2008 DEMO
    NOCODEADDR,
    // [Settings] PES2008 PC DEMO
    NOCODEADDR,
    // PES2008
    {
        0x6e0a7d, 0x6e452f,
    },
    // [Settings] PES2008 PC
    NOCODEADDR,
    NOCODEADDR,
    // PES2008 1.10
    {
        0x6e0f2d, 0x6e49df,
    },
    NOCODEADDR,
    // PES2008 1.20
    { 
        0x6e299d, 0x6e645f,
    },
    // PES2009 Demo
    {
        0xada937, 0x6d3d29,
    },
    // PES2009 
    {
        0xb1de07, 0xaeace9,
    },
    // PES2009 1.10
    {
        0xb1e3b7, 0xaeb299,
    },
    // PES2009 1.20
    {
        0xb58807, 0x6fcdd9,
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
    // PES2009 PC DEMO
    NODATAADDR,
    // PES2009 PC 
    NODATAADDR,
    // PES2009 PC 1.10
    NODATAADDR,
    // PES2009 PC 1.20
    NODATAADDR,
};

DWORD code[CODELEN];
DWORD data[DATALEN];
