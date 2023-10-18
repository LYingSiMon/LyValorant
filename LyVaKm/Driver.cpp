#include "Common.h"
#include "Memory.h"


PDRIVER_OBJECT g_DriverObject;
PDEVICE_OBJECT g_DeviceObject;
UNICODE_STRING g_DeviceName;
UNICODE_STRING g_SymLinkName;

NTSTATUS DriverUnload(IN PDRIVER_OBJECT DriverObject)
{
	UNREFERENCED_PARAMETER(DriverObject);

	KdPrintEx((0, 0, "[LyVa][%s] \n", __FUNCTION__));

	// ɾ����������
	IoDeleteSymbolicLink(&g_SymLinkName);

	// ɾ���豸
	IoDeleteDevice(g_DeviceObject);

	return STATUS_SUCCESS;
}

NTSTATUS IoctlDispatcher(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION Stack = IoGetCurrentIrpStackLocation(Irp);
	PVOID InputBuffer = Irp->AssociatedIrp.SystemBuffer;
	PVOID OutputBuffer = Irp->AssociatedIrp.SystemBuffer;
	ULONG InputBufferLength = Stack->Parameters.DeviceIoControl.InputBufferLength;
	ULONG OutputBufferLength = Stack->Parameters.DeviceIoControl.OutputBufferLength;
	ULONG Info = 0;

    switch (Stack->Parameters.DeviceIoControl.IoControlCode)
    {
    case CTL_CODE(FILE_DEVICE_UNKNOWN, CTL_READ_MEMORY, METHOD_BUFFERED, FILE_ANY_ACCESS):
    {
		CTLINFO_READ_MEMORY* pInfo = (CTLINFO_READ_MEMORY*)InputBuffer;
        KdPrintEx((0, 0, "[LyVa][%s] CTL_READ_MEMORY | Param:%d,%p,%x \n", __FUNCTION__, pInfo->Pid, pInfo->Address, pInfo->Size));

		// �����ڴ�
		PVOID pBuffer = ExAllocatePoolWithTag(PagedPool, pInfo->Size, POOL_TAG);
		if (!pBuffer)
			break;

		// ��ȡ�ڴ�
		if (!ReadProcessMemory((HANDLE)pInfo->Pid, pInfo->Address, pInfo->Size, pBuffer))
			break;
		
		// ���ظ� r3
		RtlCopyMemory(OutputBuffer, pBuffer, OutputBufferLength);

		// �ͷ��ڴ�
		ExFreePool(pBuffer);

		Info = OutputBufferLength;
        break;
    }
	case CTL_CODE(FILE_DEVICE_UNKNOWN, CTL_GET_MODULEBASE, METHOD_BUFFERED, FILE_ANY_ACCESS):
	{
		CTLINFO_GET_MODULEBASE* pInfo = (CTLINFO_GET_MODULEBASE*)InputBuffer;
		KdPrintEx((0, 0, "[LyVa][%s] CTL_READ_MEMORY | Param:%d,%S \n", __FUNCTION__, pInfo->Pid, pInfo->ModuleName));

		// ��ȡģ�����ַ
		CTLINFO_GET_MODULEBASE_RET Result = { 0 };
		if (!GetModuleBaseByPid((HANDLE)pInfo->Pid, pInfo->ModuleName, &Result))
			break;

		// ���ظ� r3
		RtlCopyMemory(OutputBuffer, &Result, sizeof(CTLINFO_GET_MODULEBASE_RET));

		Info = sizeof(CTLINFO_GET_MODULEBASE_RET);
		break;
	}
    default:
        break;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Info;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

NTSTATUS CreateDevice(IN PDRIVER_OBJECT DriverObject)
{
	NTSTATUS status;

	// ��ʼ���豸��,����������
	RtlInitUnicodeString(&g_DeviceName, L"\\Device\\LyVa");
	RtlInitUnicodeString(&g_SymLinkName, L"\\??\\LyVa");

	// �����豸
	status = IoCreateDevice(DriverObject, 0, &g_DeviceName, FILE_DEVICE_UNKNOWN, 0, TRUE, &g_DeviceObject);
	if (!NT_SUCCESS(status))
	{
		KdPrintEx((0, 0, "[LyVa][%s] IoCreateDevice error:%x \n", __FUNCTION__, status));
		return status;
	}

	// ������������
	status = IoCreateSymbolicLink(&g_SymLinkName, &g_DeviceName);
	if (!NT_SUCCESS(status))
	{
		KdPrintEx((0, 0, "[LyVa][%s] IoCreateSymbolicLink error:%x \n", __FUNCTION__, status));
		return status;
	}

	return STATUS_SUCCESS;
}

NTSTATUS DrvCreateClose(_In_ PDEVICE_OBJECT DeviceObject, _In_ PIRP Irp)
{
	UNREFERENCED_PARAMETER(DeviceObject);

	Irp->IoStatus.Status = STATUS_SUCCESS;
	Irp->IoStatus.Information = 0;

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return STATUS_SUCCESS;
}

EXTERN_C NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	VM_BEGIN();

	UNREFERENCED_PARAMETER(RegistryPath);

	KdPrintEx((0, 0, "[LyVa][%s] \n", __FUNCTION__));

	DriverObject->DriverUnload = (PDRIVER_UNLOAD)DriverUnload;
	DriverObject->MajorFunction[IRP_MJ_CREATE] = DrvCreateClose;
	DriverObject->MajorFunction[IRP_MJ_CLOSE] = DrvCreateClose;
	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IoctlDispatcher;

	// �����豸
	if (!NT_SUCCESS(CreateDevice(DriverObject)))
	{
		KdPrintEx((0, 0, "[LyVa][%s] CreateDevice faild \n", __FUNCTION__));
	}

	// ���� DriverObject
	g_DriverObject = DriverObject;

	VM_END();

	return STATUS_SUCCESS;
}