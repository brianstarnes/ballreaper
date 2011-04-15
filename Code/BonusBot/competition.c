#include "competition.h"

#include "ADC.h"
#include "bonusbot.h"
#include "debug.h"
#include "launcherPackets.h"
#include "LCD.h"
#include "motors.h"
#include "packetprotocol.h"
#include "rtc.h"
#include "serial.h"
#include "servos.h"
#include "util.h"
#include "utility.h"

enum direction
{
	FORWARD,
	REVERSE
};

//Local prototypes
void frontLeft(u08 speed, u08 direction);
void frontRight(u08 speed, u08 direction);
void backRight(u08 speed, u08 direction);
void backLeft(u08 speed, u08 direction);

static int compState;
//static u08 launcherMotorSpeed;
static u08 startTimeSecs;

enum {
	COMP_DRV_TO_WALL,
	COMP_RIGHT_APPROACH,
	COMP_LEFT_APPROACH,
	COMP_SLIDE_RIGHT_TO_LINE,
	COMP_GRAB_BONUS_BALL,
	COMP_DONE
};

void compInit()
{
	rtcRestart();
	startTimeSecs = secCount;
	compState = COMP_DRV_TO_WALL;
}

void frontLeft(u08 speed, u08 direction)

void frontRight(u08 speed, u08 direction)

void backRight(u08 speed, u08 direction)

void backLeft(u08 speed, u08 direction)


void compExec()
{
	u08 leftHit = BB_LEFT_HIT;
	u08 rightHit = BB_RIGHT_HIT;

	switch (compState) {
		case COMP_DRV_TO_WALL:
			if (leftHit && rightHit)
			{
				//hit wall, so next we go right
				compState =
			}
			else if (leftHit)
			{
				frontLeft(0);
				backLeft(0);
				backRight(DRIVE_SLOW_SPEED);
				frontRight(DRIVE_SLOW_SPEED);
			}
			else if (rightHit)


			if (LFRONT_HIT || (secCount - startTimeSecs) > 18)
			{
				compCollectBack();
				startTimeSecs = secCount;
				compState = COMP_COLLECT_DRV_BACK;
			}
			else {
				/* Pause every couple ticks to give the feeder and shooter time to get rid
				 * of the balls we just collected so that we don't drop any.
				 */
				if (innerEncoderTicks < 30 || wallEncoderTicks < 30)
				  hugWallForwards();
				else
				{
					stop();
					delayMs(2000);

					// ramp launcher speed down as you get closer to the goal
					//launcherMotorSpeed -= 7;
					// ensure that we keep a minimum launcher speed.
					//if (launcherMotorSpeed < LAUNCHER_SPEED_NEAR)
						//launcherMotorSpeed = LAUNCHER_SPEED_NEAR;

					//launcherSpeed(launcherMotorSpeed);

					resetEncoders();
				}
			}
			break;

		case COMP_COLLECT_DRV_BACK:
			if (LBACK_HIT || (secCount - startTimeSecs) > 6)
			{
				refills++;
				stop();
				clearScreen();
				printString_P(PSTR("Waiting 4 reload"));
				delayMs(15000);
				if (refills == 6)
				{
					compDone();
					compState = COMP_DONE;
				}
				else
				{
					compCollectFwd();
					startTimeSecs = secCount;
					compState = COMP_DRV_FWD;
				}
			}
			else
			{
				hugWallBackwards();
			}
			break;

		case COMP_DONE:
			buttonWait();
			break;
	}
} // End competition


