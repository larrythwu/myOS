<img src="https://cdn-60c35131c1ac185aa47dd21e.closte.com//wp-content/uploads/2019/05/terminal-app-icon.png" alt="drawing" style="width:150px;"/>

# myOS

A bootable multi-threaded kernel built from ground up, can execute C programs in user space and trap into protected mode kernel through system calls. 

## Description

Our kernel program can be abstracted into three layers:

**1. Bootloader (Real Mode)**

Resids in the first sector of our bootable disk, the very first code loaded into the memory.
- Read the actual kernel from the disk into the memory 
- Set up IDT and GDT table 
- Enable A20 Line 
- Switch to protected mode and execute the kernel


**2. The Kernel (Protected Mode)**

Here we implement the bread and butter of our OS. 
- Initialize the GDT 
- Implemented functions to handle IO to ports and heap memory allocation
- Load our interrupt discriptor table, and implement system call handlers 
- Initialize our FAT16 file system
- Enbale page mapping 
- Create construct to represent a process, and create functions to allow context switching in order to achieve multi-threading
- Finally, it kick off our very first user program - the shell terminal


**3. User Programs (User Space)**

Here we implement the standard libarary which will do priviledged operations through system call to the kernels. 
Also we can write and compile our C programs and mount it into our filesystem, so they can be executed from our shell.

## Dependencies
Pleaes follow OS Dev's guide on how to set up a cross compiler 
https://wiki.osdev.org/GCC_Cross-Compiler

The simulation is done using QEMU
```
qemu-system-x86_64 --hda ./bin/os.bin
```
The debugging tool I used is GDB, to launch QEMU simulation inside GDB, run the following in GDB
```
target remote | qemu-system-x86_64 -S -gdb stdio -hda ./bin/os.bin
```

## Executing program
Under the root directory of the project, run ./build.sh, which will set up all the env paths and run the Makefiles. The compiled files will be in the /build folder.
## Acknowledgments
Inspired by the series of lectures created by Daniel McCarthy 

He post all of his lectures on this website https://dragonzap.com/
