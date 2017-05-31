#include <stdlib.h>
#include <stdint.h>
#include <wirish/wirish.h>
#include <dma.h>
#include <usart.h>
#include <usb_cdcacm.h>
#include "dxl_protocol.h"
#include "dxl_serial.h"

// Serial com
#define DIRECTION1      29
#define DIRECTION2      30
#define DIRECTION3      31

// Devices allocation
static bool serialInitialized = false;
static uint8_t devicePorts[254];

// Device data
struct serial
{
    // Index
    int index;
    // Serial port
    HardwareSerial *port;
    // Direction pin
    int direction;
    // Transmission buffer
    char outputBuffer[DXL_BUFFER_SIZE];

    // Number of bytes to send
    ui8 toSend;
    // IDs of devices that we should sync read
    ui8 syncReadIds[64];
    // Offset of the data in the packet
    ui8 syncReadOffsets[64];
    // The count of devices that we should sync read
    ui8 syncReadCount;
    // Current device that is read
    ui8 syncReadCurrent;
    // Packet use for sync read
    struct dxl_packet syncReadPacket;
    // Timestamp of the starting
    uint32_t syncReadStart;
    // Timestamp for sending data
    uint32_t packetSent;

    // DMA
    dma_tube_config tube_config;
    dma_channel channel;
    bool txComplete;
    bool dmaEvent;

    // Registers
    struct dxl_registers registers;
};

// Ports that are online
struct serial *serials[8] = {0};

static void receiveMode(struct serial *serial);

// Sync read
// Are we in sync read mode?
bool syncReadMode = false;
// How many devices are still waiting for packets?
int syncReadDevices = 0;
// The sync read timer (incremented by 10µs step) that can be used
// for timeout
HardwareTimer syncReadTimer(2);
// Address to sync read
ui8 syncReadAddr = 0;
// Length to sync read
ui8 syncReadLength = 0;
// Packet for sync read
struct dxl_packet *syncReadResponse;

static void serial_received(struct serial *serial)
{
    dma_disable(DMA1, serial->channel);
    usart_tcie(serial->port->c_dev()->regs, 0);
    serial->dmaEvent = false;
    serial->txComplete = true;
    receiveMode(serial);
    serial->syncReadStart = syncReadTimer.getCount();
}

static void tc_event(struct serial *serial)
{
    if (serial->dmaEvent) {
        serial_received(serial);
    }
}

static void dma_event(struct serial *serial)
{
    // DMA completed
    serial->dmaEvent = true;
}
static void DMAEvent1()
{
    dma_event(serials[1]);
}
static void usart1_tc()
{
    tc_event(serials[1]);
}
static void DMAEvent2()
{
    dma_event(serials[2]);
}
static void usart2_tc()
{
    tc_event(serials[2]);
}
static void DMAEvent3()
{
    dma_event(serials[3]);
}
static void usart3_tc()
{
    tc_event(serials[3]);
}

static void setupSerialDMA(struct serial *serial, int n)
{
    // We're receiving from the USART data register. serial->c_dev()
    // returns a pointer to the libmaple usart_dev for that serial
    // port, so this is a pointer to its data register.
    serial->tube_config.tube_dst = &serial->port->c_dev()->regs->DR;
    // We're only interested in the bottom 8 bits of that data register.
    serial->tube_config.tube_dst_size = DMA_SIZE_8BITS;
    // We're storing to rx_buf.
    serial->tube_config.tube_src = serial->outputBuffer;
    // rx_buf is a char array, and a "char" takes up 8 bits on STM32.
    serial->tube_config.tube_src_size = DMA_SIZE_8BITS;
    // Only fill BUF_SIZE - 1 characters, to leave a null byte at the end.
    serial->tube_config.tube_nr_xfers = n;
    // Flags:
    // - DMA_CFG_DST_INC so we start at the beginning of rx_buf and
    // fill towards the end.
    // - DMA_CFG_CIRC so we go back to the beginning and start over when
    // rx_buf fills up.
    // - DMA_CFG_CMPLT_IE to turn on interrupts on transfer completion.
    serial->tube_config.tube_flags = DMA_CFG_SRC_INC | DMA_CFG_CMPLT_IE | DMA_CFG_ERR_IE;
    // Target data: none. It's important to set this to NULL if you
    // don't have any special (microcontroller-specific) configuration
    // in mind, which we don't.
    serial->tube_config.target_data = NULL;
    // DMA request source.
    if (serial->index == 1) serial->tube_config.tube_req_src = DMA_REQ_SRC_USART1_TX;
    if (serial->index == 2) serial->tube_config.tube_req_src = DMA_REQ_SRC_USART2_TX;
    if (serial->index == 3) serial->tube_config.tube_req_src = DMA_REQ_SRC_USART3_TX;
}

