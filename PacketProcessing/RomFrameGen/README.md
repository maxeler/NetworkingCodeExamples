# ROM based Frame Generator 


Assumptions:
* Each input frame contains a field denoting its message type
* There are 20 possible message types (As configured in the parameter `numMessageTypes`)

Operation:
* Each input message type will make the kernel generate a different output frame.
* The output frame data is stored inside a Mapped ROM called "frameRom"
* An additional ROM called "metadataRom" will store like the position inside the "frameRom" that we want to use


In pseudo code;
```
metadata = metadaRom[inputMessageType] 
for (OutputWord w : frameRom[metadata.startIndex .. metadata.endIndex]) {
	output(w)	
}
```


## How to build

### Environment
```bash
source utils/config.sh
source utils/maxenv.sh
```


### Bitstream
```bash
cd bitstream
ant sim # for Simulation
ant dfe # for Hardware
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

