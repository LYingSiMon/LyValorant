#pragma once
#include "KernelStruct.h"
#include "vmp/VMProtectDDK.h"

// ----------------------
// 宏定义
// ----------------------

#define POOL_TAG 'lyva'

#define CTL_READ_MEMORY 0x800
#define CTL_GET_MODULEBASE 0x801
#define CTL_SEND_R3_VAR 0x802

// ----------------------
// 结构体
// ----------------------

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

// ----------------------
// 导出函数
// ----------------------

EXTERN_C PVOID PsGetProcessSectionBaseAddress(PEPROCESS Process);

EXTERN_C PPEB PsGetProcessPeb(PEPROCESS Process);

EXTERN_C NTSTATUS NTAPI MmCopyVirtualMemory
(
	PEPROCESS SourceProcess,
	PVOID SourceAddress,
	PEPROCESS TargetProcess,
	PVOID TargetAddress,
	SIZE_T BufferSize,
	KPROCESSOR_MODE PreviousMode,
	PSIZE_T ReturnSize);

// ----------------------
// 功能函数
// ----------------------

BOOL GetModuleBaseByPid(HANDLE Pid, PWCHAR ModuleName, CTLINFO_GET_MODULEBASE_RET* Info);

BOOL ReadProcessMemory(HANDLE ProcessId, PVOID Address, DWORD Size, PVOID Buffer);

BOOL WriteProcessMemory(HANDLE Pid, PVOID Address, DWORD Size, PVOID Buffer);

BOOL ReadProcMemByOffset(HANDLE Pid, ULONG_PTR Offset[], DWORD OffsetCount, PVOID Result, DWORD Size);