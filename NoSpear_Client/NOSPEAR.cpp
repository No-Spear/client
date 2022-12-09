#include "pch.h"
#include "NOSPEAR_FILE.h"
#include "LIVEPROTECT.h"
#include "NOSPEAR.h"
#include "resource.h"
#include "SQLITE.h"

#pragma warning(disable:4996)
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Crypt32.lib")
#pragma comment(lib,"Secur32.lib")
#pragma comment(lib,"wininet.lib")
#pragma comment(lib,"fltLib.lib")
#pragma comment(lib,"sqlite3.lib")
#define WM_TRAY_NOTIFYICACTION (WM_USER + 10)
namespace fs = std::filesystem;

SOCKET s;
sockaddr_in dA;

SOCKET s2;
sockaddr_in dA2;
void NOSPEAR::Deletefile(CString filepath){
	CFileFind pFind;
	BOOL bRet = pFind.FindFile(filepath);
	if (bRet == TRUE) {
		if (DeleteFile(filepath) == TRUE) {
			AfxMessageBox(_T("���� �Ϸ�"));
		}
	}
}

bool NOSPEAR::NospearOnlineDiagnose(NOSPEAR_FILE& file){
	AfxTrace(TEXT("[NOSPEAR::NospearOnlineDiagnose] ���� ���ε� ����\n"));
	AfxTrace(TEXT("[NOSPEAR::NospearOnlineDiagnose] name : " + file.Getfilename() + "\n"));
	AfxTrace(TEXT("[NOSPEAR::NospearOnlineDiagnose] path : " + file.Getfilepath() + "\n"));
	AfxTrace(TEXT("[NOSPEAR::NospearOnlineDiagnose] hash : " + CString(file.Getfilehash()) + "\n"));

	//validation ȣ��
	if (file.Checkvalidation() == false) {
		AfxTrace(TEXT("[NOSPEAR::NospearOnlineDiagnose] ����Ǵ� �������� Ȯ��\n"));
		file.diag_result.result_code = -1;
		return false;
	}

	unsigned long inaddr;

	memset(&dA, 0, sizeof(dA));
	dA.sin_family = AF_INET;
	inaddr = inet_addr(SERVER_IP.c_str());
	if (inaddr != INADDR_NONE)
		memcpy(&dA.sin_addr, &inaddr, sizeof(inaddr));

	dA.sin_port = htons(SERVER_Diagnose_PORT);
	s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (connect(s, (sockaddr*)&dA, sizeof(sockaddr_in)) < 0){
		AfxTrace(TEXT("[NOSPEAR::NospearOnlineDiagnose] ������ ������ �� ����\n"));
		file.diag_result.result_code = -2;
		return false;
	}

	//CString to UTF-8
	CString filename = file.Getfilename();
	std::string utf8_filename = CW2A(filename, CP_UTF8);

	//Send File Name Length
	unsigned int length = htonl(utf8_filename.size());
	send(s, (char*)&length, 4, 0);

	//Send File Name (UTF-8 String to char*) UTP-8�� ������ �� ������ ����
	send(s, utf8_filename.c_str(), (UINT)utf8_filename.size(), 0);

	//Send File Hash
	//CString to UTF-8
	CString filehash = file.Getfilehash();
	std::string utf8_filehash = CW2A(filehash, CP_UTF8);
	send(s, utf8_filehash.c_str(), 64, 0);

	unsigned int filesize = htonl(file.Getfilesize());
	send(s, (char*)&filesize, 4, 0);

	char file_buffer[NOSPEAR::FILE_BUFFER_SIZE];
	int read_size = 0;

	FILE* fp = _wfopen(file.Getfilepath(), L"rb");
	if (fp == NULL) {
		AfxTrace(TEXT("[NOSPEAR::NospearOnlineDiagnose] ������ ��ȿ���� �ʽ��ϴ�.\n"));
		closesocket(s);
		file.diag_result.result_code = -3;
		return false;
	}

	while ((read_size = fread(file_buffer, 1, NOSPEAR::FILE_BUFFER_SIZE, fp)) != 0) {
		send(s, file_buffer, read_size, 0);
	}

	AfxTrace(TEXT("[NOSPEAR::NospearOnlineDiagnose] ���� ���ε� �Ϸ�\n"));

	//�˻� ����� ���� �޽��ϴ�. ���� ����� ���
	unsigned short diag_result = 0;
	recv(s, (char*)&diag_result, 2, 0);
	file.diag_result.result_code = ntohs(diag_result);

	fclose(fp);
	closesocket(s);

	return true;
}

