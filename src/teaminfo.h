// teaminfo.h

#ifndef _TEAMINFO_H_
#define _TEAMINFO_H_

#pragma pack(4)

typedef WORD KCOLOR;

typedef struct _KIT_INFO {
    WORD unknown1[3];
    KCOLOR mainColor;
    KCOLOR editShirtColors[4];
    KCOLOR shortsFirstColor;
    KCOLOR editKitColors[9];
    BYTE unknown2[0x10];
    BYTE collar;
    BYTE editKitStyles[9];
    BYTE fontStyle;
    BYTE fontPosition;
    BYTE unknown3;
    BYTE fontShape;
    BYTE unknown4;
    BYTE frontNumberPosition;
    BYTE shortsNumberPosition;
    BYTE unknown5;
    BYTE unknown6;
    BYTE flagPosition;
    BYTE unknown7[2];
    WORD unknown8;
    BYTE model;
    BYTE unknown9[3];
    WORD slot;
} KIT_INFO; // size = 0x52

typedef struct _TEAM_KIT_INFO
{
    KIT_INFO ga;
    KIT_INFO pa;
    KIT_INFO gb;
    KIT_INFO pb;
} TEAM_KIT_INFO; // size = 0x148

typedef struct _TEAM_MATCH_DATA_INFO
{
    BYTE unknown1[4];
    WORD teamIdSpecial;
    WORD teamId;
    BYTE unknown2[0x2d30];
    BYTE kitSelection;
    BYTE unknown3[11];
    TEAM_KIT_INFO tki;
} TEAM_MATCH_DATA_INFO;

typedef struct _NEXT_MATCH_DATA_INFO
{
    TEAM_MATCH_DATA_INFO* home;
    TEAM_MATCH_DATA_INFO* away;
} NEXT_MATCH_DATA_INFO;

#endif
