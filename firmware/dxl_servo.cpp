#include <stdlib.h>
#include <wirish/wirish.h>
#include <string.h>
#include "dxl_servo.h"
#include "dxl_protocol.h"

struct dxl_servo_private 
{
    ui8 pin;
    struct dxl_registers registers;
};

static bool dxl_servo_check_id(volatile struct dxl_device *self, ui8 id)
{
    volatile struct dxl_servo_private *data = (volatile dxl_servo_private *)self->data;
    return (id == data->registers.eeprom.id);
}

static void dxl_servo_write_data(volatile struct dxl_device *self, ui8 id, ui8 addr, ui8 *values, ui8 length)
{
    int oldTorque;
    volatile struct dxl_servo_private *data = (volatile dxl_servo_private *)self->data;
    
    oldTorque = data->registers.ram.torqueLimit;

    memcpy(((ui8 *)(&data->registers))+addr, values, length);

    if (oldTorque < data->registers.ram.torqueLimit) {
        data->registers.ram.torqueEnable = 1;
    }

    if (!data->registers.ram.torqueEnable) {
        data->registers.ram.torqueLimit = 0;
    }
    
    if (data->registers.ram.torqueLimit > 0) {
        pwmWrite(data->pin, data->registers.ram.goalPosition*8);
    } else {
        pwmWrite(data->pin, 0);
    }
}

static void dxl_servo_read_data(volatile struct dxl_device *self, ui8 id, ui8 addr, ui8 *values, ui8 length, ui8 *error)
{
    volatile struct dxl_servo_private *data = (volatile dxl_servo_private *)self->data;

    data->registers.ram.presentPosition = data->registers.ram.goalPosition;

    memcpy(values, ((ui8*)&data->registers)+addr, length);
    *error = 0;
}

static void dxl_servo_process(volatile struct dxl_device *self, volatile struct dxl_packet *packet)
{
    dxl_process(self, packet, dxl_servo_check_id, dxl_servo_write_data, dxl_servo_read_data);
}

static void dxl_servo_init_timers()
{
    HardwareTimer timer(4);
    timer.pause();
    timer.setPrescaleFactor(SERVOS_TIMERS_PRESCALE);
    timer.setOverflow(SERVOS_TIMERS_OVERFLOW);
    timer.refresh();
    timer.resume();
}

void dxl_servo_init(volatile struct dxl_device *device, ui8 pin, ui8 id)
{
    static bool timers_inited = false;

    if (!timers_inited) {
        timers_inited = true;
        dxl_servo_init_timers();
    }

    volatile struct dxl_servo_private *data = (volatile struct dxl_servo_private *)malloc(sizeof(struct dxl_servo_private));
    dxl_device_init(device);
    device->process = dxl_servo_process;
    device->data = (void *)data;

    data->registers.eeprom.modelNumber = DXL_SERVO_TYPE;
    data->registers.eeprom.firmwareVersion = 1;
    data->registers.eeprom.id = id;
    data->registers.ram.presentPosition = 0;
    data->registers.ram.goalPosition = 0;
    data->registers.ram.torqueEnable = 0;
    data->registers.ram.torqueLimit = 0;
    data->pin = pin;

    pinMode(pin, PWM);
    pwmWrite(pin, 0);
}
