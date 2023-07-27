#include "../../../Windows-Debugger/Process/Process.h"

dbg::Process* process;
 

void Observer(CONTEXT context) {
	std::cout << " Execute:  0x" << std::hex << context.Rip <<'\n';
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
	mainThread->SetMemoryProtectionBreakpoint(process->GetModuleAddressByName("HelloWorld.exe") + 0x1564, 100, &Observer, dbg::Thread::EXECUTE);
	mainThread->MemoryProtectionDebugLoop();

}