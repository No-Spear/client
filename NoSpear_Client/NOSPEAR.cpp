#include "pch.h"
#include "NOSPEAR_FILE.h"
#include "LIVEPROTECT.h"
#include "NOSPEAR.h"
#include "ssl.h"

#pragma warning(disable:4996)
#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"Crypt32.lib")
#pragma comment(lib,"Secur32.lib")
#pragma comment(lib,"wininet.lib")
#pragma comment(lib,"fltLib.lib")

SOCKET s;
SSL_SOCKET* sx = NULL;
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

int NOSPEAR::Fileupload(NOSPEAR_FILE file){
	//SSL Socket Send
	//https://www.codeproject.com/Articles/24379/SSL-Convert-your-Plain-Sockets-to-SSL-Sockets-in-a
	AfxTrace(TEXT("[NOSPEAR::Fileupload] ���� ���ε� ����\n"));
	AfxTrace(TEXT("[NOSPEAR::Fileupload] name : " + file.Getfilename() + "\n"));
	AfxTrace(TEXT("[NOSPEAR::Fileupload] path : " + file.Getfilepath() + "\n"));
	AfxTrace(TEXT("[NOSPEAR::Fileupload] hash : " + CString(file.Getfilehash()) + "\n"));

	CString filename = file.Getfilename();

	//validation ȣ��
	if (file.Checkvalidation() == false) {
		AfxTrace(TEXT("[NOSPEAR::Fileupload] ����Ǵ� �������� Ȯ��\n"));
		errormsg = TEXT("[NOSPEAR::Fileupload] ���ε尡 ����Ǵ� �����Դϴ�.");
		return -1;
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
		AfxTrace(TEXT("[NOSPEAR::Fileupload] ������ ������ �� ����\n"));
		errormsg = TEXT("[NOSPEAR::Fileupload] ������ ������ �� �����ϴ�.");
		return -2;
	}

	getpeername(s, (sockaddr*)&aa, &slen);

	//CString to UTF-8
	std::string utf8_filename = CW2A(filename, CP_UTF8);

	//Send File Name Length
	unsigned int length = htonl(utf8_filename.size());
	send(s, (char*)&length, 4, 0);

	//Send File Name (UTF-8 String to char*)
	//UTP-8�� ������ �� ������ ����

	//Send File Hash
	send(s, file.Getfilehash(), 64, 0);

	unsigned int filesize = htonl(file.Getfilesize());
	send(s, (char*)&filesize, 4, 0);

	char file_buffer[NOSPEAR::FILE_BUFFER_SIZE];
	int read_size = 0;

	FILE* fp = _wfopen(file.Getfilepath(), L"rb");
	if (fp == NULL) {
		AfxTrace(TEXT("[NOSPEAR::Fileupload] ������ ��ȿ���� �ʽ��ϴ�.\n"));
		errormsg = TEXT("[NOSPEAR::Fileupload] ������ ��ȿ���� �ʽ��ϴ�.");
		closesocket(s);
		//delete(sx);
		return -3;
	}

	while ((read_size = fread(file_buffer, 1, NOSPEAR::FILE_BUFFER_SIZE, fp)) != 0) {
		send(s, file_buffer, read_size, 0);
	}

	AfxTrace(TEXT("[NOSPEAR::Fileupload] ���� ���ε� �Ϸ�\n"));
	errormsg = TEXT("[NOSPEAR::Fileupload] ���� ���ε� �Ϸ�");

	//�˻� ����� ���� �޽��ϴ�. ���� ����� ���
	unsigned short result = 0;
	recv(s, (char*)&result, 2, 0);
	result = ntohs(result);

	fclose(fp);
	closesocket(s);
	if(sx != NULL) delete(sx);

	return result;
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

CString NOSPEAR::GetErrorMsg(){
	return errormsg;
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
