/*
 *  The Castles of Dr. Creep 
 *  ------------------------
 *
 *  Copyright (C) 2009-2014 Robert Crossfield
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *  ------------------------------------------
 *  Game
 *  ------------------------------------------
 */

#include "stdafx.h"
#include "graphics/screenSurface.h"
#include "vic-ii/screen.h"
#include "vic-ii/sprite.h"
#include "playerInput.h"
#include "castle/castle.h"
#include "castle/objects/object.hpp"
#include "castleManager.h"
#include "creep.h"
#include "sound/sound.h"
#include "builder.hpp"

#include "debug.h"

#ifdef WIN32
#include <fcntl.h>
#include <io.h>
#endif

#ifdef _WII
	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>
	#include <sys/timeb.h>
#endif

sObjectData mObjectFuncData[6] = {	0x31F6, 0x3534, 0x34EF, 0x01, 0x00,	// Player
									0x3639, 0x3682, 0x0000, 0x00, 0x04, // Lightning
									0x36B3, 0x374F, 0x0000, 0x00, 0x03, // Forcefield
									0x379A, 0x3940, 0x38CE, 0x01, 0x02, // Mummy
									0x3A08, 0x0000, 0x3A60, 0x00, 0x04, // RayGun Laser
									0x3AEB, 0x3DDE, 0x3D6E, 0x01, 0x00  // Frankie
								 };

sObjectImgData mObjectImageFuncData[16] = { 0x3FD5, 0x4075,		// Door
											0x0000, 0x41D8,		// Door Button
											0x42AD, 0x0000,		// Lightning
											0x0000, 0x44E7,		// Lightning Switch
											0x45E0, 0x4647,		// Forcefield Timer
											0x475E, 0x47A7,		// Mummy
											0x0000, 0x4990,		// Key
											0x0000, 0x4A68,		// Door Lock
											0x4B1A, 0x0000,		// Ray Gun
											0x0000, 0x4D70,		// Ray Gun Control
											0x4E32, 0x4EA8,		// Teleport
											0x50D2, 0x0000,		// Trapdoor Switch
											0x0000, 0x0000,		// 
											0x538B, 0x548B,		// Conveyor
											0x0000, 0x5611,		// Conveyor Control
											0x0000, 0x0000		//
										  };
cCreep::cCreep() {
	size_t romSize;
	mWindowTitle = "The Castles of Dr. Creep";

	mLevel = 0;
	mMenuMusicScore = 0xFF;
	mUnlimitedLives = 0;
	mQuit = false;
	mNoInput = false;

	// Prepare memory for use
	mMemorySize = 0x10000;
	mMemory = new byte[ mMemorySize ];
	for( size_t x = 0; x < mMemorySize; ++x )
		mMemory[x] = 0;

	mDebug = new cDebug();
	mCastleManager = new cCastleManager();
	mInput = new cPlayerInput( this );
	mScreen = new cScreen( this, mWindowTitle );
	mSound = 0;

	// Load the C64 Character Rom
	if( (m64CharRom = local_FileRead( "char.rom", "",  romSize, false )) == 0 ) {
		mDebug->error("File \"char.rom\" not found");
		return;
	}

	if( (mGameData = mCastleManager->fileLoad( "OBJECT", romSize )) == 0 ) {
		mDebug->error("File \"OBJECT\" not found");
		return;
	}

    mGameData += 2;
	romSize -= 2;

	if(romSize)
		// Copy the game data into the memory buffer
		memcpy( &mMemory[ 0x800 ], mGameData, romSize );

	mDisableSoundEffects = 0;

	byte_8C0 = 0x80;
	byte_8C1 = 0x40;

	mRestorePressed = false;

	mMusicPlaying = 0x00;
	mPlayingSound = -1;
	mSaveGameLoaded = 0x00;
	mEngine_Ticks = 0xA0;

	mCurrentPlayer = 0xBA;

	byte_4D60 = 0x80;
	byte_4D61 = 0x40;
	byte_4D62 = 0x20;
	byte_4D63 = 0x10;
	byte_4D64 = 0x08;
	byte_4D65 = 0x04;
	byte_4D66 = 0x02;
	byte_4D67 = 0x01;

	byte_5389 = 0x80;
	byte_538A = 0x01;

	byte_5642 = 0x80;
	byte_5643 = 0x20;
	byte_5644 = 0x10;
	byte_5645 = 0x08;
	byte_5646 = 0x04;
	byte_5647 = 0x02;
	byte_5648 = 0x01;

	byte_574C = 0x80;
	byte_574D = 0x04;
	byte_574E = 0x02;
	byte_574F = 0x01;	


	mBuilder = 0;
	mCastle = 0;
	mJoyButtonState = 0xA0;

	mMusicCurrent = "MUSIC0";
	mMusicBuffer = 0;
	mMusicBufferStart = 0;
	mMusicBufferSize = 0;

	mMenuReturn = false;
	mTicksPrevious = SDL_GetTicks();

	mPlayerStatus[0] = mPlayerStatus[1] = false;
}

cCreep::~cCreep() {

	delete mSound;
	delete m64CharRom;
	delete mMemory;
	delete mCastleManager;
	delete mScreen;
	delete mInput;
	delete mDebug;
	delete mBuilder;
}

void cCreep::builderStart( int pStartLevel ) {
	bool	run = true;

	mBuilder = new cBuilder( this );
	mBuilder->mSoundSet( mSound );

	mScreen->roomNumberSet(0);
	mBuilder->start(pStartLevel, false);

	for(; mBuilder->mTestGet(); ) {

		// Copy the castle from editor memory to game memory
		memcpy( &mMemory[0x7800], mBuilder->memory( 0x7800 ), 0x2000 );
		memcpy( &mMemory[0x9800], mBuilder->memory( 0x7800 ), 0x2000 );

		// Run the level
		mIntro = false;
		mainLoop();	

		// Return to the editor
		mQuit = false;
		mBuilder->mStartCastle = -2;
		mBuilder->mainLoop();
	}

	// Remove screen/sound ptr so builder doesnt delete it
	mBuilder->screenSet(0);
	mBuilder->mSoundSet(0);

	// Delete Builder
	delete mBuilder;
	mBuilder = 0;

	// Set the screen back
	mScreen->clear(0);
	mScreen->windowTitleSet(mWindowTitle);
	mScreen->roomNumberSet(-1);

	// Disable the sprites
	mScreen->spriteDisable();

	// Set the screen ptrs
	mScreen->bitmapLoad( &mMemory[ 0xE000 ], &mMemory[ 0xCC00 ], &mMemory[ 0xD800 ], 0 );
}

void cCreep::titleDisplay() {
	size_t size;
	byte *buffer, *introImage = buffer = mCastleManager->introLoad( size );

	if(!buffer) {
        mDebug->error( "File \"PIC A TITLE\" not found" );
		return;
	}

	// Skip load address
	buffer += 0x02;	

	mScreen->bitmapLoad( buffer,  buffer + 0x1F40, buffer + 0x2328, 1 );
	mScreen->refresh();
	hw_IntSleep(0x10);

    delete introImage;
}

void cCreep::run( int pArgCount, char *pArgs[] ) {
	bool consoleShow = false;

	int	count = 0;
	size_t	playLevel = 0;
	bool	playLevelSet = false;
	bool	unlimited = false;

#ifndef _WII

	// Output console message
	cout << "The Castles of Dr. Creep (" << VERSION << ")" << endl << endl;
	cout << "http://creep.sourceforge.net/" << endl << endl ;

	// Check command line parameters
	while( count < pArgCount ) {
		string arg = string( pArgs[count] );

		if( arg == "-c")
			consoleShow = true;
		
		if( arg == "-u") {
			cout << " Unlimited Lives enabled." << endl;
			unlimited = true;
		}

		if( arg == "-l" )
			playLevelSet = true;

		++count;
	}

	// Level selection was requested
	if(playLevelSet) {
		mCastleManager->castleListDisplay();

		string lvl;
		cout << "\nPlease select a Castle: ";

		cin >> lvl;

		playLevel = atoi( lvl.c_str() );
	}

	// Level numbers begin at 1 in the list, but 0 in the actual game
	if(playLevel)
		--playLevel;
#endif

#ifdef _DEBUG
	consoleShow = true;
#endif
	// Hide the console unless its requested by user
	if( !consoleShow )
        mDebug->consoleHide();

	// Set the default screen scale
	mScreen->scaleSet( 2 );
	mScreen->windowTitleUpdate();

	// Display the title screen
#ifndef _DEBUG
	titleDisplay();
#endif

	mStartCastle = playLevel;

	// Start main loop
	start( playLevel, unlimited );
}

void cCreep::interruptWait( byte pCount) {
	// Screen Refresh occurs 50 times per second on a PAL C64
	// and 60 times on an NTSC
	// Meaning the interrupt should fire once every
	// 0.01666666666666666666666666666667 seconds on an NTSC C64
	// and 0.02 seconds on a PAL C64

	double diffMil = (SDL_GetTicks() - mTicksPrevious);
	double	sleepTime = 20;	// Milliseconds

	if( diffMil > sleepTime) 
		sleepTime -= diffMil;

	if( sleepTime < 0 ) {
		if(sleepTime < -20 )
			sleepTime = 0;
		else 
			sleepTime = -sleepTime;
	}

	Sleep( (dword) sleepTime * pCount );

	mTicksPrevious = SDL_GetTicks();
}

//08C2
void cCreep::start( int pStartLevel, bool pUnlimited ) {
	byte	byte_30, byte_31, count;

	mQuit = false;
	mStartCastle = pStartLevel;

	byte_30 = 0x40;
	byte_31 = 0xD7;
	
	if(pUnlimited)
		mUnlimitedLives = 0xFF;

	if(!mSound)
		mSound = new cSound( this );

	for( count = 0xC8; ;) {
		
		mMemory[0xBC00 + count] = byte_30;
		mMemory[0xBB00 + count] = byte_31;

		++count;
		if(count == 0xC8)
			break;

		if( (count & 7) ) {
			++byte_30;
			if(byte_30)
				continue;

			++byte_31;
			
		} else {
			if((byte_30 + 0x39) > 0xFF)
				++byte_31;

			byte_30 += 0x39;
			++byte_31;
		}
	}

	// 0x08F9
	byte_30 = byte_31 = 0;

	for( count = 0; count < 0x20; ++count ) {
		mMemory[ 0x5CE6 + count ] = byte_30;
		mMemory[ 0x5D06 + count ] = byte_31;

		if( (word) (byte_30 + 0x28) > 0xFF)
			++byte_31;

		byte_30 += 0x28;
	}

    /** Copy Protection
      *
	  * Removed: This is not necessary as the original copy protection 
      * affects the various element function addresses (which are not used/needed here, and wouldnt work even if i tried)
      */
	// 0x091B
	//copyProtection();
	optionsMenuPrepare();

	if( mMemory[ 0x839 ] != 1 ) {

		for( word_30 = 0x7572, word_32 = 0x77F7; word_30 < 0x7800; word_30 += 0x1, word_32 += 0x1)
			mMemory[ word_30 ] = mMemory[ word_32 ];
		
		if( pStartLevel > -1 )
			if( !ChangeLevel( 0x10 + (pStartLevel * 4) ) )
				return;

	} else {
		// 0x953

	}

	mainLoop();
}

// 0x7572
void cCreep::optionsMenuPrepare() {
	word byte_30;

	byte_30 = 0x400;

	for( signed char X = 3; X >= 0; --X ) {
		for( word Y = 0; Y <= 0xFF; ++Y )
			mMemory[ byte_30 + Y ] = 0x20;

		byte_30 += 0x100;
	}

	word byte_3E;

	mFileListingNamePtr = 0;

	byte_3E = 0x7760;

	//0x759C
	for(;;) {
		byte Y = 0;
		word A = mMemory[ byte_3E ];
		if(A == 0xFF)
			break;

		A = mMemory[ byte_3E + 2 ];
		if(A != 0xFF) {

			byte X = mFileListingNamePtr;

			for( Y = 0; Y < 3; ++Y ) {
				
				mMemory[ 0xBA00 + X ] = mMemory[ byte_3E + Y];
				++X;
			}

			mFileListingNamePtr = ++X;
		}

		// 75C0
		byte X = mMemory[ byte_3E + 1 ];
		byte_30 = mMemory[ 0x5CE6 + X ];
		
		//
		A = mMemory[ 0x5D06 + X ] + 4;
		byte_30 = (A << 8) + (byte_30 & 0xFF);

		//
		A = (byte_30 & 0xFF) + mMemory[ byte_3E ];
		byte_30 = A + (byte_30 & 0xFF00);

		
		//0x75E1
		Y = 0;
		byte_3E += 3;
		
		for(;;) {
			A = mMemory[ byte_3E + Y] & 0x3F;
			mMemory[ byte_30 + Y ] = (byte) A;
			A = mMemory[ byte_3E + Y];

			if( A & 0x80 )
				break;
			else
				++Y;
		}

		// 75FC
		++Y;
		byte_3E += Y;
	}

	// 760A
	byte x = mMemory[ 0xBA09 ];

	byte A = mMemory[ 0x5CE6 + x ];
	byte_30 = (A & 0xFF) | (byte_30 & 0xFF00);

	A = 4 + mMemory[ 0x5D06 + x];
	byte_30 = (A << 8) | (byte_30 & 0xFF);
	byte Y = mMemory[ 0xBA08 ];
	Y -= 2;

	mMemory[ byte_30 + Y ] = 0x3E;

	x = 7;
	A = mMemory[ 0x5CE6 + x ];
	byte_30 = A & 0xFF;

	// 762F
	byte_30 = ((4 + mMemory[ 0x5D06 + x ]) << 8) | (byte_30 & 0xFF);
	
	for( byte Y = 0x17; Y < 0x1A; ++Y ) {
		mMemory[ byte_30 + Y ] |= 0x80;
	}

	mMemory[ 0x775F ] = 0x0C;
	mMemory[ 0x775E ] = 0x03;

	// Now do available castle names
	for(int curCastle = 0; curCastle < 0x18; ++curCastle) {
		word X = mFileListingNamePtr;
		cCastleInfo *castle = mCastleManager->castleInfoGet( curCastle );

		if( !castle )
			break;

		mMemory[ 0xBA00 + X ] = mMemory[ 0x775E ];
		mMemory[ 0xBA01 + X ] = mMemory[ 0x775F ];

		mMemory[ 0xBA02 + X ] = 2;

        // 
		X = mMemory[ 0x775F ];

        //
		A = mMemory[ 0x5D06 + X ] + 4;

        word_32 = mMemory[ 0x5CE6 + X ];
		word_32 = (A << 8) + (word_32 & 0xFF);

        // 
		X = mMemory[ 0x775E ];
		X -= 0;
		
        // 
		X += word_32 & 0xFF;
        
        // Castle Name destination
		word_32 = X + (word_32 & 0xFF00);
		
        // Loop the castle name, copying it to 'game memory'
		for( size_t counter = 0; counter < castle->nameGet().length(); ++counter, ++word_32 )
			mMemory[ word_32 ] = toupper(castle->nameGet()[ counter ]) & 0x3F;

		mFileListingNamePtr += 4;

		if(mMemory[ 0x775E ] == 3)
			mMemory[ 0x775E ] = 0x16;
		else {
			mMemory[ 0x775E ] = 3;
			++mMemory[ 0x775F ];

			if(mMemory[ 0x775F ] >= 0x18)
				break;
		}
	}
	
	mMemory[ 0x239A ] = mFileListingNamePtr - 4;

	mFileListingNamePtr = 0x08;
	sub_2973();
}

// 268F: 
void cCreep::textShow() {
	mRestorePressed = false;

	mTextColor = mMemory[ 0x278A ];
	mTextFont = mMemory[ 0x278B ] | 0x20;
	mTextXPos = mMemory[ 0x2788 ];
	mTextYPos = mMemory[ 0x2789 ];

	mMemory[ 0x27A2 ] = 0x2D;
	byte X = mMemory[ 0x278C ];
	
	while( X ) {
		
		textPrintCharacter();
		--X;
		mTextXPos += 0x08;
	}
	
	mStrLength = X;

	// 26D0
	for(;;) {
		hw_Update();

		if( mStrLength != mMemory[ 0x278C ] ) {
			++mMemory[ 0x27A3 ];

			X = mMemory[ 0x27A3 ] & 3;
			mMemory[ 0x27A2 ] = mMemory[ 0x27A4 + X ];
			mTextXPos = (mStrLength << 3) + mMemory[ 0x2788 ];
			textPrintCharacter();
		}

		// 26F7
		byte A = textGetKeyFromUser();
		if( A == 0x80 || mInput->restoreGet() == true ) {
			if( mRestorePressed == false &&  mInput->restoreGet() != true) {

				interruptWait( 3 );
				continue;
			} else {
				// 2712
				mStrLength = 0;
				
				// Wait for restore key
				do { 
					interruptWait(3);
					mInput->inputCheck( true );

				} while( !mInput->restoreGet() );
				
				return;
			}
		} else {
			// 2730 
			if( A == 8 ) {
				if( mStrLength != mMemory[ 0x278C ] ) {
					mMemory[ 0x78A2 ] = 0x2D;
					textPrintCharacter();
				}
				// 2744
				if(mStrLength)
					--mStrLength;
				continue;

			} else {
				// 274F
				if( A == 0x0D )
					return;
				
				if( mStrLength == mMemory[ 0x278C ] )
					continue;
				
				mMemory[ 0x278E + mStrLength ] = A;
				++mStrLength;

				mMemory[ 0x27A2 ] = A;
				textPrintCharacter();
			}
		}
	}
}

// 2772:
void cCreep::textPrintCharacter() {
	mMemory[ 0x27A2 ] |= 0x80;

	mObjectPtr = 0x27A2;
	stringDraw();
}

// 27A8
byte cCreep::textGetKeyFromUser() {
	byte key = mInput->keyRawGet();
	
	if( key == 0 )
		return 0x80;

	return toupper( key );
}

// 2973: 
void cCreep::sub_2973() {

	// Removed sleep loop
	sub_95F();
}

// 95F: 
void cCreep::sub_95F() {
	byte A = 0x20;

	// Sprite??
	for( byte X = 0; X < 8; ++X ) {
		mMemory[ 0x26 + X ] = A;
		++A;
	}

	mMemory[ 0x21 ] = 0;
	mScreen->spriteDisable();

	interruptWait( 2 );

	mPlayingSound = -1;
}

// 0B84
void cCreep::mainLoop() {

	mScreen->bitmapLoad( &mMemory[ 0xE000 ], &mMemory[ 0xCC00 ], &mMemory[ 0xD800 ], 0 );
	
	// Main Intro/Game Loop
	while(!mQuit) {
	
		if( Intro() == true )
			continue;
		
		if(mQuit)
			break;

		Game();
	}

}

// 18E4
void cCreep::screenClear() {

	mScreen->clear(0);

	// Disable all sprites
	mScreen->spriteDisable();

	// Clear screen memory
	for( word word_30 = 0xFFFF; word_30 >= 0xE000; --word_30 )
		mMemory[ word_30 ] = 0;

	for( byte Y = 0; Y != MAX_SPRITES; ++Y )
		mRoomSprites[Y].state = SPR_FLAG_FREE;

	for( word word_30 = 0xC000; word_30 < 0xC800; word_30++ ) 
		mMemory[word_30] = 0;
	
	mObjectCount = 0;
	mScreen->screenRedrawSet();
	mScreen->bitmapRedrawSet();
}

// 13F0
void cCreep::roomLoad() {
	
	screenClear();

	word_30 = 0xC000;

	while( word_30 < 0xC800)
		mMemory[word_30++] = 0;

	byte X, A;

	// 1411
	if( mPlayerStatus[0] != 1 )
		X = 1;
	else
		X = 0;

	if( mIntro )
		A = mMenuScreenCount;
	else
		A = mMemory[ 0x7809 + X ];

	roomPtrSet( A );
	
	// Room Color
	graphicRoomColorsSet( mMemory[mRoomPtr] & 0xF );

	//14AC
	// Ptr to start of room data
	mObjectPtr = readLEWord( &mMemory[mRoomPtr + 6] );

	if(mIntro)
		mObjectPtr += 0x2000;

	// Function ptr
	roomPrepare( );
}

void cCreep::graphicRoomColorsSet( byte pRoomColor ) {
	// Set floor colours
	// 1438
	*memory( 0x6481 )= pRoomColor;
	pRoomColor <<= 4;
	pRoomColor |= *memory( 0x6481 );

	*memory( 0x6481 ) = pRoomColor;
	*memory( 0x648E ) = pRoomColor;
	*memory( 0x649B ) = pRoomColor;
	*memory( 0x65CC ) = pRoomColor;
	*memory( 0x65CE ) = pRoomColor;
	*memory( 0x6EAE ) = pRoomColor;
	*memory( 0x6EAF ) = pRoomColor;
	*memory( 0x6EB0 ) = pRoomColor;
	*memory( 0x6EC6 ) = pRoomColor;
	*memory( 0x6EC7 ) = pRoomColor;
	*memory( 0x6EC8 ) = pRoomColor;
	*memory( 0x6EDB ) = pRoomColor;
	*memory( 0x6EDC ) = pRoomColor;
    *memory( 0x6EDD ) = pRoomColor;
	*memory( 0x6EED ) = pRoomColor;
	*memory( 0x6EEE ) = pRoomColor;
	*memory( 0x6EEF ) = pRoomColor;
	*memory( 0x6EFC ) = pRoomColor;
	*memory( 0x6EFD ) = pRoomColor;
	*memory( 0x6EFE ) = pRoomColor;
	*memory( 0x6F08 ) = pRoomColor;
	*memory( 0x6F09 ) = pRoomColor;
	*memory( 0x6F0A ) = pRoomColor;

	//1487

	for( signed char Y = 7; Y >= 0; --Y ) {
		*memory( 0x6FB2 + Y ) = pRoomColor;
		*memory( 0x6FF5 + Y ) = pRoomColor;
		*memory( 0x7038 + Y ) = pRoomColor;
		*memory( 0x707B + Y ) = pRoomColor;
	}
	
	pRoomColor &= 0x0F;
	pRoomColor |= 0x10;
	*memory( 0x6584 ) = pRoomColor;

    byte A = (*memory( 0x649B ) & 0xF0);
    A |= 0x01;

	*memory( 0x659B ) = A;
    *memory( 0x65CD ) = A;
}

// 15E0
void cCreep::roomPrepare( word pAddress ) {
	word func = 0x01;

	if(pAddress)
		mObjectPtr = pAddress;

	while(func) {

		func = readLEWord( &mMemory[ mObjectPtr ]);
		mObjectPtr  += 2;

		switch( func ) {
			case eObjectNone:			// Finished
				return;

			case eObjectDoor:				// Doors
				obj_Door_Prepare( );
				break;

			case eObjectWalkway:			// Walkway
				obj_Walkway_Prepare( );
				break;

			case eObjectSlidingPole:		// Sliding Pole
				obj_SlidingPole_Prepare( );
				break;

			case eObjectLadder:				// Ladder
				obj_Ladder_Prepare( );
				break;

			case eObjectDoorBell:			// Doorbell
				obj_Door_Button_Prepare( );
				break;

			case eObjectLightning:			// Lightning Machine
				obj_Lightning_Prepare( );
				break;

			case eObjectForcefield:			// Forcefield
				obj_Forcefield_Prepare( );
				break;

			case eObjectMummy:				// Mummy
				obj_Mummy_Prepare( );
				break;

			case eObjectKey:				// Key
				obj_Key_Load( );
				break;

			case eObjectLock:				// Lock
				obj_Door_Lock_Prepare( );
				break;

			case eObjectRayGun:				// Ray Gun
				obj_RayGun_Prepare( );
				break;

			case eObjectTeleport:			// Teleport
				obj_Teleport_Prepare( );
				break;

			case eObjectTrapDoor:			// Trap Door
				obj_TrapDoor_Prepare( );
				break;

			case eObjectConveyor:			// Conveyor
				obj_Conveyor_Prepare( );
				break;

			case eObjectFrankenstein:		// Frankenstein
				obj_Frankie_Load( );
				break;

			case eObjectText:		// String Print
			case 0x2A6D:
				obj_stringPrint();
				break;

			case eObjectImage:
				obj_Image_Draw();
				break;

			case eObjectMultiDraw:
			case 0x160A:				// Intro
				obj_MultiDraw( );
				break;

			default:
				cout << "roomPrepare: 0x";
				cout << std::hex << func << "\n";

				break;
		}

	}

}

// B8D
bool cCreep::Intro() {

	mMenuMusicScore = 0;
	mMenuScreenTimer = 3;
	mMenuScreenCount = 0;
	mIntro = true;
	byte_D10 = 0;

    // The intro screen loop
	do {
		++mMenuScreenTimer;
		
		mMenuScreenTimer &= 3;

		if( mMenuScreenTimer != 0 ) {
			++mMenuScreenCount;
			roomPtrSet( mMenuScreenCount );

			if( ((*level( mRoomPtr )) & 0x40) )
				mMenuScreenCount = 0;

			roomLoad();
		} else {
			mObjectPtr = 0x0D1A;

			screenClear();
			roomPrepare( );
		}

		// 0BE1 
		if( mMusicPlaying != 1 ) {
		
			if( !mMenuScreenTimer ) {
				if( mMenuMusicScore == 0 )
					++mMenuMusicScore;
				else {
					// 0C49
					musicChange();
					mMusicPlaying = 1;
					return true;
				}
			}
		}

		// Wait for input, or timeout (continues to next screen)
		for(byte byte_D12 = 0xC8; byte_D12; --byte_D12) {

			if( mMenuScreenTimer )
				events_Execute();
			else {
				// C0D
				interruptWait( 2 );
			}

			hw_Update();

			// C17
			KeyboardJoystickMonitor( byte_D10 );
			if( mJoyButtonState )
				break;

			// Quit the game if restore (Escape) is pressed
			if( mRestorePressed ) {
				mRestorePressed = false;
				mQuit = true;
				return false;
			}

			// Change which player controller is checked
			byte_D10 ^= 0x01;

			// Display Highscores on F3
			if( mInput->f3Get() ) {
				sub_95F();
				gameHighScores();

				mObjectPtr = 0x239C;
				obj_stringPrint();

				*memory(0x278C) = 0;
				textShow();

				return true;
			}

			if( mRunStopPressed == true ) {
				optionsMenu();
				if( mSaveGameLoaded == 1 ) {
					mJoyButtonState = 1;
					break;
				}

				return true;
			}
		}

	} while( !mJoyButtonState );

	// 0CDD
	mMusicPlaying = 0;
	mIntro = 0;
	mMusicBuffer = 0;

	// Disable music playback
	mSound->playback( false );

	signed char X = 0x0E;

	while(X >= 0) {
		mMemory[ 0x20EF + X ] &= 0xFE;
		mMemory[ 0xD404 + X ] = mMemory[ 0x20EF + X ];
		mSound->sidWrite(0x04 + X, mMemory[ 0x20EF + X ]);

		X -= 0x07;
	}

	mInput->inputCheck(true);
	return false;
}

// 20A9 : 
void cCreep::musicPtrsSet() {
	byte X = (*memory( 0x20CB ) & 3);
	
	mVoiceNum = X;

	X <<= 1;

	mVoice = readLEWord( memory( 0x20DF + X));
	mVoiceTmp = readLEWord( memory( 0x20E5 + X));
}

