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
// VGMSLAP.C - Main source file
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>

#include "opl.h"
#include "playlist.h"
#include "settings.h"
#include "timer.h"
#include "txtmode.h"
#include "ui.h"
#include "vgm.h"
#include "vgmslap.h"

///////////////////////////////////////////////////////////////////////////////
// Initialize variables
///////////////////////////////////////////////////////////////////////////////

ProgramState programState = STATE_INITIALIZATION;
char* fileName;
FILE *initialFilePointer;

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
	uint16_t i;
	// Check for arguments
		if (argc != 2)
		{
				
				killProgram(ERROR_NO_ARGUMENTS);
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
		killProgram(ERROR_FILE_MISSING);
	}
	// Did we load a playlist, or a single VGM?
	fgets(playlistLineBuffer,sizeof(playlistLineBuffer),initialFilePointer);
	// It's a playlist
	if (strncmp(playlistLineBuffer,"#VGMLIST", 8) == 0)
	{
		printf("Playlist detected!\n");
		playlistMode = TRUE;
		playlistLineNumber = 1;
	}
	
	// Close "initial file", we're done with it
	// At this point we've validated that it is in fact a file already
	if (initialFilePointer != NULL)
	{
		fclose(initialFilePointer);
	}
	
	// If playlist was found, open playlist, jump to playlist handler and load first song
	if (playlistMode == TRUE)
	{
		playlistInit();
	}
	// Playlist wasn't found, so probably a VGM, try interpreting as such.
	else if (playlistMode == FALSE)
	{
		vgmFileName = fileName;	
	}
	
	// Start playback incl. VGM load and timer init
	initPlayback();

	while (programState == STATE_PLAYING || programState == STATE_END_OF_SONG)
	{
		// If playing then we are in the main logic loop.  Commands will be processed, input will be read, and the screen will be refreshed.
		if (programState == STATE_PLAYING)
		{
			processCommands();
			
			// Press a key to quit
			// Todo: How to force the keyboard to respond if the CPU is overloaded due to playing a busy VGM on underspecced hardware?  Keyboard interrupt is getting missed.  Also keypress gets passed to next program (command, file manager, etc) after quit.  Detect release before acting?
			inputHandler();
			
			// Refresh screen
			if (settings.struggleBus == 0)
			{
				// If a register value changed, requestScreenDraw gets set so that the new table values are drawn
				if (requestScreenDraw > 0)
				{
					drawChannelTable();
					
				}
				// Channel bars at the bottom are refreshed at a slower rate
				if (screenCounter > VGA_REFRESH_TICKS)
				{
					updateLevelBars();
					screenCounter = 0;
				}
			}

		}
		// Things to do when we run out of song
		else if (programState == STATE_END_OF_SONG)
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
			if (playlistMode == FALSE)
			{
				programState = STATE_EXIT;
			}
			else if (playlistMode == TRUE)
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
					programState = STATE_EXIT;
				}
			}
		}
	}
	while (programState == STATE_EXIT)
		{
			killProgram(EXIT_OK);
		}
	return 0;
}

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

	// Register limit for display based on max channels we set
	if (maxChannels > 9)
	{
		displayRegisterMax = 0x1FF;
	}
	else
	{
		displayRegisterMax = 0xFF;
	}
					
	// Set up the OPL chip
	resetOPL();
	
	// Reset screen state
	if (settings.struggleBus == 0)
	{
		setVideoMode(TEXT_80X50);
	}
	else
	{
		setVideoMode(TEXT_80X25);
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
	if (detectedChip == DETECTED_OPL3 && vgmChipType == VGM_DUAL_OPL2)
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
	programState = STATE_PLAYING;
}

void killProgram(ProgramExitCode errorCode)
{
	// First, perform cleanup actions before closing
	
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
	if (detectedChip != DETECTED_NONE)
	{
		resetOPL();
	}
	clearTextScreen();
	setVideoMode(TEXT_80X25);
	
	// Print error code, if necessary
	switch (errorCode)
	{
		case EXIT_OK:
			printf("Thank you for slappin' with VGMSlap!\n");
			printf("by Wafflenet, 2023-2024\n");
			printf("www.wafflenet.com\n");
			printf("\n");
			break;
		case ERROR_NO_ARGUMENTS:
			printf("Usage: VGMSLAP <FILENAME>\n");
			break;
		case ERROR_FILE_MISSING:
			printf("Huh?  That file doesn't exist...");
			break;
		case ERROR_BAD_FILETYPE:
			printf("This is not a VGM file!\n");
			break;
		case ERROR_VGM_VERSION:
			printf("This VGM is too old to have OPL support!\n");
			break;
		case ERROR_VGM_NO_SUPPORTED_CHIPS:
			printf("No supported OPL chips in this VGM!\n");
			break;
		case ERROR_VGM_BAD_COMMAND:
			printf("(%08X) : Invalid command %08X! Bailing out.\n", fileCursorLocation, commandID);
			break;
		case ERROR_OPL_DETECTION_FAILED:
			printf("No OPL detected at %Xh!\n", settings.oplBase);
			break;
		case ERROR_DECOMPRESSION_FAILED:
			printf("Decompression failed!\n");
			break;
		case ERROR_LOAD_FAILED_PLAYLIST:
			printf("Load error in playlist handler!\n");
			printf("%s", fileName);
			perror("");
			break;
		case ERROR_LOAD_FAILED_VGM:
			printf("Load error in VGM handler!\n");
			printf("%s", vgmFileName);
			perror("");
			break;
		case ERROR_LOAD_FAILED_ZLIB:
			printf("Load error in decompression handler!\n");
			printf("%s", vgmFileName);
			perror("");
			break;
		case ERROR_LOAD_FAILED_TEMPFILE:
			printf("Load error in tempfile handler!\n");
			printf("%s", settings.tempPath);
			perror("");
			break;
	}
	exit(errorCode);
}
