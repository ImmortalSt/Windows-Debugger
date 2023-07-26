#include "../../../Windows-Debugger/Process/Process.h"

void Observer(CONTEXT context) {
	printf("%d\n", context.Rsi);
}

int main() {
	dbg::Process* process;
	try {
		process = new dbg::Process(L"Counter.exe");
	}
	catch (...) {
		printf("Start Counter.exe");
		exit(1);
	}
	
	process->EnableDebugging();
	dbg::Thread* mainThread = process->GetMainThread();
	mainThread->SetHardwareBreakpoint(process->GetModuleAddressByName("Counter.exe") + 0x1049, dbg::Thread::EXEC, 1, &Observer);
	mainThread->HardwareDebugLoop();
}