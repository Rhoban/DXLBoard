#include <stdlib.h>
#include <wirish/wirish.h>
#include <string.h>
#include "dxl_imu.h"
#include "dxl_protocol.h"
#include "coder.h"

#define IMU_BAUDRATE 1000000
#define IMU_DATASIZE 26
#define IMU_HEADER1 0xff
#define IMU_HEADER2 0xaa
#define IMU_ADDR 0x24

HardwareTimer imuTimer(2);

struct imu
{
    HardwareSerial *port;
    int state;
    int pos;
    unsigned char checksum;
    unsigned char buffer[IMU_DATASIZE];
    unsigned char data[IMU_DATASIZE];
    struct dxl_registers registers;
};

void imu_begin(struct imu *imu, HardwareSerial *port)
{
    int i;
    for (i=0; i<IMU_DATASIZE; i++) {
        imu->data[i] = 0;
    }
    imu->state = 0;
    imu->port = port;
    imu->port->begin(IMU_BAUDRATE);
    
    imuTimer.pause();
    imuTimer.setPrescaleFactor(CYCLES_PER_MICROSECOND);
    imuTimer.setOverflow(0xffff);
    imuTimer.refresh();
    imuTimer.resume();
}

void imu_tick(struct imu *imu)
{
    while (imu->port->available()) {
        unsigned char c = imu->port->read();

        switch (imu->state) {
            case 0:
                if (c == IMU_HEADER1) {
                    imu->state++;
                }
                break;
            case 1:
                if (c == IMU_HEADER2) {
                    imu->state++;
                    imu->pos = 0;
                    imu->checksum = 0;
                } else {
                    imu->state = 0;
                }
                break;
            case 2:
                imu->buffer[imu->pos++] = c;
                imu->checksum += c;

                if (imu->pos >= IMU_DATASIZE) {
                    imu->state++;
                }
                break;
            case 3:
                if (c == imu->checksum) {
                    int i;
                    for (i=0; i<IMU_DATASIZE; i++) {
                        imu->data[i] = imu->buffer[i];
                    }
                }
                imu->state = 0;
                break;
        }
    }
};


static bool dxl_imu_check_id(volatile struct dxl_device *self, ui8 id)
{
    struct imu *imu = (struct imu*)self->data;
    return (id == imu->registers.eeprom.id);
}

static void dxl_imu_write_data(volatile struct dxl_device *self, ui8 id, ui8 addr, ui8 *values, ui8 length)
{
    struct imu *imu = (struct imu*)self->data;
    memcpy(((ui8 *)(&imu->registers))+addr, values, length);
}

static void dxl_imu_read_data(volatile struct dxl_device *self, ui8 id, ui8 addr, ui8 *values, ui8 length, ui8 *error)
{
    struct imu *imu = (struct imu*)self->data;

    if (addr == IMU_ADDR) {
        memcpy(values, ((ui8*)&imu->data), length);
    } else {
        memcpy(values, ((ui8*)&imu->registers)+addr, length);
    }
    *error = 0;
}

static void dxl_imu_process(volatile struct dxl_device *self, volatile struct dxl_packet *packet)
{
    dxl_process(self, packet, dxl_imu_check_id, dxl_imu_write_data, dxl_imu_read_data);
}

static void imu_tick(volatile struct dxl_device *self)
{
    struct imu *imu = (struct imu*)self->data;
    imu_tick(imu);
}

void dxl_imu_init(volatile struct dxl_device *device, ui8 id, HardwareSerial *port)
{
    struct imu *imu = (struct imu *)malloc(sizeof(struct imu));
    dxl_device_init(device);
    device->process = dxl_imu_process;
    device->data = (void *)imu;
    device->tick = imu_tick;

    imu_begin(imu, port);

    imu->registers.eeprom.modelNumber = DXL_IMU_MODEL;
    imu->registers.eeprom.firmwareVersion = 1;
    imu->registers.eeprom.id = id;
    imu->registers.ram.presentPosition = 0;
    imu->registers.ram.goalPosition = 0;
    imu->registers.ram.torqueEnable = 0;
    imu->registers.ram.torqueLimit = 0;

}
