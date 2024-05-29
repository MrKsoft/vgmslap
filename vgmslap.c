///////////////////////////////////////////////////////////////////////////////
// __      _______ __  __  _____ _             _ 
// \ \    / / ____|  \/  |/ ____| |           | |
//  \ \  / / |  __| \  / | (___ | | __ _ _ __ | |
//   \ \/ /| | |_ | |\/| |\___ \| |/ _` | '_ \| |   by Wafflenet, 2023-2024
//    \  / | |__| | |  | |____) | | (_| | |_) |_|       www.wafflenet.com
//     \/   \_____|_|  |_|_____/|_|\__,_| .__/(_)
//      (VGM Silly Little AdLib Player) | |      
//                                      |_|      
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Standard includes
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <bios.h>
#include <conio.h>
#include <dos.h>
#include <i86.h>
#include <libgen.h>
#include <string.h>
#include "zlib/zlib.h"

///////////////////////////////////////////////////////////////////////////////
// Other source file includes
///////////////////////////////////////////////////////////////////////////////

#include "txtgfx.c" // Text graphics & UI elements

///////////////////////////////////////////////////////////////////////////////
// Type definitions
///////////////////////////////////////////////////////////////////////////////

// Use these where possible to reduce bit/signing confusion
// Example, recall that "char" is default unsigned, but "int" is default signed

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

///////////////////////////////////////////////////////////////////////////////
// Defines and function prototypes
///////////////////////////////////////////////////////////////////////////////

#define VGMSLAP_VERSION "R2"

// Macro definitions

#define characterCoordinate(xPos, yPos) 2*((yPos*80)+xPos)

// Default setting definitions
#define CONFIG_DEFAULT_PORT 0x388
#define CONFIG_DEFAULT_LOOPS 1
#define CONFIG_DEFAULT_DIVIDER 1
#define CONFIG_DEFAULT_STRUGGLE 0

// Display functions
void clearTextScreen(void);
void clearInterface(void);
void setVideoMode(uint16_t);
void drawTextUI(void);
void drawCharacterAtPosition(char, uint8_t, uint8_t, uint8_t, uint8_t);
void drawStringAtPosition(char*, uint8_t, uint8_t, uint8_t, uint8_t);
void drawGraphicAtPosition(const char*, uint8_t, uint8_t, uint8_t, uint8_t);
void drawChannelTable(void);

// Playlist functions
void countPlaylistSongs(void);
void playlistGet(uint32_t songNumber);
void initPlayback(void);


// OPL control functions
void detectOPL(void);
void writeOPL(uint16_t, uint8_t);
void resetOPL(void);

// Timing functions
void interrupt timerHandler(void);
void initTimer(uint16_t);
void resetTimer (void);

// VGM parsing functions
uint8_t getNextCommandData(void);
wchar_t* getNextGd3String(void);
uint8_t loadVGM(void);
void populateCurrentGd3(void);
void processCommands(void);

// Other functions
void inputHandler(void);
void setConfig(void);
void killProgram(uint8_t);
uint8_t readBytes(uint16_t);

///////////////////////////////////////////////////////////////////////////////
// Global variable definitions
///////////////////////////////////////////////////////////////////////////////

// General program vars
uint8_t programState = 0;				// Controls current state of program (init, main loop, exit, etc)
										// 0 = init, 1 = play, 2 = songEnd, 3 = quit

// Input vars
uint8_t keyboardCurrent = 0;			// Current keycode from getch()
uint8_t keyboardPrevious = 0;			// Last processed keypress, to help identify a "new" vs "repeated" key
uint8_t keyboardExtendedFlag = 0;		// Flag for if we are reading an extended keycode


// Display-related vars
char txtDrawBuffer[80];							// Temporary buffer line for text display
char far *textScreen = (char far *)0xB8000000; 	// VGA text screen memory location
char numToHex[] = "0123456789ABCDEF"; 			// For quick num > hex conversion
char textRows = 25;								// Number of rows in text mode
uint16_t displayRegisterMax = 0xFF;				// How many registers to iterate for screen refresh

// File-related vars
char vgmIdentifier[] = "Vgm ";			// VGM magic number
char gzMagicNumber[2] = {0x1F, 0x8B}; 	// GZ magic number
char* fileName;							// Filename from argument
char* vgmFileName;						// Current VGM file name
char playlistLineBuffer[255];  			// Current line of playlist (if being used)
char vgmFileBuffer[256]; 				// Buffer of up to 256 bytes (which happens to be the max size of the VGM header...)
FILE *initialFilePointer;				// Pointer to initially passed file (playlist or VGM)
FILE *playlistFilePointer;				// Pointer to loaded playlist
FILE *vgmFilePointer; 					// Pointer to loaded VGM file
FILE *configFilePointer;				// Pointer to config file
gzFile compressedFile;					// Gzipped file for decompression
uint32_t fileCursorLocation = 0; 		// Stores where we are in the file.
										// It's tracked manually to avoid expensive ftell calls when doing comparisons (for loops)
uint8_t playlistMode = 0;				// Are we using playlist or not
uint16_t playlistLineNumber = 0;		// Tracks what line (song number) we are on in a playlist
uint16_t playlistMax = 0;				// How many lines in playlist

// OPL-related vars
uint16_t oplBaseAddr;				// Base port for OPL synth.
uint8_t detectedChip;				// What OPL chip we detect on the system
uint8_t oplDelayReg = 6;			// Delay required for OPL register write (set for OPL2 by default)
uint8_t oplDelayData = 35;			// Delay required for OPL data write (set for OPL2 by default)
char oplRegisterMap[0x1FF];			// Stores current state of OPL registers
char oplChangeMap[0x1FF];			// Written alongside oplRegisterMap, tracks bytes that need interpreted/drawn
uint8_t commandReg = 0; 			// Stores current OPL register to manipulate
uint8_t commandData = 0; 			// Stores current data to put in OPL register
uint8_t maxChannels = 9;			// When iterating channels, how many to go through (9 for OPL2, 18 for OPL3)
// Due to weird operator offsets to form a channel, this is a list of offsets from the base (0x20/0x40/0x60/0x80/0xE0) for each.  First half is OPL2 and second is OPL3, so OPL3 ones have 0x100 added to fit our data model.
// Original op order numbers: {0, 3, 1, 4, 2, 5, 6, 9, 7, 10, 8, 11, 12, 15, 13, 16, 14, 17, 18, 21, 19, 22, 20, 23, 24, 27, 25, 28, 26, 29, 30, 33, 31, 34, 32, 35};
const uint16_t oplOperatorOrder[] = { 			
	0x00, 0x03,    // Channel 1 (OPL2)
	0x01, 0x04,    // Channel 2
	0x02, 0x05,    // Channel 3
	0x08, 0x0B,    // Channel 4
	0x09, 0x0C,    // Channel 5
	0x0A, 0x0D,    // Channel 6
	0x10, 0x13,    // Channel 7
	0x11, 0x14,    // Channel 8
	0x12, 0x15,    // Channel 9
	0x100, 0x103,  // Channel 10 (OPL3)
	0x101, 0x104,  // Channel 11
	0x102, 0x105,  // Channel 12
	0x108, 0x10B,  // Channel 13
	0x109, 0x10C,  // Channel 14
	0x10A, 0x10D,  // Channel 15
	0x110, 0x113,  // Channel 16
	0x111, 0x114,  // Channel 17
	0x112, 0x115}; // Channel 18

// Offsets from base of a register category (for instance Attack/Decay at 0x60) and how it maps to a channel/op.
// OPL2 only, add 9 to channel number for OPL3
const uint8_t oplOperatorToChannel[] = {
0, 1, 2, 0, 1, 2, 0, 0, 3, 4, 5, 3, 4, 5, 0, 0, 6, 7, 8, 6, 7, 8
};
const uint8_t oplOffsetToOperator[] = {
0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1
};

// Timing-related vars
void interrupt (*biosISR8)(void);			// Pointer to the BIOS interrupt service routine 8
volatile uint32_t tickCounter = 0; 			// Counts the number of timer ticks elapsed
uint16_t biosCounter = 0;					// Used to determine when to run the original BIOS ISR8
uint32_t fastTickRate;						// Divider to apply to the 8253 PIT
uint8_t requestScreenDraw;					// Set to 1 when the screen needs to redraw
const uint16_t playbackFrequency = 44100;	// Playback frequency (VGM files are set to 44100 Hz)
uint8_t playbackFrequencyDivider = 1;		// Performance hack available for slower machines
uint32_t dataCurrentSample = 0;				// VGM sample we are on in the file

// VGM-related vars
char commandID = 0; 		// Stores most recent VGM command interpreted
uint8_t loopCount = 0; 		// Tracks what loop we are on during playback
uint8_t loopMax = 1;		// How many times to loop
uint8_t vgmChipType = 0; 	// What chip configuration has been determined from the VGM file
							// We have to deal with all permutations that a PC could theoretically play
							// 0 = No OPLs found
							// 1 = OPL1
							// 2 = OPL2
							// 3 = OPL3
							// 4 = 2xOPL1 (SBPro1/PAS only without in-flight modifications)
							// 5 = 2xOPL2 (SBPro1/PAS only without in-flight modifications)
							// 6 = OPL1+OPL2? (SBPro1/PAS only without in-flight modifications)idk, would someone do this??
							// 7 = Dual OPL3? (could you do this with two cards at different IO ports?  I've got no way of testing that!  YET!

///////////////////////////////////////////////////////////////////////////////
// Structs
///////////////////////////////////////////////////////////////////////////////

// Program settings struct
typedef struct
{
	char filePath[PATH_MAX];
	char tempPath[PATH_MAX];
	uint16_t oplBase;
	uint8_t loopCount;
	uint8_t frequencyDivider; // Range should be 1-100
	uint8_t struggleBus;
} programSettings;

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
	uint8_t panning; // Two bits - (0x03 = both channels, 0x02 = right, 0x01 = left, 0x00 = neither)
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

// Storage spot for program settings
programSettings settings;

// Storage spot for split-out VGM header data
vgmHeader currentVGMHeader;

// Storage spot for split-out GD3 tag data
gd3Tag currentGD3Tag;

// Storage spot for interpreted OPL status
oplChip oplStatus;

///////////////////////////////////////////////////////////////////////////////
// Main Functions
///////////////////////////////////////////////////////////////////////////////

// Main program loop
int main(int argc, char** argv)
{
	uint16_t i;
	// Check for arguments
		if (argc != 2)
		{
				
				killProgram(1);
		}
	
	// Populate filename from argument.
	fileName = argv[1];
	
	// Print program name and version
	
	printf("VGMSlap! %s by Wafflenet\n", VGMSLAP_VERSION);
	
	// Get path of settings file (we want to be sure that even if it's called from another directory, that the CFG file in the EXE folder is used)
	strncpy(settings.filePath, argv[0], sizeof(settings.filePath));
	for (i = sizeof(settings.filePath); i > 0; i--)
	{
		// Search string from end until the first \ is found, then insert the cfg file name after that.
		if (settings.filePath[i] == '\\')
		{
			settings.filePath[i+1] = '\0';
			strcat(settings.filePath, "VGMSLAP.CFG");
			break;
		}
	}
	
	// Populate default settings (will be overridden from config file, if present)
	settings.oplBase = CONFIG_DEFAULT_PORT;
	settings.frequencyDivider = CONFIG_DEFAULT_DIVIDER;
	settings.loopCount = CONFIG_DEFAULT_LOOPS;
	settings.struggleBus = CONFIG_DEFAULT_STRUGGLE;
	
	// Read settings from config file
	setConfig();
	
	oplBaseAddr = settings.oplBase;
	playbackFrequencyDivider = settings.frequencyDivider;
	loopMax = settings.loopCount;
	
	// Detect the OPL chip
	detectOPL();
	
	// Load initially requested file
	errno = 0;
	initialFilePointer = fopen(fileName,"rb");
	if (initialFilePointer == NULL)
	{
		killProgram(2);
	}
	// Did we load a playlist, or a single VGM?
	fgets(playlistLineBuffer,sizeof(playlistLineBuffer),initialFilePointer);
	// It's a playlist
	if (strncmp(playlistLineBuffer,"#VGMLIST", 8) == 0)
	{
		printf("Playlist detected!\n");
		playlistMode = 1;
		playlistLineNumber = 1;
	}
	
	// Close "initial file", we're done with it
	// At this point we've validated that it is in fact a file already
	if (initialFilePointer != NULL)
	{
		fclose(initialFilePointer);
	}
	
	// If playlist was found, open playlist, jump to playlist handler and load first song
	if (playlistMode == 1)
	{
		errno = 0;
		playlistFilePointer = fopen(fileName,"rt");
		if (playlistFilePointer == NULL)
		{
			
			killProgram(9);
		}
		
		countPlaylistSongs();
		playlistGet(playlistLineNumber);
	}

	// Playlist wasn't found, so probably a VGM, try interpreting as such.
	if (playlistMode == 0)
	{
		vgmFileName = fileName;	
	}
	
	// Start playback incl. VGM load and timer init
	initPlayback();

	while (programState > 0 && programState < 3)
	{
		// If programState > 0 then we are in the main logic loop.  Commands will be processed, input will be read, and the screen will be refreshed.
		if (programState == 1)
		{
			processCommands();
			
			// Press a key to quit
			// Todo: How to force the keyboard to respond if the CPU is overloaded due to playing a busy VGM on underspecced hardware?  Keyboard interrupt is getting missed.  Also keypress gets passed to next program (command, file manager, etc) after quit.  Detect release before acting?
			inputHandler();
			
			// Refresh screen
			if (settings.struggleBus == 0)
			{
				if (requestScreenDraw > 0)
				{
					drawChannelTable();
				}		
			}
		}
		// State 2 is "end of song"
		else if (programState == 2)
		{
			// Reset OPL, force screen redraw to set things back to default state
			resetOPL();
			if (settings.struggleBus == 0)
			{
				drawChannelTable();
			}
			
			// Free loaded file pointer
			fclose(vgmFilePointer);
			
			// Load new VGM and then restart playback
			if (playlistMode == 0)
			{
				programState = 3;
			}
			else if (playlistMode == 1)
			{
				if (playlistLineNumber < playlistMax)
				{
					playlistLineNumber++;
					// If the playlist ends up at 0 (likely due to overflow or going backwards through the playlist) it will try to read the header line as a filename.  Prevent this.
					if (playlistLineNumber == 0)
					{
						playlistLineNumber = 1;
					}
					// Get new song
					playlistGet(playlistLineNumber);
				
					// Repeat load/init/play routines
					initPlayback();
				}
				else
				{
					// Got to end of playlist.  Quit the program.
					programState = 3;
				}
			}
		}
	}
	while (programState == 3)
		{
			killProgram(0);
		}
	return 0;
}

