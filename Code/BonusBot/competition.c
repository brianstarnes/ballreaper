#include "LCD.h"
#include "competition.h"

#include "ADC.h"
#include "bonusbot.h"
#include "motors.h"
#include "rtc.h"
#include "serial.h"
#include "servos.h"
#include "util.h"
#include "utility.h"


static void compGrabBonusBall();

static int compState;
static u08 startTimeSecs;


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
	driveForward();
	printString_P(PSTR("Drive Forward"));
	compState = COMP_DRV_TO_WALL;
}

void compExec()
{
	u08 leftHit = BB_LEFT_HIT;
	u08 rightHit = BB_RIGHT_HIT;
	u08 backLeftHit = BB_BACK_LEFT_HIT;
	u08 backRightHit = BB_BACK_RIGHT_HIT;

	switch (compState) {
		case COMP_DRV_TO_WALL:
			if (leftHit && rightHit)
			{
				clearScreen();
				printString_P(PSTR("Slide right"));
				startTimeSecs = secCount;
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

			// TODO base this off of line sensors...
			if ((secCount - startTimeSecs) > 10)
			{
				compGrabBonusBall();
				compState = COMP_GRAB_BONUS_BALL;
			}
//			if ((qrdFrontLeftReading > 200) && (qrdFrontRightReading > 200))
//			{
//				stopMotors();
//				if ((qrdFrontLeftReading > 200) && (qrdFrontRightReading > 200))
//				  compState = COMP_GRAB_BONUS_BALL;
//				else
//				{
//					strafeLeft(DRIVE_SLOW_SPEED);
//					if ((qrdFrontLeftReading > 200) && (qrdFrontRightReading > 200))
//					{
//						stopMotors();
//						compState = COMP_GRAB_BONUS_BALL;
//					}
//				}
//			}
			break;

		case COMP_GRAB_BONUS_BALL:
			// wait a few seconds to grab the ball.
			if ((secCount - startTimeSecs) > 15)
			{
				clearScreen();
				printString_P(PSTR("Drive backwards"));
				vacuumOff();
				compState = COMP_DRV_BACK;
			}
			break;

		case COMP_DRV_BACK:
			driveBackwards();

			if (backLeftHit || backRightHit)
			{
				startTimeSecs = secCount;
				clearScreen();
				printString_P(PSTR("Strafe Right"));
				compState = COMP_STRAFE_RIGHT;
			}
			break;

		case COMP_STRAFE_RIGHT:
			strafeRight(DRIVE_FAST_SPEED);

			//TODO: Strafe until both back sensors are off the line and go back the other way...
			// once time is low setup money shot.

			if (secCount > 30)
			{
				startTimeSecs = secCount;
				clearScreen();
				printString_P(PSTR("Setup shot"));
				compState = COMP_STRAFE_LEFT;
			}
			else if ((secCount - startTimeSecs) > 2)
			{
				startTimeSecs = secCount;
				clearScreen();
				printString_P(PSTR("Strafe Left"));
				compState = COMP_SET_UP_MONEY_SHOT;
			}
			break;

		case COMP_STRAFE_LEFT:
			strafeLeft(DRIVE_FAST_SPEED);

			//TODO: Strafe until sensors cross black line and go off and go back the other way...
			// once time is low setup money shot.
			if (secCount > 30)
			{
				startTimeSecs = secCount;
				clearScreen();
				printString_P(PSTR("Setup shot"));
				compState = COMP_SET_UP_MONEY_SHOT;
			}
			else if ((secCount - startTimeSecs) > 2)
			{
				startTimeSecs = secCount;
				clearScreen();
				printString_P(PSTR("Strafe Right"));
				compState = COMP_STRAFE_LEFT;
			}
			break;

		case COMP_SET_UP_MONEY_SHOT:
			launcherSpeed(LAUNCHER_LAUNCH_SPEED);

			if ((secCount - startTimeSecs) > 2)
			{
				startTimeSecs = secCount;
				clearScreen();
				printString_P(PSTR("Shoot Ball"));
				compState = COMP_SHOOT_BALL;
			}
			//Strafe back to the middle drive forward and shoot
			break;

		case COMP_SHOOT_BALL:
			vacuumOn();


			if ((secCount - startTimeSecs) > 2)
			{
				clearScreen();
				printString_P(PSTR("Fuck yeah!"));
				compState = COMP_DONE;
			}
			break;

		case COMP_DONE:
			buttonWait();
			break;
	}
} // End competition

static void compGrabBonusBall()
{
	clearScreen();
	printString_P(PSTR("Grab ball"));
	startTimeSecs = secCount;
	stopMotors();
	vacuumOn();
	launcherSpeed(LAUNCHER_GRAB_SPEED);
}

