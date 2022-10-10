﻿// FILELISTVIEWER.cpp: 구현 파일

#include "pch.h"
#include "NoSpear_Client.h"
#include "afxdialogex.h"
#include "FILELISTVIEWER.h"
using namespace std;
namespace fs = std::filesystem;


// FILELISTVIEWER 대화 상자

IMPLEMENT_DYNAMIC(FILELISTVIEWER, CFlexibleDialog)

FILELISTVIEWER::FILELISTVIEWER(CWnd* pParent /*=nullptr*/)
	: CFlexibleDialog(IDD_FILELISTVIEWDIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDI_ICON1);

}

FILELISTVIEWER::~FILELISTVIEWER()
{
}

void FILELISTVIEWER::DoDataExchange(CDataExchange* pDX)
{
	CFlexibleDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_FileListCtrl, filelistbox);
	DDX_Control(pDX, IDC_COMBO1, file_check_combo);
}

BOOL FILELISTVIEWER::OnInitDialog(){
	CDialogEx::OnInitDialog();

	filelistbox.InsertColumn(0, L"파일명", LVCFMT_LEFT, 200, -1);
	filelistbox.InsertColumn(1, L"확장자", LVCFMT_LEFT, 70, -1);
	filelistbox.InsertColumn(2, L"파일경로", LVCFMT_LEFT, 150, -1);
	filelistbox.InsertColumn(3, L"위험도", LVCFMT_LEFT, 90, -1);
	filelistbox.InsertColumn(4, L"검사 날짜", LVCFMT_LEFT, 100, -1);
	filelistbox.InsertColumn(5, L"ADS", LVCFMT_LEFT, 70, -1);
	filelistbox.InsertColumn(6, L"ZoneId", LVCFMT_LEFT, 70, -1);
	filelistbox.InsertColumn(7, L"ReferrerUrl", LVCFMT_LEFT, 70, -1);
	filelistbox.InsertColumn(8, L"HostUrl", LVCFMT_LEFT, 70, -1);
	filelistbox.InsertColumn(9, L"만든 날짜", LVCFMT_LEFT, 100, -1);
	filelistbox.InsertColumn(10, L"수정한 날짜", LVCFMT_LEFT, 100, -1);
	filelistbox.InsertColumn(11, L"엑세스한 날짜", LVCFMT_LEFT, 100, -1);
	filelistbox.SetExtendedStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);

	CRect rctComboBox, rctDropDown;
	file_check_combo.GetClientRect(&rctComboBox);
	file_check_combo.GetDroppedControlRect(&rctDropDown);
	file_check_combo.GetParent()->ScreenToClient(&rctDropDown);
	rctDropDown.bottom = rctDropDown.top + rctComboBox.Height() + 400;
	file_check_combo.MoveWindow(&rctDropDown);

	SetIcon(m_hIcon, TRUE);			// 큰 아이콘을 설정합니다.
	SetIcon(m_hIcon, FALSE);		// 작은 아이콘을 설정합니다.

	return 0;
}