bool NOSPEAR::NospearOfflineDiagnose(NOSPEAR_FILE& file){
	//BlackList DB�� Ȯ���ϰ�, Ž���Ǹ� TRUE, ��Ž���Ǹ� FALSE ����
	AfxTrace(TEXT("[NOSPEAR::NospearOfflineDiagnose] ������Ʈ DB �˻� ����\n"));
	
	sqlite3_select p_selResult = nospearDB->SelectSqlite(L"select * from NOSPEAR_BlackList WHERE Hash='"+ file.Getfilehash() + L"'");
	string timeStamp;
	if (p_selResult.pnRow != 0) {
		file.diag_result.result_code = TYPE_MALWARE;
		return true;
	}
	return false;
}


void NOSPEAR::InitNospear(){
	nospearDB = new SQLITE();
	nospearDB->DatabaseOpen(L"NOSPEAR");
	
	nospearDB->ExecuteSqlite(L"CREATE TABLE IF NOT EXISTS NOSPEAR_HISTORY(SEQ INTEGER PRIMARY KEY AUTOINCREMENT, TimeStamp TEXT not null DEFAULT (datetime('now', 'localtime')), FilePath TEXT NOT NULL, ProcessName TEXT, Operation TEXT, NOSPEAR INTEGER, Permission TEXT);");
	nospearDB->ExecuteSqlite(L"CREATE TABLE IF NOT EXISTS NOSPEAR_LocalFileList(FilePath TEXT NOT NULL PRIMARY KEY, ZoneIdentifier INTEGER, ProcessName TEXT, NOSPEAR INTEGER, DiagnoseDate TEXT, Hash TEXT, Serverity INTEGER, FileType TEXT, TimeStamp TEXT not null DEFAULT (datetime('now', 'localtime')));");
	nospearDB->ExecuteSqlite(L"CREATE TABLE IF NOT EXISTS NOSPEAR_VersionInfo(VersionName TEXT NOT NULL PRIMARY KEY, TimeStamp TEXT not null DEFAULT (datetime('now', 'localtime')));");
	nospearDB->ExecuteSqlite(L"INSERT INTO NOSPEAR_VersionInfo(VersionName, TimeStamp) VALUES ('BlackListDB', '2022-09-01 00:00:00');");
	nospearDB->ExecuteSqlite(L"CREATE TABLE IF NOT EXISTS NOSPEAR_Quarantine(FilePath TEXT NOT NULL PRIMARY KEY, FileHash TEXT, TimeStamp TEXT not null DEFAULT (datetime('now', 'localtime')));");
	nospearDB->ExecuteSqlite(L"CREATE TABLE IF NOT EXISTS NOSPEAR_BlackList(Hash TEXT NOT NULL PRIMARY KEY, TimeStamp TEXT not null DEFAULT (datetime('now', 'localtime')));");
	nospearDB->ExecuteSqlite(L"CREATE TABLE IF NOT EXISTS NOSPEAR_Backup(FileName TEXT NOT NULL PRIMARY KEY, FileHash TEXT, TimeStamp TEXT not null DEFAULT (datetime('now', 'localtime')));");
	

	office_file_ext_list.insert(L".doc");
	office_file_ext_list.insert(L".docx");
	office_file_ext_list.insert(L".xls");
	office_file_ext_list.insert(L".xlsx");
	office_file_ext_list.insert(L".pptx");
	office_file_ext_list.insert(L".ppsx");
	office_file_ext_list.insert(L".hwp");
	office_file_ext_list.insert(L".hwpx");
	office_file_ext_list.insert(L".pdf");

	CreateDirectory(L"Quarantine", NULL);
	CreateDirectory(L"Backup", NULL);
}

