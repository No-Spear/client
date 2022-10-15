#include "pch.h"
#include "NOSPEAR_FILE.h"
#include "LIVEPROTECT.h"
#include "NOSPEAR.h"

#pragma warning(disable:4996)
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Crypt32.lib")
#pragma comment(lib,"Secur32.lib")
#pragma comment(lib,"wininet.lib")
#pragma comment(lib,"fltLib.lib")

SOCKET s;
sockaddr_in dA, aa;
int slen = sizeof(sockaddr_in);

void NOSPEAR::Deletefile(NOSPEAR_FILE file){
	CFileFind pFind;
	CString filepath = file.Getfilepath();
	BOOL bRet = pFind.FindFile(filepath);
	if (bRet == TRUE) {
		if (DeleteFile(filepath) == TRUE) {
			AfxMessageBox(_T("���� �Ϸ�"));
		}
	}
}

DIAGNOSE_RESULT NOSPEAR::FileUpload(CString filepath){
	DIAGNOSE_RESULT diagnose_return;
	CFileFind pFind;
	BOOL bRet = pFind.FindFile(filepath);

	if (!bRet) {
		AfxTrace(TEXT("[NOSPEAR::FileUpload] ������ ��ȿ���� ����\n"));
		diagnose_return.result_msg = L"[NOSPEAR::FileUpload] ������ ��ȿ���� ����";
		diagnose_return.result_code = -1;
		return diagnose_return;
	}

	NOSPEAR_FILE file = NOSPEAR_FILE(filepath);

	AfxTrace(TEXT("[NOSPEAR::FileUpload] ���� ���ε� ����\n"));
	AfxTrace(TEXT("[NOSPEAR::FileUpload] name : " + file.Getfilename() + "\n"));
	AfxTrace(TEXT("[NOSPEAR::FileUpload] path : " + file.Getfilepath() + "\n"));
	AfxTrace(TEXT("[NOSPEAR::FileUpload] hash : " + CString(file.Getfilehash()) + "\n"));


	//validation ȣ��
	if (file.Checkvalidation() == false) {
		AfxTrace(TEXT("[NOSPEAR::FileUpload] ����Ǵ� �������� Ȯ��\n"));
		diagnose_return.result_msg = TEXT("[NOSPEAR::FileUpload] ���ε尡 ����Ǵ� �����Դϴ�.");
		diagnose_return.result_code = -1;
		return diagnose_return;
	}

	unsigned long inaddr;

	memset(&dA, 0, sizeof(dA));
	dA.sin_family = AF_INET;
	inaddr = inet_addr(SERVER_IP.c_str());
	if (inaddr != INADDR_NONE)
		memcpy(&dA.sin_addr, &inaddr, sizeof(inaddr));

	dA.sin_port = htons(SERVER_PORT);
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (connect(s, (sockaddr*)&dA, slen) < 0){
		AfxTrace(TEXT("[NOSPEAR::FileUpload] ������ ������ �� ����\n"));
		diagnose_return.result_msg = TEXT("[NOSPEAR::FileUpload] ������ ������ �� �����ϴ�.");
		diagnose_return.result_code = -2;
		return diagnose_return;
	}

	getpeername(s, (sockaddr*)&aa, &slen);

	//CString to UTF-8
	CString filename = file.Getfilename();
	std::string utf8_filename = CW2A(filename, CP_UTF8);

	//Send File Name Length
	unsigned int length = htonl(utf8_filename.size());
	send(s, (char*)&length, 4, 0);

	//Send File Name (UTF-8 String to char*) UTP-8�� ������ �� ������ ����
	send(s, utf8_filename.c_str(), (UINT)utf8_filename.size(), 0);

	//Send File Hash
	send(s, file.Getfilehash(), 64, 0);

	unsigned int filesize = htonl(file.Getfilesize());
	send(s, (char*)&filesize, 4, 0);

	char file_buffer[NOSPEAR::FILE_BUFFER_SIZE];
	int read_size = 0;

	FILE* fp = _wfopen(file.Getfilepath(), L"rb");
	if (fp == NULL) {
		AfxTrace(TEXT("[NOSPEAR::FileUpload] ������ ��ȿ���� �ʽ��ϴ�.\n"));
		diagnose_return.result_msg = TEXT("[NOSPEAR::FileUpload] ������ ��ȿ���� �ʽ��ϴ�.");
		closesocket(s);
		diagnose_return.result_code = -3;
		return diagnose_return;
	}

	while ((read_size = fread(file_buffer, 1, NOSPEAR::FILE_BUFFER_SIZE, fp)) != 0) {
		send(s, file_buffer, read_size, 0);
	}

	AfxTrace(TEXT("[NOSPEAR::FileUpload] ���� ���ε� �Ϸ�\n"));
	diagnose_return.result_msg = TEXT("[NOSPEAR::FileUpload] ���� ���ε� �Ϸ�");

	//�˻� ����� ���� �޽��ϴ�. ���� ����� ���
	unsigned short result = 0;
	recv(s, (char*)&result, 2, 0);
	result = ntohs(result);

	fclose(fp);
	closesocket(s);

	diagnose_return.result_code = result;
	return diagnose_return;
}

