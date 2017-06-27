Networking Code Examples
=========================

This repository contains examples for networking code.

## Dependencies

MaxPower is required by some of these examples and is included as a git submodule within the `lib` directory. This must be compiled before building the example bitstreams.

Building
---------

## Submodule dependencies

Some projects require the `maxpower` and `project utils` submodules to be present. To ensure that this is available, run the following commands:

```
$ git submodule init
$ git submodule update
```


## Environment

You will need to source the submodule environment files:

```bash
source lib/maxpower/config.sh
source lib/maxpowernet/config.sh
source utils/config.sh
source utils/maxenv.sh
```


