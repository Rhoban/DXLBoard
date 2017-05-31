#include <stdlib.h>
#include <wirish/wirish.h>
#include <string.h>
#include "dxl_magnetic_coder.h"
#include "dxl_protocol.h"
#include "coder.h"

#define MAGN_OVERFLOW 50000
#define MAGN_WINDOW_SIZE 16

static HardwareTimer timer(DXL_MAGNETIC_CODER_TIMER);

struct magnetic_coder {
    int pin;
    int rise_time;
    int fall_time;
    int state;
    int values[MAGN_WINDOW_SIZE];
    int position;
    int sum;
    volatile struct dxl_registers *registers;
};

static struct magnetic_coder coders[16];
static int coders_count;

static int mt_diff(int a, int b)
{
    int diff = (a-b);
    if (diff < 0) {
        diff *= -1;
    }

    if (diff > (MAGN_OVERFLOW/2)) {
        return MAGN_OVERFLOW-diff;
    } else {
        return diff;
    }
}

static void magnetic_eval(int i)
{
    int now = timer.getCount();
    int total = mt_diff(now, coders[i].rise_time);
    int pulse = mt_diff(coders[i].rise_time, coders[i].fall_time);

    if (total > 3500 && total < 4500) {
        int newValue = (4096*pulse)/total;
        int delta = newValue-coders[i].values[coders[i].position];
        coders[i].sum += delta;
        coders[i].values[coders[i].position] = newValue;
        coders[i].position++;
        if (coders[i].position >= MAGN_WINDOW_SIZE) {
            coders[i].position = 0;
        }
        coders[i].registers->ram.presentPosition = (coders[i].sum/MAGN_WINDOW_SIZE);
    } 
}

static void magnetic_rising(int i)
{
    magnetic_eval(i);
    coders[i].rise_time = timer.getCount();
}

static void magnetic_falling(int i)
{
    coders[i].fall_time = timer.getCount();
}

static void magnetic_pin_change()
{
    int i;
    for (i=0; i<coders_count; i++) {
        int state = digitalRead(coders[i].pin);
        if (state != coders[i].state) {
            if (state == LOW) {
                magnetic_falling(i);
            } else {
                magnetic_rising(i);
            }
            coders[i].state = state;
        }
    }
}

static bool dxl_magnetic_coder_check_id(volatile struct dxl_device *self, ui8 id)
{
    volatile struct dxl_registers *registers = (volatile struct dxl_registers*)self->data;

    return (id == registers->eeprom.id);
}

static void dxl_magnetic_coder_write_data(volatile struct dxl_device *self, ui8 id, ui8 addr, ui8 *values, ui8 length)
{
    volatile struct dxl_registers *registers = (volatile struct dxl_registers*)self->data;
    memcpy(((ui8 *)(registers))+addr, values, length);
}

static void dxl_magnetic_coder_read_data(volatile struct dxl_device *self, ui8 id, ui8 addr, ui8 *values, ui8 length, ui8 *error)
{
    volatile struct dxl_registers *registers = (volatile struct dxl_registers*)self->data;
    memcpy(values, ((ui8*)registers)+addr, length);
    *error = 0;
}

static void dxl_magnetic_coder_process(volatile struct dxl_device *self, volatile struct dxl_packet *packet)
{
    dxl_process(self, packet, dxl_magnetic_coder_check_id, dxl_magnetic_coder_write_data, dxl_magnetic_coder_read_data);
}

static volatile bool magnetic_initialized = false;

static void magnetic_initialize()
{
    magnetic_initialized = true;
    timer.pause();
    timer.setPrescaleFactor(72);
    timer.setOverflow(MAGN_OVERFLOW);
    timer.refresh();
    timer.resume();
}

void dxl_magnetic_coder_init(volatile struct dxl_device *device, int pin, ui8 id)
{
    if (!magnetic_initialized) {
        magnetic_initialize();
    }

    volatile struct dxl_registers *registers = (volatile struct dxl_registers *)malloc(sizeof(struct dxl_registers));
    dxl_device_init(device);
    device->process = dxl_magnetic_coder_process;
    device->data = (void *)registers;

    registers->eeprom.modelNumber = DXL_MAGNETIC_CODER_MODEL;
    registers->eeprom.firmwareVersion = 1;
    registers->eeprom.id = id;
    registers->ram.presentPosition = 0;
    registers->ram.goalPosition = 0;
    registers->ram.torqueEnable = 0;
    registers->ram.torqueLimit = 0;

    coders[coders_count].registers = registers;
    coders[coders_count].pin = pin;
    coders[coders_count].sum = 0;
    coders[coders_count].position = 0;
    coders_count++;
    pinMode(pin, INPUT);
    attachInterrupt(pin, magnetic_pin_change, CHANGE);
}