void inputHandler (void)
{
	// Capture key
	if (kbhit())
	{
		keyboardCurrent = getch();
		// If extended key code (arrows for instance) get the extended code and flip the extended flag
		if (keyboardCurrent == 0x00)
		{
			keyboardExtendedFlag = 1;
			keyboardCurrent = getch();
		}
	}
	// No key is being pressed - reset key tracking
	else
	{
		
		keyboardPrevious = 0;
		keyboardCurrent = 0;
		keyboardExtendedFlag = 0;
	}
	// Process input if this is a new keypress
	if (keyboardPrevious != keyboardCurrent)
	{
		// Extended keycodes
		if (keyboardExtendedFlag == 1)
		{
			// Only do these if there's a playlist
			if (playlistMode == 1)
			{
				// Left Arrow
				if (keyboardCurrent == 0x4B)
				{
					// Go back 2 because the state handler already increments by 1
					if (playlistLineNumber < 2)
					{
						playlistLineNumber = 0;
					}
					else
					{
						playlistLineNumber = playlistLineNumber - 2;
					}
					programState = 2;
				}
			
				// Right Arrow
				if (keyboardCurrent == 0x4D)
				{
					// Treat like the track finished
					programState = 2;
					
				}
			}
		}

		// R - Resets OPL (panic button)
		if (keyboardCurrent == 0x52 || keyboardCurrent == 0x72)
		{
			resetOPL();
		}
		keyboardPrevious = keyboardCurrent;
		
		// Esc - Quit
		if (keyboardCurrent == 0x1B)
		{
			programState = 3;
		}
		keyboardPrevious = keyboardCurrent;
	}
}

// Prepare for and start playback state
void initPlayback(void)
{
	// Set timer back to normal - reduces loading/decompression performance if we are still processing interrupts
	if (fastTickRate != 0)
	{
		resetTimer();
		fastTickRate = 0;
	}
	
	// Prepare VGM file
	loadVGM();
					
	// Set up the OPL chip
	resetOPL();
	
	// Reset screen state
	if (settings.struggleBus == 0)
	{
		setVideoMode(50);
	}
	else
	{
		setVideoMode(25);
	}
	clearTextScreen();
	
	// Draw static parts of the UI
	clearInterface();
	drawTextUI();

	// Seek to start of first command
	fseek(vgmFilePointer,currentVGMHeader.vgmDataOffset+0x34,SEEK_SET);
	fileCursorLocation = currentVGMHeader.gd3Offset+0x34;

	// Wait just a little bit for things to settle (yay for weird stuttering)
	delay(100);
	
	// If DualOPL2 VGM, but playing on OPL3, enable OPL3 mode.
	// This allows use of additional channels and turns on panning so we can hear it
	// When the program supports multiple files in a session, this will be moved to pre-playback init
	// Btw, playback will sound right in Dosbox regardless of this, but breaks on real hardware or 86Box
	if (detectedChip == 3 && vgmChipType == 5)
	{
		writeOPL(0x105,0x01);
	}
	
	
	// Draw initial OPL state
	if (settings.struggleBus == 0)
	{
		drawChannelTable();
	}
	
	// Set interrupt timer if it hasn't already been done
	if (fastTickRate == 0)
	{
		initTimer(playbackFrequency);
	}
	
	// Reset time counter and start playback!!
	tickCounter = 0;
	programState = 1;
}

// Parse the config file and set settings
void setConfig(void)
{
	char keyName[9];
	char keyValueText[8];
	uint16_t keyValueHex;
	uint16_t keyValueDecimal;
	char configLineBuffer[80];	// Buffer of a config line.  I picked 80 chars cause... uh... DOS
	// Try to load the settings file
	configFilePointer = fopen(settings.filePath,"rt");
	if (configFilePointer == NULL)
	{
		// If there's no config file, just give up.  Defaults have already been set.
		return;
	}
	while (fgets(configLineBuffer,sizeof(configLineBuffer),configFilePointer) != NULL)
	{
		if (configLineBuffer[0] != ';')
		{
			// Populate the setting name found and its value
			sscanf(configLineBuffer, "%s %s\n", keyName, keyValueText);
			// Convert the value to both hex and decimal so we can use whichever is best suited
			keyValueHex = (uint16_t)strtol(keyValueText, NULL, 16);
			keyValueDecimal = (uint16_t)strtol(keyValueText, NULL, 10);
			// Based on what we actually read in, set the appropriate settings value
			// OPL port
			if (strcmp(keyName, "PORT") == 0)
			{
				settings.oplBase = keyValueHex;
			}
			// Number of loops
			if (strcmp(keyName, "LOOPS") == 0)
			{
				// Bounds check
				if (keyValueDecimal > 255)
				{
					keyValueDecimal = 255;
				}
				settings.loopCount = keyValueDecimal;
			}
			// Timing frequency divider
			if (strcmp(keyName, "DIVIDER") == 0)
			{
				// Bounds check
				if (keyValueDecimal < 1)
				{
					keyValueDecimal = 1;
				}
				if (keyValueDecimal > 255)
				{
					keyValueDecimal = 255;
				}
				settings.frequencyDivider = keyValueDecimal;
			}
			// Struggle bus mode (display off)
			if (strcmp(keyName, "STRUGGLE") == 0)
			{
				// Bounds check
				if (keyValueDecimal > 1)
				{
					keyValueDecimal = 1;
				}
				settings.struggleBus = keyValueDecimal;
			}
		}
	}
}

// Gets us a count for how many lines are in the playlist so we can show a number like (1/99) or something
void countPlaylistSongs(void)
{
	// Seek back to start of file to be sure we know where we are
	fseek(playlistFilePointer, 0, SEEK_SET);
	// Increment count by 1 for every line
	while (fgets(playlistLineBuffer,sizeof(playlistLineBuffer),playlistFilePointer) != NULL)
	{
		playlistMax++;
	}
	// Subtract 1 at the end, because the first line is a header and shouldn't count towards the total.
	playlistMax--;
}

