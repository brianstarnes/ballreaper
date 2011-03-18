#ifndef REMOTEPROTOCOL_H
#define REMOTEPROTOCOL_H

#include <avr/version.h>

//Baud rate and error tolerance are set in the init functions in serial.c

//Define the versions returned by the GET_VERSION command
//Version of the PolyBot Remote System firmware - should be updated when new features or breaking changes are added
#define POLYBOT_REMOTE_VERSION "3.0"

//Version of the PolyBot board being used - etched in copper on the board's back side at the bottom right
//Note: this version is simply inferred based on MCU setting in the Makefile that the code is compiled with
//Therefore it may not be accurate if someone is using a different chip in their board.
#if defined (__AVR_ATmega32__)
	#define POLYBOT_BOARD_VERSION "1.0"
#elif defined (__AVR_ATmega644__) || defined(__AVR_ATmega644P__)
	#define POLYBOT_BOARD_VERSION "1.1"
#endif

//Build a version string
#define VERSION POLYBOT_REMOTE_VERSION "|" POLYBOT_BOARD_VERSION "|" __AVR_LIBC_VERSION_STRING__ "|" __AVR_LIBC_DATE_STRING__

//PolyBot Remote System Commands
//***The biggest command is PRINT_STRING with 1 command byte, 16 characters for the LCD line, and a null terminator
#define MAX_DATA 18

//Packet constants
#define PACKET_START1  0xA5
#define PACKET_START2  0x5A
#define PACKET_ACK     0xC5
#define PACKET_NACK    0x3A

enum Commands
{
	//parameterless commands
	CMD_GET_VERSION,
	CMD_LED_ON,
	CMD_LED_OFF,
	CMD_RELAY_ON,
	CMD_RELAY_OFF,
	CMD_LCD_ON,
	CMD_LCD_OFF,
	CMD_CLEAR_SCREEN,
	CMD_LOWER_LINE,
	CMD_GET_BUTTON1,
	CMD_KNOB,
	CMD_KNOB10,
	CMD_SOFT_RESET,
	CMD_STOP_SOUND,
	CMD_EXIT_REMOTE,

	//single-parameter commands
	CMD_DELAY_MS,
	CMD_DELAY_US,
	CMD_PRINT_STRING,
	CMD_DIGITAL,
	CMD_ANALOG,
	CMD_ANALOG10,
	CMD_SERVO_OFF,
	CMD_PLAY_SOUND,

	//two-parameter commands
	CMD_SET_SERVO_RANGE_BY_INDEX,
	CMD_SET_SERVO_RANGE,
	CMD_GET_SERVO_RANGE,
	CMD_SERVO,
	CMD_SERVO2,
	CMD_MOTOR,
	CMD_LCD_CURSOR,

	NUM_CMD //!< The number of commands defined
};


//Packet responses - packet types that the microcontroller can send
enum Responses
{
	RESP_BOOTED_UP = 0x30,
	RESP_VERSION
};

extern volatile bool remoteExited;

#endif
