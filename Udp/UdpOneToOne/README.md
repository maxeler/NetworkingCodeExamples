#UDP One To One example 

Example of using UDP One To One Streams.

## Running

Open the bitstream Java Project.

Open `UdpOneToOneManager.maxj` and execute the `main()` function.

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