// 1F29 : Play Music Buffer
bool cCreep::musicBufferFeed() {
	bool done = false;
	
	if( *memory( 0x20DC ) == 0 ) {
		if( *memory( 0x20DD ) == 0 )
			goto musicUpdate;
		
		(*memory( 0x20DD ))--;
	}

	(*memory( 0x20DC ))--;
	if( (*memory( 0x20DC ) | *memory( 0x20DD )) )
		return true;
	
musicUpdate:;

	for(; !done ;) {
		if(!mMusicBuffer)
			return false;

		byte A = *mMusicBuffer;
		A >>= 2;

		byte X;

		X = *memory( 0x20D2 + A );

		for( signed char Y = X - 1; Y >= 0; --Y )
			*memory( 0x20CB + Y ) = *(mMusicBuffer + Y);

		// 1F60

		mMusicBuffer += X;

		A = *memory( 0x20CB );
		A >>= 2;
		switch( A ) {
			case 0:
				musicPtrsSet();
				
				X = *memory( 0x20CB ) & 3;

				X = *memory( 0x20CC ) + *memory( 0x2104 + X );
				A = *memory( 0x2108 + X );

				mSound->sidWrite( (mVoiceNum * 7), A );
				*memory( mVoiceTmp ) = A;

				A = *memory( 0x2168 + X );
				mSound->sidWrite( (mVoiceNum * 7) + 1, A );
				*memory( mVoiceTmp + 1) = A;
			
				A = *memory( mVoiceTmp + 4 );
				A |= 1;
				mSound->sidWrite( (mVoiceNum * 7) + 4, A );
				*memory( mVoiceTmp + 4 ) = A;


				continue;

			case 1:
				musicPtrsSet();
				
				A = *memory( mVoiceTmp + 4 );
				A &= 0xFE;

				*memory( mVoiceTmp + 4 ) = A;
				mSound->sidWrite( (mVoiceNum * 7) + 4, A );
				
				continue;

			case 2:
				*memory( 0x20DC ) = *memory( 0x20CC );
				return true;

			case 3:
				*memory( 0x20DD ) = *memory( 0x20CC );
				return true;

			case 4:
				musicPtrsSet();
				
				for( signed char Y = 2; Y < 7; ++Y ) {
					if( Y != 4 ) {
						// 1FDD
						byte A = *memory( 0x20CA + Y );
						mSound->sidWrite( (mVoiceNum * 7) + Y, A );
						*memory( mVoiceTmp + Y ) = A;

					} else {
						// 1FE7
						byte A = *memory( mVoiceTmp + Y );
						A &= 1;
						A |= *memory( 0x20CA + Y );
						mSound->sidWrite( (mVoiceNum * 7) + Y, A );
						*memory( mVoiceTmp + Y ) = A;
					}
				}

				continue;

			case 5:
				continue;

			case 6:
				X = *memory( 0x20CB ) & 3;
				*memory( 0x2104 + X ) = *memory( 0x20CC );

				continue;

			case 7:
				A = (*memory(0x2103) & 0xF0) | *memory(0x20CC);
				*memory(0x2103) = A;
				
				mSound->sidWrite( 0x18, A );
				continue;

			case 8:
				A = *memory(0x20CC);
				*memory(0x2107) = A;
				A <<= 2;
				A |= 3;

				*memory(0xDC05) = A;
				continue;

			default:
				done = true;
				break;
		}

	}	// for
	
	if( !mIntro ) {
		mPlayingSound = -1;
		mMusicBuffer = 0;
		
	} else {
		mMusicBuffer = mMusicBufferStart;
		*memory( 0x20DD ) = 0x02;
		
		byte A = (*memory( 0x2102 ) & 0xF0);
		*memory( 0x2102 ) = A;
		mSound->sidWrite( 0x17, A );
	}

	return true;
}

// C49 : Music Change
void cCreep::musicChange() {
	vector<string>				musicFiles = mCastleManager->musicGet();
	vector<string>::iterator	musicIT;
	bool						ok = false;

	if(mMusicCurrent == "")
		ok = true;

	for( musicIT = musicFiles.begin(); musicIT != musicFiles.end(); ++musicIT ) {
		
		// Found current track?
		if( (*musicIT).compare( mMusicCurrent ) == 0 ) {
			ok = true;
			continue;
		}

		if(ok) 
			break;
	}

	if( (!ok) || musicIT == musicFiles.end()) {
		mMusicCurrent = "";
		return;
	}
	
	mMusicBufferSize = 0;

	mMusicCurrent = *musicIT;
	mMusicBuffer = mCastleManager->fileLoad( mMusicCurrent, mMusicBufferSize ) + 4;		// Skip PRG load address, and the first 2 bytes of the real-data
	mMusicBufferStart = mMusicBuffer;

	byte A = 0;

	//0C9A
	for( signed char X = 0x0E; X >= 0; X -= 7 ) {
		A = *memory( 0x20EF + X );

		A &= 0xFE;
		*memory( 0x20EF + X ) = A;
		mSound->sidWrite( 0x04 + X, A );
	}

	A = *memory( 0x2102 ) & 0xF0;
	mSound->sidWrite( 0x17, A );

	*memory( 0x2102 ) = A;
	*memory( 0x20DC ) = 0;
	*memory( 0x20DD ) = 0;
	*memory( 0x2107 ) = 0x14;
	*memory( 0x20DE ) = 1;

	*memory(0xDC05) = (0x14 << 2) | 3;

	mSound->playback( true );
}

// 0x2233 : Intro Menu
void cCreep::optionsMenu() {
s2238:
	for( ;; ) {
		mScreen->spriteDisable();

		// 0x2257: Set the color of all text positions to WHITE
		for(word_30 = 0xD800; word_30 < 0xDC00; word_30 += 0x100) {

			for( word Y=0 ;Y != 0x100; ++Y ) 
				mMemory[ word_30 + Y ] = 1;
		}
		
		// Decode the background as text
		mScreen->drawStandardText( memory( 0x400 ), 0x1000, memory( 0xD800 ));

        // 
        // 0x226E: Options Menu Loop
		for(;;) {
		    
            // Do hardware update/sleep
			hw_Update();
			hw_IntSleep(1);

            // Check for input
			KeyboardJoystickMonitor(0);

			// Leave menu if restore (escape) was pressed
			if( mRestorePressed ) {
				mRestorePressed = false;

				sub_95F();
				return;
			}

            // No joystick button?
			if( !mJoyButtonState ) {
				
				if( byte_5F56 & 0xFB )
					continue;
				
				// 22F7
				// 
				byte X = mFileListingNamePtr;
				byte Y = mMemory[ 0xBA01 + X ];
				
				word_30 = mMemory[ 0x5CE6 + Y ];
				word_30 += (mMemory[ 0x5D06 + Y ] + 4) << 8;
				
				Y = mMemory[ 0xBA00 + X ] - 2;
				
				mMemory[ word_30 + Y ] = 0x20;

				byte A = byte_5F56;
				if(A) {
					A = mFileListingNamePtr;

					if( mFileListingNamePtr == mMemory[ 0x239A ] )
						A = 0;
					else
						A += 4;

				} else {
					// 22B5
					A = mFileListingNamePtr;
					if( A )
						A -= 4;
					else
						A = mMemory[ 0x239A ];

				}

				// 22C3
				mFileListingNamePtr = A;
				X = A;

				Y = mMemory[ 0xBA01 + X ];
				A = mMemory[ 0x5CE6 + Y ];

				word_30 = A;
				word_30 |= ((mMemory[ 0x5D06 + Y ] + 4) << 8);
				
				Y = (mMemory[ 0xBA00 + X ] - 2);

				mMemory[ word_30 + Y ] = 0x3E;
				
			} else {
				// Button Press
				// 22E5
				byte X = mFileListingNamePtr;

                // Selected Menu Option
				byte A = mMemory[ 0xBA02 + X ];
                switch( A ) {

                case 0: {
                    //  22ED: Unlimited Lives Toggle
					mUnlimitedLives ^= 0xFF;

					byte Y = mMemory[ 0xBA01 + X ];
					word_30 = mMemory[ 0x5CE6 + Y ];
					word_30 |= (mMemory[ 0x5D06 + Y ] + 4) << 8;

					A = mMemory[ 0xBA00 + X ];
					A += 0x11;
					Y = A;
					for(X = 0; X < 6; ++X) {
						if( X != 2 ) {
							mMemory[word_30 + Y] ^= 0x80;
						}
						++Y;
					}
                    break;
                        }

                case 1:{
                default:
                   //inputWait();
				    sub_95F();
				    return;
                       }

                case 2:{
                    X = mFileListingNamePtr;
					ChangeLevel(X);
                    break;
                       }

                case 3:{
                    gamePositionLoad();
						
				    if( mMemory[ 0x24FD ] != 1 )
						continue;

                    break;
                       }

                case 4:{
                    if( mMemory[ 0x2399 ] == 0xFF )
						continue;

					sub_95F();
					gameHighScores();

					// Draw 'Press Enter to exit'
					mObjectPtr = 0x239C;
					obj_stringPrint();

					mMemory[ 0x278C ] = 0;
					textShow();
						
					goto s2238;
                       }
                }

			}// Button Pressed
			
			// Decode the background as text
			mScreen->drawStandardText( memory( 0x400 ), 0x1000, memory( 0xD800 ));
		}
	}
}

void cCreep::menuUpdate( size_t pCastleNumber ) {
	byte X = pCastleNumber;
	if(mMemory[ 0x2399 ] == X)
		return;

	byte Y = mMemory[ 0xBA01 + X ];
	word_30 = mMemory[ 0x5CE6 + Y ];
	word_30 |= ((mMemory[ 0x5D06 + Y ] | 4) << 8);

	word_30 += mMemory[ 0xBA00 + X ];

	mMemory[ 0x24A6 ] = X;

	// 2466

	X = mMemory[ 0x2399 ];
	
	Y = mMemory[ 0xBA01 + X ];
	word_32 = mMemory[ 0x5CE6 + Y ];
	word_32 |= ((mMemory[0x5D06 + Y ] | 4) << 8);

	word_32 += mMemory[ 0xBA00 + X ];

	// 2484
	X = mMemory[ 0x24A6 ];

	for( char Y = 0x0F; Y >= 0; --Y ) {

		if( mMemory[ 0x2399 ] != 0xFF )
			mMemory[ word_32 + Y] &= 0x7F;
			
		mMemory[ word_30 + Y] |= 0x80;
	}

	mMemory[ 0x2399 ] = X;
}

// 5EFC
void cCreep::KeyboardJoystickMonitor( byte pA ) {
	byte X = 0xFF, A = 0;
	if(mNoInput)
		return;

	byte_5F58 = pA;
	mRunStopPressed = false;

	sPlayerInput *input = mInput->inputGet( pA );

	// Pause the game, or enter the options menu
	if( mInput->runStopGet() )
		mRunStopPressed = true;

	// Start the editor, using the current castle
	if( mInput->f4Get() ) {

		if( !mBuilder ) {
			builderStart( mStartCastle );
			input->clear();
		}
	}

	// Start the editor, with a new castle
	if( mInput->f5Get() ) {
		
		if( !mBuilder ) {
			builderStart( -1 );
			input->clear();
		}
	}

	// Kill the player(s) if the restore key is pressed
	if( mInput->restoreGet() )
		mRestorePressed = true;

	if(input) {

		if( input->mButton )
			A = 1;

		if( input->mLeft )
			X ^= 0x04;
	
		if( input->mRight )
			X ^= 0x08;

		if( input->mUp )
			X ^= 0x01;

		if( input->mDown )
			X ^= 0x02;
	}

	X &= 0x0F;
	byte_5F56 = mMemory[ 0x5F59 + X ];
	mJoyButtonState = A;
}

void cCreep::events_Execute() {
	// 2E1D
	interruptWait( 2 );

	// Get collisions from the hardware, and set them in the objects
	Sprite_Collision_Set();

	// Check for any actions to execute, 
	// including collisions, movement, general checks
	Sprite_Execute();

	// Check for any other 'background' actions to execute
	object_Execute();

	++mEngine_Ticks;
}

// 29AE: 
void cCreep::convertTimerToTime() {
	
	byte A = mMemory[ mObjectPtr + 1 ];
	convertTimeToNumber( A, 6 );

	A = mMemory[ mObjectPtr + 2 ];
	convertTimeToNumber( A, 3 );
	
	A = mMemory[ mObjectPtr + 3 ];
	convertTimeToNumber( A, 0 );

}

// 29D0: 
void cCreep::convertTimeToNumber( byte pA, byte pY ) {
	byte byte_2A6B = pA;
	
	byte byte_2A6C = 0;

	byte A = byte_2A6B >> 4;
	
	for(;;) {
		byte X = A << 3;
		for(;;) {
			
			A = X;
			A &= 7;
			if( A == 7 )
				break;
			
			mMemory[ 0x736C + pY ] = mMemory[ 0x2A1B + X ];
			pY += 0x08;
			++X;
		}

		// 29FE
		++byte_2A6C;
		if( byte_2A6C == 2 )
			break;
		
		pY -= 0x37;
		A = byte_2A6B & 0x0F;
	}
}

// 2E37: 
void cCreep::Sprite_Collision_Set() {
	byte gfxSpriteCollision = 0, gfxBackgroundCollision = 0;

	vector< sScreenPiece *>				*collisions = mScreen->collisionsGet();
	vector< sScreenPiece *>::iterator	 colIT;

	for( colIT = collisions->begin(); colIT != collisions->end(); ++colIT ) {
		sScreenPiece *piece = *colIT;

		if( piece->mPriority == ePriority_Background || piece->mSprite2 == 0 ) {
			// Background collision
			gfxBackgroundCollision |= (1 << (piece->mSprite-1) );

		} 
		if( piece->mSprite2 ) {
			// Sprite collision
			gfxSpriteCollision |= (1 << (piece->mSprite-1) );
			gfxSpriteCollision |= (1 << (piece->mSprite2-1) );
		}

	}

	// Original
	//gfxSpriteCollision = mMemory[ 0xD01E ];
	//gfxBackgroundCollision = mMemory[ 0xD01F ];

	
	for( byte spriteNumber = 0; spriteNumber != MAX_SPRITES; ++spriteNumber) {
		
		byte A = mRoomSprites[spriteNumber].state;
		if( A & SPR_FLAG_FREE ) {
			gfxSpriteCollision >>= 1;
			gfxBackgroundCollision >>= 1;

		} else {

			A &= 0xF9;
			if( gfxSpriteCollision & 0x01 )
				A |= SPR_FLAG_COLLIDES;
			gfxSpriteCollision >>= 1;
			
			if( gfxBackgroundCollision & 0x01 )
				A |= SPR_FLAG_OVERLAPS;
			gfxBackgroundCollision >>= 1;

			mRoomSprites[spriteNumber].state = A;
		}
	}
}

// 2E79: Execute any objects with actions / collisions, enable their sprites
void cCreep::Sprite_Execute( ) {
	byte  A;
	byte w30 = 0;

	for(byte spriteNumber = 0 ; spriteNumber < MAX_SPRITES; ++spriteNumber ) {

		A = mRoomSprites[spriteNumber].state;

		if(! (A & SPR_FLAG_FREE) ) {
			// 2E8B
			if(! (A & SPR_ACTION_DESTROY) ) {
				
				if(! (A & SPR_ACTION_FLASH) ) {
					--mRoomSprites[spriteNumber].Sprite_field_5;

					if( mRoomSprites[spriteNumber].Sprite_field_5 != 0 ) {
						if(A & SPR_FLAG_COLLIDES) {
							Sprite_Collision_Check(spriteNumber);
							A = mRoomSprites[spriteNumber].state;
							if((A & SPR_ACTION_FLASH))
								goto s2EF3;
						}
						
						continue;
					} else {
					    // 2EAD
						if(A & SPR_ACTION_DIEING) 
							goto s2EF3;

						if(A & SPR_FLAG_OVERLAPS) {
							Sprite_Object_Collision_Check( spriteNumber );
							if( mRoomSprites[spriteNumber].state & SPR_ACTION_FLASH)
								goto s2EF3;
						}
						// 2EC2
						if(!(mRoomSprites[spriteNumber].state & SPR_FLAG_COLLIDES))
							goto s2ED5;

						Sprite_Collision_Check( spriteNumber );
						if( mRoomSprites[spriteNumber].state & SPR_ACTION_FLASH)
							goto s2EF3;

						goto s2ED5;
					}

				} else {
					// 2EF3
s2EF3:
					sprite_FlashOnOff( spriteNumber );
				}

			} else {
				// 2ED5
s2ED5:
				Sprite_Execute_Action( spriteNumber );
				if( mRoomSprites[spriteNumber].state & SPR_ACTION_FLASH )
					goto s2EF3;
			}
			// 2EF6
			if( mRoomSprites[spriteNumber].state & SPR_ACTION_DESTROY )
				goto s2ED5;

			cSprite *sprite = mScreen->spriteGet( spriteNumber );

			if( mRoomSprites[spriteNumber].state & SPR_ACTION_FREE ) {
				mRoomSprites[spriteNumber].state = SPR_FLAG_FREE;
				goto s2F51;
			} else {
				// 2F16
				word_30 = mRoomSprites[spriteNumber].mX;
				word_30 <<= 1;

				w30 = (word_30 & 0xFF00) >> 8;

				// Sprite X
				mMemory[ 0x10 + spriteNumber ] = (word_30 - 8);
				sprite->mX = (word_30 - 8);

				if((word_30 >= 0x100) && ((word_30 - 8) < 0x100))
					--w30;

				if( w30 & 0x80 ) {
s2F51:;
					// 2f51
					sprite->_rEnabled = false;

				} else {
					if( w30 ) {
						A = (mMemory[ 0x20 ] | mMemory[ 0x2F82 + spriteNumber ]);
					} else
						A = (mMemory[ 0x2F82 ] ^ 0xFF) & mMemory[ 0x20 ];

					// Sprites X Bit 8
					mMemory[ 0x20 ] = A;

					// 2F45
					if((A & mMemory[ 0x2F82 + spriteNumber ]) && (mMemory[ 0x10 + spriteNumber ] >= 0x58) && w30 ) {
						sprite->_rEnabled = false;
					} else {
						// 2F5B
						sprite->mY = mRoomSprites[spriteNumber].mY + 0x32;
						sprite->_rEnabled = true;
					}

				}

				// 2F69
				// Enabled Sprites
				mRoomSprites[spriteNumber].Sprite_field_5 = mRoomSprites[spriteNumber].Sprite_field_6;

                mScreen->spriteRedrawSet();
			}
				
		}
		// 2F72
	}

}

// 311E
void cCreep::Sprite_Object_Collision_Check( byte pSpriteNumber ) {

	byte_31F1 = mRoomSprites[pSpriteNumber].mX;
	byte_31F2 = byte_31F1 + mRoomSprites[pSpriteNumber].mCollisionWidth;
	if( (mRoomSprites[pSpriteNumber].mX +  mRoomSprites[pSpriteNumber].mCollisionWidth) > 0xFF )
		byte_31F1 = 0;

	byte_31F3 = mRoomSprites[pSpriteNumber].mY;
	byte_31F4 = byte_31F3 + mRoomSprites[pSpriteNumber].mCollisionHeight;
	if( (mRoomSprites[pSpriteNumber].mY +  mRoomSprites[pSpriteNumber].mCollisionHeight) > 0xFF )
		byte_31F3 = 0;

	// 3149
	if( !mObjectCount )
		return;

	byte_31EF = mObjectCount;

	for( byte ObjectNumber = 0; ObjectNumber != byte_31EF; ++ObjectNumber ) {

		if( !(mRoomAnim[ObjectNumber].mFlags & ITM_DISABLE ))
			if( !(byte_31F2 < mRoomAnim[ObjectNumber].mX ))
				if( !(mRoomAnim[ObjectNumber].mX + mRoomAnim[ObjectNumber].mWidth < byte_31F1))
					if( !(byte_31F4 < mRoomAnim[ObjectNumber].mY) )
						if( !(mRoomAnim[ObjectNumber].mY + mRoomAnim[ObjectNumber].mHeight < byte_31F3) ) {
						//318C
							mStartSpriteFlash = 1;

							if( Sprite_Object_Collision( pSpriteNumber, ObjectNumber ) == true ) {

								if( mStartSpriteFlash == 1 ) 
									mRoomSprites[pSpriteNumber].state |= SPR_ACTION_FLASH;
							}

							Sprite_Object_Infront_Execute( pSpriteNumber, ObjectNumber );
						} 

	}
}

bool cCreep::Sprite_Object_Collision( byte pSpriteNumber, byte pObjectNumber ) {
	word func = mObjectFuncData[ mRoomSprites[pSpriteNumber].Sprite_field_0 ].mFuncColId;

	switch( func ) {
		case 0:
			return false;

		case 0x34EF:		// Player In Front
			obj_Player_Collision( pSpriteNumber, pObjectNumber );
			break;

		case 0x38CE:		// Mummy
			obj_Mummy_Collision( pSpriteNumber, pObjectNumber );
			break;

		case 0x3A60:		// Laser
			obj_RayGun_Laser_Collision( pSpriteNumber, pObjectNumber );
			break;

		case 0x3D6E:		// Frankie
			obj_Frankie_Collision( pSpriteNumber, pObjectNumber );
			break;

		default:
			cout << "Sprite_Object_Collision: 0x";
			cout << std::hex << func << "\n";
			break;

	}

	return true;
}

bool cCreep::Sprite_Object_Infront_Execute( byte pSpriteNumber, byte pObjectNumber ) {

	word func = mObjectImageFuncData[ mRoomAnim[pObjectNumber].mFuncID ].mFuncInfrontId;

	switch( func ) {
		case 0:
			return false;
			break;
		
		case 0x4075:		// In Front Door
			obj_Door_InFront( pSpriteNumber, pObjectNumber );
			break;

		case 0x41D8:		// In Front Button
			obj_Door_Button_InFront( pSpriteNumber, pObjectNumber );
			break;
		
		case 0x44E7:		// In Front Lightning Switch
			obj_Lightning_Switch_InFront( pSpriteNumber, pObjectNumber );
			break;

		case 0x4647:		// In Front Forcefield Timer
			obj_Forcefield_Timer_InFront( pSpriteNumber, pObjectNumber );
			break;

		case 0x47A7:		// In Front Mummy Release
			obj_Mummy_Infront( pSpriteNumber, pObjectNumber );
			break;

		case 0x4990:		// In Front Key
			obj_Key_Infront( pSpriteNumber, pObjectNumber );
			break;

		case 0x4A68:		// In Front Lock
			obj_Door_Lock_InFront( pSpriteNumber, pObjectNumber );
			break;
		
		case 0x4D70:		// In Front RayGun Control
			obj_RayGun_Control_InFront( pSpriteNumber, pObjectNumber );
			break;

		case 0x4EA8:		// In Front Teleport
			obj_Teleport_InFront( pSpriteNumber, pObjectNumber );
			break;
		
		case 0x548B:		// In Front Conveyor
			obj_Conveyor_InFront( pSpriteNumber, pObjectNumber );
			break;

		case 0x5611:		// In Front Conveyor Control
			obj_Conveyor_Control_InFront( pSpriteNumber, pObjectNumber );
			break;

		default:
			cout << "Sprite_Object_Infront_Execute: 0x";
			cout << std::hex << func << "\n";

			break;

	}

	return true;
}

// 30D9
void cCreep::Sprite_Collision( byte pSpriteNumber, byte pSpriteNumber2 ) {
	
	if( mRoomSprites[pSpriteNumber].state & SPR_ACTION_DIEING )
		return;

	mStartSpriteFlash2 = 1;
	mMemory[ 0x311C ] = pSpriteNumber2;
	
	word func = mObjectFuncData[mRoomSprites[pSpriteNumber].Sprite_field_0].mFuncHitId;

	switch( func ) {
		case 0:
			break;

		case 0x3534:				//  Hit Player
			obj_Player_Sprite_Collision( pSpriteNumber, pSpriteNumber2 );
			break;

		case 0x3682:
			mStartSpriteFlash2 = 0;
			break;

		case 0x374F:
			mStartSpriteFlash2 = 0;
			return;
		
		case 0x3940:				// Hit Mummy
			obj_Mummy_Sprite_Collision( pSpriteNumber, pSpriteNumber2 );
			break;

		case 0x3DDE:				//  Hit Frankie
			obj_Frankie_Sprite_Collision( pSpriteNumber, pSpriteNumber2 );
			break;

		default:
			cout << "Sprite_Collision: 0x";
			cout << std::hex << func << "\n";
			break;
	}

	// 3104
	if( mStartSpriteFlash2 != 1 )
		return;

	mRoomSprites[pSpriteNumber].state |= SPR_ACTION_FLASH;
}

// 3026
void cCreep::Sprite_Collision_Check( byte pSpriteNumber ) {
	byte SpriteY_Bottom, byte_311B, SpriteX, SpriteX_Right, SpriteY;

	byte A = mObjectFuncData[mRoomSprites[pSpriteNumber].Sprite_field_0].mHitData;

	if(!(A & 0x80)) {
		byte_311B = A;
		SpriteX = mRoomSprites[pSpriteNumber].mX;
		SpriteX_Right = SpriteX + mRoomSprites[pSpriteNumber].mCollisionWidth;
		if( (SpriteX + mRoomSprites[pSpriteNumber].mCollisionWidth) > 0x100 )
			SpriteX = 0;

		SpriteY = mRoomSprites[pSpriteNumber].mY;
		SpriteY_Bottom = SpriteY + mRoomSprites[pSpriteNumber].mCollisionHeight;
		if( (SpriteY + mRoomSprites[pSpriteNumber].mCollisionHeight) > 0x100 )
			SpriteY = 0;

		for(byte SpriteNumber = 0; SpriteNumber < MAX_SPRITES; ++SpriteNumber ) {

			// 3068
			if( pSpriteNumber == SpriteNumber )
				continue;

			A = mRoomSprites[SpriteNumber].state;

			if( !(A & SPR_FLAG_FREE) ) {
					
				if( (A & SPR_FLAG_COLLIDES) ) {
					A = mMemory[ 0x895 + (mRoomSprites[SpriteNumber].Sprite_field_0 << 3) ];

					if( !(A & 0x80 )) {
						if(! (A & byte_311B )) {

							// 308E
							if( SpriteX_Right >= mRoomSprites[SpriteNumber].mX ) {
								A = mRoomSprites[SpriteNumber].mX + mRoomSprites[SpriteNumber].mCollisionWidth;

								if( A >= SpriteX ) {
									// 30A5
									if( SpriteY_Bottom >= mRoomSprites[SpriteNumber].mY ) {
										if( (mRoomSprites[SpriteNumber].mY + mRoomSprites[SpriteNumber].mCollisionHeight) >= SpriteY ) {
												
											Sprite_Collision( pSpriteNumber, SpriteNumber);
											Sprite_Collision( SpriteNumber, pSpriteNumber );
										}
									}
								}
							}

						}
					}

				}	// COLLIDES
			}	// FREE
		}
	}
	// 30D5
}

//2F8A
void cCreep::sprite_FlashOnOff( byte pSpriteNumber ) {
	byte state = mRoomSprites[pSpriteNumber].state;
	byte Y = 0;

	mScreen->spriteRedrawSet();
	cSprite *sprite = mScreen->spriteGet( pSpriteNumber );

	if( !(state & SPR_ACTION_FLASH) ) {
		
		// Flashing during death
		if( !(mRoomSprites[pSpriteNumber].Sprite_field_8 )) {
			state = mRoomSprites[pSpriteNumber].state ^ SPR_ACTION_DIEING;
		} else
			goto s2FE9;
	
	} else {
		// 2FA3

		// Start to die
		state ^= SPR_ACTION_FLASH;

		mRoomSprites[pSpriteNumber].state = state;

		byte A = mObjectFuncData[mRoomSprites[pSpriteNumber].Sprite_field_0].mFlashData;

		// 
		if(!(A & SPRITE_FLASH_UNK))
			state = mRoomSprites[pSpriteNumber].state;
		else
			goto s2FC4;
	}

	// 2FBB
	mRoomSprites[pSpriteNumber].state = state | SPR_ACTION_DESTROY;
	return;

s2FC4:;
	mRoomSprites[pSpriteNumber].Sprite_field_8 = 8;
	Y = pSpriteNumber;

	// Sprite multicolor mode
	mSprite_Multicolor_Enable = (mMemory[ 0x2F82 + Y ] ^ 0xFF) & mSprite_Multicolor_Enable;
	sprite->_rMultiColored = false;
	
	mRoomSprites[pSpriteNumber].state |= SPR_ACTION_DIEING;
	mRoomSprites[pSpriteNumber].Sprite_field_6 = 1;

s2FE9:;
	if(! (mEngine_Ticks & 1) ) {
		// Flash On
		// 2FF0
		sprite->_color = 1;

		--mRoomSprites[pSpriteNumber].Sprite_field_8;

		mMemory[ 0x760C ] = mRoomSprites[pSpriteNumber].Sprite_field_8 << 3;
		sound_PlayEffect(8);
		
	} else {
		// Flash Off
		// 3010
		sprite->_color = 0;
	}

	// 301C
	mRoomSprites[pSpriteNumber].Sprite_field_5 = mRoomSprites[pSpriteNumber].Sprite_field_6;
}

