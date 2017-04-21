# Binary FIFO Gateway example

Create a FIFO Ordering exchange gateway for an arbitrary binary protocol which includes a length field at a fixed offset in the header.


The length is stored at byte offset 7 and is 6 bytes long.

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
