#ifndef DXL_ADC_H
#define DXL_ADC_H

#include "dxl.h"

#define DXL_ADC_MODEL 250

void dxl_adc_init(volatile struct dxl_device *device, ui8 id);

#endif // DXL_ADC_H
