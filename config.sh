#!/bin/bash

THIS_SCRIPT=$BASH_SOURCE

if [ "$BASH_SOURCE" = "$0" ]
        then
                echo "This script must be sourced!"
                exit 1
fi

export MAXCOMPILERDIR=/network-raid/opt/maxcompiler-2014.1.1
export MAXELEROSDIR=$MAXCOMPILERDIR/lib/maxeleros-sim
export LD_PRELOAD=$MAXELEROSDIR/lib/libmaxeleros.so:$LD_PRELOAD
export SLIC_CONF="$SLIC_CONF;use_simulation=${USER}Sim"
export PATH=$MAXCOMPILERDIR/bin:$MAXELEROSDIR/utils:$PATH

EXAMPLESDIR=`dirname $THIS_SCRIPT`

export EXAMPLESDIR=`readlink -e $EXAMPLESDIR`

echo Setting EXAMPLESDIR to $EXAMPLESDIR

export PYTHONPATH="$EXAMPLESDIR/utils:$PYTHONPATH"
