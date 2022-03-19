Pascal Shell
============

Overview
--------

This is very much a work in progress.  There is not much here but my longer term goal is to have a modest shell written entirely in pascal.  I would like to have a small user interface that allows monitoring and use of resources in the Pascal sandbox, but also is secure and does not support access to anything outside of that sandbox.

The primary goals of the shell are:

- Allow visibility to the Pascal executables that are available,
- Execute Pascal variables on a different thread with its own separate sandbox,
- Allow debugging of Pascal executables, and
- If the compiler and related tools are available on target, support receipt of Pascal source and with JIT compilation for load and run or later generation and saving of Pascal executables.

No-frills implementations of most of these are available now.

Path Confusion
--------------

The most confusing thing I run into is the selection of paths.  There are two paths of interest:

- The path to the host/target binaries.
- The path to the Pascal executables

Host/target binaries are executed using the `posix_spawnp()` OS interface.  In order to locate files, that OS interface requires that the path to the Pascal tools and run-time programs be included in the `PATH` variable.  These binaries can normally be found at `pascal/bin16` in the source tree, but may be installed at a different location on the target.

Most embedded systems do not execute programs from files, however, but rather as function entry points residing in memory.  The non-standard OS interface `task_create()` must be used in that case (the in-memory executable configuration has not yet been implemented).

The Pascal application executables, `*.pex` files, are saved in `pascal/papps/image` at build time.  The location of the Pascal binaries is provided by the configuration setting `CONFIG_PASCAL_EXECDIR` which may be either an absolute or a relative path.  To simplify use of relative paths, configurations are available to set a *HOME* directory (see `CONFIG_PASCAL_HAVEHOME` and `CONFIG_PASCAL_HOMEDIR`).  The default is the Pascal source tree root directory if `CONFIG_PASCAL_HAVEHOME=y`.  The default location for `CONFIG_PASCAL_EXECDIR` is "papps/image/".

In an embedded system, the Pascal executables would probably be provided on a mounted file system, perhaps on an SD card or perhaps built into an in-memory ROMFS filesystem.  In that case `CONFIG_PASCAL_HAVEHOME` could be left undefined and `CONFIG_PASCAL_EXECDIR` could be set to the mountpoint of that file system.

Another option would be generate in-memory Pascal executable arrays (such as you might create with `xxd -include`) but this alternative has not been explored yet.
