class NOSPEAR {
	std::string SERVER_IP = "127.0.0.1";
	unsigned short SERVER_PORT = 42524;
	static const short FILE_BUFFER_SIZE = 4096;
	bool live_protect_status = false;
	void Deletefile(NOSPEAR_FILE file);
	CString errormsg;

public:
	NOSPEAR();
	NOSPEAR(std::string ip, unsigned short port);
	int Fileupload(NOSPEAR_FILE file);
	void ActivateLiveProtect(bool status);
	CString GetErrorMsg();

	//inode ��� �����ϴ� �Լ� �ʿ�
	//���� SQLite�� Ȯ���غ���, ���� ��� ������ ������
};

