//Copyright (C) 2009-2011  Patrick J. McCarty.
//Licensed under X11 License. See LICENSE.txt for details.

#ifndef SERVOS_H
#define SERVOS_H

#include "globals.h"

/*! Specifies the recommended values that can be passed to the servoRange() function.
 *  Except for SERVO_RANGE_STANDARD, the values were selected to provide ranges that
 *  match very closely between 16 MHz and 20 MHz microcontrollers.
 */
typedef enum
{
#if (F_CPU == 16000000UL)
	//16 MHz boards
	SERVO_RANGE_STANDARD = 62,   //!< 1.0040 ms - 1.9920 ms - best value within 1ms-2ms servo spec
	SERVO_RANGE_DEFAULT = 64,    //!< 0.9880 ms - 2.0080 ms
	SERVO_RANGE_EXTENDED1 = 72,  //!< 0.9240 ms - 2.0715 ms
	SERVO_RANGE_EXTENDED2 = 80,  //!< 0.8600 ms - 2.1350 ms
	SERVO_RANGE_EXTENDED3 = 88,  //!< 0.7960 ms - 2.1985 ms
	SERVO_RANGE_EXTENDED4 = 96   //!< 0.7320 ms - 2.2620 ms
#elif (F_CPU == 20000000UL)
	//20 MHz boards
	SERVO_RANGE_STANDARD = 78,   //!< 1.0008 ms - 1.9952 ms - best value within 1ms-2ms servo spec
	SERVO_RANGE_DEFAULT = 80,    //!< 0.9880 ms - 2.0080 ms
	SERVO_RANGE_EXTENDED1 = 90,  //!< 0.9240 ms - 2.0712 ms
	SERVO_RANGE_EXTENDED2 = 100, //!< 0.8600 ms - 2.1348 ms
	SERVO_RANGE_EXTENDED3 = 110, //!< 0.7960 ms - 2.1984 ms
	SERVO_RANGE_EXTENDED4 = 120  //!< 0.7320 ms - 2.2620 ms
#else
	#error ServoRange enum values are not defined for your F_CPU speed.
#endif

} ServoRange;

//Prototypes
bool setServoRange(const u08 servoNum, const ServoRange range);
u08 getServoRange(const u08 servoNum);
void servo(const u08 servoNum, const u08 position);
void servo2(const u08 servoNum, const s08 position);
void servoOff(const u08 servoNum);
void servoInit();

#endif
