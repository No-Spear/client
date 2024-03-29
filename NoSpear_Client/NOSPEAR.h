#ifndef NOSPEAR_H
#define NOSPEAR_H
class LIVEPROTECT;
class NOSPEAR_FILE;
class DIAGNOSE_RESULT;
class SQLITE;

class NOSPEAR {
	NOTIFYICONDATA nid;
	std::string SERVER_IP = "127.0.0.1";		//백신서버주소
	unsigned short SERVER_Diagnose_PORT = 42524;	//백신서버포트
	unsigned short SERVER_Update_PORT = 43534;		//백신서버의 업데이트 포트
	static const short FILE_BUFFER_SIZE = 4096;
	LIVEPROTECT* liveprotect = NULL;
	SQLITE* nospearDB;
	bool live_protect_status = false;
	bool NospearOnlineDiagnose(NOSPEAR_FILE& file);
	bool NospearOfflineDiagnose(NOSPEAR_FILE& file);
	std::queue<CString> request_diagnose_queue;
	void InitNospear();
	std::set<CString> office_file_ext_list;
	bool IsOfficeFile(CString ext);
	CString GetNowDate();
public:
	NOSPEAR();
	~NOSPEAR();
	NOSPEAR(std::string ip, unsigned short port1, unsigned short port2);
	bool ActivateLiveProtect(bool status);
	bool Diagnose(NOSPEAR_FILE& file);
	void AutoDiagnose();
	void Notification(CString title, CString body);
	SQLITE* GetSQLitePtr();
	CString GetMsgFromErrCode(short err_code);
	CString GetMsgFromNospear(short nospear);
	unsigned short ReadNospearADS(CString filepath);
	bool WriteNospearADS(CString filepath, unsigned short value);
	bool HasZoneIdentifierADS(CString filepath);
	bool WriteZoneIdentifierADS(CString filepath, CString processName);
	bool DeleteZoneIdentifierADS(CString filepath);
	bool UpdataBlackListDB();
	void AppendDiagnoseQueue(CString filepath);
	void ScanLocalFile(CString rootPath);
	void ScanFileAvailability();
	void AttachADSOther();
	bool IsQueueEmpty();
	void Quarantine(CString filepath);
	void InQuarantine(CString filepath);
	void BackUp(CString filepath);
	void Recovery(CString folderPath);
};
#endif