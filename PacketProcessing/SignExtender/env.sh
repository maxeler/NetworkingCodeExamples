export MAXCOMPILERDIR=/network-raid/opt/maxcompiler-2014.1.1
export MAXELEROSDIR=$MAXCOMPILERDIR/lib/maxeleros-sim
export LD_PRELOAD=$MAXELEROSDIR/lib/libmaxeleros.so:$LD_PRELOAD
export SLIC_CONF="$SLIC_CONF;use_simulation=${USER}Sim"
export PATH=$MAXCOMPILERDIR/bin:$MAXELEROSDIR/utils:$PATH

