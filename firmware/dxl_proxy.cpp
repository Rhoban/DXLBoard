#include <stdlib.h>
#include <wirish/wirish.h>
#include <string.h>
#include "dxl_proxy.h"
#include "dxl_protocol.h"

struct dxl_proxy_private 
{
    struct dxl_registers registers;
    volatile struct dxl_device *serial_device;
    unsigned char values[DXL_PROXY_MONITORLIST_MAXSIZE*DXL_PROXY_LENGTH];
    ui8 monitorList[DXL_PROXY_MONITORLIST_MAXSIZE];
    ui8 monitorNb;
    ui8 current;
    unsigned int last_query;
    bool answer;
    bool stop;
    bool running;
    volatile struct dxl_packet queryPacket;
};

/**
 * The tick function do a sample if necessary and store it in the 
 * data stucture
 */
static void dxl_proxy_tick(volatile struct dxl_device *self)
{
    volatile struct dxl_proxy_private *data = (volatile dxl_proxy_private *)self->data;

    if (data->running) {
        if (data->answer || millis() > data->last_query+1) {
            if (data->stop || !data->monitorNb) {
                data->running = false;
                data->serial_device->packet.process = false;
                data->serial_device->redirect_packets = NULL;
            } else {
                data->current = (data->current+1)%data->monitorNb;
                data->queryPacket.id = data->monitorList[data->current];
                data->queryPacket.instruction = DXL_READ_DATA;
                data->queryPacket.parameter_nb = 2;
                data->queryPacket.parameters[0] = DXL_PROXY_REGISTER_START;
                data->queryPacket.parameters[1] = DXL_PROXY_LENGTH;
                data->queryPacket.process = true;
                data->last_query = millis();
                data->answer = false;
                data->serial_device->process(data->serial_device, (volatile struct dxl_packet *)&data->queryPacket);
            }
        }
    }
}

static bool dxl_proxy_check_id(volatile struct dxl_device *self, ui8 id)
{
    volatile struct dxl_proxy_private *data = (volatile dxl_proxy_private *)self->data;
    return (id == data->registers.eeprom.id);
}

static void dxl_proxy_write_data(volatile struct dxl_device *self, ui8 id, ui8 addr, ui8 *values, ui8 length)
{
    volatile struct dxl_proxy_private *data = (volatile dxl_proxy_private *)self->data;

    if (id == DXL_BROADCAST) {
        return;
    }

    if (addr == DXL_PROXY_LIST) { 
        memcpy((void *)data->monitorList, values, length);
        data->monitorNb = length;
        data->current = 0;
        data->last_query = 0;
    } else {
        memcpy(((ui8 *)(&data->registers))+addr, values, length);
    }

    if (data->registers.ram.torqueEnable) {
        data->stop = false;
        data->running = true;
        data->serial_device->redirect_packets = self;
    } else {
        data->stop = true;
    }
}

static void dxl_proxy_read_data(volatile struct dxl_device *self, ui8 id, ui8 addr, ui8 *values, ui8 length, ui8 *error)
{
    volatile struct dxl_proxy_private *data = (volatile dxl_proxy_private *)self->data;

    if (addr >= DXL_PROXY_VALUES) {
        int offset = (addr-DXL_PROXY_VALUES);
        memcpy(values, (void *)(data->values+DXL_PROXY_LENGTH*offset), length);
    } else {
        memcpy(values, ((ui8*)&data->registers)+addr, length);
    }

    *error = 0;
}

static void dxl_proxy_process_redirected(volatile struct dxl_device *self, volatile struct dxl_device *from, volatile struct dxl_packet *packet)
{
    volatile struct dxl_proxy_private *data = (volatile dxl_proxy_private *)self->data;

    if (data->running) {
        if (packet->id == data->monitorList[data->current] && packet->parameter_nb == DXL_PROXY_LENGTH) {
            data->answer = true;
            memcpy((void *)(data->values+DXL_PROXY_LENGTH*data->current), (void *)packet->parameters, DXL_PROXY_LENGTH);
        }
    }
}

static void dxl_proxy_process(volatile struct dxl_device *self, volatile struct dxl_packet *packet)
{
    dxl_process(self, packet, dxl_proxy_check_id, dxl_proxy_write_data, dxl_proxy_read_data);
}

void dxl_proxy_init(volatile struct dxl_device *device, volatile struct dxl_device *serial_device, ui8 id)
{
    int i;

    volatile struct dxl_proxy_private *data = (volatile struct dxl_proxy_private *)malloc(sizeof(struct dxl_proxy_private));
    dxl_device_init(device);
    device->tick = dxl_proxy_tick;
    device->process = dxl_proxy_process;
    device->process_redirected = dxl_proxy_process_redirected;
    device->data = (void *)data;

    data->registers.eeprom.modelNumber = DXL_PROXY_MODEL;
    data->registers.eeprom.firmwareVersion = 1;
    data->registers.eeprom.id = id;
    data->registers.ram.torqueEnable = 1;

    data->serial_device = serial_device;
    data->monitorNb = 0;
    data->current = 0;
    data->running = false;
    data->stop = false;

    dxl_packet_init((volatile struct dxl_packet *)&data->queryPacket);

    for (i=0; i<sizeof(data->values); i++) {
        data->values[i] = 0;
    }
}
