# Udp Forwarding

This example takes a UDP multicast feed as its input.  
When an incoming packet arrives, it performs some minimal processing to figure out if and where to forward it.  
The destination is called a consumer and data is sent to consumers over UDP.  
  
  


## How to build

### Environment

```bash
source utils/config.sh
```


### Bitstream

```bash
cd bitstream
ant
```


### Runtime

```bash
cd runtime
cp <maxfile> .
./build.sh
```

This will build the UDP forwarding app, the consumers and the multicast generator.


## How to run


Start the UDP forwarding application:
```bash
cd runtime
./build.py run_sim
```

The UDP forwarding app takes the following arguments:

```
Usage: ./udpforwarding <Top IP> <Bot IP> <multicast_ip> <consumer_ip1> [consumer_ip2 consumer_ip3 ...]
Top IP - The IP address of the QSFP_TOP_10G_PORT1
Bot IP - The IP address of the QSFP_BOT_10G_PORT1
multicast ip - The destination IP address of the multicast feed 
consumer_ip1 - 1st consumer's IP address
Optional: consumer2_ip - 2nd consumer's IP address, etc   
```

By default, the arguments are:
```
Top IP = 172.16.50.1
Bot IP = 172.16.60.1
Multicast IP = 225.0.0.37
1st Consumer IP = 172.16.60.10
2nd Consumer IP  = 172.16.60.10
```
 
The simulator by default will start with these parameters:

```python
network_config = [
   { 'NAME' : 'QSFP_TOP_10G_PORT1', 'DFE': '172.16.50.1', 'TAP': '172.16.50.10', 'NETMASK' : '255.255.255.0' },
   { 'NAME' : 'QSFP_BOT_10G_PORT1', 'DFE': '172.16.60.1', 'TAP': '172.16.60.10', 'NETMASK' : '255.255.255.0' }
      ]
```

Which means the simulated TOP port will be connected to a TAP device that is assigned the IP address ```172.16.50.1```.  
The BOT port which will go to the consumers is connected to the TAP device with the IP address ```172.16.60.10```.  

The consumers app:
```bash
./consumers 2
```

will start 2 consumers listening on port 10000 and 10001

The Multicast generator:
```bash
./multicast 172.16.50.10 225.0.0.37
```

Will send data through the tap device associated with the TOP port. It will send to the multicast address ```225.0.0.37``` .


### Functionality

The application listens to a multicast feed.
When data arrives, it extract the message type from the payload (byte 7).
Based on the message type it makes a forwarding decision - DROP or PASS to a consumer
The message is then sent over to the specific consumer from the decision.

 



