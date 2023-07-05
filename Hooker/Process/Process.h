
#include <Windows.h>
#include <string>
#include <iostream>
#include <tlhelp32.h>
#include <codecvt>
#include <psapi.h>
#include <list>
#include <winternl.h>


class Process {
private:
	DWORD _processId = 0;
	PROCESSENTRY32 _process = {0};
	HANDLE _hProcess = 0;
	std::list<THREADENTRY32> _threads;

	void init();
	void initThreads();
	void initProcessInformation();
	void initMainThread();
public:
	Process(DWORD processId);
	Process(std::wstring processName);
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
	
	DWORD64 ReadPointer(DWORD_PTR baseAddres, DWORD_PTR offsets[], size_t lenght);
	std::string ReadString(DWORD_PTR Addres);
	PMEMORY_BASIC_INFORMATION GetRegionInformationByAddress(DWORD_PTR address);
};