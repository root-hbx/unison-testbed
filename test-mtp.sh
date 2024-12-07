#!/bin/bash

mkdir -p ./fat-tree-data/ori-data
mkdir -p ./fat-tree-data/mtp-data

echo "Cleaning ns3 for safety..."
./ns3 clean

for k in 8; do
    for c in 24; do
        # Create command strings first
        clean_cmd="./ns3 clean"
        
        if [ $c -le 24 ]; then
            config_cmd="./ns3 configure -d optimized --enable-modules applications,flow-monitor,mpi,mtp,nix-vector-routing,point-to-point --enable-mtp"
            run_cmd="./ns3 run \"fat-tree \
            --k=$k \
            --cluster=$c \
            --delay=3000 \
            --bandwidth=100Gbps \
            --flow=False \
            --incast=1 \
            --victim=$(seq -s- 0 $((k*k/4-1))) \
            --time=0.1 \
            --interval=0.01 \
            --flowmon=False \
            --thread=$c\""
        else
            config_cmd="./ns3 configure -d optimized --enable-modules applications,flow-monitor,mpi,mtp,nix-vector-routing,point-to-point --enable-mtp --enable-mpi"
            thread=$((c / (c / 24)))
            hosts_needed=$((c / 24))
            host_list=""
            for i in $(seq 2 $((hosts_needed + 1))); do
                if [ -n "$host_list" ]; then
                    host_list="${host_list},"
                fi
                host_list="${host_list}172.16.0.${i}:24"
            done
            
            run_cmd="mpirun -n $hosts_needed --map-by ppr:1:node --bind-to none --host $host_list \
            ./ns3 run \"fat-tree \
            --k=$k \
            --cluster=$c \
            --delay=3000 \
            --bandwidth=100Gbps \
            --flow=False \
            --incast=1 \
            --victim=$(seq -s- 0 $((k*k/4-1))) \
            --time=0.1 \
            --interval=0.01 \
            --flowmon=False \
            --thread=$thread\""
        fi

        # Write commands to file first
        echo "=== Commands for k=$k, cluster=$c ===" > "./fat-tree-data/mtp-data/fat-tree-mtp-k${k}-c${c}.txt"
        
        echo "Clean command:" >> "./fat-tree-data/mtp-data/fat-tree-mtp-k${k}-c${c}.txt"
        echo "$clean_cmd" >> "./fat-tree-data/mtp-data/fat-tree-mtp-k${k}-c${c}.txt"
        
        echo -e "\nConfigure command:" >> "./fat-tree-data/mtp-data/fat-tree-mtp-k${k}-c${c}.txt"
        echo "$config_cmd" >> "./fat-tree-data/mtp-data/fat-tree-mtp-k${k}-c${c}.txt"
        
        echo -e "\nRun command:" >> "./fat-tree-data/mtp-data/fat-tree-mtp-k${k}-c${c}.txt"
        echo "$run_cmd" >> "./fat-tree-data/mtp-data/fat-tree-mtp-k${k}-c${c}.txt"
        
        echo -e "\n=== Execution Output ===\n" >> "./fat-tree-data/mtp-data/fat-tree-mtp-k${k}-c${c}.txt"

        # Execute commands
        echo "Configuring ns3 (mtp mode) for k=$k, cluster=$c..."
        eval "$clean_cmd"
        eval "$config_cmd"

        echo "Building fat-tree (mtp mode)..."
        ./ns3 build fat-tree

        sleep 20

        echo "Running test with k=$k, cluster=$c"
        eval "$run_cmd 2>&1 | tee -a \"./fat-tree-data/mtp-data/fat-tree-mtp-k${k}-c${c}.txt\""
        echo "Completed test with k=$k, cluster=$c"

        sleep 20
    done
done

echo "All fat-tree (mtp mode) tests completed!"
