# SST-Spatter
SST-Spatter is an external element that simulates gather and scatter operations by replaying Spatter traces, also known as Spatter patterns. It is based on the Miranda element and incorporates the Spatter library to support the same configuration options that Spatter uses to specify one or more patterns.

For more details about Spatter and its configuration options, see [Spatter](https://github.com/hpcgarage/spatter/blob/main/README.md).

## Dependencies
* SST 14.1.0
* Spatter 2.1+
* Supported C++17 compiler (GCC, Clang)
* GNU Make
* GNU Autoconf 2.59+
* GNU Automake 1.9.6+
* GNU Libtool 1.5.22+
* Python 3.6+
* pandas

## Building SST-Spatter

See [INSTALL.md](INSTALL.md) for build and installation instructions.

## Running SST-Spatter

SST-Spatter supports the same command-line arguments as Spatter; see [Running Spatter](https://github.com/hpcgarage/spatter/blob/main/README.md#running-spatter) for details. In addition to these arguments, the statistics level and statistics output file can be specified with the `--statlevel` and `--statfile` flags.

Note: the SST-Spatter command-line arguments must be preceded by `--` so they can be appended to the model options and passed to the SST configuration file.

```
sst tests/sst_spatter_spr.py -- -p UNIFORM:8:1 -l$((2**16))

0:cpu:RequestGenCPU[RequestGenCPU:43]: Configured CPU to allow 16 maximum Load requests to be memory to be outstanding.
0:cpu:RequestGenCPU[RequestGenCPU:45]: Configured CPU to allow 16 maximum Store requests to be memory to be outstanding.
0:cpu:RequestGenCPU[RequestGenCPU:47]: Configured CPU to allow 16 maximum Custom requests to be memory to be outstanding.
0:cpu:RequestGenCPU[RequestGenCPU:54]: CPU clock configured for 3.5GHz
0:cpu:RequestGenCPU[RequestGenCPU:60]: Memory interface to be loaded is: memHierarchy.standardInterface
0:cpu:RequestGenCPU[RequestGenCPU:75]: Loaded memory interface successfully.
0:cpu:RequestGenCPU[RequestGenCPU:103]: Generator loaded successfully.
0:cpu:RequestGenCPU[RequestGenCPU:160]: Miranda CPU Configuration:
0:cpu:RequestGenCPU[RequestGenCPU:161]: - Max requests per cycle:         5
0:cpu:RequestGenCPU[RequestGenCPU:162]: - Max reorder lookups             256
0:cpu:RequestGenCPU[RequestGenCPU:163]: - Clock:                          3.5GHz
0:cpu:RequestGenCPU[RequestGenCPU:164]: - Cache line size:                64 bytes
0:cpu:RequestGenCPU[RequestGenCPU:165]: - Max Load requests pending:      16
0:cpu:RequestGenCPU[RequestGenCPU:166]: - Max Store requests pending:     16
0:cpu:RequestGenCPU[RequestGenCPU:167]: - Max Custom requests pending:     16
0:cpu:RequestGenCPU[RequestGenCPU:168]: Configuration completed.
0:TimingDRAM::TimingDRAM():52:mc=0: number of channels: 3
0:TimingDRAM::TimingDRAM():53:mc=0: address mapper:     memHierarchy.roundRobinAddrMapper
0:TimingDRAM:Channel:Channel():111:mc=0:chan=0: max pending trans: 32
0:TimingDRAM:Channel:Channel():112:mc=0:chan=0: number of ranks:   2
0:TimingDRAM:Rank:Rank():221:mc=0:chan=0:rank=0: number of banks: 16
0:TimingDRAM:Bank:Bank():289:mc=0:chan=0:rank=0:bank=0: CL:           40
0:TimingDRAM:Bank:Bank():290:mc=0:chan=0:rank=0:bank=0: CL_WR:        39
0:TimingDRAM:Bank:Bank():291:mc=0:chan=0:rank=0:bank=0: RCD:          39
0:TimingDRAM:Bank:Bank():292:mc=0:chan=0:rank=0:bank=0: TRP:          39
0:TimingDRAM:Bank:Bank():293:mc=0:chan=0:rank=0:bank=0: dataCycles:   4
0:TimingDRAM:Bank:Bank():294:mc=0:chan=0:rank=0:bank=0: transactionQ: memHierarchy.reorderTransactionQ
0:TimingDRAM:Bank:Bank():295:mc=0:chan=0:rank=0:bank=0: pagePolicy:   memHierarchy.simplePagePolicy
Simulation is complete, simulated time: 737.562 us
```

This will generate output to both the command-line and a statistics file, named `stats.csv` by default.

## SST-Spatter Output
The output to the command-line contains SST configuration information, which is specified in the SST configuration file.

The statistics file contains the SST-Spatter statistics output for the simulated runs, saved in a CSV file format.

```
head -n 20 stats.csv

ComponentName, StatisticName, StatisticSubId, StatisticType, SimTime, Rank, Sum.u64, SumSQ.u64, Count.u64, Min.u64, Max.u64
cpu, read_reqs, , Accumulator, 737642906, 0, 466944, 466944, 466944, 1, 1
cpu, write_reqs, , Accumulator, 737642906, 0, 524288, 524288, 524288, 1, 1
cpu, custom_reqs, , Accumulator, 737642906, 0, 0, 0, 0, 0, 0
cpu, split_read_reqs, , Accumulator, 737642906, 0, 57344, 57344, 57344, 1, 1
cpu, split_write_reqs, , Accumulator, 737642906, 0, 0, 0, 0, 0, 0
cpu, split_custom_reqs, , Accumulator, 737642906, 0, 0, 0, 0, 0, 0
cpu, completed_reqs, , Accumulator, 737642906, 0, 1048576, 1048576, 1048576, 1, 1
cpu, cycles_with_issue, , Accumulator, 737642906, 0, 581632, 581632, 581632, 1, 1
cpu, cycles_no_issue, , Accumulator, 737642906, 0, 1997538, 1997538, 1997538, 1, 1
cpu, total_bytes_read, , Accumulator, 737642906, 0, 4194304, 33554432, 524288, 8, 8
cpu, total_bytes_write, , Accumulator, 737642906, 0, 4194304, 33554432, 524288, 8, 8
cpu, total_bytes_custom, , Accumulator, 737642906, 0, 0, 0, 0, 0, 0
cpu, req_latency, , Accumulator, 737642906, 0, 12380594, 544433674, 1105920, 1, 136
cpu, time, , Accumulator, 737642906, 0, 0, 0, 0, 0, 0
cpu, cycles_hit_fence, , Accumulator, 737642906, 0, 0, 0, 0, 0, 0
cpu, cycles_max_issue, , Accumulator, 737642906, 0, 6, 6, 6, 1, 1
cpu, cycles_max_reorder, , Accumulator, 737642906, 0, 0, 0, 0, 0, 0
cpu, cycles, , Accumulator, 737642906, 0, 2579171, 2579171, 2579171, 1, 1
cpu:generator, configTime, , Accumulator, 737642906, 0, 737642906, 544117056772124836, 1, 737642906, 737642906
```

To compare this output with Spatter runs, you can use the [spatter_stats.py](tools/spatter_stats.py) helper script to convert the data in the statistics file into Spatter-like statistics.

```
echo '[{"pattern": "UNIFORM:8:1", "count": '"$((2**16))"'}]' > trace.json
python3 tools/spatter_stats.py stats.csv trace.json

config         bytes          time(s)        bw(MB/s)       cycles         
0              4194304        0.000737643    5686.09        2579171
```
