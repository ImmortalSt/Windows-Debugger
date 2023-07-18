#include "Module.h"

dbg::Module::Module(MODULEENTRY32 lpme)
{
	_lpme = lpme;
}

char* dbg::Module::GetName()
{
	return _lpme.szModule;
}

DWORD_PTR dbg::Module::GetBaseAddress()
{
	return (DWORD_PTR)_lpme.modBaseAddr;
}
