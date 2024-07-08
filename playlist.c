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
// PLAYLIST.C - Playlist handling
//
///////////////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "playlist.h"
#include "vgmslap.h"
#include "vgm.h"

///////////////////////////////////////////////////////////////////////////////
// Initialize variables
///////////////////////////////////////////////////////////////////////////////

char playlistLineBuffer[255];
uint8_t playlistMode = FALSE;
uint16_t playlistLineNumber = 0;
uint16_t playlistMax = 0;
FILE *playlistFilePointer;

///////////////////////////////////////////////////////////////////////////////
// Functions
///////////////////////////////////////////////////////////////////////////////

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

void playlistInit(void)
{
	errno = 0;
	playlistFilePointer = fopen(fileName,"rt");
	if (playlistFilePointer == NULL)
	{

		killProgram(ERROR_LOAD_FAILED_PLAYLIST);
	}

	countPlaylistSongs();
	playlistGet(playlistLineNumber);
}
