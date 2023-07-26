#include "../../../Windows-Debugger/Process/Process.h"

dbg::Process* process;
 

void Observer(CONTEXT context) {
	std::cout << " Printf arg: " << process->ReadString(context.Rcx) << "  Rip: 0x" << std::hex << context.Rip <<'\n';
}


int main() {
	try {
		process = new dbg::Process(L"HelloWorld.exe");
	}
	catch (...) {
		printf("Start HelloWorld.exe");
		exit(1);
	}

	process->EnableDebugging();
	dbg::Thread* mainThread = process->GetMainThread();
	mainThread->SetHardwareBreakpoint(process->GetModuleAddressByName("HelloWorld.exe") + 0x1564, dbg::Thread::EXEC, 1, &Observer);
	mainThread->HardwareDebugLoop();

}