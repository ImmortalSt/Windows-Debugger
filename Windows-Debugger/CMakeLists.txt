cmake_minimum_required (VERSION 3.8)

add_library(WindowsDebugger SHARED  
	Process/Process.h 
	Process/Porcess.cpp 
	Process/Thread/Thread.h 
	Process/Thread/Thread.cpp 
	Process/Module/Module.h 
	Process/Module/Module.cpp
)
