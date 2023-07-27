#include "Thread.h"

dbg::Thread::Thread(THREADENTRY32 te32) {
    _te32 = te32;
	_threadId = te32.th32ThreadID;
	_processId = te32.th32OwnerProcessID;
}
dbg::Thread::~Thread() {
    ClearHardwareBreakpoints();
}
DWORD dbg::Thread::GetThreadId()
{
    return _threadId;
}


void dbg::Thread::HardwareDebugLoop()
{
    DEBUG_EVENT DebugEv;

    ZeroMemory(&DebugEv, sizeof(DebugEv));
    DebugEv.dwThreadId = _threadId;
    DebugEv.dwProcessId = _processId;
    while(true) {
        WaitForDebugEventEx(&DebugEv, INFINITE);

        auto context = GetContext();
        
        int Drx = -1;
        if (context.Rip == context.Dr0) Drx = 0;
        else if (context.Rip == context.Dr1) Drx = 1;
        else if (context.Rip == context.Dr2) Drx = 2;
        else if (context.Rip == context.Dr3) Drx = 3;

        if (Drx != -1) {
            DoDebugStep();
        }

        ContinueDebugEvent(DebugEv.dwProcessId, DebugEv.dwThreadId, DBG_CONTINUE); // There should be DebugEv.dwProcessId and DebugEv.dwThreadId. Not _processId and _threadId.
        
        if (Drx != -1) {
            _hardwareBreakpointObservers[Drx](context);
        }

    }
}
void dbg::Thread::SetHardwareBreakpoint(DWORD_PTR address, HardwareBreakpointCondition condition, int len, ContextFunction observer)
{
    // https://codeby.net/threads/skrytyj-potencial-registrov-otladki-dr0-dr7.74387/

    if (len != 1 && len != 2 && len != 4 && len != 8) throw std::exception("Len should be 1, or 2, or 4, or 8");

    CONTEXT context = GetContext();

    int Drx = -1;

    if (context.Dr0 == 0) {
        Drx = 0;
        context.Dr0 = address;
    }
    else if (context.Dr1 == 0) {
        Drx = 1;
        context.Dr1 = address;
    }
    else if (context.Dr2 == 0) {
        Drx = 2;
        context.Dr2 = address;
    }
    else if (context.Dr3 == 0) {
        Drx = 3;
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

    context.Dr7 = context.Dr7 | options << 16 + (Drx * 4);
    context.Dr7 = context.Dr7 | 0b1 << (Drx * 2);

    context.Dr7 = context.Dr7 & 0xFFFFFFFF;

    SetContext(context);

    _hardwareBreakpointObservers[Drx] = observer;
}
void dbg::Thread::DelHardwareBreakpoint(DWORD_PTR address)
{

    CONTEXT context = GetContext();

    int Drx = -1;
    if (context.Dr0 == address) {
        Drx = 0;
        context.Dr0 = 0;
    }
    else if (context.Dr1 == address) {
        Drx = 1;
        context.Dr1 = 0;
    }
    else if (context.Dr2 == address) {
        Drx = 2;
        context.Dr2 = 0;
    }
    else if (context.Dr3 == address) {
        Drx = 3;
        context.Dr3 = 0;
    }
    else {
        throw std::exception("There is no such breakpoint");
    }

    context.Dr7 = context.Dr7 & ~(0b11ll << Drx * 2); // This is execute automatically

    SetContext(context);

    _hardwareBreakpointObservers[Drx] = 0;
}
void dbg::Thread::ClearHardwareBreakpoints()
{

    CONTEXT context = GetContext();

    context.Dr0 = context.Dr1 = context.Dr2 = context.Dr3 = 0;

    context.Dr7 &= ~(0xFFll);

    SetContext(context);

    _hardwareBreakpointObservers[0] = 0;
    _hardwareBreakpointObservers[1] = 0;
    _hardwareBreakpointObservers[2] = 0;
    _hardwareBreakpointObservers[3] = 0;
}

size_t dbg::Thread::WriteMemory(DWORD address, byte data)
{
    size_t numberOfBytesWritten = 0;
    HANDLE _hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, _processId);

    DWORD oldprotect;
    VirtualProtectEx(_hProcess, (LPVOID)address, sizeof(byte), PAGE_EXECUTE_READWRITE, &oldprotect);
    WriteProcessMemory(_hProcess, (LPVOID)address, &data, sizeof(byte), &numberOfBytesWritten);
    VirtualProtectEx(_hProcess, (LPVOID)address, sizeof(byte), oldprotect, &oldprotect);

    CloseHandle(_hProcess);

    return numberOfBytesWritten;
}