void playlistGet(uint32_t songNumber)
{
	uint32_t i = 0;
	
	// Seek back to start of file to be sure we know where we are
	fseek(playlistFilePointer, 0, SEEK_SET);
	// Read lines until we reach the next song number
	while (fgets(playlistLineBuffer,sizeof(playlistLineBuffer),playlistFilePointer) != NULL)
	{
		if (i == songNumber)
		{	
			vgmFileName = strtok(playlistLineBuffer, "\n");
			break;
		}
		else
		{		
			i++;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// Timer Functions
///////////////////////////////////////////////////////////////////////////////

// Reprogram the PIT to run at our desired playback rate and insert our interrupt service routine 8 handler.
void initTimer(uint16_t frequency)
{
	// Divide the PIT's hz rate by the target hz rate
	fastTickRate = 1193182 / (frequency/playbackFrequencyDivider);
	// Store pointer to the original BIOS ISR8
	biosISR8 = _dos_getvect(8);
	// Hijack ISR8 to point to our handler routine instead
	_dos_setvect(8, timerHandler);
	// Configure and set the timer, which sits at 40h-43h.
	// Let's break down these magic-looking values.
	//
	// 43h = Command to set a channel's mode of operation
	// 36h = Command data:
	//       0 0 1 1 0 1 1 0
	//       └┬┘ └┬┘ └─┬─┘ └─► Process count as binary
	//        │   │    └─────► Mode 3: square-wave rate generator
	//        │   └──────────► Read/load LSB then MSB
	//        └──────────────► Select counter #0
	outp(0x43, 0x36);
	// 40h = Frequency divider for counter #0
	outp(0x40, fastTickRate & 0xFF); // Low byte
	outp(0x40, fastTickRate >> 8); // High byte
}

// Restore PIT to its original rate and remove our interrupt service routine 8 handler.
void resetTimer(void)
{
	// Configure and set the timer, which sits at 40h-43h.
	outp(0x43, 0x36);
	// 40h = Frequency divider for counter #0 - Setting to zero puts us back to the default rate
	outp(0x40, 0x00);
	outp(0x40, 0x00);
	// Point ISR8 back to the original BIOS routine
	_dos_setvect(8, biosISR8);
}

// Handler executes every PIT frequency cycle (ISR8)
void interrupt timerHandler(void)
{
	// Increment the counter cause the interrupt has happened
	tickCounter=tickCounter+playbackFrequencyDivider;
	
	// Since we have changed the timer rate, we have to determine when to call the BIOS ISR8 at its old rate.  This avoids the clock getting messed up.  While it may appear that biosCounter will only be less than fastTickRate one time, it will actually overflow back to 0 over time, making this a constant cycle.
	biosCounter += fastTickRate;
	if (biosCounter < fastTickRate)
	{
		biosISR8();
	}
	else
	{
		// If we aren't calling the BIOS ISR8 routine, we have to manually send an EOI (End of Interrupt) like it usually does.
		outp(0x20, 0x20);
	}
}

///////////////////////////////////////////////////////////////////////////////
// Display Functions
///////////////////////////////////////////////////////////////////////////////

// Reads OPL values, packs them into the structs, then draws the result to the screen
// Only interprets and draws parts that change
void drawChannelTable(void)
{
	uint16_t i;
	uint16_t j;
	uint16_t k;
	uint8_t targetChannel;
	uint8_t targetOperator;
	uint8_t tempAttribute = 0x0;
	
	// Start at 0x20 as that is the lowest register we care about
	for (i = 20; i <= displayRegisterMax; i++)
	{
		// Found a changed register
		if (oplChangeMap[i] == 1)
		{
			// What register changed?
			
			// Tremolo / Vibrato / Sustain / KSR Flags & Multiplier
			if ((i >= 0x20 && i <= 0x35) || (i >= 0x120 && i <= 0x135))
			{
				if (i >= 0x20 && i <= 0x35)
				{
					targetChannel = oplOperatorToChannel[(i - 0x20)];
					targetOperator = oplOffsetToOperator[(i - 0x20)];
				}
				else if (i >= 0x120 && i <= 0x135)
				{
					targetChannel = oplOperatorToChannel[(i - 0x120)] + 9;
					targetOperator = oplOffsetToOperator[(i - 0x120)];
				}
				
				// Multiplier
				oplStatus.channels[targetChannel].operators[targetOperator].frequencyMultiplierFactor = (oplRegisterMap[i] & 0x0F);
				
				// Tremolo
				oplStatus.channels[targetChannel].operators[targetOperator].flagTremolo = ((oplRegisterMap[i] >> 7) & 0x01);
				
				// Vibrato
				oplStatus.channels[targetChannel].operators[targetOperator].flagFrequencyVibrato = ((oplRegisterMap[i] >> 6) & 0x01);
				
				// Sustain
				oplStatus.channels[targetChannel].operators[targetOperator].flagSoundSustaining = ((oplRegisterMap[i] >> 5) & 0x01);
				
				// KSR
				oplStatus.channels[targetChannel].operators[targetOperator].flagKSR = ((oplRegisterMap[i] >> 4) & 0x01);
				
				if (targetOperator == 0)
				{
					// Multiplier
					drawStringAtPosition(oplMultiplierNames[oplStatus.channels[targetChannel].operators[targetOperator].frequencyMultiplierFactor], oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+15, oplStatus.channels[targetChannel].displayY+1,COLOR_LIGHTCYAN,COLOR_BLACK);
					
					// Tremolo
					if (oplStatus.channels[targetChannel].operators[targetOperator].flagTremolo == 1)
					{
						drawCharacterAtPosition('T', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+10, oplStatus.channels[targetChannel].displayY+1, COLOR_LIGHTGREY, COLOR_BLACK);
					}
					else
					{
						drawCharacterAtPosition('-', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+10, oplStatus.channels[targetChannel].displayY+1, COLOR_DARKGREY, COLOR_BLACK);
					}
					
					// Vibrato
					if (oplStatus.channels[targetChannel].operators[targetOperator].flagFrequencyVibrato == 1)
					{
						drawCharacterAtPosition('V', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+11, oplStatus.channels[targetChannel].displayY+1, COLOR_LIGHTGREY, COLOR_BLACK);
					}
					else
					{
						drawCharacterAtPosition('-', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+11, oplStatus.channels[targetChannel].displayY+1, COLOR_DARKGREY, COLOR_BLACK);
					}
					
					// Sustain
					if (oplStatus.channels[targetChannel].operators[targetOperator].flagSoundSustaining == 1)
					{
						drawCharacterAtPosition('S', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+12, oplStatus.channels[targetChannel].displayY+1, COLOR_LIGHTGREY, COLOR_BLACK);
					}
					else
					{
						drawCharacterAtPosition('-', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+12, oplStatus.channels[targetChannel].displayY+1, COLOR_DARKGREY, COLOR_BLACK);
					}
					
					// KSR
					if (oplStatus.channels[targetChannel].operators[targetOperator].flagKSR == 1)
					{
						drawCharacterAtPosition('K', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+13, oplStatus.channels[targetChannel].displayY+1, COLOR_LIGHTGREY, COLOR_BLACK);
					}
					else
					{
						drawCharacterAtPosition('-', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+13, oplStatus.channels[targetChannel].displayY+1, COLOR_DARKGREY, COLOR_BLACK);
					}
				}
				else if (targetOperator == 1)
				{
					// Multiplier
					drawStringAtPosition(oplMultiplierNames[oplStatus.channels[targetChannel].operators[targetOperator].frequencyMultiplierFactor],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+15,oplStatus.channels[targetChannel].displayY+3,COLOR_LIGHTGREEN, COLOR_BLACK);
					
					// Tremolo
					if (oplStatus.channels[targetChannel].operators[targetOperator].flagTremolo == 1)
					{
						drawCharacterAtPosition('T', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+10, oplStatus.channels[targetChannel].displayY+3, COLOR_LIGHTGREY, COLOR_BLACK);
					}
					else
					{
						drawCharacterAtPosition('-', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+10, oplStatus.channels[targetChannel].displayY+3, COLOR_DARKGREY, COLOR_BLACK);
					}
					
					// Vibrato
					if (oplStatus.channels[targetChannel].operators[targetOperator].flagFrequencyVibrato == 1)
					{
						drawCharacterAtPosition('V', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+11, oplStatus.channels[targetChannel].displayY+3, COLOR_LIGHTGREY, COLOR_BLACK);
					}
					else
					{
						drawCharacterAtPosition('-', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+11, oplStatus.channels[targetChannel].displayY+3, COLOR_DARKGREY, COLOR_BLACK);
					}
					
					// Sustain
					if (oplStatus.channels[targetChannel].operators[targetOperator].flagSoundSustaining == 1)
					{
						drawCharacterAtPosition('S', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+12, oplStatus.channels[targetChannel].displayY+3, COLOR_LIGHTGREY, COLOR_BLACK);
					}
					else
					{
						drawCharacterAtPosition('-', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+12, oplStatus.channels[targetChannel].displayY+3, COLOR_DARKGREY, COLOR_BLACK);
					}
					
					// KSR
					if (oplStatus.channels[targetChannel].operators[targetOperator].flagKSR == 1)
					{
						drawCharacterAtPosition('K', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+13, oplStatus.channels[targetChannel].displayY+3, COLOR_LIGHTGREY, COLOR_BLACK);
					}
					else
					{
						drawCharacterAtPosition('-', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+13, oplStatus.channels[targetChannel].displayY+3, COLOR_DARKGREY, COLOR_BLACK);
					}
				}
			}
			
			// KSL / Output Level
			else if ((i >= 0x40 && i <= 0x55) || (i >= 0x140 && i <= 0x155))
			{
				if (i >= 0x40 && i <= 0x55)
				{
					targetChannel = oplOperatorToChannel[(i - 0x40)];
					targetOperator = oplOffsetToOperator[(i - 0x40)];
				}
				else if (i >= 0x140 && i <= 0x155)
				{
					targetChannel = oplOperatorToChannel[(i - 0x140)] + 9;
					targetOperator = oplOffsetToOperator[(i - 0x140)];
				}
				
				// Key Scaling Level
				oplStatus.channels[targetChannel].operators[targetOperator].keyScaleLevel = (oplRegisterMap[i] >> 6);
			
				// Output level
				oplStatus.channels[targetChannel].operators[targetOperator].outputLevel = (oplRegisterMap[i] & 0x3F);
				
				if (targetOperator == 0)
				{
					// Output level
					sprintf(txtDrawBuffer, "%.2X", oplStatus.channels[targetChannel].operators[targetOperator].outputLevel);
					drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+21,oplStatus.channels[targetChannel].displayY+1,COLOR_LIGHTCYAN, COLOR_BLACK);
					
					// Key Scaling Level
					drawStringAtPosition(oplKSLNames[oplStatus.channels[targetChannel].operators[targetOperator].keyScaleLevel],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+17,oplStatus.channels[targetChannel].displayY+1,COLOR_LIGHTCYAN, COLOR_BLACK);
					
				}
				else if (targetOperator == 1)
				{
					// Output level
					sprintf(txtDrawBuffer, "%.2X", oplStatus.channels[targetChannel].operators[targetOperator].outputLevel);
					drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+21,oplStatus.channels[targetChannel].displayY+3,COLOR_LIGHTGREEN, COLOR_BLACK);
					
					// Key Scaling Level
					drawStringAtPosition(oplKSLNames[oplStatus.channels[targetChannel].operators[targetOperator].keyScaleLevel],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+17,oplStatus.channels[targetChannel].displayY+3,COLOR_LIGHTGREEN, COLOR_BLACK);
				}
			}
			
			// Attack/Decay
			else if ((i >= 0x60 && i <= 0x75) || (i >= 0x160 && i <= 0x175))
			{
				if (i >= 0x60 && i <= 0x75)
				{
					targetChannel = oplOperatorToChannel[(i - 0x60)];
					targetOperator = oplOffsetToOperator[(i - 0x60)];
				}
				else if (i >= 0x160 && i <= 0x175)
				{
					targetChannel = oplOperatorToChannel[(i - 0x160)] + 9;
					targetOperator = oplOffsetToOperator[(i - 0x160)];
				}
				
				oplStatus.channels[targetChannel].operators[targetOperator].attackRate = (oplRegisterMap[i] >> 4);
				oplStatus.channels[targetChannel].operators[targetOperator].decayRate = (oplRegisterMap[i] & 0x0F);
				
				if (targetOperator == 0)
				{
					drawCharacterAtPosition(numToHex[oplStatus.channels[targetChannel].operators[targetOperator].attackRate],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+5,oplStatus.channels[targetChannel].displayY+1, COLOR_LIGHTCYAN, COLOR_BLACK);
					drawCharacterAtPosition(numToHex[oplStatus.channels[targetChannel].operators[targetOperator].decayRate],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+6,oplStatus.channels[targetChannel].displayY+1,COLOR_LIGHTCYAN, COLOR_BLACK);
				}
				else if (targetOperator == 1)
				{
					drawCharacterAtPosition(numToHex[oplStatus.channels[targetChannel].operators[targetOperator].attackRate],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+5,oplStatus.channels[targetChannel].displayY+3, COLOR_LIGHTGREEN, COLOR_BLACK);
					drawCharacterAtPosition(numToHex[oplStatus.channels[targetChannel].operators[targetOperator].decayRate],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+6,oplStatus.channels[targetChannel].displayY+3, COLOR_LIGHTGREEN, COLOR_BLACK);
				}
			}
			
			// Sustain/Release
			else if ((i >= 0x80 && i <= 0x95) || (i >= 0x180 && i <= 0x195))
			{
				if (i >= 0x80 && i <= 0x95)
				{
					targetChannel = oplOperatorToChannel[(i - 0x80)];
					targetOperator = oplOffsetToOperator[(i - 0x80)];
				}
				else if (i >= 0x180 && i <= 0x195)
				{
					targetChannel = oplOperatorToChannel[(i - 0x180)] + 9;
					targetOperator = oplOffsetToOperator[(i - 0x180)];
				}
				
				oplStatus.channels[targetChannel].operators[targetOperator].sustainLevel = (oplRegisterMap[i] >> 4);
				oplStatus.channels[targetChannel].operators[targetOperator].releaseRate = (oplRegisterMap[i] & 0x0F);
				
				if (targetOperator == 0)
				{
					drawCharacterAtPosition(numToHex[oplStatus.channels[targetChannel].operators[targetOperator].sustainLevel],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+7,oplStatus.channels[targetChannel].displayY+1, COLOR_LIGHTCYAN, COLOR_BLACK);
					drawCharacterAtPosition(numToHex[oplStatus.channels[targetChannel].operators[targetOperator].releaseRate],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+8,oplStatus.channels[targetChannel].displayY+1, COLOR_LIGHTCYAN, COLOR_BLACK);
				}
				else if (targetOperator == 1)
				{
					drawCharacterAtPosition(numToHex[oplStatus.channels[targetChannel].operators[targetOperator].sustainLevel],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+7,oplStatus.channels[targetChannel].displayY+3, COLOR_LIGHTGREEN, COLOR_BLACK);
					drawCharacterAtPosition(numToHex[oplStatus.channels[targetChannel].operators[targetOperator].releaseRate],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+8,oplStatus.channels[targetChannel].displayY+3, COLOR_LIGHTGREEN, COLOR_BLACK);
				}
			}
			
			// Frequency (Low)
			
			else if ((i >= 0xA0 && i <= 0xA8) || (i >= 0x1A0 && i <= 0x1A8))
			{
				if (i >= 0xA0 && i <= 0xA8)
				{
					targetChannel = i - 0xA0;
				}
				else if (i >= 0x1A0 && i <= 0x1A8)
				{
					targetChannel = (i - 0x1A0) + 9;
				}
				
				oplStatus.channels[targetChannel].frequencyNumber = (oplRegisterMap[i]) + ((oplRegisterMap[i+0x10] & 0x03) << 8);
				
				// Entire frequency must be redrawn because it's split between bytes
				if (targetChannel <= 5 || (targetChannel >= 9 && targetChannel <= 14))
				{
					if (oplStatus.channels[targetChannel].flag4Op == 1)
					{
						if (targetChannel <= 2 || (targetChannel >= 9 && targetChannel <= 11))
						{
							// Frequency number
							sprintf(txtDrawBuffer, "%.3X", oplStatus.channels[targetChannel].frequencyNumber);
							drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+4,COLOR_YELLOW,COLOR_BLACK);
						}
					}
					else
					{
						// Frequency number
						sprintf(txtDrawBuffer, "%.3X", oplStatus.channels[targetChannel].frequencyNumber);
						drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+2,COLOR_YELLOW,COLOR_BLACK);
					}
				}
				// Positioning for 2-op channels
				else
				{
					// Frequency number
					sprintf(txtDrawBuffer, "%.3X", oplStatus.channels[targetChannel].frequencyNumber);
					drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+2,COLOR_YELLOW,COLOR_BLACK);
				}
			}
			
			// Key-On, Block, Frequency (High)
			
			else if ((i >= 0xB0 && i <= 0xB8) || (i >= 0x1B0 && i <= 0x1B8))
			{
				if (i >= 0xB0 && i <= 0xB8)
				{
					targetChannel = i - 0xB0;
				}
				else if (i >= 0x1B0 && i <= 0x1B8)
				{
					targetChannel = (i - 0x1B0) + 9;
				}
				
				// Frequency (High)
				oplStatus.channels[targetChannel].frequencyNumber = (oplRegisterMap[i-0x10]) + ((oplRegisterMap[i] & 0x03) << 8);
				
				// Block number
				oplStatus.channels[targetChannel].blockNumber = ((oplRegisterMap[i] >> 2) & 0x07);
				
				// Key on
				oplStatus.channels[targetChannel].keyOn = ((oplRegisterMap[i] >> 5) & 0x01);
				// Set what color to draw the Note icon with based on whether Key-On was set.
				if (oplStatus.channels[targetChannel].keyOn == 1)
				{
					tempAttribute = COLOR_LIGHTMAGENTA;
				}
				else
				{
					tempAttribute = COLOR_DARKGREY;
				}
				
				// Draw
				// Entire frequency must be redrawn because it's split between bytes
				
				if (targetChannel <= 5 || (targetChannel >= 9 && targetChannel <= 14))
				{
					if (oplStatus.channels[targetChannel].flag4Op == 1)
					{
						if (targetChannel <= 2 || (targetChannel >= 9 && targetChannel <= 11))
						{
							// Block number
							sprintf(txtDrawBuffer, " %X ", oplStatus.channels[targetChannel].blockNumber);
							drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+3, COLOR_BROWN, COLOR_BLACK);
							// Frequency number
							sprintf(txtDrawBuffer, "%.3X", oplStatus.channels[targetChannel].frequencyNumber);
							drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+4, COLOR_YELLOW, COLOR_BLACK);
							// Key on
							drawCharacterAtPosition(CHAR_MUSIC_NOTE, oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO+1, oplStatus.channels[targetChannel].displayY+5, tempAttribute, COLOR_BLACK);
						}
					}
					else
					{
						// Block number
						sprintf(txtDrawBuffer, " %X ", oplStatus.channels[targetChannel].blockNumber);
						drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+1, COLOR_BROWN, COLOR_BLACK);
						// Frequency number
						sprintf(txtDrawBuffer, "%.3X", oplStatus.channels[targetChannel].frequencyNumber);
						drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+2, COLOR_YELLOW, COLOR_BLACK);
						// Key on
						drawCharacterAtPosition(CHAR_MUSIC_NOTE, oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO+1, oplStatus.channels[targetChannel].displayY+3, tempAttribute, COLOR_BLACK);
					}
				}
				// Positioning for 2-op channels
				else
				{
					// Block number
					sprintf(txtDrawBuffer, " %X ", oplStatus.channels[targetChannel].blockNumber);
					drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+1, COLOR_BROWN, COLOR_BLACK);
					// Frequency number
					sprintf(txtDrawBuffer, "%.3X", oplStatus.channels[targetChannel].frequencyNumber);
					drawStringAtPosition(txtDrawBuffer,oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+2, COLOR_YELLOW, COLOR_BLACK);
					// Key on
					drawCharacterAtPosition(CHAR_MUSIC_NOTE, oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO+1, oplStatus.channels[targetChannel].displayY+3, tempAttribute, COLOR_BLACK);
				}
			}
			
			// Panning, Feedback, Algorithm
			else if ((i >= 0xC0 && i <= 0xC8) || (i >= 0x1C0 && i <= 0x1C8))
			{
				if (i >= 0xC0 && i <= 0xC8)
				{
					targetChannel = i - 0xC0;
				}
				else if (i >= 0x1C0 && i <= 0x1C8)
				{
					targetChannel = (i - 0x1C0) + 9;
				}
				
				// Panning
				oplStatus.channels[targetChannel].panning = ((oplRegisterMap[i] >> 4) & 0x03);
			
				// Algorithm type
				oplStatus.channels[targetChannel].synthesisType = ((oplRegisterMap[i]) & 0x01);
			
				// Feedback
				oplStatus.channels[targetChannel].feedback = ((oplRegisterMap[i] >> 1) & 0x07);
				
				// Draw Panning (only if the OPL3 is turned on, or playing DualOPL2)
				if (oplStatus.flagOPL3Mode != 0 || vgmChipType == 5)
				{
					if (oplStatus.channels[targetChannel].flag4Op == 0)
					{
						// Left
						if ((oplStatus.channels[targetChannel].panning & 0x01) == 1)
						{
							drawCharacterAtPosition('(', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO, (oplStatus.channels[targetChannel].displayY+3), COLOR_WHITE, COLOR_BLACK);		
						}
						else
						{
							drawCharacterAtPosition('(', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO, (oplStatus.channels[targetChannel].displayY+3), COLOR_DARKGREY, COLOR_BLACK);
						}
						// Right
						if ((oplStatus.channels[targetChannel].panning & 0x02) == 2)
						{
							drawCharacterAtPosition(')', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO+2, (oplStatus.channels[targetChannel].displayY+3), COLOR_WHITE, COLOR_BLACK);		
						}
						else
						{
							drawCharacterAtPosition(')', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO+2, (oplStatus.channels[targetChannel].displayY+3), COLOR_DARKGREY, COLOR_BLACK);
						}
					}
					else if (oplStatus.channels[targetChannel].flag4Op != 0)
					{
						// Positioning for 4-op channels
						if (targetChannel <= 2 || (targetChannel >= 9 && targetChannel <= 11))
						{
							// Left
							if ((oplStatus.channels[targetChannel].panning & 0x01) == 1)
							{
								drawCharacterAtPosition('(', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO, (oplStatus.channels[targetChannel].displayY+5), COLOR_WHITE, COLOR_BLACK);
							}
							else
							{
								drawCharacterAtPosition('(', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO, (oplStatus.channels[targetChannel].displayY+5), COLOR_DARKGREY, COLOR_BLACK);
							}
							// Right
							if ((oplStatus.channels[targetChannel].panning & 0x02) == 2)
							{
								drawCharacterAtPosition(')', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO+2, (oplStatus.channels[targetChannel].displayY+5), COLOR_WHITE, COLOR_BLACK);						
							}
							else
							{
								drawCharacterAtPosition(')', oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO+2, (oplStatus.channels[targetChannel].displayY+5), COLOR_DARKGREY, COLOR_BLACK);
							}
						}
					}
				}
				
				// Draw Algorithm type
				if (oplStatus.channels[targetChannel].flag4Op == 0)
				{
					// FM Algorithm
					if(oplStatus.channels[targetChannel].synthesisType == 0)
					{
						drawGraphicAtPosition(tgAlgoFM, 7, 3, oplStatus.channels[targetChannel].displayX+1, oplStatus.channels[targetChannel].displayY+1);
					}
					// AS Algorithm
					else if (oplStatus.channels[targetChannel].synthesisType == 1)
					{
						drawGraphicAtPosition(tgAlgoAS, 7, 3, oplStatus.channels[targetChannel].displayX+1, oplStatus.channels[targetChannel].displayY+1);
					}
				}
				else if (oplStatus.channels[targetChannel].flag4Op != 0)
				{
					// Only do this if we are on the first channel of the 4-op pairing
					if (targetChannel <= 2 || (targetChannel >= 9 && targetChannel <= 11))
					{
						// Clean up previously written text from switching between 2/4op
						drawStringAtPosition("   ",oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+24,oplStatus.channels[targetChannel].displayY+2, COLOR_BLACK, COLOR_BLACK);
						drawStringAtPosition("   ",oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+24,oplStatus.channels[targetChannel].displayY+6, COLOR_BLACK, COLOR_BLACK);
						drawStringAtPosition("   ",oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+1, COLOR_BLACK, COLOR_BLACK);
						drawStringAtPosition("   ",oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+2, COLOR_BLACK, COLOR_BLACK);
						drawStringAtPosition("   ",oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+6, COLOR_BLACK, COLOR_BLACK);
						drawStringAtPosition("   ",oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_CHANNEL_NOTEINFO,oplStatus.channels[targetChannel].displayY+7, COLOR_BLACK, COLOR_BLACK);
						
						// FM+FM
						if(oplStatus.channels[targetChannel].synthesisType == 0 && oplStatus.channels[targetChannel+3].synthesisType == 0 )
						{
							drawGraphicAtPosition(tgAlgoFMFM, 7, 7, oplStatus.channels[targetChannel+3].displayX+1, oplStatus.channels[targetChannel].displayY+1);
						}
						// AS+FM
						else if(oplStatus.channels[targetChannel].synthesisType == 1 && oplStatus.channels[targetChannel+3].synthesisType == 0 )
						{
							drawGraphicAtPosition(tgAlgoASFM, 7, 7, oplStatus.channels[targetChannel].displayX+1, oplStatus.channels[targetChannel].displayY+1);
						}
						// FM+AS
						else if(oplStatus.channels[targetChannel].synthesisType == 0 && oplStatus.channels[targetChannel+3].synthesisType == 1 )
						{
							drawGraphicAtPosition(tgAlgoFMAS, 7, 7, oplStatus.channels[targetChannel].displayX+1, oplStatus.channels[targetChannel].displayY+1);
						}
						// AS+AS
						else if(oplStatus.channels[targetChannel].synthesisType == 1 && oplStatus.channels[targetChannel+3].synthesisType == 1 )
						{
							drawGraphicAtPosition(tgAlgoASAS, 7, 7, oplStatus.channels[targetChannel].displayX+1, oplStatus.channels[targetChannel].displayY+1);
						}
					}
				}
				
				// Draw Feedback
				if (oplStatus.channels[targetChannel].flag4Op == 0)
				{
					drawStringAtPosition(oplFeedbackNames[oplStatus.channels[targetChannel].feedback],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+24,oplStatus.channels[targetChannel].displayY+2, COLOR_YELLOW, COLOR_BLACK);	
					
				}
				else if (oplStatus.channels[targetChannel].flag4Op != 0)
				{
					if (targetChannel <= 2 || (targetChannel >= 9 && targetChannel <= 11))
					{
					drawStringAtPosition(oplFeedbackNames[oplStatus.channels[targetChannel].feedback],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS+24,oplStatus.channels[targetChannel].displayY+4, COLOR_YELLOW, COLOR_BLACK);
					}
				}
			}
			
			// Waveform Select
			else if ((i >= 0xE0 && i <= 0xF5) || (i >= 0x1E0 && i <= 0x1F5))
			{
				if (i >= 0xE0 && i <= 0xF5)
				{
					targetChannel = oplOperatorToChannel[(i - 0xE0)];
					targetOperator = oplOffsetToOperator[(i - 0xE0)];
				}
				else if (i >= 0x1E0 && i <= 0x1F5)
				{
					targetChannel = oplOperatorToChannel[(i - 0x1E0)] + 9;
					targetOperator = oplOffsetToOperator[(i - 0x1E0)];
				}
				
				// If OPL2, ignore the highest bit as those waveforms can't be used
				if (oplStatus.flagOPL3Mode == 0)
				{
					oplStatus.channels[targetChannel].operators[targetOperator].waveform = (oplRegisterMap[i] & 0x03);
				}
				else
				{
					oplStatus.channels[targetChannel].operators[targetOperator].waveform = (oplRegisterMap[i] & 0x07);
				}
				
				if (targetOperator == 0)
				{
					drawStringAtPosition(oplWaveformNames[oplStatus.channels[targetChannel].operators[targetOperator].waveform],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS,oplStatus.channels[targetChannel].displayY+1, COLOR_LIGHTCYAN, COLOR_BLACK);
				}
				else if (targetOperator == 1)
				{
					drawStringAtPosition(oplWaveformNames[oplStatus.channels[targetChannel].operators[targetOperator].waveform],oplStatus.channels[targetChannel].displayX+CHAN_DISP_OFFSET_OPERATOR_PARAMETERS,oplStatus.channels[targetChannel].displayY+3, COLOR_LIGHTGREEN, COLOR_BLACK);
				}
			}
			
			// OPL3 4-op flags
			else if (i == 0x104)
			{
				oplStatus.channels[0].flag4Op = oplRegisterMap[0x104] & 0x01;
				oplStatus.channels[3].flag4Op = oplRegisterMap[0x104] & 0x01;
				oplStatus.channels[1].flag4Op = oplRegisterMap[0x104] & 0x02 >> 1;
				oplStatus.channels[4].flag4Op = oplRegisterMap[0x104] & 0x02 >> 1;
				oplStatus.channels[2].flag4Op = oplRegisterMap[0x104] & 0x04 >> 2;
				oplStatus.channels[5].flag4Op = oplRegisterMap[0x104] & 0x04 >> 2;
				oplStatus.channels[9].flag4Op = oplRegisterMap[0x104] & 0x08 >> 3;
				oplStatus.channels[12].flag4Op = oplRegisterMap[0x104] & 0x08 >> 3;
				oplStatus.channels[10].flag4Op = oplRegisterMap[0x104] & 0x10 >> 4;
				oplStatus.channels[13].flag4Op = oplRegisterMap[0x104] & 0x10 >> 4;
				oplStatus.channels[11].flag4Op = oplRegisterMap[0x104] & 0x20 >> 5;
				oplStatus.channels[14].flag4Op = oplRegisterMap[0x104] & 0x20 >> 5;
				
				// After changing 4-op flags, force redraw of things that may need repositioning or changed
				// (Only on the first channel of each 4-op pair)
				
				// 0xAx Frequency (Low)
				// 0xBx Key-On / Block / Frequency (High)
				// 0xCx Algorithm / Feedback / Panning
				for (j = 0x00; j <= 0x02; j++)
				{
					oplChangeMap[0xA0+j] = 1;
					oplChangeMap[0xB0+j] = 1;
					oplChangeMap[0xC0+j] = 1;
				}
				for (j = 0x00; j <= 0x02; j++)
				{
					oplChangeMap[0x1A0+j] = 1;
					oplChangeMap[0x1B0+j] = 1;
					oplChangeMap[0x1C0+j] = 1;
				}
				
				// Channel names
				for (j=0; j < maxChannels; j++)
				{
					// Rename 4-op channels
					if (j <=2 || (j >= 9 && j <= 11))
					{
						
						if (oplStatus.channels[j].flag4Op == 1)
						{
							drawStringAtPosition("4OP", oplStatus.channels[j].displayX+36, oplStatus.channels[j].displayY, COLOR_LIGHTRED, COLOR_BLACK);
							if (j == 0)
							{
								drawStringAtPosition("01+04", oplStatus.channels[0].displayX+2, oplStatus.channels[0].displayY, COLOR_WHITE, COLOR_BLACK);
							}
							else if (j == 1)
							{
								drawStringAtPosition("02+05", oplStatus.channels[1].displayX+2, oplStatus.channels[1].displayY, COLOR_WHITE, COLOR_BLACK);
							}
							else if (j == 2)
							{
								drawStringAtPosition("03+06", oplStatus.channels[2].displayX+2, oplStatus.channels[2].displayY, COLOR_WHITE, COLOR_BLACK);
							}
							else if (j == 9)
							{
								drawStringAtPosition("10+13", oplStatus.channels[9].displayX+2, oplStatus.channels[9].displayY, COLOR_WHITE, COLOR_BLACK);
							}
							else if (j == 10)
							{
								drawStringAtPosition("11+14", oplStatus.channels[10].displayX+2, oplStatus.channels[10].displayY, COLOR_WHITE, COLOR_BLACK);
							}
							else if (j == 11)
							{
								drawStringAtPosition("12+15", oplStatus.channels[11].displayX+2, oplStatus.channels[11].displayY, COLOR_WHITE, COLOR_BLACK);
							}
						}
					}
					// Channel has reverted to 2-op - need to redraw the paired channels too
					if ( j <=5 || (j >= 9 && j <= 14))
					{
						if (oplStatus.channels[j].flag4Op == 0)
						{
							
							sprintf(txtDrawBuffer, "Ch.%02d", j+1);
							drawStringAtPosition(txtDrawBuffer, oplStatus.channels[j].displayX+2, oplStatus.channels[j].displayY, COLOR_WHITE, COLOR_BLACK);
							drawStringAtPosition("\xCD\xCD\xCD", oplStatus.channels[j].displayX+36, oplStatus.channels[j].displayY, COLOR_DARKGREY, COLOR_BLACK);
						}					
					}
				}
				// Redraw lines 'n stuff
				for (j=0; j < maxChannels; j++)
				{
					
					// Channel headers (for channels that can switch between 2op and 4op)
					if (j < 3 || (j > 5 && j < 12) || j > 14)
					{
						for (k=1; k<36; k++)
						{
							// Skip the channel number location
							if ( k<2 || k>6 )
							{	// Draw horizontal line
								drawCharacterAtPosition(CHAR_BOX_DOUBLE_HORIZONTAL, oplStatus.channels[j].displayX+k, oplStatus.channels[j].displayY, COLOR_DARKGREY, COLOR_BLACK);
							}
						}
						// Draw corner pieces
						drawCharacterAtPosition(CHAR_BOX_DOWN_SINGLE_RIGHT_DOUBLE, oplStatus.channels[j].displayX, oplStatus.channels[j].displayY, COLOR_DARKGREY, COLOR_BLACK);
						drawCharacterAtPosition(CHAR_BOX_DOWN_SINGLE_LEFT_DOUBLE, oplStatus.channels[j].displayX+39, oplStatus.channels[j].displayY, COLOR_DARKGREY, COLOR_BLACK);
						// Clear extra characters that may have been left behind in 2-op mode
						if (oplStatus.channels[j].flag4Op == 1)
						{
							drawCharacterAtPosition(CHAR_BOX_SINGLE_VERTICAL, oplStatus.channels[j].displayX, oplStatus.channels[j].displayY+4, COLOR_DARKGREY, COLOR_BLACK);
							drawCharacterAtPosition(CHAR_BOX_SINGLE_VERTICAL, oplStatus.channels[j].displayX+39, oplStatus.channels[j].displayY+4, COLOR_DARKGREY, COLOR_BLACK);
							for (k=8; k<33; k++)
							{
								drawCharacterAtPosition(' ', oplStatus.channels[j].displayX+k, oplStatus.channels[j].displayY+4, COLOR_BLACK, COLOR_BLACK);
							}
							drawCharacterAtPosition(' ', oplStatus.channels[j].displayX+35, oplStatus.channels[j].displayY+4, COLOR_BLACK, COLOR_BLACK);
						}
						
					}
					// Channel headers (2op channels)
					else
					{
						if (oplStatus.channels[j].flag4Op == 0)
						{
							for (k=1; k<36; k++)
							{
								// Skip the channel number location
								if ( k<2 || k>6 )
								{
									// Draw horizontal line
									drawCharacterAtPosition(CHAR_BOX_DOUBLE_HORIZONTAL, oplStatus.channels[j].displayX+k, oplStatus.channels[j].displayY, COLOR_DARKGREY, COLOR_BLACK);
								}
							}
							// Draw corner pieces
							drawCharacterAtPosition(CHAR_BOX_DOWN_SINGLE_RIGHT_DOUBLE, oplStatus.channels[j].displayX, oplStatus.channels[j].displayY, COLOR_DARKGREY, COLOR_BLACK);
							drawCharacterAtPosition(CHAR_BOX_DOWN_SINGLE_LEFT_DOUBLE, oplStatus.channels[j].displayX+39, oplStatus.channels[j].displayY, COLOR_DARKGREY, COLOR_BLACK);
						}
					}
				}
			}
			
			// OPL3 "new bit"
			else if (i == 0x105)
			{
				oplStatus.flagOPL3Mode = (oplRegisterMap[0x105] & 0x01);
				
				// Force redraw panning
				for (j = 0x00; j <= 0x08; j++)
				{
					oplChangeMap[0xC0+j] = 1;
				}
				for (j = 0x00; j <= 0x08; j++)
				{
					oplChangeMap[0x1C0+j] = 1;
				}
			}
		
			// End of processing, this byte has been handled
			oplChangeMap[i] = 0;
		}	
	}
	// Draw operation is done
	requestScreenDraw = 0;
}

