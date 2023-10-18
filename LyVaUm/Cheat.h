#pragma once
#include <Windows.h>

struct GameInfo {
	HWND Hwnd;
	RECT GameWndRect;
	DWORD Pid;
	HANDLE HProc;
	ULONG_PTR Base_exe;
	DWORD Size_exe;
	ULONG_PTR Base_PlayerInfo;
	FLOAT Matrix[4][4];
	BYTE SelfCamp;
	FLOAT SelfXYZ[3];
	UINT PlayersCount;
};

struct PlayerInfo {
	FLOAT X;
	FLOAT Y;
	FLOAT Z;
	FLOAT Screen[3];
	BYTE Id;
	BYTE Self;
	BYTE Camp;
	FLOAT Health;
};

extern GameInfo g_GameInfo;
extern PlayerInfo g_PlayerInfo[0x10];
extern HANDLE g_hDevice;

BOOL CheatInit();
BOOL GameXYZToScreen(FLOAT mcax, FLOAT mcay, FLOAT mcaz, DWORD Index);
VOID MatrixTransit();