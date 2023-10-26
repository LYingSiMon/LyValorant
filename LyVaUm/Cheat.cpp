#include "Cheat.h"
#include "Common.h"

#include "spdlog/spdlog.h"

#include <cmath>

GameInfo g_GameInfo;
PlayerInfo g_PlayerInfo[0x10] = {0};
HANDLE g_hDevice;
DrvReceiver g_DrvReceiver = { 0 };

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

#if 0
	// 把 R3 变量传给驱动
	spdlog::info("g_DrvReceiver address {0:x}", (ULONG_PTR)&g_DrvReceiver);
	R3Var VarInfo = { 0 };
	VarInfo.Pid_Game = g_GameInfo.Pid;
	VarInfo.Pid_LyVaUm = GetCurrentProcessId();
	VarInfo.Base_Exe = g_GameInfo.Base_exe;
	VarInfo.Base_PlayerInfo = g_GameInfo.Base_PlayerInfo;
	VarInfo.Base_DrvReceiver = (ULONG_PTR)&g_DrvReceiver;
	DrvSendR3Var(VarInfo);
#endif

	return TRUE;
}

FLOAT CalculateDistance(FLOAT x1, FLOAT y1, FLOAT z1, FLOAT x2, FLOAT y2, FLOAT z2)
{
	return (FLOAT)sqrt(pow(x2 - x1, 2) + pow(y2 - y1, 2) + pow(z2 - z1, 2));
}

BOOL GameXYZToScreen(FLOAT mcax, FLOAT mcay, FLOAT mcaz, DWORD Index)
{
	// 获取游戏窗口矩形
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

DWORD Thread_MonitorPlayersInfo(PVOID pParam)
{
	while (1)
	{
#if 1
		// 方案1：发送 ioctrl 请求数据
		ULONG_PTR Offset_Matrix[] = { 0xf6db50 };
		ReadProcMemByOffset(g_GameInfo.HProc, Offset_Matrix, 0, &g_GameInfo.Matrix, sizeof(g_GameInfo.Matrix));

		MatrixTransit();

		for (UINT i = 0; i < 10; ++i)
		{
			ULONG_PTR Offset_Self[] = { g_GameInfo.Base_PlayerInfo, 0x280, 0x38, 0x50,i * 0x8, 0x218, 0x790, 0xf8 };
			ULONG_PTR Offset_Camp[] = { g_GameInfo.Base_PlayerInfo, 0x280, 0x38, 0x50,i * 0x8, 0x218, 0x628, 0xf8 };
			ULONG_PTR Offset_Health[] = { g_GameInfo.Base_PlayerInfo, 0x280, 0x38, 0x50,i * 0x8, 0x228, 0xa00, 0x1b0 };
			ULONG_PTR Offset_X[] = { g_GameInfo.Base_PlayerInfo, 0x280, 0x38, 0x50,i * 0x8, 0x228, 0x440, 0x148 };
			ULONG_PTR Offset_Y[] = { g_GameInfo.Base_PlayerInfo, 0x280, 0x38, 0x50,i * 0x8, 0x228, 0x440, 0x14c };
			ULONG_PTR Offset_Z[] = { g_GameInfo.Base_PlayerInfo, 0x280, 0x38, 0x50,i * 0x8, 0x228, 0x440, 0x150 };

			ReadProcMemByOffset(g_GameInfo.HProc, Offset_Self, 7, &g_PlayerInfo[i].Self, sizeof(BYTE));
			ReadProcMemByOffset(g_GameInfo.HProc, Offset_Camp, 7, &g_PlayerInfo[i].Camp, sizeof(BYTE));
			ReadProcMemByOffset(g_GameInfo.HProc, Offset_Health, 7, &g_PlayerInfo[i].Health, sizeof(FLOAT));
			ReadProcMemByOffset(g_GameInfo.HProc, Offset_X, 7, &g_PlayerInfo[i].X, sizeof(FLOAT));
			ReadProcMemByOffset(g_GameInfo.HProc, Offset_Y, 7, &g_PlayerInfo[i].Y, sizeof(FLOAT));
			ReadProcMemByOffset(g_GameInfo.HProc, Offset_Z, 7, &g_PlayerInfo[i].Z, sizeof(FLOAT));

			GameXYZToScreen(g_PlayerInfo[i].X, g_PlayerInfo[i].Y, g_PlayerInfo[i].Z, i);

			if (g_PlayerInfo[i].Self == 1)
			{
				g_GameInfo.SelfCamp = g_PlayerInfo[i].Camp;
				g_GameInfo.SelfXYZ[0] = g_PlayerInfo[i].X;
				g_GameInfo.SelfXYZ[1] = g_PlayerInfo[i].Y;
				g_GameInfo.SelfXYZ[2] = g_PlayerInfo[i].Z;
			}
		}
#endif

#if 0 
		// 方案2：驱动主动赋值
		for (UINT i = 0; i < 4; ++i)
		{
			for (UINT j = 0; j < 4; ++j)
			{
				g_GameInfo.Matrix[i][j] = g_DrvReceiver.Matrix[i][j];
			}
		}
		MatrixTransit();

		for (UINT i = 0; i < 10; ++i)
		{
			g_PlayerInfo[i].Self = g_DrvReceiver.Players[i].Self;
			g_PlayerInfo[i].Camp = g_DrvReceiver.Players[i].Camp;
			g_PlayerInfo[i].Health = g_DrvReceiver.Players[i].Health;
			g_PlayerInfo[i].X = g_DrvReceiver.Players[i].X;
			g_PlayerInfo[i].Y = g_DrvReceiver.Players[i].Y;
			g_PlayerInfo[i].Z = g_DrvReceiver.Players[i].Z;
			g_PlayerInfo[i].Self = g_DrvReceiver.Players[i].Self;
			g_PlayerInfo[i].Self = g_DrvReceiver.Players[i].Self;

			GameXYZToScreen(g_PlayerInfo[i].X, g_PlayerInfo[i].Y, g_PlayerInfo[i].Z, i);

			if (g_PlayerInfo[i].Self == 1)
			{
				g_GameInfo.SelfCamp = g_PlayerInfo[i].Camp;
				g_GameInfo.SelfXYZ[0] = g_PlayerInfo[i].X;
				g_GameInfo.SelfXYZ[1] = g_PlayerInfo[i].Y;
				g_GameInfo.SelfXYZ[2] = g_PlayerInfo[i].Z;
			}
		}
#endif

		Sleep(10);
	}

	return 0;
}