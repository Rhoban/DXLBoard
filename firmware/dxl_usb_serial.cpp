#include <wirish/wirish.h>
#include "dxl_usb_serial.h"
#include <stdio.h>
char sprintfBufferusb[120];

static void dxl_usb_serial_tick(volatile struct dxl_device *self)
{
    //int micro = micros();
    bool print = false;
    // Reads data from SerialUSB to the dynamixel packet
    while (SerialUSB.available() && !self->packet.process) {
        dxl_packet_push_byte(&self->packet, SerialUSB.read());
        print=true;
    }

    if(print) {
        //sprintf(sprintfBufferusb, "tickUSB: %d", micros() - micro);
        //SerialUSB.println(sprintfBufferusb);
    }
}

static void dxl_usb_serial_process(volatile struct dxl_device *self, volatile struct dxl_packet *packet)
{
    //int micro = micros();
    // If a packet is available for process, sends it through the USB adapter
    ui8 buffer[DXL_BUFFER_SIZE];
    // we always send status packages to the master
    packet->instruction = 0x55;
    int n = dxl_write_packet(packet, buffer);
    SerialUSB.write(buffer, n);

    //sprintf(sprintfBufferusb, "SendUSB: %d", micros() - micro);
    //SerialUSB.println(sprintfBufferusb);
}

void dxl_usb_serial_init(volatile struct dxl_device *device)
{
    dxl_device_init(device);
    device->tick = dxl_usb_serial_tick;
    device->process = dxl_usb_serial_process;
}
