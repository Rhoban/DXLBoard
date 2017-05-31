#ifndef _CODER_H
#define _CODER_H

void coder_init(int pinA, int pinB, int pinZ);
double coder_get_value();
int coder_get_raw_value();

#endif
