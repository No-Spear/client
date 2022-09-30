class NOSPEAR {
	std::string SERVER_IP = "127.0.0.1";
	unsigned short SERVER_PORT = 42524;
	static const short FILE_BUFFER_SIZE = 4096;
	CString errormsg;
	void Deletefile(NOSPEAR_FILE file);

public:
	int Fileupload(NOSPEAR_FILE file);
	NOSPEAR();
	NOSPEAR(std::string ip, unsigned short port);
	CString GetErrorMsg();

};

