# FIFO Gateway example

Create a FIFO ordering exchange gateway for the FIX protocol. 

The gateway derives the message length by inspecting the body length field (tag 9) of the incoming FIX message.


## Running

Open the bitstream Java Project.

Open `FIFOGatewayManager.maxj` and execute the `main()` function.

```
$ source config.sh
$ cd runtime
$ cp <path-to-new-max-file> .
$ ./build.py
<...>
$ ./run.py
<...>
```

In a separate terminal window, run the following:

```
$ cd ..
$ source config.sh
$ cd sender
$ ./build.py
$ ./sender
```