void dbg::Thread::DisableHardwareBreakPoint(int Drx)
{
    auto context = GetContext();
    context.Dr7 &= ~(0b11ll) << (Drx * 2);
    SetContext(context);
}
void dbg::Thread::EnableHardwareBreakPoint(int Drx)
{
    auto context = GetContext();
    context.Dr7 |= (0b11ll) << (Drx * 2);
    SetContext(context);
}
void dbg::Thread::DisableAllHardwareBreakPoint()
{
    auto context = GetContext();
    context.Dr7 &= ~(0xFF);
    SetContext(context);
}
void dbg::Thread::EnableHardwareBreakPoint()
{
    auto context = GetContext();
    context.Dr7 |= 0b01010101;
    SetContext(context);
}


void dbg::Thread::SetSoftwareExecutionBreakpoint(DWORD_PTR address, ContextFunction observer) {
    _softwareExecutionBreakpoints[address] = SoftwareBreakpoint { address, observer, 0 };
}
void dbg::Thread::DelSoftwareExecutionBreakpoint(DWORD_PTR address) {
    _softwareExecutionBreakpoints.erase(address);
}
void dbg::Thread::SoftwareExecutionDebugLoop() {
    HANDLE _hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, _processId);
    for (auto& [address, breakpoint] : _softwareExecutionBreakpoints) {
        PMEMORY_BASIC_INFORMATION segmentInfo = new MEMORY_BASIC_INFORMATION;
        VirtualQueryEx(_hProcess, (LPVOID)address, segmentInfo, 0x100);
        int a = GetLastError();
        if (segmentInfo->Protect == PAGE_EXECUTE_READ) {
            int oldByte = 0;
            size_t numberOfBytesRead;
            ReadProcessMemory(_hProcess, (LPVOID)address, &oldByte, 1, &numberOfBytesRead);
            breakpoint.oldByte = oldByte;

            DWORD oldprotect;
            char data = 0xCC;
            WriteMemory(address, 0xCC);
        }
#ifdef _DEBUG
        else {
            std::cout << "The software breakpoint in the segment is not PAGE_EXECUTE_READ\n";
        }
        delete segmentInfo;
#endif 
    }

    DEBUG_EVENT DebugEv;

    ZeroMemory(&DebugEv, sizeof(DebugEv));
    DebugEv.dwThreadId = _threadId;
    DebugEv.dwProcessId = _processId;


    while (true) {
        ContextFunction futureFunction = 0;

        WaitForDebugEventEx(&DebugEv, INFINITE);
        
        auto context = GetContext();

        if (DebugEv.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            DWORD_PTR rip = context.Rip - 1;
            auto it = _softwareExecutionBreakpoints.find(rip);
            if (it != _softwareExecutionBreakpoints.end()) 
            {                    
                WriteMemory(rip, it->second.oldByte);
                context.Rip -= 1;
                SetContext(context);
                DoDebugStep();
                auto d = GetContext();
                WriteMemory(rip, 0xCC);
                futureFunction = it->second.function;
            }
        }
        ContinueDebugEvent(DebugEv.dwProcessId, DebugEv.dwThreadId, DBG_CONTINUE); // There should be DebugEv.dwProcessId and DebugEv.dwThreadId. Not _processId and _threadId.

        if (futureFunction)
            futureFunction(context);
    }
}

