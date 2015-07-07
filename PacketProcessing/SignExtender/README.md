# PacketProcessing example

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

* A : 64 bit signed int
* B : 64 bit signed int
* C : 32 bit signed int

## Expected behaviour

The following two scenarios are used in this example (values set in `sender.c`).

* Packet 1:

Variable | Size | Value | Expected output to CPU
:-------:|:----:|:-----:|:---------------------:
A | 3 | 0x800201 | 0xffffffffff800201
B | 0 | 0x7766554433221100UL | 0x7766554433221100
C | 1 | 0x80 | 0xffffff80

* Packet 2:

Variable | Size | Value | Expected output to CPU
:-------:|:----:|:-----:|:---------------------:
A | 6 | 0x800102030405UL | 0xffff800102030405
B | 1 | 0x78 | 0x78
C | 3 | 0x818283 | 0xff818283

## Building

### Bitstreams

Open the bitstream Java Project.

Open `SignExtManager.maxj` and execute the `main()` function.

### Runtime

Copy the maxfile output from the Bitstream to the `runtime` directory.

```
$ source env.sh
$ cd runtime
$ ./build.py
/network-raid/opt/maxcompiler-2014.1.1/bin/sliccompile SignExt.max SignExt.o
Processing maxfile for MAX4AB24B_SIM from 'SignExt.max'.
gcc -std=gnu99 -Wall -Werror -fno-guess-branch-probability -frandom-seed=foo -Wno-unused-variable -Wno-unused-function -fPIC -I /network-raid/opt/maxcompiler-2014.1.1/include/slic -DMAXFILE_INC="/home/mhaslehurst/Workspaces/networking-demo/PacketProcessing/SignExtender/hostcode/SignExt.max" -DSLIC_NO_DESTRUCTORS -c /network-raid/opt/maxcompiler-2014.1.1/src/slicinterface/MaxFileInit.c -o SignExt.o 
Copying .max file C object into '/home/mhaslehurst/Workspaces/networking-demo/PacketProcessing/SignExtender/hostcode'
gcc -ggdb -O2 -fPIC -std=gnu99 -Wall -Werror -DDESIGN_NAME=SignExt -I. -I/network-raid/opt/maxcompiler-2014.1.1/lib/maxeleros-sim/include -I/network-raid/opt/maxcompiler-2014.1.1/include/slic -c signext.c -o signext.o
gcc signext.o -L/network-raid/opt/maxcompiler-2014.1.1/lib -L/network-raid/opt/maxcompiler-2014.1.1/lib/maxeleros-sim/lib -lslic -lmaxeleros -lm -lpthread SignExt.o -o signext
$ cd ..
```

### Packet sender

Build the auxiliary application, used to send the packets.

```
$ cd sender
$ ./build.py
gcc -ggdb -O2 -fPIC -std=gnu99 -Wall -Werror -c sender.c -o sender.o
gcc sender.o -o sender
$ cd ..
```

## Running

```
$ cd runtime
$ ./run.py
<...>
Listening on 172.16.50.1 port 2000
Waiting for kernel response...
```

Then open a new terminal and inject some packets:

```
$ sudo ./sender/sender
Sender finished
```

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

## Verifying output

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

## Next steps

Try to enhance the application by performing further processing on the DFE. Define a criteria for which matching packets should conditionally be forwarded back out over UDP.
