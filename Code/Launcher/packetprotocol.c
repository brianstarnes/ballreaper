#include "debug.h"
#include "LCD.h"
#include "main.h"
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
static u08 downSequenceNum = 0;

static ValidateDataLengthCallback_t validator = NULL;
static ExecCallback_t executor = NULL;
static u08 maxPacketType = 0;

//Stats

//! Counter that tracks how many times a buffer overfill condition occurs, for development/testing purposes.
volatile u16 bufferOverfilledCounter = 0;

static volatile u08 codeTracks[16];

//Local Prototypes
static void processPacketBuffer();
//static void resetPolyBot(const u08 * const data);
static void codeTrack(const u08 num);
//static void printCodeTracks();
static u16 updateCrcCcitt(const u16 crc, const u08 dataByte);


void configPacketProcessor(ValidateDataLengthCallback_t newValidator, ExecCallback_t newExecutor, u08 newMaxPacketType)
{
	validator = newValidator;
	executor = newExecutor;
	maxPacketType = newMaxPacketType;
}

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

void sendPacket(const u08 packetType, const u08 *const data, const u08 dataLength)
{
	//We must wait until the previous packet is done transmitting before modifying any transmit variables.
	//We simply loop until the Data Register Empty Interrupt Enable bit is cleared.
	while (UCSR0B & (1<<UDRIE0));

	//[0] and [1] are already filled with the start bytes, so start filling at [2]
	transmitBuffer[2] = packetType;
	transmitBuffer[3] = downSequenceNum;
	transmitBuffer[4] = dataLength;

	//CRC-CCITT initializes all bits to 1
	u16 crc = 0xFFFF;
	//calculate CRC-CCITT over packetType, sequenceNum, dataLength, and data bytes
	crc = updateCrcCcitt(crc, packetType);
	crc = updateCrcCcitt(crc, downSequenceNum);
	crc = updateCrcCcitt(crc, dataLength);

	//copy data bytes into transmitBuffer and roll them into the CRC
	for (u08 i = 0; i < dataLength; i++)
	{
		transmitBuffer[5 + i] = data[i];
		crc = updateCrcCcitt(crc, data[i]);
	}

	//store the CRC, MSB first
	transmitBuffer[5 + dataLength] = (u08)(crc >> 8);
	transmitBuffer[6 + dataLength] = (u08)crc;

	//increment downSequenceNum
	downSequenceNum++;

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
}

enum PacketStates
{
	STATE_Start1,
	STATE_Start2,
	STATE_PacketType,
	STATE_SequenceNum,
	STATE_DataLength,
	STATE_DataSection,
	STATE_CrcMsb,
	STATE_CrcLsb,
	NUM_States
};


