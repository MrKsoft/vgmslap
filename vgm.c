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
// VGM.C - VGM file processing
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opl.h"
#include "settings.h"
#include "timer.h"
#include "vgm.h"
#include "vgmslap.h"

///////////////////////////////////////////////////////////////////////////////
// Initialize variables
///////////////////////////////////////////////////////////////////////////////

char vgmIdentifier[] = "Vgm ";
char gzMagicNumber[2] = {0x1F, 0x8B};
char vgmFileBuffer[256];
FILE *vgmFilePointer;
char* vgmFileName;
gzFile compressedFile;
uint32_t fileCursorLocation = 0;
uint32_t dataCurrentSample = 0;
char commandID = 0;
uint8_t loopCount = 0;
uint8_t loopMax = 1;
VgmChipType vgmChipType = VGM_NO_OPL;

vgmHeader currentVGMHeader;
gd3Tag currentGD3Tag;

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

uint8_t getNextCommandData(void)
{
	uint32_t currentWait = 0;
	// Proper functioning of this function is based on the file already being seeked to the VGM data offset

	// Read the first byte to see what command we are looking at.
	// The byte length can be different depending on the command, so we have to decide how many bytes to read in.
	if (vgmReadBytes(1) == 0)
	{
		commandID = vgmFileBuffer[0];
	}
	// Read failed - probably ran out of bytes.  End song.
	else
	{
		programState = STATE_END_OF_SONG;
	}


	// Mega size switch case for all known VGM commands.  Most will be ignored, though, since they don't apply to OPL.
	// We still need to handle the right number of bytes though, especially for multichip VGMs that happen to have OPL in them too.
	// The good news is that all OPL commands are two bytes (reg/data) so we can return a command easily.
	// There are ways to shrink this down considerably (grouping all x-byte commands), but for now I'm being explicit and can optimize later.
	switch (commandID)
	{

		// SN76489 (2nd chip) write value (skip 1 byte)
		case 0x30:
			vgmReadBytes(1);
			break;

		// AY8910 stereo mask (skip 1 byte)
		case 0x31:
			vgmReadBytes(1);
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
			vgmReadBytes(1);
			break;


		// Game Gear Stereo write value (skip 1 byte)
		case 0x3F:
			vgmReadBytes(1);
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
				vgmReadBytes(1);
			}
			else
			{
				vgmReadBytes(2);
			}
			break;

		// Game Gear PSG stereo (skip 1 byte)
		case 0x4F:
			vgmReadBytes(1);
			break;

		// SN76489 write value (skip 1 byte)
		case 0x50:
			vgmReadBytes(1);
			break;

		// YM2413 write value to register (skip 2 bytes)
		case 0x51:
			vgmReadBytes(2);
			break;

		// YM2612 port 0 write value to register (skip 2 bytes)
		case 0x52:
			vgmReadBytes(2);
			break;

		// YM2612 port 1 write value to register (skip 2 bytes)
		case 0x53:
			vgmReadBytes(2);
			break;

		// YM2151 write value to register (skip 2 bytes)
		case 0x54:
			vgmReadBytes(2);
			break;

		// YM2203 write value to register (skip 2 bytes)
		case 0x55:
			vgmReadBytes(2);
			break;

		// YM2608 port 0 write value to register (skip 2 bytes)
		case 0x56:
			vgmReadBytes(2);
			break;

		// YM2608 port 1 write value to register (skip 2 bytes)
		case 0x57:
			vgmReadBytes(2);
			break;

		// YM2610 port 0 write value to register (skip 2 bytes)
		case 0x58:
			vgmReadBytes(2);
			break;

		// YM2610 port 1 write value to register (skip 2 bytes)
		case 0x59:
			vgmReadBytes(2);
			break;

		// YM3812 write value to register (first chip)
		case 0x5A:
			vgmReadBytes(2);
			commandReg = vgmFileBuffer[0];
			commandData = vgmFileBuffer[1];
			break;

		// YM3526 write value to register (first chip)
		// Add additional handling later for OPL1+OPL2 if someone uses it...
		case 0x5B:
			vgmReadBytes(2);
			commandReg = vgmFileBuffer[0];
			commandData = vgmFileBuffer[1];
			break;

		// Y8950 write value to register (skip 2 bytes)
		case 0x5C:
			vgmReadBytes(2);
			break;

		// YMZ280B write value to register (skip 2 bytes)
		case 0x5D:
			vgmReadBytes(2);
			break;

		// YMF262 port 0 write value to register (first chip)
		case 0x5E:
			vgmReadBytes(2);
			commandReg = vgmFileBuffer[0];
			commandData = vgmFileBuffer[1];
			break;

		// YMF262 port 1 write value to register (first chip)
		case 0x5F:
			vgmReadBytes(2);
			commandReg = vgmFileBuffer[0];
			commandData = vgmFileBuffer[1];
			break;

		// Wait
		case 0x61:
			vgmReadBytes(2);
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
			vgmReadBytes(6);
			// Use the last 4 bytes to find out how much to skip
			// Use fseek because it may be greater than our buffer size.
			fseek(vgmFilePointer,*((uint32_t *)vgmFileBuffer[2]),SEEK_CUR);
			fileCursorLocation = *((uint32_t *)vgmFileBuffer[2]);
			break;

		// PCM RAM write (skip 11 bytes)
		case 0x68:
			vgmReadBytes(11);
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
			vgmReadBytes(4);
			break;

		// DAC stream control - set stream data (skip 4 bytes)
		case 0x91:
			vgmReadBytes(4);
			break;

		// DAC stream control - set stream frequency (skip 5 bytes)
		case 0x92:
			vgmReadBytes(5);
			break;

		// DAC stream control - start stream (skip 10 bytes)
		case 0x93:
			vgmReadBytes(10);
			break;

		// DAC stream control - stop stream (skip 1 byte)
		case 0x94:
			vgmReadBytes(1);
			break;

		// DAC stream control - start stream fast call (skip 4 bytes)
		case 0x95:
			vgmReadBytes(4);
			break;

		// AY8910 write value to register (skip 2 bytes)
		case 0xA0:
			vgmReadBytes(2);
			break;

		// YM2413 write value to register (second chip) (skip 2 bytes)
		case 0xA1:
			vgmReadBytes(2);
			break;

		// YM2612 port 0 write value to register (second chip) (skip 2 bytes)
		case 0xA2:
			vgmReadBytes(2);
			break;

		// YM2612 port 1 write value to register (second chip) (skip 2 bytes)
		case 0xA3:
			vgmReadBytes(2);
			break;

		// YM2151 write value to register (second chip) (skip 2 bytes)
		case 0xA4:
			vgmReadBytes(2);
			break;

		// YM2203 write value to register (second chip) (skip 2 bytes)
		case 0xA5:
			vgmReadBytes(2);
			break;

		// YM2608 port 0 write value to register (second chip) (skip 2 bytes)
		case 0xA6:
			vgmReadBytes(2);
			break;

		// YM2608 port 1 write value to register (second chip) (skip 2 bytes)
		case 0xA7:
			vgmReadBytes(2);
			break;

		// YM2610 port 0 write value to register (second chip) (skip 2 bytes)
		case 0xA8:
			vgmReadBytes(2);
			break;

		// YM2610 port 1 write value to register (second chip) (skip 2 bytes)
		case 0xA9:
			vgmReadBytes(2);
			break;

		// YM3812 write value to register (second chip)
		case 0xAA:
			vgmReadBytes(2);
			commandReg = vgmFileBuffer[0];
			commandData = vgmFileBuffer[1];
			break;

		// YM3526 write value to register (second chip)
		// Add additional handling later for OPL1+OPL2 if someone uses it...
		case 0xAB:
			vgmReadBytes(2);
			commandReg = vgmFileBuffer[0];
			commandData = vgmFileBuffer[1];
			break;

		// Y8950 write value to register (second chip) (skip 2 bytes)
		case 0xAC:
			vgmReadBytes(2);
			break;

		// YMZ280B write value to register (second chip) (skip 2 bytes)
		case 0xAD:
			vgmReadBytes(2);
			break;

		// YMF262 port 0 write value to register (second chip) (skip 2 bytes)
		case 0xAE:
			vgmReadBytes(2);
			break;

		// YMF262 port 1 write value to register (second chip) (skip 2 bytes)
		case 0xAF:
			vgmReadBytes(2);
			break;

		// RF5C68 write value to register (skip 2 bytes)
		case 0xB0:
			vgmReadBytes(2);
			break;

		// RF5C164 write value to register (skip 2 bytes)
		case 0xB1:
			vgmReadBytes(2);
			break;

		//PWM write value to register (skip 2 bytes)
		case 0xB2:
			vgmReadBytes(2);
			break;

		// GameBoy DMG write value to register (skip 2 bytes)
		case 0xB3:
			vgmReadBytes(2);
			break;

		// NES APU write value to register (skip 2 bytes)
		case 0xB4:
			vgmReadBytes(2);
			break;

		// MultiPCM write value to register (skip 2 bytes)
		case 0xB5:
			vgmReadBytes(2);
			break;

		// uPD7759 write value to register (skip 2 bytes)
		case 0xB6:
			vgmReadBytes(2);
			break;

		// OKIM6258 write value to register (skip 2 bytes)
		case 0xB7:
			vgmReadBytes(2);
			break;

		// OKIM6295 write value to register (skip 2 bytes)
		case 0xB8:
			vgmReadBytes(2);
			break;

		// HuC6280 write value to register (skip 2 bytes)
		case 0xB9:
			vgmReadBytes(2);
			break;

		// K053260 write value to register (skip 2 bytes)
		case 0xBA:
			vgmReadBytes(2);
			break;

		// POKEY write value to register (skip 2 bytes)
		case 0xBB:
			vgmReadBytes(2);
			break;

		// WonderSwan write value to register (skip 2 bytes)
		case 0xBC:
			vgmReadBytes(2);
			break;

		// SAA1099 write value to register (skip 2 bytes)
		case 0xBD:
			vgmReadBytes(2);
			break;

		// ES5506 write value to register (skip 2 bytes)
		case 0xBE:
			vgmReadBytes(2);
			break;

		// GA20 write value to register (skip 2 bytes)
		case 0xBF:
			vgmReadBytes(2);
			break;

		// SegaPCM write value to memory offset (skip 3 bytes)
		case 0xC0:
			vgmReadBytes(3);
			break;

		// RF5C68 write value to memory offset (skip 3 bytes)
		case 0xC1:
			vgmReadBytes(3);
			break;

		// RF5C164 write value to memory offset (skip 3 bytes)
		case 0xC2:
			vgmReadBytes(3);
			break;

		// MultiPCM write set bank offset (skip 3 bytes)
		case 0xC3:
			vgmReadBytes(3);
			break;

		// QSound write value to register (skip 3 bytes)
		case 0xC4:
			vgmReadBytes(3);
			break;

		// SCSP write value to memory offset (skip 3 bytes)
		case 0xC5:
			vgmReadBytes(3);
			break;

		// WonderSwan write value to memory offset (skip 3 bytes)
		case 0xC6:
			vgmReadBytes(3);
			break;

		// VSU write value to memory offset (skip 3 bytes)
		case 0xC7:
			vgmReadBytes(3);
			break;

		// X1-010 write value to memory offset (skip 3 bytes)
		case 0xC8:
			vgmReadBytes(3);
			break;

		// Reserved/unused three byte commands (skip 3 bytes)
		case 0xC9:
		case 0xCA:
		case 0xCB:
		case 0xCC:
		case 0xCD:
		case 0xCE:
		case 0xCF:
			vgmReadBytes(3);
			break;

		// YMF278B write value to register (skip 3 bytes)
		case 0xD0:
			vgmReadBytes(3);
			break;

		// YMF271 write value to register (skip 3 bytes)
		case 0xD1:
			vgmReadBytes(3);
			break;

		// SCC1 write value to register (skip 3 bytes)
		case 0xD2:
			vgmReadBytes(3);
			break;

		// K054539 write value to register (skip 3 bytes)
		case 0xD3:
			vgmReadBytes(3);
			break;

		// C140 write value to register (skip 3 bytes)
		case 0xD4:
			vgmReadBytes(3);
			break;

		// ES5503 write value to register (skip 3 bytes)
		case 0xD5:
			vgmReadBytes(3);
			break;

		// ES5506 write value to register (skip 3 bytes)
		case 0xD6:
			vgmReadBytes(3);
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
			vgmReadBytes(3);
			break;

		// YM2612 seek to offset in PCM data bank of data block type 0 (skip 4 bytes)
		case 0xE0:
			vgmReadBytes(4);
			break;

		// C352 write value to register (skip 4 bytes)
		case 0xE1:
			vgmReadBytes(4);
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
			vgmReadBytes(4);
			break;

		// We found something else.
		// If the file pointer has advanced into the GD3 header, just treat it like the end of the song.  (That is to say that we probably have a malformed VGM missing the "end of song data" command.
		// On the other hand, if we are somewhere in the middle of the song, we can't just skip an invalid command because we don't know how many bytes that command should be, so error out.
		default:
			if (currentVGMHeader.gd3Offset > 0 && (ftell(vgmFilePointer) >= (currentVGMHeader.gd3Offset+0x14)))
			{
				programState = STATE_END_OF_SONG;
				return 1;
			}
			else
			{
				killProgram(ERROR_VGM_BAD_COMMAND);
			}
	}
	// Based on whatever the last wait value was, set what sample we "should" be on.
	// There can be multiple commands per sample, so this won't advance the sample counter in that case.
	dataCurrentSample = dataCurrentSample + currentWait;
	// Successful, return good status
	return 0;
}

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
		vgmReadBytes(2);
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

