#include "debug.h"
#include "launcherPackets.h"
#include "packetprotocol.h"
#include "serial.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

//Local prototypes
static void sendLog(const DownlinkPacketType packetType, const char *messageFormat, const va_list args);

//! Configures the UART to send logs to.
void debugInit()
{

}

//! Logs information that is merely for debugging.
void logDebug(const char *messageFormat, ...)
{
	va_list args;
	va_start(args, messageFormat);
	sendLog(DEBUG_LOG, messageFormat, args);
	va_end(args);
}

//! Logs a warning - something that is cause for concern but is not critical to the system.
void logWarning(const char *messageFormat, ...)
{
	va_list args;
	va_start(args, messageFormat);
	sendLog(WARNING_LOG, messageFormat, args);
	va_end(args);
}

//! Logs a critical error - indicates a runtime error (ex: invalid data) or system/component failure.
void logCritical(const char *messageFormat, ...)
{
	va_list args;
	va_start(args, messageFormat);
	sendLog(CRITICAL_LOG, messageFormat, args);
	va_end(args);
}

/*! Transmits a log message as a packet over a UART.
 *  @param
 */
static void sendLog(const DownlinkPacketType packetType, const char *messageFormat, const va_list args)
{
	u08 buffer[MAX_PACKET_DATA];
	//returns the number of characters printed or that should have printed (not counting null)
	int numChars = vsnprintf((char *)buffer, sizeof buffer, messageFormat, args);
	sendPacket(packetType, buffer, numChars + 1);
}

/*! Logs a software fault - indicates a software bug. Typically used via the SOFTWARE_FAULT macro.
 *  @param filename The name of the source file where the bug is.
 *  @param lineNumber The line number in the source file where the bug is.
 *  @param message The error message to print out.
 */
void logSoftwareFault(const char *filename, u16 lineNumber, const char *message, u16 arg1, u16 arg2)
{
	u08 buffer[MAX_PACKET_DATA];
	buffer[0] = (u08)(lineNumber >> 8);
	buffer[1] = (u08)lineNumber;
	buffer[2] = (u08)(arg1 >> 8);
	buffer[3] = (u08)arg1;
	buffer[4] = (u08)(arg2 >> 8);
	buffer[5] = (u08)arg2;

	u08 index = 6;
	u08 i;
	//copy up to 20 filename characters into buffer
	for (i = 0; (i < 20) && (filename[i] != 0); i++)
	{
		buffer[index++] = filename[i];
	}
	//null terminate the filename
	buffer[index++] = '\0';

	//copy message characters into buffer up to max allowed
	for (i = 0; (index < MAX_PACKET_DATA - 1) && (message[i] != 0); i++)
	{
		buffer[index++] = filename[i];
	}
	//null terminate the message
	buffer[index++] = '\0';

	sendPacket(SW_FAULT, buffer, index);
}