// Originally this was not a function, but its too big to bother
// implementing in the original location
void cCreep::Sprite_Execute_Action( byte pSpriteNumber ) {

	word func = mObjectFuncData[mRoomSprites[pSpriteNumber].Sprite_field_0].mFuncExecId;

	//2ED5
	switch( func ) {

		case 0x31F6:
			obj_Player_Execute( pSpriteNumber );
			break;

		case 0x3639:
			obj_Lightning_Execute( pSpriteNumber );
			break;

		case 0x36B3:
			obj_Forcefield_Execute( pSpriteNumber );
			break;
		
		case 0x379A:
			obj_Mummy_Execute( pSpriteNumber );
			break;

		case 0x3A08:
			obj_RayGun_Laser_Execute( pSpriteNumber );
			break;

		case 0x3AEB:
			obj_Frankie_Execute( pSpriteNumber );
			break;

		default:
			cout << "Sprite_Execute_Action: 0x";
			cout << std::hex << mRoomSprites[pSpriteNumber].Sprite_field_0 << "\n";
			break;
	}

}

void cCreep::obj_Player_Execute( byte pSpriteNumber ) {
	byte A =  mRoomSprites[ pSpriteNumber ].state;

	// Player leaving room?
	if( A & SPR_ACTION_DESTROY ) {
		A ^= SPR_ACTION_DESTROY;
		A |= SPR_ACTION_FREE;
		mRoomSprites[ pSpriteNumber ].state = A;

		char Y = mRoomSprites[ pSpriteNumber ].playerNumber << 1;
		
		time_t diffSec;
		timeb timeNow;
		ftime(&timeNow);
		size_t seconds, secondsO, minutes, hours;

		// Player 1/2 Time management
		if( Y == 0 ) {
			diffSec = timeNow.time - mPlayer1Time.time;
			mPlayer1Seconds += (size_t) diffSec;
			secondsO = mPlayer1Seconds;
		}
		if( Y == 2 ) {
			diffSec = timeNow.time - mPlayer2Time.time;
			mPlayer2Seconds += (size_t) diffSec;
			secondsO = mPlayer2Seconds;
		}

		// Ptr to CIA Timer
		word_30 = readLEWord( &mMemory[ 0x34E7 + Y ] );
		word_32 = readLEWord( &mMemory[ 0x34EB + Y ] );

		// Do conversions, as the time was originally stored using only 1-10 digits
		int t = (secondsO % 60);
		t /= 10;
		seconds = (secondsO % 60) + (6 * t);
 
		t = (secondsO / 60);
		t /= 10;
		minutes = (secondsO / 60) + (6 * t);

		t = ((secondsO / 60) / 60);
		t /= 10;
		hours = ((secondsO / 60) / 60) + (6 * t);

		// Store the current players time
		// Seconds
		mMemory[ word_32 + 1 ] = seconds;

		// Minutes
		mMemory[ word_32 + 2 ] = minutes;

		// Hours
		mMemory[ word_32 + 3 ] = hours;

		return;
	} 

	//322C
	// Player entering room?
	if( A & SPR_ACTION_CREATED ) {
		A ^= SPR_ACTION_CREATED;
		mRoomSprites[ pSpriteNumber ].state = A;

		// Current Player
		char Y = mRoomSprites[ pSpriteNumber ].playerNumber << 1;

		if( Y == 0 )
			ftime(&mPlayer1Time);

		if( Y == 2 )
			ftime(&mPlayer2Time);

		// Get Ptr to CIA Timer and store area
		word_32 = readLEWord( &mMemory[ 0x34E7 + Y ] );
		word_30 = readLEWord( &mMemory[ 0x34EB + Y ] );

		// Restore CIA Timer
		//for( Y = 3; Y >= 0; --Y ) 
		//	mMemory[ word_32 + Y ] = mMemory[ word_30 + Y ];
		
		Y = mRoomSprites[ pSpriteNumber ].playerNumber;
		A = mMemory[ 0x780D + Y ];
		if( A != 6 ) {
			obj_Player_Color_Set( pSpriteNumber );
			goto s32DB;
		}

	} else {
		// 3269
		byte Y = mRoomSprites[ pSpriteNumber ].playerNumber;
		A = mMemory[ 0x780D + Y ];

		if( A == 5 ) {
			//3280
			mPlayerExecutingSpriteNumber = Y;
			Y = mRoomSprites[ pSpriteNumber ].Sprite_field_1B;
			A = mMemory[ 0x34A4 + Y ];

			if( A != 0xFF ) {
				Y = mPlayerExecutingSpriteNumber;
				mMemory[ 0x780D + Y ] = A;
				mRoomSprites[ pSpriteNumber ].Sprite_field_6 = 1;
				A = mMemory[ 0x780D + Y ];
				goto s32CB;

			} else {
				// 329E
				mRoomSprites[ pSpriteNumber ].Sprite_field_1B += 0x04;
				Y = mRoomSprites[ pSpriteNumber ].Sprite_field_1B;

				mRoomSprites[ pSpriteNumber ].mX += mMemory[ 0x34A1 + Y ];
				mRoomSprites[ pSpriteNumber ].mY += mMemory[ 0x34A2 + Y ];
			}

		} else if( A == 6 ) {
			mMemory[ 0x780D + Y ] = 5;
		} else
			goto s32CB;
	}
	// 32BC
	mRoomSprites[ pSpriteNumber ].spriteImageID = mMemory[ 0x34A3 + mRoomSprites[pSpriteNumber].Sprite_field_1B ];
	obj_Player_Color_Set( pSpriteNumber );
	return;
s32CB:;
	
	if( A != 0 ) {
		mRoomSprites[pSpriteNumber].state |= SPR_ACTION_DESTROY;
		return;
	}

s32DB:;
	A = mRoomSprites[pSpriteNumber].Sprite_field_1A;
	char a = A;
	if( A != 0xFF )
		if( A != mRoomSprites[pSpriteNumber].Sprite_field_19 ) {
			sub_526F( a );
			A = a;
		}

	mRoomSprites[pSpriteNumber].Sprite_field_19 = A;
	mRoomSprites[pSpriteNumber].Sprite_field_1A = 0xFF;

	positionCalculate( pSpriteNumber );
	
	byte byte_34D5 = mMemory[ word_3C ] & mRoomSprites[pSpriteNumber].Sprite_field_18;
	//32FF
	mRoomSprites[pSpriteNumber].Sprite_field_18 = 0xFF;

	if( byte_5FD8 != 0 ) {
		// 3309
		if( !(byte_34D5 & 0x11) ) {
			byte_34D5 &= 0xBB;	
			if( byte_5FD8 >> 1 != byte_5FD7 )
				byte_34D5 &= 0x77;
			else
				byte_34D5 &= 0xDD;
		}

	} else {
		// 3337
		A = byte_5FD7;
		if( A == 3 ) {
			word_3C -= 0x4E;

			byte_34D5 &= 0x75;
			byte_34D5 |= mMemory[ word_3C ] & 2;

		} else {
			// 3360
			if( A == 0 ) {
				word_3C -= 0x52;

				byte_34D5 &= 0x5D;
				byte_34D5 |= mMemory[ word_3C ] & 0x80;

			} else {
				// 3386
				byte_34D5 &= 0x55;
			}
		}
	}
	// 338E
	KeyboardJoystickMonitor( mRoomSprites[pSpriteNumber].playerNumber );
	mRoomSprites[pSpriteNumber].mButtonState = mJoyButtonState;
	mRoomSprites[pSpriteNumber].Sprite_field_1E = byte_5F56;
	
	byte Y = byte_5F56;
	if( !(Y & 0x80 )) {

		if( mMemory[ 0x2F82 + Y ] & byte_34D5 ) {
			mRoomSprites[pSpriteNumber].Sprite_field_1F = Y;
			goto s33DE;

		} 

		// 33B2
		A = mRoomSprites[pSpriteNumber].Sprite_field_1F;

		if(!( A & 0x80 )) {
			A += 1;
			A &= 7;
			if( A != byte_5F56 ) {
				A -= 2;
				A &= 7;
				if( A != byte_5F56 )
					goto s33D6;
			}
			
			if( mMemory[ 0x2F82 + mRoomSprites[pSpriteNumber].Sprite_field_1F ] & byte_34D5 )
				goto s33DE;
		}
	}
s33D6:;
	// 33D6
	mRoomSprites[pSpriteNumber].Sprite_field_1F = 0x80;
	return;

	// Player Input
	// 33DE
s33DE:;
	A = (mRoomSprites[pSpriteNumber].Sprite_field_1F & 3);

	if( A == 2 ) {
		mRoomSprites[pSpriteNumber].mY -= byte_5FD8;

	} else {
		// 33F4
		if( A == 0 ) {
			mRoomSprites[pSpriteNumber].mX -= byte_5FD7;
			++mRoomSprites[pSpriteNumber].mX;
		}
	}
	// 3405
	Y = mRoomSprites[pSpriteNumber].Sprite_field_1F;
	mRoomSprites[pSpriteNumber].mX += mMemory[ 0x34D7 + Y ];
	mRoomSprites[pSpriteNumber].mY += mMemory[ 0x34DF + Y ];

	if( !(Y & 3) ) {
		// 3421
		if( byte_34D5 & 1 ) {
			if( !mRoomSprites[pSpriteNumber].Sprite_field_1F )
				++mRoomSprites[pSpriteNumber].spriteImageID;
			else 
				--mRoomSprites[pSpriteNumber].spriteImageID;
			
			// 3436
			// Ladder Movement 
			A = mRoomSprites[pSpriteNumber].spriteImageID;
			if( A >= 0x2E ) {
				// 3445
				// Moving Up Ladder
				if( A >= 0x32 )
					mRoomSprites[pSpriteNumber].spriteImageID = 0x2E;
			} else {
				// 343D
				// Moving Down Ladder
				mRoomSprites[pSpriteNumber].spriteImageID = 0x31;
			}

		} else {
			//3451
			// Down Pole
			mRoomSprites[pSpriteNumber].spriteImageID = 0x26;
		}

	} else {
		// 3459
		// Player Frame
		++mRoomSprites[pSpriteNumber].spriteImageID;

		if( mRoomSprites[pSpriteNumber].Sprite_field_1F < 4 ) {
			// 3463
			A = mRoomSprites[pSpriteNumber].spriteImageID;
			if( A >= 6 || A < 3 )
				mRoomSprites[pSpriteNumber].spriteImageID = 3;

		} else {
			// 3476
			// Max frame reached?
			if( mRoomSprites[pSpriteNumber].spriteImageID >= 3 )
				mRoomSprites[pSpriteNumber].spriteImageID = 0;
		}
	}

	// 3482
	obj_Player_Color_Set( pSpriteNumber );

}

// 3488: 
void cCreep::obj_Player_Color_Set( byte pSpriteNumber ) {
	hw_SpritePrepare( pSpriteNumber );

	mPlayerExecutingSpriteNumber = pSpriteNumber;

	cSprite *sprite = mScreen->spriteGet( pSpriteNumber );

	sprite->_color = mMemory[ 0x34D3 + mRoomSprites[pSpriteNumber].playerNumber ];
	mScreen->spriteRedrawSet();
}

// 3AEB: Frankie Movement
void cCreep::obj_Frankie_Execute( byte pSpriteNumber ) {
	byte Y;
	char A = mRoomSprites[pSpriteNumber].state;
	byte byte_3F0B, byte_3F12;
	char byte_3F0A, byte_3F10, byte_3F11;

	if( mNoInput )
		return;

	if( A & SPR_ACTION_DESTROY ) {
		mRoomSprites[pSpriteNumber].state = (A ^ SPR_ACTION_DESTROY) | SPR_ACTION_FREE;
		return;
	}

	if( A & SPR_ACTION_CREATED ) 
		mRoomSprites[pSpriteNumber].state ^= SPR_ACTION_CREATED;

	word_40 = mFrankiePtr + mRoomSprites[pSpriteNumber].Sprite_field_1F;
	
	if( !(mRoomSprites[pSpriteNumber].Sprite_field_1E & byte_574E) ) {
		if( mIntro == 1 )
			return;

		// 3B31
		for(byte_3F0A = 1; byte_3F0A >= 0; --byte_3F0A) {
			Y = byte_3F0A;

			if( mMemory[ 0x780D + Y ] != 0 )
				continue;

			Y = mMemory[ 0x34D1 + Y ];
			short int distance = mRoomSprites[pSpriteNumber].mY;
			distance -= mRoomSprites[Y].mY;

			// Within 4 on the Y axis, then frank can wake up
			if( distance >= 4 )
				continue;

			// 3B4A
			distance = mRoomSprites[pSpriteNumber].mX;
			distance -= mRoomSprites[Y].mX;

			if( !(distance < 0)) {
				// We are behind frank

				if( !(mRoomSprites[pSpriteNumber].Sprite_field_1E & byte_574F) )
					continue;
				else
					goto s3B6E;
			}

			// 3B5E
			A = mRoomSprites[pSpriteNumber].Sprite_field_1E;
			if( !(A & byte_574F)) {
s3B6E:
				A |= byte_574E;
				mRoomSprites[pSpriteNumber].Sprite_field_1E = A;
				mMemory[ word_40 ] = A;
				mRoomSprites[pSpriteNumber].mButtonState = 0x80;

				sound_PlayEffect(0x07);
				break;
			}

		}
	
		if(byte_3F0A < 0 )
			return;

	}
	
	// 3B82
	A = mRoomSprites[pSpriteNumber].Sprite_field_1B;

	if( (byte) A != 0xFF )
		if( A != mRoomSprites[pSpriteNumber].Sprite_field_1A ) {
			sub_526F(A);
		}

	mRoomSprites[pSpriteNumber].Sprite_field_1A = A;
	mRoomSprites[pSpriteNumber].Sprite_field_1B = 0xFF;
	positionCalculate( pSpriteNumber );

	//3B9C
	A = mMemory[ word_3C ] & mRoomSprites[pSpriteNumber].playerNumber;
	byte byte_3F13 = A;

	mRoomSprites[pSpriteNumber].playerNumber = 0xFF;
	A = byte_3F13;
	if(!A) {
		mRoomSprites[pSpriteNumber].mButtonState = 0x80;
		goto s3CB4;
	} else {
		byte_3F0A = 0;
		
		// 3BBF
		for(char Y = 6; Y >= 0;) {
			if( (mMemory[ 0x2F82 + Y ] & byte_3F13 )) {
				++byte_3F0A;
				byte_3F0B = Y;
			}

			Y -= 2;
		}
	
		// 3BD1
		if( byte_3F0A == 1 ) {
			mRoomSprites[pSpriteNumber].mButtonState = byte_3F0B;
			goto s3CB4;
		}
		if( byte_3F0A == 2 ) {
			byte Y = (byte_3F0B - 4) & 7;
			if( mMemory[ 0x2F82 + Y ] & byte_3F13 ) {
				Y = mRoomSprites[pSpriteNumber].mButtonState;
				if( !(Y & 0x80 ))
					if( mMemory[ 0x2F82 + Y ] & byte_3F13 )
						goto s3CB4;
			}
		}
		// 3C06
		for(char Y = 3; Y >= 0; --Y) {
			mMemory[ 0x3F0C + Y ] = 0xFF;
		}

		// 3C15
		for(byte_3F0A = 1; byte_3F0A>=0; --byte_3F0A) {

			byte Y;
			if( mMemory[ 0x780D + byte_3F0A ] == 0 ) {
				
				Y = mMemory[ 0x34D1 + byte_3F0A ];
				A = mRoomSprites[Y].mX;
				A -= mRoomSprites[pSpriteNumber].mX;
				//3C2A
				if( A < 0 ) {
					A ^= 0xFF;
					++A;
					Y = 3;
				} else {
					Y = 1;
				}
				if( A < mMemory[ 0x3F0C +  Y ] )
					mMemory[ 0x3F0C + Y ] = A;

				Y = mMemory[ 0x34D1 + byte_3F0A ];
				A = mRoomSprites[Y].mY;
				A -= mRoomSprites[pSpriteNumber].mY;
				if( A < 0 ) {
					A ^= 0xFF;
					++A;
					Y = 0;
				} else {
					Y = 2;
				}
				if( A < mMemory[ 0x3F0C + Y ] )
					mMemory[ 0x3F0C + Y ] = A;
			}
			// 3C62
		}

		// 3C67
		byte_3F10 = -1;
		for(;;) {
			byte_3F11 = 0x00;
			byte_3F12 = 0xFF;
			
			for( signed char Y = 3; Y >= 0; --Y ) {
				A = mMemory[ 0x3F0C + Y ];
				if( (byte) A >= (byte) byte_3F10 )
					continue;
				if( (byte) A < (byte) byte_3F11 )
					continue;

				byte_3F11 = A;
				byte_3F12 = Y;
			}
		
			//3C8E
			A = byte_3F12;
			if( A == -1 ) {
				mRoomSprites[pSpriteNumber].mButtonState = 0x80;
				goto s3CB4;
			}

			A = mMemory[ 0x2F82 + (byte_3F12 << 1) ];
			if( A & byte_3F13 ) {
				// 3CB0
				mRoomSprites[pSpriteNumber].mButtonState = (byte_3F12 << 1);
				break;
			}

			byte_3F10 = byte_3F11;
		}

	}

	// 3CB4
s3CB4:;
	if( mRoomSprites[pSpriteNumber].mButtonState & 2 ) {
		mRoomSprites[pSpriteNumber].mY -= byte_5FD8;

		++mRoomSprites[pSpriteNumber].spriteImageID;
		if( mRoomSprites[pSpriteNumber].mButtonState != 2 ) {
			// 3ccf
			--mRoomSprites[pSpriteNumber].mX;
			if( mRoomSprites[pSpriteNumber].spriteImageID >= 0x87 )
				if( mRoomSprites[pSpriteNumber].spriteImageID < 0x8A )
					goto s3D4C;
			
			mRoomSprites[pSpriteNumber].spriteImageID = 0x87;
			
		} else {
			// 3ce5
			++mRoomSprites[pSpriteNumber].mX;
			if( mRoomSprites[pSpriteNumber].spriteImageID >= 0x84 )
				if( mRoomSprites[pSpriteNumber].spriteImageID < 0x87 )
					goto s3D4C;

			mRoomSprites[pSpriteNumber].spriteImageID = 0x84;

			goto s3D4C;
		}
	} else {
		// 3CFB
		A = mRoomSprites[pSpriteNumber].mButtonState;
		if(A & 0x80)
			goto s3D4F;

		mRoomSprites[pSpriteNumber].mX -= byte_5FD7;
		++mRoomSprites[pSpriteNumber].mX;
		if( !(mMemory[ word_3C ] & 1) ) {
			// 3d15
			mRoomSprites[pSpriteNumber].spriteImageID = 0x8A;
			mRoomSprites[pSpriteNumber].mY += 2;
			goto s3D4C;
		} else {
			// 3d26
			if( !(mRoomSprites[pSpriteNumber].mButtonState) )
				mRoomSprites[pSpriteNumber].mY -= 2;
			else
				mRoomSprites[pSpriteNumber].mY += 2;

			// 3d40
			mRoomSprites[pSpriteNumber].spriteImageID = ((mRoomSprites[pSpriteNumber].mY & 0x06) >> 1) + 0x8B;
		}
	}
s3D4C:;
	// 3D4C
	hw_SpritePrepare( pSpriteNumber );

s3D4F:;
	mMemory[ word_40 + 6 ] = mRoomSprites[pSpriteNumber].mButtonState;
	mMemory[ word_40 + 3 ] = mRoomSprites[pSpriteNumber].mX;
	mMemory[ word_40 + 4 ] = mRoomSprites[pSpriteNumber].mY;
	mMemory[ word_40 + 5 ] = mRoomSprites[pSpriteNumber].spriteImageID;
}

// 3D6E: Frankie?
void cCreep::obj_Frankie_Collision( byte pSpriteNumber, byte pObjectNumber ) {

	char A = mRoomSprites[pSpriteNumber].mX + mRoomSprites[pSpriteNumber].mWidth;
	A -= mRoomAnim[pObjectNumber].mX;

	if( A >= 4 ) {
		mStartSpriteFlash = 0;
		return;
	}
	
	// 3d85
	if( mRoomAnim[pObjectNumber].mFuncID != 0x0B ) {
		mStartSpriteFlash = 0;
		if( mRoomAnim[pObjectNumber].mFuncID != 0x0C )
			return;
		mRoomSprites[pSpriteNumber].Sprite_field_1B = mRoomObjects[pObjectNumber].objNumber;
		return;

	} else {
		// 3DA1
		word_40 = mRoomTrapDoorPtr + mRoomObjects[pObjectNumber].objNumber;

		if( !(mMemory[ word_40 ] & byte_538A) ) {
			mStartSpriteFlash = 0;
			return;
		}
		
		word_40 = mFrankiePtr + mRoomSprites[pSpriteNumber].Sprite_field_1F;

		A = (byte_574E ^ 0xFF) & mMemory[ word_40 ];
		A |= byte_574D;
		mMemory[ word_40 ] = A;
		mRoomSprites[pSpriteNumber].Sprite_field_1E = A;
	}
}

// 3DDE: Franky Hit 
void cCreep::obj_Frankie_Sprite_Collision( byte pSpriteNumber, byte pSpriteNumber2 ) {
	if( mRoomSprites[pSpriteNumber].Sprite_field_1E & byte_574E ) {
		byte A = mRoomSprites[pSpriteNumber2].Sprite_field_0;

		if( A && A != 2 && A != 3 ) {
			// 3DF3
			if( A != 5 ) {
			
				word_40 = mFrankiePtr + mRoomSprites[pSpriteNumber].Sprite_field_1F;
				mMemory[ word_40 ] = ((byte_574E ^ 0xFF) & mMemory[ word_40 ]) | byte_574D;
				return;

			} else {
				// 3E18
				A = mRoomSprites[pSpriteNumber].spriteImageID;
				if( A >= 0x8A && A < 0x8F ) {
					// 3E23
					A = mRoomSprites[pSpriteNumber2].spriteImageID;
					if( A < 0x8A || A >= 0x8F ) {
						mStartSpriteFlash2 = 0;
						return;
					}
					// 3E2E
					if( mRoomSprites[pSpriteNumber].mY == mRoomSprites[pSpriteNumber2].mY ) {
						mStartSpriteFlash2 = 0;
						return;
					}
					if( mRoomSprites[pSpriteNumber].mY < mRoomSprites[pSpriteNumber2].mY ) {
						mRoomSprites[pSpriteNumber].playerNumber &= 0xEF;
						mStartSpriteFlash2 = 0;
						return;
					}
					// 3E43
					mRoomSprites[pSpriteNumber].playerNumber &= 0xFE;
					mStartSpriteFlash2 = 0;
					return;
				}
				// 3E4E
				byte A =  mRoomSprites[pSpriteNumber].spriteImageID;
				byte B = mRoomSprites[pSpriteNumber2].spriteImageID;

				// 3E51
				if( A < 0x84 || A >= 0x8A || B < 0x84 || B >= 0x8A ) {
					mStartSpriteFlash2 = 0;
					return;
				}
				if( mRoomSprites[pSpriteNumber].mX < mRoomSprites[pSpriteNumber2].mX ) {
					mRoomSprites[pSpriteNumber].playerNumber &= 0xFB;
					mStartSpriteFlash2 = 0;
					return;
				}
				// 3E77
				mRoomSprites[pSpriteNumber].playerNumber &= 0xBF;
			}
		} 	
	}

	// 3E4B
	mStartSpriteFlash2 = 0;
	return;
}			

// 3E87: 
void cCreep::obj_Frankie_Sprite_Create() {
	if( mMemory[ mObjectPtr ] & byte_574D )
		return;

	byte X = sprite_CreepFindFree();

	mRoomSprites[X].Sprite_field_0 = 5;
	mRoomSprites[X].Sprite_field_1F = byte_574A;
	mRoomSprites[X].Sprite_field_1E = mMemory[ mObjectPtr ];

	if( !(mMemory[ mObjectPtr ] & byte_574E) ) {
		mRoomSprites[X].mX= mMemory[ mObjectPtr + 1 ];
		mRoomSprites[X].mY= mMemory[ mObjectPtr + 2 ] + 7;
		mRoomSprites[X].spriteImageID= 0x8F;
	} else {
		// 3EC8
		mRoomSprites[X].mX= mMemory[ mObjectPtr + 3 ];
		mRoomSprites[X].mY= mMemory[ mObjectPtr + 4 ];
		mRoomSprites[X].spriteImageID= mMemory[ mObjectPtr + 5 ];
		mRoomSprites[X].mButtonState = mMemory[ mObjectPtr + 6 ];
	}
	// 3ee4
	mRoomSprites[X].mWidth = 3;
	mRoomSprites[X].mHeight = 0x11;
	hw_SpritePrepare(X);
	mRoomSprites[X].playerNumber = 0xFF;
	mRoomSprites[X].Sprite_field_1A = 0xFF;
	mRoomSprites[X].Sprite_field_1B = 0xFF;
	mRoomSprites[X].Sprite_field_6 = 2;
	mRoomSprites[X].Sprite_field_5 = 2;
}

void cCreep::object_Execute() {

	for(byte X = 0; X < mObjectCount; ++X ) {
		
		byte A = mRoomAnim[X].mFlags;
		if(A & ITM_EXECUTE) {

			word func = mObjectImageFuncData[ mRoomAnim[X].mFuncID ].mFuncExecId;

			switch( func ) {
				case eAnimNone:
					mRoomAnim[X].mFlags ^= ITM_EXECUTE;
					break;

				case eAnimDoor:
					obj_Door_Execute( X );
					break;

				case eAnimLightning:
					obj_Lightning_Pole_Execute( X );
					break;

				case eAnimForceFieldTimer:	
					obj_Forcefield_Timer_Execute( X );
					break;

				case eAnimMummy:
					obj_Mummy_Tomb_Execute( X );
					break;

				case eAnimRayGun:
					obj_RayGun_Execute( X );
					break;

				case eAnimTeleport:
					obj_Teleport_Execute( X );
					break;

				case eAnimTrapDoorSwitch:
					obj_TrapDoor_Switch_Execute( X );
					break;

				case eAnimConveyor:
					obj_Conveyor_Execute( X );
					break;

				default:
					cout << "object_Execute: 0x";
					cout << std::hex << func << "\n";
					break;
			}

			// 3F93
			A = mRoomAnim[X].mFlags;
		}
	
		// 3F96
		// Key picked up
		if( A & ITM_PICKED ) {
			roomAnim_Disable( X );

			// Decrease object count
			--mObjectCount;

			// Last object? then nothing to do
			if( X == mObjectCount )
				break;

			mRoomAnim[X] = mRoomAnim[mObjectCount];
			mRoomObjects[X] = mRoomObjects[mObjectCount];
		}

		// 3FC7
	}

	//
	
}

// 3639: 
void cCreep::obj_Lightning_Execute( byte pSpriteNumber ) {
	byte A = mRoomSprites[pSpriteNumber].state;
	if( A & SPR_ACTION_DESTROY ) {
		mRoomSprites[pSpriteNumber].state = (A ^ SPR_ACTION_DESTROY) | SPR_ACTION_FREE;
		return;
	}

	if( A & SPR_ACTION_CREATED ) {
		A ^= SPR_ACTION_CREATED;
		mRoomSprites[pSpriteNumber].state = A;
	}

	A = seedGet();
	A &= 0x03;
	mRoomSprites[pSpriteNumber].Sprite_field_6 = A;
	++mRoomSprites[pSpriteNumber].Sprite_field_6;

	A = seedGet();
	A &= 3;

	A += 0x39;
	if( A == mRoomSprites[pSpriteNumber].spriteImageID ) {
		A += 0x01;
		if( A >= 0x3D )
			A = 0x39;
	}
	// 3679
	mRoomSprites[pSpriteNumber].spriteImageID = A;
	hw_SpritePrepare( pSpriteNumber );
}

