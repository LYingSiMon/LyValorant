#include "Common.h"
#include "Cheat.h"
#include "spdlog/spdlog.h"
#include "openssl/md5.h"

#include <tlhelp32.h>
#include <psapi.h>
#include <comdef.h>
#include <Wbemidl.h>

#pragma comment(lib, "wbemuuid.lib")

BOOL DrvReadProcMemory(CTLINFO_READ_MEMORY ReadInfo, PVOID pBuffer)
{
	DWORD RetSize = 0;

	if (!DeviceIoControl(
		g_hDevice,
		CTL_CODE(FILE_DEVICE_UNKNOWN, CTL_READ_MEMORY, METHOD_BUFFERED, FILE_ANY_ACCESS),
		&ReadInfo, sizeof(CTLINFO_READ_MEMORY),
		pBuffer, ReadInfo.Size,
		&RetSize, 0))
	{
		spdlog::error("DeviceIoControl error {}", GetLastError());
		return FALSE;
	}

	return TRUE;
}

BOOL DrvGetModuleBase(CTLINFO_GET_MODULEBASE ReadInfo)
{
	DWORD RetSize = 0;
	CTLINFO_GET_MODULEBASE_RET Buffer = { 0 };

	if (!DeviceIoControl(
		g_hDevice,
		CTL_CODE(FILE_DEVICE_UNKNOWN, CTL_GET_MODULEBASE, METHOD_BUFFERED, FILE_ANY_ACCESS),
		&ReadInfo, sizeof(CTLINFO_GET_MODULEBASE),
		&Buffer, sizeof(CTLINFO_GET_MODULEBASE_RET),
		&RetSize, 0))
	{
		spdlog::error("DeviceIoControl error {}", GetLastError());
		return FALSE;
	}

	g_GameInfo.Base_exe = Buffer.Base;
	g_GameInfo.Size_exe = Buffer.Size;

	return TRUE;
}

ULONG_PTR GetModuleBaseByPid(DWORD Pid, PCHAR ModuleName, DWORD& Size)
{
	HANDLE  hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, Pid);
	MODULEENTRY32 Module = { sizeof(MODULEENTRY32) };

	if (!Module32First(hModuleSnap, &Module))
	{
		return 0;
	}
	do
	{
		if (_stricmp(ModuleName, Module.szModule) == 0)
		{
			Size = Module.modBaseSize;
			return (ULONG_PTR)Module.modBaseAddr;
		}
	} while (Module32Next(hModuleSnap, &Module));

	return 0;
}

BOOL ReadProcMemByOffset(HANDLE hProc, ULONG_PTR Offset[], DWORD OffsetCount, PVOID Result, DWORD Size)
{
	BYTE Buffer[256] = { 0 };
	ULONG_PTR Temp = 0;

	for (UINT i = 0; i <= OffsetCount; ++i)
	{
		Temp = *(PULONG_PTR)Buffer;
		Temp += Offset[i];

		memset(Buffer, 0, 256);
		if (i == OffsetCount)
		{
#if 0
			// 普通方式读内存（弃用）
			if (!ReadProcessMemory(hProc, (PVOID)Temp, Buffer, Size, 0))
#endif
			if(!DrvReadProcMemory({ g_GameInfo.Pid, (PVOID)Temp, Size }, &Buffer))
			{
				return FALSE;
			}
		}
		else
		{
#if 0
			// 普通方式读内存（弃用）
			if (!ReadProcessMemory(hProc, (PVOID)Temp, Buffer, sizeof(ULONG_PTR), 0))
#endif
			if (!DrvReadProcMemory({ g_GameInfo.Pid, (PVOID)Temp, sizeof(ULONG_PTR) }, &Buffer))
			{
				return FALSE;
			}
		}
	}

	memcpy(Result, Buffer, Size);

	return TRUE;
}

