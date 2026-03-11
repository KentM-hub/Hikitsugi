import m5
from m5.objects import *
from m5.util import addToPath
addToPath('../../')
from caches import *

import argparse
import os

thispath = os.path.dirname(os.path.realpath(__file__))
default_binary = os.path.join(
    thispath,
    "../../../",
    "/home/kento/Documents/SPEC/benchspec/CPU/519.lbm_r/run/run_base_refrate_firstrun-m64.0000/lbm_r_base.firstrun-m64",
)

parser = argparse.ArgumentParser()
parser.add_argument("--cpu_type", default="X86TimingSimpleCPU", help="CPU model to use")
parser.add_argument("--fast_forward", type=int, default=0, help="Number of instructions to fast forward")
parser.add_argument("--l1i_size", default="16kB", help="L1 instruction cache size")
parser.add_argument("--l1d_size", default="64kB", help="L1 data cache size")
parser.add_argument("--l2_size", default="256kB", help="L2 cache size")
parser.add_argument("--binary", default=default_binary, help="Path to the binary to execute")

args = parser.parse_args()
print(f"Binary path: {args.binary}")
print(f"Binary exists: {os.path.exists(args.binary)}")

system = System()

system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "1GHz"
system.clk_domain.voltage_domain = VoltageDomain()

system.mem_mode = "timing"
system.mem_ranges = [AddrRange("8192MB")]

# CPU creation
if args.fast_forward:
    system.cpu = AtomicSimpleCPU()
    system.mem_mode = 'atomic'
    system.fast_forward_cpu = DerivO3CPU()
else:
    system.cpu = DerivO3CPU()

system.cpu.createThreads()

# Cache configuration
system.cpu.icache = L1ICache(size=args.l1i_size)
system.cpu.dcache = L1DCache(size=args.l1d_size)

system.cpu.icache.connectCPU(system.cpu)
system.cpu.dcache.connectCPU(system.cpu)

system.l2bus = L2XBar()

system.cpu.icache.connectBus(system.l2bus)
system.cpu.dcache.connectBus(system.l2bus)

system.l2cache = L2Cache(size=args.l2_size)
system.l2cache.connectCPUSideBus(system.l2bus)

system.membus = SystemXBar()

system.l2cache.connectMemSideBus(system.membus)

system.cpu.createInterruptController()
system.cpu.interrupts[0].pio = system.membus.mem_side_ports
system.cpu.interrupts[0].int_requestor = system.membus.cpu_side_ports
system.cpu.interrupts[0].int_responder = system.membus.mem_side_ports

system.system_port = system.membus.cpu_side_ports

system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.mem_side_ports

process = Process()
process.cmd = [
  args.binary,
  "3000",
  "/home/kento/Documents/SPEC/benchspec/CPU/519.lbm_r/run/run_base_refrate_firstrun-m64.0000/reference.dat",
  "0",
  "0",
  "/home/kento/Documents/SPEC/benchspec/CPU/519.lbm_r/run/run_base_refrate_firstrun-m64.0000/100_100_130_ldc.of",
]

system.cpu.workload = [process]
system.cpu.createThreads()

root = Root(full_system=False, system=system)

m5.instantiate()

print("Beginning simulation!")
if args.fast_forward:
    print(f"Fast-forwarding {args.fast_forward} instructions")
    exit_event = m5.simulate(args.fast_forward)
    print("Switching CPU mode")
    system.switchCpus(system.cpu, system.fast_forward_cpu)

exit_event = m5.simulate()
print(f"Exiting @ tick {m5.curTick()} because {exit_event.getCause()}")
