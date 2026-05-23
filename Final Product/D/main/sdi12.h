#ifndef SDI12_HANDLER_H
#define SDI12_HANDLER_H

#include <Arduino.h>

// Core SDI-12 interface
void sdi12Init(char address, int dirPin);
void sdi12Handle();

// Optional (only if you want external access)
void sdiSend(String response);
void parseCommand(String cmd);

#endif