//Copyright (C) 2009-2011  Patrick J. McCarty.
//Licensed under X11 License. See LICENSE.txt for details.

#ifndef MOTORS_H
#define MOTORS_H

#include "globals.h"

//Prototypes
void motorInit();
void motor0(const u08 speedAndDirection);
void brake0(const u08 brakingPower);
u08 motor0Faulted();
void motor1(const u08 speedAndDirection);
void brake1(const u08 brakingPower);
u08 motor1Faulted();

#endif
