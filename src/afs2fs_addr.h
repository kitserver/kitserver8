// ADDRESSES for afs2fs.cpp
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

#define CODELEN 1
enum { DUMMY };

#define NOCODEADDR {0}
DWORD codeArray[][CODELEN] = { 
    // PES2008 DEMO
    NOCODEADDR,
    // [Settings] PES2008 PC DEMO
    NOCODEADDR,
    // PES2008
    NOCODEADDR,
    // [Settings] PES2008 PC
    NOCODEADDR,
    NOCODEADDR,
    // PES2008 1.10
    NOCODEADDR,
    NOCODEADDR,
    // PES2008 1.20
    NOCODEADDR,
    // PES2009 DEMO
    NOCODEADDR,
    // PES2009
    NOCODEADDR,
    // PES2009 1.10
    NOCODEADDR,
};

#define DATALEN 3 
enum {
    BIN_SIZES_TABLE, SONGS_INFO_TABLE, BALLS_INFO_TABLE,
};

#define NODATAADDR {0,0,0},
DWORD dataArray[][DATALEN] = {
    // PES2008 DEMO
    NODATAADDR
    // [Settings] PES2008 PC DEMO
    NODATAADDR
    // PES2008
    { 
        0x1316b60, 0xd7ab78, 0xd15268,
    },
    // [Settings] PES2008 PC
    NODATAADDR
    NODATAADDR
    // PES2008 1.10
    { 
        0x1317b80, 0xd7bb80, 0xd16258,
    },
    NODATAADDR
    // PES2008 1.20
    {
        0x1318b80, 0xd7cc78, 0xd17240,
    },
    // PES2009 DEMO
    {
        0x12a6940, 0xe44b70, 0xdd070c,
    },
    // PES2009 
    {
        0x16cdce0, 0xfd3fe8, 0xfb98e4,
    },
    // PES2009 1.10
    {
        0x16cdce0, 0xfd3f48, 0xfb98d4,
    },
    // PES2009 1.20
    {
        0x1700c80, 0x1013108, 0xf944e8,
    },
};

DWORD code[CODELEN];
DWORD data[DATALEN];
