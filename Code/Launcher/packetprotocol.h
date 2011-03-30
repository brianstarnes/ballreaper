#ifndef PACKETPROTOCOL_H
#define PACKETPROTOCOL_H

#include "globals.h"

//! Defines a function pointer type for a method that validates dataLength.
typedef bool(*ValidateDataLengthCallback_t)(const u08 packetType, const u08 dataLength);
//! Defines a function pointer type for a method that handles a received packet.
typedef void(*ExecCallback_t)(const u08 packetType, const u08 * const data, const u08 dataLength);

/*! The minimum number of bytes in a packet (a packet with no data section).
 *  Includes: start1, start2, packetType, sequenceNum, dataLength, and 2 CRC bytes.
 */
#define PACKET_OVERHEAD 7

/*! The maximum number of data payload bytes in a packet.
 *  Limited to 255 by dataLength only being one byte wide.
 */
#define MAX_PACKET_DATA 200

void configPacketProcessor(ValidateDataLengthCallback_t validate, ExecCallback_t exec, u08 maxPacketType);
void initPacketDriver();
void execPacketDriver();
void sendPacket(const u08 packetType, const u08 *const data, const u08 dataLength);

extern volatile u16 bufferOverfilledCounter;

#endif
