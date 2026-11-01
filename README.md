This repository contains the NetBSD port separately from the simulator and GNU
ports. Defaults are sibling repositories `../upsilon` and `../upsilon-gnu`.
Override `UPSILON_ROOT`, `UPSILON_GNU_ROOT` and `UPSILON_C` when needed.
Downloaded NetBSD sources, build output and media are excluded from Git.

# NetBSD/Upsilon bring-up

The machine-dependent bootstrap now includes CPU/LWP contexts, safe user
copies, native atomic operations, a managed-page pmap, and fork/exec context
setup. It compiles **unmodified NetBSD 11.0 source files and headers** with the Upsilon GCC port,
loads the resulting ELF through the emulator's new kernel boot interface, and
exercises the CPU mechanisms needed by a NetBSD port.

**This is a bootstrap probe, not a booting NetBSD kernel.** It has no MI kernel
initialization, UVM, scheduler, processes, root filesystem or shell. Its output
labels it as a probe. The syscall tests use NetBSD's syscall numbers and errno
headers, with a small test dispatcher rather than the MI syscall table.

## Run

From this repository:

```sh
make demo
# Or build and launch separately:
make
../upsilon/build/upsilon -m 32 --kernel build/bootstrap.elf --bootargs '-s'
make test
```

The existing GNU cross-compiler is required (`make gnu-toolchain`). The build
script downloads the official `NetBSD-11.0/source/sets/syssrc.tgz` when missing,
verifies its SHA-512 against the published release checksum pinned in
`build.py`, and extracts it under `netbsd/src/`. It does not modify upstream
source files. Port additions are under `overlay/sys/arch/upsilon/`; probe code
and its linker script are under `bootstrap/`.

The upstream files actually compiled are NetBSD libsa's `printf.c`,
`subr_prf.c` and `panic.c`, libfdt's `fdt.c` and `fdt_ro.c`, eleven common
libc string and memory routines, and fourteen generic atomic routines. The libraries use NetBSD's own standalone headers, including
`stand.h` and `libkern.h`. MD context, copy and pmap code now also compiles in the
`_KERNEL` namespace against the real `sys/lwp.h`, `sys/cpu.h`, `cpu_data` and
`systm.h` interfaces, plus the real UVM `vm_page`, `vmspace` and pmap
declarations. There are no replacement libc headers or host OS callbacks.
A small BSD-licensed assembly file supplies integer division and byte-swap
compiler helpers; the probe links with `-nostdlib` and does not depend on the
KotlTOS/GNU demonstration runtime.

Expected final lines:

```text
MMU: identity + high-half alias, device mapping passed
trap: unmapped load recovered through RTE
timer: 3 interrupts passed
context: 60 switches + 20 timer IRQs, registers, stacks, TLS, private mappings, roots/ASID, IPL passed
atomics: native CAS, upstream RMW, aliases, builtins, barriers and IRQ sharing passed
pmap: lifecycle, activation, isolation, PV/protection, ref/dirty, kernel maps passed
pmap resources: CANFAIL rollback, pool exhaustion, reclamation and ASID reuse passed
fork/exec: MI startup ABI, kthread/child stacks, inherited state, setregs and native user return passed
copy: user permissions, onfault stack unwind, EFAULT and UA restore passed
user mode: write/getpid/ENOSYS/EFAULT passed
user TLS: MFTP and TP-relative memory passed
BRINGUP PASS: NetBSD libsa/libfdt, MMU, traps, timer, user transition
```

The probe halts after these checks. The bootstrap tests run it with 16, 31, 32
and 256 MiB, including a RAM size requiring 4 KiB mappings at the end. Under
16 MiB it reports the probe's minimum-memory requirement and halts.

## Boot contract

`upsilon -m N --kernel image.elf [--bootargs string]` loads ELF64 little-endian
ET_EXEC images for the local experimental Upsilon machine ID `0xf5a0`.
`upsilon run image.elf --elf --memory bytes` is also available. Kernel boot
selects the ELF path directly; the firmware's tape/disk boot path remains
separate.

The loader checks the entire program-header table, all load segments, file/RAM
bounds, alignment, segment overlap, ROM/FDT collision and executable entry
before writing guest memory. It copies PT_LOAD bytes to physical p_paddr and
zeroes the remaining p_memsz. Dynamic images are rejected. An image must have
an identity-mapped executable entry containing file-backed code; a later
trampoline can enable the MMU for high virtual segments. Up to 128 program
headers are accepted; the current input-file limit is 64 MiB.

At entry:

- R0 is the physical address of a version-17 flattened device tree.
- R1 and R2 are zero. Other scratch registers have no boot ABI meaning.
- Kernel mode, IPL 7, translation off; timer compare disabled.
- SP initially points immediately below the reserved FDT region. The probe
  switches to a stack in its own BSS before calling C.
