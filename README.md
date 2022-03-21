README
======

Language Implementation
-----------------------

**What It Is**.  This Pascal implementation is a traditional, Pascal compiler
tool suite with many enhancement for compatibility with modern Pascal usage.
The tool suit include a compiler, optimizer, linker, lister, and run-time
package.  It targets a 16-bit, emulated stack machine that is implemented
as a virtual machine by the run-time logic.

Refer to the PascalNotes.md file in the docs sub-directory for more
language-oriented discussion.

**What It Is Not**.  This Pascal Implementation would not be the best choice for
desk-top application; it does not have a feature set that is as rich as other
desk-top Pascal tool suites.  It's virtual machine would probably not be
optimal for the desk-top environement.  But neither is it a tiny, minimal
Pascal tool set.  Although traditional and not object-based, it is a rather
comple Pascal implementation suitable for significant developments.

**Embedded Pascal**.  The environment where the Pascal compiler would be a
good choice would probably be the embedded environment.  The run-time builds
on the standard C library and expects little more from the hardware platform.
An RTOS environment or, perhaps, even a bare-metal environment would be a
good fit.  A port to NuttX is provided.

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
GNU environment, Linux should also build with no issues.  No attempt has
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

2. Then configure NuttX using the board board configuration of your choice.
   Make sure to enable Pascal support in the *Application Configuration* menu.

    $ cd *nuttx-directory*
    $ make menuconfig

   The most important decision to make in the configuration is if you want to
   host the entire Pascal development toolchain on the target.  That does not
   require as much memory as you might think, but neither is it for the
   memory-limited embedded platform.

   The run-time alone has value because you can code, compile, and link Pascal
   programs on the host, then run them on the target.

3. And make in the *nuttx-directory*

    $ cd *nuttx-directory*
    $ make

And you should have the Pascal components that you have selected in your
board's flash image.

Tips:

- Select the NuttX readline() function with `CONFIG_SYSTEM_READLINE=y`. By default `fgets` is used and the NuttX `fgets` does not behave in the expected way.
