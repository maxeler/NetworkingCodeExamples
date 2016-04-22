# TCP Forwarding

Input: UDP Multicast Feed
Output: Same message over TCP to a selected consumer


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
./run_consumers.sh
./run_multicast.sh
```