void dbg::Thread::SetMemoryProtectionBreakpoint(DWORD_PTR address, size_t size, ContextFunction observer, MemoryProtectionBreakpointCondition condition)
{
    _memoryProtectionBreakpoints.push_back(MemoryProtectionBreakpoint {address, size, condition, observer, 0});
}
void dbg::Thread::DelMemoryProtectionBreakpoint(DWORD_PTR address)
{
    for (std::list<MemoryProtectionBreakpoint>::iterator it = _memoryProtectionBreakpoints.begin(); it != _memoryProtectionBreakpoints.end(); it++) {
        if (it->address == address) {
            _memoryProtectionBreakpoints.erase(it);
            break;
        }
    }
}
void dbg::Thread::MemoryProtectionDebugLoop()
{
    // #define PAGE_NOACCESS           0x01    
    // #define PAGE_READONLY           0x02    
    // #define PAGE_READWRITE          0x04    
    // #define PAGE_WRITECOPY          0x08    
    // #define PAGE_EXECUTE            0x10    
    // #define PAGE_EXECUTE_READ       0x20    
    // #define PAGE_EXECUTE_READWRITE  0x40    
    // #define PAGE_EXECUTE_WRITECOPY  0x80    
    // #define PAGE_GUARD             0x100    
    // #define PAGE_NOCACHE           0x200    
    // #define PAGE_WRITECOMBINE      0x400   
    HANDLE _hProcess = OpenProcess(PROCESS_ALL_ACCESS, false, _processId);
    for (auto& bp : _memoryProtectionBreakpoints) {
        DWORD newCondition = 0;

        if (bp.condition == MemoryProtectionBreakpointCondition::EXECUTE) {
            newCondition = PAGE_READWRITE;
        }

        VirtualProtectEx(_hProcess, (LPVOID)bp.address, bp.size, newCondition, &bp.oldProtection);


    }
    CloseHandle(_hProcess);

    DEBUG_EVENT DebugEv;

    ZeroMemory(&DebugEv, sizeof(DebugEv));
    DebugEv.dwThreadId = _threadId;
    DebugEv.dwProcessId = _processId;

    while (true) {
        ContextFunction futureFunction = 0;

        WaitForDebugEventEx(&DebugEv, INFINITE);

        auto context = GetContext();

        if (DebugEv.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_ACCESS_VIOLATION) {
            DWORD_PTR ExceptionAddress = (DWORD_PTR) DebugEv.u.Exception.ExceptionRecord.ExceptionAddress;
            for (auto& bp : _memoryProtectionBreakpoints) {
                if (ExceptionAddress >= bp.address && ExceptionAddress <= bp.address + bp.size) {
                    futureFunction = bp.function;
                    break;
                }
            }
        }


        ContinueDebugEvent(DebugEv.dwProcessId, DebugEv.dwThreadId, DBG_CONTINUE); // There should be DebugEv.dwProcessId and DebugEv.dwThreadId. Not _processId and _threadId.

        if (futureFunction)
            futureFunction(context);

    }

}

// should be called between WaitForDebugEventEx and ContinueDebugEvent
void dbg::Thread::DoDebugStep() 
{
    DEBUG_EVENT DebugEv;

    auto oldContext = GetContext();
    DisableAllHardwareBreakPoint();

    ZeroMemory(&DebugEv.u, sizeof(DebugEv.u));
    DebugEv.dwThreadId = _threadId;
    DebugEv.dwProcessId = _processId;

    while (true) {
        CONTEXT context = GetContext();
        context.EFlags |= 0b100000000;
        SetContext(context);

        ContinueDebugEvent(DebugEv.dwProcessId, DebugEv.dwThreadId, DBG_CONTINUE);
        WaitForDebugEventEx(&DebugEv, INFINITE);
        if (DebugEv.u.Exception.ExceptionRecord.ExceptionCode == 0x80000004) break;
    }

    auto context = GetContext();

    context.Dr0 = oldContext.Dr0;
    context.Dr1 = oldContext.Dr1;
    context.Dr2 = oldContext.Dr2;
    context.Dr3 = oldContext.Dr3;
    context.Dr7 = oldContext.Dr7;

    SetContext(context);
}


CONTEXT dbg::Thread::GetContext()
{
    HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, _threadId);
    CONTEXT context = { 0 };
    context.ContextFlags = CONTEXT_ALL;
    GetThreadContext(hThread, &context);
    CloseHandle(hThread);
    return context;
}
void dbg::Thread::SetContext(CONTEXT context)
{
    HANDLE hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, _threadId);
    SetThreadContext(hThread, &context);
    CloseHandle(hThread);
}