uint8_t loadVGM(void)
{
	char gzipBuffer[512];
	uint16_t i;

	// Try to load the VGM
	errno = 0;
	vgmFilePointer = fopen(vgmFileName,"rb");
	if (vgmFilePointer == NULL)
	{
		killProgram(ERROR_LOAD_FAILED_VGM);
	}

	// Ensure we are at the beginning of the file.
	fseek(vgmFilePointer, 0, SEEK_SET);
	fileCursorLocation = 0;
	dataCurrentSample = 0;

	// Read the first 4 bytes so we can see the identifier.
	// We want to check if this is a VGZ before continuing, and decompress first if so.

	vgmReadBytes(4);
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
			killProgram(ERROR_LOAD_FAILED_ZLIB);
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
			killProgram(ERROR_LOAD_FAILED_TEMPFILE);
		}
	}

	// We should have a file now, so seek back to the beginning.
	fseek(vgmFilePointer, 0, SEEK_SET);
	// Read enough bytes to get the VGM header, then populate header struct
	vgmReadBytes(256);
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
		killProgram(ERROR_BAD_FILETYPE);
	}

	// If VGM version is < 1.51 just bail out cause they don't support OPL anyway
	if (currentVGMHeader.versionNumber < 0x151)
	{
		killProgram(ERROR_VGM_VERSION);
	}

	// Check for OPL2
	if (currentVGMHeader.ym3812Clock > 0)
	{
		// OK, it's OPL2.  Is it dual OPL2?
		if (currentVGMHeader.ym3812Clock > 0x40000000)
		{
			vgmChipType = VGM_DUAL_OPL2;
			maxChannels = 18;
		}
		// No, it's single OPL2.
		else
		{
			vgmChipType = VGM_SINGLE_OPL2;
			maxChannels = 9;
		}
	}

	// Check for OPL1
	if (currentVGMHeader.ym3526Clock > 0)
	{
		// OK, it's OPL1.  Is it dual OPL1?
		if (currentVGMHeader.ym3526Clock > 0x40000000)
		{
			vgmChipType = VGM_DUAL_OPL1;
			maxChannels = 18;
		}
		// No, it's single OPL1.
		else
		{
			// Was a single OPL2 already found?  This could be OPL1+OPL2.  A weird combination, but theoretically valid.
			if (vgmChipType == VGM_SINGLE_OPL2)
			{
				vgmChipType = VGM_OPL1_OPL2;
				maxChannels = 18;
			}
			// Nah, it's just an OPL1 on its own.
			else
			{
				vgmChipType = VGM_SINGLE_OPL1;
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
			vgmChipType = VGM_NO_OPL;
		}
		// No, it's single OPL3.
		else
		{
			vgmChipType = VGM_SINGLE_OPL3;
			maxChannels = 18;
		}
	}

	// If the detected chip type is still 0, then there's nothing we can play for this anyway.
	if (vgmChipType == VGM_NO_OPL)
	{
		killProgram(ERROR_VGM_NO_SUPPORTED_CHIPS);
	}

	// Chip check was ok.  Now compare vs detected OPL chip to see if it's playable, and if not, kill the program.  We also setup the base IO due to Dual OPL2 shenanigans
	switch (vgmChipType)
	{
		// OPL1
		case VGM_SINGLE_OPL1:
			if (detectedChip == DETECTED_OPL2 || detectedChip == DETECTED_OPL3)
			{
				oplBaseAddr = settings.oplBase;
				break;
			}
			// If Dual OPL2 was detected we secretly shift the base IO to base+8 so that single OPL2 goes to both stereo channels
			if (detectedChip == DETECTED_DUAL_OPL2)
			{
				oplBaseAddr = settings.oplBase+8;
				break;
			}
			else
			{
				killProgram(ERROR_VGM_NO_SUPPORTED_CHIPS);
				break;
			}
		// OPL2
		case VGM_SINGLE_OPL2:
			if (detectedChip == DETECTED_OPL2 || detectedChip == DETECTED_OPL3)
			{
				oplBaseAddr = settings.oplBase;
				break;
			}
			// If Dual OPL2 was detected we secretly shift the base IO to base+8 so that single OPL2 goes to both stereo channels
			if (detectedChip == DETECTED_DUAL_OPL2)
			{
				oplBaseAddr = settings.oplBase+8;
				break;
			}
			else
			{
				killProgram(ERROR_VGM_NO_SUPPORTED_CHIPS);
				break;
			}
		// OPL3
		case VGM_SINGLE_OPL3:
			if (detectedChip == DETECTED_OPL3)
			{
				oplBaseAddr = settings.oplBase;
				break;
			}
			else
			{
				killProgram(ERROR_VGM_NO_SUPPORTED_CHIPS);
				break;
			}
		// Dual OPL1
		case VGM_DUAL_OPL1:
			if (detectedChip == DETECTED_DUAL_OPL2 || detectedChip == DETECTED_OPL3)
			{
				oplBaseAddr = settings.oplBase;
				break;
			}
			else
			{
				killProgram(ERROR_VGM_NO_SUPPORTED_CHIPS);
				break;
			}
		// Dual OPL2
		case VGM_DUAL_OPL2:
			if (detectedChip == DETECTED_DUAL_OPL2 || detectedChip == DETECTED_OPL3)
			{
				oplBaseAddr = settings.oplBase;
				break;
			}
			else
			{
				killProgram(ERROR_VGM_NO_SUPPORTED_CHIPS);
				break;
			}
		// OPL1 + OPL2 (Haven't bothered to work on this yet)
		case VGM_OPL1_OPL2:
			if (detectedChip == DETECTED_DUAL_OPL2 || detectedChip == DETECTED_OPL3)
			{
				oplBaseAddr = settings.oplBase;
				killProgram(ERROR_VGM_NO_SUPPORTED_CHIPS); // Remove this when feature is ready
				break;
			}
			else
			{
				killProgram(ERROR_VGM_NO_SUPPORTED_CHIPS);
				break;
			}
	}

	// Everything else is okay, I say it's time to load the GD3 tag!
	populateCurrentGd3();

	// Success!
	return 0;
}

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
			vgmReadBytes(12);
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

