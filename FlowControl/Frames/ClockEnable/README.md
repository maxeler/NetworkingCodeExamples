# Kernel Lite - Clock Enable 

Flow control example based on the Clock Enable paradigm. 


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

