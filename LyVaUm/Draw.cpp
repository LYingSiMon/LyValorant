#include "Draw.h"
#include "Common.h"
#include "Cheat.h"
#include "spdlog/spdlog.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"

#include <d3d11.h>

static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
static HWND g_ImguiHwnd;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (wParam == SIZE_MINIMIZED)
			return 0;
		g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
		g_ResizeHeight = (UINT)HIWORD(lParam);
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

void CleanupRenderTarget()
{
	if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	if (!pBackBuffer)
		return;
	g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
	pBackBuffer->Release();
}

bool CreateDeviceD3D(HWND hWnd)
{
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	UINT createDeviceFlags = 0;
	D3D_FEATURE_LEVEL featureLevel;
	const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT res = D3D11CreateDeviceAndSwapChain(
		nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 
		createDeviceFlags, featureLevelArray, 
		2, D3D11_SDK_VERSION, &sd, 
		&g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res == DXGI_ERROR_UNSUPPORTED) 
		res = D3D11CreateDeviceAndSwapChain(
			nullptr, D3D_DRIVER_TYPE_WARP, nullptr,
			createDeviceFlags, featureLevelArray,
			2, D3D11_SDK_VERSION, &sd, 
			&g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
	if (res != S_OK)
		return false;

	CreateRenderTarget();
	return true;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();
	if (g_pSwapChain) { g_pSwapChain->Release(); g_pSwapChain = nullptr; }
	if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = nullptr; }
}

DWORD WINAPI Thread_MonitorGameWnd(PVOID pParam)
{
	while (1)
	{
		// 获取游戏窗口位置
		::GetWindowRect(g_GameInfo.Hwnd, &g_GameInfo.GameWndRect);

		// 移动 imgui 窗口
		MoveWindow(
			g_ImguiHwnd,
			g_GameInfo.GameWndRect.left + 1, g_GameInfo.GameWndRect.top + 1,
			g_GameInfo.GameWndRect.right - g_GameInfo.GameWndRect.left - 2, g_GameInfo.GameWndRect.bottom - g_GameInfo.GameWndRect.top - 2,
			TRUE);

		Sleep(100);
	}
}

