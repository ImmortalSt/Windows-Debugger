﻿#include "Hooker.h"
#include "Process/Process.h"

void Observer(CONTEXT context) {
	printf("%X\n", context.Rip);
}

int main()
{
	Process* process = new Process(L"a.exe");
	
	//process->SetHardwareBreakpoint(process->GetModuleAddressByName("Counter.exe") + 0x1044, Process::EXEC, 1, &Observer);
	process->HardwareDebuggerLoop();
	
	std::string b = process->ReadString(0x00404000);

	auto a = process->GetRegionInformationByAddress(0x00404004);
	process->WriteMemory(0x00404004, 'D');
	std::string c = process->ReadString(0x00404000);
	int bc = 0;
}
