cmake_minimum_required (VERSION 3.8)

add_library(WindowsDebuggerStatic STATIC  
	"Process/Process.h" 
	"Process/Porcess.cpp" 
	"Process/Thread/Thread.h" 
	"Process/Thread/Thread.cpp" 
	"Process/Module/Module.h"
	"Process/Module/Module.cpp"
)
add_library(WindowsDebuggerShared SHARED  
	"Process/Process.h" 
	"Process/Porcess.cpp" 
	"Process/Thread/Thread.h" 
	"Process/Thread/Thread.cpp" 
	"Process/Module/Module.h"
	"Process/Module/Module.cpp"
)
