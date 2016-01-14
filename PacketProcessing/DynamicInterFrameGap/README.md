# Add Header Example

This example will insert a 'gap' packet every time it sees a packet arrive that has it's 20th byte (0 base) set to 'G'



For example, the original packet could be 28 bytes of Data (D):

```
7 6 5 4 3 2 1 0
D D D D D D D D
D D D D D D D D
D D D G D D D D
- - - - D D D D
```

The output, will be the original packet, followed by a new packet of length 100. 
The new packet will have a GapIndicator metadata field set.
 
 
 Architecture:
 
 ![Architecture]
 (Architecture.png)
 