CString NOSPEAR::GetMsgFromErrCode(short err_code){
	switch (err_code) {
		case -1:
			return L"���ε� ���� ����";
		case -2:
			return L"���� ���� ����";
		case -3:
			return L"������ ��ȿ���� ����";
		case TYPE_NORMAL:
			return L"���� ����";
		case TYPE_MALWARE:
			return L"�Ǽ� ����";
		case TYPE_SUSPICIOUS:
			return L"�Ǽ� �ǽ�";
		case TYPE_UNEXPECTED:
			return L"�� �� ����";
		case TYPE_NOFILE:
			return L"���� ���� �ƴ�";
		case TYPE_RESEND:
			return L"���� ���ε� ����";
		case TYPE_REJECT:
			return L"�˻� �ź�";
		case TYPE_LOCAL:
			return L"����";
		default:
			return L"Error";
	}
}
CString NOSPEAR::GetMsgFromNospear(short nospear) {
	switch (nospear) {
		case 0:
			return L"��� ����";
		case 1:
			return L"���� ����";
		case 2:
			return L"��ü ���";
		default:
			return L"Error";
	}
}

bool NOSPEAR::ActivateLiveProtect(bool status){
	//�Է°��� ���� ���� ���� ��� �Լ� ���� ���
	if (live_protect_status == status)
		return status;

	//LIVEPROTECT ��ü ����
	if (status) {
		if (liveprotect == NULL) {
			liveprotect = new LIVEPROTECT();
			//����� ����ü�� �Ѱܼ� �޽����� ���� ������ָ� ������
			HRESULT result = liveprotect->ActivateLiveProtect();
			CString resultext;
			if (result == S_OK) {
				Notification(L"�ǽð� ���ð� Ȱ��ȭ�Ǿ����ϴ�.", L"���ο� ���� ���� ������ ���� ���� ������ �����մϴ�.");
				//AfxMessageBox(_T("����̹� ���� ����\n"));
				live_protect_status = true;
			}
			else {
				if (result == 2)
					AfxMessageBox(L"����̹��� ������ �� �����ϴ�.\n����̹��� ���� ������ Ȯ���ϼ���.");
				else {
					resultext.Format(_T("LIVEPROTECT::Init() return : %ld\n"), result);
					AfxMessageBox(resultext);
				}
				
				delete(liveprotect);
				liveprotect = NULL;
			}
		}
	
	}
	else {
		Notification(L"�ǽð� ���ð� ��Ȱ��ȭ�Ǿ����ϴ�.", L"���ο� ���� ���� ������ ���� ���� ���ٿ� ���� ������ �����մϴ�.");
		liveprotect->InActivateLiveProtect();
		delete(liveprotect);
		liveprotect = NULL;
		//AfxMessageBox(_T("����̹� ���� ����\n"));
		live_protect_status = false;
	}
	return live_protect_status;
}

