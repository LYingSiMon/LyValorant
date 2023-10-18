#include "Cheat.h"
#include "Common.h"

#include "spdlog/spdlog.h"

#include <cmath>

GameInfo g_GameInfo;
PlayerInfo g_PlayerInfo[0x10] = {0};
HANDLE g_hDevice;

ULONG_PTR GetPlayerInfoBase(ULONG_PTR SignatureAddr)
{
	SignatureAddr -= 46;

	ULONG32 Offset = 0;
#if 0
	// 普通方式读内存（弃用）
	if (!ReadProcessMemory(g_GameInfo.HProc, (PVOID)(SignatureAddr + 3), &Offset, sizeof(ULONG32), 0))
#endif
	if (!DrvReadProcMemory({ g_GameInfo.Pid,(PVOID)(SignatureAddr + 3),sizeof(ULONG32) }, &Offset))
	{
		return 0;
	}

	return (SignatureAddr + Offset + 7) + 0x28;
}

BOOL CheatInit()
{
	// 获取驱动设备句柄
	g_hDevice = CreateFile("\\\\.\\LyVa",
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
	if (g_hDevice == INVALID_HANDLE_VALUE)
	{
		spdlog::error("CreateFile error {}", GetLastError());
		return FALSE;
	}
	spdlog::info("g_hDevice {}", g_hDevice);
	
	// 获取窗口句柄
	g_GameInfo.Hwnd = FindWindow("UnrealWindow", "VALORANT  ");
	if (!g_GameInfo.Hwnd)
	{
		spdlog::error("FindWindow error {}",GetLastError());
		return FALSE;
	}
	spdlog::info("g_GameInfo.Hwnd {0:x}", (ULONG_PTR)g_GameInfo.Hwnd);
		
	// 获取 Pid
	GetWindowThreadProcessId(g_GameInfo.Hwnd, &g_GameInfo.Pid);
	if (!g_GameInfo.Pid)
	{
		spdlog::error("GetWindowThreadProcessId error {}", GetLastError());
		return FALSE;
	}
	spdlog::info("g_GameInfo.Pid {}", g_GameInfo.Pid);

	// 获取进程句柄
	g_GameInfo.HProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, g_GameInfo.Pid);
	if (!g_GameInfo.HProc)
	{
		spdlog::error("OpenProcess error {}",GetLastError());
		return FALSE;
	}
	spdlog::info("g_GameInfo.HProc {0:x}", (ULONG_PTR)g_GameInfo.HProc);

	// 获取模块基地址
#if 0
	// 句柄降权时无法使用这个方法（已弃用）
	g_GameInfo.Base_exe = GetModuleBaseByPid(g_GameInfo.Pid, "VALORANT-Win64-Shipping.exe", g_GameInfo.Size_exe);
#endif
	DrvGetModuleBase({g_GameInfo.Pid, L"VALORANT-Win64-Shipping.exe" });
	if (!g_GameInfo.Base_exe)
	{
		spdlog::error("GetModuleBaseByPid error {}", GetLastError());
		return FALSE;
	}
	else
		spdlog::info("g_GameInfo.Base_exe {0:x}", g_GameInfo.Base_exe);

	// 查找基地址特征码
	BYTE Code[] = "\x40\x53\x48\x83\xec\x20\x65\x48\x8b\x04\x25\x58\x00\x00\x00\x8b\x0d\x00\x00\x00\x00\xba\x68\x01\x00\x00\x48\x8b\x0c\xc8\x8b\x04\x0a\x39\x05\x00\x00\x00\x00\x7f\x00\x48\x8d\x05\x00\x00\x00\x00\x48\x83\xc4\x20\x5b\xc3\x48\x8d\x0d\x00\x00\x00\x00\xe8\x00\x00\x00\x00\x83\x3d\x00\x00\x00\x00\xff\x75\x00\x48\x8d\x1d\x00\x00\x00\x00\x48\x89\x7c\x24\x30";
	BYTE Rule[] = "xxxxxxxxxxxxxxxxx????xxxxxxxxxxxxxx????x?xxx????xxxxxxxxx????x????xx????xx?xxx????xxxxx";
	ULONG_PTR SignPlayerInfo = ScanSignature(g_GameInfo.HProc, g_GameInfo.Base_exe, g_GameInfo.Size_exe, Code, Rule);
	g_GameInfo.Base_PlayerInfo = GetPlayerInfoBase(SignPlayerInfo);
	spdlog::info("Base_PlayerInfo:{0:x}", g_GameInfo.Base_PlayerInfo);

	return TRUE;
}

