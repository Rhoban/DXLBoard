# Firmware

Here is the Maple Mini firmware for the DXL Board, it is based on the
Rhoban fork of the LibMaple

## Building it

First, you will need the ARM cortex toolchain:

```
sudo apt-get install libstdc++-arm-none-eabi-newlib gcc-arm-none-eabi libnewlib-arm-none-eabi binutils-arm-none-eabi dfu-util
```

Then, run the following commands:

```
./prepare.sh
make
make install
```

`make install` will send the program to the board using the DFU maple bootloader.

## How does it works

The board will appear as a virtual serial port (typically `/dev/ttyACM0` on linux).
You can directly send dynamixel packet through it.

## Sync Read

To take advantage of the 3 buses, there is a pseudo Sync Read command implemented
in the controller.

Note that you'll have to configure your servos to have the smallest possible 
return delay time.

The mapping of what device is on which bus is done on-the-fly when a response
is received. Thus, you may want to ping each device at the begining to be sure
the mapping is OK (it's a good practice anyway to check that all the devices are
present).

If you want to read `N` bytes at address `A` from `M` motors, here is the instruction
packet:

ID             | Instruction | Length | Data
---------------|-------------|--------|---------------------------------
Broadcast      | `0x84`      | M+2    | `A`, `N`, `ID1`, `ID2` ... `IDM`

Then, the board will send regular read packets on the buses, in parallel. Currently,
the timeout for these reads is hard coded `650 µS`.

The response will then be:

ID             | Instruction | Length | Data
---------------|-------------|--------|---------------------------------
Broadcast      | `0x84`      | (1 + `N`) * `M`    | `STATUS1`, `DATA1` (`N` bytes), `STATUS2`, `DATA2` ... `STATUSM`, `DATAM`

The `STATUSX` byte is the error field from the motor respond. In case of timeout, this
value will be `0xFF`.

