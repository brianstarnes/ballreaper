#ifndef PACKETPROTOCOL_H
#define PACKETPROTOCOL_H

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

/*! The minimum number of bytes in a packet (a packet with no data section).
 *  Includes: start1, start2, packetType, sequenceNum, dataLength, and 2 CRC bytes.
 */
#define PACKET_OVERHEAD 7

/*! The maximum number of data payload bytes in a packet.
 *  Limited to 255 by dataLength only being one byte wide.
 */
#define MAX_PACKET_DATA 200

void initPacketDriver();
void execPacketDriver();
void sendBootNotification();
void sendPacket(const DownlinkPacketType packetType, const u08 *const data, const u08 dataLength);

#endif
