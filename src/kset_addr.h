// ADDRESSES for kset.cpp
BYTE allowedGames[] = {
	gvPES2008demoSet,
	gvPES2008Set,
    gvPES2009demoSet,
    gvPES2009Set,
};

#define CODELEN 4
enum {
	C_CONTROLLERADDED, C_BEFORECONTROLLERADD, C_SOMEFUNCTION, C_QUALITYCHECK, 
};

#define NOCODEADDR {0,0,0,0},
DWORD codeArray[][CODELEN] = { 
  // PES2008 DEMO
  NOCODEADDR
	// [Settings] PES2008 PC DEMO
	{
		0x4049c9, 0x4048be, 0x415aaf, 0x4159c0,
	},
  // PES2008
  NOCODEADDR
	// [Settings] PES2008 PC
	{
		0, 0, 0, 0x418f40,
	},
  NOCODEADDR
  // PES2008 1.10
  NOCODEADDR
  NOCODEADDR
  // PES2008 1.20
  NOCODEADDR
  // PES2009 demo
  NOCODEADDR
  // PES2009
  NOCODEADDR
    // [Settings] PES2009 PC demo
    {
        0, 0, 0, 0x419b90,
    },
    // [Settings] PES2009 PC
    {
        0, 0, 0, 0x41c910,
    },
};

#define DATALEN 1
enum {
	CONTROLLER_NUMBER,	
};

#define NODATAADDR {0},
DWORD dataArray[][DATALEN] = {
  // PES2008 DEMO
  NODATAADDR
	// [Settings] PES2008 PC DEMO
	{
		0x45a6a2,
	},
  // PES2008 DEMO
  NODATAADDR
	// [Settings] PES2008 PC
	{
		0,
	},
  NODATAADDR
  NODATAADDR
  // PES2008 1.10
  NODATAADDR
  NODATAADDR
  // PES2008 1.20
  NODATAADDR
  // PES2009 DEMO
  NODATAADDR
  // PES2009 
  NODATAADDR
  // [Settings] PES2009 DEMO
  NODATAADDR
  // [Settings] PES2009 
  NODATAADDR
};

DWORD code[CODELEN];
DWORD data[DATALEN];
int gameVersion;
