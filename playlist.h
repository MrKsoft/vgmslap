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
// PLAYLIST.H - Playlist handling
//
///////////////////////////////////////////////////////////////////////////////

#ifndef VGMSLAP_PLAYLIST_H
#define VGMSLAP_PLAYLIST_H

#include <stdio.h>

#include "types.h"

///////////////////////////////////////////////////////////////////////////////
// Function declarations
///////////////////////////////////////////////////////////////////////////////

void countPlaylistSongs(void); 			// Gets us a count for how many lines are in the playlist
void playlistGet(uint32_t songNumber);	// Get the filename on line "songNumber" of the playlist
										// so we can show a number like (1/99) or something
void playlistInit(void);				// If a playlist was detected, this sets up playlist mode

///////////////////////////////////////////////////////////////////////////////
// Variable declarations
///////////////////////////////////////////////////////////////////////////////

extern char playlistLineBuffer[255];	// Current line of playlist (if being used)
extern uint8_t playlistMode;			// Are we using playlist or not?
extern uint16_t playlistLineNumber;		// Tracks what line (song number) we are on in a playlist
extern uint16_t playlistMax;			// How many lines are in the loaded playlist
extern FILE *playlistFilePointer;		// Pointer to loaded playlist

#endif