bool NOSPEAR::Diagnose(NOSPEAR_FILE& file){
	//BlackList DB�� ���� Ȯ���ϰ�, BlackList DB�� ���ٸ� OnLine �˻� ��û
	bool result = NospearOfflineDiagnose(file);
	if (!result) {
		result = NospearOnlineDiagnose(file);
	}

	file.diag_result.result_msg = GetMsgFromErrCode(file.diag_result.result_code);
	if (result) {
		int NOSPEAR = 1;
		bool updateDB = true;;
		switch (file.diag_result.result_code) {
			case TYPE_NORMAL:
				NOSPEAR = 2;
				WriteNospearADS(file.Getfilepath(), 2);
				break;
			case TYPE_MALWARE:
				Quarantine(file.Getfilepath());
				Notification(L"�Ǽ� ������ Ž���Ǿ����ϴ�.", file.Getfilename() + L"�� �˿��ҷ� �̵��Ͽ����ϴ�.");
				AfxTrace(L"[NOSPEAR::Diagnose] " + file.Getfilename() + L"�� �Ǽ� �����̹Ƿ� �˿��ҷ� �̵��Ͽ����ϴ�.\n");
				break;
			case TYPE_SUSPICIOUS:
				Notification(L"�Ǽ� �ǽ� ������ Ž���Ǿ����ϴ�.", file.Getfilename() + L"���� ���࿡ �����ϼ���.");
				WriteNospearADS(file.Getfilepath(), 1);
				break;
			default:
				//��Ÿ ���� ������ �˻簡 ������� �ʾ��� ���, DB�� �˻� ����� ������ ����
				updateDB = false;
				break;
		}
		if (updateDB) {
			CString strInsQuery;
			strInsQuery.Format(TEXT("UPDATE NOSPEAR_LocalFileList SET DiagnoseDate=(datetime('now', 'localtime')), Hash='%ws', Serverity='%d', NOSPEAR='%d' WHERE FilePath='%ws';"), file.Getfilehash(), file.diag_result.result_code, NOSPEAR, file.Getfilepath());
			nospearDB->ExecuteSqlite(strInsQuery);
		}
	}
	return result;
}

void NOSPEAR::Notification(CString title, CString body) {
	ZeroMemory(&nid, sizeof(nid));
	nid.cbSize = sizeof(nid);
	nid.dwInfoFlags = NIIF_WARNING;
	nid.uFlags = NIF_MESSAGE | NIF_INFO | NIF_ICON;
	nid.uTimeout = 2000;
	nid.hWnd = AfxGetApp()->m_pMainWnd->m_hWnd;
	nid.uCallbackMessage = WM_TRAY_NOTIFYICACTION;
	nid.hIcon = AfxGetApp()->LoadIconW(IDR_MAINFRAME);
	lstrcpy(nid.szInfoTitle, title);
	lstrcpy(nid.szInfo, body);
	::Shell_NotifyIcon(NIM_MODIFY, &nid);
}

void NOSPEAR::AutoDiagnose() {
	int total = 0, success = 0;
	total = request_diagnose_queue.size();
	while (!request_diagnose_queue.empty()) {
		CString filepath = request_diagnose_queue.front();
		request_diagnose_queue.pop();
		CFileFind pFind;
		BOOL bRet = pFind.FindFile(filepath);
		if (bRet == FALSE) {
			continue;
		}
		NOSPEAR_FILE file(filepath);
		bRet = Diagnose(file);
		if (!bRet && file.diag_result.result_code == -2) {
			Notification(L"No-Spear ���� ���� ����", L"���� �˻縦 ���� No-Spear ���� ���ῡ �����Ͽ����ϴ�.");
			while (!request_diagnose_queue.empty()) request_diagnose_queue.pop();
		}
		if (bRet) {
			success++;
		}
	}
	CString result;
	result.Format(TEXT("��ü %d�� �� %d�� �˻簡 �Ϸ�Ǿ����ϴ�."), total, success);
	Notification(L"No-Spear �˻� ���", result);

}

SQLITE* NOSPEAR::GetSQLitePtr(){
	return nospearDB;
}

NOSPEAR::NOSPEAR(){
	//�⺻������
	//�Ϲ� ����� ȯ�濡���� �ϵ��ڵ��� ���� �ּҷ� ������
	InitNospear();
}
NOSPEAR::NOSPEAR(std::string ip, unsigned short port1, unsigned short port2) {
	//config.dat ������ ���� �� ���Ǵ� ������
	SERVER_IP = ip;
	SERVER_Diagnose_PORT = port1;
	SERVER_Update_PORT = port2;
	InitNospear();
}

