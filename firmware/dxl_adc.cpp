#include <stdlib.h>
#include <wirish/wirish.h>
#include <string.h>
#include "dxl_adc.h"
#include "dxl_protocol.h"

// A/D
int directPins[] = {3, 4, 5, 6, 7};
#define DIRECTPINS (sizeof(directPins)/sizeof(int))

#define ADC_CHANNELS 5
#define ADC_WINDOW 16
#define ADC_ADDR 0x24

/**
 * Samples a channel (abstracting the multiplexer)
 *  - 0 -> 7: direct pins
 */
int sampleChannel(int channel)
{
    if (channel < DIRECTPINS) {
        return analogRead(directPins[channel]);
    }
}

/**
 * This flag is set to true @1Khz (see below)
 */
static bool sample = false;

struct dxl_adc_private 
{
    struct dxl_registers registers;
    unsigned int sums[ADC_CHANNELS];
    unsigned int windows[ADC_CHANNELS][ADC_WINDOW];
};

/**
 * The tick function do a sample if necessary and store it in the 
 * data stucture
 */
static void dxl_adc_tick(volatile struct dxl_device *self)
{
    volatile struct dxl_adc_private *data = (volatile dxl_adc_private *)self->data;
    static int currentChannel = 0;
    static int cursor = 0;

    if (sample) {
        sample = false;
        currentChannel = 0;
        cursor = (cursor+1);
        if (cursor >= ADC_WINDOW) {
            cursor = 0;
        }
    }

    if (currentChannel < ADC_CHANNELS) {
        int channel = currentChannel;
        int newSample = sampleChannel(channel);
        data->sums[channel] -= data->windows[channel][cursor];
        data->windows[channel][cursor] = newSample;
        data->sums[channel] += data->windows[channel][cursor];
        currentChannel++;
    }
}

static bool dxl_adc_check_id(volatile struct dxl_device *self, ui8 id)
{
    volatile struct dxl_adc_private *data = (volatile dxl_adc_private *)self->data;
    return (id == data->registers.eeprom.id);
}

static void dxl_adc_write_data(volatile struct dxl_device *self, ui8 id, ui8 addr, ui8 *values, ui8 length)
{
    volatile struct dxl_adc_private *data = (volatile dxl_adc_private *)self->data;
    
    memcpy(((ui8 *)(&data->registers))+addr, values, length);
}

static void dxl_adc_read_data(volatile struct dxl_device *self, ui8 id, ui8 addr, ui8 *values, ui8 length, ui8 *error)
{
    volatile struct dxl_adc_private *data = (volatile dxl_adc_private *)self->data;

    if (addr == ADC_ADDR) {
        unsigned short adcValues[ADC_CHANNELS];
        for (int i=0; i<ADC_CHANNELS; i++) {
            adcValues[i] = data->sums[i]/ADC_WINDOW;
        }
        memcpy(values, ((ui8*)adcValues), length);
    } else {
        memcpy(values, ((ui8*)&data->registers)+addr, length);
    }
    *error = 0;
}

static void dxl_adc_process(volatile struct dxl_device *self, volatile struct dxl_packet *packet)
{
    dxl_process(self, packet, dxl_adc_check_id, dxl_adc_write_data, dxl_adc_read_data);
}

static void timer_tick()
{
    sample = true;
}

static void dxl_adc_init_hardware()
{
    // Timer1 @1Khz
    HardwareTimer timer(1);
    timer.pause();
    timer.setPrescaleFactor(7200);
    timer.setOverflow(10);
    timer.setCompare(1, 1);
    timer.attachInterrupt(1, timer_tick);
    timer.refresh();
    timer.resume();
    
    // All the direct sampling pins are input analog
    for (int i=0; i<DIRECTPINS; i++) {
        pinMode(directPins[i], INPUT_ANALOG);
    }
}

void dxl_adc_init(volatile struct dxl_device *device, ui8 id)
{
    int i, j;
    static bool timer_inited = false;

    if (!timer_inited) {
        timer_inited = true;
        dxl_adc_init_hardware();
    }

    volatile struct dxl_adc_private *data = (volatile struct dxl_adc_private *)malloc(sizeof(struct dxl_adc_private));
    dxl_device_init(device);
    device->tick = dxl_adc_tick;
    device->process = dxl_adc_process;
    device->data = (void *)data;

    data->registers.eeprom.modelNumber = DXL_ADC_MODEL;
    data->registers.eeprom.firmwareVersion = 1;
    data->registers.eeprom.id = id;
    data->registers.ram.presentPosition = 0;
    data->registers.ram.goalPosition = 0;
    data->registers.ram.torqueEnable = 0;
    data->registers.ram.torqueLimit = 0;

    for (i=0; i<ADC_CHANNELS; i++) {
        data->sums[i] = 0;

        for (j=0; j<ADC_WINDOW; j++) {
            data->windows[i][j] = 0;
        }
    }
}
