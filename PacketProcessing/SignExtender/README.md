# Sign Extension Example

An example project, which will receive packets containing 3 signed integer values of variable lengths, convert them to a fixed size and pass them to the CPU. Since the values are signed, we must consider sign extension.

The structure of the incoming packet *data* field should be as follows:

* First byte
    * defines the size of the following content (see table below)
* Rest of data
    * contains the three values A, B and C (in that order)
    * where n = size of A + size of B + size of C
    * little endian

Allocation of bits in the first data byte:

Bits  | 7 - 6 | 5 - 3 | 2 - 0
:---- |:-----:|:-----:|:-----:
Usage | size of C | size of B | size of A

These values indicate the sizes in bytes. If any of these sizes are equal to 0, this indicates the maximum possible size for that variable. E.g. for the size of B, 0x0 would indicate a length of 8 bytes.

The values passed to the CPU should be as follows:

* A : 64 bit signed integer
* B : 64 bit signed integer
* C : 32 bit signed integer

## Expected behaviour

The following two scenarios are used in this example (values set in `sender.c`).

* Packet 1:

First byte : 0x43

Variable | Size | Value | Expected output to CPU
:-------:|:----:|:-----:|:---------------------:
A | 3 | 0x800201 | 0xffffffffff800201
B | 0 | 0x7766554433221100UL | 0x7766554433221100
C | 1 | 0x80 | 0xffffff80

* Packet 2:

First byte : 0xCE

Variable | Size | Value | Expected output to CPU
:-------:|:----:|:-----:|:---------------------:
A | 6 | 0x800102030405UL | 0xffff800102030405
B | 1 | 0x78 | 0x78
C | 3 | 0x818283 | 0xff818283

## Building & running

### Bitstream

```
$ cd bitstream
$ ant
<...>
$ cd ..
```

This will build the max file. When the command completes, it will display the path to the new max file. Copy this into the `runtime` directory.

### Runtime

Ensure that the maxfile output from the previous step has been copied to the `runtime` directory.

```
$ cd runtime
$ ./run.sh
<...>
Waiting for kernel response...
```

### Packet sender

Open a new terminal and inject some packets:

```
$ cd sender
$ ./build.py
<...>
$ ./sender
Sender finished
```

## Verifying output

In the first terminal, you should see the received packet details:

```
Kernel got: aSz = 3, bSz = 0, cSz = 1
Kernel got: aSz = 6, bSz = 1, cSz = 3
CPU: Got output frame 1 - size 20 bytes
Frame [1] Word[0]: 0xffffffffff800201
Frame [1] Word[1]: 0x7766554433221100
Frame [1] Word[2]: 0xffffff80
CPU: Got output frame 2 - size 20 bytes
Frame [2] Word[0]: 0xffff800102030405
Frame [2] Word[1]: 0x78
Frame [2] Word[2]: 0xff818283
```

The application will continue to wait for more packets, so when you are finished, hit <kbd>ctrl</kbd> + <kbd>c</kbd> to exit.

### Viewing all traffic

This requires that *wireshark* is installed.

Verify that a network trace has been captured by running `ls` and looking for `top1.pcap`.

```
$ ls
<...>
top1.pcap
<...>
$ wireshark top1.pcap &
```

You can view only the packets of interest by filtering with the search term `ip.dst==172.16.50.1`.
