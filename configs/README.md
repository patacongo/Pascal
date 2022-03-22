# Example NuttX Configuration Files

These are variable NuttX configuration files that were used for testing.

## defconfig.sim-hostfs

### Setup:

- Retains Pascal executables in a HostFS file system that links to the pascal/papps/image directory.  The path of CONFIG_PASCAL_MOUNT_OPTIONS in the deconfig.sim-hostfs file is an absolute path and, hence, will need to be adapted for your environment.

### Usage:

1. Make sure that the Pascal directory is clean

    make distclean

2. Link the Pascal directory into the apps directory

    cd <nuttx-apps-directory>
    ln -s <pascal-directory> pascal

3. Configure the NuttX directory

    cd <nuttx-directory>
    tools/configure.sh [OPTIONS] sim:nsh
    cp <pascal-directory>/configs/defconfig.sim-hostfs .config
    make olddefconfig

4. Change the CONFIG_PASCAL_MOUNT_OPTIONS path

    make menuconfig

5. And build

    make

### Comments/Status:

- I am seeing instability.  If you just run the simulator, it will fail with a segment violation.

    $ ./nuttx.exe
    Segmentation fault (core dumped)

- But if I use GDB and single step, I can see that most of the code is running fine and that the segment violation happens in `closedir()` which is passed a bad DIR pointer.
- If I run with the Pcode debugger (i.e., with `CONFIG_PASCAL_STARTUP_DEBUG=y`) then it runs close to normally:  The Pshell prompt appears.  There is normal output to `stdout` from Pascal *WRITELN*.  Input is received okay, but there is missing echo of the debugger character input on the console:

    (gdb) b pstart_main
    Breakpoint 1 at 0x10042c7cc: file pstart_main.c, line 78.
    (gdb) r

    Thread 1 "nuttx" hit Breakpoint 1, pstart_main (argc=1, argv=0x6ffffc0231e0,
        envp=0x10042c9c0 <pstart_main>) at pstart_main.c:78
    
    (gdb) c
    Continuing.
    /bin/Pshell.pex Loaded
    Commands:
      RE[set]   - Reset
      RU[n]     - Run
      S[tep]    - Single Step (Into)
      N[ext]    - Single Step (Over)
      G[o]      - Go
      BS xxxx   - Set Breakpoint
      BC n      - Clear Breakpoint
      WS xxxx   - [Re]set Watchpoint
      WF xxxx   - Level 0 Frame Watchpoint
      WC        - Clear Watchpoint
      DP        - Display Program Status
      DT        - Display Program Trace
      DS [xxxx] - Display Stack
      DI [xxxx] - Display Instructions
      DB        - Display Breakpoints
      H or ?    - Shows this list
      Q[uit]    - Quit
    PC:025e  b4..0034          INDS  52
    SP:0628  0000
       0626  0000
    FP:061e CSP:0000
    PC:0261  7400....          PUSHB 0
    SP:065c  0000
       065a  0000
    FP:061e CSP:0000
    CMD:
    PC:0263  a4..0000          ST    0
    SP:065e  0000
       065c  0000
    FP:061e CSP:0000
    CMD:
    PC:0266  7401....          PUSHB 1
    SP:065c  0000
       065a  0000
    FP:061e CSP:0000
    CMD:

- If I continue from the P-Code debug, some commands work, like H[elp].  But the same segment violation in `closedir()` occurs when the L[ist] command is entered:

    CMD: g
    psh> h
    PShell Commands:
      H[elp]
        Show this text
      L[ist]
        Show Pascal Executables
      R[un] <PexFileName>
        Run a Pascal Executable
      D[bg] <PexFileName>
        Debug a Pascal Executable
      Q[uit]
        Exit
    psh> l
    
    Thread 1 "nuttx" received signal SIGSEGV, Segmentation fault.
    0x0000000100402f4d in closedir (dirp=0x61e000100010632) at dirent/fs_closedir.c:83
    83        if (idir->fd_root)
    (gdb)

## defconfig.stm32f4discovery-sd

### Setup:

- Requires the STM32-BB baseboard for its micro SD slot (and uses USART6)
- Requires that a micro-SD card be inserted that contains Pascal executables in the root directory.  The Pshell, `Pshell.pex`, in particular, must be there.

### Usage:

1. Make sure that the Pascal directory is clean

    cd <pascal-directory>
    make distclean

2. Link the Pascal directory into the apps directory

    cd <nuttx-apps-directory>
    ln -s <pascal-directory> pascal

3. Configure the NuttX directory

    cd <nuttx-directory>
    tools/configure.sh [OPTIONS] stm32f4discovery:netnsh
    cp <pascal-directory>/configs/defconfig.stm32f4discovery-sd .config
    make olddefconfig

4. And build

    make

### Comments/Status:

- Fails with a read error of some kind while loading Pshell.pex.  The SD card and file are known good and verify (with the same read logic) on the host system.  So I am suspecting some issue with my old STM32 F4-Discovery board.

## defconfig.stm32f4discovery-romfs

### Setup:

- Requires the STM32-BB baseboard *only* because it uses USART6.  Than can be re-configured.

### Usage:

1. Run the host build in order to create compile the Pascal files and build the romfs image

    cd <pascal-directory>
    make memconfig -- configure to run natively on Linux

2. Copy the romfs image into the NuttX source tree

    cp <pascal-directory>/papps/romfs/romfs.img <nuttx-directory>/boards/arm/stm32/common/.

3. Make sure that the Pascal directory is clean

    make distclean

4. Link the Pascal directory into the apps directory

    cd <nuttx-apps-directory>
    ln -s <pascal-directory> pascal

5. Configure the NuttX directory

    cd <nuttx-directory>
    tools/configure.sh [OPTIONS] stm32f4discovery:netnsh
    cp <pascal-directory>/configs/defconfig.stm32f4discovery-romfs .config
    make olddefconfig

6. And build

    make

### Comments/Status:

- See above discusion with regard to `defconfig.sim-hostfs` above for status; the behavior of Pascal on the STM32-F4 Discovery is same as that under the simulator.
- This is a work in progress and not quite ready for prime time.
