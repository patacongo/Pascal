# Pascal Applications

## Directories

### pstart Directory

The `pstart` directory holds a tiny application that can be used as the NuttX
system startup function; simply enable `CONFIG_PASCAL_STARTUP` and configure
`CONFIG_INIT_ENTRY` to be `pstart_main`.  Then when the system boots,
`pstart_main` will run and will start and run any pre-compiled Pascal program
for the system start-up logic.

### image and romfs Directories

These directories are used to generate a filesystem image that provides Pascal executales.  More about this later.

### punits

This is where standard and semi-standard pascal Units are retained.

### phello, pshell, ...

The remaining directories hold Pascal source files and build logic for Pascal.

## Image Directory

If any Pascal executables are generated during the build, the executable files will be installed in the `papps/image` subdirectory.  This subdirectory may then be used to create a file system image (amongst other things).  These files might, perhaps, be simply copied onto an SD card.  There is, for another example, logic in the `papps/romfs` subdirectory to convert the `papps/image` subdirectory into a ROMFS filesystem image that can be builtin as an in-memory filesystem.

This may not be as simple as it sounds, however.  There may be three different configurations. Consider these:

### Host-Only Configuration

The simplest case is when Pascal is use natively on a Linux or Cygwin host.  In that case, the generated Pascal executable can be provided as command line arguments to run-time program directly.  There is really no need to deal with generating filesystem images at all.

### Host Tools/Target Run-Time Configuration

For small embedded systems, the Pascal run-time program (*prun*) is the only tool component that is needed on the target machine.  Since there is no possibility of generating new Pascal executables on the target, they must be generated on the host machine and transferred to the target machine.  Using a filesystem to convey the Pascal executables is one of the simpler ways to do this.

As mentioned above, this could mean just copying the executable files onto and SD card.  Or it may mean generating an in-memory ROMFS filestem image from `papps/image` directory and including that in the build.  But that cannot be done simply.  It first requires a host build configuration to generate the tools and to populate the `papps/image` directory.  That image directory can then be copied to a target build configuration to generate the in-memory filesystem image with the tools under `papps/romfs`.

The simplest way to do this is two two clones of the Pascal repository.  The first is set up in host configuration and is used to generate Pascal exectables.  The Pascal binaries can then be copied into a second clone which is set up with a target, run-time only cross-build configuration.

### Target-Only Configuration

If you have sufficient memory resources you can also build the entire set of tools on the target.  But you would still have to build the Pascal executables on the host.  Why?  Because in a cross-development environment, the target image will be built on a host machine and later executed on a different target machine.  In order to generate Pascal executables, you must first have usable compiler tools at the time of the build.  But that is on the wrong machine!

The solution to this chicken-n-egg problem is the same as before, you still need a host Pascal configuration if only to build any initial Pascal executables that you want to included in the build.  The only benefit of this configuration is that you can then generate new Pascal executables on the fly in the target machine.
