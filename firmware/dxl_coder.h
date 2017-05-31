#ifndef DXL_CODER_H
#define DXL_CODER_H

#include "dxl.h"

#define DXL_CODER_MODEL 251

void dxl_coder_init(volatile struct dxl_device *device, int pinA, int pinB, int pinZ, ui8 id);

#endif // DXL_ADC_H
