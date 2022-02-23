README
======

Language Implementation
-----------------------

Refer to the PascalNotes.rtf file in the docs sub-directory.

Configuring Pascal
------------------

The Pascal configuration is based on kconfig-frontends tools extracted from
the Linux kernel.  As of this writing, there is no officially maintainer for
these tools.  The kconfig-frontends are often available as an installable
package under Linx distrubitions.  A source snapshot and build instructions
are avaiable at https://bitbucket.org/nuttx/tools/src/master/

    $ cd *pascal-directory*
    $ make menuconfig

Building Pascal Under Linux
---------------------------

The build system has been well excercised under a GNU development
environment, in particular, the Cygwin environment.  Because it is the same
GNU environment, Linux should also build with no issues.  Not attempt has
been made to verify the build under BSD environments (including macOS).

    $ cd *pascal-directory*
    $ make

Building Pascal Under NuttX
---------------------------

NuttX is a mature, open-source RTOS that runs on many MCUs.  When building
for NuttX, you need to cross compile the code on a host machine.  Code under
`tools/`, for example must always build in the host environment.  And the
run-time code must always be built for the target MCU environment.  It is an
option whether you want to host the build tools (compiler, optimizer, linker,
lister, etc) on the host cross-development environment or on the target
itself.

1. *Before* configuring NuttX, create a symbolic link to the Pascal root
   directory in the NuttX apps directory:

    $ cd *nuttx-apps-directory*
    $ ln -s *pascal-directory* pascal

*More to be provided.*

