#ifndef _DEBUGTYPES_H_
#define _DEBUGTYPES_H_

#include "SYS_Types.h"

enum EmulatorType
{
	EMULATOR_TYPE_UNKNOWN = 0,
	EMULATOR_TYPE_C64_VICE = 1,
	EMULATOR_TYPE_ATARI800 = 2,
	EMULATOR_TYPE_NESTOPIA = 3
};

#define DEBUGGER_MODE_RUNNING				0x00
#define DEBUGGER_MODE_PAUSED				0x80
#define DEBUGGER_MODE_RUN_ONE_CYCLE			0x81
#define DEBUGGER_MODE_RUN_ONE_INSTRUCTION	0x82
#define DEBUGGER_MODE_SHUTDOWN				0xFF

#define MACHINE_TYPE_UNKNOWN	0
#define MACHINE_TYPE_PAL		1
#define MACHINE_TYPE_NTSC	2
#define MACHINE_TYPE_LOADING_SNAPSHOT	100

#define DEBUGGER_EVENT_TYPE_KEYBOARD	0x01
#define DEBUGGER_EVENT_TYPE_JOYSTICK	0x02

#define DEBUGGER_EVENT_BUTTON_UP		0x00
#define DEBUGGER_EVENT_BUTTON_DOWN		0x01

#define JOYPAD_SELECT	0x80
#define JOYPAD_START	0x40
#define JOYPAD_FIRE_B	0x20
#define JOYPAD_FIRE		0x10
#define JOYPAD_E		0x08
#define JOYPAD_W		0x04
#define JOYPAD_S		0x02
#define JOYPAD_N		0x01
#define JOYPAD_IDLE		0x00
#define JOYPAD_SW   (JOYPAD_S | JOYPAD_W)
#define JOYPAD_SE   (JOYPAD_S | JOYPAD_E)
#define JOYPAD_NW   (JOYPAD_N | JOYPAD_W)
#define JOYPAD_NE   (JOYPAD_N | JOYPAD_E)


enum MemoryBreakpointComparison //: unsigned char
{
	MEMORY_BREAKPOINT_EQUAL = 0,
	MEMORY_BREAKPOINT_NOT_EQUAL,
	MEMORY_BREAKPOINT_LESS,
	MEMORY_BREAKPOINT_LESS_OR_EQUAL,
	MEMORY_BREAKPOINT_GREATER,
	MEMORY_BREAKPOINT_GREATER_OR_EQUAL,
	
	MEMORY_BREAKPOINT_ARRAY_SIZE
};

enum WatchRepresentation
{
	WATCH_REPRESENTATION_HEX_8 = 0,
	WATCH_REPRESENTATION_HEX_16_LITTLE_ENDIAN,
	WATCH_REPRESENTATION_HEX_16_BIG_ENDIAN,
	WATCH_REPRESENTATION_HEX_32_LITTLE_ENDIAN,
	WATCH_REPRESENTATION_HEX_32_BIG_ENDIAN,
	WATCH_REPRESENTATION_UNSIGNED_DEC_8,
	WATCH_REPRESENTATION_UNSIGNED_DEC_16_LITTLE_ENDIAN,
	WATCH_REPRESENTATION_UNSIGNED_DEC_16_BIG_ENDIAN,
	WATCH_REPRESENTATION_UNSIGNED_DEC_32_LITTLE_ENDIAN,
	WATCH_REPRESENTATION_UNSIGNED_DEC_32_BIG_ENDIAN,
	WATCH_REPRESENTATION_SIGNED_DEC_8,
	WATCH_REPRESENTATION_SIGNED_DEC_16_LITTLE_ENDIAN,
	WATCH_REPRESENTATION_SIGNED_DEC_16_BIG_ENDIAN,
	WATCH_REPRESENTATION_SIGNED_DEC_32_LITTLE_ENDIAN,
	WATCH_REPRESENTATION_SIGNED_DEC_32_BIG_ENDIAN,
	WATCH_REPRESENTATION_BIN,
	WATCH_REPRESENTATION_TEXT,
	//	WATCH_REPRESENTATION_OCT,		// not used
	WATCH_REPRESENTATION_MAX
};

// below are all ANSI C definitions that belong to a proper *.h file from the interface, keeping here for now due to inconsistency:

//
// C64 Debugger interface definitions go below:

#define C64_MAX_NUM_SIDS	3
#define C64_NUM_DRIVES 4

enum c64ViciiRecordMode
{
	C64D_VICII_RECORD_MODE_NONE	= 0,
	C64D_VICII_RECORD_MODE_EVERY_LINE,
	C64D_VICII_RECORD_MODE_EVERY_CYCLE
	
};

enum c64PictureMode
{
	C64_PICTURE_MODE_TEXT_HIRES=1,
	C64_PICTURE_MODE_TEXT_MULTI,
	C64_PICTURE_MODE_BITMAP_HIRES,
	C64_PICTURE_MODE_BITMAP_MULTI
};

enum c64CanvasType
{
	C64_CANVAS_TYPE_BLANK	= 0,
	C64_CANVAS_TYPE_BITMAP	= 1,
	C64_CANVAS_TYPE_TEXT	= 2,
};

enum
{
	PAINT_RESULT_ERROR			= 0,
	PAINT_RESULT_OUTSIDE		= 1,
	PAINT_RESULT_BLOCKED		= 2,
	PAINT_RESULT_OK				= 10,
	PAINT_RESULT_REPLACED_COLOR	= 11,
};

struct C64StateCPU
{
	u8 a, x, y;
	u8 processorFlags, sp;
	u16 pc;
	//	uint8 intr[4];		// Interrupt state
	u16 lastValidPC;
	u8 instructionCycle;
	
	u8 memory0001;
};

struct C64StateVIC
{
	int raster_line;
	int raster_cycle;
};


struct C64StateDrive1541
{
	int headTrackPosition;
};

struct C64StateCartridge
{
	u8 exrom;
	u8 game;
};



#endif

