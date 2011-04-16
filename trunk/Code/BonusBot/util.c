#include "ADC.h"

#include "bonusbot.h"
#include "LCD.h"
#include "motors.h"
#include "rtc.h"
#include "serial.h"
#include "servos.h"
#include "util.h"
#include "utility.h"

#include <util/atomic.h>

static u08 curLauncherSpeed = 127;
static u08 requestedLauncherSpeed = 127;

#define LAUNCHER_SPEED_STEP 12

#define LIMIT(v, min, max) (((v) < (min)) ? (min) : (((v) > (max)) ? (max) : (v)))
#define ABS(v) (((v) < 0) ? -(v) : (v))


void compDone()
{
    haltRobot();

	// Wait for end of competition
	clearScreen();
	printString_P(PSTR("Remaining"));
	u08 priorSeconds = 255;
	while (secCount < COMPETITION_DURATION_SECS)
	{
		// only print when the time has changed
		if (secCount != priorSeconds)
		{
			priorSeconds = secCount;
			lcdCursor(0, 11);
			u08 secsRemaining = COMPETITION_DURATION_SECS - secCount;
			// print minutes
			printChar((secsRemaining / 60) + '0');
			printChar(':');
			// print seconds (tens digit)
			printChar(((secsRemaining % 60) / 10) + '0');
			// print seconds (ones digit)
			printChar(((secsRemaining % 60) % 10) + '0');
			printChar('s');
		}
	}

	victoryDance();
}

//! Do da Dance!
void victoryDance()
{
	// Done, declare domination!
	clearScreen();
	printString_P(PSTR("    PWNED!!!"));
	lowerLine();
	printString_P(PSTR("ReapedYourBalls"));

	haltRobot();
}

void frontLeft(u08 speed, u08 direction)
{
	if (direction)
		servo(SERVO_FRONT_LEFT, 128 - speed);
	else
		servo(SERVO_FRONT_LEFT, 128 + speed);
}

void frontRight(u08 speed, u08 direction)
{
	if (direction)
		servo(SERVO_FRONT_RIGHT, 128 - speed);
	else
		servo(SERVO_FRONT_RIGHT, 128 + speed);
}

void backRight(u08 speed, u08 direction)
{
	if (direction)
		servo(SERVO_BACK_RIGHT, 128 - speed);
	else
		servo(SERVO_BACK_RIGHT, 128 + speed);
}

void backLeft(u08 speed, u08 direction)
{
	if (direction)
		servo(SERVO_BACK_LEFT, 128 - speed);
	else
		servo(SERVO_BACK_LEFT, 128 + speed);
}

void driveForward(void)
{
	frontLeft(DRIVE_SLOW_SPEED, FORWARD);
	backLeft(DRIVE_SLOW_SPEED, FORWARD);
	backRight(DRIVE_SLOW_SPEED, FORWARD);
	frontRight(DRIVE_SLOW_SPEED, FORWARD);
}

void driveBackwards(void)
{
	frontLeft(DRIVE_SLOW_SPEED, REVERSE);
	backLeft(DRIVE_SLOW_SPEED, REVERSE);
	backRight(DRIVE_SLOW_SPEED, REVERSE);
	frontRight(DRIVE_SLOW_SPEED, REVERSE);
}

void strafeRight(u08 speed)
{
	frontLeft(speed, FORWARD);
	backLeft(speed, REVERSE);
	backRight(speed, REVERSE);
	frontRight(speed, FORWARD);
}

void strafeLeft(u08 speed)
{
	frontLeft(speed, REVERSE);
	backLeft(speed, FORWARD);
	backRight(speed, REVERSE);
	frontRight(speed, FORWARD);
}

void stopMotors()
{
	frontLeft(0, FORWARD);
	backLeft(0, FORWARD);
	backRight(0, FORWARD);
	frontRight(0, FORWARD);
}

void vacuumOn()
{
	servo(SERVO_RELAY, 255);
}

void vacuumOff()
{
	servo(SERVO_RELAY, 0);
}

void stopLauncher()
{
	launcherSpeed(LAUNCHER_STOP);
}

//! Sets the speed of the launcher motors: 128-255 for forward operation.
void launcherSpeed(u08 speed)
{
	requestedLauncherSpeed = speed;
}

void launcherExec()
{
	if (ABS(curLauncherSpeed - requestedLauncherSpeed) <= LAUNCHER_SPEED_STEP)
		curLauncherSpeed = requestedLauncherSpeed;
	else if (requestedLauncherSpeed > curLauncherSpeed)
		curLauncherSpeed += LAUNCHER_SPEED_STEP;
	else if (requestedLauncherSpeed < curLauncherSpeed)
		curLauncherSpeed -= LAUNCHER_SPEED_STEP;

	motor0(curLauncherSpeed);
	motor1(curLauncherSpeed);
}


void haltRobot()
{
	stopMotors();
	stopLauncher();
}
