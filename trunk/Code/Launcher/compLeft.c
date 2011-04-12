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
static u08 launcherMotorSpeed;
static u08 startTimeSecs;

enum {
	COMP_COLLECT_DRV_FWD,
	COMP_COLLECT_DRV_BACK,
	COMP_DONE
};

void compLeftInit()
{
	rtcRestart();

	configPacketProcessor(&validateLauncherPacket, &execLauncherPacket, LAST_UplinkPacketType - 1);

	resetEncoders();
	compCollectFwd();
	startTimeSecs = secCount;
	compState = COMP_COLLECT_DRV_FWD;
}

void compLeftExec()
{
	switch (compState) {
		case COMP_COLLECT_DRV_FWD:
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


