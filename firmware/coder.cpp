#include <wirish/wirish.h>
#include "coder.h"

#define CODER_STATE_NONE 0      // 00
#define CODER_STATE_A    1      // 01
#define CODER_STATE_B    2      // 10
#define CODER_STATE_AB   3      // 11

#define CODER_STATES    4
#define CODER_STEPS     4096

static int coder_state;
static int coder_direction;
static int coder_state_order;
static int coder_value;
static int pinA, pinB, pinZ;

static int coder_state_orders[4] = {
    0, // CODER_STATE_NONE
    1, // CODER_STATE_A
    3, // CODER_STATE_B
    2 // CODER_STATE_AB
};

static void coder_compute_state()
{
    coder_state = digitalRead(pinA) | (digitalRead(pinB)<<1);
    coder_state_order = coder_state_orders[coder_state];
}

static void coder_interrupt()
{
    int old_order = coder_state_order;
    coder_compute_state();
    int diff = (CODER_STATES+old_order-coder_state_order)%CODER_STATES;

    switch (diff) {
        case 0:
            break;
        case 1:
            coder_value++;
            coder_direction = 1;
            break;
        case 2:
            coder_value += 2*coder_direction;
            break;
        case 3:
            coder_value--;
            coder_direction = -1;
            break;
    }
}

static void coder_interrupt_zero()
{
    coder_value = 0;
}

double coder_get_value()
{
    return 360.0*(coder_value/(float)CODER_STEPS);
}

int coder_get_raw_value()
{
    return coder_value;
}

void coder_init(int pinA_, int pinB_, int pinZ_)
{
    coder_value = 0;
    coder_direction = 0;

    pinA = pinA_;
    pinB = pinB_;
    pinZ = pinZ_;

    pinMode(pinA, INPUT);
    pinMode(pinB, INPUT);
    pinMode(pinZ, INPUT);

    coder_compute_state();

    attachInterrupt(pinA, coder_interrupt, CHANGE);
    attachInterrupt(pinB, coder_interrupt, CHANGE);
    attachInterrupt(pinZ, coder_interrupt_zero, RISING);
}
