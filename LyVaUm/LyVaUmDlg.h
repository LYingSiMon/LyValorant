
// LyVaUmDlg.h: 头文件
//

#pragma once
// CLyVaUmDlg 对话框
class CLyVaUmDlg : public CDialogEx
{
// 构造
public:
	CLyVaUmDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_LyVaUm_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:	
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_ListCtrl_Players;
	afx_msg void OnBnClickedButtonStart();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void DisPlayPlayersInfo();
	afx_msg void MonitorPlayersInfo();
	CListCtrl m_ListCtrl_Matrix;
	afx_msg void OnBnClickedButtonStop();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
};
