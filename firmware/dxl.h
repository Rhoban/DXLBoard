//#include <algorithm>
#include <cstring>

#ifndef DXL_H
#define DXL_H

// Protocol definition
#define DXL_BROADCAST   0xFE

// Size limit for a buffer containing a dynamixel packet
#define DXL_BUFFER_SIZE 330

// Maximum parameters in a packet
#define DXL_MAX_PARAMS  240

//max number of packages in sync read
#define SYNC_READ_MAX_PACKAGES 30

typedef unsigned char ui8;

/**
 * A dynamixel packet
 */
struct dxl_packet {
    ui8 id;
    ui8 instruction;
    ui8 error;
    ui8 parameter_nb;
    ui8 parameters[DXL_MAX_PARAMS];
    bool process;
    int dxl_state;
    int crc16;
    struct dxl_packet& operator=(const volatile dxl_packet &rhs){
        id = rhs.id;
        instruction = rhs.instruction;
        error = rhs.error;
        parameter_nb = rhs.parameter_nb;
        //std::copy(std::begin(rhs.parameters), std::end(rhs.parameters), std::begin(parameters));
        //std::memcpy(parameters, rhs.parameters, sizeof(parameters));
        for(int i = 0; i < parameter_nb; i++){
            parameters[i] = rhs.parameters[i];
        }
        process = rhs.process;
        dxl_state = rhs.dxl_state;
        crc16 = rhs.crc16;
        return *this;
    }
};



void dxl_packet_init(volatile struct dxl_packet *packet);
void dxl_packet_push_byte(volatile struct dxl_packet *packet, ui8 b);
int dxl_write_packet(volatile struct dxl_packet *packet, ui8 *buffer);
void dxl_copy_packet(volatile struct dxl_packet *from, volatile struct dxl_packet *to);
unsigned short update_crc(unsigned short crc_accum, unsigned char *data_blk_ptr, unsigned short data_blk_size);
void split_sync_package(struct dxl_bus *bus);
struct dxl_packet copy_non_volatile(volatile struct dxl_packet);


/**
 * A Dynamixel Device which is on the bus
 */
struct dxl_device
{
    void (*tick)(volatile struct dxl_device *self);
    void (*process)(volatile struct dxl_device *self, volatile struct dxl_packet *packet);
    volatile struct dxl_packet packet;
    volatile struct dxl_device *next;
    volatile void *data;
    int bus_index;
};

void dxl_device_init(volatile struct dxl_device *device);

/**
 * A bus is composed of one master and some slaves
 */
struct dxl_bus
{
    volatile struct dxl_device *master;
    volatile struct dxl_device *slaves;
    uint8_t devicePorts[254];
    volatile struct dxl_device *bus1;
    volatile struct dxl_device *bus2;
    volatile struct dxl_device *bus3;
    volatile struct dxl_packet bus1_package;
    volatile struct dxl_packet bus2_package;
    volatile struct dxl_packet bus3_package;
    bool syn_read_mode;
    bool super_sync_read_mode;
    volatile struct dxl_packet *sync_read_master_package;
    volatile struct dxl_packet syn_read_recieved_packages[SYNC_READ_MAX_PACKAGES];
    bool sync_read_is_packages_returned [SYNC_READ_MAX_PACKAGES];
    uint8_t sync_read_packages_returned;
    uint8_t sync_read_packages_recieved;
    uint8_t super_sync_packages_recieved;
    volatile struct dxl_packet super_sync_return_packet;

};

/**
 * Initialize the bus and run its main loop
 */
void dxl_bus_init(struct dxl_bus *bus);
void dxl_bus_tick(struct dxl_bus *bus);

/**
 * Sets the master of add a slave on the bus
 */
void dxl_set_master(struct dxl_bus *bus, volatile struct dxl_device *master);
void dxl_add_slave(struct dxl_bus *bus, volatile struct dxl_device *slave);

#endif // DXL_H
