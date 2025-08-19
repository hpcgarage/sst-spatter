################### Sources ###################
# This config is compiled from various sources including
# - Anandtech: Intel Xeon Sapphire Rapids: How To Go Monolithic with Tiles
# - Anandtech: Intel Architecture Day 2021: Alder Lake, Golden Cove, and Gracemont Detailed
# - wccftech: Intel Sapphire Rapids-SP Xeon CPU Lineup Detaield: Platinum & HBM Variants with Over 350W TDP, C740 Chipset Compatibility (2022-05-15)
# https://wccftech.com/intel-sapphire-rapids-sp-xeon-amd-epyc-7773x-milan-x-cpu-cache-memory-benchmarks-leak/
# - Wikipedia (accessed 7/1/2022 and 4/11/2024)
# - LANL arXiv paper: arxiv.org/pdf/2211/05712.pdf
# - https://wccftech.com/intel-4th-gen-xeon-cpus-official-sapphire-rapids-up-to-60-cores-8-socket-scalability-350w-tdp-17000-usd/
# - https://chipsandcheese.com/2023/03/12/a-peek-at-sapphire-rapids/
# - https://www.ixpug.org/images/docs/ISC23/McCalpin_SPR_BW_limits_2023-05-24_final.pdf
##############################################


from typing import Any
import sst
from sst import UnitAlgebra
import sys
import argparse


def remove_none(map: dict[str, Any]) -> dict[str, Any]:
    """Utility method to remove None values from map"""
    return {key: value for key, value in map.items() if value is not None}


# Parse commandline arguments
parser = argparse.ArgumentParser()
parser.add_argument("--statfile", help="statistics file", default="./stats.csv")
parser.add_argument("--statlevel", help="statistics level", type=int, default=16)


parser.add_argument("--prefetch-l1-history", type=int, default=16)
parser.add_argument("--prefetch-l1-reach", type=int, default=2)
parser.add_argument("--prefetch-l1-detect-range", type=int, default=4)
parser.add_argument("--prefetch-l1-address-count", type=int, default=64)

parser.add_argument("--prefetch-l2-history", type=int, default=16)
parser.add_argument("--prefetch-l2-reach", type=int, default=2)
parser.add_argument("--prefetch-l2-detect-range", type=int, default=4)
parser.add_argument("--prefetch-l2-address-count", type=int, default=64)


parser.add_argument("--l1-max-outstanding-prefetch", type=int)


# orignal default is 48
parser.add_argument("--l2-mshr-num-entries", type=int, default=-1)

# orignal default is 72
parser.add_argument("--l3-mshr-num-entries", type=int, default=-1)


args, unknown = parser.parse_known_args()

statFile = args.statfile
statLevel = args.statlevel

# Define SST core options
sst.setProgramOption("timebase", "1ps")
sst.setProgramOption("output-config", "./config.py")
sst.setProgramOption("output-json", "./config.json")

load_queue = 240
store_queue = 112
load_per_cycle = 3
store_per_cycle = 2
l1_ltu_latency = 5  # ~1.4ns (5 cycles @ turbo)
l1_latency = l1_ltu_latency - 2  # account for cycles of transfer to and from cache
protocol = "mesi"

freq_turbo = "3.4GHz"

l1_cache_params = remove_none(
    {
        "cache_frequency": freq_turbo,
        "coherence_protocol": "mesi",
        "cache_size": "48KiB",
        "associativity": 12,
        "access_latency_cycles": l1_latency,  # Assume parallel tag/data lookup so no separate tag latency
        "mshr_num_entries": 16,  # this is invaid since this is l1
        "max_outstanding_prefetch": args.l1_max_outstanding_prefetch,
        "maxRequestDelay": 1000000000,  # if a request is delayed for 1M cycles there's a problem
        "events_up_per_cycle": load_per_cycle
        + store_per_cycle,  # Not perfect, could result in 4 loads
        "mshr_latency_cycles": 1,  # Trivial at 16 entries, but still a guess
        "L1": 1,
    }
)

l1_prefetcher_params = {
    "detect_range": args.prefetch_l1_detect_range,
    "reach": args.prefetch_l1_reach,
    "history": args.prefetch_l1_history,
    "address_count": args.prefetch_l1_address_count,
}

l2_ltu_latency = 16  # ~5ns
l2_tag_latency = 2  # Guess
l2_latency = l2_ltu_latency - l1_ltu_latency - l2_tag_latency - 2

l2_cache_params = {
    "cache_frequency": freq_turbo,
    "coherence_protocol": protocol,
    "cache_size": "2MiB",
    "associativity": 16,
    # Total load-to-use = l2_latency
    "access_latency_cycles": l2_latency
    - 2,  # Total is l2_latency, assuming serial tag/data lookup so split
    "tag_access_latency_cycles": 2,
    "mshr_num_entries": args.l2_mshr_num_entries,
    "events_up_per_cycle": 1,
    "mshr_latency_cycles": 1,  # Trivial at 16 entries, but still a guess
}

