#include "ADC.h"
#include "debug.h"
#include "LCD.h"
#include "motors.h"
#include "packetprotocol.h"
#include "remoteControl.h"
#include "serial.h"
#include "servos.h"
#include "utility.h"

volatile bool remoteExited = FALSE;
volatile u08 ReceivedData[MAX_DATA];
volatile u08 DataIndex = 0;

//Prototypes
inline u16 parse_u16(const u08 * const data);
void getVersion();
ServoRange namedServoRangeByIndex(u08 number);
bool remoteSystemValidator(const u08 packetType, const u08 dataLength);
void remoteSystemExecutor(const u08 packetType, const u08 * const data, const u08 dataLength);

void remoteSystemInit()
{

}

//! Runs the Remote System functionality until exited.
void remoteSystemExec()
{
	//configure packet processor for Remote System packets
	configPacketProcessor(&remoteSystemValidator, &remoteSystemExecutor, NUM_CMD - 1);
	printString_P(PSTR("Remote Control"));
	lowerLine();
	printString_P(PSTR("System v" REMOTE_SYSTEM_VERSION));

	//process received packets until exit, everything else is handled by interrupts
	remoteExited = FALSE;
	while (remoteExited == FALSE)
	{
		execPacketDriver();
	}
}

//! Parses a u16 value from a byte array.
inline u16 parse_u16(const u08 * const data)
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

//! Validates the packetType and its dataLength.
bool remoteSystemValidator(const u08 packetType, const u08 dataLength)
{
	switch(packetType)
	{
		//parameterless commands
		case CMD_GET_VERSION:
		case CMD_LED_ON:
		case CMD_LED_OFF:
		case CMD_RELAY_ON:
		case CMD_RELAY_OFF:
		case CMD_LCD_ON:
		case CMD_LCD_OFF:
		case CMD_CLEAR_SCREEN:
		case CMD_LOWER_LINE:
		case CMD_GET_BUTTON1:
		case CMD_KNOB:
		case CMD_KNOB10:
		case CMD_SOFT_RESET:
		case CMD_STOP_SOUND:
		case CMD_EXIT_REMOTE:
			return (dataLength == 0);

		//single-parameter commands
		case CMD_DELAY_MS:
		case CMD_DELAY_US:
			return (dataLength == 2);
		case CMD_PRINT_STRING:
			return (dataLength > 0 && dataLength <= MAX_PACKET_DATA);
		case CMD_DIGITAL_INPUT:
		case CMD_ANALOG:
		case CMD_ANALOG10:
		case CMD_SERVO_OFF:
		case CMD_GET_SERVO_RANGE:
		case CMD_PLAY_SOUND:
			return (dataLength == 1);

		//two-parameter commands
		case CMD_SET_SERVO_RANGE_BY_INDEX:
		case CMD_SET_SERVO_RANGE:
		case CMD_SERVO:
		case CMD_SERVO2:
		case CMD_MOTOR:
		case CMD_LCD_CURSOR:
			return (dataLength == 2);
		default:
			SOFTWARE_FAULT("invalid packetType", packetType, dataLength);
			return FALSE;
	}
}

//! Executes a packet of the specified type.
void remoteSystemExecutor(const u08 packetType, const u08 * const data, const u08 dataLength)
{
	u16 u16temp;

	switch (packetType)
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
		/*TODO
		case CMD_RELAY_ON:
			relayOn();
			break;
		case CMD_RELAY_OFF:
			relayOff();
			break;*/
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
			//print up to dataLength characters
			printStringN((char *)data, dataLength);
			break;
		case CMD_SERVO_OFF:
			servoOff(data[0]);
			//TODO remove this debug code
			if (data[0] == 0)
			{
				upperLine();
			}
			else if (data[0] == 1)
			{
				lowerLine();
			}
			printString("Off ");
			break;
		//case CMD_PLAY_SOUND:
		//	playSoundNum(parse_u16(data));
		//	break;

		//two-parameter functions
		case CMD_SERVO:
			servo(data[0], data[1]);
			//TODO remove this debug code
			if (data[0] == 0)
			{
				upperLine();
			}
			else if (data[0] == 1)
			{
				lowerLine();
			}
			print_u08(data[1]);
			printChar(' ');
			break;

		case CMD_SERVO2:
			servo2(data[0], (s08)data[1]);
			//TODO remove this debug code
			if (data[0] == 0)
			{
				upperLine();
			}
			else if (data[0] == 1)
			{
				lowerLine();
			}
			print_s08((s08)data[1]);
			break;

		case CMD_MOTOR:
			if (data[0] == 0)
			{
#if USE_MOTOR0 == 1
				motor0(data[1]);
#endif
			}
			else if (data[0] == 1)
			{
#if USE_MOTOR1 == 1
				motor1(data[1]);
#endif
			}
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
		case CMD_DIGITAL_INPUT:
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
		/*TODO
		case CMD_KNOB:
			uart0Transmit(knob());
			break;
		case CMD_KNOB10:
			u16temp = knob10();
			uart0Transmit(u16temp >> 8);
			uart0Transmit((u08)u16temp);
			break;*/
		default:
			// Command was not recognized
			clearScreen();
			printString("CMD Unrecognized");
			lowerLine();
			printString("len=");
			print_u08(dataLength);
			printChar(' ');
			printString("cmd=");
			print_u08(packetType);
			break;
	}
}
