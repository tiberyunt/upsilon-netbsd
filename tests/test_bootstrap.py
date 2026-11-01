#!/usr/bin/env python3
"""Run the actual GCC-built NetBSD-source probe through ELF/FDT boot."""
from pathlib import Path
import json, os, subprocess, tempfile, unittest
ROOT=Path(__file__).resolve().parents[1]
EMU=Path(os.environ.get('UPSILON_C',Path(os.environ.get('UPSILON_ROOT',ROOT.parent/'upsilon'))/'build/upsilon'))
class NetBSDBootstrap(unittest.TestCase):
    def test_bootstrap(self):
        for memory in [16,31,32,256]:
            with self.subTest(memory=memory),tempfile.TemporaryDirectory() as d:
                state=Path(d)/'state.json'
                p=subprocess.run([str(EMU),'-m',str(memory),'--kernel',str(ROOT/'build/bootstrap.elf'),'--bootargs','-s','--no-keyboard','--max-steps','4000000','--state',str(state)],capture_output=True,text=True)
                self.assertEqual(p.returncode,0,p.stderr+p.stdout)
                for part in ['NetBSD 11.0 bootstrap probe','FDT: '+str(memory)+' MiB','bootargs: -s','MMU: identity + high-half alias','unmapped load recovered','timer: 3 interrupts passed','context: 60 switches + 20 timer IRQs','TLS, private mappings, roots/ASID, IPL passed','atomics: native CAS, upstream RMW, aliases, builtins, barriers and IRQ sharing passed','pmap: lifecycle, activation, isolation, PV/protection, ref/dirty, kernel maps passed','pmap resources: CANFAIL rollback, pool exhaustion, reclamation and ASID reuse passed','fork/exec: MI startup ABI, kthread/child stacks, inherited state, setregs and native user return passed','copy: user permissions, onfault stack unwind, EFAULT and UA restore passed','user mode: write/getpid/ENOSYS/EFAULT passed','user TLS: MFTP and TP-relative memory passed','BRINGUP PASS']:
                    self.assertIn(part,p.stdout)
                s=json.loads(state.read_text());self.assertTrue(s['halted']);self.assertEqual(s['system_registers']['MMUCTL'],1)
                self.assertEqual(s['system_registers']['TIMECMP'],2**64-1)
                self.assertEqual(s['system_registers']['ASID'],0)
                self.assertEqual(s['system_registers']['TP'],0x40001000)
                self.assertFalse(s['psr'] & 0x23000)
    def test_returning_kernel_thread_is_rejected(self):
        p=subprocess.run([str(EMU),'-m','32','--kernel',str(ROOT/'build/bootstrap.elf'),'--bootargs','--test-returning-kthread','--no-keyboard','--max-steps','4000000'],capture_output=True,text=True)
        self.assertEqual(p.returncode,0,p.stderr+p.stdout)
        self.assertIn('ups_return_user: invalid user context or returned kernel thread',p.stdout)
        self.assertNotIn('BRINGUP PASS',p.stdout)
    def test_insufficient_bootstrap_ram_is_reported(self):
        p=subprocess.run([str(EMU),'-m','8','--kernel',str(ROOT/'build/bootstrap.elf'),'--no-keyboard'],capture_output=True,text=True)
        self.assertEqual(p.returncode,0,p.stderr);self.assertIn('BRINGUP FAILED: 16 MiB required',p.stdout)
if __name__=='__main__':unittest.main(verbosity=2)
