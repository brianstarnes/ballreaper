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
			//drive until either side wall switches hit
			if (REAR_SIDE_WALL_HIT || FRONT_SIDE_WALL_HIT)
			{
				compTurnLeft();
				pidStop = TRUE;
				compState = COMP_TURN_LEFT;
			}
			break;

		case COMP_TURN_LEFT:
			// Turn left until you hit the 90 switch
			if (PIVOT_HIT)
			{
				stop();
				launcherSpeed(180);
				feederOn();
				// Start Ball Reaping! We only get 2 refills so make them count!
				compCollectFwd();
				compState = COMP_COLLECT_DRV_FWD;
			}
			break;

		case COMP_COLLECT_DRV_FWD:
			if (FRONT_HIT)
			{
				compCollectBack();
				compState = COMP_COLLECT_DRV_BACK;
			}
			else {
				hugWallForwards();
			}
			break;

		case COMP_COLLECT_DRV_BACK:
			if (BACK_RIGHT_HIT || BACK_LEFT_HIT)
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

static void compStart()
{
	clearScreen();
	lowerLine();
	printString_P(PSTR("Drive forward"));

	pidDrive(FAST_SPEED_WALL_WHEEL, FAST_SPEED_INNER_WHEEL);
}

static void compTurnLeft()
{
	// Hit wall so turn left
	clearScreen();
	printString_P(PSTR("Turn Left"));

    turnLeft();
}

