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
// TXTMODE.H - General text mode display functions
//
///////////////////////////////////////////////////////////////////////////////

#ifndef VGMSLAP_TXTMODE_H
#define VGMSLAP_TXTMODE_H

#include "types.h"

///////////////////////////////////////////////////////////////////////////////
// Macro definitions
///////////////////////////////////////////////////////////////////////////////

#define characterCoordinate(xPos, yPos) 2*((yPos*80)+xPos)

///////////////////////////////////////////////////////////////////////////////
// Function declarations
///////////////////////////////////////////////////////////////////////////////

void clearTextScreen(void); 		// Blanks out the text screen by writing spaces
void setVideoMode(ScreenMode mode);	// Change screen mode and set parameters needed

// Text-drawing functions

// Put a character directly into memory at a given coordinate
void drawCharacterAtPosition(char text, uint8_t xPos, uint8_t yPos, uint8_t foregroundColor, uint8_t backgroundColor);

// Put a full string directly into memory at a given coordinate
void drawStringAtPosition(char* text, uint8_t xPos, uint8_t yPos, uint8_t foregroundColor, uint8_t backgroundColor);

// Put an array based "graphic" into memory at a given coordinate
// Pass the name of the graphic array, dimensions, and then where you want it to start (top left)
void drawGraphicAtPosition(const char* graphicArray, uint8_t xSize, uint8_t ySize, uint8_t xOrigin, uint8_t yOrigin);

// Special drawing function for level bars - vertical, fixed height of 6
// Drawn from top to bottom with shared attribute line
void drawLevelBar(const char* graphicArray, uint8_t value, uint8_t xOrigin, uint8_t yOrigin, uint8_t width);

///////////////////////////////////////////////////////////////////////////////
// Variable declarations
///////////////////////////////////////////////////////////////////////////////

extern char txtDrawBuffer[80];		// Temporary buffer line for text display
extern char far *textScreen;		// VGA text screen memory location
extern char numToHex[];				// For quick num > hex conversion
extern char textRows;				// Number of rows in text mode
extern uint16_t displayRegisterMax;	// How many registers to iterate for screen refresh

#endif
