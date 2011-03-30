#include "debug.h"
#include "launcherPackets.h"
#include "LCD.h"
#include "main.h"
#include "packetprotocol.h"
#include <avr/version.h>
#include <stddef.h>

//! The version string returned by the ::GET_VERSIONS command. Should be stored in program space.
#define VERSION_STRING (LAUNCHER_FIRMWARE_VERSION "|" __TIMESTAMP__ "|" __AVR_LIBC_VERSION_STRING__ "|" __AVR_LIBC_DATE_STRING__)

//Local Prototypes
static void sendVersionData();
static void sendStats();

bool validateLauncherPacket(const u08 packetType, const u08 dataLength)
{
	switch (packetType)
	{
		case GET_VERSIONS:
		case PAUSE:
		case RESUME:
		case ABORT_TO_MENU:
		case GET_STATS:
			return (dataLength == 0);
		default:
			SOFTWARE_FAULT("invalid packetType", packetType, dataLength);
			return FALSE;
	}
}

void execLauncherPacket(const u08 packetType, const u08 * const data, const u08 dataLength)
{
	switch (packetType)
	{
		case GET_VERSIONS:
			sendVersionData();
			break;
		case PAUSE:
			pauseCompetition();
			break;
		case RESUME:
			resumeCompetition();
			break;
		case ABORT_TO_MENU:
			//TODO
			break;
		case GET_STATS:
			sendStats();
			break;
		default:
			lowerLine();
			printString("Unknown Pkt: ");
			printHex_u08(packetType);
			SOFTWARE_FAULT("Unknown Pkt", packetType, dataLength);
			break;
	}
}

void sendBootNotification(u08 resetCause)
{
	sendPacket(BOOTED_UP, &resetCause, 1);
}

static void sendVersionData()
{
	//allocate a buffer large enough to store the VERSION_STRING.
	char buffer[sizeof(VERSION_STRING)];
	//copy the VERSION_STRING from program memory to the buffer in SRAM.
	strcpy_P(buffer, PSTR(VERSION_STRING));
	//send the packet using the data in SRAM.
	sendPacket(VERSION_DATA, (u08 *)buffer, sizeof(buffer));
}

static void sendStats()
{
	//assemble the data section of the STATS_DATA packet
	u08 statsData[2];
	statsData[0] = (u08)(bufferOverfilledCounter >> 8);
	statsData[1] = (u08)bufferOverfilledCounter;

	sendPacket(STATS_DATA, statsData, sizeof(statsData));
}
