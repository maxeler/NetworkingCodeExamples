#UDP One To Many example 

This is an Maxeler DFE networking example project.

## Running

Open the bitstream Java Project.

Open `UdpOneToManyManager.maxj` and execute the `main()` function.

```
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
$ cd sender
$ ./build.py
$ ./sender
```