// 368A
void cCreep::obj_Lightning_Sprite_Create( byte pObjectNumber  ) {
	sCreepSprite *sprite = sprite_CreepGetFree();

	sprite->Sprite_field_0 = 1;

	sprite->mX = mRoomAnim[pObjectNumber].mX;
	sprite->mY = mRoomAnim[pObjectNumber].mY + 8;
	
	sprite->Sprite_field_1F = mRoomObjects[pObjectNumber].objNumber;
}

// 36B3: Forcefield
void cCreep::obj_Forcefield_Execute( byte pSpriteNumber ) {
	byte A = mRoomSprites[pSpriteNumber].state;
	byte Y;

	if(A & SPR_ACTION_DESTROY ) {
		A ^= SPR_ACTION_DESTROY;

		A |= SPR_ACTION_FREE;
		mRoomSprites[pSpriteNumber].state = A;
		return;
	}

	if(A & SPR_ACTION_CREATED ) 
		mRoomSprites[pSpriteNumber].state = (A ^ SPR_ACTION_CREATED);

	Y = mRoomSprites[pSpriteNumber].Sprite_field_1F;

	if( mMemory[ 0x4750 + Y ] == 1 ) {

		if( mRoomSprites[pSpriteNumber].Sprite_field_1E != 1 ) {
			mRoomSprites[pSpriteNumber].Sprite_field_1E = 1;
			positionCalculate( pSpriteNumber );
			
			word_3C -= 2;
			mMemory[ word_3C + 0 ] = mMemory[ word_3C + 0 ] & 0xFB;
			mMemory[ word_3C + 4 ] = mMemory[ word_3C + 4 ] & 0xBF;

			A = 0x35;
		} else {
			// 3709
			if( mRoomSprites[pSpriteNumber].spriteImageID == 0x35 )
				A = 0x3D;
			else
				A = 0x35;
		}

	} else {
	// 371A
		if( mRoomSprites[pSpriteNumber].Sprite_field_1E != 1 )
			return;

		mRoomSprites[pSpriteNumber].Sprite_field_1E = 0;
		positionCalculate( pSpriteNumber );

		word_3C -= 2;
		mMemory[ word_3C + 0 ] |= 4;
		mMemory[ word_3C + 4 ] |= 0x40;
		A = 0x41;
	}

	// 3746
	mRoomSprites[pSpriteNumber].spriteImageID = A;

	// Draw the forcefield
	hw_SpritePrepare( pSpriteNumber );
}

// 5FD9
void cCreep::roomPtrSet( byte pRoomNumber ) {

	mRoomPtr = pRoomNumber << 3;
	mRoomPtr += 0x7900;

	if( mIntro )
		mRoomPtr += 0x2000;
}

bool cCreep::ChangeLevel( size_t pMenuItem ) {
	size_t castleNumber = (pMenuItem - 0x10);
	castleNumber /= 4;

	if( (mCastle = mCastleManager->castleLoad( castleNumber )) == 0)
			return false;

	menuUpdate(pMenuItem); 

	return true;
}

// 0D71: 
void cCreep::Game() {

	mMenuReturn = false;
	mPlayer1Seconds = 0;
	mPlayer2Seconds = 0;

	if( mSaveGameLoaded == 1 ) {
		// D7D
		mSaveGameLoaded = 0;
		mMemory[ 0x7802 ] |= 1;
	} else {
		// D8D
		word_30 = 0x9800;
		word_32 = 0x7800;

		// 
		memcpy( &mMemory[ word_32 ], &mMemory[ word_30 ], readLEWord( &mMemory[ 0x9800 ] ) );

		// DC6
		// Which joystick was selected when button press was detected (in intro)
		mMemory[ 0x7812 ] = byte_D10;

		// Clear Stored CIA Timers
		for( signed char Y = 7; Y >= 0; --Y )
			mMemory[ 0x7855 + Y ] = 0;

		mMemory[ 0x785D ] = mMemory[ 0x785E ] = 0;
		mMemory[ 0x7809 ] = mMemory[ 0x7803 ];	// Player1 Start Room
		mMemory[ 0x780A ] = mMemory[ 0x7804 ];	// Player2 Start Room
		mMemory[ 0x780B ] = mMemory[ 0x7805 ];	// Player1 Start Door
		mMemory[ 0x780C ] = mMemory[ 0x7806 ];	// Player2 Start Door
		mMemory[ 0x7811 ] = 0;
		mMemory[ 0x780F ] = 1;

		// E00
		if( mMemory[ 0x7812 ] != 1 ) {
			mMemory[ 0x7810 ] = 0;
			mMemory[ 0x780E ] = 4;
		} else
			mMemory[ 0x7810 ] = 1;
	}

	// E19
	for(;;) {
		if( mMemory[ 0x780F ] != 1 )
			if( mMemory[ 0x7810 ] != 1 )
				break;
		// E2A
		if( mMemory[ 0x780F ] == 1 && mMemory[ 0x7810 ] == 1 ) {

			// Player1 and 2 in same room?
			if( mMemory[ 0x7809 ] != mMemory[ 0x780A ] ) {

				// No
				byte X = mMemory[ 0x7811 ];

				mPlayerStatus[X] = false;
				X ^= 0x01;
				mPlayerStatus[X] = true;

			} else {
				mPlayerStatus[0] = mPlayerStatus[1] =true;
			}

		} else {
			// E5F
			if( mMemory[ 0x780F ] != 1 ) {
				mPlayerStatus[1] = true;
				mPlayerStatus[0] = false;
			} else {
				// E73
				mPlayerStatus[0] = true;
				mPlayerStatus[1] = false;
			}
		}

		// E7D
		while( mapDisplay() );

		if(mMenuReturn) {
			mInput->inputClear();
			hw_IntSleep(1);
			return;
		}

		roomMain();
		screenClear();
		
		mMemory[ 0xF62 ] = 0;

		// E8D
		for( byte X = 0; X < 2; ++X ) {

			if( mPlayerStatus[X] == 1 ) {
				if( mMemory[ 0x780D + X ] != 2 ) {
					if( mMemory[ 0x785D + X ] != 1 )
						continue;

					mMemory[ 0x1AB2 ] = X;
					gameEscapeCastle();

					if(! (mMemory[ 0x7802 ] & 1 )) {
						
						if( mUnlimitedLives != 0xFF ) {
							
							byte A = X << 2;
							
							word_30 = 0x7855 + A;
							for( signed char Y = 3; Y >= 0; --Y )
								mMemory[ 0x1CF9 + Y ] = mMemory[ word_30 + Y ];
							
							mMemory[ 0x1CFD ] = X;
							gameHighScoresHandle();
						}
					}
					// EFC
sEFC:;
					mMemory[ 0x780F + X ] = 0;
					mMemory[ 0xF62 ] = 1;
				} else {
				// EDE
					// Player Died
					if( mUnlimitedLives != 0xFF ) {
						--mMemory[ 0x7807 + X ];
						byte A = mMemory[ 0x7807 + X ];
						if(A == 0)
							goto sEFC;
					}
					// Set player1/2 start room
					mMemory[ 0x7809 + X ] = mMemory[ 0x7803 + X ];
					mMemory[ 0x780B + X ] = mMemory[ 0x7805 + X ];
				}
			}
			// F06

		}

		// Game Over Check
		// F0B
		if( mMemory[ 0xF62 ] == 1 ) {
			screenClear();

			mObjectPtr = 0x0F64;		// Game Over
			obj_stringPrint();

			if( mMemory[ 0x7812 ] != 0 ) {
				if( mMemory[ 0x780F ] != 1 ) {
					mObjectPtr = 0x0F72;	// For Player
					obj_stringPrint();
				}

				// F39
				if( mMemory[ 0x7810 ] != 1 ) {
					mObjectPtr = 0x0F83;	// For Player
					obj_stringPrint();
				}

			}

			hw_Update();

			// F4B
			hw_IntSleep( 0x23 );
		}

	}
	// F5B
}

void cCreep::mapArrowDraw( byte pPlayer ) {
	sub_6009( mMemory[ 0x780B + pPlayer ] );
			
	mMemory[ 0x11D9 ] = mMemory[ word_40 + 2 ] & 3;
	
	// Object Number (saved as sprite number)
	pPlayer = sprite_CreepFindFree( );
	mMemory[ 0x11D8 ] = pPlayer;

	cSprite *sprite = mScreen->spriteGet( pPlayer );
	
	// Calculate X
	byte A = mMemory[ mRoomPtr + 1 ];	// X
	A += mMemory[ word_40 + 5 ];

	byte Y = mMemory[ 0x11D9 ];
	A += mMemory[ 0x11DA + Y ];

	word posX = A;
	posX -= 4;
	posX <<= 1;

	// Sprite X
	sprite->mX = posX;
	mMemory[ 0x10 + Y ] = A;

	// Calculate Y
	// 100D
	A = mMemory[ mRoomPtr + 2 ];		// Y
	A += mMemory[ word_40 + 6 ];
	A += mMemory[ 0x11DE + mMemory[ 0x11D9 ] ];
	A += 0x32;

	// Sprite Y
	sprite->mY = A;
	mRoomSprites[pPlayer].spriteImageID = mMemory[ 0x11E2 + mMemory[0x11D9] ];
	
	// Enable the Arrow sprite
	hw_SpritePrepare( pPlayer );
	
	// Sprites Enabled
	sprite->_rEnabled = true;
}

// F94: Display the map/time taken screen
bool cCreep::mapDisplay() {
	byte gfxPosX, gfxPosY;

	screenClear();
	
	Sleep(300);
	mInput->inputCheck( true );

	// Draw both players Name/Time/Arrows
	// FA9
	for(mMemory[ 0x11D7 ] = 0 ; mMemory[ 0x11D7 ] != 2; ++mMemory[ 0x11D7 ] ) {

		byte X = mMemory[ 0x11D7 ];

		if( mPlayerStatus[X] == 1 ) {
			// FB6

			// Mark Room as visible on map
			byte A = mMemory[ 0x7809 + X ];
			roomPtrSet( A );
			mMemory[ mRoomPtr ] |= byte_8C0;
			
			// 
			mapArrowDraw( X );

			// 103C
			byte Y = mMemory[ 0x11D7 ];
			X = mMemory[ 0x7807 + Y ];
			A = mMemory[ 0x11E5 + X ];

			mMemory[ 0x11EF ] = mMemory[ 0x11FA ] = A;
			X = Y << 1;
			
			// Player (One Up / Two Up)
			mObjectPtr = mMemory[ 0x11E9 + X ];
			mObjectPtr += (mMemory[ 0x11EA + X ] << 8);

			obj_stringPrint();

			// 1058
			mObjectPtr = (Y << 2);
			mObjectPtr += 0x7855;

			convertTimerToTime();

			gfxPosX = mMemory[ 0x11D3 + Y ] + 8;
			gfxPosY = 0x10;

			// Draw Time ' : : '
			screenDraw(0, 0x93, gfxPosX, gfxPosY, 0);
		} 
		
		// 1087
	}
	
	// 1094
	mapRoomDraw();

	mMemory[ 0x11CB ] = 0;
	mMemory[ 0x11CD ] = mMemory[ 0x11CC ] = 1;
	if( mPlayerStatus[0] != 1 ) {
		if( mPlayerStatus[1] == 1 )
			mMemory[ 0x11CD ] = 0;

	} else {
		// 10BA
		mMemory[ 0x11CC ] = 0;
		if( mPlayerStatus[1] == 1 ) {
			mMemory[ 0x11CD ] = 0;
			if( mMemory[ 0x780B ] == mMemory[ 0x780C ] ) {
				mMemory[ 0x11CB ] = 1;

				mScreen->spriteGet( 0 )->_color = 1;
				mScreen->spriteGet( 1 )->_color = 1;
				goto s10EB;
			}
		}
	}

	// 10E3
	mScreen->spriteGet( 0 )->_color = 0;
	mScreen->spriteGet( 1 )->_color = 0;

s10EB:;
	mMemory[ 0x11D0 ] = 1;
	mRestorePressed = false;
	mMemory[ 0x11CE ] = mMemory[ 0x11D1 ];
	mMemory[ 0x11CF ] = mMemory[ 0x11D2 ];
	mMemory[ 0x11D7 ] = 0;
	

	// 110E
	for( ;; ) {

		byte X = mMemory[ 0x11D7 ];

		// Player Arrow Flash
		if( mMemory[ 0x11CB ] != 1 ) {
			--mMemory[ 0x11CE + X ];
			if(mMemory[ 0x11CE + X ] == 0 ) {
				// 1122
				mMemory[ 0x11CE + X ] = mMemory[ 0x11D1 + X ];
				if( X == 0 || mPlayerStatus[0] != 1 ) {
					// 1133
					mScreen->spriteGet( 0 )->_color ^= 0x01;
				} else {
					// 113E
					mScreen->spriteGet( 1 )->_color ^= 0x01;
				}
				
				mScreen->spriteRedrawSet();
			}
		}

		// 115A
		KeyboardJoystickMonitor( X );

		// 1146
		if( mRestorePressed == true ) {
			mRestorePressed = false;
			mMemory[ 0x11D0 ] = 0;
			mMenuReturn = true;
			mInput->inputCheck(true);
			return false;
		}

		// Check if run/stop was pressed
		if( mRunStopPressed == true ) {

			// Save the game
			gamePositionSave( false );

			return true;
		}

		if( mInput->f2Get() ) {

			// Load a game
			gamePositionLoad();

			return true;
		}

		// 117D
		// Button Press Check
		if( mJoyButtonState )
			mMemory[ 0x11CC + X ] = 1;

		if( mMemory[ 0x11CC ] == 1 )
			if( mMemory[ 0x11CD ] == 1 )
				break;
		
		// 1195
		mMemory[ 0x11D7 ] ^= 0x01;

		interruptWait( 1 );

		hw_Update();
	}
	// 11A5
	do {
		hw_Update();

		KeyboardJoystickMonitor(0);
		if( mJoyButtonState )
			continue;

		KeyboardJoystickMonitor(1);

	} while( mJoyButtonState );
		

	sound_PlayEffect( 9 );
	mMemory[ 0x11D0 ] = 0;
	return false;
}

// 34EF
void cCreep::obj_Player_Collision( byte pSpriteNumber, byte pObjectNumber ) {
	byte A;
	if( mRoomAnim[pObjectNumber].mFuncID == 0x0B ) {
		
		A = mRoomSprites[pSpriteNumber].mX + mRoomSprites[pSpriteNumber].mWidth;
		A -= mRoomAnim[pObjectNumber].mX;

		if( A < 4 ) {
			mMemory[ 0x780D + mRoomSprites[pSpriteNumber].playerNumber ] = 2;
			return;
		}

	} 
	// 3505
	mStartSpriteFlash = 0;
	if( mRoomAnim[pObjectNumber].mFuncID != 0x0C ) 
		return;

	A = mRoomSprites[pSpriteNumber].mX + mRoomSprites[pSpriteNumber].mWidth;
	A -= mRoomAnim[pObjectNumber].mX;

	if( A >= 4 )
		return;

	mRoomSprites[pSpriteNumber].Sprite_field_1A = mRoomObjects[pObjectNumber].objNumber;
	return;
}

// 3534: Hit Player
void cCreep::obj_Player_Sprite_Collision( byte pSpriteNumber, byte pSpriteNumber2 ) {
	byte A = mRoomSprites[pSpriteNumber2].Sprite_field_0;

	if( A == 2 ) {
		mStartSpriteFlash2 = 0;
		return;
	}

	if( A != 0 ) {
		// 358C
		if( mMemory[ 0x780D + mRoomSprites[pSpriteNumber].playerNumber ] != 0 ) {
			mStartSpriteFlash2 = 0;
			return;
		}

		mMemory[ 0x780D + mRoomSprites[pSpriteNumber].playerNumber ] = 2;
		return;
	}
	// 353F

	A = mRoomSprites[pSpriteNumber2].spriteImageID;

	if( A == 0x2E || A == 0x2F || A == 0x30 || A == 0x31 || A == 0x26 ) {
		mStartSpriteFlash2 = 0;
		return;
	} 
	
	if( mRoomSprites[pSpriteNumber2].mY == mRoomSprites[pSpriteNumber].mY ) {
		mStartSpriteFlash2 = 0;
		return;
	}
	if( mRoomSprites[pSpriteNumber2].mY >= mRoomSprites[pSpriteNumber].mY ) {
		mRoomSprites[pSpriteNumber].Sprite_field_18 = 0xEF;
		mStartSpriteFlash2 = 0;
		return;
	}

	mRoomSprites[pSpriteNumber].Sprite_field_18 = 0xFE;
	mStartSpriteFlash2 = 0;
	return;
}

// 359E
void cCreep::obj_Player_Add( ) {
	byte spriteNumber = sprite_CreepFindFree();
	
	byte Y = mCurrentPlayer;
	mMemory[ 0x34D1 + Y ] = spriteNumber;		// Set player object number
	
	byte A = mMemory[ 0x780B + Y ];
	A <<= 3;
	word_40 = A + mRoomDoorPtr;
	
	// 35C0
	if( mMemory[ word_40 + 2 ] & 0x80 ) {
		mMemory[ 0x780D + mCurrentPlayer ] = 6;
		mRoomSprites[spriteNumber].mX= mMemory[ word_40 ] + 0x0B;
		mRoomSprites[spriteNumber].mY= mMemory[ word_40 + 1 ] + 0x0C;
		mRoomSprites[spriteNumber].Sprite_field_1B = 0x18;
		mRoomSprites[spriteNumber].Sprite_field_6 = 0x03;

	} else {
		// 35F1
		mMemory[ 0x780D + mCurrentPlayer ] = 0;
		mRoomSprites[spriteNumber].mX= mMemory[ word_40 ] + 6;
		mRoomSprites[spriteNumber].mY= mMemory[ word_40 + 1 ] + 0x0F;
	}

	// 360D
	mRoomSprites[spriteNumber].mWidth = 3;
	mRoomSprites[spriteNumber].mHeight = 0x11;
	mRoomSprites[spriteNumber].Sprite_field_1F = 0x80;
	mRoomSprites[spriteNumber].playerNumber = mCurrentPlayer;
	mRoomSprites[spriteNumber].spriteImageID= 0;
	mRoomSprites[spriteNumber].Sprite_field_19 = mRoomSprites[spriteNumber].Sprite_field_1A = mRoomSprites[spriteNumber].Sprite_field_18 = 0xFF;
}

void cCreep::roomMain() {

	roomLoad();
	mInput->inputCheck( true );

	for(byte X = 0; X < 2; ++X ) {
		
		if( mPlayerStatus[X] == 1 ) {
			mCurrentPlayer = X;
			obj_Player_Add();
		}
	}

	//14EA
	mMemory[ 0x15D7 ] = 1;
	mRestorePressed = false;

	for(;;) {

		events_Execute();
		hw_Update();

		// Do pause?
		if( mRunStopPressed == true ) {
			//150E
			do {
				interruptWait( 3 );

				hw_Update();
				KeyboardJoystickMonitor( 0 );						

			} while( !mRunStopPressed );

		}
		// 156B
		if( mRestorePressed == true ) {
			mRestorePressed = false;

			for(signed char X=1; X>=0 ; --X) {
				if( mMemory[ 0x780D + X ] == 0 ) {
					mMemory[ 0x780D + X ] = 2;
					byte Y = mMemory[ 0x34D1 + X ];
					mRoomSprites[Y].state |= SPR_ACTION_FLASH;
				}
			}
		}
		// 1594
		if( mMemory[ 0x780D ] == 0 ) {
			mMemory[ 0x7811 ] = 0;
			continue;
		}
		// 15A3
		if( mMemory[ 0x780E ] == 0 ) {
			mMemory[ 0x7811 ] = 1;
			continue;
		}
		
		byte X = 0;
s15B4:;
		if( (mMemory[ 0x780D + X ] == 5) || 
			(mMemory[ 0x780D + X ] == 6) )
			continue;

		++X;
		if( X < 2 )
			goto s15B4;

		break;
	}

	// Leaving Room
	// 15C4
	mMemory[ 0x15D7] = 0;

	for( byte X = 0x1E; X; --X ) {
		events_Execute();
		hw_Update();
	}
}

byte cCreep::sub_6009( byte pA ) {
	word_40 = readLEWord( &mMemory[ mRoomPtr + 4] );
	
	byte A = mMemory[ word_40 ];
	
	word_40 += (pA << 3) + 1;

	return A;	
}

// 2AA9
void cCreep::stringDraw() {
	byte gfxPosX, gfxPosY;

	gfxPosX = mTxtX_0 = mTextXPos;
	gfxPosY = mTxtY_0 = mTextYPos;
	
	if( mTextFont == 0 )
		mTextFontt = 1;
	else
		mTextFontt = mTextFont & 0x03;

	mMemory[0x73B5] = mTextFontt << 3;
	mMemory[0x73E8] = mMemory[0x73B5];

	word_30 = ((mMemory[0x73E8] << 1) + 0xEA);
	word_30 += 0x7300;

	for( signed char Y = 5; Y >= 0; --Y)
		mMemory[ word_30 + Y] = (mTextColor << 4);
	
	//2AFE
	for(;;) {
		byte X = mTextFont & 0x30;
		X >>= 3;

		word_30 = (mMemory[ mObjectPtr ] & 0x7F) << 3;
		word_30 += mMemory[ 0x2BE8 + X ];
		word_30 += (mMemory[ 0x2BE9 + X ] << 8);

		// Copy from Char ROM
		for( signed char count = 7; count >= 0; --count )
			mMemory[ 0x2BF0 + count ] = charRom( word_30 + count);

		word_30 = 0x73EA;

		// 2B50
		for(X = 0; X < 8; ++X) {

			byte Y = mMemory[ 0x2BF0 + X ];
			Y >>= 4;
			Y &= 0x0F;

			word tmp =  mMemory[ 0x2BF8 + Y ];
			Y = mMemory[ 0x2BF0 + X ] & 0xF;
			tmp += (mMemory[ 0x2BF8 + Y] << 8);
			
			writeLEWord(&mMemory[ word_30 ], tmp);

			byte A;

			if( mTextFontt < 2 ) {
				A = 2;

			} else {
				if( mTextFontt == 2 ) {
					mMemory[ word_30 + 2 ] = mMemory[ word_30 ];
					mMemory[ word_30 + 3 ] = mMemory[ word_30 + 1 ];

					A = 4;

				} else {
					mMemory[ word_30 + 2 ] = mMemory[ word_30 ];
					mMemory[ word_30 + 4 ] = mMemory[ word_30 ];
					mMemory[ word_30 + 3 ] = mMemory[ word_30 + 1 ];
					mMemory[ word_30 + 5 ] = mMemory[ word_30 + 1 ];

					A = 6;
				}
			}
			
			//2BAb
			word_30 += A;
		}

		screenDraw( 2, 0x95, gfxPosX, gfxPosY, 0x94 );

		if( ((char) mMemory[ mObjectPtr ]) < 0 )
			break;

		++mObjectPtr;
		gfxPosX += 8;
		mTxtX_0 = gfxPosX;
	}

	// 2BD7
	++mObjectPtr;
}

//2A6D
void cCreep::obj_stringPrint( ) {

	while( (mTextXPos = mMemory[ mObjectPtr ]) ) {
		mTextYPos = mMemory[ mObjectPtr + 1 ];
		mTextColor = mMemory[ mObjectPtr + 2 ];
		mTextFont = mMemory[ mObjectPtr + 3 ];

		mObjectPtr += 0x04;

		stringDraw( );
	}

	++mObjectPtr;
}