- The FDT occupies a reserved 64 KiB area near the top of RAM, aligned to 4 KiB.
  Its memory-reservation table records the full region. The loader rejects
  PT_LOAD segments that would overwrite it.

The FDT describes the RAM range, one Upsilon v0.4 CPU, 1 MHz timebase, the
polled NS16550 console, the existing platform registers, chosen stdout path,
and boot arguments (up to 1023 bytes). It does not advertise unimplemented
KIC or virtio devices. The root compatible string is `kotl,upsilon-virt`.

## Console

A byte-wide, polled NS16550-compatible register bank is at **0xffff0400**.
Its data queue and serial output share the existing terminal connection with
the legacy UART at 0xffff0000. The original firmware console remains usable.

The new bank implements RBR/THR, IER storage, FIFO control/reset, IIR with no
pending interrupt, LCR and DLAB divisor latches, MCR storage, TX-ready/data-ready
LSR, fixed modem-ready status and scratch register. It is functional, without
baud timing, modem wiring or loopback. **No UART IRQ is connected yet**, and
no interrupts property is advertised in the FDT. Writes to read-only registers
are ignored. Accesses must be one byte. Control writes, receive consumption and
transmit bytes participate in the emulator's instruction transaction, so a
faulting instruction cannot partly change the UART.

## Machine-dependent code

The machine headers define the LP64 types, integer constants/format macros,
page constants, PTE fields, a provisional machine context and the trap frame.
The existing external `upsilon-unknown-elf` GCC is used in freestanding mode;
the build defines the NetBSD/standalone namespaces explicitly. This does not
claim an integrated NetBSD `build.sh` toolchain or an installed userland sysroot.

`locore.S` saves R0–R31 followed by the hardware PSR/PC pair and aligns the
kernel stack before calling C. Frame offsets are checked by C static
assertions. Trap return restores registers and the hardware frame through RTE;
user SP is carried through SPU. A kernel onfault return can also restore an
older SP by relocating the hardware PSR/PC pair before RTE. The test user
program runs on two user pages,
with FPD set, executes SYSCALL, checks carry/errno, and returns through a
controlled exit path. The test `write` dispatcher now copies user data in
64-byte chunks through the MD `copyin` implementation. This remains a test
syscall dispatcher, not the MI process syscall implementation.

`pmap_bootstrap.c` constructs four-level tables with 2 MiB RAM leaves, falling
back to 4 KiB leaves where necessary. It maps RAM at both physical addresses
and `0xffff800000000000 + PA`, and maps the MMIO page with Device memory type.
It can split a 2 MiB leaf when entering a 4 KiB mapping. The initial RAM mapping
is RWX for bring-up; W^X and UVM-managed page tables are unfinished. The probe
continues executing through the low identity map and verifies the high alias;
it is not a complete high-half kernel transition.

The clock test programs CYCLES/TIMECMP and handles vector 64 at IPL 6. WAIT
allows deterministic idle-time advance in the emulator. This validates trap
entry and rearming; NetBSD's hardclock/timecounter integration remains ahead.

## CPU/LWP and copy milestone

`context.c` supplies the UP `cpu_info_store`, native SPL operations,
`cpu_lwp_setprivate` and the early thread-context helper. `switch.S` implements
NetBSD's `cpu_switchto(old, new, returning)` interface: it returns the LWP that
most recently switched out, restores R16–R30 and SP, and switches TP, PTBR0,
ASID and PSR/IPL while updating `ci_curlwp`/`ci_onproc`. The bootstrap-only
`cpu_bootstrap.c` supplies `lwp0` for the probe. A full kernel will use MI
lwp0 initialization; the MD implementation does not define a duplicate MI LWP.

The saved-switch frame is distinct from the complete exception trapframe.
`assym.c` is compiled by **Upsilon GCC** against the real NetBSD structures.
The build reads a dedicated object section to generate `assym.h`, using the
shared definitions in `assym.def`. Assembly offsets therefore come from the
target C layout. This does not require running a target executable on the host.

The harness drives three actual NetBSD `struct lwp` instances cooperatively,
with separate kernel stacks, user TP values, page-table roots, ASIDs and IPLs.
Each worker also maps the same user VA to a different physical page. Repeated
copyin/out operations verify that the active root selects its own page and
that the parent cannot read either private mapping. The page tables share the
bootstrap kernel mappings; they are not UVM pmap objects or a managed-page
implementation.

