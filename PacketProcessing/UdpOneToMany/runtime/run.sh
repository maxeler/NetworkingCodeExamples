#!/bin/bash


export MAXELEROSDIR=$MAXCOMPILERDIR/lib/maxeleros-sim
export LD_PRELOAD=$MAXELEROSDIR/lib/libmaxeleros.so:$LD_PRELOAD
export SLIC_CONF="$SLIC_CONF;use_simulation=${USER}Sim"

make run_sim

