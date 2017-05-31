#ifndef DXL_MAGNETIC_CODER_H
#define DXL_MAGNETIC_CODER_H

#include "dxl.h"

#define DXL_MAGNETIC_CODER_MODEL 252
#define DXL_MAGNETIC_CODER_TIMER 2

void dxl_magnetic_coder_init(volatile struct dxl_device *device, int pin, ui8 id);

#endif 
