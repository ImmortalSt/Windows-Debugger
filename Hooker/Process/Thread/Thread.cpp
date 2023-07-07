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


void dbg::Thread::HardwareDebuggerLoop()
{
    
    DEBUG_EVENT DebugEv;
    
    ZeroMemory(&DebugEv.u, sizeof(DebugEv.u));
    DebugEv.dwThreadId = _threadId;
    DebugEv.dwProcessId = _processId;
    DebugEv.dwDebugEventCode = 0;

    SetHardwareBreakpoint(0x404000 + 0x1564, EXEC, 1, 0);
    WaitForDebugEventEx(&DebugEv, INFINITE);
    for (;;) {
        DoDebugStep();
        printf("%X\n", GetContext().Rip);
    }
    int a = 0;
}
void dbg::Thread::SetHardwareBreakpoint(DWORD_PTR address, BreakPointCondition condition, int len, void(*observer)(CONTEXT))
{
    // https://codeby.net/threads/skrytyj-potencial-registrov-otladki-dr0-dr7.74387/

    if (len != 1 && len != 2 && len != 4 && len != 8) throw std::exception("Len should be 1, or 2, or 4, or 8");

    CONTEXT context = GetContext();

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
    context.Dr7 = context.Dr7 | 0b1 << (index * 2);

    context.Dr7 = context.Dr7 & 0xFFFFFFFF;

    SetContext(context);

    _hardwareBreakpointObservers[index] = observer;
}
void dbg::Thread::DelHardwareBreakpoint(DWORD_PTR address)
{

    CONTEXT context = GetContext();

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

    SetContext(context);

    _hardwareBreakpointObservers[index] = 0;
}
void dbg::Thread::ClearHardwareBreakpoints()
{

    CONTEXT context = GetContext();

    context.Dr0 = context.Dr1 = context.Dr2 = context.Dr3 = 0;

    context.Dr7 = context.Dr7 & ~(0xFFll);

    SetContext(context);

    _hardwareBreakpointObservers[0] = 0;
    _hardwareBreakpointObservers[1] = 0;
    _hardwareBreakpointObservers[2] = 0;
    _hardwareBreakpointObservers[3] = 0;
}

// should be called before WaitForDebugEventEx
void dbg::Thread::DoDebugStep()
{
    CONTEXT context = GetContext();
    context.EFlags |= 0b100000000;
    SetContext(context);
    ContinueDebugEvent(_processId, _threadId, DBG_CONTINUE);

    DEBUG_EVENT DebugEv;

    ZeroMemory(&DebugEv.u, sizeof(DebugEv.u));
    DebugEv.dwThreadId = _threadId;
    DebugEv.dwProcessId = _processId;
    WaitForDebugEventEx(&DebugEv, INFINITE);

    while (DebugEv.u.Exception.ExceptionRecord.ExceptionCode != EXCEPTION_SINGLE_STEP) {
        printf("!!!!!!!!!!!!!!!!!!!!!!!!!%d\n", DebugEv.u.Exception.ExceptionRecord.ExceptionCode);
        ContinueDebugEvent(_processId, _threadId, DBG_CONTINUE); // <---------------------------- НЕ фиксирует SINGLE STEP
        WaitForDebugEventEx(&DebugEv, INFINITE);
    }
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