// 580D
void cCreep::screenDraw( word pDecodeMode, word pGfxID, byte pGfxPosX, byte pGfxPosY, byte pTxtCurrentID = 0 ) {
	byte gfxPosTopY;
	byte gfxHeight_0;

	byte gfxPosBottomY, gfxBottomY;
	byte gfxDestX;
	byte gfxDestX2;
	byte gfxPosRightX;
	byte gfxDestY;
	byte gfxCurPos;

	byte videoPtr0, videoPtr1;
	byte Counter2;
	byte drawingFirst = 0;
	byte byte_5CE2;

	mScreen->bitmapRedrawSet();

	if( pDecodeMode	!= 0 ) {
		// Draw Text
		word word_38 = pTxtCurrentID;
		
		word_38 <<= 1;
		word_38 += 0x603B;

		word_30 = mMemory[ word_38 ];
		word_30 += (mMemory[ word_38 + 1 ] << 8);
		
		mTxtWidth = mMemory[ word_30 ];
		mTxtHeight = mMemory[ word_30 + 1];
		mCount = mMemory[ word_30 + 1 ];

		mTxtPosLowerY = mTxtY_0 + mTxtHeight;
		--mTxtPosLowerY;

		if( mTxtX_0 < 0x10 ) {
			mTxtDestX = 0xFF;
			mTxtEdgeScreenX = 0xFF;
		} else {
			mTxtDestX = mTxtX_0 - 0x10;
			mTxtEdgeScreenX = 0;
		}		
		
		mTxtDestXLeft = mTxtDestX >> 2;
		mTxtDestXLeft |= (mTxtEdgeScreenX & 0xC0);
		
		mTxtEdgeScreenX <<= 1;
		if( mTxtDestX & 0x80 )
			mTxtEdgeScreenX |= 0x01;

		mTxtDestX <<= 1;
		mTxtDestXRight = mTxtDestXLeft + mTxtWidth;
		--mTxtDestXRight;
		drawingFirst = false;

		word_30 += 0x03;
	} 

	if( pDecodeMode != 1 ) {
		word byte_38;

		//58B7
		// Draw Graphics
		byte_38 = pGfxID << 1;
		byte_38 += 0x603B;
		
		word_32 = readLEWord( &mMemory[ byte_38 ] );
		
		mGfxWidth = mMemory[ word_32 ];
		mGfxHeight = mMemory[ word_32 + 1 ];
		gfxHeight_0 = mGfxHeight;

		//58ED
		gfxPosBottomY = pGfxPosY + mGfxHeight;
		--gfxPosBottomY;

		gfxDestX = (pGfxPosX - 0x10);

		if( pGfxPosX < 0x10 )
			mGfxEdgeOfScreenX = 0xFF;
		else
			mGfxEdgeOfScreenX = 0;

		gfxDestX2 = gfxDestX >> 2;
		gfxDestX2 |= (mGfxEdgeOfScreenX & 0xC0);
		
		mGfxEdgeOfScreenX <<= 1;
		if(gfxDestX & 0x80)
			mGfxEdgeOfScreenX |= 0x1;

		gfxDestX <<= 1;
		
		//592C
		gfxPosRightX = gfxDestX2 + mGfxWidth;
		--gfxPosRightX;
		
		Counter2 = 0;

		word_32 += 3;
		gfxDestY = pGfxPosY;

		if( pGfxPosY < 0xDC )
			byte_38 = 0;
		else
			byte_38 = 0xFF;

		// 595B
		gfxDestY >>= 1;
		if(byte_38 & 0x1) {
			gfxDestY |= 0x80;
		}
		byte_38 >>= 1;
		gfxDestY >>= 1;
		if(byte_38 & 0x1) {
			gfxDestY |= 0x80;
		}
		byte_38 >>= 1;
		gfxDestY >>= 1;
		if(byte_38 & 0x1) {
			gfxDestY |= 0x80;
		}
		byte_38 >>= 1;

		// 596A
		byte_5CE2 = mGfxHeight;
		--byte_5CE2;
		byte_5CE2 >>= 3;
		byte_5CE2 += gfxDestY;

		byte A = gfxDestY;

		if(gfxDestY >= 0x80) {
			A = 0;
			A -= gfxDestY;
		}
		
		videoPtr0 = mMemory[0x5CE6 + A];
		videoPtr1 = mMemory[0x5D06 + A];
		
		if( gfxDestY > 0x80 ) {
			signed char a = 0 - videoPtr0;
			videoPtr0 = a;
			
			a = 0 - videoPtr1;
			videoPtr1 = a;
		}

		// 59A8

		byte_38 = pGfxPosX - 0x10;
		if( pGfxPosX < 0x10 )
			A = 0xFF;
		else
			A = 0;

		byte byte38 = byte_38 & 0xFF, byte39 = ((byte_38 & 0xFF00) >> 8);

		byte39 = A;
		
		if( byte39 & 0x01 ) {
			byte38 |= 0x80;
		}
		byte38 >>= 1;
		
		if( byte39 & 0x01 ) {
			byte38 |= 0x80;
		}
		byte38 >>= 1;
		byte39 = A;

		// 59C6
		if((videoPtr0 + byte38) > 0xFF)
			++videoPtr1;

		videoPtr0 += byte38;
		videoPtr1 += byte39;

		if( pDecodeMode == 0 ) {
			gfxPosTopY = pGfxPosY;
			gfxBottomY = gfxPosBottomY;
		}
			
	}

	// 59F0
	if( pDecodeMode == 1 ) {

		gfxPosTopY = mTxtY_0;
		gfxBottomY = mTxtPosLowerY;

	} else if( pDecodeMode == 2 ) {
		byte A = 0;

		if( pGfxPosY != mTxtY_0 ) {
			if( pGfxPosY >= mTxtY_0 ) {
				
				if( pGfxPosY >= 0xDC || mTxtY_0 < 0xDC ) {
					 A = mTxtY_0;
				}  
			} else {
				// 5a17
				if( mTxtY_0 >= 0xDC && pGfxPosY < 0xDC ) {
					A = mTxtY_0;
				}
			}
		}

		//if(!A)
		//	A = pGfxPosY;
		
		// 5A2E
		gfxPosTopY = A;

		A = 0;

		if( gfxPosBottomY != mTxtPosLowerY ) {

			if( gfxPosBottomY >= mTxtPosLowerY ) {

				if( gfxPosBottomY >= 0xDC && mTxtPosLowerY < 0xDC ) {
					gfxBottomY = mTxtPosLowerY;
				}  else
					gfxBottomY = gfxPosBottomY;

			} else {

				// 5A49
				if( mTxtPosLowerY >= 0xDC && gfxPosBottomY < 0xDC ) {
					gfxBottomY = gfxPosBottomY;
				} else
					gfxBottomY = mTxtPosLowerY;
			}

		} else {
			//5a60
			gfxBottomY = mTxtPosLowerY;
		}
	}

	// 5A66
	byte gfxCurrentPosY = gfxPosTopY;
	word byte_36 = 0;
	word byte_34 = (*memory( 0xBC00 + gfxCurrentPosY ));

	word A = *memory( 0xBB00 + gfxCurrentPosY );

	byte_34 |= (A << 8);

	// 5A77
	for(;;) {
		
        // Bitmap: Overlay
		if( pDecodeMode != 0 && mCount != 0 ) {
			
			if( drawingFirst != 1 ) {
				if( mTxtY_0 == gfxCurrentPosY ) 
					drawingFirst = 1;
				else
					goto s5AED;
			}
			//5A97
			--mCount;

			if( gfxCurrentPosY < 0xC8 ) {
				
				gfxCurPos = mTxtDestXLeft;

				word word_36 = (byte_34 + mTxtDestX) + (mTxtEdgeScreenX << 8);


				// 5AB8
				for( byte Y = 0;; ++Y ) {
					if( gfxCurPos < 0x28 ) {
						
						byte A = mMemory[ word_30 + Y ];
						mMemory[ word_36 + Y] &= (A ^ 0xFF );
					} 
					// 5AC7
					if( gfxCurPos == mTxtDestXRight )
						break;
					
					word_36 += 0x7;
					++gfxCurPos;

				}
			}
			// 5AE1
			word_30 += mTxtWidth;
		}
s5AED:;

		//5aed
		if( pDecodeMode != 1 && gfxHeight_0 != 0) {
			if( Counter2 != 1 ) {
				if( gfxCurrentPosY == pGfxPosY ) {
					Counter2 = 1;
				} else {
					goto noCarry2;
				}
			}

			--gfxHeight_0;
			if( gfxCurrentPosY < 0xC8 ) {
				// 5b17
				gfxCurPos = gfxDestX2;
				byte_36 = byte_34 +  (mGfxEdgeOfScreenX << 8);
				byte_36 += gfxDestX;

				for( byte Y = 0; ; ++Y ) {
					// 5B2E
					if( gfxCurPos < 0x28 )
						*memory( byte_36 + Y ) |= mMemory[ word_32 + Y ];

					if( gfxCurPos == gfxPosRightX )
						break;

					//5B43
					byte_36 += 7;
					++gfxCurPos;
				}
			}

			//5B55
			word_32 += mGfxWidth;
		}
noCarry2:;
		//5B61
		if( gfxCurrentPosY == gfxBottomY ) 
			break;

		++gfxCurrentPosY;
		if(gfxCurrentPosY & 0x7 ) {
			++byte_34;
			continue;
		}

		byte_34 += 0x0139;
	}
	// 5B8C

	if(pDecodeMode == 1)
		return;

    //
    // Color 1
    //
	if( pGfxPosY & 0x7)
		mMemory[0x5CE5] = 1;
	else
		mMemory[0x5CE5] = 0;

	// 5BA4
	gfxCurrentPosY = gfxDestY;
	word_30 = videoPtr0;
	word_30 += (0xCC + videoPtr1) << 8;

	byte Y = 0;

	// 5BBC
	for( ;; ) {

		if( gfxCurrentPosY < 0x19 ) {
			
			for( Y = 0, gfxCurPos = gfxDestX2; gfxCurPos <= gfxPosRightX; ++gfxCurPos, ++Y ) {
			
				if( gfxCurPos < 0x28 )
					mMemory[ word_30 + Y ] = mMemory[ word_32 + Y ];
			}
		}

		// 5BE5
		if( gfxCurrentPosY != byte_5CE2 ) {
			++gfxCurrentPosY;
			
			word_32 += mGfxWidth;

		} else {
			// 5BFF
			if( mMemory[ 0x5CE5 ] != 1 )
				break;

			mMemory[ 0x5CE5 ] = 0;

			if( gfxCurrentPosY != 0xFF )
				if( gfxCurrentPosY >= 0x18 )
					break;
		}				
		// 5C16
		word_30 += 0x28;
	}
	
	// 5C24
    // Color 2
	word_32 += mGfxWidth;
	if( pGfxPosY & 0x7 )
		mMemory[ 0x5CE5 ] = 1;
	else
		mMemory[ 0x5CE5 ] = 0;

	gfxCurrentPosY = gfxDestY;
	word_30 = videoPtr0;
	word_30 += (0xD8 + videoPtr1) << 8;
	
	// 5C56
	for( ;; ) {
		
		if( gfxCurrentPosY < 0x19 ) {

			for(Y = 0, gfxCurPos = gfxDestX2; gfxCurPos <= gfxPosRightX; ++gfxCurPos, ++Y) {

				if( gfxCurPos < 0x28 ) 
					mMemory[ word_30 + Y ] = mMemory[ word_32 + Y ];
			}
		}
		//5C7F
		if( gfxCurrentPosY != byte_5CE2 ) {
			++gfxCurrentPosY;
			word_32 += mGfxWidth;
		} else {
		// 5C99
			if( mMemory[ 0x5CE5 ] != 1 )
				return;

			mMemory[ 0x5CE5 ] = 0;
			if( gfxCurrentPosY != 0xFF )
				if( gfxCurrentPosY >= 0x18 )
					return;
		}
		// 5CB0
		word_30 += 0x28;
	}
}

// 1203: 
void cCreep::mapRoomDraw() {
	byte byte_13EA, byte_13EB, roomX;
	byte roomY, roomHeight, roomWidth;

	mRoomPtr = 0x7900;
	
	byte gfxPosX;
	byte gfxPosY;

	//1210
	for(;;) {
		
		if( mMemory[ mRoomPtr ] & byte_8C1 )
			return;
		
		if( mMemory[ mRoomPtr ] & byte_8C0 ) {
			//1224
			
			mMemory[ 0x63E7 ] = mMemory[ mRoomPtr ] & 0xF;		// color
			roomX		= mMemory[ mRoomPtr + 1 ];				// top left x
			roomY		= mMemory[ mRoomPtr + 2 ];				// top left y
			roomWidth	= mMemory[ mRoomPtr + 3 ] & 7;			// width
			roomHeight	= (mMemory[ mRoomPtr + 3 ] >> 3) & 7;	// height

			gfxPosY = roomY;
			
			// Draw Room Floor Square
			// 1260
			for( byte_13EB = roomWidth; byte_13EB ; --byte_13EB) {
				
				gfxPosX = roomX;
				
				for(byte_13EA = roomHeight; byte_13EA; --byte_13EA) {
					screenDraw( 0, 0x0A, gfxPosX, gfxPosY, 0 );
					gfxPosX += 0x04;
				}

				gfxPosY += 0x08;
			}

			// 128B
			// Top edge of room
			mTxtX_0 = roomX;
			mTxtY_0 = roomY;
			

			for( byte_13EA = roomHeight; byte_13EA; --byte_13EA) {
				screenDraw(1, 0, 0, 0, 0x0B );
				mTxtX_0 += 0x04;
			}

			// 12B8
			// Bottom edge of room
			mTxtX_0 = roomX;
			mTxtY_0 = ((roomWidth << 3) + roomY) - 3;

			for( byte_13EA = roomHeight; byte_13EA; --byte_13EA) {
				screenDraw(1, 0, 0, 0, 0x0B );
				mTxtX_0 += 0x04;
			}

			//12E5
			// Draw Left Edge
			mTxtX_0 = roomX;
			mTxtY_0 = roomY;

			for( byte_13EA = roomWidth; byte_13EA; --byte_13EA) {
				screenDraw(1, 0, 0, 0, 0x0C );
				mTxtY_0 += 0x08;
			}

			//130D
			// Draw Right Edge
			mTxtX_0 = ((roomHeight << 2) + roomX) - 4;
			mTxtY_0 = roomY;

			for( byte_13EA = roomWidth; byte_13EA; --byte_13EA) {
				screenDraw(1, 0, 0, 0, 0x0D );
				mTxtY_0 += 0x08;
			}

			// 133E
			byte_13EA = sub_6009( 0 );
s1349:;
			if( byte_13EA )
				break;
		} 

		//134E
		mRoomPtr += 0x08;
	}

	//135C
	byte A = mMemory[ word_40 + 2 ];
	A &= 3;

	if( !( A & 3) ) {
		mTxtY_0 = roomY;
	} else {
		// 136D
		if( A == 2 ) {
			// 136F
			mTxtY_0 = (roomWidth << 3) + roomY;
			mTxtY_0 -= 3;

		} else {
			// 13A0
			mTxtY_0 = roomY + mMemory[ word_40 + 6 ];
			
			if( A != 3 ) {
				mTxtX_0 = ((roomHeight << 2) + roomX) - 4;
				A = 0x11;
			} else {
				// 13C5
				mTxtX_0 = roomX;
				A = 0x10;
			}
			
			goto s13CD;
		}
	}

	// 1381
	mTxtX_0 = A = roomX + mMemory[ word_40 + 5 ];

	A &= 2;
	
	if( A ) {
		A ^= mTxtX_0;
		mTxtX_0 = A;
		A = 0x0F;
	} else 
		A = 0x0E;
		
	// Draw Doors in sides
s13CD:;
	screenDraw( 1, 0, 0, 0, A );

	word_40 += 0x08;
	--byte_13EA;
	goto s1349;

}

void cCreep::obj_Image_Draw() {
	writeLEWord(&mMemory[ 0x6067 ], mObjectPtr);
	
	byte gfxDecodeMode = 0, gfxCurrentID = 0x16;

	word_32 = (mMemory[ mObjectPtr + 1 ] - 1) >> 3;
	++word_32;

	word_30 = 0;
	
	//1B18
	for( byte X =  mMemory[ mObjectPtr ]; X; --X) {
		word_30 += word_32;
	}

	// 1B2D
	word_30 <<= 1;

	for( byte X = mMemory[ mObjectPtr + 1 ]; X; --X) {
		word_30 += mMemory[ mObjectPtr ];
	}

	// 1B4D
	word_30 += 3;

	mObjectPtr += word_30;

	//1B67
	byte A;
	while( (A = mMemory[ mObjectPtr ]) ) {
		// 1B7D
		screenDraw( gfxDecodeMode, gfxCurrentID, A, mMemory[ mObjectPtr + 1 ], 0 );
			
		mObjectPtr += 2;
	}

	++mObjectPtr;
}

// 160A: Draw multiples of an object
void cCreep::obj_MultiDraw() {
	byte gfxCurrentID, gfxPosX, gfxPosY;
	signed char gfxRepeat;

	while( (gfxRepeat = mMemory[ mObjectPtr ]) != 0 ) {

		gfxCurrentID = mMemory[ mObjectPtr + 1 ];
		gfxPosX = mMemory[ mObjectPtr + 2 ];
		gfxPosY = mMemory[ mObjectPtr + 3 ];

		--gfxRepeat;

		// Draw an object a number of times,starting at a certain X/Y and repeating every X/Y
		for( ; gfxRepeat >= 0; --gfxRepeat ) {
		
			screenDraw( 0, gfxCurrentID, gfxPosX, gfxPosY );
		
			gfxPosX += mMemory[ mObjectPtr + 4 ];
			gfxPosY += mMemory[ mObjectPtr + 5 ];
		}

		mObjectPtr += 0x06;
	}
	
	++mObjectPtr;
}

void cCreep::hw_Update() {
	mScreen->refresh();
	mInput->inputCheck();
}

// 1935: Sleep for X amount of interrupts
void cCreep::hw_IntSleep( byte pA ) {

	for( byte X = 6; X > 0; --X ) {
		interruptWait( pA );
	}

}

// 1950: Player Escapes from the Castle
void cCreep::gameEscapeCastle() {
	
	screenClear();
	mMemory[ 0x0B72 ] = 6;
	if( mMemory[ 0x7802 ] & 0x80 ) {
		mObjectPtr = readLEWord( &mMemory[ 0x785F ] );
		roomPrepare();
	}

	byte A = mMemory[ 0x1AB2 ];
	A += 0x31;

	// Set player number in string 'Player Escapes'
	mMemory[ 0x1ABE ] = A;

	mObjectPtr = 0x1AB3;
	obj_stringPrint();
	mObjectPtr = 0x7855 + (mMemory[ 0x1AB2 ] << 2);
	convertTimerToTime();

	screenDraw(0, 0x93, 0x68, 0x18, 0 );
	// 19AF
	
	byte Y = mMemory[ 0x1AB2 ];
	byte X = mMemory[ 0x34D1 + Y ];
	mRoomSprites[X].mY= 0x87;
	mRoomSprites[X].mX= 0x08;

	A =	seedGet() & 0x0E;
	if( A != 0 )
		A = 0;
	else
		A = 8;

	mMemory[ 0x1AE5 ] = A;
	mMemory[ 0x1AE4 ] = 0;

	// 19DF
	for(;;) {
		A = mMemory[ 0x1AE4 ];

		if( A == 0 )  {
			byte Y = mMemory[ 0x1AE5 ];
			A = mMemory[ 0x1AD1 + Y ];
			if( A == 0 )
				break;

			// 19EF
			mMemory[ 0x1AE4 ] = A;
			mMemory[ 0x1AE3 ] = mMemory[ 0x1AD2 + Y ];
			mMemory[ 0x1AE5 ] += 0x02;
		}
		// 1A01
		if( mMemory[ 0x1AE3 ] >= 1 ) {
			if( mMemory[ 0x1AE3 ] != 1 ) {
				// 1A0A
				++mRoomSprites[X].spriteImageID;
				A = mRoomSprites[X].spriteImageID;
				if( A >= 0x9B || A < 0x97 ) {
					A = 0x97;
				}
			} else {
				// 1A33
				--mRoomSprites[X].mX;
				++mRoomSprites[X].spriteImageID;
				A = mRoomSprites[X].spriteImageID;
				if( A >= 3 )
					A = 0;
			}

		} else {
			// 1A1D
			++mRoomSprites[X].mX;
			++mRoomSprites[X].spriteImageID;
			A = mRoomSprites[X].spriteImageID;
			if( A >= 6 || A < 3 )
				A = 0x03;
		}

		// 1A42
		mRoomSprites[X].spriteImageID= A;
		Y = X;

		cSprite *sprite = mScreen->spriteGet( Y );

		// 1A4B
		mMemory[ 0x10 + Y ] = ((mRoomSprites[X].mX - 0x10) << 1) + 0x18;
		sprite->mX = ((mRoomSprites[X].mX - 0x10) << 1) + 0x18;
		
		// 1A72
		sprite->mY = mRoomSprites[X].mY + 0x32;

		hw_SpritePrepare( X );
		sprite->_rEnabled = true;

		if( mMemory[ 0x1AB2 ] )
			A = mMemory[ 0x34D4 ];
		else
			A = mMemory[ 0x34D3 ];

		sprite->_color = A;
		--mMemory[ 0x1AE4 ];

		// 1A95
		interruptWait(2);
		hw_Update();
	}

	// 1AA7
	hw_IntSleep(0xA);
}

// 1B9F
void cCreep::gameHighScoresHandle() {
	if( mMemory[ 0x7812 ] ) 
		word_30 = 0xB840;
	else
		word_30 = 0xB804;

	// 1BBE
	byte HighScorePosition = 0x0A;
	
	for(;;) {

		for(byte Y = 3;Y != 0; --Y) {
			// 1BC5
			byte A = mMemory[ word_30 + Y ];
			if( A < mMemory[ 0x1CF9 + Y ] ) {
				// 1BD1
				word_30 += 0x06;
				--HighScorePosition;
				break;
			}

			if( A != mMemory[ 0x1CF9 + Y ] )
				goto s1BE7;	
		}
	}

	// 1BE7
s1BE7:;
	byte Y;

	if( mMemory[ 0x7812 ] != 0 ) {
		Y = 0x73;
		mMemory[ 0x2788 ] = 0x68;
	} else {
		// 1BF8
		Y = 0x37;
		mMemory[ 0x2788 ] = 0x18;
	}
	// 1BFF
	byte A = 0x0A - HighScorePosition;
	A <<= 3;
	A += 0x38;
	mMemory[ 0x2789 ] = A;

	byte X = 0x0A - HighScorePosition;
	
	mMemory[ 0x278A ] = mMemory[ 0x1E85 + X ];

	writeLEWord(&mMemory[ 0x1D03 ], (word_30 - 2));
	
	for( ;HighScorePosition ; --HighScorePosition ) {

		mMemory[ 0x1CFF ] = 6;

		for( ; Y ; --Y ) 
			mMemory[ 0xB806 + Y ] = mMemory[ 0xB800 + Y ];
		
		//1C40
	}

	// 1C43
	for(Y = 3; Y != 0; --Y )
		mMemory[ word_30 + Y ] = mMemory[ 0x1CF9 + Y ];
	
	// 1C4D
	word_30 = readLEWord( &mMemory[ 0x1D03 ] );
	mMemory[ word_30 ] = 0;
	gameHighScores();

	// 1C60

	mObjectPtr = 0x1D05;
	X = mMemory[ 0x1CFD ];

	A = mMemory[ 0x1D01 + X ];
	mMemory[ 0x1D10 ] = A;
	obj_stringPrint();

	mMemory[ 0x278C ] = 3;
	mMemory[ 0x278B ] = 1;

	textShow();
	word_30 = readLEWord( &mMemory[ 0x1D03 ] );

	for(Y = 0; Y < 3; ++Y) {
		if( Y >= 3 )
			A = 0x20;
		else
			A = mMemory[ 0x278E + Y ];

		mMemory[ word_30 + Y ] = A;
	}
	
	// 1CA9
	X = mMemory[ 0x2399 ];
	Y = mMemory[ 0xBA01 + X ];

	A = mMemory[ 0x5CE6 + Y ];
	A += mMemory[ 0xBA00 + X ];

	word_30 = A + ((mMemory[ 0x5D06 + Y ] | 4) << 8);
	mMemory[ 0x28D6 ] = 0x59;

	for( signed char Y = 0x0E; Y >= 0; --Y ) {
		A = mMemory[ word_30 + Y ];

		if( (A & 0x7F) < 0x20 )
			A |= 0x40;

		mMemory[ 0x28D7 + Y ] = A;
	}

	// 1CD8
	mMemory[ 0x28D2 ] = mMemory[ 0xBA03 + X ];
	mMemory[ 0x28D1 ] = 2;
	
	// Save highscores
	mCastleManager->scoresSave( mCastle->nameGet(), readLEWord( memory( 0xB800 ) ), memory( 0xB800 ) );

	sub_2973();
}

// 1D42: Display Highscores for this Castle
void cCreep::gameHighScores( ) {
	screenClear();

	byte X = (mCastle->infoGet()->castleNumberGet() * 4) + 0x10;
	mMemory[ 0x2399 ] = X;

	byte Y = mMemory[ 0xBA01 + X ];
	byte A = mMemory[ 0x5CE6 + Y ];

	word_30 = (A + mMemory[ 0xBA00 + X ]) + ((mMemory[ 0x5D06 + Y] | 0x04) << 8);

	mMemory[ 0xBA03 + X ] = mCastle->nameGet().size() + 1;

	Y =  mMemory[ 0xBA03 + X ] - 2;

	// 1D67
	mMemory[ 0xB87A + Y ] = mMemory[ word_30 + Y ];

	// Convert name
	for( ; (char) Y >= 0; --Y )
		mMemory[ 0xB87A + Y ] = toupper(mCastle->nameGet()[Y]) & 0x3F;

	// Mark end of name
	mMemory[ 0xB87A + (mCastle->nameGet().size()-1) ] |= 0x80;


	mObjectPtr = 0xB87A;
	//1D81
	A = 0x15 - mMemory[ 0xBA03 + X ];

	A <<= 2;
	A += 0x10;
	
	mTextXPos = A;
	mTextYPos = 0x10;
	mTextColor = 0x01;
	mTextFont = 0x02;

	// Draw castle name
	stringDraw();

	X = 0;
	mTextXPos = 0x18;
	mTextFont = 0x21;

	// 1DAD
	for(;;) {
		mTextYPos = 0x38;
		
		for( mMemory[ 0xB889 ] = 0; mMemory[ 0xB889 ] < 0x0A; ++mMemory[ 0xB889 ]) {
			
			Y = mMemory[ 0xB889 ];
			mTextColor = mMemory[ 0x1E85 + Y ];
			A = mMemory[ 0xB802 + X ];
			if( A != 0xFF ) {
				mMemory[ 0xB87A ] = A;
				mMemory[ 0xB87B ] = mMemory[ 0xB803 + X ];
				A = *memory( 0xB804 + X );

			} else {
				// 1DD6
				mMemory[ 0xB87A ] = mMemory[ 0xB87B ] = A = 0x2E; 
			}

			// 1DDE
			mMemory[ 0xB87C ] = A | 0x80;

			mObjectPtr = 0xB87A; 
			stringDraw();

			if( mMemory[ 0xB802 + X ] != 0xFF ) {
				// 1DF5
				mObjectPtr = (X + 4) + 0xB800;
				convertTimerToTime();
				screenDraw( 0, 0x93, mTextXPos + 0x20, mTextYPos, 0x94 );
			}
				
			// 1E20
			mTextYPos += 0x08;
			X += 0x06;
		}
		
		// 1E3B
		if( mTextXPos != 0x18 )
			break;

		mTextXPos = 0x68;
	}

	// Draw BEST TIMES
	// 1E4A
	mObjectPtr = 0x1E5B;
	obj_stringPrint();
}

// 21C8
void cCreep::sound_PlayEffect( char pA ) {

	if( mIntro )
		return;

	if( mDisableSoundEffects == 1 )
		return;

	if( mPlayingSound >= 0 )
		return;

	mPlayingSound = pA;

	byte Y = mPlayingSound << 1;

	mSound->sidWrite( 0x04, 0 );
	mSound->sidWrite( 0x0B, 0 );
	mSound->sidWrite( 0x12, 0 );
	mSound->sidWrite( 0x17, 0 );

	mMusicBuffer = memory( readLEWord(memory( 0x7572 + Y )) );

	mSound->sidWrite( 0x18, 0x0F );

	mMemory[ 0xD404 ] = mMemory[ 0xD40B ] = mMemory[ 0xD412 ] = mMemory[ 0xD417 ] = 0;
	mMemory[ 0x20DC ] = mMemory[ 0x20DD ] = 0;
	mMemory[ 0xD418 ] = 0x0F;
	mMemory[ 0x2104 ] = 0x18;
	mMemory[ 0x2105 ] = 0x18;
	mMemory[ 0x2106 ] = 0x18;
	mMemory[ 0x2107 ] = 0x14;
	mMemory[ 0xDC05 ] = (mMemory[ 0x2107 ] << 2 ) | 3;
	mMemory[ 0xDC0D ] = 0x81;
	mMemory[ 0xDC0E ] = 0x01;

	mSound->playback(true);
}

void cCreep::obj_Walkway_Prepare() {
	byte CurrentX, Width, Length;
	byte gfxCurrentID, gfxPosX, gfxPosY;

	while( (Length = mMemory[ mObjectPtr ]) ) {
		
		gfxPosX = mMemory[ mObjectPtr + 1 ];
		gfxPosY = mMemory[ mObjectPtr + 2 ];
		
		byte_5FD5 = (gfxPosX >> 2);
		byte_5FD5 -= 4;

		byte_5FD6 = (gfxPosY >> 3);
		word_3C_Calculate();

		// 16A9
		
		for( CurrentX = 1; CurrentX <= Length; ++CurrentX ) {
			byte A;

			if( CurrentX != 1 ) {
				if( CurrentX != Length )
					A = 0x1C;
				else
					A = 0x1D;
			} else 
				A = 0x1B;

			// 16C1
			gfxCurrentID = A;
			screenDraw( 0, gfxCurrentID, gfxPosX, gfxPosY );
			
			// 16D1
			for( Width = 1; Width <= mGfxWidth; ++Width ) {
				
				if( CurrentX != 1 ) {
					
					if( CurrentX != Length )
						A = 0x44;

					else {
						// 16EE
						if( Width == mGfxWidth )
							A = 0x40;
						else
							A = 0x44;
					}

				} else {
					// 16E2
					if( Width == 1 )
						A = 0x04;
					else
						A = 0x44;
				}

				// 16F8
				A |= mMemory[ word_3C ];
				mMemory[ word_3C ] = A;
				
				
				word_3C += 2;
			}
			
			gfxPosX += (mGfxWidth << 2);
		}
		// 1732

		mObjectPtr += 3;
	}

	++mObjectPtr;
}

void cCreep::roomAnim_Disable( byte pSpriteNumber ) {

	if( !(mRoomAnim[pSpriteNumber ].mFlags & ITM_DISABLE) ) {

		mTxtX_0 = mRoomAnim[pSpriteNumber ].mX;
		mTxtY_0 = mRoomAnim[pSpriteNumber ].mY;
		
		screenDraw( 1, 0, 0, 0, mRoomAnim[pSpriteNumber ].mGfxID );
		
		mRoomAnim[pSpriteNumber ].mFlags |= ITM_DISABLE;
	}


}

byte cCreep::seedGet() {
	static byte byte_5EFB = 0x57, byte_5EFA = 0xC6, byte_5EF9 = 0xA0;
	
	byte A = byte_5EFB;
	bool ocf = false, cf = false;

	if( A & 0x01 )
		cf = true;
	
	A >>= 1;
	if(ocf)
		A |= 0x80;
	
	ocf = cf;
	cf = false;
	A = byte_5EFA;
	if( A & 0x01 )
		cf = true;
	A >>= 1;
	if(ocf)
		A |= 0x80;

	byte_5EF9 = A;
	A = 0;
	if(cf)
		A = 0x01;

	A ^= byte_5EFA;
	byte_5EFA = A;

	A = byte_5EF9;
	A ^= byte_5EFB;
	byte_5EFB = A;
	
	A ^= byte_5EFA;
	byte_5EFA = A;

	return A;
}

