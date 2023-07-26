#include "Process.h"

#define MAKEULONGLONG(ldw, hdw) ((ULONGLONG(hdw) << 32) | ((ldw) & 0xFFFFFFFF))
#define MAXULONGLONG (~((ULONGLONG)0))

static DWORD FindProcessIdByName(const std::wstring& name)
{
    DWORD pid = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32 process;
    ZeroMemory(&process, sizeof(PROCESSENTRY32));
    process.dwSize = sizeof(PROCESSENTRY32);

    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    if (Process32First(snapshot, &process)) {
        do {
            std::wstring pname(converter.from_bytes(process.szExeFile));
            if (pname == name) {
                pid = process.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &process));
    }

    CloseHandle(snapshot);

    return pid;
}
static PROCESSENTRY32 GetPROCESSENTRY32byProcessID(DWORD _processId) {
    PROCESSENTRY32 processEntry = { 0 };
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

    processEntry.dwSize = sizeof(processEntry);
    BOOL found = Process32First(snapshot, &processEntry);

    while (found) {
        if (processEntry.th32ProcessID == _processId) {
            return processEntry;
        }
        found = Process32Next(snapshot, &processEntry);
    }
    return PROCESSENTRY32();
}

dbg::Process::Process(DWORD processId)
{
	_processId = processId;
    init();
}
dbg::Process::Process(std::wstring processName)
{
	_processId = FindProcessIdByName(processName);
    init();
}
dbg::Process::~Process()
{
    CloseHandle(_hProcess);
}

void dbg::Process::init() {
    if (_processId == 0) throw std::exception("PID is zero");
    _processEntry = GetPROCESSENTRY32byProcessID(_processId);
    _hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, _processId);

    initThreads();
    initMainThreadId();
    initModules();
}
void dbg::Process::initThreads()
{
    HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create thread snapshot." << std::endl;
        return;
    }

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);

    if (Thread32First(hThreadSnapshot, &te32)) {
        do {
            if (te32.th32OwnerProcessID == _processId) {
                _threads.push_back(new dbg::Thread(te32));
            }
        } while (Thread32Next(hThreadSnapshot, &te32));
    }

    CloseHandle(hThreadSnapshot);
}
void dbg::Process::initMainThreadId() {
    ULONGLONG minTime = MAXULONGLONG;
    for (auto sThread : _threads)
    {
        FILETIME time[4] = { 0 };
        HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, sThread->GetThreadId());

        GetThreadTimes(hThread, &time[0], &time[1], &time[2], &time[3]);
        ULONGLONG timeCreated = MAKEULONGLONG(time[0].dwLowDateTime, time[0].dwHighDateTime);
        if (timeCreated < minTime) {
            _mainThread = sThread;
            minTime = timeCreated;
        }

        CloseHandle(hThread);
    }
}
void dbg::Process::initModules()
{
    HANDLE moduleSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, _processId);
    MODULEENTRY32 lpme;
    lpme.dwSize = sizeof(MODULEENTRY32);
    if (Module32First(moduleSnapshot, &lpme)) {
        do {
            if (lpme.th32ProcessID == _processId) {
                _modules.push_back(new Module(lpme));
            }
        } while (Module32Next(moduleSnapshot, &lpme));
    }
}

void dbg::Process::EnableDebugging()
{
    if (_isDebugging) return;

    DebugActiveProcess(_processId);
    _isDebugging = true;
}
void dbg::Process::DisableDebugging()
{
    if (!_isDebugging) return;
    DebugActiveProcessStop(_processId);
    _isDebugging = false;
}

DWORD dbg::Process::GetPID()
{
    return _processId;
}

DWORD_PTR dbg::Process::ReadPointer(DWORD_PTR baseAddres, DWORD_PTR offsets[], size_t lenght) {
    for (int i = 0; i <= lenght - 2; i++) {
        baseAddres = Process::ReadMemory<DWORD_PTR>(baseAddres + offsets[i]);
    }
    return baseAddres + offsets[lenght - 1];
}
std::string dbg::Process::ReadString(DWORD_PTR address) {
    std::string result = "";
    char tempChar;
    for (int i = 0; (tempChar = Process::ReadMemory<char>(address + i)) != '\0'; i++) {
        result += tempChar;
    }
    return result;
}
MEMORY_BASIC_INFORMATION dbg::Process::GetRegionInformationByAddress(DWORD_PTR address)
{
    MEMORY_BASIC_INFORMATION result;

    VirtualQueryEx(_hProcess, (LPVOID)address, &result, 0x100);

    return result;
}
DWORD_PTR dbg::Process::GetModuleAddressByName(std::string name)
{

    for (auto module : _modules)
    {
        if (strcmp(module->GetName(), name.c_str()) == 0) {
            return module->GetBaseAddress();
        }
    }

    return 0;
}

dbg::Thread* dbg::Process::GetMainThread()
{
    return _mainThread;
}