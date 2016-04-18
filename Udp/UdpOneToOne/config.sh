#!/bin/bash

THIS_SCRIPT=$BASH_SOURCE
THIS_SCRIPT_DIR="$(readlink -e `dirname $THIS_SCRIPT`)"

if [ "$BASH_SOURCE" = "$0" ]
then
    echo "This script must be sourced!"
    exit 1
fi

export MAXELEROSDIR=$MAXCOMPILERDIR/lib/maxeleros-sim
export LD_PRELOAD=$MAXELEROSDIR/lib/libmaxeleros.so:$LD_PRELOAD
export SLIC_CONF="$SLIC_CONF;use_simulation=${USER}Sim"
export PATH=$MAXCOMPILERDIR/bin:$MAXELEROSDIR/utils:$PATH
export PYTHONPATH="$THIS_SCRIPT_DIR/../../utils:$PYTHONPATH"
