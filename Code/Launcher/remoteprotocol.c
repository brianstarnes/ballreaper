#include "ADC.h"
#include "globals.h"
#include "LCD.h"
#include "motors.h"
#include "remoteprotocol.h"
#include "serial.h"
#include "servos.h"
#include "utility.h"

volatile bool remoteExited = FALSE;
volatile u08 ReceivedData[MAX_DATA];
volatile u08 DataIndex = 0;

//Prototypes
inline u16 parse_u16(u08 *data);
void getVersion();
ServoRange namedServoRangeByIndex(u08 number);
u08 execute_function(u08 length, u08 command, u08 *data);

//! Parses a u16 value from a byte array.
inline u16 parse_u16(u08 *data)
{
	u16 output;
	//load high byte
	output = ((u16)data[0])<<8;
	//load low byte
	output |= data[1];
	return output;
}

//! Transmits a pipe-separated string containing various version numbers.
void getVersion()
{
	char *version = VERSION;
	u08 i = 0;
	//transmit the characters of the string until hitting a null terminator
	while (version[i] != 0)
	{
		uart0Transmit(version[i++]);
	}
	//Send a newline character to signal the end of the version string
	uart0Transmit('\n');
}

//! Gets one of the predefined ServoRange values based on a generic index number.
ServoRange namedServoRangeByIndex(u08 number)
{
	switch (number)
	{
		case 0:
			return SERVO_RANGE_STANDARD;
		default:
		case 1:
			return SERVO_RANGE_DEFAULT;
		case 2:
			return SERVO_RANGE_EXTENDED1;
		case 3:
			return SERVO_RANGE_EXTENDED2;
		case 4:
			return SERVO_RANGE_EXTENDED3;
		case 5:
	        return SERVO_RANGE_EXTENDED4;
	}
}

u08 execute_function(u08 length, u08 command, u08 *data)
{
	u16 u16temp;

	switch (command)
	{
		//parameterless functions
		case CMD_GET_VERSION:
			getVersion();
			break;
		case CMD_LED_ON:
			ledOn();
			break;
		case CMD_LED_OFF:
			ledOff();
			break;
		case CMD_RELAY_ON:
			relayOn();
			break;
		case CMD_RELAY_OFF:
			relayOff();
			break;
		case CMD_LCD_ON:
			lcdOn();
			break;
		case CMD_LCD_OFF:
			lcdOff();
			break;
		case CMD_CLEAR_SCREEN:
			clearScreen();
			break;
		case CMD_LOWER_LINE:
			lowerLine();
			break;
		case CMD_SOFT_RESET:
			softReset();
			break;
		//case CMD_STOP_SOUND:
		//	stopSound();
		//	break;
		case CMD_EXIT_REMOTE:
			remoteExited = TRUE;
			break;

		//single-parameter functions
		case CMD_DELAY_MS:
			delayMs(parse_u16(data));
			break;
		case CMD_DELAY_US:
			delayUs(parse_u16(data));
			break;
		case CMD_PRINT_STRING:
			data[MAX_DATA - 1] = '\0';
			printString((char *)data);
			break;
		case CMD_SERVO_OFF:
			servoOff(data[0]);
			break;
		//case CMD_PLAY_SOUND:
		//	playSoundNum(parse_u16(data));
		//	break;

		//two-parameter functions
		case CMD_SERVO:
			servo(data[0], data[1]);
			//TODO remove this debug code
			clearScreen();
			printString("servo ");
			print_u08(data[0]);
			printChar(' ');
			print_u08(data[1]);
			break;

		case CMD_SERVO2:
			servo2(data[0], (s08)data[1]);
			//TODO remove this debug code
			clearScreen();
			printString("servo2 ");
			print_u08(data[0]);
			printChar(' ');
			print_s08((s08)data[1]);
			break;

		case CMD_MOTOR:
			//TODO motor(data[0], (s08)data[1]);
			break;
		case CMD_LCD_CURSOR:
			lcdCursor(data[0], data[1]);
			break;

		//functions with return values
		case CMD_SET_SERVO_RANGE_BY_INDEX:
			uart0Transmit(setServoRange(data[0], namedServoRangeByIndex(data[1])));
			break;
		case CMD_SET_SERVO_RANGE:
			uart0Transmit(setServoRange(data[0], data[1]));
			break;
		case CMD_GET_SERVO_RANGE:
			uart0Transmit(getServoRange(data[0]));
			break;
		case CMD_DIGITAL:
			uart0Transmit(digitalInput(data[0]));
			break;
		case CMD_ANALOG:
			uart0Transmit(analog(data[0]));
			break;
		case CMD_ANALOG10:
			u16temp = analog10(data[0]);
			uart0Transmit(u16temp >> 8);
			uart0Transmit((u08)u16temp);
			break;
		case CMD_GET_BUTTON1:
			uart0Transmit(getButton1());
			break;
		case CMD_KNOB:
			uart0Transmit(knob());
			break;
		case CMD_KNOB10:
			u16temp = knob10();
			uart0Transmit(u16temp >> 8);
			uart0Transmit((u08)u16temp);
			break;
		default:
			// Command was not recognized
			lcdCursor(0, 0);
			printString("CMD Unrecognized");
			lowerLine();
			printString("len=");
			print_u08(length);
			printChar(' ');
			printString("cmd=");
			print_u08(command);
			break;
	}
	return 0;
}