NOSPEAR::~NOSPEAR(){
	if (nospearDB != NULL) {
		nospearDB->~SQLITE();
		delete(nospearDB);
	}
	if (liveprotect != NULL) {
		ActivateLiveProtect(false);
		delete(liveprotect);
	}
}


bool NOSPEAR::HasZoneIdentifierADS(CString filepath) {
	CStdioFile ads_stream;
	CFileException e;
	if (!ads_stream.Open(filepath + L":Zone.Identifier", CFile::modeRead, &e)) {
		return false;
	}
	return true;
}
unsigned short NOSPEAR::ReadNospearADS(CString filepath) {

	CStdioFile ads_stream;
	CFileException e;
	if (!ads_stream.Open(filepath + L":NOSPEAR", CFile::modeRead, &e)) {
		return -1;
	}

	CString str;
	ads_stream.ReadString(str);

	if (str == L"0")
		return 0;
	else if (str == L"1")
		return 1;
	else if (str == L"2")
		return 2;
	else {
		return -1;
	}
}
bool NOSPEAR::WriteNospearADS(CString filepath, unsigned short value) {
	//value ������ ADS:NOSPEAR ����
	CString strInsQuery;
	strInsQuery.Format(TEXT("UPDATE NOSPEAR_LocalFileList SET NOSPEAR='%d' WHERE FilePath='%ws';"), value, filepath);
	nospearDB->ExecuteSqlite(strInsQuery);

	CStdioFile ads_stream;
	CFileException e;
	if (!ads_stream.Open(filepath + L":NOSPEAR", CStdioFile::modeCreate | CStdioFile::modeWrite, &e)) {
		AfxTrace(TEXT("WriteNospearADS ���� ���� %d\n"), value);
		return false;
	}
	CString str;
	str.Format(TEXT("%d"), value);
	ads_stream.WriteString(str);
	AfxTrace(TEXT("WriteNospearADS ���� ���� %d\n"), value);

	return true;
}

bool NOSPEAR::WriteZoneIdentifierADS(CString filepath, CString processName) {
	//pid�� �̿��ؼ� ProcessName���� ADS:Zone.Identifier ����
	CStdioFile ads_stream;
	CFileException e;
	if (!ads_stream.Open(filepath + L":Zone.Identifier", CStdioFile::modeCreate | CStdioFile::modeWrite, &e)) {
		AfxTrace(L"WriteZoneIdentifierADS ���� ����\n");
		return false;
	}
	ads_stream.WriteString(L"[ZoneTransfer]\n");
	ads_stream.WriteString(L"ZoneId=3\n");
	ads_stream.WriteString(L"ADS Appended By No-Spear Client\n");
	ads_stream.WriteString(L"ProcessName=" + processName);
	AfxTrace(L"WriteZoneIdentifierADS ���� ����\n");

	CString strInsQuery;
	strInsQuery.Format(TEXT("UPDATE NOSPEAR_LocalFileList SET ZoneIdentifier='3', ProcessName='%ws' WHERE FilePath='%ws';"), processName, filepath);
	nospearDB->ExecuteSqlite(strInsQuery);
	return true;
}

bool NOSPEAR::DeleteZoneIdentifierADS(CString filepath) {
	CString strInsQuery;
	strInsQuery.Format(TEXT("UPDATE NOSPEAR_LocalFileList SET ZoneIdentifier='0' WHERE FilePath='%ws';"), filepath);
	nospearDB->ExecuteSqlite(strInsQuery);
	return DeleteFile(filepath + L":Zone.Identifier");
}

