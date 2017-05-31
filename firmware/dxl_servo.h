#ifndef DXL_SERVO_H
#define DXL_SERVO_H

#include "dxl.h"

/**
 * Timers parameters for output
 * PWM generation
 */
#define SERVOS_TIMERS_PRESCALE 24
#define SERVOS_TIMERS_OVERFLOW 60000
#define DXL_SERVO_TYPE 200

void dxl_servo_init(volatile struct dxl_device *device, ui8 pin, ui8 id);

#endif // DXL_SERVO_H