// 5D26: Prepare sprites
void cCreep::hw_SpritePrepare( byte pSpriteNumber ) {
	byte byte_5E8C, byte_5E8D;
	byte A;

	word word_38 = mRoomSprites[pSpriteNumber].spriteImageID;
	word_38 <<= 1;

	word_38 += 0x603B;
	
	word_30 = readLEWord( &mMemory[ word_38 ] );
	
	mRoomSprites[pSpriteNumber].spriteFlags = mMemory[ word_30 + 2 ];
	
	byte_5E8D = mMemory[ word_30 ];
	mRoomSprites[pSpriteNumber].mCollisionWidth = byte_5E8D << 2;
	mRoomSprites[pSpriteNumber].mCollisionHeight = mMemory[ word_30 + 1 ];
	
	byte Y = pSpriteNumber;
	// 5D72
	
	word_32 = mMemory[ 0x26 + Y ] ^ 8;
	word_32 <<= 6;
	word_32 += 0xC000;

	word_30 += 0x03;
	byte_5E8C = 0;
	
	// 5DB2
	for(;;) {

		for(Y = 0; Y < 3; ++Y ) {
			
			if( Y < byte_5E8D )
				A = mMemory[ word_30 + Y ];
			else
				A = 0;

			mMemory[ word_32 + Y ] = A;
		}

		++byte_5E8C;
		if( byte_5E8C == 0x15 )
			break;

		if( byte_5E8C < mRoomSprites[pSpriteNumber].mCollisionHeight ) 
			word_30 += byte_5E8D;
		else 
			word_30 = 0x5E89;
		
		// 5DED
		word_32 += 0x03;
	}

	// 5DFB
	Y = pSpriteNumber;
	
	cSprite *sprite = mScreen->spriteGet(pSpriteNumber);

	word dataSrc = mMemory[ 0x26 + Y ] ^ 8;
	dataSrc <<= 6;
	dataSrc += 0xC000;

	mMemory[ 0x26 + Y ] = mMemory[ 0x26 + Y ] ^ 8;

	// Sprite Color
	sprite->_color = mRoomSprites[pSpriteNumber].spriteFlags & 0x0F;

	if( !(mRoomSprites[pSpriteNumber].spriteFlags & SPRITE_DOUBLEWIDTH )) {
		A = (mMemory[ 0x2F82 + Y ] ^ 0xFF) & mSprite_X_Expansion;
		sprite->_rDoubleWidth = false;
	} else {
		A = (mSprite_X_Expansion | mMemory[ 0x2F82 + Y ]);
		mRoomSprites[pSpriteNumber].mCollisionWidth <<= 1;
		sprite->_rDoubleWidth = true;
	}

	// Sprite X Expansion
	mSprite_X_Expansion = A;
	
	// 5E2D
	if( !(mRoomSprites[pSpriteNumber].spriteFlags & SPRITE_DOUBLEHEIGHT )) {
		A = (mMemory[ 0x2F82 + Y ] ^ 0xFF) & mSprite_Y_Expansion;
		sprite->_rDoubleHeight = false;
	} else {
		A = (mSprite_Y_Expansion | mMemory[ 0x2F82 + Y ]);
		mRoomSprites[pSpriteNumber].mCollisionHeight <<= 1;
		sprite->_rDoubleHeight = true;
	}

	// Sprite Y Expansion
	mSprite_Y_Expansion = A;

	// 5E4C
	if( !(mRoomSprites[pSpriteNumber].spriteFlags & SPRITE_PRIORITY )) {
		A = mSprite_DataPriority | mMemory[ 0x2F82 + Y ];
		sprite->_rPriority = true;
	} else {
		A = (mMemory[ 0x2F82 + Y ] ^ 0xFF) & mSprite_DataPriority; 
		sprite->_rPriority = false;
	}

	// Sprite data priority
	mSprite_DataPriority = A;

	// 5E68
	if(! (mRoomSprites[pSpriteNumber].spriteFlags & SPRITE_MULTICOLOR )) {
		A = mSprite_Multicolor_Enable | mMemory[ 0x2F82 + Y ];
		sprite->_rMultiColored = true;
	} else {
		A = (mMemory[ 0x2F82 + Y ] ^ 0xFF) & mSprite_Multicolor_Enable;
		sprite->_rMultiColored = false;
	}
		
	// MultiColor Enable
	mSprite_Multicolor_Enable = A;

	sprite->streamLoad( &mMemory[ dataSrc ] );
	mScreen->spriteRedrawSet();
}

void cCreep::stringSet( byte pPosX, byte pPosY, byte pColor, string pMessage ) {
	memcpy( &mMemory[ 0xB906 ], pMessage.c_str(), pMessage.size() );
 
	mMemory[ 0xB900 ] = 0x6D;
    mMemory[ 0xB901 ] = 0x2A;
    mMemory[ 0xB902 ] = pPosX;
	mMemory[ 0xB903 ] = pPosY;
	mMemory[ 0xB904 ] = pColor;
    mMemory[ 0xB905 ] = 0x22;

    mMemory[ 0xB905 + pMessage.size() ] |= 0x80;
	mMemory[ 0xB906 + pMessage.size() ] = 0;
	mMemory[ 0xB907 + pMessage.size() ] = 0;
}

// 25B8
void cCreep::gameFilenameGet( bool pLoading, bool pCastleSave ) {
	
	screenClear();
	mInput->inputCheck(true);

	mObjectPtr = 0x2633;
	roomPrepare();
	
	if( pLoading )
		mObjectPtr = 0x261F;
	else {

		if(!pCastleSave)
			mObjectPtr = 0x2609;
		else {
			stringSet( 0x34, 0x00, 0x01, "SAVE CASTLE" );
			mObjectPtr = 0xB900;
		}
	}

	roomPrepare();
	*memory( 0x2788 ) = 0x20;
	*memory( 0x2789 ) = 0x48;
	*memory( 0x278C ) = 0x10;
	*memory( 0x278A ) = 0x01;
	*memory( 0x278B ) = 0x02;

	textShow();
}

// 24A7
void cCreep::gamePositionLoad() {
	
	gameFilenameGet( true, false );
	if(!mStrLength)
		return;

	string filename = string( (char*) &mMemory[ 0x278E ], mStrLength );
	
	if( mCastleManager->positionLoad( filename, memory( 0x7800 ) ) == true)
		mSaveGameLoaded = 1;

	sub_2973();
}

// 24FF: Save a game, or a castle
void cCreep::gamePositionSave( bool pCastleSave ) {
	
	gameFilenameGet( false, pCastleSave );
	if(!mStrLength)
		return;

	string filename = string( (char*) &mMemory[ 0x278E ], mStrLength );
	// Save from 0x7800

	word saveSize = readLEWord( memory( 0x7800 ) );
	
	// Castles save with a Z
	if( pCastleSave )
		filename.insert(0, "Z" );

	bool result = false;

	if( pCastleSave )
		result = mCastleManager->castleSave( filename, saveSize, memory( 0x7800 ) );
	else
		result = mCastleManager->positionSave( filename, saveSize, memory( 0x7800 ) );

	if( result == false) {

		mObjectPtr = 0x25AA;	// IO ERROR
		screenClear();
		obj_stringPrint();

		hw_Update();
		hw_IntSleep(0x23);
	} else
		sub_2973();

}

void cCreep::positionCalculate( byte pSpriteNumber ) {

	byte_5FD5 = mRoomSprites[pSpriteNumber].mX + mRoomSprites[pSpriteNumber].mWidth;
	
	byte_5FD7 = byte_5FD5 & 3;
	byte_5FD5 = (byte_5FD5 >> 2) - 4;

	byte_5FD6 = mRoomSprites[pSpriteNumber].mY + mRoomSprites[pSpriteNumber].mHeight;
	byte_5FD8 = byte_5FD6 & 7;
	byte_5FD6 >>= 3;
	
	word_3C_Calculate();
}

void cCreep::word_3C_Calculate() {
	// 5fa6
	word_3C = mMemory[ 0x5CE6 + byte_5FD6 ];
	word_3C += mMemory[ 0x5D06 + byte_5FD6 ] << 8;

	word_3C <<= 1;
	word_3C += 0xC000;

	byte A = byte_5FD5 << 1;
	word_3C += A;
}

void cCreep::obj_SlidingPole_Prepare() {
	byte Height;
	byte A, gfxPosX, gfxPosY;

	while( (Height = mMemory[ mObjectPtr ]) ) {

		gfxPosX = mMemory[ mObjectPtr + 1 ];
		gfxPosY = mMemory[ mObjectPtr + 2 ];
		
		byte_5FD5 = (gfxPosX >> 2) - 0x04;
		byte_5FD6 = (gfxPosY >> 3);

		word_3C_Calculate();

		//1781
		for(;;) {
			if (mMemory[ word_3C ]  & 0x44) {
				mTxtX_0 = gfxPosX - 4;
				mTxtY_0 = gfxPosY;
				
				screenDraw( 2, 0x27, gfxPosX, gfxPosY, 0x25 );
			} else {
				// 17AA
				screenDraw( 0, 0x24, gfxPosX, gfxPosY );
			}

			A = mMemory[ word_3C ];
			A |= 0x10;
			mMemory[ word_3C ] = A;

			--Height;
			if( !Height ) {
				mObjectPtr += 0x03;
				break;
			}
			
			gfxPosY += 0x08;
			word_3C += 0x50;
		}
	}

	++mObjectPtr;
}

void cCreep::obj_Ladder_Prepare() {
	byte Height, gfxPosX, gfxPosY;
	
	while( (Height = mMemory[ mObjectPtr ]) ) {

		// 1800
		gfxPosX = mMemory[ mObjectPtr + 1 ];
		gfxPosY = mMemory[ mObjectPtr + 2 ];

		byte A = gfxPosX >> 2;
		A -= 0x04;

		byte_5FD5 = A;
		A = (gfxPosY >> 3);
		byte_5FD6 = A;

		word_3C_Calculate();

		// 1828
		for(;;) {
			if( (mMemory[ word_3C ] & 0x44) == 0 ) {
				byte  gfxCurrentID;

				if( Height != 1 ) 
					gfxCurrentID = 0x28;
				else
					gfxCurrentID = 0x2B;

				screenDraw( 0, gfxCurrentID, gfxPosX, gfxPosY, 0 );

			} else {
				// 184C
				if( Height == 1 ) {
					mTxtX_0 = gfxPosX;
					mTxtY_0 = gfxPosY;

					screenDraw( 2, 0x29, gfxPosX, gfxPosY, 0x2A );
				} else {
				// 1874
					gfxPosX -= 4;
					mTxtX_0 = gfxPosX;
					mTxtY_0 = gfxPosY;
					screenDraw( 2, 0x2C, gfxPosX, gfxPosY, 0x2D );
					gfxPosX += 4;
				}
			}
			// 189C
			if( Height != mMemory[ mObjectPtr ] )
				mMemory[ word_3C ] = ( mMemory[ word_3C ] | 1);
			
			--Height;
			if( !Height ) { 
				mObjectPtr += 3;
				break;
			}

			mMemory[ word_3C ] |= 0x10;
			gfxPosY += 8;
		
			word_3C += 0x50;
		}
	}

	++mObjectPtr;
}

// 3FD5: Door Opening
void cCreep::obj_Door_Execute( byte pObjectNumber ) {

	if( mRoomObjects[pObjectNumber].Object_field_1 == 0 ) {
		mRoomObjects[pObjectNumber].Object_field_1 = 1;
		mRoomObjects[pObjectNumber].Object_field_2 = 0x0E;

		word_40 = (mRoomObjects[pObjectNumber].objNumber << 3) + mRoomDoorPtr;
		
		mMemory[ word_40 + 2 ] |= 0x80;
		byte A = mMemory[ word_40 + 4 ];

		roomPtrSet( mMemory[ word_40 + 3 ] );
		sub_6009( A );
		mMemory[ word_40 + 2 ] |= 0x80;
	}
	// 4017
	byte A = 0x10;
	A -= mRoomObjects[pObjectNumber].Object_field_2;
	mMemory[ 0x75B7 ] = A;

	sound_PlayEffect( 3 );
	A = mRoomObjects[pObjectNumber].Object_field_2;

	if( A ) {
		--mRoomObjects[pObjectNumber].Object_field_2;
		A += mRoomAnim[pObjectNumber].mY; 
		mTxtY_0 = A;
		mTxtX_0 = mRoomAnim[pObjectNumber].mX;
		screenDraw( 1, 0, 0, 0, 0x7C );
		return;
	}
	mRoomAnim[pObjectNumber].mFlags ^= ITM_EXECUTE;
	for(char Y = 5; Y >= 0; --Y ) 
		mMemory[ 0x6390 + Y ] = mRoomObjects[pObjectNumber].color;

	Draw_RoomAnimObject( 0x08, mRoomAnim[pObjectNumber].mX, mRoomAnim[pObjectNumber].mY, 0, pObjectNumber );
}

// 4075: In Front Door
void cCreep::obj_Door_InFront( byte pSpriteNumber, byte pObjectNumber ) {
	if( mRoomObjects[pObjectNumber].Object_field_1 == 0 )
		return;

	if( mRoomSprites[pSpriteNumber].Sprite_field_0 )
		return;

	// 4085
	if( mRoomSprites[pSpriteNumber].Sprite_field_1E != 1 )
		return;
	
	byte playerNumber = mRoomSprites[pSpriteNumber].playerNumber;

	if( mMemory[ 0x780D + playerNumber ] != 0 )
		return;

	// Enter the door
	mMemory[ 0x780D + playerNumber ] = 6;
	mRoomSprites[pSpriteNumber].Sprite_field_1B = 0;
	mRoomSprites[pSpriteNumber].Sprite_field_6 = 3;
	
	byte A = mRoomObjects[pObjectNumber].objNumber;
	A <<= 3;

	word_40 = mRoomDoorPtr + A;

	// 40BB

	mRoomSprites[pSpriteNumber].mY = mMemory[ word_40 + 1 ] + 0x0F;
	mRoomSprites[pSpriteNumber].mX = mMemory[ word_40 ] + 0x06;
	if( mMemory[ word_40 + 7 ] != 0 )
		mMemory[ 0x785D + mRoomSprites[pSpriteNumber].playerNumber ] = 1;

	//40DD
	word word_41D6 = readLEWord( &mMemory[ word_40 + 3 ] );
	roomPtrSet( (word_41D6 & 0xFF) );
	
	mMemory[ mRoomPtr ] |= byte_8C0;

	playerNumber = mRoomSprites[pSpriteNumber].playerNumber;

	// Set player room / door
	mMemory[ 0x7809 + playerNumber ] = word_41D6 & 0xFF;
	mMemory[ 0x780B + playerNumber ] = (word_41D6 & 0xFF00) >> 8;
}

// 4F5C: Load the rooms' Teleports
void cCreep::obj_Teleport_Prepare() {

	mTxtX_0 = mMemory[ mObjectPtr ];
	mTxtY_0 = mMemory[ mObjectPtr + 1 ] + 0x18;

	for(byte byte_50D0 = 3; byte_50D0 > 0; --byte_50D0) {
		screenDraw( 1, 0, 0, 0, 0x1C );
		
		mTxtX_0 += 0x04;
	}

	byte gfxPosX = mMemory[ mObjectPtr ];
	byte gfxPosY = mMemory[ mObjectPtr + 1 ];

	// Draw the teleport unit
	screenDraw( 0, 0x6F, gfxPosX, gfxPosY, 0 );

	//4fad
	gfxPosX += 0x0C;
	gfxPosY += 0x18;
	screenDraw( 0, 0x1C, gfxPosX, gfxPosY, 0 );

	byte X;
	object_Create( X );
	
	mRoomAnim[X].mFuncID = 0x0A;
	gfxPosX = mMemory[ mObjectPtr ] + 4;
	gfxPosY = mMemory[ mObjectPtr + 1 ] + 0x18;

	mRoomObjects[X].objNumber = (mObjectPtr & 0xFF);
	mRoomObjects[X].Object_field_1 = (mObjectPtr & 0xFF00) >> 8;
	
	Draw_RoomAnimObject( 0x70, gfxPosX, gfxPosY, 0, X );

	obj_Teleport_SetColour( (mMemory[ mObjectPtr + 2 ] + 2), X );
	
	byte A = 0x20;

    // Draw the Teleport Sources
	while( mMemory[ mObjectPtr + 3 ] ) {
		mMemory[ 0x6E95 ] = mMemory[ 0x6E96 ] = mMemory[ 0x6E97 ] = mMemory[ 0x6E98 ] = A;
		gfxPosX = mMemory[ mObjectPtr + 3 ];
		gfxPosY = mMemory[ mObjectPtr + 4 ];

		screenDraw( 0, 0x72, gfxPosX, gfxPosY, 0 );
		
		mObjectPtr += 2;

		A += 0x10;
	}

	mObjectPtr += 0x04;
}

// 475E: Mummy Releasing
void cCreep::obj_Mummy_Tomb_Execute( byte pObjectNumber ) {
	if( mEngine_Ticks & 3 )
		return;

	byte A;

	--mRoomObjects[pObjectNumber].Object_field_1;
	if( mRoomObjects[pObjectNumber].Object_field_1 == 0 ) {
		mRoomAnim[pObjectNumber].mFlags ^= ITM_EXECUTE;
		A = 0x66;
	} else {
		// 4776
		if( mRoomObjects[pObjectNumber].Object_field_2 == 0x66 )
			A = 0x11;
		else
			A = 0x66;
	}
	
	// 4784
	for(char Y = 5; Y >= 0; --Y)
		mMemory[ 0x68F0 + Y ] = A;

	// 478C
	mRoomObjects[pObjectNumber].Object_field_2 = A;
	Draw_RoomAnimObject( mRoomAnim[pObjectNumber].mGfxID, mRoomAnim[pObjectNumber].mX, mRoomAnim[pObjectNumber].mY, 0, pObjectNumber );
}

// 4B1A: 
void cCreep::obj_RayGun_Execute( byte pObjectNumber ) {
	byte gfxPosX = 0;
	byte gfxPosY = 0;
	byte Y = 0;

	if( mEngine_Ticks & 3 )
		return;

	word_40 = mRoomRayGunPtr + mRoomObjects[pObjectNumber].objNumber;

	byte A = mRoomAnim[pObjectNumber].mFlags;
	if(!( A & ITM_DISABLE )) {
		if( mIntro == 1 || mNoInput )
			return;

		// 4B46
		if(!(mMemory[ word_40 ] & byte_4D62) ) {
			mRaygunCount = 0xFF;
			byte_4D5E = 0x00;
			

			for(byte_4D5F = 0x01; ((char) byte_4D5F) >= 0; --byte_4D5F) {

				if( mMemory[ 0x780D + byte_4D5F ] == 0 ) {
					byte Y = mMemory[ 0x34D1 + byte_4D5F ];
					char A = mRoomSprites[Y].mY;
					A -= mRoomAnim[pObjectNumber].mY;
					if( A < 0 )
						A = (A ^ 0xFF) + 1;

					if( A < mRaygunCount ) {
						mRaygunCount = A;
						byte A = mRoomSprites[Y].mY;

						if( A >= 0xC8 || A < mRoomAnim[pObjectNumber].mY ) {
							byte_4D5E = byte_4D65;	// Will Move Up
						} else {
							byte_4D5E = byte_4D66;	// Will Move Down
						}
					}
				}
				//4B9C
			}
			// 4BA1
			A = 0xFF;
			A ^= byte_4D65;
			A ^= byte_4D66;

			A &= mMemory[ word_40 ];
			A |= byte_4D5E;
			mMemory[ word_40 ] = A;
		}
		//4BB2
		A = mMemory[ word_40 ];

		if( A & byte_4D65 ) {

			A = mMemory[ word_40 + 4 ];

			// Can RayGun Move Up
			if( A != mMemory[ word_40 + 2 ] ) {
				mMemory[ word_40 + 4 ] = A - 1;
				obj_RayGun_Control_Update( 0x5C );
			} else
				goto s4BD9;

		} else {
			// 4BD4
			if( !(A & byte_4D66) ) {
s4BD9:;
				obj_RayGun_Control_Update( 0xCC );
				goto s4C27;
			}
			// 4BE1
			A = mMemory[ word_40 + 4 ];

			// Can Raygun Move Down
			if( A >= mRoomObjects[pObjectNumber].Object_field_1 )
				goto s4BD9;

			mMemory[ word_40 + 4 ] = A + 1;
			obj_RayGun_Control_Update( 0xC2 );
		}	
	}
	// 4BF4
	gfxPosX = mRoomAnim[pObjectNumber].mX;
	gfxPosY = mMemory[ word_40 + 4 ];

	A = mMemory[ word_40 ];
	if( A & byte_4D67 )
		A = 4;
	else
		A = 0;

	byte_4D5E = A;
	Y = mMemory[ word_40 + 4 ] & 3;
	Y |= byte_4D5E;

	// Draw the ray gun
	Draw_RoomAnimObject( mMemory[ 0x4D68 + Y ], gfxPosX, gfxPosY, 0, pObjectNumber );

s4C27:;
	A = mMemory[ word_40 ];

	if( A & byte_4D62 ) {
		A ^= byte_4D62;
		mMemory[ word_40 ] = A;

		if( !(A & byte_4D64 ))
			return;
	} else {
		// 4C3D
		if( mRaygunCount >= 5 )
			return;
	}

	// Fire Weapon

	// 4C44
	A = mMemory[ word_40 ];
	if( (A & byte_4D61) )
		return;

	obj_RayGun_Laser_Sprite_Create( pObjectNumber );

	A |= byte_4D61;
	mMemory[ word_40 ] = A;
}

// 4E32: Teleport?
void cCreep::obj_Teleport_Execute( byte pObjectNumber ) {
	if( mEngine_Ticks & 1 )
		return;

	byte A = seedGet();
	A &= 0x3F;
	
	mMemory[ 0x75CD + 2 ] = A;
	sound_PlayEffect( 0x04 );
	if( mEngine_Ticks & 3  )
		A = 1;
	else
		A = mRoomObjects[pObjectNumber].Object_field_2;

	A <<= 4;
	mMemory[ 0x6E95 ] = mMemory[ 0x6E96 ] = mMemory[ 0x6E97 ] = mMemory[ 0x6E98 ] = A;
	byte gfxPosX = mRoomObjects[pObjectNumber].Object_field_4;
	byte gfxPosY = mRoomObjects[pObjectNumber].Object_field_5;
	screenDraw( 0, 0x72, gfxPosX, gfxPosY, 0 );

	if( mEngine_Ticks & 3 ) 
		A = 0;
	else
		A = mRoomObjects[pObjectNumber].Object_field_2;

	obj_Teleport_SetColour(A, pObjectNumber);
	if( mEngine_Ticks & 3 )
		return;
	
	--mRoomObjects[pObjectNumber].color;
	if( mRoomObjects[pObjectNumber].color )
		return;

	mRoomAnim[pObjectNumber].mFlags ^= ITM_EXECUTE;
}

// 4C58: Load the rooms' Ray Guns
void cCreep::obj_RayGun_Prepare() {

	mRoomRayGunPtr = mObjectPtr;
	mRaygunCount = 0;

	do {
		// 4C7E

		mMemory[ mObjectPtr ] &=( 0xFF ^ byte_4D61);
		byte gfxPosX = mMemory[ mObjectPtr + 1 ];
		byte gfxPosY = mMemory[ mObjectPtr + 2 ];
		byte gfxCurrentID;

		if( mMemory[ mObjectPtr ] & byte_4D67 )
			gfxCurrentID = 0x5F;
		else
			gfxCurrentID = 0x60;

		// Length
		for( byte_4D5E = mMemory[ mObjectPtr + 3 ]; byte_4D5E; --byte_4D5E ) {
			
			screenDraw( 0, gfxCurrentID, gfxPosX, gfxPosY, 0 );
			gfxPosY += 0x08;
		}

		// 4CCB
		if(!( mMemory[ mObjectPtr ] & byte_4D63 )) {
			byte X;

			object_Create( X );
			
			mRoomAnim[X].mFuncID = 8;
			mRoomObjects[X].objNumber = mRaygunCount;
			mRoomAnim[X].mFlags |= ITM_EXECUTE;
			
			byte A = mMemory[ mObjectPtr + 3 ];
			A <<= 3;
			A += mMemory[ mObjectPtr + 2 ];
			A -= 0x0B;
			mRoomObjects[X].Object_field_1 = A;
			// 4D01
			if( !(mMemory[ mObjectPtr ] & byte_4D67) ) {
				A = mMemory[ mObjectPtr + 1 ];
				A += 4;
			} else {
				A = mMemory[ mObjectPtr + 1 ];
				A -= 8;
			}
			mRoomAnim[X].mX = A;
		} 
		
		// 4D1A
		byte X;

		object_Create( X );
		mRoomAnim[X].mFuncID = 9;
		gfxPosX = mMemory[ mObjectPtr + 5 ];
		gfxPosY = mMemory[ mObjectPtr + 6 ];

		Draw_RoomAnimObject( 0x6D, gfxPosX, gfxPosY, 0, X );

		mRoomObjects[X].objNumber = mRaygunCount;

		mObjectPtr += 0x07;
		mRaygunCount += 0x07;

	} while( !(mMemory[ mObjectPtr ] & byte_4D60) );

	++mObjectPtr;
}

// 49F8: Load the rooms' Keys
void cCreep::obj_Key_Load() {
	mRoomKeyPtr = mObjectPtr;

	byte KeyID = 0;
	
	do {

		if( mMemory[ mObjectPtr + 1 ] != 0 ) {
			byte X;
			
			object_Create( X );
			mRoomAnim[X].mFuncID = 6;

			byte gfxPosX = mMemory[ mObjectPtr + 2 ];
			byte gfxPosY = mMemory[ mObjectPtr + 3 ];
			byte gfxCID = mMemory[ mObjectPtr + 1 ];

			mRoomObjects[X].objNumber = KeyID;

			Draw_RoomAnimObject( gfxCID, gfxPosX, gfxPosY, 0, X );
		}

		// 4A47
		KeyID += 0x04;
		mObjectPtr += 0x04;

	} while( mMemory[ mObjectPtr ] != 0 );

	++mObjectPtr;
}

void cCreep::obj_Door_Lock_Prepare() {
	
	byte X, gfxPosX, gfxPosY;

	for( ; mMemory[ mObjectPtr ]; mObjectPtr += 0x05) {
		
		object_Create( X );
		
		mRoomAnim[X].mFuncID = 7;
		gfxPosX = mMemory[ mObjectPtr + 3 ];
		gfxPosY = mMemory[ mObjectPtr + 4 ];
		
		byte A = (mMemory[ mObjectPtr ] << 4);
		A |= mMemory[ mObjectPtr ];

		for( signed char Y = 8; Y >= 0; --Y )
			mMemory[ 0x6C53 + Y ] = A;

		mRoomObjects[X].objNumber = *level( mObjectPtr );
		mRoomObjects[X].Object_field_1 = *level( mObjectPtr + 2 );
		Draw_RoomAnimObject( 0x58, gfxPosX, gfxPosY, 0, X );
	}

	++mObjectPtr;
}

