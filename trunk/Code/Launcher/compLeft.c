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
static u08 refills = 0;
//static u08 launcherMotorSpeed;
static u08 startTimeSecs;


enum {
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
	compState = COMP_LAUNCHER_SPINUP;
}

void compLeftExec()
{
	switch (compState) {
		case COMP_LAUNCHER_SPINUP:
			if ((secCount - startTimeSecs) > 2) {
				resetEncoders();
				startTimeSecs = secCount;
				compCollectFwd();

				compState = COMP_COLLECT_DRV_FWD;
			}
			break;

		case COMP_COLLECT_DRV_FWD:
			if (LFRONT_HIT || (secCount - startTimeSecs) > 25)
			{
				compEmptyHopper();
				startTimeSecs = secCount;
				compState = COMP_EMPTY_HOPPER;
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
			if (LBACK_HIT || (secCount - startTimeSecs) > 20)
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
					compState = COMP_COLLECT_DRV_FWD;
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


