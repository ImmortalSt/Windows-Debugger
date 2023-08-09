# External Process Debugger for Windows

###### _It is project for education._
###### The architecture of this project is worse than for Linux
\
The project serves as a library for debugging processes, supporting three types of breakpoints:
- Hardware breakpoint
- Software breakpoint
- Memory Protection breakpoint


### Hardware breakpoint

Using the Drx registers in the process (Dr0, Dr1, Dr2, Dr3, Dr6, Dr7), these breakpoints offer support for read, write, execute, and COM method. When a breakpoint is triggered, a debugging step is performed. This is necessary so that the breakpoint call does not loop

##### DR0, DR1, DR2, DR3 (Debug Registers 0-3):
These registers are used to set normal breakpoints. Each of them allows you to set the address in memory to which the processor will stop when it executes an instruction at this address. When the processor reaches the specified address, it generates a debug exception.

#####  DR6 (Debug Control Register 6):
DR6 is a special register that contains the status of debugging events. When a breakpoint or conditional breakpoint is activated (i.e. triggered), the bits in the DR6 register are set to indicate the type of event that caused the stop.

#####  DR7 (Debug Control Register 7):
DR7 is a register that controls the conditions and activation of breakpoints. In DR7, conditions are set (for example, reading, writing, execution) and activation of breakpoints for each of the registers DR0-DR3. DR7 is also used to control conditional breakpoints.

#### Be careful
When using the READ/WRITE mode, make sure that you have specified the correct address.
For example:
```asm
0x40155D                 lea     rcx, 0x404000     ; "Hello, world!"
0x401564                 call    puts
```
If you write the address 0x404000, the breakpoint will work inside the puts function

### Software breakpoint
This breakpoint only supports the execute method. It overwrites the process memory at the specified location with the INT3(0xCC) instruction, which throws an exception. When the breakpoint is triggered, the old instruction is rewritten into place, the debugging step is performed and the INT3 instruction is returned to place. Thus, the operation of the technological process is not disrupted

### Memory Protection breakpoint
This breakpoint only supports execute method. In theory, it can support the READ, WRITE, READ/WRITE method. It sets the protect flag of the specified memory area to the PAGE_READONLY state, which throws an EXCEPTION_ACCESS_VIOLATION exception when calling instructions
