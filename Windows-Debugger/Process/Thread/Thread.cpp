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

    ZeroMemory(&DebugEv, sizeof(DebugEv));
    DebugEv.dwThreadId = _threadId;
    DebugEv.dwProcessId = _processId;
    for (;;) {
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
void dbg::Thread::SetHardwareBreakpoint(DWORD_PTR address, BreakPointCondition condition, int len, void(*observer)(CONTEXT))
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

    context.Dr7 = context.Dr7 & ~(0xFFll);

    SetContext(context);

    _hardwareBreakpointObservers[0] = 0;
    _hardwareBreakpointObservers[1] = 0;
    _hardwareBreakpointObservers[2] = 0;
    _hardwareBreakpointObservers[3] = 0;
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