// Draws the static UI components
void drawTextUI(void)
{
	uint8_t i;
	uint8_t j;
	
	// Blue bar at top
	for (i=0; i<55; i++)
	{
		drawStringAtPosition(" ",i,0,COLOR_WHITE,COLOR_BLUE);
	}
	sprintf(txtDrawBuffer, "VGMSlap! %s", VGMSLAP_VERSION);
	drawStringAtPosition(txtDrawBuffer,0,0,COLOR_WHITE,COLOR_BLUE);
	if (playlistMode == 0)
	{
		drawStringAtPosition("Now playing:             ",55,0,COLOR_WHITE,COLOR_BLUE);
		drawStringAtPosition(basename(fileName),68,0,COLOR_YELLOW,COLOR_BLUE);
	}
	else if (playlistMode == 1)
	{
		drawStringAtPosition("Playlist:               ",55,0,COLOR_WHITE,COLOR_BLUE);
		sprintf(txtDrawBuffer, "[%5u / %5u]", playlistLineNumber, playlistMax);
		drawStringAtPosition(txtDrawBuffer,65,0,COLOR_YELLOW,COLOR_BLUE);
	}
	
	// GD3 tag info
	drawStringAtPosition("Title:  ",GD3_LABEL_START_X,GD3_START_Y,COLOR_LIGHTGREY,COLOR_BLACK);
	sprintf(txtDrawBuffer, "%S", currentGD3Tag.trackNameE);
	drawStringAtPosition(txtDrawBuffer,GD3_TAG_START_X,GD3_START_Y,COLOR_LIGHTCYAN,COLOR_BLACK);
	
	drawStringAtPosition("Artist: ",GD3_LABEL_START_X,GD3_START_Y+1,COLOR_LIGHTGREY,COLOR_BLACK);
	sprintf(txtDrawBuffer, "%S", currentGD3Tag.originalAuthorE);
	drawStringAtPosition(txtDrawBuffer,GD3_TAG_START_X,GD3_START_Y+1,COLOR_LIGHTCYAN,COLOR_BLACK);
	
	drawStringAtPosition("Game:   ",GD3_LABEL_START_X,GD3_START_Y+2,COLOR_LIGHTGREY,COLOR_BLACK);
	sprintf(txtDrawBuffer, "%S", currentGD3Tag.gameNameE);
	drawStringAtPosition(txtDrawBuffer,GD3_TAG_START_X,GD3_START_Y+2,COLOR_LIGHTCYAN,COLOR_BLACK);
	
	drawStringAtPosition("Date:   ",GD3_LABEL_START_X,GD3_START_Y+3,COLOR_LIGHTGREY,COLOR_BLACK);
	sprintf(txtDrawBuffer, "%S", currentGD3Tag.releaseDate);
	drawStringAtPosition(txtDrawBuffer,GD3_TAG_START_X,GD3_START_Y+3,COLOR_LIGHTCYAN,COLOR_BLACK);
	
	// Set positions for the channels (remember, they're out of order due to OPL3 pairing)
	for (i=0; i<9; i++)
	{
	oplStatus.channels[i].displayX = CHAN_TABLE_START_X;
	}
	if (maxChannels > 9)
	{
		for (i=9; i<18; i++)
		{
		oplStatus.channels[i].displayX = CHAN_TABLE_START_X+40;
		}
	}
	
	oplStatus.channels[0].displayY = CHAN_TABLE_START_Y;
	oplStatus.channels[3].displayY = CHAN_TABLE_START_Y+4;
	oplStatus.channels[1].displayY = CHAN_TABLE_START_Y+8;
	oplStatus.channels[4].displayY = CHAN_TABLE_START_Y+12;
	oplStatus.channels[2].displayY = CHAN_TABLE_START_Y+16;
	oplStatus.channels[5].displayY = CHAN_TABLE_START_Y+20;
	oplStatus.channels[6].displayY = CHAN_TABLE_START_Y+24;
	oplStatus.channels[7].displayY = CHAN_TABLE_START_Y+28;
	oplStatus.channels[8].displayY = CHAN_TABLE_START_Y+32;
	
	oplStatus.channels[9].displayY = CHAN_TABLE_START_Y;
	oplStatus.channels[12].displayY = CHAN_TABLE_START_Y+4;
	oplStatus.channels[10].displayY = CHAN_TABLE_START_Y+8;
	oplStatus.channels[13].displayY = CHAN_TABLE_START_Y+12;
	oplStatus.channels[11].displayY = CHAN_TABLE_START_Y+16;
	oplStatus.channels[14].displayY = CHAN_TABLE_START_Y+20;
	oplStatus.channels[15].displayY = CHAN_TABLE_START_Y+24;
	oplStatus.channels[16].displayY = CHAN_TABLE_START_Y+28;
	oplStatus.channels[17].displayY = CHAN_TABLE_START_Y+32;
	
	if (settings.struggleBus == 0)
	{
		
		for (i=0; i<maxChannels; i++)
		{
			// Initial channel numbers
			sprintf(txtDrawBuffer, "Ch.%02d", i+1);
			drawStringAtPosition(txtDrawBuffer, oplStatus.channels[i].displayX+2, oplStatus.channels[i].displayY, COLOR_WHITE, COLOR_BLACK);
			drawStringAtPosition("\xCD\xCD\xCD", oplStatus.channels[i].displayX+36, oplStatus.channels[i].displayY, COLOR_DARKGREY, COLOR_BLACK);
			
			// Initial horizontal lines
			// Will be redrawn if we change to 4op mode
			for (j=1; j<36; j++)
			{
				// Skip the channel number location
				if ( j<2 || j>6 )
				{	// Draw horizontal line
					drawCharacterAtPosition(CHAR_BOX_DOUBLE_HORIZONTAL, oplStatus.channels[i].displayX+j, oplStatus.channels[i].displayY, COLOR_DARKGREY, COLOR_BLACK);
				}
			}
			// Draw corner pieces
			drawCharacterAtPosition(CHAR_BOX_DOWN_SINGLE_RIGHT_DOUBLE, oplStatus.channels[i].displayX, oplStatus.channels[i].displayY, COLOR_DARKGREY, COLOR_BLACK);
			drawCharacterAtPosition(CHAR_BOX_DOWN_SINGLE_LEFT_DOUBLE, oplStatus.channels[i].displayX+39, oplStatus.channels[i].displayY, COLOR_DARKGREY, COLOR_BLACK);
		}
		
		// Some vertical lines (these never change)
		for (i=0; i<maxChannels; i++)
		{
			drawCharacterAtPosition(CHAR_BOX_SINGLE_VERTICAL, oplStatus.channels[i].displayX, oplStatus.channels[i].displayY+1, COLOR_DARKGREY, COLOR_BLACK);
			drawCharacterAtPosition(CHAR_BOX_SINGLE_VERTICAL, oplStatus.channels[i].displayX, oplStatus.channels[i].displayY+2, COLOR_DARKGREY, COLOR_BLACK);
			drawCharacterAtPosition(CHAR_BOX_SINGLE_VERTICAL, oplStatus.channels[i].displayX, oplStatus.channels[i].displayY+3, COLOR_DARKGREY, COLOR_BLACK);
			drawCharacterAtPosition(CHAR_BOX_SINGLE_VERTICAL, oplStatus.channels[i].displayX+39, oplStatus.channels[i].displayY+1, COLOR_DARKGREY, COLOR_BLACK);
			drawCharacterAtPosition(CHAR_BOX_SINGLE_VERTICAL, oplStatus.channels[i].displayX+39, oplStatus.channels[i].displayY+2, COLOR_DARKGREY, COLOR_BLACK);
			drawCharacterAtPosition(CHAR_BOX_SINGLE_VERTICAL, oplStatus.channels[i].displayX+39, oplStatus.channels[i].displayY+3, COLOR_DARKGREY, COLOR_BLACK);
		}
		
		// Bottom of channel display (never changes)
		
		drawCharacterAtPosition(CHAR_BOX_UP_SINGLE_RIGHT_SINGLE, oplStatus.channels[8].displayX, oplStatus.channels[8].displayY+4, COLOR_DARKGREY, COLOR_BLACK);
		drawCharacterAtPosition(CHAR_BOX_UP_SINGLE_LEFT_SINGLE, oplStatus.channels[8].displayX+39, oplStatus.channels[8].displayY+4, COLOR_DARKGREY, COLOR_BLACK);
		for (i=1; i<39; i++)
		{
			drawCharacterAtPosition(CHAR_BOX_SINGLE_HORIZONTAL, oplStatus.channels[8].displayX+i, oplStatus.channels[8].displayY+4, COLOR_DARKGREY, COLOR_BLACK);
		}
		if (maxChannels > 9)
		{
			drawCharacterAtPosition(CHAR_BOX_UP_SINGLE_RIGHT_SINGLE, oplStatus.channels[17].displayX, oplStatus.channels[17].displayY+4, COLOR_DARKGREY, COLOR_BLACK);
			drawCharacterAtPosition(CHAR_BOX_UP_SINGLE_LEFT_SINGLE, oplStatus.channels[17].displayX+39, oplStatus.channels[17].displayY+4, COLOR_DARKGREY, COLOR_BLACK);
			for (i=1; i<39; i++)
			{
				drawCharacterAtPosition(CHAR_BOX_SINGLE_HORIZONTAL, oplStatus.channels[17].displayX+i, oplStatus.channels[8].displayY+4, COLOR_DARKGREY, COLOR_BLACK);
			}
		}
	}
}

