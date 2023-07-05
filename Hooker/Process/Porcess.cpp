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

Process::Process(DWORD processId)
{
	_processId = processId;
    init();
}
Process::Process(std::wstring processName)
{
	_processId = FindProcessIdByName(processName);
    init();
}
Process::~Process()
{
    ClearHardwareBreakpoints();
}

void Process::init() {
    if (_processId == 0) throw std::exception("PID is zero");
    _processEntry = GetPROCESSENTRY32byProcessID(_processId);
    _hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, _processId);
    initThreads();
    initMainThreadId();
}
void Process::initThreads()
{
    HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (hThreadSnapshot == INVALID_HANDLE_VALUE) {
        std::cerr << "Failed to create thread snapshot." << std::endl;
        return;
    }

    THREADENTRY32 te32;
    te32.dwSize = sizeof(THREADENTRY32);
    int aq = GetLastError();
    if (Thread32First(hThreadSnapshot, &te32)) {
        do {
            if (te32.th32OwnerProcessID == _processId) {
                _threads.push_back(te32);
            }
        } while (Thread32Next(hThreadSnapshot, &te32));
    }

    CloseHandle(hThreadSnapshot);
}
void Process::initMainThreadId() {
    ULONGLONG minTime = MAXULONGLONG;
    for each (auto sThread in _threads)
    {
        FILETIME time[4] = { 0 };
        HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, sThread.th32ThreadID);

        GetThreadTimes(hThread, &time[0], &time[1], &time[2], &time[3]);
        ULONGLONG timeCreated = MAKEULONGLONG(time[0].dwLowDateTime, time[0].dwHighDateTime);
        if (timeCreated < minTime) {
            _mainThreadId = sThread.th32ThreadID;
            minTime = timeCreated;
        }
        
        CloseHandle(hThread);
    }
}

DWORD Process::GetPID()
{
    return _processId;
}

DWORD_PTR Process::ReadPointer(DWORD_PTR baseAddres, DWORD_PTR offsets[], size_t lenght) {
    for (int i = 0; i <= lenght - 2; i++) {
        baseAddres = Process::ReadMemory<DWORD_PTR>(baseAddres + offsets[i]);
    }
    return baseAddres + offsets[lenght - 1];
}

std::string Process::ReadString(DWORD_PTR Addres) {
    std::string g = "";
    char a;
    for (int i = 0; (a = Process::ReadMemory<char>(Addres + i)) != '\0'; i++) {
        g += a;
    }
    return g;
}

PMEMORY_BASIC_INFORMATION Process::GetRegionInformationByAddress(DWORD_PTR address)
{
    PMEMORY_BASIC_INFORMATION result = (PMEMORY_BASIC_INFORMATION)new MEMORY_BASIC_INFORMATION();

    VirtualQueryEx(_hProcess, (LPVOID)address, result, 0x100);

    return result;
}

