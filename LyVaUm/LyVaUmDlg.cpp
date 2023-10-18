
// LyVaUmDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "LyVaUm.h"
#include "LyVaUmDlg.h"
#include "afxdialogex.h"

#include "Common.h"
#include "Cheat.h"
#include "Draw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

enum TimerId {
	TimerId_MonitorPlayersInfo = 1,
	TimerId_DisPlayPlayersInfo,
};

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CLyVaUmDlg 对话框



CLyVaUmDlg::CLyVaUmDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_LyVaUm_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CLyVaUmDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_PLAYERS, m_ListCtrl_Players);
	DDX_Control(pDX, IDC_LIST_MATRIX, m_ListCtrl_Matrix);
}

BEGIN_MESSAGE_MAP(CLyVaUmDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_START, &CLyVaUmDlg::OnBnClickedButtonStart)
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CLyVaUmDlg::OnBnClickedButtonStop)
	ON_WM_CREATE()
END_MESSAGE_MAP()


// CLyVaUmDlg 消息处理程序

BOOL CLyVaUmDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	
	DWORD ListCtrl_Players_Style = m_ListCtrl_Players.GetExtendedStyle();
	ListCtrl_Players_Style |= LVS_EX_GRIDLINES;
	m_ListCtrl_Players.SetExtendedStyle(ListCtrl_Players_Style);
	m_ListCtrl_Players.InsertColumn(0, _T("Self"), LVCFMT_CENTER, 50);
	m_ListCtrl_Players.InsertColumn(1, _T("Name"), LVCFMT_CENTER, 50);
	m_ListCtrl_Players.InsertColumn(2, _T("Camp"), LVCFMT_CENTER, 50);
	m_ListCtrl_Players.InsertColumn(3, _T("Health"), LVCFMT_CENTER, 50);
	m_ListCtrl_Players.InsertColumn(4, _T("X"), LVCFMT_CENTER, 50);
	m_ListCtrl_Players.InsertColumn(5, _T("Y"), LVCFMT_CENTER, 50);
	m_ListCtrl_Players.InsertColumn(6, _T("Z"), LVCFMT_CENTER, 50);
	m_ListCtrl_Players.InsertColumn(7, _T("Screen"), LVCFMT_CENTER, 50);

	m_ListCtrl_Matrix.SetExtendedStyle(ListCtrl_Players_Style);
	m_ListCtrl_Matrix.InsertColumn(0, _T("index"), LVCFMT_CENTER, 50);
	m_ListCtrl_Matrix.InsertColumn(1, _T("value"), LVCFMT_CENTER, 50);

	// 非调试模式
	if (strstr(GetCommandLine(), "/debug") == 0)
	{
		// 隐藏窗口
		SetWindowPos(NULL, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), 0, 0, SWP_NOZORDER);
		ShowWindow(SW_HIDE);
		ModifyStyleEx(WS_EX_APPWINDOW, WS_EX_TOOLWINDOW);

		// 初始化作弊器
		if (CheatInit())
		{
			// 提示
			MessageBox("启动成功");

			// 设置定时器
			SetTimer(TimerId_MonitorPlayersInfo, 10, NULL);

			// 绘制
			StartImguiDraw();
		}
	}

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CLyVaUmDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CLyVaUmDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CLyVaUmDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CLyVaUmDlg::OnBnClickedButtonStart()
{
	// TODO: 在此添加控件通知处理程序代码

	// 初始化日志
	AllocConsole();

	// 初始化作弊器
	if (!CheatInit())
	{
		MessageBox("CheatInit error.");
		return;
	}

	// 设置定时器
	SetTimer(TimerId_MonitorPlayersInfo, 10, NULL);
	SetTimer(TimerId_DisPlayPlayersInfo, 100, NULL);

	// 绘制
	StartImguiDraw();
}

void CLyVaUmDlg::MonitorPlayersInfo()
{

#if 0
	// 这个指针有时候会指向一个无效地址
	ULONG_PTR Offset_Matrix[] = { g_GameInfo.Base_PlayerInfo, 0x8, 0x18, 0xb0, 0x78, 0xd0, 0xf30 };
	ReadProcMemByOffset(g_GameInfo.HProc, Offset_Matrix, 6, &g_GameInfo.Matrix, sizeof(g_GameInfo.Matrix));
#endif
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

}