BOOL StartImguiDraw()
{
	//VM_BEGIN2();

	// 注册窗口类、创建窗口
	WNDCLASSEX wc = {
		sizeof(wc),
		CS_CLASSDC,
		WndProc,
		0L, 0L,
		GetModuleHandle(nullptr),
		nullptr, nullptr, nullptr, nullptr,
		"lyva_imgui_class",
		nullptr };
	if (0 == ::RegisterClassEx(&wc))
	{
		spdlog::error("RegisterClassEx error:{}", GetLastError());
		return FALSE;
	}
	g_ImguiHwnd = ::CreateWindowEx(
		WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_LAYERED,
		wc.lpszClassName,
		"",
		WS_POPUP,
		0, 0, 1280, 768,
		nullptr, nullptr,
		wc.hInstance,
		nullptr);
	if (!g_ImguiHwnd)
	{
		spdlog::error("CreateWindowEx error:{}", GetLastError());
		return FALSE;
	}

	// 指定颜色透明
	if (0 == ::SetLayeredWindowAttributes(g_ImguiHwnd, RGB(0, 100, 200), NULL, LWA_COLORKEY))
	{
		spdlog::error("SetLayeredWindowAttributes error:{}", GetLastError());
		return FALSE;
	}

	// 反截屏
	if (!SetWindowDisplayAffinity(g_ImguiHwnd, WDA_EXCLUDEFROMCAPTURE))
	{
		return 0;
	}

	// 绘制窗口跟随游戏窗口移动
	CreateThread(0, 0, Thread_MonitorGameWnd, 0, 0, 0);

	// 创建 d3d 设备
	if (!CreateDeviceD3D(g_ImguiHwnd))
	{
		spdlog::error("CreateDeviceD3D error");
		CleanupDeviceD3D();
		::UnregisterClass(wc.lpszClassName, wc.hInstance);
		return FALSE;
	}

	// 显示窗口
	::ShowWindow(g_ImguiHwnd, SW_SHOWDEFAULT);
	::UpdateWindow(g_ImguiHwnd);

	// 初始化 imgui
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// 设置 imgui 主题风格
	ImGui::StyleColorsDark();

	// 设置 dx 版本
	ImGui_ImplWin32_Init(g_ImguiHwnd);
	ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

	// 消息循环
	bool done = false;
	ImVec4 clear_color = ImGui::ColorConvertU32ToFloat4(IM_COL32(0, 100, 200, 255));
	while (!done)
	{
		// 消息循环
		MSG msg;
		while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;

		// 处理窗口大小变化
		if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
		{
			CleanupRenderTarget();
			g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
			g_ResizeWidth = g_ResizeHeight = 0;
			CreateRenderTarget();
		}

		// 启动 imgui
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// 构造游戏透视窗口
		bool perspective_window = true;
		if (perspective_window)
		{
			static bool use_work_area = true;
			static ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;

			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(use_work_area ? viewport->WorkPos : viewport->Pos);
			ImGui::SetNextWindowSize(use_work_area ? viewport->WorkSize : viewport->Size);

			ImGui::Begin("Perspective Window", &perspective_window, flags);

			// 绘制
			ImDrawList* draw_list = ImGui::GetWindowDrawList();
			ImVec2 LineP1 = ImVec2((FLOAT)(g_GameInfo.GameWndRect.right - g_GameInfo.GameWndRect.left) / 2, 100);
			ImVec2 LineP2 = ImVec2(0, 0);
			for (UINT i = 0; i < 10; ++i)
			{
				if (g_PlayerInfo[i].Health == 0 ||
					g_PlayerInfo[i].Self == 1 ||
					g_PlayerInfo[i].X == 0 || 
					g_PlayerInfo[i].Y == 0 || 
					g_PlayerInfo[i].Z == 0)
					continue;

				FLOAT x1 = g_PlayerInfo[i].Screen[0] - (g_PlayerInfo[i].Screen[2] - g_PlayerInfo[i].Screen[1]) / 4;
				FLOAT x2 = g_PlayerInfo[i].Screen[0] + (g_PlayerInfo[i].Screen[2] - g_PlayerInfo[i].Screen[1]) / 4;

				if (g_PlayerInfo[i].Camp == g_GameInfo.SelfCamp)
				{
					draw_list->AddRect(
						ImVec2(x1, g_PlayerInfo[i].Screen[1]),
						ImVec2(x2, g_PlayerInfo[i].Screen[2]),
						IM_COL32(0, 255, 0, 255),
						2.0);

					LineP2 = ImVec2(g_PlayerInfo[i].Screen[0], g_PlayerInfo[i].Screen[1]);
					draw_list->AddLine(LineP1, LineP2, IM_COL32(0, 255, 0, 255), 0.5);
				}
				else
				{
					draw_list->AddRect(
						ImVec2(x1, g_PlayerInfo[i].Screen[1]),
						ImVec2(x2, g_PlayerInfo[i].Screen[2]),
						IM_COL32(255, 0, 0, 255),
						2.0);

					LineP2 = ImVec2(g_PlayerInfo[i].Screen[0], g_PlayerInfo[i].Screen[1]);
					draw_list->AddLine(LineP1, LineP2, IM_COL32(255, 0, 0, 255) , 0.5);
				}
			}

			ImGui::End();
		}

		// 显示
		ImGui::Render();
		const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
		g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
		g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		g_pSwapChain->Present(0, 0);
	}

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	::DestroyWindow(g_ImguiHwnd);
	::UnregisterClass(wc.lpszClassName, wc.hInstance);

	return TRUE;

	//VM_END();
}