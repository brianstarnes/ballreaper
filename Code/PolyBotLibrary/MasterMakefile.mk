##Copyright (C) 2009-2011  Patrick J. McCarty.
##Licensed under X11 License. See LICENSE.txt for details.

# Specify the port name for your In-System Programmer (ISP) device or cable here.
#  For a FTDI USB-to-UART converter on Windows this is something like COM2 (check in the Windows Device Manager under Ports for the number)
#    In XP: Right-click My Computer, click "Properties" then navigate to the "Hardware" tab followed by clicking "Device Manager".
#    Scroll down to "Ports" and click the "+" to the left of the name. Finally read the "COM" number next to the name "USB Serial Port (COMx)".
#  For a FTDI USB-to-UART converter on Linux this is something like /dev/ttyUSB0 (for a listing of USB serial ports, run: ls /dev/ttyUSB* ).
PORT = /dev/ttyUSB0

# Specify the type of In-System Programmer (ISP) device or cable you are using.
#  For a standard FTDI USB-to-UART converter used with Seng's butterfly bootloader use butterfly -b 57600
ISP = butterfly -b 57600

# Enter the target microcontroller model.
#  For PolyBot 1.0 Boards use atmega32.
#  For PolyBot 1.1 Boards use atmega644 or atmega644P (You must look at the chip to see what you have).
#  For a complete list of models supported by avrdude, run: avrdude -p ?
MCU = atmega644


# Determine which library files to compile and #defines to create based on variables set in the project Makefile.
FILES += $(LIB)/utility.c

ifeq ($(USE_LCD), 1)
	FILES += $(LIB)/LCD.c
	DEFINES += -D USE_LCD=1
endif

ifeq ($(USE_ADC), 1)
	FILES += $(LIB)/ADC.c
	DEFINES += -D USE_ADC=1
endif

ifneq ($(NUM_MOTORS), 0)
	FILES += $(LIB)/motors.c
	DEFINES += -D NUM_MOTORS=$(NUM_MOTORS)
endif

ifneq ($(NUM_SERVOS), 0)
	FILES += $(LIB)/servos.c
	DEFINES += -D NUM_SERVOS=$(NUM_SERVOS)
endif


# Makefile Targets

# Since the "all" target is the first target in the file, it will run when you simply type make (or make all).
#  We have configured this target so that it only compiles the program into .elf and .hex files, and displays their final sizes.

#for a map, add: -Wl,-Map,$(PROJECTNAME).map
#for a lst, add: -Wa,-adhlms=$(PROJECTNAME).lst
#tried unsuccessfully to remove unused code with: -Wl,-static -ffunction-sections -fdata-sections
#the -g is required to get C code interspersed in the disassembly listing
all:
	avr-gcc -g -mmcu=$(MCU) $(DEFINES) -I . -I $(LIB) -Os -Wall -Werror -mcall-prologues -std=gnu99 -o $(PROJECTNAME).elf $(FILES) $(PROJECTNAME).c -lm
	avr-objcopy -O ihex $(PROJECTNAME).elf $(PROJECTNAME).hex
	avr-size $(PROJECTNAME).elf

# This target first executes the "all" target to compile your code, and then programs the hex file into the ATmega using avrdude.
program: all
	avrdude -p $(MCU) -P $(PORT) -c $(ISP) -F -u -U flash:w:$(PROJECTNAME).hex

# This target can be called to delete any existing compiled files (binaries), so you know that your next compile is fresh.
# The dash in front of rm is not passed to the shell, and just tells make to keep running if an error code is returned by rm.
clean:
	-rm -f $(PROJECTNAME).elf $(PROJECTNAME).hex $(PROJECTNAME).lss

# This target generates a .lss extended listing file from your compiled .elf file, and prints info on the sections.
# The file shows you the assembly code with your original C code interspersed between it to help you make sense of the assembly.
# This allows you to inspect the actual AVR instructions that your code will be running, which is useful to compare
# two similar implementations and pick the more optimized version, or to verify correctness of an operation.
asm: all
	avr-objdump -h -S $(PROJECTNAME).elf > $(PROJECTNAME).lss



Launcher:
	avr-gcc -g -mmcu=$(MCU) $(DEFINES) -I . -I $(LIB) -Os -Werror -mcall-prologues -std=c99 -o $(PROJECTNAME).elf $(FILES) $(PROJECTNAME).c -lm
	avr-objcopy -O ihex $(PROJECTNAME).elf $(PROJECTNAME).hex
	avr-size $(PROJECTNAME).elf
