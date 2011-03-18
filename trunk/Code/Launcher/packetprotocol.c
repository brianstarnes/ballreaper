#include "globals.h"
#include "LCD.h"
#include "packetprotocol.h"
#include "utility.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/version.h>
#include <string.h>
#include <util/crc16.h>

#define START_BYTE1 0xA5
#define START_BYTE2 0x5A

//!Circular buffer, large enough to store the maximum packet size plus 1 byte.
#define RX_BUFFER_LENGTH 255
//!Plain buffer, large enough to store the maximum packet size. Only stores 1 packet at a time.
#define TX_BUFFER_LENGTH 254
//!Plain buffer, just large enough to store the maximum data section (not counting the optional dataLength).
#define DATA_SECTION_LENGTH 20


#define LAUNCHER_FIRMWARE_VERSION "1.0"

//! The version string returned by the ::GET_VERSIONS command. Stored in program space.
#define VERSION_STRING PSTR(LAUNCHER_FIRMWARE_VERSION "|" __TIMESTAMP__ "|" __AVR_LIBC_VERSION_STRING__ "|" __AVR_LIBC_DATE_STRING__)

/*! Indexes that track the beginning and end of the circular ::receiveBuffer.
    When they are equal, there is nothing in the buffer.
    Length of data stored in the buffer is tail - head.

 */
static volatile u08 head = 0, tail = 0;
//! Circular buffer that stores data received from the PC.
static volatile u08 receiveBuffer[RX_BUFFER_LENGTH];
//! Plain buffer that stores data to be sent to the PC.
u08 transmitBuffer[TX_BUFFER_LENGTH];
//! Index in the ::transmitBuffer of the next byte to send.
static volatile u08 transmitIndex;
//! Length in bytes of the current packet in ::transmitBuffer.
static u08 transmitLength;
//! Sequence Number to include in the next outgoing packet.
static u08 sequenceNum = 0;

//Stats

//Counter that tracks whether buffer overfill condition ever occurs, for development/testing purposes.
static volatile u16 bufferOverfilledCounter = 0;

static volatile u08 codeTracks[16];

//Local Prototypes
static void sendVersionData();
static void sendStats();
static void processPacketBuffer();
static void executePacket(const u08 packetType, const u08 * const data, const u08 dataLength);
//static void resetPolyBot(const u08 * const data);
static void codeTrack(const u08 num);
//static void printCodeTracks();

void initPacketDriver()
{
	//these bytes never change
	transmitBuffer[0] = START_BYTE1;
	transmitBuffer[1] = START_BYTE2;

	//initialize codeTracks
	memset((char *)codeTracks, '-', sizeof codeTracks);
}

void execPacketDriver()
{
	//process the serial packet data in receiveBuffer, if any
	processPacketBuffer();
	codeTrack(0);
}

static void codeTrack(const u08 num)
{
	codeTracks[num] = num;
}
/*
static void printCodeTracks()
{
	lowerLine();
	for (u08 i = 0; i < sizeof(codeTracks); i++)
	{
		if (codeTracks[i] == '-')
			printChar('-');
		else
			printHexDigit(codeTracks[i]);
	}
}*/

void sendBootNotification()
{
	sendPacket(BOOTED_UP, NULL, 0);
}

static void sendVersionData()
{
	char *versionString = VERSION_STRING;
	sendPacket(VERSION_DATA, (u08 *)versionString, strlen(versionString));
}

static void sendStats()
{
	//assemble the data section of the STATS_DATA packet
	u08 statsData[2];
	statsData[0] = (u08)(bufferOverfilledCounter >> 8);
	statsData[1] = (u08)bufferOverfilledCounter;

	sendPacket(STATS_DATA, statsData, sizeof(statsData));
}

void sendPacket(const DownlinkPacketType packetType, const u08 *const data, const u08 dataLength)
{
	//We must wait until the previous packet is done transmitting before modifying any transmit variables.
	//We simply loop until the Data Register Empty Interrupt Enable bit is cleared.
	while (UCSR0B & (1<<UDRIE0));

	//[0] and [1] are already filled with the start bytes, so start filling at [2]
	transmitBuffer[2] = packetType;
	transmitBuffer[3] = sequenceNum;
	transmitBuffer[4] = dataLength;

	//CRC-CCITT initializes all bits to 1
	u16 crc = 0xFFFF;
	//calculate CRC-CCITT over packetType, sequenceNum, dataLength, and data bytes
	crc = _crc_ccitt_update(crc, packetType);
	crc = _crc_ccitt_update(crc, sequenceNum);
	crc = _crc_ccitt_update(crc, dataLength);

	//copy data bytes into transmitBuffer and roll them into the CRC
	for (u08 i = 0; i < dataLength; i++)
	{
		transmitBuffer[5 + i] = data[i];
		crc = _crc_ccitt_update(crc, data[i]);
	}

	//store the CRC, MSB first
	transmitBuffer[5 + dataLength] = (u08)(crc >> 8);
	transmitBuffer[6 + dataLength] = (u08)crc;

	//increment sequenceNum
	sequenceNum++;

	//setup variables used by the ISR
	transmitIndex = 0;
	transmitLength = 7 + dataLength;

	codeTrack(2);

	//Enable the USART0_UDRE_vect interrupt by setting Data Register Empty Interrupt Enable bit to 1.
	sbi(UCSR0B, UDRIE0);
}