void NOSPEAR::GetMsgFromCode(DIAGNOSE_RESULT& result){

	if (result.result_code < 0) {
		return;
	}

	switch (result.result_code) {
		case TYPE_NORMAL:
			result.result_msg = "�м� ��� : ���� ����";
			break;
		case TYPE_MALWARE:
			result.result_msg = L"�м� ��� : �Ǽ� ����";
			break;
		case TYPE_SUSPICIOUS:
			result.result_msg = L"�м� ��� : �Ǽ� �ǽ� ����";
			break;
		case TYPE_UNEXPECTED:
			result.result_msg = L"�м� ��� : �� �� ���� ����";
			break;
		case TYPE_NOFILE:
			result.result_msg = L"�м� ��� : ���� ������ �ƴ�";
			break;
		case TYPE_RESEND:
			result.result_msg = L"������ ���ε��ϴ� �� ������ �߻��Ͽ����ϴ�";
			break;
		case TYPE_REJECT:
			result.result_msg = L"�������� �˻縦 �ź��Ͽ����ϴ�.";
			break;
		default:
			result.result_msg = L"Unknown Response";
			break;
	}
}

void NOSPEAR::ActivateLiveProtect(bool status){
	//�Է°��� ���� ���� ���� ��� �Լ� ���� ���
	if (live_protect_status == status)
		return;

	//LIVEPROTECT ��ü ����
	if (status) {
		if (liveprotect == NULL) {
			liveprotect = new LIVEPROTECT();
			HRESULT result = liveprotect->ActivateLiveProtect();
			CString resultext;
			if (result == S_OK) {
				AfxMessageBox(_T("����̹� ���� ����\n"));
				live_protect_status = true;
			}
			else {
				resultext.Format(_T("LIVEPROTECT::Init() return : %ld\n"), result);
				AfxMessageBox(resultext);
				delete(liveprotect);
				liveprotect = NULL;
			}
		}
	
	}
	else {
		liveprotect->InActivateLiveProtect();
		delete(liveprotect);
		liveprotect = NULL;
		AfxMessageBox(_T("����̹� ���� ����\n"));
		live_protect_status = false;
	}
}

DIAGNOSE_RESULT NOSPEAR::SingleDiagnose(CString file){
	DIAGNOSE_RESULT dignose_result;
	dignose_result = FileUpload(file);
	GetMsgFromCode(dignose_result);
	return dignose_result;
}

vector<DIAGNOSE_RESULT> NOSPEAR::MultipleDiagnose(vector<CString> files){
	vector<DIAGNOSE_RESULT> diagnose_result;

	for (int i = 0; i < files.size(); i++) {
		DIAGNOSE_RESULT tmp;
		CString filepath = files.at(i);
		tmp = FileUpload(filepath);
		GetMsgFromCode(tmp);
		diagnose_result.push_back(tmp);
	}
	return diagnose_result;
}


NOSPEAR::NOSPEAR(){
	//�⺻������
	//�Ϲ� ����� ȯ�濡���� �ϵ��ڵ��� ���� �ּҷ� ������
}

NOSPEAR::NOSPEAR(std::string ip, unsigned short port){
	//config.dat ������ ���� �� ���Ǵ� ������
	SERVER_IP = ip;
	SERVER_PORT = port;
}