FLOAT CalculateDistance(FLOAT x1, FLOAT y1, FLOAT z1, FLOAT x2, FLOAT y2, FLOAT z2)
{
	return (FLOAT)sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2) + pow(z2 - z1, 2));
}

BOOL GameXYZToScreen(FLOAT mcax, FLOAT mcay, FLOAT mcaz, DWORD Index)
{
	// 获取游戏窗口矩形
	::GetWindowRect(g_GameInfo.Hwnd, &g_GameInfo.GameWndRect);
	FLOAT WndWidth = (FLOAT)g_GameInfo.GameWndRect.right - (FLOAT)g_GameInfo.GameWndRect.left;
	FLOAT WndHeight = (FLOAT)g_GameInfo.GameWndRect.bottom - (FLOAT)g_GameInfo.GameWndRect.top;
	
	// 计算玩家距离
	FLOAT Distance = CalculateDistance(mcax, mcay, mcaz, g_GameInfo.SelfXYZ[0], g_GameInfo.SelfXYZ[1], g_GameInfo.SelfXYZ[2]);
	
	// 计算屏幕坐标
	FLOAT x, y, y1, y2, w;
	x = g_GameInfo.Matrix[0][0] * mcax + g_GameInfo.Matrix[0][1] * mcay + g_GameInfo.Matrix[0][2] * mcaz + g_GameInfo.Matrix[0][3];
	y = g_GameInfo.Matrix[1][0] * mcax + g_GameInfo.Matrix[1][1] * mcay + g_GameInfo.Matrix[1][2] * mcaz + g_GameInfo.Matrix[1][3];
	y1 = g_GameInfo.Matrix[1][0] * mcax + g_GameInfo.Matrix[1][1] * mcay + g_GameInfo.Matrix[1][2] * (mcaz + 100) + g_GameInfo.Matrix[1][3];
	y2 = g_GameInfo.Matrix[1][0] * mcax + g_GameInfo.Matrix[1][1] * mcay + g_GameInfo.Matrix[1][2] * (mcaz - 100) + g_GameInfo.Matrix[1][3];
	w = g_GameInfo.Matrix[3][0] * mcax + g_GameInfo.Matrix[3][1] * mcay + g_GameInfo.Matrix[3][2] * mcaz + g_GameInfo.Matrix[3][3];
	if (w < 0.01f)
		return FALSE;

	x = x / w;
	y = y / w;
	y1 = y1 / w;
	y2 = y2 / w;

	/* <bug>
	* 某些情况下，定位到屏幕中心点
	*/
	if(abs((WndWidth / 2) - ((WndWidth / 2 * x) + (x + WndWidth / 2))) < 20 &&
		abs((WndHeight / 2) - (-(WndHeight / 2 * y) + (y + WndHeight / 2))) < 20)
		return FALSE;
	//spdlog::info("{},{} | {},{}", (WndWidth / 2 * x) + (x + WndWidth / 2), WndWidth / 2 ,-(WndHeight / 2 * y) + (y + WndHeight / 2), WndHeight / 2);

	// 保存屏幕坐标
	g_PlayerInfo[Index].Screen[0] = (WndWidth / 2 * x) + (x + WndWidth / 2);
	g_PlayerInfo[Index].Screen[1] = -(WndHeight / 2 * y1) + (y1 + WndHeight / 2);
	g_PlayerInfo[Index].Screen[2] = -(WndHeight / 2 * y2) + (y2 + WndHeight / 2);

	return TRUE;
}

VOID MatrixTransit()
{
	FLOAT tmp[4][4] = { 0 };

	for (UINT i = 0; i < 4; ++i)
	{
		for (UINT j = 0; j < 4; ++j)
		{
			tmp[i][j] = g_GameInfo.Matrix[i][j];
		}
	}

	for (UINT i = 0; i < 4; ++i)
	{
		for (UINT j = 0; j < 4; ++j)
		{
			g_GameInfo.Matrix[i][j] = tmp[j][i];
		}
	}

	g_GameInfo.Matrix[0][2] = 0;
}