#include "competition.h"

#include "ADC.h"
#include "bonusbot.h"
#include "LCD.h"
#include "motors.h"
#include "rtc.h"
#include "serial.h"
#include "servos.h"
#include "util.h"
#include "utility.h"


//Local prototypes
void frontLeft(u08 speed, u08 direction);
void frontRight(u08 speed, u08 direction);
void backRight(u08 speed, u08 direction);
void backLeft(u08 speed, u08 direction);

static int compState;
static u08 startTimeSecs;


enum direction
{
	FORWARD,
	REVERSE
};

enum {
	COMP_DRV_TO_WALL,
	COMP_SLIDE_RIGHT_TO_LINE,
	COMP_GRAB_BONUS_BALL,
	COMP_DRV_BACK,
	COMP_STRAFE_RIGHT,
	COMP_STRAFE_LEFT,
	COMP_SHOOT_BALL,
	COMP_DONE
};

void compInit()
{
	rtcRestart();
	startTimeSecs = secCount;
	compState = COMP_DRV_TO_WALL;
}

void frontLeft(u08 speed, u08 direction)
{
	if (direction)
		servo(SERVO_FRONT_LEFT, speed);
	else
		servo(SERVO_FRONT_LEFT, -speed);
}

void frontRight(u08 speed, u08 direction)
{
	if (direction)
		servo(SERVO_FRONT_RIGHT, speed);
	else
		servo(SERVO_FRONT_RIGHT, -speed);
}

void backRight(u08 speed, u08 direction)
{
	if (direction)
		servo(SERVO_BACK_RIGHT, speed);
	else
		servo(SERVO_BACK_RIGHT, -speed);
}

void backLeft(u08 speed, u08 direction)
{
	if (direction)
		servo(SERVO_BACK_LEFT, speed);
	else
		servo(SERVO_BACK_LEFT, -speed);
}


void compExec()
{
	u08 leftHit = BB_LEFT_HIT;
	u08 rightHit = BB_RIGHT_HIT;

	switch (compState) {
		case COMP_DRV_TO_WALL:
			if (leftHit && rightHit)
			{
				//hit wall, so next we go right
				compState = COMP_SLIDE_RIGHT_TO_LINE;
			}
			else if (leftHit)
			{
				frontLeft(DRIVE_STOP, TRUE);
				backLeft(DRIVE_STOP, TRUE);
				backRight(DRIVE_SLOW_SPEED, TRUE);
				frontRight(DRIVE_SLOW_SPEED, TRUE);
			}
			else if (rightHit)
			{
				frontLeft(DRIVE_SLOW_SPEED, TRUE);
				backLeft(DRIVE_SLOW_SPEED, TRUE);
				backRight(DRIVE_STOP, TRUE);
				frontRight(DRIVE_STOP, TRUE);
			}
			break;

		case COMP_SLIDE_RIGHT_TO_LINE:
			break;

		case COMP_GRAB_BONUS_BALL:
			break;

		case COMP_DRV_BACK:
			break;

		case COMP_STRAFE_RIGHT:
			break;

		case COMP_STRAFE_LEFT:
			break;

		case COMP_SHOOT_BALL:
			break;

		case COMP_DONE:
			buttonWait();
			break;
	}
} // End competition