void FILELISTVIEWER::OnPaint(){
	if (IsIconic())
	{
		CPaintDC dc(this); // 그리기를 위한 디바이스 컨텍스트입니다.

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 클라이언트 사각형에서 아이콘을 가운데에 맞춥니다.
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 아이콘을 그립니다.
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}


BEGIN_MESSAGE_MAP(FILELISTVIEWER, CFlexibleDialog)
	ON_WM_GETMINMAXINFO()
	ON_NOTIFY(HDN_ITEMCLICK, 0, &FILELISTVIEWER::OnHdnItemclickFilelistctrl)
	ON_BN_CLICKED(IDC_BUTTON1, &FILELISTVIEWER::OnBnClickedButton1)
	ON_BN_CLICKED(btn_SelectFolder, &FILELISTVIEWER::OnBnClickedSelectfolder)
	ON_NOTIFY(NM_DBLCLK, IDC_FileListCtrl, &FILELISTVIEWER::OnNMDblclkFilelistctrl)
	ON_CBN_SELCHANGE(IDC_COMBO1, &FILELISTVIEWER::OnCbnSelchangeCombo1)
	ON_BN_CLICKED(IDC_BUTTON2, &FILELISTVIEWER::OnBnClickedButton2)
END_MESSAGE_MAP()


// FILELISTVIEWER 메시지 처리기


void FILELISTVIEWER::OnGetMinMaxInfo(MINMAXINFO* lpMMI)
{
	// TODO: 여기에 메시지 처리기 코드를 추가 및/또는 기본값을 호출합니다.
	lpMMI->ptMinTrackSize.x = 1140;
	lpMMI->ptMinTrackSize.y = 600;

	CFlexibleDialog::OnGetMinMaxInfo(lpMMI);
}

bool FILELISTVIEWER::Has_ADS(CString filepath) {
	//filepath로 주어진 파일의 ADS를 확인해보고, Zone.Identifier가 있으면 true, 없으면 false 리턴
	WIN32_FIND_STREAM_DATA fsd;
	HANDLE hFind = NULL;

	try {
		hFind = ::FindFirstStreamW(filepath, FindStreamInfoStandard, &fsd, 0);
		if (hFind == INVALID_HANDLE_VALUE) throw ::GetLastError();

		for (;;) {
			CString tmp;
			tmp.Format(TEXT("%s"), fsd.cStreamName);
			if (tmp == L":Zone.Identifier:$DATA") {
				return true;
			}
			if (!::FindNextStreamW(hFind, &fsd)) {
				DWORD dr = ::GetLastError();
				if (dr != ERROR_HANDLE_EOF) throw dr;
				break;
			}
		}
	}
	catch (DWORD err) {
		AfxTrace(TEXT("Error! Windows error code: %u\n", err));
	}

	if (hFind != NULL) ::FindClose(hFind);

	return false;
}

void FILELISTVIEWER::PrintFolder(CString folderpath) {
	//매개변수로 입력된 폴더 경`로를 재귀 탐색하여 문서 파일을 Listview에 출력해줌.
	//filesystem test, https://stackoverflow.com/questions/62988629/c-stdfilesystemfilesystem-error-exception-trying-to-read-system-volume-inf

	string strfilepath = string(CT2CA(folderpath));
	fs::path rootdir(strfilepath);
	CString rootname = CString(rootdir.root_name().string().c_str()) + "\\";

	//기존 리스트 컨트롤 아이템 삭제
	filelistbox.DeleteAllItems();

	//NTFS 파일 시스템만 ADS지원함
	bool bNTFS = false;
	wchar_t szVolName[MAX_PATH], szFSName[MAX_PATH];
	DWORD dwSN, dwMaxLen, dwVolFlags;

	::GetVolumeInformation(rootname, szVolName, MAX_PATH, &dwSN, &dwMaxLen, &dwVolFlags, szFSName, MAX_PATH);
	if (CString(szFSName) == L"NTFS")
		bNTFS = true;

	auto iter = fs::recursive_directory_iterator(rootdir, fs::directory_options::skip_permission_denied);
	auto end_iter = fs::end(iter);
	auto ec = std::error_code();
	int count = 0;

	for (; iter != end_iter; iter.increment(ec)) {
		if (ec) {
			continue;
		}
		string ext = iter->path().extension().string();
		//One-Drive상의 일부 폴더 탐색 안되는 문제 있음
		//*.hwp; *.hwpx; *.pdf; *.doc; *.docx; *.xls; *.xlsx;
		if (ext == ".hwp" || ext == ".hwpx" || ext == ".pdf" || ext == ".doc" || ext == ".docx" || ext == ".xls" || ext == ".xlsx") {
			CString path = CString(iter->path().string().c_str());
			AfxTrace(path + "\n");
			//추후 Listview 입력부는 따로 분리해야 함.
			//DB와 연동해야하기 때문임
			filelistbox.InsertItem(count, iter->path().filename().c_str());
			filelistbox.SetItem(count, 1, LVIF_TEXT, CString((ext.substr(1, 4)).c_str()), 0, 0, 0, NULL);
			filelistbox.SetItem(count, 2, LVIF_TEXT, path, 0, 0, 0, NULL);
			filelistbox.SetItem(count, 3, LVIF_TEXT, (count % 5 == 1) ? L"미검사" : L"안전", 0, 0, 0, NULL);
			filelistbox.SetItem(count, 4, LVIF_TEXT, L"-", 0, 0, 0, NULL);
			int pos = 5;
			if (bNTFS && Has_ADS(path)) {
				CStdioFile ads_stream;
				CFileException e;
				if (!ads_stream.Open(path + L":Zone.Identifier", CFile::modeRead, &e)) {
					e.ReportError();
				}
				CString str;
				while (ads_stream.ReadString(str))
					filelistbox.SetItem(count, pos++, LVIF_TEXT, str, 0, 0, 0, NULL);
			}
			else {
				for (pos = 5 ; pos < 9; pos++)
					filelistbox.SetItem(count, pos, LVIF_TEXT, L"-", 0, 0, 0, NULL);
			}
			count++;
		}
	}
	CString tmp;
	tmp.Format(TEXT("Folder Search Complete. Find %d"), count);
	AfxTrace(tmp);

}
int FILELISTVIEWER::CompareItem(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
	CListCtrl* pList = ((SORTPARAM*)lParamSort)->pList;
	int iSortColumn = ((SORTPARAM*)lParamSort)->iSortColumn;
	bool bSortDirect = ((SORTPARAM*)lParamSort)->bSortDirect;
	CString strItem1 = pList->GetItemText(lParam1, iSortColumn);
	CString strItem2 = pList->GetItemText(lParam2, iSortColumn);
	return	bSortDirect ? strcmp(LPSTR(LPCTSTR(strItem1)), LPSTR(LPCTSTR(strItem2))) : -strcmp(LPSTR(LPCTSTR(strItem1)), LPSTR(LPCTSTR(strItem2)));
}

void FILELISTVIEWER::OnHdnItemclickFilelistctrl(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int nColumn = pNMLV->iItem;

	for (int i = 0; i < (filelistbox.GetItemCount()); i++) {
		filelistbox.SetItemData(i, i);
	}

	bAscending = !bAscending;

	SORTPARAM sortparams;
	sortparams.pList = &filelistbox;
	sortparams.iSortColumn = nColumn;
	sortparams.bSortDirect = bAscending;

	filelistbox.SortItems(&CompareItem, (LPARAM)&sortparams);
	*pResult = 0;
}



void FILELISTVIEWER::OnBnClickedButton1()
{
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	PrintFolder(L"C:\\Users");
}


void FILELISTVIEWER::OnBnClickedSelectfolder(){
	CFolderPickerDialog Picker(_T("C:\\Users"), OFN_FILEMUSTEXIST, NULL, 0);
	if (Picker.DoModal() == IDOK) {
		CString strFolderPath = Picker.GetPathName();
		//PC에서 다운로드한 파일을 USB에 복사할 경우, ADS는 복사가 안되기에 내부로 출력될 수 있음.
		PrintFolder(strFolderPath);
	}
}

void FILELISTVIEWER::OnNMDblclkFilelistctrl(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	int row = pNMItemActivate->iItem;
	if (row != -1) {
		CString filepath = filelistbox.GetItemText(row, 2);
		ShellExecute(NULL, _T("open"), _T("explorer"), _T("/select,") + filepath, NULL, SW_SHOW);
	}
	*pResult = 0;
}


void FILELISTVIEWER::OnCbnSelchangeCombo1(){
	int index = file_check_combo.GetCurSel();
	if (index != CB_ERR) {
		for (int i = 0; i < filelistbox.GetItemCount(); i++)
			filelistbox.SetCheck(i, FALSE);
		//CString str;
		//file_check_combo.GetLBText(index, str);
		switch (index){
			case 0:
				//전체 문서 선택
				for (int i = 0; i < filelistbox.GetItemCount(); i++)
					filelistbox.SetCheck(i, TRUE);
				break;
			case 1:
				//미검사 문서 선택 column 3
				for (int i = 0; i < filelistbox.GetItemCount(); i++) {
					if (filelistbox.GetItemText(i, 3) == L"미검사")
						filelistbox.SetCheck(i, TRUE);
				}
				break;
			case 2:
				//외부 문서 선택 column 5
				for (int i = 0; i < filelistbox.GetItemCount(); i++) {
					if (filelistbox.GetItemText(i, 5) == L"[ZoneTransfer]")
						filelistbox.SetCheck(i, TRUE);
				}
				break;
			case 3:
				//전체 선택 해제
				for (int i = 0; i < filelistbox.GetItemCount(); i++)
					filelistbox.SetCheck(i, FALSE);
				break;
		default:
			//여긴 올리가 없음
			break;
		}
	}
	
}


void FILELISTVIEWER::OnBnClickedButton2(){
	// TODO: 여기에 컨트롤 알림 처리기 코드를 추가합니다.
	for (int i = 0; i < filelistbox.GetItemCount(); i++) {
		bool result = filelistbox.GetCheck(i);
		CString tmp;
		tmp.Format(TEXT("%d : %d\n"), i, result);
		AfxTrace(tmp);

	}
}