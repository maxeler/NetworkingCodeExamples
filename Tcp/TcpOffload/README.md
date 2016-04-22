# TCP Offload Example

This project shows how to use MaxTcp.
It instantiates MaxTcp in Hardware the controls it through software.

Data is sent from software and received back to software.


## How to build

### Environment
```bash
source utils/config.sh
```

### Bitstream
```bash
cd bitstream
ant
```

### Runtime
```bash
cd runtime
cp <maxfile> .
./build.py
```


## How to run
```bash
cd runtime
./myApp
```


