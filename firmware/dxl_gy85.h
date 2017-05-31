#ifndef DXL_GY85_H
#define DXL_GY85_H

#include <wirish.h>
#include "dxl.h"
#include <i2c.h>

#define DXL_GY85_MODEL 350

void dxl_gy85_init(volatile struct dxl_device *device, ui8 id, i2c_dev *dev);

#endif 
