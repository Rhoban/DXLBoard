#ifndef DXL_PROXY_H
#define DXL_PROXY_H

#include "dxl.h"

#define DXL_PROXY_MODEL 254
#define DXL_PROXY_LIST 0x1E
#define DXL_PROXY_VALUES 0x24
#define DXL_PROXY_REGISTER_START 0x24
#define DXL_PROXY_LENGTH 8
#define DXL_PROXY_MONITORLIST_MAXSIZE 200

void dxl_proxy_init(volatile struct dxl_device *device, volatile struct dxl_device *serial_device, ui8 id);

#endif // DXL_PROXY_H
