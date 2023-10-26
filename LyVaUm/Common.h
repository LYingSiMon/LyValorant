#pragma once
#include "vmp/VMProtectSDK.h"
#include "Cheat.h"
#include <Windows.h>
#include <string>

#define CTL_READ_MEMORY 0x800
#define CTL_GET_MODULEBASE 0x801
#define CTL_SEND_R3_VAR 0x802

#define DRV_SERVICE_NAME "LyVa"

struct CTLINFO_READ_MEMORY
{
	DWORD Pid;
	PVOID Address;
	DWORD Size;
};

struct CTLINFO_GET_MODULEBASE
{
	DWORD Pid;
	WCHAR ModuleName[MAX_PATH];
};
struct CTLINFO_GET_MODULEBASE_RET
{
	ULONG_PTR Base;
	DWORD Size;
};

struct R3Var
{
	DWORD Pid_Game;
	DWORD Pid_LyVaUm;
	ULONG_PTR Base_Exe;
	ULONG_PTR Base_PlayerInfo;
	ULONG_PTR Base_DrvReceiver;
};

ULONG_PTR GetModuleBaseByPid(DWORD Pid, PCHAR ModuleName, DWORD& Size);

BOOL ReadProcMemByOffset(HANDLE hProc, ULONG_PTR Offset[], DWORD OffsetCount, PVOID Result, DWORD Size);

ULONG_PTR ScanSignature(HANDLE hProc, ULONG_PTR StartAddress, DWORD Size, BYTE Code[], BYTE Rule[]);

BOOL DrvReadProcMemory(CTLINFO_READ_MEMORY ReadInfo, PVOID Buffer);
BOOL DrvGetModuleBase(CTLINFO_GET_MODULEBASE ReadInfo);
VOID DrvSendR3Var(R3Var R3);

BOOL InstallDvr(PCHAR DrvPath, PCHAR ServiceName);
BOOL StartDvr(PCHAR ServiceName);
BOOL StopDvr(PCHAR ServiceName);
BOOL UninstallDvr(PCHAR ServiceName);

std::string GetDiskSerialMD5();
BOOL IsHwidOk();