/*! This interrupt is triggered when the USART's transmit buffer register is empty.
 *  Using this interrupt allows the ATmega CPU to do something else while it waits for each byte to transmit.
 */
ISR(USART0_UDRE_vect)
{
	//Put the current byte into the data register and increment index to next byte.
	UDR0 = transmitBuffer[transmitIndex++];

	//If there is no more data in the buffer to send, then disable this interrupt so it doesn't keep getting called.
	if (transmitIndex >= transmitLength)
	{
		//disable this interrupt by setting Data Register Empty Interrupt Enable bit to 0.
		cbi(UCSR0B, UDRIE0);
	}
	codeTrack(3);
}
/*
//! This interrupt is triggered when a character is received on UART0 from the computer.
ISR(USART0_RX_vect)
{
	//Read the received byte from the UART0 Data Received register and save into the circular receiveBuffer.
	//Increment tail to the next buffer position.
	receiveBuffer[tail++] = UDR0;

	//If tail now matches head (falsely indicating empty), then the buffer is overfilled.
	//We discard the oldest byte of data to make room. Hopefully, this is never necessary.
	if (tail == head)
	{
		head++;

		//Log that a buffer overfilled condition occurred.
		ledOn();
		bufferOverfilledCounter++;
	}
	codeTrack(4);
}*/

