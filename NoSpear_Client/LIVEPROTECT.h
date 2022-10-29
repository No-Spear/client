#include "SQLITE.h"

#define SCANNER_DEFAULT_REQUEST_COUNT       5
#define SCANNER_DEFAULT_THREAD_COUNT        2
#define SCANNER_MAX_THREAD_COUNT            64
using namespace std;
typedef struct _SCANNER_THREAD_CONTEXT {
    HANDLE Port;
    HANDLE Completion;
} SCANNER_THREAD_CONTEXT, * PSCANNER_THREAD_CONTEXT;

typedef struct _SCANNER_MESSAGE {
    FILTER_MESSAGE_HEADER MessageHeader;
    SCANNER_NOTIFICATION Notification;
    OVERLAPPED Ovlp;
} SCANNER_MESSAGE, * PSCANNER_MESSAGE;

typedef struct _SCANNER_REPLY_MESSAGE {
    FILTER_REPLY_HEADER ReplyHeader;
    SCANNER_REPLY Reply;
} SCANNER_REPLY_MESSAGE, * PSCANNER_REPLY_MESSAGE;

static bool threadstatus = false;
static std::set<CString> createlist;
static std::map<CString, CString> fileToProcess;
static SQLITE historyDB;

class LIVEPROTECT{
    const DWORD requestCount = SCANNER_DEFAULT_REQUEST_COUNT;
    const DWORD threadCount = SCANNER_DEFAULT_THREAD_COUNT;
    HANDLE threads[SCANNER_MAX_THREAD_COUNT];
    SCANNER_THREAD_CONTEXT context;
    PSCANNER_MESSAGE msg;
    static unsigned short ReadNospearADS(CString filepath);
    static bool WriteNospearADS(CString filepath, unsigned short value);
    static bool WriteZoneIdentifierADS(CString filepath, CString processName);
    static PWCHAR GetCharPointerW(PWCHAR pwStr, WCHAR wLetter, int Count);
    static DWORD ScannerWorker(PSCANNER_THREAD_CONTEXT Context);
    static CString GetProcessName(unsigned long pid);
    static bool IsOfficeProgram(unsigned long pid);

    HANDLE port, completion;
    //�������α׷� �迭 �ʿ���
    //ĳ�� �迭 �ʿ���(ADS:Block ������, ����ϰ� �;�)

public:
    LIVEPROTECT();
    ~LIVEPROTECT();
    int ActivateLiveProtect();
    int InActivateLiveProtect();
};