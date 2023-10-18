#include "Memory.h"

#include <ntdef.h>
#include <wdm.h>

BOOL GetModuleBaseByPid(HANDLE Pid, PWCHAR ModuleName, CTLINFO_GET_MODULEBASE_RET* Info)
{
    BOOL Status = FALSE;
    PEPROCESS Process;

    // pid 转 eprocess
    if (!NT_SUCCESS(PsLookupProcessByProcessId(Pid, &Process)))
    {
        KdPrintEx((0, 0, "[LyVa][%s] PsLookupProcessByProcessId faild. \n", __FUNCTION__));
        return Status;
    }

    // 获取 peb
    PPEB_EX Peb = (PPEB_EX)PsGetProcessPeb(Process);
    if (!Peb)
    {
        KdPrintEx((0, 0, "[LyVa][%s] PsGetProcessPeb faild. \n", __FUNCTION__));
        return Status;
    }

    // 遍历模块链表
    KAPC_STATE ApcState;
    KeStackAttachProcess(Process, &ApcState);
    __try 
    {
        PPEB_LDR_DATA_EX Ldr = (PPEB_LDR_DATA_EX)Peb->Ldr;
        PLIST_ENTRY List = &Ldr->InLoadOrderModuleList;
        PLIST_ENTRY CurrentList = List->Flink;
        WCHAR DllPath[MAX_PATH + 1] = { 0 };

        while (CurrentList != List)
        {
            PLDR_DATA_TABLE_ENTRY_EX TableEntry = (PLDR_DATA_TABLE_ENTRY_EX)CurrentList;

            RtlZeroMemory(DllPath, MAX_PATH - 1);
            wcsncpy_s(
                DllPath,
                MAX_PATH,
                TableEntry->FullDllName.Buffer,
                TableEntry->FullDllName.Length / sizeof(WCHAR));
            if (wcsstr(DllPath, ModuleName) != 0)
            {
                Status = TRUE;
                KdPrintEx((0, 0, "[LyVa][%s] Base=%p DllPath=%wZ \n", __FUNCTION__, TableEntry->DllBase, &TableEntry->FullDllName));
                
                Info->Base = (ULONG_PTR)TableEntry->DllBase;
                Info->Size = TableEntry->SizeOfImage;
                break;
            }

            CurrentList = CurrentList->Flink;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }

    KeUnstackDetachProcess(&ApcState);
    ObDereferenceObject(Process);
    return TRUE;
}

BOOL ReadProcessMemory(
    HANDLE Pid,
    PVOID Address,
    DWORD Size,
    PVOID Buffer)
{
    NTSTATUS Status = FALSE;
    PEPROCESS Process = NULL;
    PVOID BaseAddress = NULL;
    KAPC_STATE ApcState;

    // pid 转 eprocess
    if (!NT_SUCCESS(PsLookupProcessByProcessId(Pid, &Process)))
    {
        KdPrintEx((0, 0, "[LyVa][%s] PsLookupProcessByProcessId faild. \n", __FUNCTION__));
        return Status;
    }

    // 读内存
    KeStackAttachProcess(Process, &ApcState);
    __try
    {
        ProbeForRead(Address, Size, sizeof(BYTE));
        RtlCopyMemory(Buffer, Address, Size);
    }
    __except (1)
    {
        goto end;
    }
    
    Status = TRUE;
end: 
    KeUnstackDetachProcess(&ApcState);
    ObDereferenceObject(Process);

    return Status;
}