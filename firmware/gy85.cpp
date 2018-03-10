#include <stdlib.h>
#include <wirish/wirish.h>
#include <i2c.h>
#include "gy85.h"

#define I2C_TIMEOUT 2

static bool gy85initialized = false;

extern "C" {
    void led_on() {
        digitalWrite(BOARD_LED_PIN, HIGH);
    }
    void led_off() {
        digitalWrite(BOARD_LED_PIN, LOW);
    }
}

int32 i2c_master_xfer_reinit(i2c_dev *dev,
        i2c_msg *msgs,
        uint16 num,
        uint32 timeout) 
{
    int32 r = i2c_master_xfer(dev, msgs, num, timeout);
    if (r != 0) {
        gy85initialized = false;
    }
    return r;
}

// Addresses
#define MAGN_ADDR       0x1e
#define GYRO_ADDR       0x68
#define ACC_ADDR        0x53

#define MAGN_X_CENTER   ((MAGN_X_MIN+MAGN_X_MAX)/2.0)
#define MAGN_X_AMP      (MAGN_X_MAX-MAGN_X_MIN)
#define MAGN_Y_CENTER   ((MAGN_Y_MIN+MAGN_Y_MAX)/2.0)
#define MAGN_Y_AMP      (MAGN_Y_MAX-MAGN_Y_MIN)
#define MAGN_Z_CENTER   ((MAGN_Z_MIN+MAGN_Z_MAX)/2.0)
#define MAGN_Z_AMP      (MAGN_Z_MAX-MAGN_Z_MIN)

// Signing
#define VALUE_SIGN(value, length) \
    ((value < (1<<(length-1))) ? \
     (value) \
     : (value-(1<<length)))

struct i2c_msg packet;

// Gyroscope packets
static uint8 gyro_reset[] = {0x3e, 0x80};
static uint8 gyro_scale[] = {0x16, 0b00011010};
static uint8 gyro_100hz[] = {0x15, 0x09};
static uint8 gyro_pll[] = {0x3e, 0x00};
static uint8 gyro_req[] = {0x1d};

// Accelerometer packets
static uint8 acc_measure[] = {0x2d, 0x08};
static uint8 acc_resolution[] = {0x31, 0x08};
static uint8 acc_100hz[] = {0x2c, 0x0a};
static uint8 acc_req[] = {0x32};

// Magnetometer packets
static uint8 magn_continuous[] = {0x02, 0x00};
static uint8 magn_100hz[] = {0x00, 0b00011000};
static uint8 magn_sens[] = {0x01, 0b10000000};
static uint8 magn_req[] = {0x03};

void gy85_init(i2c_dev *dev)
{
    // Initializing I2C bus
    i2c_init(dev);
    i2c_master_enable(dev, I2C_FAST_MODE);

    // Initializing magnetometer
    packet.addr = MAGN_ADDR;
    packet.flags = 0;
    packet.data = magn_continuous;
    packet.length = 2;
    if (i2c_master_xfer_reinit(dev, &packet, 1, I2C_TIMEOUT) != 0) goto init_error;

    packet.data = magn_100hz;
    if (i2c_master_xfer_reinit(dev, &packet, 1, I2C_TIMEOUT) != 0) goto init_error;

    packet.data = magn_sens;
    if (i2c_master_xfer_reinit(dev, &packet, 1, I2C_TIMEOUT) != 0) goto init_error;

    // Initializing accelerometer
    packet.addr = ACC_ADDR;
    packet.flags = 0;
    packet.data = acc_measure;
    packet.length = 2;
    if (i2c_master_xfer_reinit(dev, &packet, 1, I2C_TIMEOUT) != 0) goto init_error;

    packet.data = acc_resolution;
    if (i2c_master_xfer_reinit(dev, &packet, 1, I2C_TIMEOUT) != 0) goto init_error;

    packet.data = acc_100hz;
    if (i2c_master_xfer_reinit(dev, &packet, 1, I2C_TIMEOUT) != 0) goto init_error;

    // Initializing gyroscope
    packet.addr = GYRO_ADDR;
    packet.flags = 0;
    packet.data = gyro_reset;
    packet.length = 2;
    if (i2c_master_xfer_reinit(dev, &packet, 1, I2C_TIMEOUT) != 0) goto init_error;

    packet.data = gyro_scale;
    if (i2c_master_xfer_reinit(dev, &packet, 1, I2C_TIMEOUT) != 0) goto init_error;
    packet.data = gyro_100hz;
    if (i2c_master_xfer_reinit(dev, &packet, 1, I2C_TIMEOUT) != 0) goto init_error;
    packet.data = gyro_pll;
    if (i2c_master_xfer_reinit(dev, &packet, 1, I2C_TIMEOUT) != 0) goto init_error;

    gy85initialized = true;
    return;

init_error:
    gy85initialized = false;
}

