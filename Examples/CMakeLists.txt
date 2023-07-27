
add_executable(CouterExampleHardware "HardwareBreakpoint/Counter/main.cpp")
target_link_libraries(CouterExampleHardware PUBLIC  WindowsDebuggerStatic)

add_executable(RIPExampleHardware "HardwareBreakpoint/RIP print/main.cpp")
target_link_libraries(RIPExampleHardware PUBLIC  WindowsDebuggerStatic)

add_executable(CouterExampleSoftware "SoftwareBreakpoint/Counter/main.cpp")
target_link_libraries(CouterExampleSoftware PUBLIC  WindowsDebuggerStatic)

add_executable(RIPExampleSoftware "SoftwareBreakpoint/RIP print/main.cpp")
target_link_libraries(RIPExampleSoftware PUBLIC  WindowsDebuggerStatic)

add_executable(RIPExampleMemoryProtection "MemoryProtectionBreakpoint/RIP print/main.cpp")
target_link_libraries(RIPExampleMemoryProtection PUBLIC  WindowsDebuggerStatic)
