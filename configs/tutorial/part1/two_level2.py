# import the m5 (gem5) library created when gem5 is built
import m5
# import all of the SimObjects
from m5.objects import *

# Add the common scripts to our path
m5.util.addToPath('../../')

# import the caches which we made
from caches import *

import argparse
import os

thispath = os.path.dirname(os.path.realpath(__file__))
default_binary = os.path.join(
    thispath,
    "../../../",
    #'tests/test-progs/hello/bin/x86/linux/hello'
    "/home/kento/Documents/SPEC/benchspec/CPU/519.lbm_r/run/run_base_refrate_firstrun-m64.0000/lbm_r_base.firstrun-m64",
)

# Parse arguments
parser = argparse.ArgumentParser()
parser.add_argument("--cpu_type", default="X86TimingSimpleCPU", help="CPU model to use")
parser.add_argument("--fast_forward", type=int, default=0, help="Number of instructions to fast forward")
parser.add_argument("--l1i_size", default="16kB", help="L1 instruction cache size")
parser.add_argument("--l1d_size", default="64kB", help="L1 data cache size")
parser.add_argument("--l2_size", default="256kB", help="L2 cache size")
parser.add_argument("--binary", default=default_binary, help="Path to the binary to execute")


#SimpleOpts.add_option("binary", nargs="?", default=default_binary)

args = parser.parse_args()
print(f"Binary path: {args.binary}")
print(f"Binary exists: {os.path.exists(args.binary)}")
# create the system we are going to simulate
system = System()

# Set the clock frequency of the system (and all of its children)
system.clk_domain = SrcClockDomain()
system.clk_domain.clock = "1GHz"
system.clk_domain.voltage_domain = VoltageDomain()

# Set up the system
system.mem_mode = "timing"  # Use timing accesses
system.mem_ranges = [AddrRange("8192MB")]  # Create an address range

# Create a CPU
if args.fast_forward:
    system.cpu = X86AtomicSimpleCPU()
    system.cpu.max_insts_any_thread = args.fast_forward
    system.detailed_cpu = X86O3CPU()
else:
    system.cpu = X86O3CPU()

system.cpu.isa = [X86ISA()] * 1
system.cpu.numThreads = 1

# Create an L1 instruction and data cache
system.cpu.icache = L1ICache()
system.cpu.dcache = L1DCache()
#system.cpu.icache.configure(args.l1i_size)
#system.cpu.dcache.configure(args.l1d_size)

# Connect the instruction and data caches to the CPU
system.cpu.icache.connectCPU(system.cpu)
system.cpu.dcache.connectCPU(system.cpu)

# Create a memory bus, a coherent crossbar, in this case
system.l2bus = L2XBar()

# Hook the CPU ports up to the l2bus
system.cpu.icache.connectBus(system.l2bus)
system.cpu.dcache.connectBus(system.l2bus)

# Create an L2 cache and connect it to the l2bus
system.l2cache = L2Cache()
#system.l2cache.configure(args.l2_size)
system.l2cache.connectCPUSideBus(system.l2bus)

# Create a memory bus
system.membus = SystemXBar()

# Connect the L2 cache to the membus
system.l2cache.connectMemSideBus(system.membus)

# create the interrupt controller for the CPU and connect to the membus
system.cpu.createInterruptController()
system.cpu.interrupts[0].pio = system.membus.mem_side_ports
system.cpu.interrupts[0].int_requestor = system.membus.cpu_side_ports
system.cpu.interrupts[0].int_responder = system.membus.mem_side_ports

# Connect the system up to the membus
system.system_port = system.membus.cpu_side_ports

# Create a DDR3 memory controller and connect it to the membus
system.mem_ctrl = MemCtrl()
system.mem_ctrl.dram = DDR3_1600_8x8()
system.mem_ctrl.dram.range = system.mem_ranges[0]
system.mem_ctrl.port = system.membus.mem_side_ports

# Create a process for a simple "Hello World" application
process = Process()
# Set the command
# cmd is a list which begins with the executable (like argv)
process.cmd = [
  args.binary,
  "3000",
  "/home/kento/Documents/SPEC/benchspec/CPU/519.lbm_r/run/run_base_refrate_firstrun-m64.0000/reference.dat",
  "0",
  "0",
  "/home/kento/Documents/SPEC/benchspec/CPU/519.lbm_r/run/run_base_refrate_firstrun-m64.0000/100_100_130_ldc.of",
]

# Set the cpu to use the process as its workload and create thread contexts
system.cpu.workload =[process]*1
system.workload = SEWorkload.init_compatible(args.binary)
system.cpu.createThreads()

# set up the root SimObject and start the simulation
root = Root(full_system = False, system = system)

# instantiate all of the objects we've created above
m5.instantiate()

print("Beginning simulation!")
if args.fast_forward:
    print(f"Switching to detailed CPU model after {args.fast_forward} instructions")
    exit_event = m5.simulate()
    system.switchCpus(system.cpu, system.detailed_cpu)

exit_event = m5.simulate()
print(f"Exiting @ tick {m5.curTick()} because {exit_event.getCause()}")