ULONG_PTR ScanSignature(HANDLE hProc, ULONG_PTR StartAddress, DWORD Size, BYTE Code[], BYTE Rule[])
{
	spdlog::info("hProc:{0:x} StartAddress:{1:x} Size:{2:x}", hProc,StartAddress, Size);

	DWORD Len = 0;
	while (Rule[Len] != '\0')
	{
		Len++;
	}

	PBYTE pBuffer = (PBYTE)malloc(Size);
	if (!pBuffer)
		return 0;

#if 0
	// 普通方式读内存（弃用）
	if (!ReadProcessMemory(hProc, (PVOID)StartAddress, pBuffer, Size, 0))
#endif
	if (!DrvReadProcMemory({ g_GameInfo.Pid, (PVOID)StartAddress, Size }, pBuffer))
	{
		return 0;
	}
	spdlog::info("pBuffer:{0:x},{1:x}", pBuffer[0], pBuffer[1]);

	UINT NumOfMatch = 0;
	for (UINT i = 0; i < Size; ++i)
	{
#if 0
		// 测试特征码查找
		if (StartAddress + i == 0x1445851A0)
		{
			spdlog::info("0x1445851A0");
			for (UINT j = 0; j < 100; ++j)
			{
				spdlog::info("{0:x}|{1:x}", StartAddress + i + j, pBuffer[i + j]);
			}
		}
#endif

		if (NumOfMatch == Len)
		{
#if 0
			// 测试特征码查找
			spdlog::info("Address:{0:x}", StartAddress + i - Len);
			NumOfMatch = 0;
			continue;
#endif
			return StartAddress + i;
		}

		if (Rule[NumOfMatch] == '?')
		{
			NumOfMatch++;
			continue;
		}
		else if (Rule[NumOfMatch] == 'x')
		{
			if (pBuffer[i] == Code[NumOfMatch])
			{
				NumOfMatch++;
				continue;
			}
			else
			{
				NumOfMatch = 0;
				continue;
			}
		}
	}
	
	spdlog::info("Address not found");
	return 0;
}

BOOL InstallDvr(PCHAR DrvPath, PCHAR ServiceName)
{
    BOOL Status = FALSE;
    SC_HANDLE SchSCManager = NULL;
    SC_HANDLE SchService = NULL;

    // 打开服务控制管理器数据库
   SchSCManager = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS);
    if (SchSCManager == NULL) 
    {
        goto end;
    }

    // 创建服务对象，添加至服务控制管理器数据库
    SchService = CreateService(
        SchSCManager,
        ServiceName,
        ServiceName,
        SERVICE_ALL_ACCESS,
        SERVICE_KERNEL_DRIVER,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_IGNORE,
        DrvPath,
        NULL,NULL,NULL,NULL, NULL);
    if (SchService == NULL &&
        GetLastError() != ERROR_SERVICE_EXISTS)
    {
        goto end;
    }

    Status = TRUE;
end:
    if(SchService)
        CloseServiceHandle(SchService);
    if(SchSCManager)
        CloseServiceHandle(SchSCManager);
    return Status;
}

BOOL StartDvr(PCHAR ServiceName)
{
    BOOL Status = FALSE;
    SC_HANDLE SchSCManager = NULL;
    SC_HANDLE hs = NULL;

    // 打开服务控制管理器数据库
    SchSCManager = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS );
    if (SchSCManager == NULL) 
    {
        goto end;
    }

    // 打开服务
    hs = OpenService(
        SchSCManager,
        ServiceName,
        SERVICE_ALL_ACCESS);
    if (hs == NULL) 
    {
        goto end;
    }
    if (StartService(hs, 0, 0) == 0 &&
        GetLastError() != ERROR_SERVICE_ALREADY_RUNNING)
    {
        goto end;
    }

    Status = TRUE;
end:
    if(hs)
        CloseServiceHandle(hs);
    if(SchSCManager)
        CloseServiceHandle(SchSCManager);
    return Status;
}

