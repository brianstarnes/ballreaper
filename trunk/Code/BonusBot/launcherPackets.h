#ifndef LAUNCHERPACKETS_H
#define LAUNCHERPACKETS_H

#include "globals.h"

/*! Defines the valid PC to robot packets.
    Avoid defining a packetType that is the same as ::START_BYTE1.
 */
typedef enum
{
	GET_VERSIONS,
	PAUSE,
	RESUME,
	ABORT_TO_MENU,
	GET_STATS,
	LAST_UplinkPacketType
} UplinkPacketType;

/*! Defines the valid robot to PC packets.
    Avoid defining a packetType that is the same as ::START_BYTE1.
 */
typedef enum
{
	BOOTED_UP = 128,
	VERSION_DATA,
	STATS_DATA,
	TELEMETRY_DATA,
	DEBUG_LOG,
	WARNING_LOG,
	CRITICAL_LOG,
	SW_FAULT,
	LAST_DownlinkPacketType
} DownlinkPacketType;

//Prototypes
bool validateLauncherPacket(const u08 packetType, const u08 dataLength);
void execLauncherPacket(const u08 packetType, const u08 * const data, const u08 dataLength);
void sendBootNotification(u08 resetCause);

#endif
