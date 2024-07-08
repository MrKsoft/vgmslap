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
// UI.H - User interface control
//
///////////////////////////////////////////////////////////////////////////////

#ifndef VGMSLAP_UI_H
#define VGMSLAP_UI_H

#include "types.h"

///////////////////////////////////////////////////////////////////////////////
// Function declarations
///////////////////////////////////////////////////////////////////////////////

void clearInterface(void);		// Clear text screen, but only certain parts of the UI
void drawChannelTable(void);	// Reads OPL values, packs them into the structs, then draws the result to the screen
								// (Only interprets and draws parts that change)
void drawTextUI(void);			// Draws the static UI components
void inputHandler(void);		// Reads and acts upon keyboard commands

///////////////////////////////////////////////////////////////////////////////
// Variable declarations
///////////////////////////////////////////////////////////////////////////////

extern uint8_t keyboardCurrent;			// Current keycode from getch()
extern uint8_t keyboardPrevious;		// Last processed keypress, to help identify a "new" vs "repeated" key
extern uint8_t keyboardExtendedFlag;	// Flag for if we are reading an extended keycode
extern uint8_t requestScreenDraw;		// Set to 1 when the screen needs to redraw

#endif