BOOL StopDvr(PCHAR ServiceName)
{
    BOOL Status = FALSE;
    SC_HANDLE SchSCManager = NULL;
    SC_HANDLE hs = NULL;
    SERVICE_STATUS ServiceStatus;
    INT TimeOut = 0;

    // 打开服务控制管理器数据库
    SchSCManager = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS);
    if (SchSCManager == NULL)
    {
        goto end;
    }

    // 打开服务
    hs = OpenService(
        SchSCManager,
        ServiceName,
        SERVICE_ALL_ACCESS);
    if (hs == NULL)
    {
        goto end;
    }

    // 如果服务正在运行
    if (QueryServiceStatus(hs, &ServiceStatus) == 0) 
    {
        Status = TRUE;
        goto end;
    }
    if (ServiceStatus.dwCurrentState != SERVICE_STOPPED &&
        ServiceStatus.dwCurrentState != SERVICE_STOP_PENDING ) 
    {
        // 发送关闭服务请求
        if (ControlService(hs,SERVICE_CONTROL_STOP,&ServiceStatus) == 0) 
        {
            goto end;
        }

        // 判断超时
        while (ServiceStatus.dwCurrentState != SERVICE_STOPPED)
        {
            TimeOut++;
            QueryServiceStatus(hs, &ServiceStatus);
            Sleep(100);
        }
        if (TimeOut > 50)
        {
            goto end;
        }
    }

    Status = TRUE;
end:
    if(hs)
        CloseServiceHandle(hs);
    if(SchSCManager)
        CloseServiceHandle(SchSCManager);
    return Status;
}

BOOL UninstallDvr(PCHAR ServiceName)
{
    BOOL Status = FALSE;
    SC_HANDLE SchSCManager = NULL;
    SC_HANDLE hs = NULL;

    // 打开服务控制管理器数据库
    SchSCManager = OpenSCManager(
        NULL,
        NULL,
        SC_MANAGER_ALL_ACCESS);
    if (SchSCManager == NULL)
    {
        goto end;
    }

    // 打开服务
    hs = OpenService(
        SchSCManager,
        ServiceName,
        SERVICE_ALL_ACCESS);
    if (hs == NULL)
    {
        goto end;
    }

    // 删除服务
    if (DeleteService(hs) == 0)
    {
        goto end;
    }

    Status = TRUE;
end:
    if(hs)
        CloseServiceHandle(hs);
    if(SchSCManager)
        CloseServiceHandle(SchSCManager);
    return Status;
}

