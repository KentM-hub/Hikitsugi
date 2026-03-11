import argparse
import time
import m5
from m5.objects import Root
from m5.stats.gem5stats import get_simstat

from gem5.utils.requires import requires
from gem5.components.boards.simple_board import SimpleBoard
from gem5.components.cachehierarchies.classic.private_l1_private_l2_cache_hierarchy import (
    PrivateL1PrivateL2CacheHierarchy,
)
from gem5.components.memory.single_channel import SingleChannelDDR4_2400
from gem5.components.processors.simple_processor import SimpleProcessor
from gem5.components.processors.cpu_types import CPUTypes
from gem5.isas import ISA
from gem5.resources.resource import obtain_resource
from gem5.simulate.simulator import Simulator

# We check for the required gem5 build.
requires(isa_required=ISA.X86)

# Define benchmark choices
benchmark_choices = [
    "500.perlbench_r", "502.gcc_r", "503.bwaves_r", "505.mcf_r",
    "507.cactusBSSN_r", "508.namd_r", "510.parest_r", "511.povray_r",
    "519.lbm_r", "520.omnetpp_r", "521.wrf_r", "523.xalancbmk_r",
    "525.x264_r", "527.cam4_r", "531.deepsjeng_r", "538.imagick_r",
    "541.leela_r", "544.nab_r", "548.exchange2_r", "549.fotonik3d_r",
    "554.roms_r", "557.xz_r", "600.perlbench_s", "602.gcc_s",
    "603.bwaves_s", "605.mcf_s", "607.cactusBSSN_s", "608.namd_s",
    "610.parest_s", "611.povray_s", "619.lbm_s", "620.omnetpp_s",
    "621.wrf_s", "623.xalancbmk_s", "625.x264_s", "627.cam4_s",
    "631.deepsjeng_s", "638.imagick_s", "641.leela_s", "644.nab_s",
    "648.exchange2_s", "649.fotonik3d_s", "654.roms_s", "996.specrand_fs",
    "997.specrand_fr", "998.specrand_is", "999.specrand_ir"
]

# Define input sizes
size_choices = ["test", "train", "ref"]

# Parse command-line arguments
parser = argparse.ArgumentParser(description="Run SPEC CPU2017 benchmarks in SE mode")
parser.add_argument("--benchmark", type=str, required=True, choices=benchmark_choices, help="Benchmark to run")
parser.add_argument("--size", type=str, required=True, choices=size_choices, help="Input size")
args = parser.parse_args()

# Set up the cache hierarchy
cache_hierarchy = PrivateL1PrivateL2CacheHierarchy(
    l1d_size="32kB",
    l1i_size="32kB",
    l2_size="256kB"
)

# Set up the memory system
memory = SingleChannelDDR4_2400(size="3GB")

# Set up the processor
processor = SimpleProcessor(cpu_type=CPUTypes.TIMING, num_cores=1, isa=ISA.X86)

# Create the board
board = SimpleBoard(
    clk_freq="3GHz",
    processor=processor,
    memory=memory,
    cache_hierarchy=cache_hierarchy
)

# Set up the workload
binary = "/home/kento/Documents/SPEC/benchspec/CPU/" + args.benchmark + "/executable"
binary_resource =CustomResource(binary)
board.set_se_binary_workload(binary_resource)

# Set up the simulator
simulator = Simulator(board=board)

# Run the simulation
print(f"Running {args.benchmark} with {args.size} input")
start_tick = simulator.get_current_tick()
simulator.run()
end_tick = simulator.get_current_tick()

# Print statistics
print("\nSimulation complete")
print(f"Simulated ticks: {end_tick - start_tick}")
print(f"Simulated seconds: {(end_tick - start_tick) / 1e12}")

# Dump statistics
m5.stats.dump()
