#!/usr/bin/env bash

# Configuration paths
export MAXELEROSDIR=$MAXCOMPILERDIR/lib/maxeleros-sim
export LD_PRELOAD=$MAXELEROSDIR/lib/libmaxeleros.so:$LD_PRELOAD
export SLIC_CONF="$SLIC_CONF;use_simulation=${USER}Sim"
export PATH=$MAXCOMPILERDIR/bin:$MAXELEROSDIR/utils:$PATH

# Build & run
./build.py && ./build.py run_sim
