Ethernet Forwarding
====================


This example receives a UDP packet, checks that it's destined for the DFE, and if it is:
Forwards it to a different IP address.


However, this is done on the Ethernet level - that is, it does not use the higher level network stack blocks.
This is done to illustrate how one can process raw ethernet frames.