// Set video mode
void setVideoMode(uint16_t mode)
{
	// Using the following numbers to represent what mode to switch to:
	// 25 = 80x25 text mode
	// 50 = 80x50 text mode
	union REGS registers;
	if (mode == 25)
	{
		// Function 00h - Set video mode
		registers.h.ah = 0x00;
		// Mode 03h - Text mode
		registers.h.al = 0x03;
		// Need to zero these out to reset things
		registers.w.bx = 0x0000;
		// Send the parameters to Int 10h
		int86(0x10, &registers, &registers);
		textRows = 25;
	}
	if (mode == 50)
	{
		// Function 00h - Set video mode
		registers.h.ah = 0x00;
		// Mode 03h - Text mode
		registers.h.al = 0x03;
		// Send the parameters to Int 10h
		int86(0x10, &registers, &registers);
		// Function 11h - Change text mode character set
		registers.h.ah = 0x11;
		// Load 8x8 font
		registers.h.al = 0x12;
		// Need to zero these out to reset things
		registers.w.bx = 0x0000;
		// Call Int 10h again
		int86(0x10, &registers, &registers);
		// We don't want the blinking cursor to display in 50-column mode
		outp(0x3D4, 0x0A);
		outp(0x3D5, 0x20);
		textRows = 50;
	}
	
}

// Put a character directly into memory at a given coordinate
void drawCharacterAtPosition(char text, uint8_t xPos, uint8_t yPos, uint8_t foregroundColor, uint8_t backgroundColor)
{
	// Combine foreground and background attribute
	uint8_t attribute = foregroundColor | (backgroundColor << 4);
	// Generate the correct memory location to put the text using our predefined coordinate function
	uint16_t screenAddress = characterCoordinate(xPos, yPos);
	textScreen[screenAddress++] = text;
	textScreen[screenAddress++] = attribute;
}

// Put a full string directly into memory at a given coordinate
void drawStringAtPosition(char* text, uint8_t xPos, uint8_t yPos, uint8_t foregroundColor, uint8_t backgroundColor)
{
	
	// Combine foreground and background attribute
	uint8_t attribute = foregroundColor | (backgroundColor << 4);
	// Generate the correct memory location to put the text using our predefined coordinate function
	uint16_t screenAddress = characterCoordinate(xPos, yPos);
	while (*text)
	{
		textScreen[screenAddress++] = *text;
		textScreen[screenAddress++] = attribute;
		text++;
	}
}

// Put an array based "graphic" into memory at a given coordinate
// Pass the name of the graphic array, dimensions, and then where you want it to start (top left)
void drawGraphicAtPosition(const char* graphicArray, uint8_t xSize, uint8_t ySize, uint8_t xOrigin, uint8_t yOrigin)
{
	uint8_t xCount;
	uint8_t yCount;
	uint16_t maxChars = xSize*ySize;
	uint16_t screenAddress;
	
	for (yCount = 0; yCount < ySize; yCount++)
	{
		for (xCount = 0; xCount < xSize; xCount++)
		{
				// Generate the correct memory location to put the text using our predefined coordinate function
				screenAddress = characterCoordinate((xOrigin+xCount), (yOrigin+yCount));
				// Throw the data from the graphic array into memory at that location (character then attribute)
				textScreen[screenAddress++] = graphicArray[(xCount+(yCount*xSize))];
				textScreen[screenAddress++] = graphicArray[((xCount+(yCount*xSize))+maxChars)];
		}
	}
}

// Blanks out the text screen
void clearTextScreen(void)
{
	// Writes a blank character to every location on screen.  TextRows is set when screen mode changes.
	uint8_t column = 0;
	uint8_t row = 0;
	for (row = 0; row < textRows; row++)
	{
		for (column = 0; column < 80; column++)
		{
		drawCharacterAtPosition(' ',column,row,COLOR_LIGHTGREY,COLOR_BLACK);
		}
	}
}