//! Runs through a state machine with the received bytes.
static void processPacketBuffer()
{
	static u08 processIndex = 0;
	static u08 state = 0, receiveByte;
	static u08 packetType, sequence, dataLength, dataCounter, dataLengthMayBeData;
	static u16 computedChecksum, receivedChecksum;
	static u08 dataBuffer[DATA_SECTION_LENGTH];

	while (processIndex != tail)
	{
		receiveByte = receiveBuffer[processIndex++];
		//If processIndex is now beyond the end of the buffer, wrap around to the beginning.
		if (processIndex >= RX_BUFFER_LENGTH)
		{
			processIndex = 0;
		}

lcdCursor(0,0);
printHexDigit(state);
printChar(' ');
printHex_u08(receiveByte);
printChar(' ');


		switch (state)
		{
			//START_BYTE1
			case 0:
				//we don't need to keep storing the byte in the buffer
				head++;
				if (receiveByte == START_BYTE1)
					state = 1;
				//else state = 0 (no change)
				break;
			//START_BYTE2
			case 1:
				//we don't need to keep storing the byte in the buffer
				head++;
				if (receiveByte == START_BYTE2)
					state = 2;
				else if (receiveByte != START_BYTE1)
					state = 0;
				//else receiveByte is START_BYTE1, so stay in state 1.
				break;
			//Packet Type
			case 2:
				//we don't need to keep storing the byte in the buffer
				head++;
				//Check if this is a valid packet type for the ROV to receive.
				if (receiveByte < LAST_UplinkPacketType)
				{
					state = 3;
					packetType = receiveByte;
				}
				else if (receiveByte == START_BYTE1)
					state = 1;
				else
					state = 0;
				break;
			//Sequence Number
			case 3:
				//we don't need to keep storing the byte in the buffer
				head++;
				sequence = receiveByte;
				//start computing the checkusum
				computedChecksum = packetType + sequence;
				//Determine the next state based on packetType since it varies by packet.
				//Some packets have no data section, some have a fixed-length data section, and some have a variable-length data section.
				switch (packetType)
				{
					case GET_VERSIONS:
						dataLength = 0;
						break;
					case GET_STATS:
						dataLength = 0;
						break;
					default:
						//variable-length packet, go to Data Length state
						state = 4;
						break;
				}
				//don't modify the state variable if we already set it for a variable-length packet
				if (state != 4)
				{
					//if the packet has no data section, jump to the Checksum MSB state
					if (dataLength == 0)
						state = 6;
					//else go the Data section state
					else
					{
						state = 5;
						//initialize dataCounter used to count down data bytes in Data section state
						dataCounter = dataLength;
					}
					dataLengthMayBeData = 0;
				}
				break;
			//Data Length - this state is only used for variable-length packets, which transmit their data length
			case 4:
				//we don't need to keep storing the byte in the buffer
				head++;
				//determine the maximum data length allowed by this packetType
				u08 maxLength = 0; //TODO not being set
				switch (packetType)
				{
					/*case VARIABLE_PACKET1:
						maxLength = VARIABLE_PACKET1_MAXLENGTH;
						break;
					case VARIABLE_PACKET2:
						maxLength = VARIABLE_PACKET2_MAXLENGTH;
						break;*/
					default:
						//Unsupported packetType - we may be out of sync so do parser recovery next.
						//We are just using state 0 here as a flag to the following code, not necessarily going to state 0 next.
						state = 0;
				}
				//if we had an error (unsupported variable-length packetType or invalid data length), do parser recovery.
				if (state == 0 || receiveByte > maxLength)
				{
					//check if the sequence number and data length have start bytes.
					if (sequence == START_BYTE1 && receiveByte == START_BYTE2)
					{
						//looks like a start sequence, so go to Packet Type state
						state = 2;
					}
					else if (receiveByte == START_BYTE1)
					{
						//go to START_BYTE2 state
						state = 1;
					}
					else
					{
						//reset packet parser to beginning
						state = 0;
					}
				}
				else
				{
					//valid variable-length packetType and valid data length, so save data length
					dataLength = receiveByte;
					//add to checksum
					computedChecksum += dataLength;
					//if dataLength is 0, jump to the Checksum MSB state
					if (dataLength == 0)
						state = 6;
					//else go to Data section state
					else
					{
						state = 5;
						//initialize dataCounter used to count down data bytes in Data section state
						dataCounter = dataLength;
						dataLengthMayBeData = 1;
					}
				}
				break;
			//Data section, dataLength bytes long
			case 5:
				//keep the data bytes in the circular buffer (don't modify head here)
				//add to checksum
				computedChecksum += receiveByte;
				dataCounter--;
				//if all data has been received, advance to Checksum MSB state
				if (dataCounter == 0)
					state = 6;
				break;
			//16-bit Checksum MSB
			case 6:
				receivedChecksum = ((u16)receiveByte) << 8;
				state = 7;
				break;
			//16-bit Checksum LSB
			case 7:
				receivedChecksum |= receiveByte;
				//verify that the checksums match
				if (receivedChecksum == computedChecksum)
				{
					codeTrack(5);
					//Checksums matched, so copy the data section to another buffer to linearize it and free up space in receiveBuffer.
					u08 i;
					for (i = 0; i < dataLength; i++)
					{
						dataBuffer[i] = receiveBuffer[head++];
						//If head is now beyond the end of the buffer, wrap around to the beginning.
						if (head >= RX_BUFFER_LENGTH)
						{
							head = 0;
						}
					}
					//Free up the space occupied by the two checksum characters in the receiveBuffer.
					head += 2;
					//If head is now beyond the end of the buffer, wrap around to the beginning.
					if (head >= RX_BUFFER_LENGTH)
					{
						head = head - RX_BUFFER_LENGTH;
					}
					executePacket(packetType, dataBuffer, dataLength);
				}
				else
				{
					codeTrack(6);
					//Checksums don't match, packet is either corrupted or we are out of sync with a real packet boundary.
					//Recover as rapidly as possible by searching for packet starts within the data we already received.

					//if dataLength was read from the receiveBuffer, we have to check if it contains a start byte
					if (dataLengthMayBeData)
					{
						//check if the sequence number and data length have start bytes.
						if (sequence == START_BYTE1 && dataLength == START_BYTE2)
						{
							//looks like a start sequence, so go to Packet Type state
							state = 2;

						}
						else if (dataLength == START_BYTE1)
						{
							//go to START_BYTE2 state
							state = 1;
						}
						else
						{
							//go to START_BYTE1 state
							state = 0;
						}
					}
					else
					{
						//check if the sequence number is a start byte.
						if (sequence == START_BYTE1)
						{
							//go to START_BYTE2 state
							state = 1;
						}
						else
						{
							//go to START_BYTE1 state
							state = 0;
						}
					}
					//Back up the parser to re-process what we originally thought were data and checksum in the receiveBuffer.
					processIndex = head;
				}
				break;
			default:
				//Fell out of packet parser. This should be impossible.
				ledOn();
				state = 0;
		}
printHexDigit(state);
	}
}

static void executePacket(const u08 packetType, const u08 * const data, const u08 dataLength)
{
codeTrack(7);
	switch (packetType)
	{
		case GET_VERSIONS:
			sendVersionData();
			break;
		case GET_STATS:
			sendStats();
			break;
		default:
			lowerLine();
			printString("Unknown Pkt: ");
			printHex_u08(packetType);
	}
}
/*
static void resetPolyBot(const u08 * const data)
{
codeTrack(9);
	//Check for the reset string in the packet's data section as extra verification that a reset was really intended.
	const char * const resetString = "RESET_POLYBOT";
	u08 i;
	for (i = 0; i < 14; i++)
	{
		if (data[i] != resetString[i])
			return;
	}
	//reset the board
	softReset();
}*/
