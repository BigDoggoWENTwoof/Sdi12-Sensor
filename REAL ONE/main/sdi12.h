#ifndef SDI12_HANDLER_H
#define SDI12_HANDLER_H

#include <Arduino.h>

void sdi12Init(char address, int dirPin);
void sdi12Handle();

#endif