void Process::SetHardwareBreakpoint(DWORD_PTR address, BreakPointCondition condition, int len /* 1, 2, 4 or 8*/, DWORD threadId)
{
    // https://codeby.net/threads/skrytyj-potencial-registrov-otladki-dr0-dr7.74387/

    if (threadId == 0) {
        threadId = _mainThreadId;
    }

    if (len != 1 && len != 2 && len != 4 && len != 8) throw std::exception("Len should be 1, or 2, or 4, or 8");

    CONTEXT context = GetContext(threadId);

    int index = -1;

    if (context.Dr0 == 0) {
        index = 0;
        context.Dr0 = address;
    }
    else if (context.Dr1 == 0) {
        index = 1;
        context.Dr1 = address;
    }
    else if (context.Dr2 == 0) {
        index = 2;
        context.Dr2 = address;
    }
    else if (context.Dr3 == 0) {
        index = 3;
        context.Dr3 = address;
    }
    else throw std::exception("No space for breakpoint");

    byte options = 0;

    if (len == 1) options = 0;
    else if (len == 2) options = 1;
    else if (len == 4) options = 3;
    else if (len == 8) options = 2;

    options = options << 2;

    options = options | condition;

    context.Dr7 = context.Dr7 | options << 16 + (index * 4);
    context.Dr7 = context.Dr7 | 1 << (index * 2);

    context.Dr7 = context.Dr7 & 0xFFFFFFFF;

    HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadId);
    SetThreadContext(hThread, &context);
    CloseHandle(hThread);
}
void Process::DelHardwareBreakpoint(DWORD_PTR address, DWORD threadId)
{
    if (threadId == 0) {
        threadId = _mainThreadId;
    }

    CONTEXT context = GetContext(threadId);

    int index = -1;
    if (context.Dr0 == address) {
        index = 0;
        context.Dr0 = 0;
    }
    else if (context.Dr1 == address) {
        index = 1;
        context.Dr1 = 0;
    }
    else if (context.Dr2 == address) {
        index = 2;
        context.Dr2 = 0;
    }
    else if (context.Dr3 == address) {
        index = 3;
        context.Dr3 = 0;
    }
    else {
        throw std::exception("There is no such breakpoint");
    }

    context.Dr7 = context.Dr7 & ~(0b11ll << index * 2);

    HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadId);
    SetThreadContext(hThread, &context);
    CloseHandle(hThread);
}
void Process::ClearHardwareBreakpoints(DWORD threadId)
{
    if (threadId == 0) {
        threadId = _mainThreadId;
    }

    CONTEXT context = GetContext(threadId);

    context.Dr0 = context.Dr1 = context.Dr2 = context.Dr3 = 0;

    context.Dr7 = context.Dr7 & ~(0xFFll);

    HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadId);
    SetThreadContext(hThread, &context);
    CloseHandle(hThread);

}

CONTEXT Process::GetContext(DWORD threadId)
{
    if (threadId == 0) {
        threadId = _mainThreadId;
    }
    HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, threadId);
    CONTEXT context = {0};
    context.ContextFlags = CONTEXT_ALL;
    GetThreadContext(hThread, &context);
    CloseHandle(hThread);
    return context;
}





/*DWORD GetBaseAddresses(HANDLE processHandle) {

    HMODULE moduleHandles[1024];
    DWORD bytesNeeded;
    if (EnumProcessModules(processHandle, moduleHandles, sizeof(moduleHandles), &bytesNeeded)) {
        DWORD numModules = bytesNeeded / sizeof(HMODULE);
        for (DWORD i = 0; i < numModules; i++) {
            TCHAR moduleName[MAX_PATH];
            if (GetModuleFileNameEx(processHandle, moduleHandles[i], moduleName, MAX_PATH)) {
                MODULEINFO moduleInfo;
                if (GetModuleInformation(processHandle, moduleHandles[i], &moduleInfo, sizeof(moduleInfo))) {
                    return (DWORD)moduleInfo.lpBaseOfDll;
                }
            }
        }
    }
    return 0;
}

THREADENTRY32 GetMainThreadId(DWORD processId) {
    THREADENTRY32 mainThreadId;
    mainThreadId.cntUsage = -1;

    HANDLE threadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (threadSnapshot != INVALID_HANDLE_VALUE) {
        THREADENTRY32 threadEntry = { sizeof(threadEntry) };

        if (Thread32First(threadSnapshot, &threadEntry)) {
            do {

                if (threadEntry.th32OwnerProcessID == processId) {

                    if (mainThreadId.cntUsage == -1) {
                        mainThreadId = threadEntry;
                    }
                }
            } while (Thread32Next(threadSnapshot, &threadEntry));
        }

        CloseHandle(threadSnapshot);
    }

    return mainThreadId;
}
*/
//std::wstring processName = L"a.exe";
    //DWORD Processid = FindProcessIdByName(processName);
    //HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, Processid);
    //CONTEXT Context;
    //Context.ContextFlags = CONTEXT_ALL;
    //GetThreadContext(hProcess, &Context);
    //DWORD BaseAddress = GetBaseAddresses(hProcess);
    //Context.Dr0 = BaseAddress + BPAddresses[0];
    //SetThreadContext(hProcess, &Context);
    //DEBUG_EVENT debugEvent;
    //debugEvent.dwProcessId = Processid;
    //debugEvent.dwThreadId = GetMainThreadId(Processid).th32OwnerProcessID;
    //WaitForDebugEvent(&debugEvent, INFINITE);
    //int a = GetLastError();
    ////DebugActiveProcess(Processid);
    ////DebugActiveProcessStop(Processid)bv
