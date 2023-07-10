#include "Hooker.h"
#include "Process/Process.h"

void Observer(CONTEXT context) {
	printf("%d\n", context.Rsi);
}

int main()
{
	dbg::Process* process = new dbg::Process(L"Counter.exe");
	
	process->EnableDebugging();
	process->GetMainThread()->SetHardwareBreakpoint(process->GetModuleAddressByName("Counter.exe") + 0x1052, dbg::Thread::EXEC, 1, &Observer);
	process->GetMainThread()->HardwareDebuggerLoop();
	
	std::string b = process->ReadString(0x00404000);

	auto a = process->GetRegionInformationByAddress(0x00404004);
	process->WriteMemory(0x00404004, 'D');
	std::string c = process->ReadString(0x00404000);
	int bc = 0;
}
