# Latency Test 

This test uses low latency PCIe streams.
It measures the round trip latency between the CPU and the DFE. 




## How to build

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
./build.py run_sim
```

