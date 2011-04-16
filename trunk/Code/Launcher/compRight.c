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
static void compWaitRefill();

enum {
	COMP_DRIVE_FORWARD,
	COMP_TURN_LEFT,
	COMP_COLLECT_DRV_FWD,
	COMP_EMPTY_HOPPER,
	COMP_COLLECT_DRV_BACK,
	COMP_WAIT_REFILL,
	COMP_DONE
};

static int compState;
static u08 refills = 0;
static u08 startTimeSecs;

void compRightInit()
{
	rtcRestart();

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

			adjustWall  = ((BACK_WALL_TICK_LEN - (s16)wallEncoderTicks) * (FAST_SPEED_WALL_WHEEL - SLOW_SPEED_WALL_WHEEL)) / BACK_WALL_TICK_LEN;
			adjustInner = ((BACK_WALL_TICK_LEN - (s16)innerEncoderTicks) * (FAST_SPEED_INNER_WHEEL - SLOW_SPEED_INNER_WHEEL)) / BACK_WALL_TICK_LEN;

			if (adjustWall < 0)
				adjustWall = 0;
			else if (adjustWall > (FAST_SPEED_WALL_WHEEL - SLOW_SPEED_WALL_WHEEL))
				adjustWall = FAST_SPEED_WALL_WHEEL - SLOW_SPEED_WALL_WHEEL;

			if (adjustInner < 0)
				adjustInner = 0;
			else if (adjustInner > (FAST_SPEED_INNER_WHEEL - SLOW_SPEED_INNER_WHEEL))
				adjustInner = FAST_SPEED_INNER_WHEEL - SLOW_SPEED_INNER_WHEEL;


			hugWallStrafe((u08)(SLOW_SPEED_WALL_WHEEL + adjustWall), (u08)(SLOW_SPEED_INNER_WHEEL + adjustInner));

			//drive until either side wall switches hit
			if (REAR_SIDE_WALL_HIT || FRONT_SIDE_WALL_HIT)
			{
				delayMs(500);
				//pidStop = TRUE;
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

					startTimeSecs = secCount;
					compState = COMP_COLLECT_DRV_FWD;
				}
			}
			break;

		case COMP_COLLECT_DRV_FWD:
			if (FRONT_HIT || (secCount - startTimeSecs) > 25)
			{
				compEmptyHopper();
				startTimeSecs = secCount;
				compState = COMP_EMPTY_HOPPER;
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

					resetEncoders();
				}
			}
			break;

		case COMP_EMPTY_HOPPER:
			if ((secCount - startTimeSecs) > 5)
			{
				compCollectBack();
				startTimeSecs = secCount;
				compState = COMP_COLLECT_DRV_BACK;
			}
			break;

		case COMP_COLLECT_DRV_BACK:
			if (BACK_RIGHT_HIT || BACK_LEFT_HIT || (secCount - startTimeSecs) > 20)
			{
				stop();
				refills++;
				if (refills == 6)
				{
					compDone();
					compState = COMP_DONE;
				}
				else
				{
					compWaitRefill();
					startTimeSecs = secCount;
					compState = COMP_WAIT_REFILL;
				}
			}
			else
			{
				hugWallBackwards();
			}
			break;

		case COMP_WAIT_REFILL:
			stop();
			pidStop = TRUE;
			if ((secCount - startTimeSecs) > 15)
			{
				compCollectFwd();
				startTimeSecs = secCount;
				compState = COMP_COLLECT_DRV_FWD;
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
	pidStop = TRUE;
	resetEncoders();
}

static void compTurnLeft()
{
	// Hit wall so turn left
	clearScreen();
	lowerLine();
	printString_P(PSTR("Turn Left"));

    turnLeft();
}

static void compWaitRefill()
{
	clearScreen();
	lowerLine();
	printString_P(PSTR("Wait for refill"));
}