//! Runs through a state machine with the received bytes.
static void processPacketBuffer()
{
	static u08 processIndex = 0;
	static u08 state = STATE_Start1, receiveByte;
	static u08 packetType, sequence, dataLength, dataCounter;
	static u16 computedCRC, receivedCRC;
	static u08 dataBuffer[DATA_SECTION_LENGTH];

	while (processIndex != tail)
	{
		receiveByte = receiveBuffer[processIndex++];
		//If processIndex is now beyond the end of the buffer, wrap around to the beginning.
		if (processIndex >= RX_BUFFER_LENGTH)
		{
			processIndex = 0;
		}
/*
lcdCursor(0,0);
printHexDigit(state);
printChar(' ');
printHex_u08(receiveByte);
printChar(' ');
*/

		switch (state)
		{
			case STATE_Start1:
				//we don't need to keep storing the byte in the buffer
				head++;
				if (receiveByte == START_BYTE1)
				{
					state = STATE_Start2;
				}
				//else state = STATE_Start1 (no change)
				break;
			case STATE_Start2:
				//we don't need to keep storing the byte in the buffer
				head++;
				if (receiveByte == START_BYTE2)
				{
					state = STATE_PacketType;
				}
				else if (receiveByte != START_BYTE1)
				{
					state = STATE_Start1;
				}
				//else receiveByte is START_BYTE1, so stay in STATE_Start2.
				break;
			case STATE_PacketType:
				//we don't need to keep storing the byte in the buffer
				head++;
				//Check if this is a valid packet type to receive
				if (receiveByte <= maxPacketType)
				{
					packetType = receiveByte;
					state = STATE_SequenceNum;
				}
				else if (receiveByte == START_BYTE1)
				{
					state = STATE_Start2;
				}
				else
				{
					state = STATE_Start1;
				}
				break;
			case STATE_SequenceNum:
				//we don't need to keep storing the byte in the buffer
				head++;
				//can't do any checking on sequence, so just store it
				sequence = receiveByte;
				state = STATE_DataLength;
				break;
			case STATE_DataLength:
				//we don't need to keep storing the byte in the buffer
				head++;

				//Call the registered validator function to validate the
				//dataLength allowed by this specific packetType.
				bool valid;
				if (validator != NULL)
				{
					logDebug("call validator %d", packetType);
					valid = validator(packetType, receiveByte);
				}
				else
				{
					logDebug("no validator");
					valid = (receiveByte <= MAX_PACKET_DATA);
				}

				//if length was invalid, do parser recovery.
				if (!valid)
				{
					//check if the sequence number and data length have start bytes.
					if (sequence == START_BYTE1 && receiveByte == START_BYTE2)
					{
						//looks like a start sequence, so go to STATE_PacketType
						state = STATE_PacketType;
					}
					else if (receiveByte == START_BYTE1)
					{
						state = STATE_Start2;
					}
					else
					{
						//reset packet parser to beginning
						state = STATE_Start1;
					}
				}
				else
				{
					//data length is valid, so save it
					dataLength = receiveByte;
					//CRC-CCITT initializes all bits to 1
					computedCRC = 0xFFFF;
					//calculate CRC-CCITT over packetType, sequenceNum, and dataLength
					computedCRC = updateCrcCcitt(computedCRC, packetType);
					computedCRC = updateCrcCcitt(computedCRC, sequence);
					computedCRC = updateCrcCcitt(computedCRC, dataLength);

					if (dataLength == 0)
					{
						//skip STATE_DataSection because there is no data
						state = STATE_CrcMsb;
					}
					else
					{
						//initialize dataCounter used to count down data bytes in Data section state
						dataCounter = dataLength;
						state = STATE_DataSection;
					}
				}
				break;
			//Data section, dataLength bytes long
			case STATE_DataSection:
				//keep the data bytes in the circular buffer (don't modify head here)
				//add to CRC
				computedCRC = updateCrcCcitt(computedCRC, receiveByte);
				dataCounter--;
				//if all data has been received, advance to STATE_CrcMsb
				if (dataCounter == 0)
				{
					state = STATE_CrcMsb;
				}
				break;
			//MSB of 16-bit CRC-CCITT
			case STATE_CrcMsb:
				receivedCRC = ((u16)receiveByte) << 8;
				state = STATE_CrcLsb;
				break;
			//LSB of 16-bit CRC-CCITT
			case STATE_CrcLsb:
				receivedCRC |= receiveByte;
				//verify that the checksums match
				if (receivedCRC == computedCRC)
				{
					codeTrack(5);
					//CRC values matched, so copy the data section to another buffer to linearize it and free up space in receiveBuffer.
					for (u08 i = 0; i < dataLength; i++)
					{
						dataBuffer[i] = receiveBuffer[head++];
						//If head is now beyond the end of the buffer, wrap around to the beginning.
						if (head >= RX_BUFFER_LENGTH)
						{
							head = 0;
						}
					}
					//Free up the space occupied by the two CRC bytes in the receiveBuffer.
					head += 2;
					//If head is now beyond the end of the buffer, wrap around to the beginning.
					if (head >= RX_BUFFER_LENGTH)
					{
						head = head - RX_BUFFER_LENGTH;
					}
					//Call the registered exec function, if any
					logDebug("Exec packet %d", packetType);
					if (executor != NULL)
					{
						//TODO: start debug code
						clearScreen();
						printString("exec pkt ");
						print_u08(packetType);
						printChar(' ');
						print_u08(dataLength);
						//TODO: end debug code
						executor(packetType, dataBuffer, dataLength);
					}
				}
				else
				{
					//TODO: start debug code
					clearScreen();
					printString("BadCRC comp!=rcv");
					lowerLine();
					printHex_u16(computedCRC);
					printChar(' ');
					printHex_u16(receivedCRC);
					//TODO: end debug code

					logDebug("Bad CRC %02X != %02X", computedCRC, receivedCRC);
					codeTrack(6);
					//CRC values don't match, packet is either corrupted or we are out of sync with a real packet boundary.
					//Recover as rapidly as possible by searching for packet starts within the data we already received.

					//check if the sequence number and data length have start bytes.
					if (sequence == START_BYTE1 && dataLength == START_BYTE2)
					{
						//looks like a start sequence, so go to Packet Type state
						state = STATE_PacketType;
					}
					else if (dataLength == START_BYTE1)
					{
						state = STATE_Start2;
					}
					else
					{
						state = STATE_Start1;
					}
					//Back up the parser to re-process what we originally thought were data and checksum in the receiveBuffer.
					processIndex = head;
				}
				break;
			default:
				//Fell out of packet parser. This should be impossible.
				ledOn();
				SOFTWARE_FAULT("invalid parser state", state, processIndex);
				state = STATE_Start1;
				break;
		}
	}
}

//! Updates a CRC CCITT to include another byte of data.
static u16 updateCrcCcitt(const u16 crc, const u08 dataByte)
{
	return _crc_ccitt_update(crc, dataByte);
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
