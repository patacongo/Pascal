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