Every suspended call is checked with assembly canaries in **all** callee-saved
registers, including FP. Sixty switches run with twenty timer interrupts
pending across the switch boundary; an additional self-switch is tested.
Stack locals, per-LWP onfault pointers, current-LWP bookkeeping, hardware TP,
roots/ASIDs and software/hardware IPL agreement are checked on resume. The
harness updates LWP states as a scheduler caller would, but it is **not** the
NetBSD MI scheduler and does not implement preemption, MI process fork/exit
services or scheduler locking. The separate fork/exec context milestone below
adds the MD startup hooks.

`setjmp`/`longjmp` save and restore callee registers, SP, return PC, PSR and TP.
The MD `copyin`/`copyout` functions validate user VA bounds and active-page-table
leaf privileges, so low kernel identity aliases cannot be copied as user
buffers. They install a per-LWP fault label and restore both the previous
label and UA permission on success or EFAULT. Tests force real MMU bus faults
inside a nested copy function and verify unwinding to the saved stack. They
also cover read-only user text, kernel pointers, address-range overflow,
zero-length copies and preservation of an already-enabled UA bit.

These copies currently operate on resident bootstrap mappings. They do not
invoke `uvm_fault`, pin pages or coordinate with SMP pmap teardown. Such work
belongs with the UVM pmap integration. `copyinstr`, `copyoutstr` and `kcopy`
are not implemented yet. The new lock/mutex headers provide native simple
spinlocks and storage needed to compile MI headers; the generic mutex/softint
implementations and SMP are not integrated.

The user test additionally reads its TP with MFTP and accesses a user TLS page
through TP-relative load/store operands. This validates the architecture and
`cpu_lwp_setprivate` mechanism, not a compiler TLS relocation model or a finished
NetBSD userland TLS ABI. FP state is reserved in the PCB but remains disabled;
there is no lazy FPU save/restore yet.

## Managed-page pmap milestone

`pmap.c` implements the NetBSD pmap interfaces for create/reference/destroy,
enter/extract/remove, range and per-page protection, wiring statistics,
activation, kernel mappings, and physical page zero/copy. Activation uses the
real `lwp -> proc -> vmspace -> vm_map.pmap` path and updates PCB and hardware
PTBR0/ASID. User pmaps inherit the bootstrap kernel identity mappings; modifying
an inherited table path clones it so another address space stays unchanged.
Inherited bootstrap leaves cannot be replaced through `pmap_enter`.

Managed mappings maintain reverse lists in actual `struct vm_page` metadata.
Hardware reference/dirty bits are harvested on replacement/removal, queried
across aliases, and cleared in both live PTEs and saved page history. Kernel
`pmap_kenter_pa` mappings are wired and intentionally unmanaged. Removal prunes
empty owned tables; `PMAP_CANFAIL` allocation failure rolls back partially
constructed paths. On this UP implementation, operations raise IPL and use
whole-TLB invalidation. The bounded ASID slots are flushed before reuse.
`pmap_deactivate` retains the current root until activation or a context switch
replaces it; destroying an active address space is rejected.

This is a **bounded bootstrap implementation**, not integrated UVM. It has
8 user pmap slots, 128 page-table pages and 256 mapping records. Tables reside
in statically allocated, identity-linked RAM and are accessed through the high
RAM alias. A bootstrap callback identifies managed physical pages; the probe
provides three actual `vm_page` objects. UVM physical-page discovery/allocation,
locking, demand-fault handling, pager integration and a high-half-only kernel
remain ahead. User mappings cannot occupy the inherited low kernel aliases.
The optional advisory `pmap_copy` operation is a no-op.

The guest tests switch between pmaps with different physical pages at the same
VA, exercise writable aliases and real hardware read-only faults, replace and
remove mappings, check range boundaries and cloned-path isolation, verify
reference/dirty history, and test unmanaged kernel mappings and physical page
operations. They deliberately exhaust all three pools, repeat failed
allocations, check rollback/reclamation and reuse roots/ASIDs. These tests run
at all four supported probe RAM sizes through both normal and sanitizer builds
of the emulator.

## Atomic operations milestone

`atomic.S` provides native CAS at 8/16/32/64 bits, and native 32/64-bit
add, AND, OR and exchange. It exports NetBSD's integer, long and pointer
aliases, including the non-interlocked (`_ni`) CAS entry points. Those aliases
use the same interlocked primitive. Memory barrier functions issue `FENCE`;
atomic operations use full barriers, stronger than the minimum required by
some callers. Simple spinlock acquisition/release also uses fences.

Fourteen unmodified NetBSD common atomic sources implement the new-value
add/AND/OR operations and increment/decrement variants on top of native CAS
and add. Native runtime entry points support the tested 32/64-bit GCC
`__sync` CAS, boolean CAS, fetch/add/AND/OR, new-value add/AND/OR, exchange,
lock release and synchronization builtins. This is not a complete C11
`__atomic` runtime, a compiler inline-atomic backend, or SMP support.

