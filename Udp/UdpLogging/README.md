# UDP Logging Example 

This example simply takes in a UDP Multicast feed, counts the number of input frames and measures the input packet size and sends that information to software.

## How to build

### Environment
```bash
source utils/config.sh
source utils/maxenv.sh
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
./build.py run_sim
```

then in a different terminal:
```bash
./build_multicast.py
./run_multicast.sh
```