bool NOSPEAR::UpdataBlackListDB(){
	//���� ������Ʈ
	AfxTrace(L"[UpdataBlackListDB] Start BlackList Pattern Update\n");
	unsigned long inaddr;
	memset(&dA2, 0, sizeof(dA2));
	dA2.sin_family = AF_INET;
	inaddr = inet_addr(SERVER_IP.c_str());
	if (inaddr != INADDR_NONE)
		memcpy(&dA2.sin_addr, &inaddr, sizeof(inaddr));

	dA2.sin_port = htons(SERVER_Update_PORT);
	s2 = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (connect(s2, (sockaddr*)&dA2, sizeof(sockaddr_in)) < 0) {
		AfxTrace(TEXT("[NOSPEAR::UpdataBlackListDB] ������ ������ �� ����\n"));
		return false;
	}

	//NOSPEAR_VersionInfo ���̺��� BlackListDB ������ ������Ʈ ���ڸ� �������� ������ ������
	sqlite3_select p_selResult = nospearDB->SelectSqlite(L"select TimeStamp from NOSPEAR_VersionInfo WHERE VersionName='BlackListDB'");
	string timeStamp;
	if (p_selResult.pnRow != 0) {
		timeStamp = string(p_selResult.pazResult[1]);
		AfxTrace(TEXT("[NOSPEAR::UpdataBlackListDB] Update BlackListDB from %ws\n"), CString(p_selResult.pazResult[1]));
	}
	else {
		AfxTrace(TEXT("[NOSPEAR::UpdataBlackListDB] Can't access Last Pattern Update Data\n"));
		return false;
	}

	//������Ʈ�� ��û�� �ð����� DB �ֽ�ȭ(������Ʈ�� �����ϸ� �ٽ� timeStamp ������ ����
	nospearDB->ExecuteSqlite(L"update NOSPEAR_VersionInfo set TimeStamp=(datetime('now', 'localtime')) WHERE VersionName='BlackListDB';");

	//������ ���� ������Ʈ ���ڸ� ������ ����
	send(s2, timeStamp.c_str(), (UINT)timeStamp.size(), 0);

	//4����Ʈ json ��ü ���� ����
	unsigned int recv_size = 0;
	recv(s2, (char*)&recv_size, 4, 0);
	recv_size = ntohl(recv_size);

	//�����ؼ� Vector�� ����
	unsigned char arr[65] = {0, };
	std::vector<CString> blackList;
	unsigned int count = 0;
	int rs = 0;
	while (count < recv_size) {
		rs = recv(s2, (char*)arr, 64, 0);
		if (rs == 64) {
			arr[64] = '\0';
			blackList.push_back(CString(arr));
		}
		else{
			int size = rs;
			int idx = rs;
			AfxTrace(L"error\n");
			AfxTrace(TEXT("%d\n"), rs);
			AfxTrace(CString(arr) + "\n");
			while (size < 64) {
				idx = recv(s2, (char*)arr + size, 64 - size, 0);
				size += idx;
			}
			AfxTrace(CString(arr) + "\n");
		}
		count++;

	}

	//DB�� ����
	std::vector<CString>::iterator it;
	for (it = blackList.begin(); it != blackList.end(); it++) {
		nospearDB->ExecuteSqlite(L"INSERT INTO NOSPEAR_BlackList(Hash) VALUES ('" + *it + L"');");
	}

	closesocket(s2);

	AfxTrace(TEXT("[UpdataBlackListDB] Complete BlackList Pattern Update(size : %d)\n"), recv_size);
	return true;
}

void NOSPEAR::AppendDiagnoseQueue(CString filepath) {
	request_diagnose_queue.push(filepath);
}