// Clear text screen, but only certain parts of the UI
void clearInterface(void)
{
	uint8_t column;
	uint8_t row;
	
	// Clear GD3 tag area
	for (row = GD3_START_Y; row < GD3_START_Y+4; row++)
	{
		for (column = GD3_TAG_START_X; column < 80; column++)
		{
		drawCharacterAtPosition(' ',column,row,COLOR_LIGHTGREY,COLOR_BLACK);
		}
	}
	
	
	// Clear channel table (only right side, cause of leftover DualOPL2/OPL3 stuff)
	
	for (row = CHAN_TABLE_START_Y; row < textRows; row++)
	{
		for (column = (CHAN_TABLE_START_X+40); column < 80; column++)
		{
		drawCharacterAtPosition(' ',column,row,COLOR_LIGHTGREY,COLOR_BLACK);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// OPL Control Functions
///////////////////////////////////////////////////////////////////////////////

// Send data to the OPL chip
void writeOPL(uint16_t reg, uint8_t data)
{
		// Setup delay count
		uint8_t registerDelay = oplDelayReg;
		uint8_t dataDelay = oplDelayData;
		
		// Second OPL2 and/or OPL3 secondary register set
		if (reg >= 0x100)
		{
			// First write to target register... 
			outp(oplBaseAddr+2, (reg - 0x100));
			// Index register write delay.
			// The OPL2 requires a minimum time between writes.  We can execute inp a certain number of times to ensure enough time has passed - why does that work?  Because the time it takes to complete an inp is based on the ISA bus timings!
			while (registerDelay--)
			{
				inp(0x80);
			}
			
			// ...then go to +1 for the data
			outp(oplBaseAddr+3, data);
			// Data register write delay.
			while (dataDelay--)
			{
				inp(0x80);
			}
		}
		// OPL2 and/or OPL3 primary register set
		else
		{
			// First write to target register... 
			outp(oplBaseAddr, reg);
			// Index register write delay.
			// The OPL2 requires a minimum time between writes.  We can execute inp a certain number of times to ensure enough time has passed - why does that work?  Because the time it takes to complete an inp is based on the ISA bus timings!
			while (registerDelay--)
			{
				inp(0x80);
			}
			
			// ...then go to Base+1 for the data
			outp(oplBaseAddr+1, data);
			// Data register write delay.
			while (dataDelay--)
			{
				inp(0x80);
			}
		}
		
		// Write the same data to our "register map", used for visualizing the OPL state, as well as the change map to denote that this bit needs to be interpreted and potentially drawn.
		oplRegisterMap[reg] = data;
		oplChangeMap[reg] = 1;
		
		// Request a screen draw for the display update
		requestScreenDraw = 1;
}

// Reset OPL to original state, including turning off OPL3 mode
void resetOPL(void)
{
		// Resetting the OPL has to be somewhat systematic - otherwise you run into issues with static sounds, squeaking, etc, not only when cutting off the sound but also when the sound starts back up again.
		
		uint16_t i;
		
		// For OPL3, turn on the NEW bit.  This ensures we can write to ALL registers on an OPL3.
		if (detectedChip == 3)
		{
			writeOPL(0x105,0x01);
		}
		
		// Clear out the channel level information
		
		// OPL2
		for (i=0; i<9; i++)
		{
			writeOPL(0xA0+i, 0x00); // Frequency number (LSB)
			writeOPL(0xB0+i, 0x00); // Key-On + Block + Frequency (MSB)
			writeOPL(0xC0+i, 0x30); // Panning, Feedback, Synthesis Type
		}
		// Dual OPL2 / OPL3
		if (detectedChip > 1)
		{
			for (i=0; i<9; i++)
			{
				writeOPL(0x1A0+i, 0x00); // Frequency number (LSB)
				writeOPL(0x1B0+i, 0x00); // Key-On + Block + Frequency (MSB)
				writeOPL(0x1C0+i, 0x30); // Panning, Feedback, Synthesis Type
			}
		}
		
		// Clear out the operators one by one.
		// Why do it this way instead of just writing 0's in order?
		// That would mean that we go through each operator property, by operator, in order, putting a gap between the changes for an operator that can manifest as an audio glitch or "click"
		// Doing it this way we don't get that clicking when we kill the audio.
		
		// OPL2
		for (i=0; i<18; i++)
		{
			writeOPL(0x20+oplOperatorOrder[i], 0x00);
			writeOPL(0x40+oplOperatorOrder[i], 0x3F); // Output attenuation is set to max
			writeOPL(0x60+oplOperatorOrder[i], 0xFF);
			writeOPL(0x80+oplOperatorOrder[i], 0xFF);
			writeOPL(0xE0+oplOperatorOrder[i], 0x00);
		}
		// Dual OPL2 / OPL3
		if (detectedChip > 1)
		{
			for (i=18; i<36; i++)
			{
				writeOPL(0x20+oplOperatorOrder[i], 0x00);
				writeOPL(0x40+oplOperatorOrder[i], 0x3F); // Output attenuation is set to max
				writeOPL(0x60+oplOperatorOrder[i], 0xFF);
				writeOPL(0x80+oplOperatorOrder[i], 0xFF);
				writeOPL(0xE0+oplOperatorOrder[i], 0x00);
			}
		}
		
		// Clear out percussion mode register
		writeOPL(0xBD,0x00);
		if (detectedChip > 1)
		{
			writeOPL(0x1BD,0x00);
		}
		
		// Return to the ADSR and set them to zero - we set them to F earlier to force a note decay
		// We also set the output attenuation AGAIN, this time to 0 (no attenuation) as some poorly coded playback engines just expect the OPL to already have 0 there
		// OPL2
		for (i=0; i<18; i++)
		{
			writeOPL(0x60+oplOperatorOrder[i], 0x00);
			writeOPL(0x80+oplOperatorOrder[i], 0x00);
			writeOPL(0x40+oplOperatorOrder[i], 0x00);
		}
		// Dual OPL2 / OPL3
		if (detectedChip > 1)
		{
			for (i=18; i<36; i++)
			{
				writeOPL(0x60+oplOperatorOrder[i], 0x00);
				writeOPL(0x80+oplOperatorOrder[i], 0x00);
				writeOPL(0x40+oplOperatorOrder[i], 0x00);
			}
		}
		
		// Finally clear the initial registers
		// OPL2 regs
		for (i = 0x00; i < 0x20; i++)
		{
			writeOPL(i,0x00);
		}
		
		// If Dual OPL2 just clear it like an OPL2
		if (detectedChip == 2)
		{
			for (i = 0x100; i < 0x120; i++)
			{
				writeOPL(i,0x00);
			}
		}
		
		// If OPL3, change plans...
		if (detectedChip == 3)
		{
			// OPL3 regs - works a bit differently.  We don't turn off 4-op mode until we have zeroed everything else out, and we must touch 0x105 (OPL3 enable / "NEW" bit) ABSOLUTELY LAST or our writes to OPL3 features will be completely ignored!  (Yes, that includes zeroing them out!!)
			for (i = 0x100; i <= 0x103; i++)
			{
				writeOPL(i,0x00);
			}
			for (i = 0x106; i < 0x120; i++)
			{
				writeOPL(i,0x00);
			}
			// For OPL3, turn off 4-Op mode (if it was on)
			writeOPL(0x104,0x00);
			// For OPL3, write the NEW bit back to 0.  We're now back in OPL2 mode.
			// VGMs should have their own write to this bit to re-enable it for OPL3 songs.
			writeOPL(0x105,0x00);
		}
}

// Detect what OPL chip is in the computer.  This determines what VGMs can be played.
void detectOPL(void)
{
	uint8_t statusRegisterResult1;
	uint8_t statusRegisterResult2;
	
	// Start with assuming nothing is detected
	detectedChip = 0;
	
	// Detect OPL2
	
	// Reset timer 1 and timer 2
	writeOPL(0x04, 0x60);
	// Reset IRQ
	writeOPL(0x04, 0x80);
	// Read status register
	statusRegisterResult1 = inp(oplBaseAddr);
	// Set timer 1
	writeOPL(0x02, 0xFF);
	// Unmask and start timer 1
	writeOPL(0x04, 0x21);
	// Wait at least 80 usec (0.08ms) - a 2ms delay should be enough
	delay(2);
	// Read status register
	statusRegisterResult2 = inp(oplBaseAddr);
	// Reset timer 1, timer 2, and IRQ again
	writeOPL(0x04, 0x60);
	writeOPL(0x04, 0x80);
	// Compare results of the status register reads.  First should be 0x00, second should be 0xC0.  Results are AND with 0xE0 because of unused bits in the chip
	if ((statusRegisterResult1 & 0xE0) == 0x00 && (statusRegisterResult2 & 0xE0) == 0xC0)
	{
		// OPL2 detected
		detectedChip = 1;
		// Continue to try detecting OPL3
		if ((statusRegisterResult2 & 0x06) == 0x00)
		{
			detectedChip = 3;
		}
		// If no OPL3, try detecting second OPL2 at OPL3 registers.
		// Dual OPL2 usually is at 220h/222h (left/right) and writes to 228 will go to both OPLs
		// Note that this detection works in Dosbox, but not in 86Box with Nuked.  It does work in 86box with YMFM.  Need to test on a real SB Pro 1 which I don't have.
		else
		{
			writeOPL(0x104, 0x60);
			writeOPL(0x104, 0x80);
			statusRegisterResult1 = inp(oplBaseAddr+2);
			writeOPL(0x102, 0xFF);
			writeOPL(0x104, 0x21);
			delay(2);
			statusRegisterResult2 = inp(oplBaseAddr+2);
			writeOPL(0x104, 0x60);
			writeOPL(0x104, 0x80);
			if ((statusRegisterResult1 & 0xE0) == 0x00 && (statusRegisterResult2 & 0xE0) == 0xC0)
			{
				detectedChip = 2;
			}
		}
	}
	// Set delays, print results
	switch (detectedChip)
	{
		case 0:
			killProgram(7);
			break;
		case 1:
			oplDelayReg = 6;
			oplDelayData = 35;
			printf("OPL2 detected at %Xh!\n", settings.oplBase);
			break;
		case 2:
			oplDelayReg = 6;
			oplDelayData = 35;
			printf("Dual OPL2 detected at %Xh!\n", settings.oplBase);
			break;
		case 3:
			oplDelayReg = 3;
			oplDelayData = 3;
			printf("OPL3 detected at %Xh!\n", settings.oplBase);
			break;
		
	}
	sleep(1);
}

///////////////////////////////////////////////////////////////////////////////
// VGM Parsing Functions
///////////////////////////////////////////////////////////////////////////////

// Read from the specified VGM file and person some validity checks
uint8_t loadVGM(void)
{
	char gzipBuffer[512];
	uint16_t i;

	// Try to load the VGM
	errno = 0;
	vgmFilePointer = fopen(vgmFileName,"rb");
	if (vgmFilePointer == NULL)
	{
		killProgram(10);
	}
	
	// Ensure we are at the beginning of the file.
	fseek(vgmFilePointer, 0, SEEK_SET);
	fileCursorLocation = 0;
	dataCurrentSample = 0;
	
	// Read the first 4 bytes so we can see the identifier.
	// We want to check if this is a VGZ before continuing, and decompress first if so.
	
	readBytes(4);
	currentVGMHeader.fileIdentification = *((uint32_t *)&vgmFileBuffer[0x00]);
	if (memcmp((char *)&currentVGMHeader.fileIdentification, gzMagicNumber, 2) == 0)
	{	
		// Decompress file to a temporary directory
		printf("Compressed file detected - decompressing...\n");
		// Reopen the file with zlib
		fclose(vgmFilePointer);
		errno = 0;
		compressedFile = gzopen(vgmFileName,"rb");
		if (compressedFile == NULL)
		{
			killProgram(11);
		}
		// Setup a temporary file to decompress to, because there's not enough RAM in real mode :)
		// (We borrow the path from the config file location since argv isn't here)
		strncpy(settings.tempPath, settings.filePath, sizeof(settings.tempPath));
		for (i = sizeof(settings.tempPath); i > 0; i--)
		{
			// Search string from end until the first \ is found, then insert the temp file name after that.
			if (settings.tempPath[i] == '\\')
			{
				settings.tempPath[i+1] = '\0';
				strcat(settings.tempPath, "VGMSLAP.TMP");
				break;
			}
		}
		vgmFilePointer = fopen(settings.tempPath,"wb");
	
		// Decompress bytes and throw them into the temp file
		// This is done in 512b batches for performance, which will mean that the end of the temp file may have some extra data.  This is not a problem because 1) it's a temp file and 2) the VGM header already specifies where the EOF is and the parsing code knows what to do when it runs out of data.
		// But yes, I'm aware it's dumb and lazy.
		while(gzfread(gzipBuffer, sizeof(char), sizeof(gzipBuffer), compressedFile))
		{
			fwrite(gzipBuffer, sizeof(char), sizeof(gzipBuffer), vgmFilePointer);
		}
		// No more data - close original gzipped file
		gzclose_r(compressedFile);
		
		// Close temp file so we can reopen it in read-only mode
		// We are aggressive with the flushing/syncing because if writing the buffer to disk takes too long, the playback will start, and stutter slightly.
		fflush(vgmFilePointer);
		fsync(fileno(vgmFilePointer));
		fclose(vgmFilePointer);
		errno = 0;
		vgmFilePointer = fopen(settings.tempPath,"rb");
		if (vgmFilePointer == NULL)
		{
			killProgram(12);
		}
	}
	
	// We should have a file now, so seek back to the beginning.
	fseek(vgmFilePointer, 0, SEEK_SET);
	// Read enough bytes to get the VGM header, then populate header struct
	readBytes(256);
	currentVGMHeader.fileIdentification = *((uint32_t *)&vgmFileBuffer[0x00]);
	currentVGMHeader.eofOffset = *((uint32_t *)&vgmFileBuffer[0x04]);
	currentVGMHeader.versionNumber = *((uint32_t *)&vgmFileBuffer[0x08]);
	currentVGMHeader.gd3Offset = *((uint32_t *)&vgmFileBuffer[0x14]);
	currentVGMHeader.totalSamples = *((uint32_t *)&vgmFileBuffer[0x18]);
	currentVGMHeader.loopOffset = *((uint32_t *)&vgmFileBuffer[0x1C]);
	currentVGMHeader.loopNumSamples = *((uint32_t *)&vgmFileBuffer[0x20]);
	currentVGMHeader.recordingRate = *((uint32_t *)&vgmFileBuffer[0x24]);
	currentVGMHeader.vgmDataOffset = *((uint32_t *)&vgmFileBuffer[0x34]);
	currentVGMHeader.ym3812Clock = *((uint32_t *)&vgmFileBuffer[0x50]);
	currentVGMHeader.ym3526Clock = *((uint32_t *)&vgmFileBuffer[0x54]);
	currentVGMHeader.ymf262Clock = *((uint32_t *)&vgmFileBuffer[0x5C]);
	currentVGMHeader.loopBase = *((uint8_t *)&vgmFileBuffer[0x7E]); 
	currentVGMHeader.loopModifier = *((uint8_t *)&vgmFileBuffer[0x7F]);
	
	// Check if it's actually a VGM after all that
	if (memcmp((char *)&currentVGMHeader.fileIdentification, vgmIdentifier, 4) != 0)
	{	
		killProgram(3);
	}
	
	// If VGM version is < 1.51 just bail out cause they don't support OPL anyway
	if (currentVGMHeader.versionNumber < 0x151)
	{
		killProgram(4);
	}
	
	// Check for OPL2
	if (currentVGMHeader.ym3812Clock > 0)
	{
		// OK, it's OPL2.  Is it dual OPL2?
		if (currentVGMHeader.ym3812Clock > 0x40000000)
		{
			vgmChipType = 5;
			maxChannels = 18;
		}
		// No, it's single OPL2.
		else
		{
			vgmChipType = 2;
			maxChannels = 9;
		}
	}
	
	// Check for OPL1
	if (currentVGMHeader.ym3526Clock > 0)
	{
		// OK, it's OPL1.  Is it dual OPL1?
		if (currentVGMHeader.ym3526Clock > 0x40000000)
		{
			vgmChipType = 4;
			maxChannels = 18;
		}
		// No, it's single OPL1.
		else
		{
			// Was a single OPL2 already found?  This could be OPL1+OPL2.  A weird combination, but theoretically valid.
			if (vgmChipType == 2)
			{
				vgmChipType = 6;
				maxChannels = 18;
			}
			// Nah, it's just an OPL1 on its own.
			else
			{
				vgmChipType = 1;
				maxChannels = 9;
			}
		}
	}
	
	// Check for OPL3
	if (currentVGMHeader.ymf262Clock > 0)
	{
		// OK, it's OPL3.  Is it dual OPL3?
		if (currentVGMHeader.ymf262Clock > 0x40000000)
		{
			// That's a problem (for now).  Let's back out.  Might look into supporting it later, but I can't test it.
			// Not even taking into account how the hell to display all that info on screen.
			vgmChipType = 0;
		}
		// No, it's single OPL3.
		else
		{
			vgmChipType = 3;
			maxChannels = 18;
		}
	}
	
	// If the detected chip type is still 0, then there's nothing we can play for this anyway.
	if (vgmChipType == 0)
	{
		killProgram(5);
	}
	
	// Chip check was ok.  Now compare vs detected OPL chip to see if it's playable, and if not, kill the program.  We also setup the base IO due to Dual OPL2 shenanigans
	switch (vgmChipType)
	{
		// OPL1
		case 1:
			if (detectedChip == 1 || detectedChip == 3)
			{
				oplBaseAddr = settings.oplBase;
				break;
			}
			// If Dual OPL2 was detected we secretly shift the base IO to base+8 so that single OPL2 goes to both stereo channels
			if (detectedChip == 2)
			{
				oplBaseAddr = settings.oplBase+8;
				break;
			}
			else
			{
				killProgram(5);
				break;
			}
		// OPL2
		case 2:
			if (detectedChip == 1 || detectedChip == 3)
			{
				oplBaseAddr = settings.oplBase;
				break;
			}
			// If Dual OPL2 was detected we secretly shift the base IO to base+8 so that single OPL2 goes to both stereo channels
			if (detectedChip == 2)
			{
				oplBaseAddr = settings.oplBase+8;
				break;
			}
			else
			{
				killProgram(5);
				break;
			}
		// OPL3
		case 3:
			if (detectedChip == 3)
			{
				oplBaseAddr = settings.oplBase;
				break;
			}
			else
			{
				killProgram(5);
				break;
			}
		// Dual OPL1
		case 4:
			if (detectedChip >= 2)
			{
				oplBaseAddr = settings.oplBase;
				break;
			}
			else
			{
				killProgram(5);
				break;
			}
		// Dual OPL2
		case 5:
			if (detectedChip >= 2)
			{
				oplBaseAddr = settings.oplBase;
				break;
			}
			else
			{
				killProgram(5);
				break;
			}
		// OPL1 + OPL2 (not ready yet...)
		case 6:
			if (detectedChip >= 2)
			{
				oplBaseAddr = settings.oplBase;
				killProgram(5); // Remove this when feature is ready
				break;
			}
			else
			{
				killProgram(5);
				break;
			}
	}
	
	// Register limit for display based on max channels we set
	if (maxChannels > 9)
	{
		displayRegisterMax = 0x1FF;
	}
	else
	{
		displayRegisterMax = 0xFF;
	}
	
	// Everything else is okay, I say it's time to load the GD3 tag!
	populateCurrentGd3();
	
	// Success!
	return 0;
}

// Move through the file based on the commands encountered.  Loads in data for supported commands, to be processed during playback.
uint8_t getNextCommandData(void)
{
	uint32_t currentWait = 0;
	// Proper functioning of this function is based on the file already being seeked to the VGM data offset
	
	// Read the first byte to see what command we are looking at.
	// The byte length can be different depending on the command, so we have to decide how many bytes to read in.
	if (readBytes(1) == 0)
	{
		commandID = vgmFileBuffer[0];
	}
	// Read failed - probably ran out of bytes.  End song.
	else
	{
		programState = 2;
	}
	
	
	// Mega size switch case for all known VGM commands.  Most will be ignored, though, since they don't apply to OPL.
	// We still need to handle the right number of bytes though, especially for multichip VGMs that happen to have OPL in them too.
	// The good news is that all OPL commands are two bytes (reg/data) so we can return a command easily.
	// There are ways to shrink this down considerably (grouping all x-byte commands), but for now I'm being explicit and can optimize later.
	switch (commandID)
	{
		
		// SN76489 (2nd chip) write value (skip 1 byte)
		case 0x30:
			readBytes(1);
			break;
			
		// AY8910 stereo mask (skip 1 byte)
		case 0x31:
			readBytes(1);
			break;
			
		// Reserved/unused single byte commands (skip 1 byte)
		case 0x32:
		case 0x33:
		case 0x34:
		case 0x35:
		case 0x36:
		case 0x37:
		case 0x38:
		case 0x39:
		case 0x3A:
		case 0x3B:
		case 0x3C:
		case 0x3D:
		case 0x3E:
			readBytes(1);
			break;
			
			
		// Game Gear Stereo write value (skip 1 byte)
		case 0x3F:
			readBytes(1);
			break;

		// Reserved/unused two byte commands (skip 2 bytes)
		// Skip 1 byte if VGM version is < 1.60
		case 0x40:
		case 0x41:
		case 0x42:
		case 0x43:
		case 0x44:
		case 0x45:
		case 0x46:
		case 0x47:
		case 0x48:
		case 0x49:
		case 0x4A:
		case 0x4B:
		case 0x4C:
		case 0x4D:
		case 0x4E:
			if (currentVGMHeader.versionNumber < 0x160)
			{
				readBytes(1);
			}
			else
			{
				readBytes(2);
			}
			break;
			
		// Game Gear PSG stereo (skip 1 byte)
		case 0x4F:
			readBytes(1);
			break;
			
		// SN76489 write value (skip 1 byte)
		case 0x50:
			readBytes(1);
			break;
		
		// YM2413 write value to register (skip 2 bytes)
		case 0x51:
			readBytes(2);
			break;
		
		// YM2612 port 0 write value to register (skip 2 bytes)
		case 0x52:
			readBytes(2);
			break;
		
		// YM2612 port 1 write value to register (skip 2 bytes)
		case 0x53:
			readBytes(2);
			break;	
			
		// YM2151 write value to register (skip 2 bytes)
		case 0x54:
			readBytes(2);
			break;	
		
		// YM2203 write value to register (skip 2 bytes)
		case 0x55:
			readBytes(2);
			break;
		
		// YM2608 port 0 write value to register (skip 2 bytes)
		case 0x56:
			readBytes(2);
			break;
		
		// YM2608 port 1 write value to register (skip 2 bytes)
		case 0x57:
			readBytes(2);
			break;

		// YM2610 port 0 write value to register (skip 2 bytes)
		case 0x58:
			readBytes(2);
			break;	
			
		// YM2610 port 1 write value to register (skip 2 bytes)
		case 0x59:
			readBytes(2);
			break;
			
		// YM3812 write value to register (first chip)
		case 0x5A:
			readBytes(2);
			commandReg = vgmFileBuffer[0];
			commandData = vgmFileBuffer[1];
			break;	
			
		// YM3526 write value to register (first chip)
		// Add additional handling later for OPL1+OPL2 if someone uses it...
		case 0x5B:
			readBytes(2);
			commandReg = vgmFileBuffer[0];
			commandData = vgmFileBuffer[1];
			break;	
			
		// Y8950 write value to register (skip 2 bytes)
		case 0x5C:
			readBytes(2);
			break;		
			
		// YMZ280B write value to register (skip 2 bytes)
		case 0x5D:
			readBytes(2);
			break;		
			
		// YMF262 port 0 write value to register (first chip)
		case 0x5E:	
			readBytes(2);
			commandReg = vgmFileBuffer[0];
			commandData = vgmFileBuffer[1];
			break;		
		
		// YMF262 port 1 write value to register (first chip)
		case 0x5F:
			readBytes(2);
			commandReg = vgmFileBuffer[0];
			commandData = vgmFileBuffer[1];
			break;
			
		// Wait
		case 0x61:
			readBytes(2);
			currentWait = *(uint16_t *)&vgmFileBuffer[0];
			break;

		// Wait shortcut - 735 samples - we just turn this into a normal Wait with preset value
		case 0x62:
			currentWait = 0x02DF;
			commandID = 0x61;
			break;
		
		// Wait shortcut - 882 samples - we just turn this into a normal Wait with preset value
		case 0x63:
			currentWait = 0x0372;
			commandID = 0x61;
			break;
			
		// End of sound data (to be handled elsewhere)
		case 0x66:
			break;
			
		// Data block
		case 0x67:
			// Read in data block header
			readBytes(6);
			// Use the last 4 bytes to find out how much to skip
			// Use fseek because it may be greater than our buffer size.
			fseek(vgmFilePointer,*((uint32_t *)vgmFileBuffer[2]),SEEK_CUR);
			fileCursorLocation = *((uint32_t *)vgmFileBuffer[2]);
			break;
			
		// PCM RAM write (skip 11 bytes)
		case 0x68:
			readBytes(11);
			break;
		
		// Wait 1-16 samples
		case 0x70:
		case 0x71:
		case 0x72:
		case 0x73:
		case 0x74:
		case 0x75:
		case 0x76:
		case 0x77:
		case 0x78:
		case 0x79:
		case 0x7A:
		case 0x7B:
		case 0x7C:
		case 0x7D:
		case 0x7E:
		case 0x7F:
			currentWait = (commandID-0x6F);
			commandID = 0x61;
			break;
			
		// YM2612 port 0 address 2A write from data bank + wait (ignore)
		case 0x80:
		case 0x81:
		case 0x82:
		case 0x83:
		case 0x84:
		case 0x85:
		case 0x86:
		case 0x87:
		case 0x88:
		case 0x89:
		case 0x8A:
		case 0x8B:
		case 0x8C:
		case 0x8D:
		case 0x8E:
		case 0x8F:
			break;	
			
		// DAC stream control - setup stream control (skip 4 bytes)
		case 0x90:
			readBytes(4);
			break;
		
		// DAC stream control - set stream data (skip 4 bytes)
		case 0x91:
			readBytes(4);
			break;
		
		// DAC stream control - set stream frequency (skip 5 bytes)
		case 0x92:
			readBytes(5);
			break;
		
		// DAC stream control - start stream (skip 10 bytes)
		case 0x93:
			readBytes(10);
			break;
			
		// DAC stream control - stop stream (skip 1 byte)
		case 0x94:
			readBytes(1);
			break;
		
		// DAC stream control - start stream fast call (skip 4 bytes)
		case 0x95:
			readBytes(4);
			break;
		
		// AY8910 write value to register (skip 2 bytes)
		case 0xA0:
			readBytes(2);
			break;
		
		// YM2413 write value to register (second chip) (skip 2 bytes)
		case 0xA1:
			readBytes(2);
			break;
		
		// YM2612 port 0 write value to register (second chip) (skip 2 bytes)
		case 0xA2:
			readBytes(2);
			break;
		
		// YM2612 port 1 write value to register (second chip) (skip 2 bytes)
		case 0xA3:
			readBytes(2);
			break;	
			
		// YM2151 write value to register (second chip) (skip 2 bytes)
		case 0xA4:
			readBytes(2);
			break;	
		
		// YM2203 write value to register (second chip) (skip 2 bytes)
		case 0xA5:
			readBytes(2);
			break;
		
		// YM2608 port 0 write value to register (second chip) (skip 2 bytes)
		case 0xA6:
			readBytes(2);
			break;
		
		// YM2608 port 1 write value to register (second chip) (skip 2 bytes)
		case 0xA7:
			readBytes(2);
			break;

		// YM2610 port 0 write value to register (second chip) (skip 2 bytes)
		case 0xA8:
			readBytes(2);
			break;	
			
		// YM2610 port 1 write value to register (second chip) (skip 2 bytes)
		case 0xA9:
			readBytes(2);
			break;
			
		// YM3812 write value to register (second chip)
		case 0xAA:
			readBytes(2);
			commandReg = vgmFileBuffer[0];
			commandData = vgmFileBuffer[1];
			break;	
			
		// YM3526 write value to register (second chip)
		// Add additional handling later for OPL1+OPL2 if someone uses it...
		case 0xAB:
			readBytes(2);
			commandReg = vgmFileBuffer[0];
			commandData = vgmFileBuffer[1];
			break;	
			
		// Y8950 write value to register (second chip) (skip 2 bytes)
		case 0xAC:
			readBytes(2);
			break;		
			
		// YMZ280B write value to register (second chip) (skip 2 bytes)
		case 0xAD:
			readBytes(2);
			break;		
			
		// YMF262 port 0 write value to register (second chip) (skip 2 bytes)
		case 0xAE:	
			readBytes(2);
			break;		
		
		// YMF262 port 1 write value to register (second chip) (skip 2 bytes)
		case 0xAF:
			readBytes(2);
			break;
		
		// RF5C68 write value to register (skip 2 bytes)
		case 0xB0:
			readBytes(2);
			break;
		
		// RF5C164 write value to register (skip 2 bytes)
		case 0xB1:
			readBytes(2);
			break;
			
		//PWM write value to register (skip 2 bytes)
		case 0xB2:
			readBytes(2);
			break;
		
		// GameBoy DMG write value to register (skip 2 bytes)
		case 0xB3:
			readBytes(2);
			break;
			
		// NES APU write value to register (skip 2 bytes)
		case 0xB4:
			readBytes(2);
			break;
		
		// MultiPCM write value to register (skip 2 bytes)
		case 0xB5:
			readBytes(2);
			break;
			
		// uPD7759 write value to register (skip 2 bytes)
		case 0xB6:
			readBytes(2);
			break;
		
		// OKIM6258 write value to register (skip 2 bytes)		
		case 0xB7:
			readBytes(2);
			break;
			
		// OKIM6295 write value to register (skip 2 bytes)			
		case 0xB8:
			readBytes(2);
			break;
			
		// HuC6280 write value to register (skip 2 bytes)		
		case 0xB9:
			readBytes(2);
			break;
			
		// K053260 write value to register (skip 2 bytes)			
		case 0xBA:
			readBytes(2);
			break;
			
		// POKEY write value to register (skip 2 bytes)		
		case 0xBB:
			readBytes(2);
			break;
			
		// WonderSwan write value to register (skip 2 bytes)		
		case 0xBC:
			readBytes(2);
			break;
			
		// SAA1099 write value to register (skip 2 bytes)		
		case 0xBD:
			readBytes(2);
			break;
			
		// ES5506 write value to register (skip 2 bytes)		
		case 0xBE:
			readBytes(2);
			break;
			
		// GA20 write value to register (skip 2 bytes)		
		case 0xBF:
			readBytes(2);
			break;
		
		// SegaPCM write value to memory offset (skip 3 bytes)		
		case 0xC0:
			readBytes(3);
			break;
		
		// RF5C68 write value to memory offset (skip 3 bytes)		
		case 0xC1:
			readBytes(3);
			break;
			
		// RF5C164 write value to memory offset (skip 3 bytes)		
		case 0xC2:
			readBytes(3);
			break;
			
		// MultiPCM write set bank offset (skip 3 bytes)		
		case 0xC3:
			readBytes(3);
			break;
			
		// QSound write value to register (skip 3 bytes)		
		case 0xC4:
			readBytes(3);
			break;
			
		// SCSP write value to memory offset (skip 3 bytes)		
		case 0xC5:
			readBytes(3);
			break;
			
		// WonderSwan write value to memory offset (skip 3 bytes)		
		case 0xC6:
			readBytes(3);
			break;
			
		// VSU write value to memory offset (skip 3 bytes)		
		case 0xC7:
			readBytes(3);
			break;
			
		// X1-010 write value to memory offset (skip 3 bytes)		
		case 0xC8:
			readBytes(3);
			break;
			
		// Reserved/unused three byte commands (skip 3 bytes)
		case 0xC9:
		case 0xCA:
		case 0xCB:
		case 0xCC:
		case 0xCD:
		case 0xCE:
		case 0xCF:
			readBytes(3);
			break;
			
		// YMF278B write value to register (skip 3 bytes)		
		case 0xD0:
			readBytes(3);
			break;
		
		// YMF271 write value to register (skip 3 bytes)		
		case 0xD1:
			readBytes(3);
			break;
		
		// SCC1 write value to register (skip 3 bytes)		
		case 0xD2:
			readBytes(3);
			break;
		
		// K054539 write value to register (skip 3 bytes)		
		case 0xD3:
			readBytes(3);
			break;
		
		// C140 write value to register (skip 3 bytes)		
		case 0xD4:
			readBytes(3);
			break;
		
		// ES5503 write value to register (skip 3 bytes)		
		case 0xD5:
			readBytes(3);
			break;
		
		// ES5506 write value to register (skip 3 bytes)		
		case 0xD6:
			readBytes(3);
			break;
		
		// Reserved/unused three byte commands (skip 3 bytes)
		case 0xD7:
		case 0xD8:
		case 0xD9:
		case 0xDA:
		case 0xDB:
		case 0xDC:
		case 0xDD:
		case 0xDE:
		case 0xDF:
			readBytes(3);
			break;
		
		// YM2612 seek to offset in PCM data bank of data block type 0 (skip 4 bytes)
		case 0xE0:
			readBytes(4);
			break;
		
		// C352 write value to register (skip 4 bytes)		
		case 0xE1:
			readBytes(4);
			break;
			
		// Reserved/unused four byte commands (skip 4 bytes)
		case 0xE2:
		case 0xE3:
		case 0xE4:
		case 0xE5:
		case 0xE6:
		case 0xE7:
		case 0xE8:
		case 0xE9:
		case 0xEA:
		case 0xEB:
		case 0xEC:
		case 0xED:
		case 0xEE:
		case 0xEF:
		case 0xF0:
		case 0xF1:
		case 0xF2:
		case 0xF3:
		case 0xF4:
		case 0xF5:
		case 0xF6:
		case 0xF7:
		case 0xF8:
		case 0xF9:
		case 0xFA:
		case 0xFB:
		case 0xFC:
		case 0xFD:
		case 0xFE:
		case 0xFF:
			readBytes(4);
			break;	
		
		// We found something else.
		// If the file pointer has advanced into the GD3 header, just treat it like the end of the song.  (That is to say that we probably have a malformed VGM missing the "end of song data" command.
		// On the other hand, if we are somewhere in the middle of the song, we can't just skip an invalid command because we don't know how many bytes that command should be, so error out.
		default:
			if (currentVGMHeader.gd3Offset > 0 && (ftell(vgmFilePointer) >= (currentVGMHeader.gd3Offset+0x14)))
			{
				programState = 2;
				return 1;
			}
			else
			{
				killProgram(6);
			}
	}
	// Based on whatever the last wait value was, set what sample we "should" be on.
	// There can be multiple commands per sample, so this won't advance the sample counter in that case.
	dataCurrentSample = dataCurrentSample + currentWait;
	// Successful, return good status
	return 0;
}

// Extract the next null terminated string from the overall GD3 tag
// This is adapted from the original VGMPlay's ReadWStrFromFile
wchar_t* getNextGd3String(void)
{
	uint32_t currentPosition;
	uint32_t eofLocation;
	wchar_t* text;
	wchar_t* tempText;
	uint32_t stringLength;
	uint16_t grabbedCharacter;
	
	// Set an outer boundary on our reads.
	eofLocation = currentVGMHeader.eofOffset+0x04;
	// Track where we are now (should be the start of the GD3 based on populateCurrentGd3)
	currentPosition = ftell(vgmFilePointer);
	
	// Allocate memory the same size as the entire GD3 tag.
	text = (wchar_t*)malloc(currentGD3Tag.tagLength);
	// If there is nothing in the tag, return nothing!  (Admittedly, you shouldn't end up here at all... but just in case!)
	if (text == NULL)
	{
		return NULL;
	}
	
	tempText = text - 1;
	stringLength = 0x00;
	
	// Count characters in order until we encounter a null terminator.
	do
	{
		tempText++;
		// Read 2 bytes, the size of a wide character
		readBytes(2);
		// Store what we just read, interpret it as a wide character
		grabbedCharacter = (uint16_t)vgmFileBuffer[0x00];
		*tempText = (wchar_t)grabbedCharacter;
		// Move our position
		currentPosition+=0x02;
		// Add one character to the length of this string
		stringLength+=0x01;
		// If we overrun the end of the file for some reason, "add" a null terminator so we stop reading
		if (currentPosition >= eofLocation)
		{
			*tempText = L'\0';
			break;
		}
	} while (*tempText != L'\0');
	
	// Now that we know how long the actual single string is, reallocate the memory to the smaller size.
	text = (wchar_t *)realloc(text, stringLength * sizeof(wchar_t));
	// Send that resized allocation back.  That is the string.
	return text;
}

// Calls getNextGd3String to populate each GD3 tag value
void populateCurrentGd3(void)
{
		// Fill in default values
		currentGD3Tag.tagLength = 0;
		currentGD3Tag.trackNameE = L"";
		currentGD3Tag.trackNameJ = L"";
		currentGD3Tag.gameNameE = L"";
		currentGD3Tag.gameNameJ = L"";
		currentGD3Tag.systemNameE = L"";
		currentGD3Tag.systemNameJ = L"";
		currentGD3Tag.originalAuthorE = L"";
		currentGD3Tag.originalAuthorJ = L"";
		currentGD3Tag.releaseDate = L"";
		currentGD3Tag.converter = L"";
		currentGD3Tag.notes = L"";

		// There is a GD3, fill it in
		if (currentVGMHeader.gd3Offset != 0 )
		{
			// Ensure file seeks to the offset the GD3 sits at.
			fseek(vgmFilePointer,currentVGMHeader.gd3Offset+0x14,SEEK_SET);
			fileCursorLocation = currentVGMHeader.gd3Offset+0x14;
			// Then start reading in tag values.  As we get to the end of each one, it should be in place for the next string.
			// Jump 12 bytes forward (covering GD3 identifier + version tag + data length)
			// Also I'm ignoring the version tag cause there seems to only be one version for now... ;)
			readBytes(12);
			currentGD3Tag.tagLength = *((uint32_t *)&vgmFileBuffer [0x08]);
			// Read in all fields in order.  This makes it easy as the file seek is always in the right place.
			currentGD3Tag.trackNameE = getNextGd3String();
			currentGD3Tag.trackNameJ = getNextGd3String();
			currentGD3Tag.gameNameE = getNextGd3String();
			currentGD3Tag.gameNameJ = getNextGd3String();
			currentGD3Tag.systemNameE = getNextGd3String();
			currentGD3Tag.systemNameJ = getNextGd3String();
			currentGD3Tag.originalAuthorE = getNextGd3String();
			currentGD3Tag.originalAuthorJ = getNextGd3String();
			currentGD3Tag.releaseDate = getNextGd3String();
			currentGD3Tag.converter = getNextGd3String();
			currentGD3Tag.notes = getNextGd3String();
		}
}

// Function is called during the timer loop to actually interpret the loaded VGM command and act accordingly
void processCommands(void)
{		
	// Read commands until we are on the same sample as the timer expects.
	// This isn't -great- due to the high timer resolution we are always lagging a bit and it basically locks the app up on slow CPUs.
	while (dataCurrentSample < tickCounter)
	{
		// Get the next command data, but stop processing if it didn't work
		if (getNextCommandData() != 0)
		{
			break;
		}

		// If end of song data, check for loop, or end song
		// Properly formatted VGMs use command 0x66 for this.
		// If this command is missing, readBytes via getNextCommandData should have caught that we overran the end of the file.
		if (commandID == 0x66)
		{
			if (loopCount < loopMax && currentVGMHeader.loopOffset > 0)
			{
				fseek(vgmFilePointer, currentVGMHeader.loopOffset+0x1C, SEEK_SET);
				fileCursorLocation = currentVGMHeader.loopOffset+0x1C;
				loopCount++;
				getNextCommandData();
			}
			else
			{
				programState = 2;
				return;
			}
		}
		// Interpret which command we are looking at - data or wait?
		switch (commandID)
		{	
			// OPL2 write
			case 0x5A:
				// Dual-OPL2 on OPL3 hack
				// First chip (left pan)
				if (detectedChip == 3 && vgmChipType == 5)
				{
					if ((commandReg & 0xC0) == 0xC0)
					{
						// Zero the stereo bits and write new panning
						commandData = ((commandData & 0x0F) | 0x10);
					}
				}
				writeOPL(commandReg,commandData);
				break;
			// OPL1 write
			case 0x5B:
				writeOPL(commandReg,commandData);
				break;
			// OPL3 write (port 1)
			case 0x5E:
				writeOPL(commandReg,commandData);
				break;
			// OPL3 write (port 2) - modify destination register en-route to indicate secondary registers.
			case 0x5F:
				writeOPL(commandReg+0x100,commandData);
				break;
			// Second OPL2 write - modify destination register en-route to indicate secondary registers.
			case 0xAA:
				// Dual-OPL2 on OPL3 hack
				// Modify incoming data if needed to force panning
				if (detectedChip == 3 && vgmChipType == 5)
				{
					// Second chip (right pan)
					if ((commandReg & 0xC0) == 0xC0)
					{
						// Zero the stereo bits and write new panning
						commandData = ((commandData & 0x0F) | 0x20);
					}
					// Absolutely under no circumstances try to write OPL2 Waveform Select to the OPL3 high block
					// This will cause the OPL3 to stop outputting sound
					if (commandReg == 0x01)
					{
						break;
					}
				}
				writeOPL(commandReg+0x100,commandData);
				break;
			// Second OPL1 write - modify destination register en-route to indicate secondary registers.
			case 0xAB:
				writeOPL(commandReg+0x100,commandData);
				break;	
			default:
				break;
		}
		
	}
}

///////////////////////////////////////////////////////////////////////////////
// Misc Functions
///////////////////////////////////////////////////////////////////////////////

// Reads in how many bytes we need for the next command.
uint8_t readBytes(uint16_t numBytes)
{
	if ((fread(vgmFileBuffer,sizeof(char),numBytes,vgmFilePointer)) != numBytes)
	{
		return 1;
	}
	// Keep track of where we are in the file.
	fileCursorLocation = fileCursorLocation + numBytes;
	return 0;
}

// Ends the program, with printing informational messages
void killProgram(uint8_t errorCode)
{
	// Cleanup before closing
	
	// Only reset timer if we already hijacked ISR8
	if (biosISR8 != NULL)
	{
			resetTimer();
	}
	
	// Only release the files if we actually loaded them
	if (vgmFilePointer != NULL)
	{
		fclose(vgmFilePointer);
	}
	if (configFilePointer != NULL)
	{
		fclose(configFilePointer);
	}
	if (initialFilePointer != NULL)
	{
		fclose(initialFilePointer);
	}
	if (playlistFilePointer != NULL)
	{
		fclose(playlistFilePointer);
	}
	// Reset OPL but only if one was detected
	if (detectedChip != 0)
	{
		resetOPL();
	}
	clearTextScreen();
	setVideoMode(25);
	
	// Print error code, if necessary
	switch (errorCode)
	{
		case 0:
			printf("Thank you for slappin' with VGMSlap!\n");
			printf("by Wafflenet, 2023-2024\n");
			printf("www.wafflenet.com\n");
			printf("\n");
			break;
		case 1:
			printf("Usage: VGMSLAP <FILENAME>\n");
			break;
		case 2:
			printf("Huh?  That file doesn't exist...");
			break;
		case 3:
			printf("This is not a VGM file!\n");
			break;
		case 4:
			printf("This VGM is too old to have OPL support!\n");
			break;
		case 5:
			printf("No supported OPL chips in this VGM!\n");
			break;
		case 6:
			printf("(%08X) : Invalid command %08X! Bailing out.\n", fileCursorLocation, commandID);
			break;
		case 7:
			printf("No OPL detected at %Xh!\n", settings.oplBase);
			break;
		case 8:
			printf("Decompression failed!\n");
			break;
		case 9:
			printf("Load error in playlist handler!\n");
			printf("%s", fileName);
			perror("");
			break;
		case 10:
			printf("Load error in VGM handler!\n");
			printf("%s", vgmFileName);
			perror("");
			break;
		case 11:
			printf("Load error in decompression handler!\n");
			printf("%s", vgmFileName);
			perror("");
			break;
		case 12:
			printf("Load error in tempfile handler!\n");
			printf("%s", settings.tempPath);
			perror("");
			break;
	}
	exit(errorCode);
}
