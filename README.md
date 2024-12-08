# Unison Platform for msccl

## Original Unison README

[UNISON-for-ns-3 (NJU)](https://github.com/NASA-NJU/UNISON-for-ns-3)

## Unison for msccl

We add some basic tests for unison performance check.

### Prerequisites

Please implement these between different experiments for isolation.

```sh
./ns3 clean
./ns3 configure
./ns3 build
```

### Original DCTCP example

This part is totally same as that in original [unison example](https://github.com/NASA-NJU/UNISON-for-ns-3?tab=readme-ov-file#getting-started).

```sh
./ns3 configure --enable-mtp --enable-examples
./ns3 build dctcp-example dctcp-example-mtp
time ./ns3 run dctcp-example
time ./ns3 run dctcp-example-mtp
```

__Data on Sugon Server__:

|no mtp|with mtp|ratio|
|:---:|:---:|:---:|
|14m44.299s|4min06s|3.68|

### Reproduction for Fig 1

This is a reproduced version of Figure 1 from the original paper, conducted on a single Sugon server with only `--enable-mtp` enabled, without utilizing `--enable-mpi`.

```sh
cd ~ # unison-msccl
./run-tests.sh
# check ~/fat-tree-data
```

These scripts will open 2 tmux sessions, running `test-mtp.sh` and `test-ori.sh` respectively.

You can check `~/fat-tree-data`.

__Data on Sugon Server__:

|  k  |  c  |  no mtp  |   mtp    | ratio |
| :-: | :-: | :------: | :------: | ----- |
|  8  |  8  | 6923.66s | 455.54s  | 15.20 |
|  8  | 16  | 16574.4s | 412.331s | 40.20 |
| 16  |  8  |          | 7509.47s |       |
| 16  | 16  |          | 10103.5s |       |

### ns-3-dev with mtp kernel

[ns-3-dev (mtp branch)](https://github.com/majinchao2002/ns-3-dev/tree/mtp)
