NuttX Configuration Files
=========================

These are variable NuttX configuration files that were used for testing.

defconfig.sim
-------------

Setup:
- Nothing special

Usage:

    cd <nuttx-directory>
    tools/configure.sh [OPTIONS] sim:nsh
    cp <pascal-directory>/configs/defconfig.sim .config
    make olddefconfig
    make

Comments/Status:
- Currently fails because there is no mounted filesystem with the Pshell.pex executable in it.  This could be fixed by setting up a HostFS.

defconfig.stm32f4discovery
--------------------------

Setup:
- Requires the STM32-BB baseboard for its micro SD slot (and uses USART6)
- Requires that a micro-SD car be inserted that contains Pascal executables in the root directory.  The Pshell, `Pshell.pex`, in particular, is required there.

Usage:

    cd <nuttx-directory>
    tools/configure.sh [OPTIONS] stm32f4discovery:netnsh
    cp <pascal-directory>/configs/defconfig.sim .config
    make olddefconfig
    make

Comments/Status:
- Fails with a read error of some kind while loading Pshell.pex.  File is known good and verifies (with the same read logic) on the host system.
- The Pshell program still depends on posix_spawnp().  That would also have to be fixed by change it to task_spawn() in order to get this configuration running.