void CLyVaUmDlg::DisPlayPlayersInfo()
{
	m_ListCtrl_Players.DeleteAllItems();
	m_ListCtrl_Matrix.DeleteAllItems();

	for (UINT i = 0; i < 4; ++i)
	{
		for (UINT j = 0; j < 4; ++j)
		{
			CString StrIndex;
			StrIndex.Format("%d", i * 4 + j);
			m_ListCtrl_Matrix.InsertItem(i * 4 + j, StrIndex);

			CString StrMatrix, StrMatrixIndex;
			StrMatrixIndex.Format("[%d][%d]", i, j);
			StrMatrix.Format("%f", g_GameInfo.Matrix[i][j]);
			m_ListCtrl_Matrix.SetItemText(i * 4 + j, 0, StrMatrixIndex);
			m_ListCtrl_Matrix.SetItemText(i * 4 + j, 1, StrMatrix);
		}
	}

	for (UINT i = 0; i < 10; ++i)
	{
		CString StrSelf, StrCamp, StrHealth, StrX, StrY, StrZ, StrScreen;
		StrSelf.Format("%d", g_PlayerInfo[i].Self);
		StrCamp.Format("%x", g_PlayerInfo[i].Camp);
		StrHealth.Format("%f", g_PlayerInfo[i].Health);
		StrX.Format("%f", g_PlayerInfo[i].X);
		StrY.Format("%f", g_PlayerInfo[i].Y);
		StrZ.Format("%f", g_PlayerInfo[i].Z);
		StrScreen.Format("%f,%f,%f", g_PlayerInfo[i].Screen[0], g_PlayerInfo[i].Screen[1], g_PlayerInfo[i].Screen[2]);

		m_ListCtrl_Players.InsertItem(i, "");
		m_ListCtrl_Players.SetItemText(i, 0, StrSelf);
		m_ListCtrl_Players.SetItemText(i, 2, StrCamp);
		m_ListCtrl_Players.SetItemText(i, 3, StrHealth);
		m_ListCtrl_Players.SetItemText(i, 4, StrX);
		m_ListCtrl_Players.SetItemText(i, 5, StrY);
		m_ListCtrl_Players.SetItemText(i, 6, StrZ);
		m_ListCtrl_Players.SetItemText(i, 7, StrScreen);
	}
}

void CLyVaUmDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值

	switch (nIDEvent)
	{
	case TimerId_MonitorPlayersInfo:
		MonitorPlayersInfo();
	case TimerId_DisPlayPlayersInfo:
		DisPlayPlayersInfo();
	}

	CDialogEx::OnTimer(nIDEvent);
}


void CLyVaUmDlg::OnBnClickedButtonStop()
{
	// TODO: 在此添加控件通知处理程序代码

	// 删除定时器
	KillTimer(TimerId_MonitorPlayersInfo);
	KillTimer(TimerId_DisPlayPlayersInfo);
}


int CLyVaUmDlg::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	VM_BEGIN3();

	if (CDialogEx::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码

	// 单实例
	HANDLE hMutex = CreateMutexW(nullptr, false, L"LyVa_Mutex");
	if (hMutex) 
	{
		if (ERROR_ALREADY_EXISTS == ::GetLastError())
		{
			MessageBox("程序已在运行，请不要重复打开");
			exit(0);
		}
	}

	// 免责声明
	MessageBox("软件仅供学习交流！完全免费！禁止拿去售卖！否则产生的一切后果请自行承担！");

	// 绑定机器码
	if (!IsHwidOk())
	{
		MessageBox("机器码错误");
		exit(0);
	}

	// 加载驱动
	CHAR Dir[MAX_PATH] = { 0 };
	if (!GetCurrentDirectory(MAX_PATH, Dir))
	{
		MessageBox("文件路径错误");
		exit(0);
	}
	strcat_s(Dir, "\\LyVaKm.sys");
	if (!InstallDvr(Dir, DRV_SERVICE_NAME))
	{
		MessageBox("驱动安装失败");
		exit(0);
	}
	if (!StartDvr(DRV_SERVICE_NAME))
	{
		MessageBox("服务启动失败");
		exit(0);
	}

	VM_END();

	return 0;
}
