# Pascal Applications

## init

The `init` directory holds a tiny application that can be used as the NuttX
system startup function; simply enable `CONFIG_PASCAL_STARTUP` and configure
`CONFIG_INIT_ENTRY` to be `pstart_main`.  Then when the system boots,
`pstart_main` will run and will start and run any pre-compiled Pascal program
for the system start-up logic.

