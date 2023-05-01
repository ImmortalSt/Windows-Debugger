#include <Windows.h>
#include <string>
#include <iostream>
#include <tlhelp32.h>
#include <codecvt>
#include <psapi.h>


class Process {
private:
	DWORD _processId = 0;
	PROCESSENTRY32 _process = {0};
	HANDLE _hProcess = 0;
	
	void init();
public:
	Process(DWORD processId);
	Process(std::wstring processName);
	template<typename T> size_t WriteMemory(DWORD address, T data)
	{
		size_t numberOfBytesWritten = 0;
		WriteProcessMemory(_hProcess, (LPVOID)address, &data, sizeof(T), &numberOfBytesWritten);
		return numberOfBytesWritten;
	}

	template<typename T> T ReadMemory(DWORD address)
	{
		T result;
		size_t numberOfBytesRead;
		ReadProcessMemory(_hProcess, (LPVOID)address, &result, sizeof(T), &numberOfBytesRead);
		return result;
	}
};