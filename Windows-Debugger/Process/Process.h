#ifndef _PROCESS_
#define _PROCESS_
#include <Windows.h>
#include <string>
#include <iostream>
#include <tlhelp32.h>
#include <codecvt>
#include <psapi.h>
#include <list>
#include <winternl.h>
#include <map>
#include "Thread/Thread.h"
#include "Module/Module.h"

#define MAXIMUM_MODULES		0x100

namespace dbg {
	/*
	INTERESTING FUNCTIONS:
		1) PHYSICAL_ADDRESS d = MmGetPhysicalAddress(GetModuleAddressByName("a.exe"));
		2) KeInitializeEvent (wdm.h) (kernel)
	*/

	class Process {
	private:
		
		DWORD _processId = 0;
		PROCESSENTRY32 _processEntry = { 0 };
		HANDLE _hProcess = 0;

		std::list<Module*> _modules;
		std::list<Thread*> _threads;
		Thread* _mainThread = 0;


		bool _isDebugging = 0;

		void init();
		void initThreads();
		void initMainThreadId();
		void initModules();

	public:

		Process(DWORD processId);
		Process(std::wstring processName);
		~Process();

		template<typename T> size_t WriteMemory(DWORD address, T data)
		{
			size_t numberOfBytesWritten = 0;

			DWORD oldprotect;
			VirtualProtectEx(_hProcess, (LPVOID)address, sizeof(T), PAGE_EXECUTE_READWRITE, &oldprotect);
			WriteProcessMemory(_hProcess, (LPVOID)address, &data, sizeof(T), &numberOfBytesWritten);
			VirtualProtectEx(_hProcess, (LPVOID)address, sizeof(T), oldprotect, &oldprotect);


			return numberOfBytesWritten;
		}
		template<typename T> T ReadMemory(DWORD address)
		{
			T result = 0;
			size_t numberOfBytesRead;
			ReadProcessMemory(_hProcess, (LPVOID)address, &result, sizeof(T), &numberOfBytesRead);

			return result;
		}

		DWORD GetPID();

		DWORD64 ReadPointer(DWORD_PTR baseAddres, DWORD_PTR offsets[], size_t lenght);
		std::string ReadString(DWORD_PTR address);
		MEMORY_BASIC_INFORMATION GetRegionInformationByAddress(DWORD_PTR address);
		DWORD_PTR GetModuleAddressByName(std::string name);
		Thread* GetMainThread();
		
		void EnableDebugging();
		void DisableDebugging();
	};
}

#endif /* _PROCESS_ */
