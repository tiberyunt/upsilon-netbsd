# NetBSD/Upsilon port

Experimental machine-dependent port for Upsilon, based on verified NetBSD 11.0
kernel sources. **The current executable is a bootstrap probe, not a complete
booting NetBSD kernel.**

| Directory | Purpose |
| --- | --- |
| sys/arch/upsilon/include/ | Machine headers |
| sys/arch/upsilon/upsilon/ | MD context, atomics, traps, copy and pmap implementations |
| tests/bootstrap/ | Probe startup, user programs, linker script and guest test harness |
| tests/ | Host bootstrap regression runner |
| scripts/ | Verified source download and cross-build script |
| [docs/](docs/bringup.md) | Boot contract, completed milestones and remaining port work |

```sh
make                   # compile the probe
make demo              # run with the sibling simulator
make test              # execute bootstrap tests
```

Defaults are sibling repositories `../upsilon` and `../upsilon-gnu`.
Override `UPSILON_ROOT`, `UPSILON_GNU_ROOT` and `UPSILON_C` when needed.
The GNU cross-compiler and simulator must already be built.

The build downloads and verifies the official NetBSD source set into `vendor/`
and extracts it under `src/`. Upstream sources remain unmodified. Those
folders, generated `build/` files and local media are excluded from Git.
The `sys/` folder contains this port's source additions; `tests/bootstrap/`
contains probe-only support, including stand-ins for MI startup/exit.

Read [the bring-up guide](docs/bringup.md) for the actual status and limitations.
`COPYING` and `NOTICE` preserve the port license and upstream notices.
