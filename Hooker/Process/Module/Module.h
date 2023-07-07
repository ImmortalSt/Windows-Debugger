#ifndef _MODULE_
#define _MODULE_
#include <string>
#include <Windows.h>
#include <TlHelp32.h>

namespace dbg {
	class Module {
	private:
		MODULEENTRY32 _lpme;
	public:
		Module(MODULEENTRY32 lpme);

		char* GetName();
		DWORD_PTR GetBaseAddress();
	};
}

#endif // _MODULE_