//This interrupt is triggered when a character is received on UART0 from the computer.
//It attempts to parse the data as a packet, and then the requested function
//is called and the return value, or an ACK, is transmitted.
//If anything fails, reset DataIndex to 0
#if defined (__AVR_ATmega32__)
ISR(USART_RXC_vect)
{
	u08 data = UDR;
#elif defined (__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
ISR(USART0_RX_vect)
{
	u08 data = UDR0;
#endif

	//prevent overruns of the ReceivedData array
	if (DataIndex >= sizeof ReceivedData)
	{
		//reset DataIndex to 0, effectively ignoring this byte
		DataIndex = 0;
	}
	else if (DataIndex == 0)
	{
		//first byte of a packet must be the start1 byte
		if (data == PACKET_START1)
			ReceivedData[DataIndex++] = data;
		else
			DataIndex = 0;
	}
	else if (DataIndex == 1)
	{
		//second byte of a packet must be the start2 byte
		if (data == PACKET_START2)
			ReceivedData[DataIndex++] = data;
		else
			DataIndex = 0;
	}
	else if (DataIndex == 2)
	{
		//third byte of a packet is the length of the data
		//check that length is within the valid range
	    if (data <= MAX_DATA)
		    ReceivedData[DataIndex++] = data;
		else
		{
			DataIndex = 0;
			//TODO remove this debug code
			clearScreen();
			printString("Bad length");
			lowerLine();
			printString("len=");
			print_u08(data);
		}
	}
	else if (DataIndex == 3)
	{
		//fourth byte of a packet is the command byte that indicates the type of packet
		//check that command is one of the known command values
		if (data < NUM_CMD)
		    ReceivedData[DataIndex++] = data;
		else
		{
			DataIndex = 0;
			//TODO remove this debug code
			clearScreen();
			printString("Bad command");
			lowerLine();
			printString("len=");
			print_u08(ReceivedData[2]);
			printChar(' ');
			printString("cmd=");
			print_u08(data);
		}
	}
	else if (DataIndex < (4 + ReceivedData[2]))
	{
		//if there are data bytes, add each data byte to the receivedData array
		ReceivedData[DataIndex++] = data;
	}
	else if (DataIndex == (4 + ReceivedData[2]))
	{
		//the last byte of a packet is the checksum byte
		//4 (number of header bytes) + data length (ReceivedData[2]) gives the index of the checksum byte

		u08 i, checksum = 0;
		//Checksum is calculated by XOR-ing together the length, command, and data bytes of the packet
		for (i = 2; i <= (3 + ReceivedData[2]); i++)  //3 is index of command byte
		{
			checksum ^= ReceivedData[i];
		}

		//reset Dataindex to prepare for the next packet
		DataIndex = 0;

		//Check that the transmitted checksum matches the calculated checksum
		if (data != checksum)
		{
			//TODO remove this debug code
			clearScreen();
			printString("Bad checksum");
			lowerLine();
			printHex_u08(data);
			printString("!=");
			printHex_u08(checksum);
			return;
		}

		//Because this is a local variable allocated on the stack, it is safe from being overwritten, unlike ReceivedData.
		//We subtract 2 because we don't need a copy of the checksum (it was already verified) and the PACKET_END is never copied.
		u08 copiedData[MAX_DATA];

		//make a copy of the data from ReceivedData that is safe from being overwritten by another instance of this ISR that might run before execute_function completes
		u08 length = ReceivedData[2];
		u08 command = ReceivedData[3];
		//start copying at the first data byte
		for (i = 0; i < length; i++)
		{
		    copiedData[i] = ReceivedData[4+i];
		}

		//now that our data is safely copied, we can enable interrupts to allow interrupt nesting
		sei();

		if (execute_function(length, command, copiedData) != 0)
		{
			//Non-zero return, so indicate failure to computer by sending a NACK
			uart0Transmit(PACKET_NACK);
		}
	}
	else
	{
		//Unexpected error - reset DataIndex
		DataIndex = 0;
	}
}
