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
	compState = COMP_COLLECT_DRV_FWD;
}

void compLeftExec()
{
	switch (compState) {
		case COMP_COLLECT_DRV_FWD:
			if (LFRONT_HIT)
			{
				compCollectBack();
				compState = COMP_COLLECT_DRV_BACK;
			}
			else {
				hugWallForwards();
			}
			break;

		case COMP_COLLECT_DRV_BACK:
			if (LBACK_HIT)
			{
				refills++;
				if (refills == 2)
				{
					compDone();
					compState = COMP_DONE;
				}
				else
				{
					compCollectFwd();
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