void cCreep::obj_Door_Prepare() {
	byte DoorCount = *level(mObjectPtr++);
	mRoomDoorPtr = mObjectPtr;
	
	byte X, gfxCurrentID, gfxPosX, gfxPosY;

	for( byte count = 0; count != DoorCount; ++count) {
		X = *level(mObjectPtr + 7);
		
		gfxCurrentID  = mMemory[ 0x41D1 + X ];
		gfxPosX = *level( mObjectPtr + 0 );
		gfxPosY = *level( mObjectPtr + 1 );

		screenDraw( 0, gfxCurrentID, gfxPosX, gfxPosY );
		// 4159

		object_Create( X );
		
		gfxPosX += 0x04;
		gfxPosY += 0x10;

		mRoomObjects[X].objNumber = count;
		mRoomAnim[X].mFuncID = 0;

		roomPtrSet( *level( mObjectPtr + 3 ) );
		
		byte A =  (*level( mRoomPtr ) & 0xF);

		mRoomObjects[X].color = A;
		A <<= 4;

		A |= mRoomObjects[X].color;
		mRoomObjects[X].color = A;
		
		A = *level( mObjectPtr + 2 );
		if(A & 0x80) {
			mRoomObjects[X].Object_field_1 = 1;
			A = mRoomObjects[X].color;

			for(byte Y = 5; Y; --Y ) 
				mMemory[ 0x6390 + Y ] = A;

			A = 0x08;
		} else
			A = 0x07;

		// 41B2
		gfxCurrentID = A;
		Draw_RoomAnimObject( gfxCurrentID, gfxPosX, gfxPosY, 0, X );

		mObjectPtr += 0x08;
	}

}

// 42AD: Lightning Machine pole movement
void cCreep::obj_Lightning_Pole_Execute( byte pObjectNumber ) {
	byte gfxPosX, gfxPosY;

	byte byte_43E2, byte_43E3;
	word_40 = mRoomLightningPtr + mRoomObjects[pObjectNumber].objNumber;
	byte Y = 0;

	if( mRoomObjects[pObjectNumber].Object_field_1 != 1 ) {
		mRoomObjects[pObjectNumber].Object_field_1 = 1;

		obj_Lightning_Sprite_Create( pObjectNumber );

	} else {
		// 42CF
		if( !(mMemory[ word_40 + Y ] & byte_45DE )) {
			// 42D8
			mRoomObjects[pObjectNumber].Object_field_1 = 0;
			mRoomAnim[pObjectNumber].mFlags ^= ITM_EXECUTE;
			mMemory[ 0x66E6 ] = mMemory[ 0x66E7 ] = 0x55;

			gfxPosX = mMemory[ word_40 + 1 ];
			gfxPosY = mMemory[ word_40 + 2 ];
			
			for( byte_43E2 = mMemory[ word_40 + 3 ]; byte_43E2; --byte_43E2 ) {

				screenDraw( 0, 0x34, gfxPosX, gfxPosY, 0 );
				gfxPosY += 0x08;
			}

			// 4326
			for( Y = 0; ;Y++ ) {

				if( mRoomSprites[Y].Sprite_field_0 == 1 ) {
					if( !(mRoomSprites[Y].state & SPR_FLAG_FREE) )
						if( mRoomSprites[Y].Sprite_field_1F == mRoomObjects[pObjectNumber].objNumber )
							break;
				}
			}

			// 4345
			mRoomSprites[Y].state |= SPR_ACTION_DESTROY;			// Turning Off 
            mScreen->clear(0);
            mScreen->refresh();
			return;

		} else {
			// 4351
			if( mEngine_Ticks & 3 )
				return;
		}
	}
	// 435B
	++mRoomObjects[pObjectNumber].Object_field_2;

	if( mRoomObjects[pObjectNumber].Object_field_2 >= 3 )
		mRoomObjects[pObjectNumber].Object_field_2 = 0;
	
	byte_43E3 = mRoomObjects[pObjectNumber].Object_field_2;
	
	gfxPosX = mMemory[ word_40 + 1 ];
	gfxPosY = mMemory[ word_40 + 2 ];

	for(byte_43E2 = mMemory[ word_40 + 3 ]; byte_43E2; --byte_43E2 ) {

		if( byte_43E3 ) {
			if( byte_43E3 != 1 ) {
				mMemory[ 0x66E6 ] = 0x66;
				mMemory[ 0x66E7 ] = 0x01;

			} else {
			// 43B4
				mMemory[ 0x66E6 ] = 0x61;
				mMemory[ 0x66E7 ] = 0x06;
			}
		} else {
			// 43A7
			mMemory[ 0x66E6 ] = 0x16;
			mMemory[ 0x66E7 ] = 0x06;
		}

		// 43BE
		screenDraw( 0, 0x34, gfxPosX, gfxPosY, 0 );
		++byte_43E3;
		if( byte_43E3 >= 3 )
			byte_43E3 = 0;

		gfxPosY += 0x08;
	}

}

void cCreep::obj_Door_Button_Prepare() {
	byte gfxCurrentID, gfxPosX, gfxPosY;
	byte X = 0;

	for( byte byte_42AB = mMemory[ mObjectPtr++ ]; byte_42AB; --byte_42AB) {

		object_Create( X );
		mRoomAnim[X].mFuncID = 1;

		gfxPosX = mMemory[mObjectPtr];
		gfxPosY = mMemory[mObjectPtr+1];
		gfxCurrentID = 0x09;
		mRoomObjects[X].objNumber = mMemory[ mObjectPtr+2 ];
		
		byte A = 0;

		// Find the colour of the door this button connects to
		for( unsigned char Y = 0; Y < MAX_OBJECTS; ++Y ) {
			if( mRoomAnim[Y].mFuncID == 0 ) {

				if( mRoomObjects[Y].objNumber == mRoomObjects[X].objNumber ) {
					A = mRoomObjects[Y].color;
					break;
				}
			}	
		}

		// Set the palette color
		for( signed char Y = 8; Y >= 0; --Y )
			mMemory[ 0x63D2 + Y ] = A;

		A >>= 4;
		A |= 0x10;

		mMemory[ 0x63D6 ] = A;
		Draw_RoomAnimObject( gfxCurrentID, gfxPosX, gfxPosY, 0, X);
		mObjectPtr += 0x03;
	}

}

// 44E7: Lightning Switch
void cCreep::obj_Lightning_Switch_InFront( byte pSpriteNumber, byte pObjectNumber ) {
	if( mRoomSprites[pSpriteNumber].Sprite_field_0 )
		return;

	if( (mRoomSprites[pSpriteNumber].mX + mRoomSprites[pSpriteNumber].mWidth) - mRoomAnim[pObjectNumber].mX >= 4 )
		return;

	if( mRoomSprites[pSpriteNumber].Sprite_field_1E != 0 && mRoomSprites[pSpriteNumber].Sprite_field_1E != 4 )
		return;

	// 4507
	
	word_30 = mRoomLightningPtr + mRoomObjects[pObjectNumber].objNumber;

	if( !(mMemory[ word_30 ] & byte_45DE )) {
		if( mRoomSprites[pSpriteNumber].Sprite_field_1E )
			return;
	} else {
		if( !(mRoomSprites[pSpriteNumber].Sprite_field_1E) )
			return;
	}
	
	// 4535
	mMemory[ word_30 ] ^= byte_45DE;
	for(byte byte_45D7 = 0; byte_45D7 < 4; ++byte_45D7) {

		byte A = mMemory[ word_30 + (byte_45D7 + 4) ];
		if( A == 0xFF )
			break;

		// 4553
		byte byte_45DA = A;
		word_32 = mRoomLightningPtr + A;
		
		mMemory[ word_32 ] ^= byte_45DE;
		byte Y;

		for( Y = 0; Y < MAX_OBJECTS; ++Y ) {
			
			if( mRoomAnim[Y].mFuncID != 2 )
				continue;

			if( mRoomObjects[Y].objNumber == byte_45DA )
				break;
		}

		// 4585
		mRoomAnim[Y].mFlags |= ITM_EXECUTE;
	}

	byte A;

	// 4594
	if( !(mMemory[ word_30 ] & byte_45DE )) {
		mMemory[ 0x75E7 ] = 0x2F;
		A = 0x38;
	} else {
		mMemory[ 0x75E7 ] = 0x23;
		A = 0x37;
	}

	Draw_RoomAnimObject( A, mRoomAnim[pObjectNumber].mX, mRoomAnim[pObjectNumber].mY, 0, pObjectNumber );

	sound_PlayEffect(0x06);
}

// 45E0: Forcefield Timer
void cCreep::obj_Forcefield_Timer_Execute( byte pObjectNumber ) {

	if( --mRoomObjects[pObjectNumber].Object_field_1 != 0 )
		return;

	--mRoomObjects[pObjectNumber].Object_field_2;
	byte A = mMemory[ 0x4756 + mRoomObjects[pObjectNumber].Object_field_2 ];

	mMemory[ 0x75AB ] = A;
	sound_PlayEffect( 2 );

	for( byte Y = 0; Y < 8; ++Y ) {
		if( Y >= mRoomObjects[pObjectNumber].Object_field_2 )
			A = 0x55;
		else
			A = 0;

		mMemory[ 0x6889 + Y ] = A;
	}

	screenDraw( 0, 0x40, mRoomAnim[pObjectNumber].mX, mRoomAnim[pObjectNumber].mY, 0 );
	if( mRoomObjects[pObjectNumber].Object_field_2 != 0 ) {
		mRoomObjects[pObjectNumber].Object_field_1 = 0x1E;
		return;
	} 

	// 4633
	mRoomAnim[pObjectNumber].mFlags ^= ITM_EXECUTE;
	mMemory[ 0x4750 + mRoomObjects[pObjectNumber].objNumber ] = 1;
}

// Lightning Machine Setup
void cCreep::obj_Lightning_Prepare() {
	byte	byte_44E5, gfxPosX, gfxPosY;

	mRoomLightningPtr = mObjectPtr;
	byte_44E5 = 0;

	byte X = 0, A;

	do {

		object_Create( X );

		mRoomObjects[X].objNumber = byte_44E5;

		// Is Switch On?
		if( mMemory[ mObjectPtr ] & byte_45DD ) {
			// 441C
			gfxPosX = mMemory[ mObjectPtr + 1 ];
			gfxPosY = mMemory[ mObjectPtr + 2 ];
			screenDraw( 0, 0x36, gfxPosX, gfxPosY, 0 );
			
			gfxPosX += 0x04;
			gfxPosY += 0x08;

			mRoomAnim[X].mFuncID = 3;
			if( mMemory[ mObjectPtr ] & byte_45DE )
				A = 0x37;
			else
				A = 0x38;

			Draw_RoomAnimObject( A, gfxPosX, gfxPosY, 0, X );

		} else {
			// 4467
			mRoomAnim[X].mFuncID = 2;
			gfxPosX = mMemory[ mObjectPtr + 1 ];
			gfxPosY = mMemory[ mObjectPtr + 2 ];
			
			mRoomObjects[X].color = mMemory[ mObjectPtr + 3 ];

			for( byte byte_44E6 = mMemory[ mObjectPtr + 3 ]; byte_44E6; --byte_44E6) {

				screenDraw( 0, 0x32, gfxPosX, gfxPosY, 0 );
				gfxPosY += 0x08;
			}

			gfxPosX -= 0x04;

			Draw_RoomAnimObject( 0x33, gfxPosX, gfxPosY, 0, X );
			if( mMemory[ mObjectPtr ] & byte_45DE )
				mRoomAnim[X].mFlags |= ITM_EXECUTE;
		}

		// 44C8
		byte_44E5 += 0x08;
		mObjectPtr += 0x08;

	} while ( !(mMemory[ mObjectPtr ] & byte_45DF) );

	++mObjectPtr;
}

// 46AE: Forcefield
void cCreep::obj_Forcefield_Prepare() {
	byte X = 0;
	byte gfxPosX, gfxPosY;

	byte_474F = 0;
	
	do {

		object_Create( X );

		mRoomAnim[X].mFuncID = 4;

		gfxPosX = mMemory[ mObjectPtr ];
		gfxPosY = mMemory[ mObjectPtr + 1 ];

		// Draw outside of timer
		screenDraw( 0, 0x3F, gfxPosX, gfxPosY, 0 );

		// 46EA
		gfxPosX += 4;
		gfxPosY += 8;

		for( signed char Y = 7; Y >= 0; --Y ) 
			mMemory[ 0x6889 + Y ] = 0x55;

		// Draw inside of timer
		Draw_RoomAnimObject( 0x40, gfxPosX, gfxPosY, 0, X );

		mRoomObjects[X].objNumber = byte_474F;
		mMemory[ 0x4750 + byte_474F ] = 1;

		obj_Forcefield_Create( );

		gfxPosX = mMemory[ mObjectPtr + 2 ];
		gfxPosY = mMemory[ mObjectPtr + 3 ];

		// Draw top of forcefield
		screenDraw( 0, 0x3E, gfxPosX, gfxPosY, 0 );
		
		++byte_474F;
		mObjectPtr += 0x04;
	} while( mMemory[ mObjectPtr ] );

	++mObjectPtr;
}

// 4872 : Load the rooms' Mummys
void cCreep::obj_Mummy_Prepare( ) {
	byte	ObjectNumber = 0, YCount, XCount;
	byte	X;
	byte	gfxCurrentID;

	mRoomMummyPtr = mObjectPtr;

	do {
		object_Create( X );
		
		mRoomAnim[X].mFuncID = 5;

		byte gfxPosX = mMemory[ mObjectPtr + 1 ];
		byte gfxPosY = mMemory[ mObjectPtr + 2 ];
		gfxCurrentID = 0x44;

		mRoomObjects[X].objNumber = ObjectNumber;
		mRoomObjects[X].Object_field_2 = 0x66;
		for( signed char Y = 5; Y >= 0; --Y )
			mMemory[ 0x68F0 + Y ] = 0x66;

		Draw_RoomAnimObject( gfxCurrentID, gfxPosX, gfxPosY, 0, X );
		
		gfxPosY = mMemory[ mObjectPtr + 4 ];
		gfxCurrentID = 0x42;

		for( YCount = 3; YCount; --YCount ) {
			
			gfxPosX = mMemory[ mObjectPtr + 3 ];

			for( XCount = 5; XCount; --XCount ) {
				screenDraw( 0, gfxCurrentID, gfxPosX, gfxPosY, 0 );
				gfxPosX += 4;
			}

			gfxPosY += 8;
		}

		if( mMemory[ mObjectPtr ] != 1 ) {
			// 4911
			mTxtX_0 = mMemory[ mObjectPtr + 3 ] + 4;
			mTxtY_0 = mMemory[ mObjectPtr + 4 ] + 8;
			

			for( YCount = 3; YCount; --YCount ) {
				screenDraw( 1, gfxCurrentID, gfxPosX, gfxPosY, 0x42 );
				mTxtX_0 += 4;
			}
			
			gfxPosX = mTxtX_0 - 0x0C;
			gfxPosY = mTxtY_0;
			screenDraw( 0, 0x43, gfxPosX, gfxPosY, 0x42 );
			
			if( mMemory[ mObjectPtr ] == 2 )
				obj_Mummy_Sprite_Create(0xFF, X);
		}
		// 496E
		mObjectPtr += 0x07;
		ObjectNumber += 0x07;

	} while( mMemory[ mObjectPtr ] );

	++mObjectPtr;
}

// 517F : Load the rooms' Trapdoors
void cCreep::obj_TrapDoor_Prepare( ) {
	byte	byte_5381;
	mRoomTrapDoorPtr = mObjectPtr;

	byte X;

	byte_5381 = 0;
	do {
		
		object_Create( X );
		mRoomAnim[X].mFuncID = 0x0B;
		mRoomObjects[X].objNumber = byte_5381;
		if( !(mMemory[ mObjectPtr ] & byte_538A) ) {
			// 51BC
			mMemory[ 0x6F2E ] = 0xC0;
			mMemory[ 0x6F30 ] = 0x55;
		} else {
			// 51c9
			mTxtX_0 = mMemory[ mObjectPtr + 1 ];
			mTxtY_0 = mMemory[ mObjectPtr + 2 ];
			screenDraw( 1, 0, 0, 0, 0x7B );
			
			byte gfxPosX, gfxPosY;

			gfxPosX = mTxtX_0 + 4;
			gfxPosY = mTxtY_0;

			Draw_RoomAnimObject( 0x79, gfxPosX, gfxPosY, 0x7B, X );

			mMemory[ 0x6F2E ] = 0x20;
			mMemory[ 0x6F30 ] = 0xCC;
			byte_5FD5 = mMemory[ mObjectPtr + 1 ] >> 2;
			byte_5FD5 -= 4;
			
			byte_5FD6 = mMemory[ mObjectPtr + 2 ] >> 3;
			word_3C_Calculate();

			mMemory[ word_3C ] = mMemory[ word_3C ] & 0xFB;
			mMemory[ word_3C + 4 ] = mMemory[ word_3C + 4 ] & 0xBF;
		}

		// 522E
		object_Create( X );
		mRoomAnim[X].mFuncID = 0x0C;
		
		byte gfxPosX = mMemory[ mObjectPtr + 3 ];
		byte gfxPosY = mMemory[ mObjectPtr + 4 ];
		
		mRoomObjects[X].objNumber = byte_5381;
		Draw_RoomAnimObject( 0x7A, gfxPosX, gfxPosY, 0, X );
		
		byte_5381 += 0x05;
		mObjectPtr += 0x05;

	} while( !(mMemory[ mObjectPtr ] & byte_5389) );

	++mObjectPtr;
}

// 50D2: Floor Switch
void cCreep::obj_TrapDoor_Switch_Execute( byte pObjectNumber ) {
	
	word_40 = mRoomTrapDoorPtr + mRoomObjects[pObjectNumber].objNumber;
	if( mRoomObjects[pObjectNumber].Object_field_1 ) {
		mTxtX_0 = mMemory[ word_40 + 1 ];
		mTxtY_0 = mMemory[ word_40 + 2 ];

		byte A = mRoomObjects[pObjectNumber].Object_field_2;

		obj_TrapDoor_PlaySound( A );
		screenDraw( 1, 0, 0, 0, A );
		if( mRoomObjects[pObjectNumber].Object_field_2 != 0x78 ) {
			// 515F
			++mRoomObjects[pObjectNumber].Object_field_2;
			return;
		}
		
		Draw_RoomAnimObject( 0x79, mMemory[ word_40 + 1 ] + 4, mMemory[ word_40 + 2 ], 0, pObjectNumber );
		
	} else {
		// 5129
		if( mRoomObjects[pObjectNumber].Object_field_2 == 0x78 )
			roomAnim_Disable( pObjectNumber );
		
		byte A = mRoomObjects[pObjectNumber].Object_field_2;

		obj_TrapDoor_PlaySound( A );
		screenDraw( 0, A, mMemory[ word_40 + 1 ], mMemory[ word_40 + 2 ], 0 );
		if( mRoomObjects[pObjectNumber].Object_field_2 != 0x73 ) {
			--mRoomObjects[pObjectNumber].Object_field_2;
			return;
		}
	}
	// 5165
	mRoomAnim[pObjectNumber].mFlags ^= ITM_EXECUTE;
}

// 538B: Conveyor
void cCreep::obj_Conveyor_Execute( byte pObjectNumber ) {
	
	word_40 = mConveyorPtr + mRoomObjects[pObjectNumber].objNumber;
	byte A = mMemory[ word_40 ];

	// 539F
	if( (A & byte_5646) && !(A & byte_5644) ||
		(A & byte_5645) && !(A & byte_5643) ) {

			if( A & byte_5648 ) {
				
				A ^= byte_5648;
				A ^= byte_5647;

				mMemory[ word_40 ] = A;
				mMemory[ 0x70A8 ] = mMemory[ 0x70A6 ] = 0xC0;
				mMemory[ 0x7624 ] = 0x18;

			} else {
				// 53D0
				A |= byte_5648;
				mMemory[ word_40 ] = A;
				if( A & byte_5647 ) {
					mMemory[ 0x70A6 ] = 0x50;
					mMemory[ 0x70A8 ] = 0xC0;
					mMemory[ 0x7624 ] = 0x18;
				} else {
					// 53EC
					mMemory[ 0x70A6 ] = 0xC0;
					mMemory[ 0x70A8 ] = 0x20;
					mMemory[ 0x7024 ] = 0x18;
				}
			}

			// 53FB
			byte gfxPosX = mMemory[ word_40 + 3 ];
			byte gfxPosY = mMemory[ word_40 + 4 ];
			screenDraw( 0, 0x82, gfxPosX, gfxPosY, 0 );

			sound_PlayEffect(0xA);
	}

	// 541B
	A = 0xFF;

	A ^= byte_5644;
	A ^= byte_5643;
	A &= mMemory[ word_40 ];
	// 5427
	if( A & byte_5646 ) {
		A |= byte_5644;
		A ^= byte_5646;
	}
	if( A & byte_5645 ) {
		A |= byte_5643;
		A ^= byte_5645;
	}

	mMemory[ word_40 ] = A;
	// 543F
	if( A & byte_5648 ) {
		A = mEngine_Ticks & 1;
		if( A )
			return;

		byte gfxCurrentID = mRoomAnim[pObjectNumber].mGfxID;

		if( !(mMemory[ word_40 ] & byte_5647) ) {
			// 5458
			++gfxCurrentID;
			if( gfxCurrentID >= 0x82 )
				gfxCurrentID = 0x7E;

		} else {
			//546a
			--gfxCurrentID;
			if( gfxCurrentID < 0x7E )
				gfxCurrentID = 0x81;
		}
		// 5479
		byte gfxPosX = mRoomAnim[pObjectNumber].mX;
		byte gfxPosY = mRoomAnim[pObjectNumber].mY;
		
		Draw_RoomAnimObject( gfxCurrentID, gfxPosX, gfxPosY, 0, pObjectNumber );
	}
}

// 47A7: In Front Mummy Release
void cCreep::obj_Mummy_Infront( byte pSpriteNumber, byte pObjectNumber ) {

	if( mRoomSprites[pSpriteNumber].Sprite_field_0 )
		return;

	byte A = mRoomSprites[pSpriteNumber].mX + mRoomSprites[pSpriteNumber].mWidth;
	A -= mRoomAnim[pObjectNumber].mX;

	if( A >= 8 )
		return;

	word_40 = mRoomMummyPtr + mRoomObjects[pObjectNumber].objNumber;

	// 47D1
	if( mMemory[ word_40 ] != 1 )
		return;

	mMemory[ word_40 ] = 2;
	mMemory[ word_40 + 5 ] = mMemory[ word_40 + 3 ] + 4;
	// 47ED
	mMemory[ word_40 + 6 ] = mMemory[ word_40 + 4 ] + 7;
	
	// 47FB
	mRoomAnim[pObjectNumber].mFlags |= ITM_EXECUTE;

	mRoomObjects[pObjectNumber].Object_field_1 = 8;
	mRoomObjects[pObjectNumber].Object_field_2 = 0x66;
	mTxtX_0 = mMemory[ word_40 + 3 ] + 4;
	mTxtY_0 = mMemory[ word_40 + 4 ] + 8;
	
	for( byte byte_4871 = 3; byte_4871 ; --byte_4871) {
		screenDraw( 1, 0, 0, 0, 0x42 );
		mTxtX_0 += 4;
	}

	// 4842
	byte gfxPosX = mTxtX_0 - 0x0C;
	byte gfxPosY = mTxtY_0;

	screenDraw( 0, 0x43, gfxPosX, gfxPosY, 0 );
	obj_Mummy_Sprite_Create(0, pObjectNumber );
}

// 564E: Load the rooms' frankensteins
void cCreep::obj_Frankie_Load() {
	mFrankiePtr = mObjectPtr;

	byte_574A = 0;

	do {

		mTxtX_0 = mMemory[ mObjectPtr + 1 ];
		mTxtY_0 = mMemory[ mObjectPtr + 2 ] + 0x18;
		screenDraw( 1, 0, 0, 0, 0x92 );

		byte_5FD5 = (mTxtX_0 >> 2) - 4;
		byte_5FD6 = (mTxtY_0 >> 3);

		word_3C_Calculate();
		byte A;

		if( ( mMemory[ mObjectPtr ] & byte_574F )) {
			word_3C -= 2;
			A = 0xFB;
		} else
			A = 0xBF;

		// 56C4
		for( signed char Y = 4; Y >= 0; Y -= 2 ) {
			mMemory[ word_3C + Y ] &= A;
		}
		byte X;
		object_Create( X );
		mRoomAnim[X].mFuncID = 0x0F;
		byte gfxPosX = mMemory[ mObjectPtr + 1 ];
		byte gfxPosY = mMemory[ mObjectPtr + 2 ];
		if( !(mMemory[ mObjectPtr ] & byte_574F ))
			A = 0x90;
		else
			A = 0x91;

		Draw_RoomAnimObject( A, gfxPosX, gfxPosY, 0, X );

		//5700
		if(!( mMemory[ mObjectPtr ]  & byte_574F )) {
			gfxPosX += 4;
			gfxPosY += 0x18;
			screenDraw( 0, 0x1C, gfxPosX, gfxPosY, 0 );
		}
		obj_Frankie_Sprite_Create();

		mObjectPtr += 0x07;
		byte_574A += 0x07;

	} while( !(mMemory[ mObjectPtr ] & byte_574C) );

	++mObjectPtr;
}

// 5501: Load the rooms' Conveyors
void cCreep::obj_Conveyor_Prepare() {
	mConveyorPtr = mObjectPtr;

	byte byte_5649 = 0, gfxPosX = 0, gfxPosY = 0;
	
	byte A, X;

	do {
		
		//5527
		A = 0xFF;

		A ^= byte_5646;
		A ^= byte_5645;
		A ^= byte_5644;
		A ^= byte_5643;

		A &= mMemory[ mObjectPtr ];
		mMemory[ mObjectPtr ] = A;

		object_Create( X );

		mRoomAnim[X].mFuncID = 0x0D;
		mRoomObjects[X].objNumber = byte_5649;

		mRoomAnim[X].mFlags = (mRoomAnim[X].mFlags | ITM_EXECUTE);
		
		mTxtX_0 = mMemory[ mObjectPtr + 1 ];
		mTxtY_0 = mMemory[ mObjectPtr + 2 ];

		screenDraw( 1, 0x7D, gfxPosX, gfxPosY, 0x7D );
		gfxPosX = mTxtX_0;
		gfxPosY = mTxtY_0;
		
		Draw_RoomAnimObject( 0x7E, gfxPosX, gfxPosY, 0x7D, X );
		object_Create( X );

		mRoomAnim[X].mFuncID = 0x0E;
		mRoomObjects[X].objNumber = byte_5649;
		
		gfxPosX = mMemory[ mObjectPtr + 3 ];
		gfxPosY = mMemory[ mObjectPtr + 4 ];
		byte gfxCurrentID = 0x82;
		byte gfxDecodeMode = 0;

		if( (mMemory[ mObjectPtr ] & byte_5648) == 0 ) {
			mMemory[ 0x70A6 ] = 0xC0;
			mMemory[ 0x70A8 ] = 0xC0;
		} else {
			if( (mMemory[ mObjectPtr ] & byte_5647) == 0 ) {
				// 55BE
				mMemory[ 0x70A6 ] = 0xC0;
				mMemory[ 0x70A8 ] = 0x20;
			} else {
				// 55CB
				mMemory[ 0x70A6 ] = 0x50;
				mMemory[ 0x70A8 ] = 0xC0;
			}
		}

		screenDraw( gfxDecodeMode, gfxCurrentID, gfxPosX, gfxPosY, 0 );
		gfxPosX = mMemory[ mObjectPtr + 3] + 0x04;
		gfxPosY = mMemory[ mObjectPtr + 4] + 0x08;

		Draw_RoomAnimObject( 0x83, gfxPosX, gfxPosY, 0, X );
		byte_5649 += 0x05;
		mObjectPtr += 0x05;

	} while( !(mMemory[ mObjectPtr ] & byte_5642) );

	++mObjectPtr;
}

void cCreep::obj_Forcefield_Create() {
	sCreepSprite *sprite = sprite_CreepGetFree();

	sprite->Sprite_field_0 = 2;
	sprite->mX = mMemory[ mObjectPtr + 2 ];
	sprite->mY = mMemory[ mObjectPtr + 3 ] + 2;
	sprite->spriteImageID= 0x35;

	sprite->Sprite_field_1F = byte_474F;
	sprite->Sprite_field_1E = 0;
	sprite->Sprite_field_6 = 4;
	sprite->mWidth = 2;
	sprite->mHeight = 0x19;
}

