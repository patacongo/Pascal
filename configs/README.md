NuttX Configuration Files
=========================

These are variable NuttX configuration files that were used for testing.

defconfig.sim
-------------

Setup:
- Nothing special

Usage:

    cd <pascal-directory>
    make distclean
    cd <nuttx-apps-directory>
    ln -s <pascal-directory> pascal
    cd <nuttx-directory>
    tools/configure.sh [OPTIONS] sim:nsh
    cp <pascal-directory>/configs/defconfig.sim .config
    make olddefconfig
    make

Comments/Status:
- Currently fails because there is no mounted filesystem with the Pshell.pex executable in it.  This could be fixed by setting up a HostFS.

defconfig.stm32f4discovery-sd
-----------------------------

Setup:
- Requires the STM32-BB baseboard for its micro SD slot (and uses USART6)
- Requires that a micro-SD card be inserted that contains Pascal executables in the root directory.  The Pshell, `Pshell.pex`, in particular, must be there.

Usage:

    cd <pascal-directory>
    make distclean
    cd <nuttx-apps-directory>
    ln -s <pascal-directory> pascal
    cd <nuttx-directory>
    tools/configure.sh [OPTIONS] stm32f4discovery:netnsh
    cp <pascal-directory>/configs/defconfig.stm32f4discovery-sd .config
    make olddefconfig
    make

Comments/Status:
- Fails with a read error of some kind while loading Pshell.pex.  File is known good and verifies (with the same read logic) on the host system.  So I am suspecting some issue with my old STM32 F4Discovery board.
- The Pshell program still depends on posix_spawnp().  That would also have to be fixed by change it to task_spawn() in order to get this configuration running.

defconfig.stm32f4discovery-romfs
--------------------------------

Setup:
- Requires the STM32-BB baseboard *only* because it uses USART6.  Than can be re-configured.

Usage:

    cd <pascal-directory>
    make memconfig -- configure to run natively on Linux
    make clean all
    cp <pascal-directory>/papps/romfs/romfs.img <nuttx-directory>/boards/arm/stm32/stm32f4discovery/src/.
    make distclean
    cd <nuttx-apps-directory>
    ln -s <pascal-directory> pascal
    cd <nuttx-directory>
    tools/configure.sh [OPTIONS] stm32f4discovery:netnsh
    cp <pascal-directory>/configs/defconfig.stm32f4discovery-romfs .config
    make olddefconfig
    make

Comments/Status:
- This is a work in progress
