#include <stdlib.h>
#include <wirish/wirish.h>
#include <string.h>
#include "dxl_coder.h"
#include "dxl_protocol.h"
#include "coder.h"

static bool dxl_coder_check_id(volatile struct dxl_device *self, ui8 id)
{
    volatile struct dxl_registers *registers = (volatile struct dxl_registers*)self->data;

    return (id == registers->eeprom.id);
}

static void dxl_coder_write_data(volatile struct dxl_device *self, ui8 id, ui8 addr, ui8 *values, ui8 length)
{
    volatile struct dxl_registers *registers = (volatile struct dxl_registers*)self->data;
    
    memcpy(((ui8 *)(registers))+addr, values, length);
}

static void dxl_coder_read_data(volatile struct dxl_device *self, ui8 id, ui8 addr, ui8 *values, ui8 length, ui8 *error)
{
    volatile struct dxl_registers *registers = (volatile struct dxl_registers*)self->data;

    registers->ram.presentPosition = coder_get_raw_value()+2048;
    memcpy(values, ((ui8*)registers)+addr, length);
    *error = 0;
}

static void dxl_coder_process(volatile struct dxl_device *self, volatile struct dxl_packet *packet)
{
    dxl_process(self, packet, dxl_coder_check_id, dxl_coder_write_data, dxl_coder_read_data);
}

void dxl_coder_init(volatile struct dxl_device *device, int pinA, int pinB, int pinZ, ui8 id)
{
    coder_init(pinA, pinB, pinZ);

    volatile struct dxl_registers *registers = (volatile struct dxl_registers *)malloc(sizeof(struct dxl_registers));
    dxl_device_init(device);
    device->process = dxl_coder_process;
    device->data = (void *)registers;

    registers->eeprom.modelNumber = DXL_CODER_MODEL;
    registers->eeprom.firmwareVersion = 1;
    registers->eeprom.id = id;
    registers->ram.presentPosition = 0;
    registers->ram.goalPosition = 0;
    registers->ram.torqueEnable = 0;
    registers->ram.torqueLimit = 0;
}
