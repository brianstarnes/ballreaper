#ifndef MAIN_H
#define MAIN_H

#include "globals.h"

/*! Version of the Launcher firmware, part of the response to a ::GET_VERSIONS command.
    Should be incremented when new features or breaking changes are added.
 */
#define LAUNCHER_FIRMWARE_VERSION "0.1"

//! The ADC reference voltage supplied to the microcontroller.
#define AREF_VOLTAGE 5
//! The number of possible 10-bit ADC output values. 10-bit ADC resolution gives 2^10=1024 values.
#define NUM_ADC10_VALUES 1024
//! The maximum ADC output value. ADC has 1024 output values so it ranges from 0 to 1023.
#define ADC_MAX 1023

//! The number of seconds per competition round.
#define COMPETITION_DURATION_SECS (3 * 60)

//! When to fire the bonus ball
#define BONUS_TIME_SECS (COMPETITION_DURATION_SECS - 10)

// Bonus Bot switches
#define BB_LEFT_HIT   !digitalInput(SWITCH_LEFT)
#define BB_RIGHT_HIT  !digitalInput(SWITCH_RIGHT)
#define BB_BACK_LEFT_HIT   !digitalInput(SWITCH_LEFT)
#define BB_BACK_RIGHT_HIT  !digitalInput(SWITCH_RIGHT)

enum servos
{
	SERVO_BACK_LEFT,
	SERVO_FRONT_LEFT,
	SERVO_BACK_RIGHT,
	SERVO_FRONT_RIGHT,
	SERVO_RELAY,
};

enum motors
{
	LAUNCHER_RIGHT,
	LAUNCHER_LEFT
};

enum servoPositions
{
	LAUNCHER_GRAB_SPEED   = 160,   //!< The speed to reverse the launcher wheels at to grab the bonus ball.
	LAUNCHER_LAUNCH_SPEED = 25, //!< The speed to spin the launcher wheels at, when farthest away from the goal.
	LAUNCHER_STOP         = 127
};

typedef enum
{
	SWITCH_LEFT   = 2,
	SWITCH_RIGHT  = 3,
	SWITCH_SCROLL = 8
} switch_t;

// QDB-1114 reflective sensor analog inputs.
typedef enum
{
	ANALOG_FRONT_LEFT  = 0,
	ANALOG_FRONT_RIGHT = 1,
	ANALOG_BACK_RIGHT  = 2,
	ANALOG_BACK_LEFT   = 3
} analog_t;

enum motorSpeeds
{
	DRIVE_STOP       = 0,
	DRIVE_SLOW_SPEED = 25,
	DRIVE_FAST_SPEED = 75
};

extern volatile u08 encoderUpdated;
extern volatile u16 qrdFrontLeftReading;
extern volatile u16 qrdFrontRightReading;
extern volatile u16 qrdBackRightReading;
extern volatile u16 qrdBackLeftReading;
extern volatile s16 error;
extern volatile s16 totalError;

#endif
