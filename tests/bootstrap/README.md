# Bootstrap probe

This is the standalone test harness for the Upsilon MD code. It supplies probe
startup, a synthetic MI startup caller, controlled syscall/exit handling and
user programs. It is not the NetBSD MI scheduler, exec loader or process manager.

Build and run through `make test` at the repository root. Generated assym offsets,
objects, maps and the probe ELF belong under the ignored root `build/` folder.
