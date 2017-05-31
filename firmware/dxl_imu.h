#ifndef DXL_IMU_H
#define DXL_IMU_H

#include <wirish.h>
#include "dxl.h"

#define DXL_IMU_MODEL 253

void dxl_imu_init(volatile struct dxl_device *device, ui8 id, HardwareSerial *port);

#endif 
