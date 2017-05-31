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
PMEG2005AEA        | 8737843RL  | 1        | Diode for on-board USB power supply
Maple mini      | N/A        | 1        | Controller board, featuring a 72Mhz 32-bit Cortex-M3 ARM controller
GY-85           | N/A        | 1        | 9 DOF IMU


