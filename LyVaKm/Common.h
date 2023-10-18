#pragma once
#include "KernelStruct.h"
#include "vmp/VMProtectDDK.h"

#define POOL_TAG 'lyva'

#define CTL_READ_MEMORY 0x800
#define CTL_GET_MODULEBASE 0x801

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

// ----------------------
// 导出函数定义
// ----------------------

EXTERN_C PVOID PsGetProcessSectionBaseAddress(PEPROCESS Process);

EXTERN_C PPEB PsGetProcessPeb(PEPROCESS Process);