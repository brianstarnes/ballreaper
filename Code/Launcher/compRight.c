#include "compRight.h"

#include "ADC.h"
#include "debug.h"
#include "launcherPackets.h"
#include "LCD.h"
#include "main.h"
#include "motors.h"
#include "packetprotocol.h"
#include "rtc.h"
#include "serial.h"
#include "servos.h"
#include "util.h"
#include "utility.h"

static void compStart();
static void compTurnLeft();

enum {
	COMP_DRIVE_FORWARD,
	COMP_TURN_LEFT,
	COMP_COLLECT_DRV_FWD,
	COMP_COLLECT_DRV_BACK,
	COMP_DONE
};

static int compState;
static u08 refills = 0;
static u08 launcherMotorSpeed;
static u08 startTimeSecs;

void compRightInit()
{
	rtcRestart();

	configPacketProcessor(&validateLauncherPacket, &execLauncherPacket, LAST_UplinkPacketType - 1);

	resetEncoders();
	compStart();
	compState = COMP_DRIVE_FORWARD;
}

void compRightExec()
{
	switch (compState) {
		case COMP_DRIVE_FORWARD:
		{
			s16 adjustWall;
			s16 adjustInner;



			adjustWall = (BACK_WALL_TICK_LEN - wallEncoderTicks) * FAST_SPEED_WALL_WHEEL / BACK_WALL_TICK_LEN;
			adjustInner = (BACK_WALL_TICK_LEN - innerEncoderTicks) * FAST_SPEED_INNER_WHEEL / BACK_WALL_TICK_LEN;

			if (adjustWall < 0)
				adjustWall = 0;

			if (adjustInner < 0)
				adjustInner = 0;

			pidDrive(SLOW_SPEED_WALL_WHEEL + adjustWall, SLOW_SPEED_INNER_WHEEL + adjustInner);

			//drive until either side wall switches hit
			if (REAR_SIDE_WALL_HIT || FRONT_SIDE_WALL_HIT)
			{
				delayMs(500);
				pidStop = TRUE;
				compTurnLeft();
				compState = COMP_TURN_LEFT;
			}
			break;
		}

		case COMP_TURN_LEFT:
			// Turn left until you hit the 90 switch
			if (PIVOT_HIT)
			{
				//Drive backwards into wall until back right switch hits
				if (!BACK_RIGHT_HIT)
					driveForward(-SLOW_SPEED_WALL_WHEEL, -SLOW_SPEED_INNER_WHEEL);
				else
				{
					// Start Ball Reaping! We only get 2 refills so make them count!
					compCollectFwd();
					// initialize launcher speed variable to the same value used in compCollectFwd
					launcherMotorSpeed = LAUNCHER_SPEED_FAR;
					startTimeSecs = secCount;
					compState = COMP_COLLECT_DRV_FWD;
				}
			}
			break;

		case COMP_COLLECT_DRV_FWD:
			if (FRONT_HIT || (secCount - startTimeSecs) > 18)
			{
				compCollectBack();
				startTimeSecs = secCount;
				compState = COMP_COLLECT_DRV_BACK;
			}
			else
			{
				/* Pause every couple ticks to give the feeder and shooter time to get rid
				 * of the balls we just collected so that we don't drop any.
				 */
				if (innerEncoderTicks < 40 || wallEncoderTicks < 40)
				  hugWallForwards();
				else
				{
					stop();
					delayMs(2000);

					// ramp launcher speed down as you get closer to the goal
					launcherMotorSpeed -= 7;
					// ensure that we keep a minimum launcher speed.
					if (launcherMotorSpeed < LAUNCHER_SPEED_NEAR)
						launcherMotorSpeed = LAUNCHER_SPEED_NEAR;

					launcherSpeed(launcherMotorSpeed);

					resetEncoders();
				}
			}
			break;

		case COMP_COLLECT_DRV_BACK:
			if (BACK_RIGHT_HIT || BACK_LEFT_HIT || (secCount - startTimeSecs) > 6)
			{
				refills++;
				stop();
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
					compState = COMP_COLLECT_DRV_FWD;
				}
			}
			else
			{
				pidDrive(-SLOW_SPEED_BK_WALL_WHEEL, -SLOW_SPEED_BK_INNER_WHEEL - 2);
				//hugWallBackwards();
			}
			break;

		case COMP_DONE:
			buttonWait();
			break;
	}
} // End competition

static void compStart()
{
	clearScreen();
	lowerLine();
	printString_P(PSTR("Drive forward"));

	pidDrive(FAST_SPEED_WALL_WHEEL, FAST_SPEED_INNER_WHEEL);
	resetEncoders();
}

static void compTurnLeft()
{
	// Hit wall so turn left
	clearScreen();
	printString_P(PSTR("Turn Left"));

    turnLeft();
}

