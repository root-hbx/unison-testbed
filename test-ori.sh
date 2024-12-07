#!/bin/bash

mkdir -p ./fat-tree-data/ori-data
mkdir -p ./fat-tree-data/mtp-data

# Note: this time mtp and ori scripts are totally different.
# Differ: 1) configuration cmd 2) --thread
echo "Cleaning ns3 for safety..."
./ns3 clean
echo "Configuring ns3 (ori mode)..."
./ns3 configure -d optimized --enable-modules applications,flow-monitor,mpi,mtp,nix-vector-routing,point-to-point --enable-examples
echo "Building fat-tree (ori mode)..."
./ns3 build fat-tree-ori

sleep 20

for k in 8 16; do
  for c in 8 16; do
    cmd="./ns3 run \"fat-tree-ori \
    --k=$k \
    --cluster=$c \
    --delay=3000 \
    --bandwidth=100Gbps \
    --flow=false \
    --incast=1 \
    --victim=$(seq -s- 0 $((k*k/4-1))) \
    --time=0.1 \
    --interval=0.01 \
    --flowmon=false\" \
    2>&1 | tee \"./fat-tree-data/ori-data/fat-tree-ori-k${k}-c${c}.txt\""

    echo "Running test with k=$k, cluster=$c"
    eval $cmd
    echo "Completed test with k=$k, cluster=$c"

    sleep 20
  done
done

echo "All fat-tree-ori tests completed!"
