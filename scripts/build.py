#!/usr/bin/env python3
"""Build the Upsilon NetBSD bootstrap probe from verified NetBSD sources."""
from pathlib import Path
import hashlib,subprocess,tarfile,sys,struct,re,os
ROOT=Path(__file__).resolve().parents[1]
UPSILON=Path(os.environ.get('UPSILON_ROOT',ROOT.parent/'upsilon'))
GNU=Path(os.environ.get('UPSILON_GNU_ROOT',ROOT.parent/'upsilon-gnu'))
SRC=ROOT/'src/usr/src'
SHA512='318d451ecc83749607d5448198c02663ea5f0b7f4cc74fb565747f86eead7b66b5809a37deee219dc6ddb2e5ee332d1cbb94c1a4e9db63620c3abd7586fb3057'
URL='https://cdn.netbsd.org/pub/NetBSD/NetBSD-11.0/source/sets/syssrc.tgz'

def run(args):
 print('+ '+' '.join(map(str,args)),flush=True)
 subprocess.run(list(map(str,args)),check=True)

def sources():
 archive=ROOT/'vendor/syssrc-11.0.tgz';archive.parent.mkdir(parents=True,exist_ok=True)
 if not archive.exists():
  part=archive.with_suffix('.part')
  run(['curl','-fL','--retry','2',URL,'-o',part])
  if hashlib.sha512(part.read_bytes()).hexdigest()!=SHA512:raise SystemExit('NetBSD source checksum mismatch')
  part.replace(archive)
 if hashlib.sha512(archive.read_bytes()).hexdigest()!=SHA512:raise SystemExit('NetBSD source checksum mismatch')
 if not (SRC/'sys/sys/param.h').exists():
  (ROOT/'src').mkdir(exist_ok=True)
  with tarfile.open(archive) as t:t.extractall(ROOT/'src')

def main():
 sources();build=ROOT/'build';build.mkdir(exist_ok=True);inc=build/'include';inc.mkdir(exist_ok=True)
 md=ROOT/'sys/arch/upsilon'
 link=inc/'machine'
 if link.is_symlink() and link.readlink()!=Path('../../sys/arch/upsilon/include'):
  link.unlink()
 if not link.exists():link.symlink_to('../../sys/arch/upsilon/include',target_is_directory=True)
 cc=GNU/'install/bin/upsilon-unknown-elf-gcc'
 if not cc.exists():raise SystemExit('Build the GNU toolchain first: make gnu-toolchain')
 libfdt=SRC/'sys/external/bsd/libfdt/dist'
 flags=['-O2','-g','-ffreestanding','-fno-builtin','-fno-stack-protector','-D__NetBSD__','-D_STANDALONE',
        '-DLIBSA_PRINTF_LONGLONG_SUPPORT','-DLIBSA_PRINTF_WIDTH_SUPPORT','-DLIBSA_NO_FS_CLOSE',
        '-I'+str(inc),'-I'+str(SRC/'sys'),'-I'+str(SRC/'common/include'),'-I'+str(libfdt),'-I'+str(ROOT/'tests/bootstrap')]
 kflags=[v for v in flags if v!='-D_STANDALONE']+['-D_KERNEL','-DNDEBUG']
 # Generate offsets using the target compiler, not the host's C layout.
 run([cc,*kflags,'-c',ROOT/'tests/bootstrap/assym.c','-o',build/'assym.o'])
 objcopy=cc.parent/'upsilon-unknown-elf-objcopy'
 run([objcopy,'-O','binary','-j','.rodata.assym',build/'assym.o',build/'assym.bin'])
 names=re.findall(r'ASSYM\(([A-Z0-9_]+),',(ROOT/'tests/bootstrap/assym.def').read_text())
 raw=(build/'assym.bin').read_bytes()
 if len(raw)!=8*len(names):raise SystemExit('unexpected target assym data size')
 values=struct.unpack('<'+'Q'*len(names),raw)
 (build/'assym.h').write_text('/* Generated from target NetBSD headers. */\n'+''.join(f'#define {n} {v}\n' for n,v in zip(names,values)))
 files=[md/'upsilon/vm_machdep.c',ROOT/'tests/bootstrap/fork_test.c',ROOT/'tests/bootstrap/fork_user.S',md/'upsilon/atomic.S',ROOT/'tests/bootstrap/atomic_test.c',md/'upsilon/locore.S',md/'upsilon/pmap_bootstrap.c',md/'upsilon/pmap.c',md/'upsilon/trap.c',md/'upsilon/switch.S',md/'upsilon/context.c',md/'upsilon/copy.c',ROOT/'tests/bootstrap/context_test.c',ROOT/'tests/bootstrap/pmap_test.c',ROOT/'tests/bootstrap/cpu_bootstrap.c',ROOT/'tests/bootstrap/machdep.c',ROOT/'tests/bootstrap/compiler_rt.S',
        SRC/'sys/lib/libsa/panic.c',SRC/'sys/lib/libsa/printf.c',SRC/'sys/lib/libsa/subr_prf.c',libfdt/'fdt.c',libfdt/'fdt_ro.c']
 files += [SRC/'common/lib/libc/string'/f'{n}.c' for n in ['strlen','strnlen','strcmp','strncmp','strchr','strrchr','memchr','memcpy','memmove','memset','memcmp']]
 # Keep the architecture-independent routines unmodified. These selected
 # sources have fixed argument lists, matching the target's builtin libcall ABI.
 atomic=SRC/'common/lib/libc/atomic'
 atomic_names=[f'atomic_{op}_{bits}_{tail}.c' for bits in (32,64)
               for op,tail in [('add','nv_cas'),('and','nv_cas'),('or','nv_cas'),
                               ('inc','add'),('inc','nv_add'),('dec','add'),('dec','nv_add')]]
 files += [atomic/name for name in atomic_names]
 objects=[]
 for src in files:
  obj=build/(src.stem+'.o');objects.append(obj)
  # Assembly does not need the C preprocessor environment or header paths.
  actual=kflags if src.name in ['context.c','copy.c','context_test.c','cpu_bootstrap.c','trap.c','pmap.c','pmap_test.c','atomic_test.c','vm_machdep.c','fork_test.c'] else flags
  if src.parent==atomic:actual=kflags+['-D_HARDKERNEL']
  if src.suffix=='.S':actual=['-g','-I'+str(build)]
  run([cc,*actual,'-c',src,'-o',obj])
 run([cc,'-nostdlib','-Wl,--build-id=none','-Wl,-Map='+str(build/'bootstrap.map'),'-T',ROOT/'tests/bootstrap/probe.ld',*objects,'-o',build/'bootstrap.elf'])
 print('Built bootstrap probe (not a full NetBSD kernel): '+str(build/'bootstrap.elf'))

if __name__=='__main__':main()
