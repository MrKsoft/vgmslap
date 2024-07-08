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
// VGM.H - VGM file processing
//
///////////////////////////////////////////////////////////////////////////////

#ifndef VGMSLAP_VGM_H
#define VGMSLAP_VGM_H

#include "types.h"
#include "deps/zlib.h"

///////////////////////////////////////////////////////////////////////////////
// Function declarations
///////////////////////////////////////////////////////////////////////////////

uint8_t getNextCommandData(void);			// Move through the file based on the commands encountered &
											// load in data for supported commands, to be processed during playback.
wchar_t* getNextGd3String(void);			// Extract the next null terminated string from the overall GD3 tag
uint8_t loadVGM(void);						// Read from the specified VGM file and performs some validity checks
void populateCurrentGd3(void);				// Calls getNextGd3String to populate each GD3 tag value
void processCommands(void);					// Called during the timer loop to process the next VGM command
uint8_t vgmReadBytes(uint16_t numBytes);	// Reads in how many bytes we need for the next VGM command.

///////////////////////////////////////////////////////////////////////////////
// Variable declarations
///////////////////////////////////////////////////////////////////////////////

extern char vgmIdentifier[];		// VGM magic number
extern char gzMagicNumber[2];		// GZ magic number
extern char vgmFileBuffer[256];		// Buffer of up to 256 bytes (which happens to be the max size of the VGM header...)
extern FILE *vgmFilePointer;		// Pointer to loaded VGM file
extern char* vgmFileName;			// Current VGM file name
extern gzFile compressedFile;		// Gzipped file for decompression
extern uint32_t fileCursorLocation;	// Stores where we are in the file.
									// It's tracked manually to avoid expensive ftell calls when doing comparisons (for loops)
extern uint32_t dataCurrentSample;	// VGM sample we are on in the file
extern char commandID;				// Stores most recent VGM command read
extern uint8_t loopCount;			// Tracks what loop we are on during playback
extern uint8_t loopMax;				// How many times to loop
extern VgmChipType vgmChipType;		// What chip configuration has been determined from the VGM file (see types.h)

///////////////////////////////////////////////////////////////////////////////
// Struct declarations
///////////////////////////////////////////////////////////////////////////////

// VGM header struct
// Only uses the values we care about, not the entire header
typedef struct
{
	uint32_t fileIdentification;
	uint32_t eofOffset; 		// Relative offset from 0x04!
	uint32_t versionNumber;
	uint32_t gd3Offset;			// Relative offset from 0x14!
	uint32_t totalSamples;
	uint32_t loopOffset; 		// Relative offset from 0x1C!
	uint32_t loopNumSamples;	// Todo: Apply to loop count
	uint32_t recordingRate; 	// Could be used as a divider to cut down on the timing accuracy?
	uint32_t vgmDataOffset; 	// Relative offset from 0x34!
	uint32_t ym3812Clock; 		// Reminder: Dual chip is set by adding 0x40000000
	uint32_t ym3526Clock; 		// Should be able to convert dual OPL1 to dual OPL2
	uint32_t ymf262Clock; 		// Dual chip is like above, but there are no dual OPL3 cards on PC :)
	uint8_t loopBase;
	uint8_t loopModifier;
} vgmHeader;

// GD3 tag struct
typedef struct
{
	uint32_t tagLength;
	wchar_t* trackNameE;
	wchar_t* trackNameJ;
	wchar_t* gameNameE;
	wchar_t* gameNameJ;
	wchar_t* systemNameE;
	wchar_t* systemNameJ;
	wchar_t* originalAuthorE;
	wchar_t* originalAuthorJ;
	wchar_t* releaseDate;
	wchar_t* converter;
	wchar_t* notes;
} gd3Tag;

// Storage spot for split-out VGM header data
extern vgmHeader currentVGMHeader;

// Storage spot for split-out GD3 tag data
extern gd3Tag currentGD3Tag;

#endif
