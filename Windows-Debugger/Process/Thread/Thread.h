#ifndef _THREAD_
#define _THREAD_
#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>
#include <list>
#include <map>

namespace dbg {
	typedef void (*ContextFunction)(CONTEXT);
	
	class Thread {
	public:
		enum HardwareBreakpointCondition { EXEC = 0, WO = 1, PORT = 2, RW = 3 };
		enum MemoryProtectionBreakpointCondition { EXECUTE = 1 }; //, WRITE = 3, READWRITE = 4 };

		Thread(THREADENTRY32 te32);
		~Thread();
		
		void HardwareDebugLoop();
		void SetHardwareBreakpoint(DWORD_PTR address, HardwareBreakpointCondition condition, int len /* 1, 2, 4 or 8*/, ContextFunction observer);
		void DelHardwareBreakpoint(DWORD_PTR address);
		void ClearHardwareBreakpoints();

		void SetSoftwareExecutionBreakpoint(DWORD_PTR address, ContextFunction observer);
		void DelSoftwareExecutionBreakpoint(DWORD_PTR address);
		void SoftwareExecutionDebugLoop();

		void SetMemoryProtectionBreakpoint(DWORD_PTR address, size_t size, ContextFunction observer, MemoryProtectionBreakpointCondition condition);
		void DelMemoryProtectionBreakpoint(DWORD_PTR address);
		void MemoryProtectionDebugLoop();

		DWORD GetThreadId();

	private:
		struct SoftwareBreakpoint {
			DWORD_PTR address;
			ContextFunction function;
			byte oldByte;
		};

		struct MemoryProtectionBreakpoint {
			DWORD_PTR address;
			size_t size;
			MemoryProtectionBreakpointCondition condition;
			ContextFunction function;
			DWORD oldProtection;
		};

		void (*_hardwareBreakpointObservers[4])(CONTEXT) = { 0 };
		std::map<DWORD_PTR, SoftwareBreakpoint> _softwareExecutionBreakpoints;
		std::list<MemoryProtectionBreakpoint> _memoryProtectionBreakpoints;

		THREADENTRY32 _te32;
		DWORD _threadId = 0;
		DWORD _processId = 0;
		
		void DisableHardwareBreakPoint(int Drx);
		void EnableHardwareBreakPoint(int Drx);
		void DisableAllHardwareBreakPoint();
		void EnableHardwareBreakPoint();

		CONTEXT GetContext();
		void SetContext(CONTEXT context);

		void DoDebugStep();
		size_t WriteMemory(DWORD address, byte data);
	};

}

#endif // _THREAD_
