# Udp Forwarding

This example takes a a UDP multicast feed as its input.  
When an incoming packet arrives, it performs some minimal processing to figure out if and where to forward it.  
The destination is called a consumer and data is sent to consumers over UDP.  
  
  


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
````


## How to run

```bash
cd runtime
./build.py run_sim
```