BOOL GetDiskSerial(PCHAR pBuf, size_t bufSize)
{
    HRESULT hres;

    // Step 1: --------------------------------------------------
    // Initialize COM. ------------------------------------------

    hres = CoInitializeEx(0, COINIT_MULTITHREADED);
    if (FAILED(hres))
    {
        return FALSE;                  // Program has failed.
    }

    // Step 2: --------------------------------------------------
    // Set general COM security levels --------------------------
    // Note: If you are using Windows 2000, you need to specify -
    // the default authentication credentials for a user by using
    // a SOLE_AUTHENTICATION_LIST structure in the pAuthList ----
    // parameter of CoInitializeSecurity ------------------------

    hres = CoInitializeSecurity(
        NULL,
        -1,                          // COM authentication
        NULL,                        // Authentication services
        NULL,                        // Reserved
        RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
        RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
        NULL,                        // Authentication info
        EOAC_NONE,                   // Additional capabilities 
        NULL                         // Reserved
    );

    if (FAILED(hres))
    {
        CoUninitialize();
        return FALSE;                    // Program has failed.
    }

    // Step 3: ---------------------------------------------------
    // Obtain the initial locator to WMI -------------------------

    IWbemLocator* pLoc = NULL;

    hres = CoCreateInstance(
        CLSID_WbemLocator,
        0,
        CLSCTX_INPROC_SERVER,
        IID_IWbemLocator, (LPVOID*)&pLoc);

    if (FAILED(hres))
    {
        CoUninitialize();
        return FALSE;                 // Program has failed.
    }

    // Step 4: -----------------------------------------------------
    // Connect to WMI through the IWbemLocator::ConnectServer method

    IWbemServices* pSvc = NULL;

    // Connect to the root\cimv2 namespace with
    // the current user and obtain pointer pSvc
    // to make IWbemServices calls.
    hres = pLoc->ConnectServer(
        _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
        NULL,                    // User name. NULL = current user
        NULL,                    // User password. NULL = current
        0,                       // Locale. NULL indicates current
        NULL,                    // Security flags.
        0,                       // Authority (e.g. Kerberos)
        0,                       // Context object 
        &pSvc                    // pointer to IWbemServices proxy
    );

    if (FAILED(hres))
    {
        pLoc->Release();
        CoUninitialize();
        return FALSE;                // Program has failed.
    }

    // Step 5: --------------------------------------------------
    // Set security levels on the proxy -------------------------

    hres = CoSetProxyBlanket(
        pSvc,                        // Indicates the proxy to set
        RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
        RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
        NULL,                        // Server principal name 
        RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
        RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
        NULL,                        // client identity
        EOAC_NONE                    // proxy capabilities 
    );

    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return FALSE;               // Program has failed.
    }

    // Step 6: --------------------------------------------------
    // Use the IWbemServices pointer to make requests of WMI ----

    // For example, get the name of the operating system
    IEnumWbemClassObject* pEnumerator = NULL;
    hres = pSvc->ExecQuery(
        bstr_t("WQL"),
        bstr_t("SELECT * FROM Win32_PhysicalMedia"),
        WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
        NULL,
        &pEnumerator);

    if (FAILED(hres))
    {
        pSvc->Release();
        pLoc->Release();
        CoUninitialize();
        return FALSE;               // Program has failed.
    }

    // Step 7: -------------------------------------------------
    // Get the data from the query in step 6 -------------------

    IWbemClassObject* pclsObj = NULL;
    ULONG uReturn = 0;
    BOOL rVal = FALSE;
    while (pEnumerator)
    {
        HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
            &pclsObj, &uReturn);

        if (0 == uReturn)
        {
            break;
        }

        VARIANT vtProp = { 0 };

        // Get the value of the Name property
        hr = pclsObj->Get(L"SerialNumber", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr))
        {
            char* ps = _com_util::ConvertBSTRToString(vtProp.bstrVal);
            strcpy_s(pBuf, bufSize, ps);
            // HexStrToStr(pBuf, pBuf, bufSize);
            delete[] ps;
            rVal = TRUE;

            VariantClear(&vtProp);
            //break;
        }
        VariantClear(&vtProp);
    }

    // Cleanup
    // ========

    pSvc->Release();
    pLoc->Release();
    pEnumerator->Release();
    pclsObj->Release();
    CoUninitialize();
    return rVal;
}

std::string Md5Encode(std::string text)
{
    UCHAR result[MD5_DIGEST_LENGTH];
    std::string md5_hex;
    CHAR map[] = "0123456789abcdef";

    MD5((PUCHAR)text.c_str(), text.length(), result);
    for (size_t i = 0; i < MD5_DIGEST_LENGTH; ++i)
    {
        md5_hex += map[result[i] / 16];
        md5_hex += map[result[i] % 16];
    }

    return md5_hex;
}

std::string GetDiskSerialMD5()
{
    CHAR DiskSerial[MAX_PATH] = { 0 };
    GetDiskSerial(DiskSerial, MAX_PATH - 1);

    return Md5Encode(DiskSerial);
}

BOOL IsHwidOk()
{
    std::string HwidList[] = {
        "3e6f74c0a86ea0eec0228b113d6ff5da",         // 台式
        "0b7a6452feb7cac8ddcb95da871f59d3",         // 小笔记本
        "f982f790f4ece02e4168e2704000e6df",
        "d3cc6fe8a7b67abba082ae84b113a491",
        "d41d8cd98f00b204e9800998ecf8427e",
        "8575245dff7c89c2f5fee3922aea197e",
        "3d2d55b6ad4ace41e5f6cd94515c8035",
        "69b6711a0d60bc94a878d1d694d41830",
        "a09ba0b3fb378da7314a30ebbe1fd1ab",
    };
    std::string HwidMd5 = GetDiskSerialMD5();

    for (UINT i = 0; i < sizeof(HwidList) / sizeof(HwidList[0]); ++i)
    {
        if (HwidList[i] == HwidMd5)
        {
            return TRUE;
        }
    }

    return FALSE;
}