        Linux Module - character device driver

    Please follow the steps below to build & install the module.

Dynamic allocate ver:

    [1. Build the module]
    # make

    [2. Load the module]
    # sudo insmod char_dev.ko

    [3. Get the major number]
    # dmesg
    (It will show you the <major number>, it is 248 in my env.)

    [4. Create device node]
    # sudo mknod /dev/char_dev c <major number> 0

    [5. Run testing program]
    # sudo ./a.out

Static ver:

    [1. Build the module]
    # make ccflags-y+=-DSTATIC

    [2. Load the module]
    # sudo insmod char_dev.ko

    [3. Create device node]
    # sudo mknod /dev/char_dev c 70 0

    [4. Run testing program]
    # sudo ./a.out