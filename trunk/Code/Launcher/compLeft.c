#include "compLeft.h"

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

static int compState;
static u08 passes = 0;
//static u08 launcherMotorSpeed;
static u08 startTimeSecs;


enum {
	COMP_WAIT_START,
	COMP_LAUNCHER_SPINUP,
	COMP_COLLECT_DRV_FWD,
	COMP_EMPTY_HOPPER,
	COMP_COLLECT_DRV_BACK,
	COMP_DONE
};

void compLeftInit()
{
	rtcRestart();

	startTimeSecs = secCount;
	compState = COMP_WAIT_START;
}

void compLeftExec()
{
	filterDigitalInput(&dBackRight, SWITCH_BACK_WALL_RIGHT);
	filterDigitalInput(&dFront, SWITCH_FRONT_WALL);
	filterDigitalInput(&dSide, SWITCH_PIVOT);

	switch (compState) {
	case COMP_WAIT_START:
		if (!PRESSED(dSide))
		{
			startTimeSecs = secCount;
			compState = COMP_LAUNCHER_SPINUP;
		}
		break;

		case COMP_LAUNCHER_SPINUP:
			if ((secCount - startTimeSecs) > 2) {
				resetEncoders();
				startTimeSecs = secCount;
				compCollectFwd();

				compState = COMP_COLLECT_DRV_FWD;
			}
			break;

		case COMP_COLLECT_DRV_FWD:
			if (PRESSED(dFront) || (secCount - startTimeSecs) > 35)
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
				if (innerEncoderTicks < 15 || wallEncoderTicks < 15)
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
				passes++;
				if (passes == 3)
				{
					compDone();
					compState = COMP_DONE;
				}
				else
				{
					compCollectBack();
					startTimeSecs = secCount;
					compState = COMP_COLLECT_DRV_BACK;
				}
			}
			break;

		case COMP_COLLECT_DRV_BACK:
			if (PRESSED(dBackRight) || (secCount - startTimeSecs) > 10)
			{
				stop();
				clearScreen();
				printString_P(PSTR("Waiting 4 reload"));
				delayMs(10000);

				compCollectFwd();
				startTimeSecs = secCount;
				compState = COMP_COLLECT_DRV_FWD;
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


