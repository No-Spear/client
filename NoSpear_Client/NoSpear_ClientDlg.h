﻿#pragma once
// CNoSpearClientDlg 대화 상자
class LIVEPROTECT;
class CNoSpearClientDlg : public CDialogEx{
// 생성입니다.
public:
	CNoSpearClientDlg(CWnd* pParent = nullptr);	// 표준 생성자입니다.

// 대화 상자 데이터입니다.
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_NOSPEAR_CLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 지원입니다.


// 구현입니다.
protected:
	HICON m_hIcon;

	// 생성된 메시지 맵 함수
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
private:
	CString filename;
	CString filepath;
	bool Has_ADS(CString filepath);
	void PrintFolder(CString folderpath);

public:
	afx_msg void OnBnClickedselectfile();
	afx_msg void OnBnClickeduploadfile();
	afx_msg void OnBnClickedactivelive();
	afx_msg void OnBnClickedinactivelive();
	afx_msg void OnBnClickedButton1();

	afx_msg void OnBnClickedButton2();
};
