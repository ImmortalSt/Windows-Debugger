#include <Windows.h>
#include <string>
#include <iostream>
#include <tlhelp32.h>
#include <codecvt>
#include <psapi.h>
#include <list>
#include <winternl.h>
#include <map>

#define MAXIMUM_MODULES		0x100


/*
INTERESTING FUNCTIONS:
	1) PHYSICAL_ADDRESS d = MmGetPhysicalAddress(GetModuleAddressByName("a.exe"));
	2) KeInitializeEvent (wdm.h) (kernel)
*/


class Process {
private:
	void (*_hardwareObservers[4])(CONTEXT) = {0};
	DWORD _processId = 0;
	DWORD _mainThreadId = 0;
	PROCESSENTRY32 _processEntry = {0};
	HANDLE _hProcess = 0;

	std::list<MODULEENTRY32> _modules;
	std::list<THREADENTRY32> _threads;

	void init();
	void initThreads();
	void initMainThreadId();
	void initModules();

	void SetContext(CONTEXT context, DWORD threadId = 0);
public:
	void HardwareDebuggerLoop(DWORD threadId = 0);
	enum BreakPointCondition { EXEC = 0, WO = 1, PORT = 2, RW = 3 };

	Process(DWORD processId);
	Process(std::wstring processName);
	~Process();

	template<typename T> size_t WriteMemory(DWORD address, T data)
	{
		size_t numberOfBytesWritten = 0;

		DWORD oldprotect;
		VirtualProtectEx(_hProcess, (LPVOID)address, sizeof(T), PAGE_WRITECOPY, &oldprotect);
		WriteProcessMemory(_hProcess, (LPVOID)address, &data, sizeof(T), &numberOfBytesWritten);
		VirtualProtectEx(_hProcess, (LPVOID)address, sizeof(T), oldprotect, &oldprotect);

		if (GetLastError() != 0) {
			std::cout << "WriteMemory Error with error: " << GetLastError() << "\n";
			throw std::exception("WriteMemory Error with error: " + GetLastError());
		}
		return numberOfBytesWritten;
	}
	template<typename T> T ReadMemory(DWORD address)
	{
		T result;
		size_t numberOfBytesRead;
		ReadProcessMemory(_hProcess, (LPVOID)address, &result, sizeof(T), &numberOfBytesRead);
		if (GetLastError() != 0) {
			std::cout << "WriteMemory Error with error: " << GetLastError() << "\n";
			throw std::exception("ReadMemory Error with error: " + GetLastError());
		}
		return result;
	}

	DWORD GetPID();
	
	DWORD64 ReadPointer(DWORD_PTR baseAddres, DWORD_PTR offsets[], size_t lenght);
	std::string ReadString(DWORD_PTR address);
	PMEMORY_BASIC_INFORMATION GetRegionInformationByAddress(DWORD_PTR address);
	DWORD_PTR GetModuleAddressByName(std::string name);

	CONTEXT GetContext(DWORD ThreadId = 0);

	void SetHardwareBreakpoint(DWORD_PTR address, BreakPointCondition condition, int len /* 1, 2, 4 or 8*/, void (*observer)(CONTEXT), DWORD threadId = 0);
	void DelHardwareBreakpoint(DWORD_PTR address, DWORD threadId = 0);
	void ClearHardwareBreakpoints(DWORD threadId = 0);
};