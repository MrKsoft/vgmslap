///////////////////////////////////////////////////////////////////////////////
// __      _______ __  __  _____ _             _
// \ \    / / ____|  \/  |/ ____| |           | |
//  \ \  / / |  __| \  / | (___ | | __ _ _ __ | |
//   \ \/ /| | |_ | |\/| |\___ \| |/ _` | '_ \| |         by Wafflenet
//    \  / | |__| | |  | |____) | | (_| | |_) |_|       www.wafflenet.com
//     \/   \_____|_|  |_|_____/|_|\__,_| .__/(_)
//      (VGM Silly Little AdLib Player) | |
//                                      |_|
//
///////////////////////////////////////////////////////////////////////////////
//
// OPL.H - OPL chip functions
//
///////////////////////////////////////////////////////////////////////////////

#ifndef VGMSLAP_OPL_H
#define VGMSLAP_OPL_H

#include "types.h"

///////////////////////////////////////////////////////////////////////////////
// Function declarations
///////////////////////////////////////////////////////////////////////////////

void detectOPL(void);						// Detect what OPL chip is in the computer.
											// (This determines what VGMs can be played.)
void resetOPL(void);						// Reset OPL to original state, including turning off OPL3 mode
void writeOPL(uint16_t reg, uint8_t data);	// Sends data to OPL chip, register then data

///////////////////////////////////////////////////////////////////////////////
// Variable declarations
///////////////////////////////////////////////////////////////////////////////

extern uint16_t oplBaseAddr;			// Base port for OPL synth.
extern OplDetectedType detectedChip;	// What OPL chip we detect on the system from detectOPL (see types.h)
extern uint8_t oplDelayReg;				// Delay required for OPL register write (set for OPL2 by default)
extern uint8_t oplDelayData;			// Delay required for OPL data write (set for OPL2 by default)
extern char oplRegisterMap[0x1FF];		// Stores current state of OPL registers
extern char oplChangeMap[0x1FF];		// Written alongside oplRegisterMap, tracks bytes that need interpreted/drawn
extern uint8_t commandReg;				// Stores current OPL register to manipulate
extern uint8_t commandData;				// Stores current data to put in OPL register
extern uint8_t maxChannels;				// When iterating channels, how many to go through (9 for OPL2, 18 for OPL3)

// Due to weird operator offsets to form a channel, this is a list of offsets from the base (0x20/0x40/0x60/0x80/0xE0) for each.  First half is OPL2 and second is OPL3, so OPL3 ones have 0x100 added to fit our data model.
// On the chip itself, the operators are laid out as follows:
// [OPL2/OPL3 Ch1-9] 0, 3, 1, 4, 2, 5, 6, 9, 7, 10, 8, 11, 12, 15, 13, 16, 14, 17
// [OPL3 Ch10-18] 18, 21, 19, 22, 20, 23, 24, 27, 25, 28, 26, 29, 30, 33, 31, 34, 32, 35
extern const uint16_t oplOperatorOrder[];

// Offsets from the base of a register category (for instance Attack/Decay at 0x60) and how it maps to a channel/op.
// This covers OPL2 only.  Simply add 9 to the channel number for the OPL3 ones.
extern const uint8_t oplOperatorToChannel[];
extern const uint8_t oplOffsetToOperator[];

///////////////////////////////////////////////////////////////////////////////
// Struct declarations
///////////////////////////////////////////////////////////////////////////////

// Individual OPL operator struct, which are nested in an OPL channel struct
typedef struct
{
	uint8_t attackRate;
	uint8_t decayRate;
	uint8_t sustainLevel;
	uint8_t releaseRate;
	uint8_t flagTremolo;
	uint8_t flagFrequencyVibrato;
	uint8_t flagSoundSustaining;
	uint8_t flagKSR;
	uint8_t frequencyMultiplierFactor;
	uint8_t waveform;
	uint8_t keyScaleLevel;
	uint8_t outputLevel;
} oplOperator;

// Individual OPL channel struct, which are nested in an OPL chip struct
typedef struct
{
	oplOperator operators[2];
	uint16_t frequencyNumber;
	uint8_t blockNumber;
	uint8_t keyOn;
	uint8_t flag4Op;
	uint8_t panning; 		// Two bits - (0x03 = both channels, 0x02 = right, 0x01 = left, 0x00 = neither)
	uint8_t feedback;
	uint8_t synthesisType; 	// Aka "Algorithm"
	uint8_t displayX;		// Channel block location on screen
	uint8_t displayY;		// Channel block location on screen
} oplChannel;

// OPL chip struct
typedef struct
{
	oplChannel channels[18];
	uint8_t flagOPL3Mode;
	uint8_t tremoloDepth;
	uint8_t frequencyVibratoDepth;
	uint8_t percModeData;
} oplChip;

// Storage spot for interpreted OPL status
extern oplChip oplStatus;

#endif
