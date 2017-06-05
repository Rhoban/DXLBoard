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

## Reading data from the IMU

The GY-85 IMU is accessed using I²C bus and the raw values can be retrieved the same
way you read from a dynamixel device.

The dynamixel model of the imu will be `350`.

To read the IMU, you should ask for 50 bytes from the `0x24` address. Here is what you
will receive:


Field           | Size             | Encoding   
----------------|------------------|--------------------------
Accelerometer X | 16 bits signed   | Values for the accelerometer (raw)
Accelerometer Y | 16 bits signed   | 
Accelerometer Z | 16 bits signed   | 
Gyrometer X     | 16 bits signed   | Values for the gyrometer (raw)
Gyrometer Y     | 16 bits signed   | 
Gyrometer Z     | 16 bits signed   | 
Magnetometer X  | 16 bits signed   | Values for the magnetometer (raw)
Sequence        | 32 bits unsigned | Sequence number

This block of data is repeated 5 times, because the values are stored in a circular
buffer. Thus, the higher sequence number is the more recent data.

The circular buffer is used to ensure that you don't miss any packet, since dynamixel
is a master/slave protocol.

* [ADXL345](http://www.analog.com/media/en/technical-documentation/data-sheets/ADXL345.pdf): Values from the accelerometer are 256 LSB per `g`.

* [ITG-3200](https://www.sparkfun.com/datasheets/Sensors/Gyro/PS-ITG-3200-00-01.4.pdf) Values from the gyrometer are 14.375 LSB per `deg/s`. We recommend you also calibrate the offset for its 0 value to avoid drift.

