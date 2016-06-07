# Stall Latency Flow Control Example 

This project shows how to build a kernel with a free-running compute path. 

Flow control is handled by stopping the input when the output stalls. This will ultimately cause the output data to stop.

The output stall latency needs to be set such that it allows for the compute path to flush through, without overflowing the downstream buffer.

