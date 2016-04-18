source ../../../utils/config.sh
/usr/bin/env python ./build_consumers.py $*
/usr/bin/env python ./build_multicast.py $*
/usr/bin/env python ./build.py $*
