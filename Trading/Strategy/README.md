# Trading Strategy Example 

This project shows how one could write and test a Trading Strategy using MaxCompiler.

## Description

Pre-canned market data will be streamed in to the strategy kernel.

The kernel will compute 3 signals:
 1. VWAP
 2. EMA
 3. Liveness

The signals are then combined to form a new execution signal.

If the execution signal exceeds a configurable threshold - the system will place a new order.

The new order will be recieved back in software.

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