void NOSPEAR::ScanLocalFile(CString rootPath) {
	//�Ű������� �Էµ� ���� ��θ� ��� Ž���Ͽ� ���� ������ DB�� ������
	//Host -> DB ���� �˻�
	//filesystem test, https://stackoverflow.com/questions/62988629/c-stdfilesystemfilesystem-error-exception-trying-to-read-system-volume-inf

	string strfilepath = string(CT2CA(rootPath));
	fs::path rootdir(strfilepath);
	CString rootname = CString(rootdir.root_name().string().c_str()) + "\\";

	//NTFS ���� �ý��۸� ADS������
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

		CString ext(iter->path().extension().string().c_str());
		CString path(iter->path().string().c_str());
		// || ext == L".lnk"
		if (ext == L".exe") {
			CString tmp = path;
			CString tmp_ext;
			while ((tmp_ext = PathFindExtension(tmp)).GetLength() != 0) {
				if (IsOfficeFile(tmp_ext)) {
					Notification(L"������ ������ ���� ������ �߰��Ͽ����ϴ�.", path);
					AfxTrace(TEXT("������ ������ ���� ������ �߰� : %ws\n"), path);
					break;
				}
				AfxTrace(TEXT("Find Ext : %ws\n"), tmp_ext);
				tmp.Replace(tmp_ext, L"");
				tmp.Trim();
			}
		}
		if (IsOfficeFile(ext)) {
			CString strInsQuery;
			short nospear = 2, zoneid = 0, serverity = TYPE_LOCAL;
			if (bNTFS && HasZoneIdentifierADS(path)) {
				nospear = 1;
				zoneid = 3;
				serverity = TYPE_SUSPICIOUS;
			}
			strInsQuery.Format(TEXT("INSERT INTO NOSPEAR_LocalFileList(FilePath, ZoneIdentifier, ProcessName, NOSPEAR, DiagnoseDate, Hash, Serverity, FileType) VALUES ('%ws','%d','No-Spear Client','%d','-', '-', '%d','DOCUMENT');"), path, zoneid, nospear, serverity);
			int rc = nospearDB->ExecuteSqlite(strInsQuery);
			//ADS:NOSPEAR�� �ٸ��� Ȯ���ϴ� ����
			//rc�� 19�� �̹� �� �ִ� ��.
			if (rc == 0) {
				count++;
			}
			else {
				nospear = ReadNospearADS(path);
				if(nospear > -1) {
					strInsQuery.Format(TEXT("UPDATE NOSPEAR_LocalFileList SET NOSPEAR='%d' WHERE FilePath='%ws';"), nospear, path);
					nospearDB->ExecuteSqlite(strInsQuery);
				}
			}
			AfxTrace(TEXT("path : %ws, rc : %d\n"), path, rc);
		}
	}
	CString tmp;
	tmp.Format(TEXT("���ο� %d���� ������ LocalFileListDB�� �߰��Ǿ����ϴ�.\n"), count);
	AfxTrace(tmp);
}

bool NOSPEAR::IsOfficeFile(CString ext) {
	set<CString>::iterator it = office_file_ext_list.find(ext);

	if (it != office_file_ext_list.end())
		return true;
	else
		return false;
}

bool NOSPEAR::IsQueueEmpty() {
	return request_diagnose_queue.empty();
}