The generic upstream fetch-operation helpers use variadic declarations. The
Upsilon ABI puts variadic arguments on the stack, while compiler-generated
builtin libcalls pass fixed arguments in registers. Consequently those helper
sources are not used as builtin entry points; the native fixed-argument
wrappers supply them. The fourteen selected generic routines have fixed
argument lists and compile with NetBSD's `_HARDKERNEL` aliases.

Guest checks cover success/failure CAS, narrow-access neighbor preservation,
wraparound and negative addition, all public integer/pointer aliases, GCC
builtin calls, and NetBSD release/acquire accessors. Timer interrupts and the
mainline update shared 32/64-bit counters while CAS-loop routines run without
masking interrupts. A deliberately stale CAS verifies that an interrupt's
update survives. The emulator is single-CPU; these tests do not establish
multiprocessor memory-ordering behavior. Generic mutexes, scheduler locks and
soft interrupts still require integration.

The NetBSD build now creates its machine-header symlink relative to the build
folder and repairs the previous absolute symlink, so moving the repository
does not leave headers pointing to its former location.

## Fork/exec context milestone

`vm_machdep.c` implements `cpu_lwp_fork`, `md_child_return`, `cpu_lwp_pc` and
`setregs`. The child has a trapframe at the top of its own 16 KiB uarea, with
the PCB at offset zero and a separate initial switch frame below the trapframe.
Fork copies user registers and saved FP storage, inherits the live parent TP,
selects the child's pmap, supports an alternate user stack, and clears inherited
onfault/AST state. User PSR construction excludes kernel privilege, UA and IPL.
FP remains disabled under the provisional soft-float ABI.

The fork trampoline calls the MI `lwp_startup(old, new)` interface and then the
requested entry function. An entry that returns continues through the child's
user trapframe and native RTE path. `md_child_return` supplies R0=0, R1=1 and
clears carry. A kernel-thread entry has no user continuation and must not
return; the guarded return path rejects its empty frame.

`setregs` uses the actual NetBSD `exec_package` entry point and process
`p_psstrp`. It clears the user register frame and saved FP storage, aligns SP
to 16 bytes, clears TP, and supplies initial `crt0(cleanup, ps_strings)` values
in R0/R1. The loader's argc/argv data stay on the user stack. This is an initial
port ABI, not an implemented NetBSD crt0, dynamic linker or ELF exec loader.

The probe runs a fork-style child and an exec-initialized child in user mode,
each with its own uarea, mapped stack, pmap and ASID, and also starts a kernel
thread without a parent user frame. Assembly checks inherited/cleared
registers, TP, stack alignment and stack data before making syscalls. Parent
register canaries, uarea boundary guards and pmap reclamation are checked after
the native switch back. A negative test deliberately returns from a kernel
thread and verifies rejection before user entry.

The harness supplies a **probe-only `lwp_startup` implementation**, a small
child entry and a controlled syscall-exit callback. It verifies the startup
ABI; it does not run `kern_lwp.c`, `child_return` from `kern_fork.c`, MI exec,
process exit/reaping or the MI scheduler. A full kernel will supply those MI
implementations and uarea allocation; the MD hooks do not allocate uareas.

## Remaining port work

| Area | Current state / next implementation |
| --- | --- |
| Compiler/ABI | External GCC/GAS works; integrate NetBSD target specs, the remaining compiler runtime and the final kernel/user ABI. Native kernel atomics now work. The current GCC soft-float variant is provisional. |
| MI kernel | CPU/LWP/PCB, native switching, SPL and setjmp foundations work. MD fork startup and user return now work against the startup ABI. Next: config/build glue, MI scheduler locking/startup, exit/reaping, soft interrupts and MI init. |
| Virtual memory | Bounded pmap lifecycle, managed-page reverse mappings, protection/removal and ref/dirty tracking work. Next: UVM allocation/locking, physical-page discovery, high-half-only execution and demand faults. |
| User ABI | Architectural TP/TLS access works. Exec register setup works. Real syscall dispatch, ELF exec loading, signals, compiler TLS ABI and FP state remain. |
| Platform | KIC/external interrupts, full console attachment, coherent bus_space/bus_dma and virtio-mmio block storage. |
| Userland | Build a static init and shell, boot a ramdisk, then mount persistent storage. |

The custom tape/qcow2 controllers used by KotlTOS have not been presented as
virtio devices. Existing saved media are not reformatted or used by the probe.

The source baseline is [NetBSD 11.0](https://www.netbsd.org/releases/formal.html);
[official kernel sources](https://cdn.netbsd.org/pub/NetBSD/NetBSD-11.0/source/sets/)
and their published SHA512 file are the inputs. Upstream copyright notices
remain in the downloaded source files. New port files are BSD-2-Clause; see
`COPYING` and `NOTICE`.