l3_cache_params = {
    "cache_frequency": freq_turbo,
    "coherence_protocol": protocol,
    "cache_size": "1875KiB",
    "associativity": 15,
    "access_latency_cycles": 26,
    "tag_latency_cycles": 4,  # Guesss
    "mshr_num_entries": args.l3_mshr_num_entries,
    "mshr_latency_cycles": 4,  # Guess
}

## Memory - DDR5 @ 4800MT/s
mem_channels = 8
mem_capacity = UnitAlgebra("16GiB")  # Per-channel (8 channels total)
mem_page_size = UnitAlgebra("4KiB")
mem_pages = mem_capacity * UnitAlgebra(mem_channels) / mem_page_size
ddr_clock = "4800MHz"  # ddr5 4800
ddr_tCL = 40
ddr_tCWL = 39
ddr_tRCD = 39
ddr_tRP = 39

mem_timing_dram_params = {
    "addrMapper": "memHierarchy.roundRobinAddrMapper",
    "clock": ddr_clock,
    "channels": 3,
    "channel.numRanks": 2,
    "channel.transaction_Q_size": 32,
    "channel.rank.numBanks": 16,
    "channel.rank.bank.CL": ddr_tCL,
    "channel.rank.bank.CL_WR": ddr_tCWL,
    "channel.rank.bank.RCD": ddr_tRCD,
    "channel.rank.bank.TRP": ddr_tRP,
    "channel.rank.bank.dataCycles": 4,  # Cycles to return data (4 if burst8)
    "channel.rank.bank.pagePolicy": "memHierarchy.simplePagePolicy",
    "channel.rank.bank.transactionQ": "memHierarchy.reorderTransactionQ",
    "channel.rank.bank.pagePolicy.close": 0,
    "id": 0,
    "mem_size": mem_capacity,
}

miranda_params_cpu = {
    "verbose": 1,
    "printStats": 1,
    "clock": freq_turbo,
    "max_reqs_cycle": load_per_cycle + store_per_cycle,
    "max_reorder_lookups": 256,
    "maxmemreqpending": load_queue + store_queue,
    "pagesize": int(mem_page_size),
    "pagecount": mem_pages,
}


l2_prefetcher_params = {
    "detect_range": args.prefetch_l2_detect_range,
    "reach": args.prefetch_l2_reach,
    "history": args.prefetch_l2_history,
    "address_count": args.prefetch_l2_address_count,
}

# Define the simulation components
cpu = sst.Component("cpu", "sstSpatter.BaseCPU")
cpu.addParams(miranda_params_cpu)

gen = cpu.setSubComponent("generator", "sstSpatter.SpatterGenerator")
gen.addParams({"verbose": 2, "args": " ".join(unknown)})

l1_cache = sst.Component("l1cache", "memHierarchy.Cache")
l1_cache.addParams(l1_cache_params)
l1_cache.setSubComponent("replacement", "memHierarchy.replacement.lru")

prefetcher_l1 = l1_cache.setSubComponent("prefetcher", "cassini.StridePrefetcher")
prefetcher_l1.addParams(l1_prefetcher_params)

l2_cache = sst.Component("l2cache", "memHierarchy.Cache")
l2_cache.addParams(l2_cache_params)
l2_cache.setSubComponent("replacement", "memHierarchy.replacement.lru")
l2_cache.setSubComponent("nb_prefetcher", "cassini.NextBlockPrefetcher")
prefetcher_l2 = l2_cache.setSubComponent("prefetcher", "cassini.StridePrefetcher")
prefetcher_l2.addParams(l2_prefetcher_params)


l3_cache = sst.Component("l3cache", "memHierarchy.Cache")
l3_cache.addParams(l3_cache_params)
l3_cache.setSubComponent("replacement", "memHierarchy.replacement.random")

memctrl = sst.Component("memory", "memHierarchy.MemController")
memctrl.addParams({"clock": "1GHz", "addr_range_end": 4096 * 1024 * 1024 - 1})
memory = memctrl.setSubComponent("backend", "memHierarchy.timingDRAM")
memory.addParams(mem_timing_dram_params)

# Define the simulation links
link_cpu_l1 = sst.Link("link_cpu_l1")
link_l1_l2 = sst.Link("link_l1_l2")
link_l2_l3 = sst.Link("link_l2_l3")
link_l3_mem = sst.Link("link_l3_mem")

link_cpu_l1.connect((cpu, "cache_link", "100ps"), (l1_cache, "high_network_0", "100ps"))

link_l1_l2.connect(
    (l1_cache, "low_network_0", "100ps"), (l2_cache, "high_network_0", "100ps")
)

link_l2_l3.connect(
    (l2_cache, "low_network_0", "100ps"), (l3_cache, "high_network_0", "100ps")
)

link_l3_mem.connect(
    (
        l3_cache,
        "low_network_0",
        "100ps",
    ),
    (memctrl, "direct_link", "100ps"),
)

# Enable statistics
sst.setStatisticLoadLevel(statLevel)
sst.setStatisticOutput("sst.statOutputCSV", {"filepath": statFile})
sst.enableAllStatisticsForAllComponents({"type": "sst.AccumulatorStatistic"})
