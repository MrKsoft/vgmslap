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
// TXTMODE.C - General text mode display functions
//
///////////////////////////////////////////////////////////////////////////////

#include <conio.h>
#include <dos.h>

#include "txtgfx.h"
#include "txtmode.h"
#include "vgmslap.h"

///////////////////////////////////////////////////////////////////////////////
// Initialize variables
///////////////////////////////////////////////////////////////////////////////

char txtDrawBuffer[80];
char far *textScreen = (char far *)0xB8000000;
char numToHex[] = "0123456789ABCDEF";
char textRows = 25;
uint16_t displayRegisterMax = 0xFF;

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

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

void setVideoMode(ScreenMode mode)
{
	// Using the following numbers to represent what mode to switch to:
	// 25 = 80x25 text mode
	// 50 = 80x50 text mode
	union REGS registers;
	if (mode == TEXT_80X25)
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
	if (mode == TEXT_80X50)
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

void drawCharacterAtPosition(char text, uint8_t xPos, uint8_t yPos, uint8_t foregroundColor, uint8_t backgroundColor)
{
	// Combine foreground and background attribute
	uint8_t attribute = foregroundColor | (backgroundColor << 4);
	// Generate the correct memory location to put the text using our predefined coordinate function
	uint16_t screenAddress = characterCoordinate(xPos, yPos);
	textScreen[screenAddress++] = text;
	textScreen[screenAddress++] = attribute;
}

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
