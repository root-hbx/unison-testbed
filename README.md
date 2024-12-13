# Unison Platform for MSCCL

## Original Unison README

For reference, please consult the [UNISON-for-ns-3 (NJU)](https://github.com/NASA-NJU/UNISON-for-ns-3) repository.

## Unison for MSCCL

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