void processCommands(void)
{
	// Read commands until we are on the same sample as the timer expects.
	while (dataCurrentSample < tickCounter)
	{
		// Get the next command data, but stop processing if it didn't work
		if (getNextCommandData() != 0)
		{
			break;
		}

		// If end of song data, check for loop, or end song
		// Properly formatted VGMs use command 0x66 for this.
		// If this command is missing, vgmReadBytes via getNextCommandData should have caught that we overran the end of the file.
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
				programState = STATE_END_OF_SONG;
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
				if (detectedChip == DETECTED_OPL3 && vgmChipType == VGM_DUAL_OPL2)
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
				if (detectedChip == DETECTED_OPL3 && vgmChipType == VGM_DUAL_OPL2)
				{
					// Second chip (right pan)
					if ((commandReg & 0xC0) == 0xC0)
					{
						// Zero the stereo bits and write new panning
						commandData = ((commandData & 0x0F) | 0x20);
					}
					// Absolutely under no circumstances try to write OPL2 Waveform Select to the OPL3 high block
					// That will cause the OPL3 to stop outputting sound
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

uint8_t vgmReadBytes(uint16_t numBytes)
{
	if ((fread(vgmFileBuffer,sizeof(char),numBytes,vgmFilePointer)) != numBytes)
	{
		return 1;
	}
	// Keep track of where we are in the file.
	fileCursorLocation = fileCursorLocation + numBytes;
	return 0;
}