void NOSPEAR::ScanFileAvailability() {
	sqlite3_select p_selResult = nospearDB->SelectSqlite(L"select FilePath from NOSPEAR_LocalFileList;");
	if (p_selResult.pnRow != 0) {
		for (int i = 1; i <= p_selResult.pnRow; i++) {
			int colCtr = 0;
			int nCol = 1;
			int cellPosition = (i * p_selResult.pnColumn) + colCtr;
			CString FilePath = SQLITE::Utf8ToCString(p_selResult.pazResult[cellPosition++]);
			CFileFind pFind;
			BOOL bRet = pFind.FindFile(FilePath);
			if (bRet == FALSE) {
				nospearDB->ExecuteSqlite(L"DELETE FROM NOSPEAR_LocalFileList WHERE FilePath='" + FilePath + L"';");
				AfxTrace(FilePath+ L" ������ ��ȿ���� ����.\n");
			}
		}
	}
}
void NOSPEAR::AttachADSOther() {
	sqlite3_select p_selResult = nospearDB->SelectSqlite(L"select FilePath, ZoneIdentifier from NOSPEAR_LocalFileList WHERE FileType='OTHER';");
	if (p_selResult.pnRow != 0) {
		for (int i = 1; i <= p_selResult.pnRow; i++) {
			int colCtr = 0;
			int nCol = 1;
			int cellPosition = (i * p_selResult.pnColumn) + colCtr;
			CString filePath = SQLITE::Utf8ToCString(p_selResult.pazResult[cellPosition++]);
			CString zoneId = SQLITE::Utf8ToCString(p_selResult.pazResult[cellPosition++]);
			CFileFind pFind;
			BOOL bRet = pFind.FindFile(filePath);
			if (bRet == FALSE) {
				nospearDB->ExecuteSqlite(L"DELETE FROM NOSPEAR_LocalFileList WHERE FilePath='" + filePath + L"';");
				AfxTrace(filePath + L" ������ ��ȿ���� ����.\n");
			}
			else {
				if (HasZoneIdentifierADS(filePath)) {
					nospearDB->ExecuteSqlite(L"DELETE FROM NOSPEAR_LocalFileList WHERE FilePath='" + filePath + L"';");
					AfxTrace(filePath + L" ADS�� �����Ǿ����Ƿ� DB���� ����\n");
				}
				WriteZoneIdentifierADS(filePath, zoneId);
			}
		}
	}
}
void NOSPEAR::Quarantine(CString filepath){
	//���� ������ XOR ���ڵ�
	NOSPEAR_FILE file(filepath);
	if (file.Quarantine()) {
		nospearDB->ExecuteSqlite(L"INSERT INTO NOSPEAR_Quarantine(FilePath, FileHash) VALUES ('"+filepath+"', '"+ file.Getfilehash() + L"');");
		DeleteFile(filepath);
		nospearDB->ExecuteSqlite(L"DELETE FROM NOSPEAR_LocalFileList WHERE FilePath='" + filepath + L"';");
	}
}
void NOSPEAR::InQuarantine(CString filepath) {
	//���� ���Ϸ� XOR ���ڵ�
	sqlite3_select p_selResult = nospearDB->SelectSqlite(L"select FileHash from NOSPEAR_Quarantine WHERE FilePath='"+ filepath + L"'");
	CString hash;
	if (p_selResult.pnRow != 0) {
		hash = CString(p_selResult.pazResult[1]);
		NOSPEAR_FILE file(L".\\Quarantine\\" + hash);
		file.InQuarantine(filepath);
		DeleteFile(file.Getfilepath());
		nospearDB->ExecuteSqlite(L"DELETE FROM NOSPEAR_Quarantine WHERE FilePath='" + filepath + L"';");
	}
 }

void NOSPEAR::BackUp(CString filepath){
	NOSPEAR_FILE file(filepath);
	if (file.BackUp()) {
		nospearDB->ExecuteSqlite(L"REPLACE INTO NOSPEAR_Backup (FileName, FileHash) VALUES ('" + CString(PathFindFileName(filepath)) + "', '" + file.Getfilehash() + L"');");
	}
}

void NOSPEAR::Recovery(CString folderPath){
	sqlite3_select p_selResult = nospearDB->SelectSqlite(L"select FileName, FileHash from NOSPEAR_Backup");
	if (p_selResult.pnRow != 0) {
		for (int i = 1; i <= p_selResult.pnRow; i++) {
			int colCtr = 0;
			int nCol = 1;
			int cellPosition = (i * p_selResult.pnColumn) + colCtr;
			CString fileName = SQLITE::Utf8ToCString(p_selResult.pazResult[cellPosition++]);
			CString fileHash = SQLITE::Utf8ToCString(p_selResult.pazResult[cellPosition++]);
			CFileFind pFind;
			BOOL bRet = pFind.FindFile(L".\\Backup\\" + fileHash);
			if (bRet == FALSE) {
				nospearDB->ExecuteSqlite(L"DELETE FROM NOSPEAR_Backup WHERE FileHash='" + fileHash + L"';");
				AfxTrace(fileName + L" ������ ��ȿ���� ����.\n");
			}
			else {
				NOSPEAR_FILE file(L".\\Backup\\" + fileHash);
				file.Recovery(folderPath + fileName);
			}
		}
	}
}
