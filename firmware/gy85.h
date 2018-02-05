#ifndef _GY85_H
#define _GY85_H

#include <wirish/wirish.h>
#include <i2c.h>

// Raw values
struct __attribute__((__packed__)) gy85_value
{
    int16_t acc_x, acc_y, acc_z;
    int16_t gyro_x, gyro_y, gyro_z;
    //int16_t magn_x, magn_y, magn_z;
    uint32_t sequence;
};

// Initializing
void gy85_init(i2c_dev *dev);
void gy85_update(i2c_dev *dev, struct gy85_value *value, int sensor);

#endif
