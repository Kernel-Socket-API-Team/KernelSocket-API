savedcmd_driverCheckCompileModule.mod := printf '%s\n'   main.o ../../../../compileLibraryForLinux/ksockapi_sources.o | awk '!x[$$0]++ { print("./"$$0) }' > driverCheckCompileModule.mod