bool magn_update(i2c_dev *dev, struct gy85_value *values)
{
    if (!gy85initialized) return false;

    packet.addr = MAGN_ADDR;
    packet.flags = 0;
    packet.data = magn_req;
    packet.length = 1;
    if (i2c_master_xfer_reinit(dev, &packet, 1, I2C_TIMEOUT) != 0) return false;

    char buffer[6];
    packet.flags = I2C_MSG_READ;
    packet.data = (uint8*)buffer;
    packet.length = 6;
    if (i2c_master_xfer_reinit(dev, &packet, 1, I2C_TIMEOUT) != 0) return false;

    values->magn_x = ((buffer[0]&0xff)<<8)|(buffer[1]&0xff);
    values->magn_x = VALUE_SIGN(values->magn_x, 16);
    values->magn_y = ((buffer[2]&0xff)<<8)|(buffer[3]&0xff);
    values->magn_y = VALUE_SIGN(values->magn_y, 16);
    values->magn_z = ((buffer[4]&0xff)<<8)|(buffer[5]&0xff);
    values->magn_z = VALUE_SIGN(values->magn_z, 16);

    return true;
}

bool gyro_update(i2c_dev *dev, struct gy85_value *values)
{
    if (!gy85initialized) return false;

    packet.addr = GYRO_ADDR;
    packet.flags = 0;
    packet.data = gyro_req;
    packet.length = 1;
    if (i2c_master_xfer_reinit(dev, &packet, 1, I2C_TIMEOUT) != 0) return false;

    char buffer[6];
    packet.flags = I2C_MSG_READ;
    packet.data = (uint8*)buffer;
    packet.length = 6;
    if (i2c_master_xfer_reinit(dev, &packet, 1, I2C_TIMEOUT) != 0) return false;

    values->gyro_x = ((buffer[0]&0xff)<<8)|(buffer[1]&0xff);
    values->gyro_x = VALUE_SIGN(values->gyro_x, 16);
    values->gyro_y = ((buffer[2]&0xff)<<8)|(buffer[3]&0xff);
    values->gyro_y = VALUE_SIGN(values->gyro_y, 16);
    values->gyro_z = ((buffer[4]&0xff)<<8)|(buffer[5]&0xff);
    values->gyro_z = VALUE_SIGN(values->gyro_z, 16);

    return true;
}

bool acc_update(i2c_dev *dev, struct gy85_value *values)
{
    if (!gy85initialized) return false;

    packet.addr = ACC_ADDR;
    packet.flags = 0;
    packet.data = acc_req;
    packet.length = 1;
    if (i2c_master_xfer_reinit(dev, &packet, 1, I2C_TIMEOUT) != 0) return false;

    char buffer[6];
    packet.flags = I2C_MSG_READ;
    packet.data = (uint8*)buffer;
    packet.length = 6;
    if (i2c_master_xfer_reinit(dev, &packet, 1, I2C_TIMEOUT) != 0) return false;

    values->acc_x = ((buffer[1]&0xff)<<8)|(buffer[0]&0xff);
    values->acc_x = VALUE_SIGN(values->acc_x, 16);
    values->acc_y = ((buffer[3]&0xff)<<8)|(buffer[2]&0xff);
    values->acc_y = VALUE_SIGN(values->acc_y, 16);
    values->acc_z = ((buffer[5]&0xff)<<8)|(buffer[4]&0xff);
    values->acc_z = VALUE_SIGN(values->acc_z, 16);

    return true;
}

bool gy85_update(i2c_dev *dev, struct gy85_value *value, int sensor)
{
    if (!gy85initialized) {
        gy85_init(dev);
    } else {
        // XXX: We should populate value with the last known value, even if
        // it fails
        bool ok = false;
        if (sensor == 0) ok = gyro_update(dev, value);
        if (sensor == 1) ok = magn_update(dev, value);
        if (sensor == 2) ok = acc_update(dev, value);
        return ok;
    }
}
