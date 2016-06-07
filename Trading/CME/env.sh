THIS_SCRIPT=$BASH_SOURCE

if [ "$BASH_SOURCE" = "$0" ]
	then
		echo "This script must be sourced!"
		exit 1
fi

SCRIPTDIR=`dirname $THIS_SCRIPT`

export MAXMPTDIR=$(readlink -ef ${SCRIPTDIR}/MaxMPT-2016.1.0)