// 38CE: Mummy ?
void cCreep::obj_Mummy_Collision( byte pSpriteNumber, byte pObjectNumber ) {
	if( mRoomAnim[pObjectNumber].mFuncID == 0x0B ) {
		
		char A = mRoomSprites[pSpriteNumber].mX + mRoomSprites[pSpriteNumber].mWidth;
		A -= mRoomAnim[pObjectNumber].mX;
		if( A < 4 ) {
			
			word_40 = mRoomTrapDoorPtr + mRoomObjects[pObjectNumber].objNumber;
			// 38F7
			if( mMemory[ word_40 ] & byte_538A ) {
				// 3900
				word_40 = mRoomMummyPtr + mRoomSprites[pSpriteNumber].mButtonState;
				mMemory[ word_40 ] = 3;
				return;
			}
		}
	} 

	// 3919
	mStartSpriteFlash = 0;
	if( mRoomAnim[pObjectNumber].mFuncID != 0x0C )
		return;

	char A = mRoomSprites[pSpriteNumber].mX + mRoomSprites[pSpriteNumber].mWidth;
	A -= mRoomAnim[pObjectNumber].mX;
	if( A >= 4 )
		return;

	mRoomSprites[pSpriteNumber].playerNumber = mRoomObjects[pObjectNumber].objNumber;
}

// 3940: 
void cCreep::obj_Mummy_Sprite_Collision( byte pSpriteNumber, byte pSpriteNumber2 ) {

	if( mRoomSprites[pSpriteNumber2].Sprite_field_0 == 0 
	 || mRoomSprites[pSpriteNumber2].Sprite_field_0 == 5 ) {
		mStartSpriteFlash2 = 0;
		return;
	}

	word_40 = mRoomMummyPtr + mRoomSprites[pSpriteNumber].mButtonState;

	mMemory[ word_40 ] = 3;
}

// 3A60:  
void cCreep::obj_RayGun_Laser_Collision( byte pSpriteNumber, byte pObjectNumber ) {
	if( mRoomAnim[pObjectNumber].mFuncID == 2 )
		return;

	if( mRoomAnim[pObjectNumber].mFuncID == 0x0F )
		return;

	if( mRoomAnim[pObjectNumber].mFuncID == 8 ) {
		if( mRoomSprites[pSpriteNumber].Sprite_field_1E != mRoomObjects[pObjectNumber].objNumber )
			return;
	}

	mStartSpriteFlash = 0;
}

void cCreep::obj_RayGun_Laser_Sprite_Create( byte pObjectNumber ) {
	byte A = mRoomObjects[pObjectNumber].objNumber + 0x07;
	A |= 0xF8;
	A >>= 1;
	A += 0x2C;
	
	mMemory[ 0x7591 + 2 ] = A;

	sound_PlayEffect( 0 );

	byte X = sprite_CreepFindFree( );

	mRoomSprites[X].Sprite_field_0 = 4;
	mRoomSprites[X].mX= mRoomAnim[pObjectNumber].mX;
	mRoomSprites[X].mY= mRoomAnim[pObjectNumber].mY + 0x05;
	mRoomSprites[X].spriteImageID= 0x6C;
	mRoomSprites[X].Sprite_field_1E = mRoomObjects[pObjectNumber].objNumber;

	if( mMemory[ word_40 ] & byte_4D67 ) {
		mRoomSprites[X].mX-= 0x08;
		mRoomSprites[X].Sprite_field_1F = 0xFC;
	} else {
		// 3AD4	
		mRoomSprites[X].mX+= 0x08;
		mRoomSprites[X].Sprite_field_1F = 4;
	}

	hw_SpritePrepare( X );
}

// 396A: 
void cCreep::obj_Mummy_Sprite_Create( byte pA, byte pObjectNumber ) {
	byte sprite = sprite_CreepFindFree( );
	
	mRoomSprites[ sprite ].Sprite_field_0 = 3;
	mRoomSprites[ sprite ].Sprite_field_1B = 0xFF;
	mRoomSprites[ sprite ].playerNumber = 0xFF;
	mRoomSprites[ sprite ].mButtonState = mRoomObjects[pObjectNumber].objNumber;
	
	word_40 = mRoomMummyPtr + mRoomSprites[ sprite ].mButtonState;
	//3998

	mRoomSprites[ sprite ].mWidth = 5;
	mRoomSprites[ sprite ].mHeight = 0x11;
	mRoomSprites[ sprite ].spriteImageID= 0xFF;
	if( pA == 0 ) {
		mRoomSprites[ sprite ].Sprite_field_1E = 0;
		mRoomSprites[ sprite ].Sprite_field_1F = 0xFF;
		mRoomSprites[ sprite ].Sprite_field_6 = 4;
		
		mRoomSprites[ sprite ].mX= mMemory[ word_40 + 3 ] + 0x0D;
		mRoomSprites[ sprite ].mY= mMemory[ word_40 + 4 ] + 0x08;
	} else {
		// 39D0
		mRoomSprites[ sprite ].Sprite_field_1E = 1;
		mRoomSprites[ sprite ].mX= mMemory[ word_40 + 5 ];
		mRoomSprites[ sprite ].mY= mMemory[ word_40 + 6 ];
		mRoomSprites[ sprite ].Sprite_field_6 = 2;
	}

	// 39E8
}

// 379A: Mummy
void cCreep::obj_Mummy_Execute( byte pSpriteNumber ) {

	if( mRoomSprites[pSpriteNumber].state & SPR_ACTION_DESTROY ) {
		mRoomSprites[pSpriteNumber].state ^= SPR_ACTION_DESTROY;
		mRoomSprites[pSpriteNumber].state |= SPR_ACTION_FREE;
		return;
	}

	if( mRoomSprites[pSpriteNumber].state & SPR_ACTION_CREATED ) {

		mRoomSprites[pSpriteNumber].state ^= SPR_ACTION_CREATED;

		if( mRoomSprites[pSpriteNumber].Sprite_field_1E ) {
			mRoomSprites[pSpriteNumber].spriteImageID = 0x4B;
			hw_SpritePrepare( pSpriteNumber );
		}
	}
	// 37C6
	char AA = mRoomSprites[pSpriteNumber].playerNumber;
	if( AA != -1 ) {
		if( AA != mRoomSprites[pSpriteNumber].Sprite_field_1B )
			sub_526F( AA );
	}
	// 37D5
	mRoomSprites[pSpriteNumber].Sprite_field_1B = AA;
	mRoomSprites[pSpriteNumber].playerNumber = 0xFF;
	word_40 = mRoomMummyPtr + mRoomSprites[pSpriteNumber].mButtonState;
	if( mRoomSprites[pSpriteNumber].Sprite_field_1E == 0 ) {
		++mRoomSprites[pSpriteNumber].Sprite_field_1F;
		byte Y = mRoomSprites[pSpriteNumber].Sprite_field_1F;
		byte A = mMemory[ 0x39EF + Y ];

		if( A != 0xFF ) {
			mRoomSprites[pSpriteNumber].spriteImageID = A;
			mRoomSprites[pSpriteNumber].mX += mMemory[ 0x39F7 + Y ];

			mRoomSprites[pSpriteNumber].mY += mMemory[ 0x39FF + Y ];
			
			mMemory[ 0x7630 ] = (mRoomSprites[pSpriteNumber].Sprite_field_1F << 2) + 0x24;
			sound_PlayEffect( 0x0B );
			hw_SpritePrepare( pSpriteNumber );
			return;
		}
		// 3828
		mRoomSprites[pSpriteNumber].Sprite_field_1E = 0x01;
		mRoomSprites[pSpriteNumber].mX = mMemory[ word_40 + 3 ] + 4;
		mRoomSprites[pSpriteNumber].mY = mMemory[ word_40 + 4 ] + 7;
		mRoomSprites[pSpriteNumber].Sprite_field_6 = 2;
	}
	byte Y;

	// 3846
	if( mMemory[ 0x780D ] != 0 ) {
		if( mMemory[ 0x780E ] != 0 )
			return;
		else
			Y = 1;

	} else {
	// 3857
		Y = 0;
	}

	// 385E
	Y = mMemory[ 0x34D1 + Y ];
	positionCalculate( pSpriteNumber );

	AA = mRoomSprites[pSpriteNumber].mX;
	AA -= mRoomSprites[Y].mX;
	if( AA < 0 ) {
		AA ^= 0xFF;
		++AA;
	}
	// 3872
	if( AA < 2 )
		return;

	// Frame
	++mRoomSprites[pSpriteNumber].spriteImageID;
	if( mRoomSprites[pSpriteNumber].mX < mRoomSprites[Y].mX ) {
		// Walking Right
		// 3881
		if( !(mMemory[ word_3C ] & 0x04) )
			return;
		
		// 3889
		++mRoomSprites[pSpriteNumber].mX;
		if( mRoomSprites[pSpriteNumber].spriteImageID < 0x4E || mRoomSprites[pSpriteNumber].spriteImageID >= 0x51 )
				mRoomSprites[pSpriteNumber].spriteImageID = 0x4E;
	
	} else {
		// Walking Left
		// 389F
		if( !(mMemory[ word_3C ] & 0x40) )
			return;

		--mRoomSprites[pSpriteNumber].mX;
		if( mRoomSprites[pSpriteNumber].spriteImageID < 0x4B || mRoomSprites[pSpriteNumber].spriteImageID >= 0x4E)
			mRoomSprites[pSpriteNumber].spriteImageID = 0x4B;
	}

	// 38BA
	mMemory[ word_40 + 5 ] = mRoomSprites[pSpriteNumber].mX;
	mMemory[ word_40 + 6 ] = mRoomSprites[pSpriteNumber].mY;

	hw_SpritePrepare( pSpriteNumber );
}

// 3A08
void cCreep::obj_RayGun_Laser_Execute( byte pSpriteNumber ) {
	if( mNoInput )
		return;
		
	mScreen->spriteRedrawSet();

	if( mRoomSprites[pSpriteNumber].state & SPR_ACTION_DESTROY ) {

		mRoomSprites[pSpriteNumber].state ^= SPR_ACTION_DESTROY;
		mRoomSprites[pSpriteNumber].state |= SPR_ACTION_FREE;
		
		word_40 = mRoomRayGunPtr + mRoomSprites[pSpriteNumber].Sprite_field_1E;
		mMemory[ word_40 ] = (0xFF ^ byte_4D61) & mMemory[ word_40 ];

	} else {
		if( mRoomSprites[pSpriteNumber].state & SPR_ACTION_CREATED )
			mRoomSprites[pSpriteNumber].state ^= SPR_ACTION_CREATED;

		// 3A42
		byte A = mRoomSprites[pSpriteNumber].mX + mRoomSprites[pSpriteNumber].Sprite_field_1F;
		mRoomSprites[pSpriteNumber].mX = A;

		// Edge of screen?
		if( A < 0xB0 && A >= 8 )
			return;
		
		// Reached edge
		mRoomSprites[pSpriteNumber].state |= SPR_ACTION_DESTROY;
	}
}

// 3F14: Find a free object position, and clear it
int cCreep::sprite_CreepFindFree( ) {

	for( int number = 0 ; number < MAX_SPRITES; ++number ) {
		sCreepSprite *sprite = &mRoomSprites[number];

		if( sprite->state & SPR_FLAG_FREE ) {

			sprite->clear();

			sprite->state = SPR_ACTION_CREATED;

			sprite->Sprite_field_5 = 1;
			sprite->Sprite_field_6 = 1;

			return number;
		}
	}

	return -1;
}

sCreepSprite *cCreep::sprite_CreepGetFree( ) {
	int number = sprite_CreepFindFree();
	sCreepSprite *sprite = 0;

	if( number != -1 )
		sprite = &mRoomSprites[ number ];

	return sprite;
}

void cCreep::Draw_RoomAnimObject( byte pGfxID, byte pGfxPosX, byte pGfxPosY, byte pTxtCurrentID, byte pObjectNumber ) {
	//5783
	byte gfxDecodeMode;

	if( !(mRoomAnim[pObjectNumber].mFlags & ITM_DISABLE) ) {
		gfxDecodeMode = 2;
		mTxtX_0 = mRoomAnim[pObjectNumber].mX;
		mTxtY_0 = mRoomAnim[pObjectNumber].mY;
		pTxtCurrentID = mRoomAnim[pObjectNumber].mGfxID;

	} else {
		gfxDecodeMode = 0;
	}

	screenDraw( gfxDecodeMode, pGfxID, pGfxPosX, pGfxPosY, pTxtCurrentID );
	//57AE
	mRoomAnim[pObjectNumber].mFlags = ((ITM_DISABLE ^ 0xFF) & mRoomAnim[pObjectNumber].mFlags);
	mRoomAnim[pObjectNumber].mGfxID = pGfxID;
	mRoomAnim[pObjectNumber].mX = pGfxPosX;
	mRoomAnim[pObjectNumber].mY = pGfxPosY;
	mRoomAnim[pObjectNumber].mWidth = mGfxWidth;
	mRoomAnim[pObjectNumber].mHeight = mGfxHeight;

	mRoomAnim[pObjectNumber].mWidth <<= 2;
}

// 41D8: In Front Button?
void cCreep::obj_Door_Button_InFront( byte pSpriteNumber, byte pObjectNumber ) {

	if( mRoomSprites[pSpriteNumber].Sprite_field_0 )
		return;

	if( mRoomSprites[pSpriteNumber].mButtonState == 0 )
		return;

	byte A = mRoomSprites[pSpriteNumber].mX + mRoomSprites[pSpriteNumber].mWidth;

	A -= mRoomAnim[pObjectNumber].mX;
	if( A >= 0x0C )
		return;

	if( mMemory[ 0x780D + mRoomSprites[pSpriteNumber].playerNumber ] != 0 )
		return;
	
	byte DoorID;

	for(DoorID = 0;; ++DoorID ) {
		if( (mRoomAnim[DoorID].mFuncID ))
			continue;
		
		if( mRoomObjects[DoorID].objNumber == mRoomObjects[pObjectNumber].objNumber )
			break;
	}
	//4216
	if( mRoomObjects[DoorID].Object_field_1 )
		return;

	mRoomAnim[DoorID].mFlags |= ITM_EXECUTE;
}

// 4647: In Front Forcefield Timer
void cCreep::obj_Forcefield_Timer_InFront( byte pSpriteNumber, byte pObjectNumber ) {
	if(mRoomSprites[pSpriteNumber].Sprite_field_0)
		return;

	if(!mRoomSprites[pSpriteNumber].mButtonState)
		return;

	mMemory[ 0x75AB ] = 0x0C;
	sound_PlayEffect( 0x02 );
	
	mRoomAnim[pObjectNumber].mFlags |= ITM_EXECUTE;
	mRoomObjects[pObjectNumber].Object_field_1 = 0x1E;
	mRoomObjects[pObjectNumber].Object_field_2 = 0x08;
	mMemory[ 0x6889 ] = mMemory[ 0x688A ] = mMemory[ 0x688B ] = 0x55;
	mMemory[ 0x688C ] = mMemory[ 0x688D ] = mMemory[ 0x688E ] = 0x55;
	mMemory[ 0x688F ] = mMemory[ 0x6890 ] = 0x55;

	mTxtX_0 = mRoomAnim[pObjectNumber].mX;
	mTxtY_0 = mRoomAnim[pObjectNumber].mY;
	screenDraw( 1, 0, 0, 0, mRoomAnim[pObjectNumber].mGfxID);

	mMemory[ 0x4750 + mRoomObjects[pObjectNumber].objNumber ] = 0;
}

// 4990: In front of key
void cCreep::obj_Key_Infront( byte pSpriteNumber, byte pObjectNumber ) {

	if( mRoomSprites[pSpriteNumber].Sprite_field_0 )
		return;

	if( mMemory[ 0x780D + mRoomSprites[pSpriteNumber].playerNumber ] != 0 )
		return;
	
	if( mRoomSprites[pSpriteNumber].mButtonState == 0 )
		return;

	sound_PlayEffect( 0x0C );
	mRoomAnim[pObjectNumber].mFlags |= ITM_PICKED;

	word_40 = mRoomKeyPtr + mRoomObjects[pObjectNumber].objNumber;

	mMemory[ word_40 + 1 ] = 0;
	byte KeyID = mMemory[ word_40 ];

	if( mRoomSprites[pSpriteNumber].playerNumber ) {
		// 49DA
		mMemory[ 0x7835 + mMemory[ 0x7814 ] ] = KeyID;
		++mMemory[ 0x7814 ];
		
	} else {
		mMemory[ 0x7815 + mMemory[ 0x7813 ] ] = KeyID;
		++mMemory[ 0x7813 ];
	}
}

// 4A68: In Front Lock
void cCreep::obj_Door_Lock_InFront( byte pSpriteNumber, byte pObjectNumber ) {

	if( mRoomSprites[pSpriteNumber].Sprite_field_0 )
		return;

	if( mMemory[ 0x780D + mRoomSprites[pSpriteNumber].playerNumber ] != 0 )
		return;

	if( mRoomSprites[pSpriteNumber].mButtonState == 0 ) 
		return;

	if( obj_Key_NotFound( mRoomObjects[pObjectNumber].objNumber, pSpriteNumber ) == true )
		return;

	// This loop expects to find the object, if it doesnt, its meant to loop forever
	for( byte X = 0; X < MAX_OBJECTS; ++X ) {

		if( mRoomAnim[X].mFuncID )
			continue;

		if( mRoomObjects[X].objNumber == mRoomObjects[pObjectNumber].Object_field_1 ) {

			// 4AA2
			if( mRoomObjects[X].Object_field_1 )
				return;

			mRoomAnim[X].mFlags |= ITM_EXECUTE;
			return;
		}
	}
	// Therefor, we should probably crash or something if this point is ever reached
}

bool cCreep::obj_Key_NotFound( byte pObjectNumber, byte pSpriteNumber ) {
	byte byte_5ED3;
	
	if( mRoomSprites[pSpriteNumber].playerNumber != 0 ) {
		byte_5ED3 = mMemory[ 0x7814 ];
		word_30 = 0x7835;
	} else {
		// 5EAA
		byte_5ED3 = mMemory[ 0x7813 ];
		word_30 = 0x7815;
	}

	//5EB8
	for( byte pY = 0;; ++pY ) {
		if( pY == byte_5ED3 )
			return true;

		if( mMemory[ word_30 + pY ] == pObjectNumber )
			return false;
	}
}

// 4D70: In Front RayGun Control
void cCreep::obj_RayGun_Control_InFront( byte pSpriteNumber, byte pObjectNumber ) {

	if(mRoomSprites[pSpriteNumber].Sprite_field_0)
		return;

	byte A = mRoomSprites[pSpriteNumber].mX + mRoomSprites[pSpriteNumber].mWidth;
	A -= mRoomAnim[pObjectNumber].mX;

	if( A >= 8 )
		return;

	if( mMemory[ 0x780D + mRoomSprites[pSpriteNumber].playerNumber ] != 0)
		return;

	word_40 = mRoomRayGunPtr + mRoomObjects[pObjectNumber].objNumber;
	A = 0xFF;
	A ^= byte_4D65;
	A ^= byte_4D66;
	A &= mMemory[ word_40 ];

	if( !mRoomSprites[pSpriteNumber].Sprite_field_1E ) 
		A |= byte_4D65;
	else {
		if( mRoomSprites[pSpriteNumber].Sprite_field_1E == 4 )
			A |= byte_4D66;
		else
			if(mRoomSprites[pSpriteNumber].Sprite_field_1E != 0x80 )
				return;
	}

	// 4DC9
	A |= byte_4D62;

	mMemory[ word_40 ] = A;
	if( mRoomSprites[pSpriteNumber].mButtonState )
		A = mMemory[ word_40 ] | byte_4D64;
	else
		A = (0xFF ^ byte_4D64) & mMemory[ word_40 ];

	mMemory[ word_40 ] = A;
}


// 4EA8: Teleport?
void cCreep::obj_Teleport_InFront( byte pSpriteNumber, byte pObjectNumber ) {

	if( mRoomAnim[pObjectNumber].mFlags & ITM_EXECUTE )
		return;

	if( mRoomSprites[pSpriteNumber].Sprite_field_0 )
		return;

	// 4EB5
	if( mMemory[ 0x780D + mRoomSprites[pSpriteNumber].playerNumber ] != 0 )
		return;

	// 4EC5
	word_40 = readLEWord( &mRoomObjects[pObjectNumber].objNumber );
	if(! (mRoomSprites[pSpriteNumber].mButtonState) ) {
		// 4ED4
		if( (mRoomSprites[pSpriteNumber].Sprite_field_1E) )
			return;

		if( mEngine_Ticks & 0x0F )
			return;
		
		// Change teleport destination
		byte A = mMemory[ word_40 + 2 ];
		A += 1;
		mMemory[ word_40 + 2 ] = A;
		A <<= 1;
		A += 3;

		if( !(mMemory[ word_40 + A ]) )
			mMemory[ word_40 + 2 ] = 0;

		// 4EF7
		mMemory[ 0x75DB ] = mMemory[ word_40 + 2 ] + 0x32;

		sound_PlayEffect(5);
		A = mMemory[ word_40 + 2 ] + 2;

		obj_Teleport_SetColour( A, pObjectNumber );
		
		return;
	} else {
		// 4F1A
		// Use Teleport
		mRoomAnim[pObjectNumber].mFlags |= ITM_EXECUTE;
		mRoomObjects[pObjectNumber].color = 8;
		
		byte A = mMemory[ word_40 + 2 ] + 0x02;
		// 4F35
		mRoomObjects[pObjectNumber].Object_field_2 = A;

		A = mMemory[ word_40 + 2 ] << 1;
		A += 0x03;

		byte A2 = mMemory[ word_40 + A ];
		++A;
		//4F44
		mRoomObjects[pObjectNumber].Object_field_5 = mMemory[ word_40 + A ];

		// Set player new X/Y
		mRoomSprites[pSpriteNumber].mY = mRoomObjects[pObjectNumber].Object_field_5 + 0x07;
		mRoomObjects[pObjectNumber].Object_field_4 = mRoomSprites[pSpriteNumber].mX = A2;
	}

}

// 4DE9: 
void cCreep::obj_RayGun_Control_Update( byte pA ) {

	mMemory[ 0x6DBF ] = pA;
	mMemory[ 0x6DC0 ] = pA;

	byte gfxPosX = mMemory[ word_40 + 5 ];
	byte gfxPosY = mMemory[ word_40 + 6 ];

	screenDraw( 0, 0x6E, gfxPosX, gfxPosY, 0 );
	mMemory[ 0x6DBF ] = pA << 4;
	mMemory[ 0x6DC0 ] = pA << 4;

	gfxPosY += 0x10;
	screenDraw( 0, 0x6E, gfxPosX, gfxPosY, 0 );
}

// 505C: 
void cCreep::obj_Teleport_SetColour( byte pColor, byte pObjectNumber ) {

	byte A =  (pColor << 4) | 0x0A;

	mMemory[ 0x6E70 ] = mMemory[ 0x6E71 ] = mMemory[ 0x6E72 ] = A;
	mMemory[ 0x6E73 ] = mMemory[ 0x6E74 ] = mMemory[ 0x6E75 ] = 0x0F;

	word_40 = readLEWord( &mRoomObjects[pObjectNumber].objNumber );

	byte gfxPosX = mMemory[ word_40 ] + 4;
	byte gfxPosY = mMemory[ word_40 + 1 ] ;
	screenDraw( 0, 0x71, gfxPosX, gfxPosY, 0 );

	gfxPosY += 0x08;

	mMemory[ 0x6E73 ] = mMemory[ 0x6E74 ] = mMemory[ 0x6E75 ] = 1;
	screenDraw( 0, 0x71, gfxPosX, gfxPosY, 0 );

	gfxPosY += 0x08;
	screenDraw( 0, 0x71, gfxPosX, gfxPosY, 0 );
}

// 5171: 
void cCreep::obj_TrapDoor_PlaySound( byte pA ) {
	
	mMemory[ 0x759F ] = pA - 0x48;
	sound_PlayEffect(1);

}

// 526F: 
void cCreep::sub_526F( byte pA ) {
	word SavedWord40, SavedWord3C;

	SavedWord40 = word_40;
	SavedWord3C = word_3C;

	word_40 = mRoomTrapDoorPtr + pA;
	// 529B
	mMemory[ word_40 ] ^= byte_538A;
	byte X;

	for( X = 0 ;;++X) {
		if( mRoomAnim[X].mFuncID != 0x0B )
			continue;
		if( mRoomObjects[X].objNumber == pA )
			break;
	}

	//52bd
	mRoomAnim[X].mFlags |= ITM_EXECUTE;
	
	if( !(mMemory[ word_40 ] & byte_538A) ) {
		// 52cf
		mRoomObjects[X].Object_field_1 = 0;
		mRoomObjects[X].Object_field_2 = 0x78;
		mMemory[ 0x6F2E ] = 0xC0;
		mMemory[ 0x6F30 ] = 0x55;

		byte_5FD5 = (mMemory[ word_40 + 1 ] >> 2) - 4;
		byte_5FD6 = mMemory[ word_40 + 2 ] >> 3;

		word_3C_Calculate();

		mMemory[ word_3C ] |= 0x04;
		mMemory[ word_3C + 4 ] |= 0x40;

	} else {
		// 530F
		mRoomObjects[X].Object_field_1 = 1;
		mRoomObjects[X].Object_field_2 = 0x73;
		mMemory[ 0x6F2E ] = 0x20;
		mMemory[ 0x6F30 ] = 0xCC;
		
		byte_5FD5 = (mMemory[ word_40 + 1 ] >> 2) - 4;
		byte_5FD6 = mMemory[ word_40 + 2 ] >> 3;

		word_3C_Calculate();

		mMemory[ word_3C ] &= 0xFB;
		mMemory[ word_3C + 4 ] &= 0xBF;
	}

	// 534c
	byte gfxPosX = mMemory[ word_40 + 3 ];
	byte gfxPosY = mMemory[ word_40 + 4 ];

	screenDraw( 0, 0x7A, gfxPosX, gfxPosY, 0 );

	word_40 = SavedWord40;
	word_3C = SavedWord3C;
}

// 548B: In Front Conveyor
void cCreep::obj_Conveyor_InFront( byte pSpriteNumber, byte pObjectNumber ) {

	word_40 = mConveyorPtr + mRoomObjects[pObjectNumber].objNumber;

	if( !(mMemory[ word_40 ] & byte_5648 ))
		return;
	
	byte A = mRoomSprites[pSpriteNumber].Sprite_field_0;

	if( !A ) {
		// 54B7
		if( mRoomSprites[pSpriteNumber].spriteImageID >= 6 )
			return;

	} else
		if( A != 3 && A != 5 ) 
			return;
	
	// 54BE
	A = mRoomSprites[pSpriteNumber].mX + mRoomSprites[pSpriteNumber].mWidth;
	if( (char) A >= 0 && ((char) (A - mRoomAnim[pObjectNumber].mX)) < 0 )
		return;

	A -= mRoomAnim[pObjectNumber].mX;
	if( A >= 0x20 )
		return;

	if( mMemory[ word_40 ] & byte_5647 )
		A = 0xFF;
	else
		A = 0x01;

	// 54E2
	if( !(mRoomSprites[pSpriteNumber].Sprite_field_0 == 0 && (mEngine_Ticks & 7) ))
		A <<= 1;

	mRoomSprites[pSpriteNumber].mX += A;
}

// 5611: In Front Conveyor Control
void cCreep::obj_Conveyor_Control_InFront( byte pSpriteNumber, byte pObjectNumber ) {
	if( mRoomSprites[pSpriteNumber].Sprite_field_0 )
		return;

	if( !mRoomSprites[pSpriteNumber].mButtonState )
		return;

	word_40 = mConveyorPtr + mRoomObjects[pObjectNumber].objNumber;
	byte A;

	if( mRoomSprites[pSpriteNumber].playerNumber )
		A = byte_5645;
	else
		A = byte_5646;

	mMemory[ word_40 ] |= A;
}

bool cCreep::object_Create( byte &pX ) {

	if( mObjectCount == MAX_OBJECTS )
		return false;

	pX = mObjectCount++;

	mRoomAnim[pX].clear();
	mRoomObjects[pX].clear();

	mRoomAnim[pX].mFlags = ITM_DISABLE;

	return true;
}