static void receiveMode(struct serial *serial)
{
    asm volatile("nop");
    // Disabling transmitter
    serial->port->enableTransmitter(false);

    // Sets the direction to receiving
    digitalWrite(serial->direction, LOW);

    // Enabling the receiver
    serial->port->enableReceiver(true);
    asm volatile("nop");
}

void transmitMode(struct serial *serial)
{
    asm volatile("nop");
    // Disabling transmitter
    serial->port->enableReceiver(false);

    // Sets the direction to receiving
    digitalWrite(serial->direction, HIGH);

    // Enabling the receiver
    serial->port->enableTransmitter(true);
    asm volatile("nop");
}

void initSerial(struct serial *serial, int baudrate = DXL_DEFAULT_BAUDRATE)
{
    pinMode(serial->direction, OUTPUT);

    // Enabling serial port in receive mode
    serial->port->begin(baudrate);
    serial->port->c_dev()->regs->CR3 = USART_CR3_DMAT;
    
    // Entering receive mode
    receiveMode(serial);

    // Registering the serial port
    serials[serial->index] = serial;
}

void sendSerialPacket(struct serial *serial, volatile struct dxl_packet *packet)
{
    // We have a packet for the serial bus
    // First, clear the serial input buffers
    serial->port->flush();

    // Writing the packet in the buffer
    int n = dxl_write_packet(packet, (ui8 *)serial->outputBuffer);

    // Go in transmit mode
    transmitMode(serial);

#if 1
    // Then runs the DMA transfer
    serial->txComplete = false;
    serial->dmaEvent = false;
    setupSerialDMA(serial, n);
    dma_tube_cfg(DMA1, serial->channel, &serial->tube_config);
    dma_set_priority(DMA1, serial->channel, DMA_PRIORITY_VERY_HIGH);
    if (serial->index == 1) dma_attach_interrupt(DMA1, serial->channel, DMAEvent1);
    if (serial->index == 2) dma_attach_interrupt(DMA1, serial->channel, DMAEvent2);
    if (serial->index == 3) dma_attach_interrupt(DMA1, serial->channel, DMAEvent3);
    usart_tcie(serial->port->c_dev()->regs, 1);
    dma_enable(DMA1, serial->channel);
    serial->packetSent = millis();
#else
    // Directly send the packet
    char buffer[1024];
    n = dxl_write_packet(packet, (ui8 *)buffer);
    for (int i=0; i<n; i++) {
        serial->port->write(buffer[i]);
    }
    serial->port->waitDataToBeSent();
    receiveMode(serial);
#endif
}
          
/**
 * Sends the next sync read packet
 */
void syncReadSendPacket(struct serial *serial)
{
    // Sending the read packet on the bus
    struct dxl_packet readPacket;
    readPacket.instruction = DXL_READ_DATA;
    readPacket.parameter_nb = 2;
    readPacket.id = serial->syncReadIds[serial->syncReadCurrent];
    readPacket.parameters[0] = syncReadAddr;
    readPacket.parameters[1] = syncReadLength;
    sendSerialPacket(serial, &readPacket);

    // Resetting the receive packet
    serial->syncReadPacket.dxl_state = 0;
    serial->syncReadPacket.process = false;
}

/**
 * Ticking
 */
static void dxl_serial_tick(volatile struct dxl_device *self) 
{
    struct serial *serial = (struct serial*)self->data;
    static int baudrate = DXL_DEFAULT_BAUDRATE;

    // Timeout on sending packet, this should never happen
    if (!serial->txComplete && ((millis() - serial->packetSent) > 3)) {
        serial_received(serial);
    }

    /*
    if (!serial->txComplete) {
        if (serial->dmaEvent) {
            // DMA completed
            serial->dmaEvent = false;
            serial->txComplete = true;
            //serial->port->waitDataToBeSent();
            receiveMode(serial);
            serial->syncReadStart = syncReadTimer.getCount();
        }
    } 
    */
 
    if (serial->txComplete) {
        if (syncReadMode) {
            bool processed = false;

            if (syncReadDevices <= 0 && syncReadResponse == &self->packet) {
                // The process is over, no devices are currently trying to get packet and we
                // are the device that will respond
                syncReadResponse->process = true;
                syncReadMode = false;
            } else if (serial->syncReadCount) {
                if (serial->syncReadCurrent == 0xff) {
                    // Sending the first packet
                    serial->syncReadCurrent = 0;
                    syncReadSendPacket(serial);
                } else {
                    // Reading available data from the port
                    while (serial->port->available() && !serial->syncReadPacket.process) {
                        dxl_packet_push_byte(&serial->syncReadPacket, serial->port->read());
                    }
                    if (serial->syncReadPacket.process && serial->syncReadPacket.parameter_nb == syncReadLength) {
                        // The packet is OK, copying data to the response at correct offset
                        ui8 i;
                        ui8 off = serial->syncReadOffsets[serial->syncReadCurrent]*(syncReadLength+1);
                        syncReadResponse->parameters[off] = serial->syncReadPacket.error;
                        for (i=0; i<syncReadLength; i++) {
                            syncReadResponse->parameters[off+1+i] = serial->syncReadPacket.parameters[i];
                        }
                        processed = true;
                    } else if (syncReadTimer.getCount()-serial->syncReadStart > 65) {
                        // The timeout is reached, answer with code 0xff
                        ui8 off = serial->syncReadOffsets[serial->syncReadCurrent]*(syncReadLength+1);
                        syncReadResponse->parameters[off] = 0xff;
                        processed = true;
                    }
                    if (processed) {
                        serial->syncReadCurrent++;
                        if (serial->syncReadCurrent >= serial->syncReadCount) {
                            // The process is over for this bus
                            syncReadDevices--;
                            serial->syncReadCount = 0;
                        } else {
                            // Sending the next packet
                            syncReadSendPacket(serial);
                        }
                    }
                }
            }
        } else {
            if (baudrate != usb_cdcacm_get_baud()) {
                // baudrate = usb_cdcacm_get_baud();
                // initSerial(serial, baudrate);
            }

            // Reading data that come from the serial bus
            while (serial->port->available() && !self->packet.process) {
                dxl_packet_push_byte(&self->packet, serial->port->read());
                if (self->packet.process) {
                    // A packet is coming from our bus, noting it in the devices allocation
                    // table
                    devicePorts[self->packet.id] = serial->index;
                }
            }
        }
    }
}

