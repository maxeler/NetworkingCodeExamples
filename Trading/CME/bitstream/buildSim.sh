#!/bin/bash

if [ -z "$MAXMPTDIR" ]; then
	echo "MAXMPTDIR must be set to the location of the uncompressed MaxMPT release directory"
	exit 1
fi

maxJavaRun CmeTradingManager MPPREndCT=4 target=DFE_SIM
