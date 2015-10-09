# SignExtenderWithPatternMatching example

This project is an extension of the original `SignExtender` example. That project will not be explained again here.

## Enhancements

The packets have an additional text field of fixed length.

If this text field contains a specified substring, then the packet will be forwarded on via UDP.

Whether it matches or not, it is still processed as before.

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
All frames sent
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

Returning to the terminal where the sender is running, it should show the following, as the first packet was forwarded back. Note that the data matches that originally sent.

```
Got response '0x430102800011223344556677806162635945533030'
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

You can view only the packets of interest by filtering with the search term `ip.dst==172.16.50.1 || ip.dst==172.16.50.10`.
