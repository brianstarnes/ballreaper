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
void stopMotors();
void strafeRight(u08 speed);
void strafeLeft(u08 speed);

static int compState;
static u08 startTimeSecs;


enum direction
{
	REVERSE,
	FORWARD
};

enum {
	COMP_DRV_TO_WALL,
	COMP_SLIDE_RIGHT_TO_LINE,
	COMP_GRAB_BONUS_BALL,
	COMP_DRV_BACK,
	COMP_STRAFE_RIGHT,
	COMP_STRAFE_LEFT,
	COMP_SET_UP_MONEY_SHOT,
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
	backRight(speed, FORWARD);
	frontRight(speed, REVERSE);
}

void strafeLeft(u08 speed)
{
	frontLeft(speed, REVERSE);
	backLeft(speed, FORWARD);
	backRight(speed, REVERSE);
	frontRight(speed, FORWARD);
}

void compExec()
{
	u08 leftHit = BB_LEFT_HIT;
	u08 rightHit = BB_RIGHT_HIT;

	switch (compState) {
		case COMP_DRV_TO_WALL:
			if (leftHit && rightHit)
			{
				compState = COMP_SLIDE_RIGHT_TO_LINE;
			}
			else if (leftHit)
			{
				frontLeft(DRIVE_STOP, FORWARD);
				backLeft(DRIVE_STOP, FORWARD);
				backRight(DRIVE_SLOW_SPEED, FORWARD);
				frontRight(DRIVE_SLOW_SPEED, FORWARD);
			}
			else if (rightHit)
			{
				frontLeft(DRIVE_SLOW_SPEED, FORWARD);
				backLeft(DRIVE_SLOW_SPEED, FORWARD);
				backRight(DRIVE_STOP, FORWARD);
				frontRight(DRIVE_STOP, FORWARD);
			}
			break;

		case COMP_SLIDE_RIGHT_TO_LINE:
			strafeRight(DRIVE_FAST_SPEED);

			if ((qrdFrontLeftReading > 200) && (qrdFrontRightReading > 200))
			{
				stopMotors();
				if ((qrdFrontLeftReading > 200) && (qrdFrontRightReading > 200))
				  compState = COMP_GRAB_BONUS_BALL;
				else
				{
					strafeLeft(DRIVE_SLOW_SPEED);
					if ((qrdFrontLeftReading > 200) && (qrdFrontRightReading > 200))
					{
						stopMotors();
						compState = COMP_GRAB_BONUS_BALL;
					}
				}
			}
			break;

		case COMP_GRAB_BONUS_BALL:
			//TODO: Turn on relay to suck in ball...
			launcherSpeed(LAUNCHER_GRAB_SPEED);
			break;

		case COMP_DRV_BACK:
			//TODO: Drive backward until we hit the back switches...
			//driveBackwards();

			//if (backWallHit)
				//compState = COMP_STRAFE_RIGHT;
			break;

		case COMP_STRAFE_RIGHT:
			strafeRight(DRIVE_SLOW_SPEED);
			//TODO: Strafe until both back sensors are off the line and go back the other way...
			// once time is low setup money shot.
			break;

		case COMP_STRAFE_LEFT:
			strafeLeft(DRIVE_SLOW_SPEED);
			//TODO: Strafe until sensors cross black line and go off and go back the other way...
			// once time is low setup money shot.
			break;

		case COMP_SET_UP_MONEY_SHOT:
			//Strafe back to the middle drive forward and shoot
			break;

		case COMP_SHOOT_BALL:
			launcherSpeed(LAUNCHER_LAUNCH_SPEED);
			delayMs(500);
			//TODO: Turn on relay to shoot the ball...
			break;

		case COMP_DONE:
			buttonWait();
			break;
	}
} // End competition


