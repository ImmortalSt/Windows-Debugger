#ifndef _THREAD_
#define _THREAD_
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>

namespace dbg {

	class Thread {
	private:
		void (*_hardwareBreakpointObservers[4])(CONTEXT) = { 0 };

		THREADENTRY32 _te32;
		DWORD _threadId = 0;
		DWORD _processId = 0;
	public:

		enum BreakPointCondition { EXEC = 0, WO = 1, PORT = 2, RW = 3 };
		Thread(THREADENTRY32 te32);
		~Thread();

		void HardwareDebuggerLoop();
		void SetHardwareBreakpoint(DWORD_PTR address, BreakPointCondition condition, int len /* 1, 2, 4 or 8*/, void (*observer)(CONTEXT));
		void DelHardwareBreakpoint(DWORD_PTR address);
		void ClearHardwareBreakpoints();

		void DoDebugStep();

		CONTEXT GetContext();
		void SetContext(CONTEXT context);

		DWORD GetThreadId();
	};

}

#endif // _THREAD_
