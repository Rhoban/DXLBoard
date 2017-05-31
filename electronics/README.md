# Electronics

Here you'll find the schematic and (auto)routed board. You will need Eagle
to open the `.sch` and `.brd` files.

## Components

Here is a list of required components to build the board

Name               | Farnell ID | Quantity | Description
-------------------|------------|----------|--------------
2212S-20SG-85      | 1593469    | 2        | 1x20 0.1" female sockets for the Maple Mini
2212S-08SG-85      | 1593463    | 1        | 1x8 0.1" female socket for the IMU
MOLEX 22-03-5045   | 1104204    | 3        | Dynamixel 4 pins connector
MOLEX 22-03-5035   | 9979620    | 3        | Dynamixel 3 pins connector
ST485              | 1564306    | 3        | Buses transciever (can be replaced with MAX485), SOIC-8
R0805 22 ohms      | 2128982    | 8        | Protection resistors
R0805 1.5 Kohms    | 2129125    | 1        | USB detection resistor
R0805 2 Kohms      | 2129135    | 3        | Pull-up/Pull-down
R0805 3 Kohms      | 2129150    | 15       | Pull-up/Pull-down
C0805 100 nF       | 1759265RL  | 3        | Decoupling condensators
PMEG2005AEA        | 8737843RL  | 1        | Diode for on-board USB power supply
Maple mini      | N/A        | 1        | Controller board, featuring a 72Mhz 32-bit Cortex-M3 ARM controller
GY-85           | N/A        | 1        | 9 DOF IMU

## Powering

In the current version, the board is powered using 5V USB. You can use the mini
USB port from the Maple Mini, or solder wires to the USB part of the board (we
recommend doing that to have a better integration). There is protection diode
on the Maple Mini and on the board.

The motor power lines are connected, however the board is not designed to
handle current so you still have to build a separate hub if you want to
power servomotors. 

## Bus design

The ST485 transciever is used here for both RS485 and TTL versions. It can also
be replaced with MAX485 which is pin compatible.

22 ohms serie resistors are used for bus protections to avoid damaging transcivers from
the devices or the board in the case the bus would be driven simultaneously by both.

There is a pull-up on the A line and a the B line is pulled to 2.5V. 

When transmitting, the lines are driven by the ST485, A is the positive output and B
the negative. In this case, the A line acts exactly as the data line of a TTL device.

When receiving, if you are using only 3-pins TTL, the A line will be either 5V (for '1')
or 0V (for '0') and the B line will remain at 2.5V. In this case, the RS485 speficiation
is still respected, because A-B should be above 200mV to read a '1' and below -200mV to
read a '0'.
