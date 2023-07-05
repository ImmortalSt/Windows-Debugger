#include "Hooker.h"
#include "Process/Process.h"

int main()
{
	Process* process = new Process(L"a.exe");
	process->SetHardwareBreakpoint(0x333, Process::RW, 4);
	process->SetHardwareBreakpoint(0x334, Process::RW, 4);
	process->SetHardwareBreakpoint(0x335, Process::RW, 4);
	process->DelHardwareBreakpoint(0x334);
	process->SetHardwareBreakpoint(0x335, Process::RW, 4);
	std::string b = process->ReadString(0x00404000);

	auto a = process->GetRegionInformationByAddress(0x00404004);
	process->WriteMemory(0x00404004, 'D');
	std::string c = process->ReadString(0x00404000);
	int bc = 0;
}
