#pragma once
#include "Common.h"

BOOL GetModuleBaseByPid(HANDLE Pid, PWCHAR ModuleName, CTLINFO_GET_MODULEBASE_RET* Info);

BOOL ReadProcessMemory(HANDLE ProcessId, PVOID Address, DWORD Size, PVOID Buffer);