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
// VGMSLAP.H - Main source file
//
///////////////////////////////////////////////////////////////////////////////

#ifndef VGMSLAP_H
#define VGMSLAP_H

#include <stdio.h>

#include "types.h"

#define VGMSLAP_VERSION "R4"

///////////////////////////////////////////////////////////////////////////////
// Function declarations
///////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv);				// Main program loop
void initPlayback(void);						// Prepare for and start playback state
void killProgram(ProgramExitCode errorCode);    // End program with message/error code

///////////////////////////////////////////////////////////////////////////////
// Variable declarations
///////////////////////////////////////////////////////////////////////////////

// General program vars
extern ProgramState programState;	// Controls current state of program (init, main loop, exit, etc)

// File-related vars
extern char* fileName;				// Filename from argument
extern FILE *initialFilePointer;	// Pointer to initially passed file (playlist or VGM)

#endif
