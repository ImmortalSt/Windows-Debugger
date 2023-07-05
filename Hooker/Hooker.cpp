#include "Hooker.h"
#include "Process/Process.h"

int main()
{
	Process* process = new Process(L"a.exe");
	std::string b = process->ReadString(0x00404000);
	auto a = process->GetRegionInformationByAddress(0x00404004);
	process->WriteMemory(0x00404004, 'D');
	std::string c = process->ReadString(0x00404000);
	int bc = 0;
}
