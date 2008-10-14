// ADDRESSES for afsio.cpp
BYTE allowedGames[] = {
    //gvPES2008demo,
    gvPES2008,
    gvPES2008v110,
    gvPES2008v120,
    gvPES2009demo,
    gvPES2009,
};

#define CODELEN 7
enum {
    C_AT_GET_BINBUFFERSIZE, C_BEFORE_READ,
    C_AFTER_CREATE_EVENT, C_AT_CLOSE_HANDLE, C_AFTER_GET_OFFSET_PAGES, C_AT_GET_SIZE,
    C_AT_GET_IMG_SIZE,
};

#define NOCODEADDR {0,0,0,0,0,0,0},
DWORD codeArray[][CODELEN] = { 
    // PES2008 DEMO
    {
        0, 0, 
        0, 0, 0, 0,
        0,
    },
    // [Settings] PES2008 PC DEMO
    NOCODEADDR
    // PES2008
    {
        0xa5ea01, 0x4a0e4b, 
        0x496df0, 0x497010, 0x495159, 0x4950e1,
        0x496e68,
    },
    // [Settings] PES2008 PC
    NOCODEADDR
    NOCODEADDR
    // PES2008 1.10
    {
        0xa5ed51, 0x4a17ab, 
        0x497740, 0x497960, 0x495aa9, 0x495a31,
        0x4977b8,
    },
    NOCODEADDR
    // PES2008 1.20
    {
        0xa60f01, 0x4a172b, 
        0x4976d0, 0x4978f0, 0x495969, 0x4958f1,
        0x497748,

        // 0x4976d0: after CreateEvent 
        //           (eax has event id, [[esp+28]+0c] - item offset in pages, pointer to
        //           relative .img file name is also on the stack)
        // 0x4978f0: after CloseHandle: event is being released
        // 0x4a174f: after reading thread was signaled to start reading
    },
    // PES2009 DEMO
    {
        0xb2f161, 0x4a81eb,
        0x49f0e0, 0x49f300, 0x4966d9, 0x496661,
        0x49f158,  
    },
    // PES2009
    {
        0x8768e1, 0x4a6f6b,
        0x49cc30, 0x49ce50, 0x49aeb9, 0x49ae41,
        0x49cca8,  
    },
};

#define DATALEN 1 
enum {
    BIN_SIZES_TABLE
};

#define NODATAADDR {0},
DWORD dataArray[][DATALEN] = {
    // PES2008 DEMO
    NODATAADDR
    // [Settings] PES2008 PC DEMO
    NODATAADDR
    // PES2008
    { 
        0x1316b60,
    },
    // [Settings] PES2008 PC
    NODATAADDR
    NODATAADDR
    // PES2008 1.10
    { 
        0x1317b80,
    },
    NODATAADDR
    // PES2008 1.20
    {
        0x1318b80,
    },
    // PES2009 DEMO
    {
        0x12a6940,
    },
    // PES2009
    {
        0x16cdce0,
    },
};

DWORD code[CODELEN];
DWORD data[DATALEN];
