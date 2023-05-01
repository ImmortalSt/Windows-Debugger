#include "Hooker.h"
#include "Process/Process.h"

int main()
{
	Process* process = new Process(L"a.exe");
	auto b = process->ReadMemory<char>(0x0061E200);
	process->WriteMemory(0x0061E200, 'Q');
	auto a = process->ReadMemory<char>(0x0061E200);
}
