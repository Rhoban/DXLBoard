#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <wirish/wirish.h>
#include <libmaple/usart.h>
#include <Servo/Servo.h>
#include <gpio.h>
#include "dxl.h"
#include "dxl_usb_serial.h"
#include "dxl_servo.h"
#include "dxl_serial.h"
#include "dxl_magnetic_coder.h"
#include "dxl_gy85.h"
#include "dxl_pins.h"
    
struct dxl_bus bus;
volatile struct dxl_device dxl_usb_serial;
volatile struct dxl_device slaves[8];

void button()
{
    digitalWrite(BOARD_LED_PIN, HIGH);
}

void setup()
{ 
    int k = 0;

    // Enabling custom USB port
    pinMode(28, OUTPUT);
    digitalWrite(28, HIGH);

    //test();
    pinMode(BOARD_BUTTON_PIN, INPUT_PULLDOWN);
    pinMode(BOARD_LED_PIN, OUTPUT);
    digitalWrite(BOARD_LED_PIN, LOW);

    nvic_irq_set_priority(NVIC_USART1, 0x0);
    nvic_irq_set_priority(NVIC_USART2, 0x0);
    nvic_irq_set_priority(NVIC_USART3, 0x0);
    nvic_irq_set_priority(NVIC_DMA_CH2, 0x0);
    nvic_irq_set_priority(NVIC_DMA_CH4, 0x0);
    nvic_irq_set_priority(NVIC_DMA_CH7, 0x0);

    attachInterrupt(BOARD_BUTTON_PIN, button, RISING);

    dxl_bus_init(&bus);

    // The master of the bs is the usb serial emulator
    dxl_usb_serial_init(&dxl_usb_serial);
    dxl_set_master(&bus, &dxl_usb_serial);

    // Adding servo pin 15 to dynamixel id 220, and pin 16 to id 221
    // dxl_servo_init(&slaves[k++], 15, 220);
    // dxl_servo_init(&slaves[k++], 16, 221);
    
    // Add the Serial-forward dynamixel device as a slave
    dxl_serial_init(&slaves[k++], 1);
    // We can only use one bus with this version (see other git branch for multi bus)
    //dxl_serial_init(&slaves[k++], 2);
    //dxl_serial_init(&slaves[k++], 3);

    // Add the ADC dynamixel on the bus, id 240
    // dxl_adc_init(&slaves[k++], 240);

    // Add the IMU dynamixel, id 241, port Serial2
    dxl_gy85_init(&slaves[k++], 241, I2C1);

    // Adding pins
    dxl_pins_init(&slaves[k++], 242);

    // Add a magnetic coder on the bus, ID 235
    // dxl_magnetic_coder_init(&slaves[k++], 3, 235);

    // Add an ADC coder on the bus, ID 235
    // dxl_coder_init(&slaves[k++], 3, 4, 5, 235);
   
    // Adding all slaves on the b us
    for (k--; k>=0; k--) {
        dxl_add_slave(&bus, &slaves[k]);
    }
}

void loop()
{
    dxl_bus_tick(&bus);
}

// Force init to be called *first*, i.e. before static object allocation.
// Otherwise, statically allocated objects that need libmaple may fail.
__attribute__((constructor)) void premain()
{
    init();
}

int main(void)
{
    setup();

    while (true) {
        loop();
    }
    return 0;
}
