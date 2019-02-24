#ifndef DXL_SERIAL_H
#define DXL_SERIAL_H

#include "dxl.h"

#define DXL_DEFAULT_BAUDRATE 2000000

void dxl_serial_init(volatile struct dxl_device *device, int index);

#endif // DXL_SERIAL_H
