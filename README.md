# Unison Platform for MSCCL

## Original Unison README

For reference, please consult the [UNISON-for-ns-3 (NJU)](https://github.com/NASA-NJU/UNISON-for-ns-3) repository.

## Unison Validation

We have introduced a set of foundational tests to evaluate the performance of the Unison platform.

### (0) Prerequisites

To ensure isolation between different experimental runs, execute the following steps prior to each experiment:

```sh
./ns3 clean
./ns3 configure
./ns3 build
```

### (1) Original DCTCP Example

This section remains identical to the original [Unison example](https://github.com/NASA-NJU/UNISON-for-ns-3?tab=readme-ov-file#getting-started).

```sh
./ns3 configure --enable-mtp --enable-examples
./ns3 build dctcp-example dctcp-example-mtp
time ./ns3 run dctcp-example
time ./ns3 run dctcp-example-mtp
```

**Performance Data (Obtained on Sugon Server):**

| No MTP | With MTP | Ratio |
|:------:|:--------:|:-----:|
| 14m44.299s | 4m06s | 3.68 |

### (2) Reproduction of Figure 1

This section reproduces Figure 1 from the original publication. The experiments were conducted on a single Sugon server with only the `--enable-mtp` flag enabled. The `--enable-mpi` flag was not utilized.

```sh
cd ~ # Navigate to the Unison-MSCCL directory
./run-tests.sh
# Output is stored in ~/fat-tree-data
```

These scripts initialize two tmux sessions, executing `test-mtp.sh` and `test-ori.sh`, respectively.

Performance metrics can be found in the `~/fat-tree-data` directory.

**Performance Data (Obtained on Sugon Server):**

|  k  |  c  | No MTP (s) | MTP (s) | Ratio |
|:---:|:---:|:----------:|:-------:|:-----:|
|  8  |  8  | 6923.66    | 455.54  | 15.20 |
|  8  | 16  | 16574.4    | 412.33  | 40.20 |
| 16  |  8  | 131269 | 7509.47 | 17.48 |
| 16  | 16  |       299513     | 10103.5 |    29.65   |

### (3) Modified ns-3-dev with MTP Kernel

The modified `ns-3-dev` implementation (to better fix with arguments and mechanism in Unison Fig.1), incorporating the MTP kernel, can be accessed via the [mtp branch](https://github.com/majinchao2002/ns-3-dev/tree/mtp).

**Performance Data (Obtained on Sugon Server):**

|  k  | t_flow | No MTP (s) | MTP (s) | Ratio |
|:---:|:------:|:----------:|:-------:|:-----:|
|  4  |   1    | 41489      | 8794    | 4.72  |
|  8  |   1    | 299513 | 144773 | 6.38 |
| 12  |   1    |            |  |       |
| 16  |   1    |            |  |       |

### (4) Original ns-3-dev with MTP Kernel

The original `ns-3-dev` implementation, enhanced with the MTP kernel, is also accessible via the [mtp branch](https://github.com/majinchao2002/ns-3-dev/tree/mtp).

The ultimate objective is to leverage the MTP kernel to accelerate the original experiments conducted in `ns-3-dev`. Preliminary evaluations have confirmed the functional effectiveness of the `--enable-mtp` flag. 

🙌 Further assessments will focus on quantifying the practical performance improvements. 🤩

**Data Collection in Progress:**

|  k  | t_flow | No MTP (s) | MTP (s) | Ratio |
|:---:|:------:|:----------:|:-------:|:-----:|
|     |        |            |         |       |
|     |        |            |         |       |
|     |        |            |         |       |
|     |        |            |         |       |


> *Last updated: 2024-12-11 19:29 (berkeley time)*

## Hierarchy Analysis

### Module Documentation

#### 1. Overview

Unison for ns-3 is mainly implemented in the `mtp` module (located at `src/mtp/*`), which stands for multi-threaded parallelization.
This module contains three parts: A parallel simulator implementation `multithreaded-simulator-impl`, an interface to users `mtp-interface`, and `logical-process` to represent LPs in terms of parallel simulation.

All LPs and threads are stored in the `mtp-interface`.
It controls the simulation progress, schedules LPs to threads and manages the lifecycles of LPs and threads.
The interface also provides some methods and options for users to tweak the simulation.

Each LP's logic is implemented in `logical-process`. It contains most of the methods of the default sequential simulator plus some auxiliary methods for parallel simulation.

The simulator implementation `multithreaded-simulator-impl` is a derived class from the base simulator.
It converts calls to the base simulator into calls to logical processes based on the context of the current thread.
It also provides a partition method for automatic fine-grained topology partition.

For distributed simulation with MPI, we added `hybrid-simulator-impl` in the `mpi` module (located at `src/mpi/model/hybrid-simulator-impl*`).
This simulator uses both `mtp-interface` and `mpi-interface` to coordinate local LPs and global MPI communications.
We also modified the module to make it locally thread-safe.

#### 2. Modifications to ns-3 Architecture

In addition to the `mtp` and `mpi` modules, we also modified the following part of the ns-3 architecture to make it thread-safe, also with some bug fixing for ns-3.

You can find the modifications to each unison-enabled ns-3 version via `git diff unison-* ns-*`.

- [x] Modifications to the build system to provide `--enable-mtp` option to enable/disable Unison:

```
ns3                                                |    2 +
CMakeLists.txt                                     |    1 +
build-support/custom-modules/ns3-configtable.cmake |    3 +
build-support/macros-and-definitions.cmake         |   10 +
```

配置参数指令罢了

- [x] Modifications to the `core` module to make reference counting thread-safe:

```
src/core/CMakeLists.txt                            |    1 +
src/core/model/atomic-counter.h                    |   50 +
src/core/model/hash.h                              |   16 +
src/core/model/object.cc                           |    2 +
src/core/model/simple-ref-count.h                  |   11 +-
```

定义原子操作，保证线程安全，不是核心

Modifications to the `network` module to make packets thread-safe:

```
src/network/model/buffer.cc                        |   15 +-
src/network/model/buffer.h                         |    7 +
src/network/model/byte-tag-list.cc                 |   14 +-
src/network/model/node.cc                          |    7 +
src/network/model/node.h                           |    7 +
src/network/model/packet-metadata.cc               |   26 +-
src/network/model/packet-metadata.h                |   14 +-
src/network/model/packet-tag-list.h                |   11 +-
src/network/model/socket.cc                        |    6 +
```

Modifications to the `internet` module to make it thread-safe and add per-flow ECMP routing:

```
src/internet/model/global-route-manager-impl.cc    |    2 +
src/internet/model/ipv4-global-routing.cc          |   32 +-
src/internet/model/ipv4-global-routing.h           |    8 +-
src/internet/model/ipv4-packet-info-tag.cc         |    2 +
src/internet/model/ipv6-packet-info-tag.cc         |    2 +
src/internet/model/tcp-option.cc                   |    2 +-
```

Modifications to the `flow-monitor` module to make it thread-safe:

```
src/flow-monitor/model/flow-monitor.cc             |   48 +
src/flow-monitor/model/flow-monitor.h              |    4 +
src/flow-monitor/model/ipv4-flow-classifier.cc     |   12 +
src/flow-monitor/model/ipv4-flow-classifier.h      |    5 +
src/flow-monitor/model/ipv4-flow-probe.cc          |    2 +
src/flow-monitor/model/ipv6-flow-classifier.cc     |   12 +
src/flow-monitor/model/ipv6-flow-classifier.h      |    5 +
src/flow-monitor/model/ipv6-flow-probe.cc          |    2 +
```

Modifications to the `nix-vector-routing` module to make it thread-safe:

```
src/nix-vector-routing/model/nix-vector-routing.cc |   92 ++
src/nix-vector-routing/model/nix-vector-routing.h  |    8 +
```

Modifications to the `mpi` module to make it thread-safe with the hybrid simulator:

```
src/mpi/model/granted-time-window-mpi-interface.cc |   25 +
src/mpi/model/granted-time-window-mpi-interface.h  |    7 +
src/mpi/model/mpi-interface.cc                     |    3 +-
```

#### 3. Logging

The reason behind Unison's fast speed is that it divides the network into multiple logical processes (LPs) with fine granularity and schedules them dynamically.
To get to know more details of such workflow, you can enable the following log component:

```c++
LogComponentEnable("LogicalProcess", LOG_LEVEL_INFO);
LogComponentEnable("MultithreadedSimulatorImpl", LOG_LEVEL_INFO);
```

#### 4. Advanced Options

These options can be modified at the beginning of the `main` function using the native config syntax of ns-3.

You can also change the default maximum number of threads by setting

```c++
Config::SetDefault("ns3::MultithreadedSimulatorImpl::MaxThreads", UintegerValue(8));
Config::SetDefault("ns3::HybridSimulatorImpl::MaxThreads", UintegerValue(8));
```

The automatic partition will cut off stateless links whose delay is above the threshold.
The threshold is automatically calculated based on the delay of every link.
If you are not satisfied with the partition results, you can set a custom threshold by setting

```c++
Config::SetDefault("ns3::MultithreadedSimulatorImpl::MinLookahead", TimeValue(NanoSeconds(500));
Config::SetDefault("ns3::HybridSimulatorImpl::MinLookahead", TimeValue(NanoSeconds(500));
```

The scheduling method determines the priority (estimated completion time of the next round) of each logical process.
There are five available options:

- `ByExecutionTime`: LPs with a higher execution time of the last round will have higher priority.
- `ByPendingEventCount`: LPs with more pending events of this round will have higher priority.
- `ByEventCount`: LPs with more pending events of this round will have higher priority.
- `BySimulationTime`: LPs with larger current clock time will have higher priority.
- `None`: Do not schedule. The partition's priority is based on their ID.

Many experiments show that the first one usually leads to better performance.
However, you can still choose one according to your taste by setting

```c++
GlobalValue::Bind("PartitionSchedulingMethod", StringValue("ByExecutionTime"));
```

By default, the scheduling period is 2 when the number of partitions is less than 16, 3 when it is less than 256, 4 when it is less than 4096, etc.
Since more partitions lead to more scheduling costs.
You can also set how frequently scheduling occurs by setting

```c++
GlobalValue::Bind("PartitionSchedulingPeriod", UintegerValue(4));
```
