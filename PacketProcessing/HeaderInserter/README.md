# Add Header Example

This example will add a fixed size header on to every incoming frame.
It will take the information to put in to the header from the CPU.


For example, the original packet could be 28 bytes of Data (D):

```
7 6 5 4 3 2 1 0
D D D D D D D D
D D D D D D D D
D D D D D D D D
- - - - D D D D
```

We would like to add a fixed length header (H), of length 19 bytes:

```
7 6 5 4 3 2 1 0
H H H H H H H H
H H H H H H H H
- - - - - H H H
```

The combined, new packet is of length 28+19=47 bytes:

```
7 6 5 4 3 2 1 0
H H H H H H H H
H H H H H H H H
D D D D D H H H
D D D D D D D D
D D D D D D D D
- D D D D D D D
```


We had to realign the data as well as move the SOF/EOF flags.