/**
 * Processing master packets (sending it to the local bus)
 */
static void process(volatile struct dxl_device *self, volatile struct dxl_packet *packet)
{
    struct serial *serial = (struct serial*)self->data;
    dxl_serial_tick(self);

    if (serial->txComplete && !syncReadMode) {
        if (packet->instruction == DXL_SYNC_READ && packet->parameter_nb > 2) {
            ui8 i;
            syncReadMode = true;
            syncReadAddr = packet->parameters[0];
            syncReadLength = packet->parameters[1];
            syncReadDevices = 0;
            ui8 total = 0;

            for (i=0; (i+2)<packet->parameter_nb; i++) {
                ui8 id = packet->parameters[i+2];

                if (devicePorts[id]) {
                    struct serial *port = serials[devicePorts[id]];
                    if (port->syncReadCount == 0) {
                        port->syncReadCurrent = 0xff;
                        syncReadDevices++;
                    }
                    port->syncReadIds[port->syncReadCount] = packet->parameters[i+2];
                    port->syncReadOffsets[port->syncReadCount] = i;
                    port->syncReadCount++;
                    total++;
                }
            }

            syncReadTimer.refresh();
            syncReadResponse = (struct dxl_packet*)&self->packet;
            syncReadResponse->error = 0;
            syncReadResponse->parameter_nb = (syncReadLength+1)*total;
            syncReadResponse->process = false;
            syncReadResponse->id = packet->id;
        } else {
            // Forwarding the packet to the serial bus
            if (packet->id == DXL_BROADCAST || packet->id < 200) {
                self->packet.dxl_state = 0;
                self->packet.process = false;
                sendSerialPacket(serial, packet);
            }
        }
    }
}

void dxl_serial_init(volatile struct dxl_device *device, int index)
{
    ASSERT(index >= 1 && index <= 3);

    // Initializing device
    struct serial *serial = (struct serial*)malloc(sizeof(struct serial));
    dxl_device_init(device);
    device->data = (void *)serial;
    device->tick = dxl_serial_tick;
    device->process = process;

    serial->index = index;
    serial->txComplete = true;
    serial->dmaEvent = false;

    if (index == 1) {
        serial->port = &Serial1;
        serial->direction = DIRECTION1;
        serial->channel = DMA_CH4;
    } else if (index == 2) {
        serial->port = &Serial2;
        serial->direction = DIRECTION2;
        serial->channel = DMA_CH7;
    } else {
        serial->port = &Serial3;
        serial->direction = DIRECTION3;
        serial->channel = DMA_CH2;
    }
            
    serial->syncReadCurrent = 0xff;
    serial->syncReadCount = 0;

    initSerial(serial);

    if (!serialInitialized) {
        serialInitialized = true;
    
        // Initializing DMA
        dma_init(DMA1);

        usart1_tc_handler = usart1_tc;
        usart2_tc_handler = usart2_tc;
        usart3_tc_handler = usart3_tc;

        // Reset allocation
        for (unsigned int k=0; k<sizeof(devicePorts); k++) {
            devicePorts[k] = 0;
        }

        // Initialize sync read timer (1 step = 10uS)
        syncReadTimer.pause();
        syncReadTimer.setPrescaleFactor(CYCLES_PER_MICROSECOND*10);
        syncReadTimer.setOverflow(0xffff);
        syncReadTimer.refresh();
        syncReadTimer.resume();
    }
}
