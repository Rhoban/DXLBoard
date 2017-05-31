#include <stdlib.h>
#include <wirish/wirish.h>
#include <string.h>
#include "dxl_led.h"
#include "dxl_protocol.h"

static bool dxl_check_id(volatile struct dxl_device *self, ui8 id)
{
    volatile struct dxl_registers *registers = (volatile struct dxl_registers*)self->data;
    return (id == registers->eeprom.id);
}

static void dxl_write_data(volatile struct dxl_device *self, ui8 id, ui8 addr, ui8 *values, ui8 length)
{
    volatile struct dxl_registers *registers = (volatile struct dxl_registers*)self->data;
    memcpy(((ui8 *)(registers))+addr, values, length);
}

static void dxl_read_data(volatile struct dxl_device *self, ui8 id, ui8 addr, ui8 *values, ui8 length, ui8 *error)
{
    volatile struct dxl_registers *registers = (volatile struct dxl_registers*)self->data;

    registers->ram.presentPosition = digitalRead(BOARD_BUTTON_PIN) ? 1024 : 0;

    memcpy(values, ((ui8*)registers)+addr, length);
    *error = 0;
}

static void dxl_led_process(volatile struct dxl_device *self, volatile struct dxl_packet *packet)
{
    volatile struct dxl_registers *registers = (volatile struct dxl_registers*)self->data;
    dxl_process(self, packet, dxl_check_id, dxl_write_data, dxl_read_data);

    if (registers->ram.led) {
        digitalWrite(BOARD_LED_PIN, HIGH);
    } else {
        digitalWrite(BOARD_LED_PIN, LOW);
    }
}

void dxl_led_init(volatile struct dxl_device *device, ui8 id)
{
    volatile struct dxl_registers *registers = (volatile struct dxl_registers *)malloc(sizeof(struct dxl_registers));
    dxl_device_init(device);
    device->process = dxl_led_process;
    device->data = (void *)registers;

    registers->eeprom.modelNumber = 500;
    registers->eeprom.firmwareVersion = 1;
    registers->eeprom.id = id;
}
