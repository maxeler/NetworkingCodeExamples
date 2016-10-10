# Frame Fragmenter 

This example takes a stream of frames and fragments each frame in to multiple smaller parts. 

It uses the fragmenter component from MaxPower. It configures it to take the Maximum Fragment size from a Scalar Input.


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

