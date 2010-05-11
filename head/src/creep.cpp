/*
 *  The Castles of Dr. Creep 
 *  ------------------------
 *
 *  Copyright (C) 2009-2010 Robert Crossfield
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
#include "castleManager.h"
#include "creep.h"
#include "sound/sound.h"

#ifdef WIN32
#include <fcntl.h>
#include <io.h>
#endif

cCreep::cCreep() {
	size_t romSize;

	stringstream windowTitle;
	windowTitle << "The Castles of Dr. Creep";

	mLevel = 0;
	mMenuMusicScore = 0xFF;
	mUnlimitedLives = 0;
	mQuit = false;

	// Prepare memory for use
	mMemorySize = 0x10000;
	mMemory = new byte[ mMemorySize ];
	for( size_t x = 0; x < mMemorySize; ++x )
		mMemory[x] = 0;

	mCastleManager = new cCastleManager();
	mInput = new cPlayerInput( this );
	mScreen = new cScreen( this, windowTitle.str() );
	mSound = 0;

	// Load the C64 Character Rom
	m64CharRom = local_FileRead( "char.rom", romSize, false );
	
	mGameData = mCastleManager->fileLoad( "OBJECT", romSize ) + 2;
	romSize -= 2;

	if(romSize)
		// Copy the game data into the memory buffer
		memcpy( &mMemory[ 0x800 ], mGameData, romSize );

	byte_839 = 0;

	byte_882 = 0x80;
	byte_883 = 0x40;
	byte_884 = 0x20;
	byte_885 = 0x10;
	byte_886 = 0x08;
	byte_887 = 0x04;
	byte_888 = 0x02;
	byte_889 = 0x01;
	byte_88A = 0x80;
	byte_88B = 0x40;
	byte_88C = 0x20;
	byte_88D = 0x10;
	byte_88E = 0x01;
	
	byte_8C0 = 0x80;
	byte_8C1 = 0x40;

	byte_B83 = 0;

	byte_20DE = 0x00;
	byte_24FD = 0x00;
	mInterruptCounter = 0x00;
	byte_2E36 = 0xA0;

	byte_3638 = 0xBA;

	byte_45DD = 0x80;
	byte_45DE = 0x40;
	byte_45DF = 0x20;

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
	byte_574D = 0x04;
	byte_574E = 0x02;
	byte_574F = 0x01;	
	byte_574C = 0x80;

	byte_5F57 = 0xA0;

	mMusicCurrent = "MUSIC0";
	mMusicBuffer = 0;
	mMusicBufferStart = 0;
	mMusicBufferSize = 0;

	mMenuReturn = false;

	ftime(&mTimePrevious);
}

cCreep::~cCreep() {

	delete m64CharRom;
	delete mMemory;
	delete mCastleManager;
	delete mSound;
	delete mScreen;
	delete mInput;
}

void cCreep::titleDisplay() {
	size_t size;
	byte *buffer = mCastleManager->fileLoad( "PIC A TITLE", size );
	if(!buffer)
		return;

	// Skip load address
	buffer += 0x02;	
	
	mScreen->bitmapLoad( buffer,  buffer + 0x1F40, buffer + 0x2328, 1 );
	mScreen->refresh();

	hw_IntSleep(0x10);
}

void cCreep::run( int pArgCount, char *pArgs[] ) {
	bool consoleShow = false;

	int	count = 0;
	size_t	playLevel = 0;
	bool	playLevelSet = false;
	bool	unlimited = false;

	// Output console message
	cout << "The Castles of Dr. Creep (SVN:" << SVNREV << " " << SVNDATE << ")" << endl << endl;
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

		if( arg == "-l" ) {
			playLevel = atoi( pArgs[ ++count ] );
			playLevelSet = true;
		}

		++count;
	}

	// If the level wasn't set on command line, request it from the user
	if(!playLevelSet) {
		mCastleManager->castleListDisplay();

		string lvl;
		cout << "\nPlease select a Castle: ";
		cin >> lvl;

		playLevel = atoi( lvl.c_str() );
	}

	// Level numbers begin at 1 in the list, but 0 in the actual game
	if(playLevel)
		--playLevel;

#ifdef _DEBUG
	consoleShow = true;
#endif
	// Hide the console unless its requested by user
	if( !consoleShow ) {
#ifdef WIN32
		HWND hWnd = GetConsoleWindow();
		ShowWindow( hWnd, SW_HIDE );
#endif
	}

	// Set the default screen scale
	mScreen->scaleSet( 2 );

	// Display the title screen
#ifndef _DEBUG
	titleDisplay();
#endif

	// Start main loop
	start( playLevel, unlimited );
}

void cCreep::interruptWait( byte pCount) {
	timeb tickNow;
	ftime(&tickNow);

	time_t diffSec = tickNow.time - mTimePrevious.time;
	time_t diffMil = tickNow.millitm - mTimePrevious.millitm;

	if(diffMil < 0)
		diffMil = -diffMil;

	// Calculate time taken between calls to this function
	double	sleepTime = 0;
	//diffMil = 10;

	if(diffSec <= 1) {
		if(diffMil > 30)
			sleepTime = 0;
		else
			sleepTime = 30 - diffMil;

		mInterruptCounter = pCount;
		
		while(mInterruptCounter > 0 ) {
			//
			if(sleepTime) {
				//if( mScreen->fpsGet() >= 25 ) {
					Sleep( sleepTime );
				//}
			}

			--mInterruptCounter;
			
		}
	}

	ftime(&mTimePrevious);
}

//08C2
void cCreep::start( size_t pStartLevel, bool pUnlimited ) {
	byte	byte_30, byte_31, count;

	byte_30 = 0x40;
	byte_31 = 0xD7;
	count = 0xC8;
	
	if(pUnlimited)
		mUnlimitedLives = 0xFF;

	mSound = new cSound( this );
	
	for(;;) {
		
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

	count = 0;
	while( count < 0x20 ) {
		mMemory[ 0x5CE6 + count ] = byte_30;
		mMemory[ 0x5D06 + count ] = byte_31;

		if( (word) (byte_30 + 0x28) > 0xFF)
			++byte_31;

		byte_30 += 0x28;
		++count;
	}

	// Removed 
	// 0x091B
	//copyProtection();
	gameMenuDisplaySetup();

	if( mMemory[ 0x839 ] != 1 ) {

		word_30 = 0x7572;
		word_32 = 0x77F7;
		// 93A
		for( byte Y = 0; ; ) {
			for(;;) {
				mMemory[ word_30 + Y ] = mMemory[ word_32 + Y ];
				if(++Y==0)
					break;
			}
			
			word_32 += 0x100;
			word_30 += 0x100;

			if( word_30 >= 0x7800 )
				break;
		}

		if( (mCastle = mCastleManager->castleLoad( pStartLevel )) == 0)
			return;

	} else {
		// 0x953

	}

	mainLoop();
}

// 0x7572
void cCreep::gameMenuDisplaySetup() {
	word byte_30;

	byte_30 = 0x400;

	for( char X = 3; X >= 0; --X ) {
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

		byte A = mMemory[ byte_3E + Y ];
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
		
		A = mMemory[ 0x5CE6 + X ];

		byte_30 = A | (byte_30 & 0xFF00);
		
		A = mMemory[ 0x5D06 + X ];
		A += 4;
		byte_30 = (A << 8) | (byte_30 & 0xFF);

		A = (byte_30 & 0xFF);

		A += mMemory[ byte_3E ];
		byte_30 = (A) | (byte_30 & 0xFF00);
		
		//0x75E1
		byte_3E += 3;
		
		for(;;) {
			A = mMemory[ byte_3E + Y] & 0x3F;
			mMemory[ byte_30 + Y ] = A;
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
	A = mMemory[ 0x5CE6 + 7 ];
	byte_30 = (A & 0xFF) | (byte_30 & 0xFF00);

	// 762F
	byte_30 = ((4 + mMemory[ 0x5D06 + x ]) << 8) | (byte_30 & 0xFF);
	
	for( byte Y = 0x17; Y < 0x1A; ++Y ) {
		A = mMemory[ byte_30 + Y ] | 0x80;
		mMemory[ byte_30 + Y] = A;
	}

	sub_2973();
}

// 268F: 
void cCreep::textShow() {
	byte_B83 = 0;

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
			if( byte_B83 != 1 &&  mInput->restoreGet() != true) {

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

	word_3E = 0x27A2;
	stringDraw();
}

// 27A8
byte cCreep::textGetKeyFromUser() {
	byte key = mInput->keyGet();
	
	if( key == 0 )
		return 0x80;

	return toupper( key );
}

// 2973: 
void cCreep::sub_2973() {
	byte byte_29AB, byte_29AC, byte_29AD;

	byte_29AB = byte_29AC = byte_29AD = 0xF8;

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

	mMemory[ 0xD025 ] = 0x0A;
	mMemory[ 0xD026 ] = 0x0D;

	mMemory[ 0x21 ] = 0;
	mScreen->spriteDisable();

	interruptWait( 2 );

}

// 0B84
void cCreep::mainLoop() {

	mScreen->bitmapLoad( &mMemory[ 0xE000 ], &mMemory[ 0xCC00 ], &mMemory[ 0xD800 ], 0 );

	//mCastle->castleStart( 2 );

	while(!mQuit) {
		
		if( Intro() == true )
			continue;

		Game();
	}

}

// 18E4
void cCreep::screenClear() {
	word word_30 = 0xFF00;

	byte Y = 0xF9;
	
	mScreen->clear(0);
	mInput->inputCheck( true );

	// Disable all sprites
	mScreen->spriteDisable();

	// Clear screen memory
	for( ; word_30 >= 0xE000; word_30 -= 0x0100) {

		for(;;) {
			mMemory[ word_30 + Y ] = 0;
			--Y;
			if(Y == 0xFF)
				break;
		}
	}

	for( Y = 0;; ) {
		mMemory[ 0xBD04 + Y ] = byte_889;
		Y += 0x20;
		if(!Y)
			break;
	}

	mImageCount = 0;
	mScreen->screenRedrawSet();
	mScreen->bitmapRedrawSet();
}

// 13F0
void cCreep::roomLoad() {
	
	screenClear();

	word_30 = 0xC000;

	while( word_30 < 0xC800) {
		mMemory[word_30++] = 0;
	}

	byte X, A;

	// 1411
	if( mMemory[0x11C9] != 1 )
		X = 1;
	else
		X = 0;

	if( mIntro )
		A = mMenuScreenCount;
	else
		A = mMemory[ 0x7809 + X ];

	lvlPtrCalculate( A );
	
	// Room Color
	A = mMemory[word_42] & 0xF;

	graphicRoomColorsSet( A );

	//14AC
	// Ptr to start of room data
	word_3E = readWord( &mMemory[word_42 + 6] );

	if(mIntro)
		word_3E += 0x2000;

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

	for( char Y = 7; Y >= 0; --Y ) {
		*memory( 0x6FB2 + Y ) = pRoomColor;
		*memory( 0x6FF5 + Y ) = pRoomColor;
		*memory( 0x7038 + Y ) = pRoomColor;
		*memory( 0x707B + Y ) = pRoomColor;
	}
	
	pRoomColor &= 0x0F;
	pRoomColor |= 0x10;
	*memory( 0x6584 ) = pRoomColor;
	*memory( 0x659B ) = *memory( 0x65CD ) = (*memory( 0x649B ) & 0xF0) | 0x01;
}

// 15E0
void cCreep::roomPrepare( ) {
	word func = 0x01;

	while(func) {

		func = readWord( &mMemory[ word_3E ]);
		word_3E  += 2;

		switch( func ) {
			case 0:			// Finished
				return;

			case 0x0803:	// Doors
				obj_Door_Prepare( );
				break;
			case 0x0806:	// Walkway
				obj_Walkway_Prepare( );
				break;
			case 0x0809:	// Sliding Pole
				obj_SlidingPole_Prepare( );
				break;
			case 0x080C:	// Ladder
				obj_Ladder_Prepare( );
				break;
			case 0x080F:	// Doorbell
				obj_Door_Button_Prepare( );
				break;
			case 0x0812:	// Lightning Machine
				obj_Lightning_Prepare( );
				break;
			case 0x0815:	// Forcefield
				obj_Forcefield_Prepare( );
				break;
			case 0x0818:	// Mummy
				obj_Mummy_Prepare( );
				break;
			case 0x081B:	// Key
				obj_Key_Load( );
				break;
			case 0x081E:	// Lock
				obj_Door_Lock_Prepare( );
				break;
			case 0x0824:	// Ray Gun
				obj_RayGun_Prepare( );
				break;
			case 0x0827:	// Teleport
				obj_Teleport_Prepare( );
				break;
			case 0x082A:	// Trap Door
				obj_TrapDoor_Prepare( );
				break;
			case 0x082D:	// Conveyor
				obj_Conveyor_Prepare( );
				break;
			case 0x0830:	// Frankenstein
				obj_Frankie_Load( );
				break;
			case 0x0833:	// String Print
			case 0x2A6D:
				obj_stringPrint();
				break;
			case 0x0836:
				obj_Image_Draw();
				break;
			case 0x0821:
			case 0x160A:	// Intro
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

	for(;;) {
		++mMenuScreenTimer;
		
		mMenuScreenTimer &= 3;

		if( mMenuScreenTimer != 0 ) {
			++mMenuScreenCount;
			lvlPtrCalculate( mMenuScreenCount );

			if( ((*level( word_42 )) & 0x40) )
				mMenuScreenCount = 0;

			roomLoad();
		} else {
			word_3E = 0x0D1A;

			screenClear();
			roomPrepare( );
		}
		
		// 0BE1 
		if( byte_20DE != 1 ) {
		
			// Play music straight away when debug mode (music testing)
#ifdef _DEBUG
			if(!mMenuMusicScore)
				mMenuMusicScore = 1;
#endif
			if( !mMenuScreenTimer ) {
				if( mMenuMusicScore == 0 )
					++mMenuMusicScore;
				else {
					// 0C49
					musicChange();
					byte_20DE = 1;
					return true;
				}
			}
		}

		byte_D12 = 0xC8;

		for(;;) {

			if( mMenuScreenTimer )
				events_Execute();
			else {
				// C0D
				interruptWait( 2 );

				mInterruptCounter = 2;
			}

			hw_Update();

			// C17
			KeyboardJoystickMonitor( byte_D10 );
			if( byte_5F57 )
				break;

			// Change which player controller is checked
			byte_D10 ^= 0x01;

			// Display Highscores on F3
			if( mInput->f3Get() ) {
				sub_95F();
				gameHighScores();

				word_3E = 0x239C;
				obj_stringPrint();

				*memory(0x278C) = 0;
				textShow();

				return true;
			}

			if( mRunStopPressed == 1 ) {
				optionsMenu();
				if( byte_24FD == 1 ) {
					byte_5F57 = 1;
					break;
				}

				return true;
			}
			--byte_D12;
			if( byte_D12 == 0 )
				break;
		}

		if( byte_5F57 )
			break;
	}
	
	// 0CDD
	byte_20DE = 0;
	mIntro = 0;
	mMusicBuffer = 0;

	// Disable music playback
	mSound->playback( false );

	char X = 0x0E;

	while(X >= 0) {
		mMemory[ 0x20EF + X ] &= 0xFE;
		mMemory[ 0xD404 + X ] = mMemory[ 0x20EF + X ];
		mSound->sidWrite(0x04 + X, mMemory[ 0x20EF + X ]);

		X -= 0x07;
	}


	return false;
}

// 20A9 : 
void cCreep::musicPtrsSet() {
	byte X = (*memory( 0x20CB ) & 3);
	
	mVoiceNum = X;

	X <<= 1;

	mVoice = *((word*) memory( 0x20DF + X));
	mVoiceTmp = *((word*) memory( 0x20E5 + X));
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
		byte A = *mMusicBuffer;
		A >>= 2;

		byte X;

		X = *memory( 0x20D2 + A );

		for( char Y = X - 1; Y >= 0; --Y )
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
				
				for( char Y = 2; Y < 7; ++Y ) {
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
		*memory( 0x2232 ) = 0xFF;
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
	for( char X = 0x0E; X >= 0; X -= 7 ) {
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

// 2233 : Intro Menu
void cCreep::optionsMenu() {
	// TODO:
	return;

	for(;; ) {
		mScreen->spriteDisable();

		word_30 = 0xD800;
		for(byte Y = 0;;) {

			byte A = 1;
			
			for( ;Y != 0; ++Y ) 
				mMemory[ word_30 + Y ] = A;

			word_30 += 0x100;
			if( word_30 >= 0xDC00 )
				break;
		}
		
		// 226E
		for(;;) {
			
			KeyboardJoystickMonitor(0);
			if( !byte_5F57 ) {
				
				if( byte_5F56 & 0xFB )
					continue;
				
				// 227F
				
			} else {
				// 22E5
			}


		}
	}
}

// 5EFC
void cCreep::KeyboardJoystickMonitor( byte pA ) {
	byte X = 0xFF, A = 0;
	
	byte_5F58 = pA;
	mRunStopPressed = false;

	sPlayerInput *input = mInput->inputGet( pA );

	// Pause the game, or enter the options menu
	if( mInput->runStopGet() )
		mRunStopPressed = true;

	// Kill the player(s) if the restore key is pressed
	if( mInput->restoreGet() ) {
		byte_B83 = 1;
	}

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
	byte_5F57 = A;
}

void cCreep::events_Execute() {
	// 2E1D
	interruptWait( 2 );

	// Get collisions from the hardware, and set them in the objects
	obj_CollisionSet();

	// Check for any actions to execute, 
	// including collisions, movement, general checks
	obj_Actions();

	// Check for any other 'background' actions to execute
	img_Actions();
	++byte_2E36;
}

// 29AE: 
void cCreep::convertTimerToTime() {
	
	byte A = mMemory[ word_3E + 1 ];
	convertTimeToNumber( A, 6 );

	A = mMemory[ word_3E + 2 ];
	convertTimeToNumber( A, 3 );
	
	A = mMemory[ word_3E + 3 ];
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
void cCreep::obj_CollisionSet() {
	byte gfxSpriteCollision = 0, gfxBackgroundCollision = 0;

	vector< sScreenPiece *>				*collisions = mScreen->collisionsGet();
	vector< sScreenPiece *>::iterator	 colIT;

	for( colIT = collisions->begin(); colIT != collisions->end(); ++colIT ) {
		sScreenPiece *piece = *colIT;

		if( piece->mSprite2 == 0 ) {
			// Background collision
			gfxBackgroundCollision |= (1 << (piece->mSprite-1) );

		} else {
			// Sprite collision
			gfxSpriteCollision |= (1 << (piece->mSprite-1) );
			gfxSpriteCollision |= (1 << (piece->mSprite2-1) );
		
		}
	}

	// Original
	//gfxSpriteCollision = mMemory[ 0xD01E ];
	//gfxBackgroundCollision = mMemory[ 0xD01F ];

	byte X = 0;
	for(;;) {
		byte A = mMemory[ 0xBD04 + X ];
		if( A & byte_889 ) {
			gfxSpriteCollision >>= 1;
			gfxBackgroundCollision >>= 1;
		} else {

			A &= 0xF9;
			if( gfxSpriteCollision & 0x01 )
				A |= 2;
			gfxSpriteCollision >>= 1;
			
			if( gfxBackgroundCollision & 0x01 )
				A |= 4;
			gfxBackgroundCollision >>= 1;

			mMemory[ 0xBD04 + X ] = A;
		}

		X += 0x20;
		if(!X)
			break;
	}
}

// 2E79: Execute any objects with actions / collisions, enable their sprites
void cCreep::obj_Actions( ) {
	byte X = 0, A;
	byte w30 = 0;

	for(;;) {
		A = mMemory[ 0xBD04 + X ];

		if(! (A & byte_889) ) {
			// 2E8B
			if(! (A & byte_885) ) {
				
				if(! (A & byte_883) ) {
					--mMemory[ 0xBD05 + X ];

					if( mMemory[ 0xBD05 + X ] != 0 ) {
						if((A & byte_888)) {
							obj_CheckCollisions(X);
							A = mMemory[ 0xBD04 + X];
							if((A & byte_883))
								goto s2EF3;
						}
						
						goto s2F72;
					} else {
					// 2EAD
						if(A & byte_884) 
							goto s2EF3;
						if((A & byte_887)) {
							obj_OverlapCheck( X );
							if( mMemory[ 0xBD04 + X ] & byte_883)
								goto s2EF3;
						}
						// 2EC2
						if(!(mMemory[ 0xBD04 + X ] & byte_888))
							goto s2ED5;

						obj_CheckCollisions(X);
						if( mMemory[ 0xBD04 + X ] & byte_883)
							goto s2EF3;

						goto s2ED5;
					}

				} else {
					// 2EF3
s2EF3:
					sprite_FlashOnOff( X );
				}

			} else {
				// 2ED5
s2ED5:
				obj_Actions_Execute(X);
				if( mMemory[ 0xBD04 + X ] & byte_883 )
					goto s2EF3;
			}
			// 2EF6
			if( mMemory[ 0xBD04 + X ] & byte_885 )
				goto s2ED5;

			byte Y = (X >> 5);
			cSprite *sprite = mScreen->spriteGet( Y );

			if( (mMemory[ 0xBD04 + X ] & byte_886) ) {
				mMemory[ 0xBD04 + X ] = byte_889;
				goto s2F51;
			} else {
				// 2F16
				word_30 = mMemory[ 0xBD01 + X ];
				word_30 <<= 1;

				w30 = (word_30 & 0xFF00) >> 8;

				// Sprite X
				mMemory[ 0x10 + Y ] = (word_30 - 8);
				sprite->_X = (word_30 - 8);

				if((word_30 >= 0x100) && ((word_30 - 8) < 0x100))
					--w30;

				if( (w30 & 0x80) ) {
s2F51:;
					// 2f51
					A = (mMemory[ 0x2F82 + Y ] ^ 0xFF);
					A &= mMemory[ 0x21 ];
					sprite->_rEnabled = false;

				} else {
					if( w30 ) {
						A = (mMemory[ 0x20 ] | mMemory[ 0x2F82 + Y ]);
					} else
						A = (mMemory[ 0x2F82 ] ^ 0xFF) & mMemory[ 0x20 ];

					// Sprites X Bit 8
					mMemory[ 0x20 ] = A;

					// 2F45
					if((A & mMemory[ 0x2F82 + Y ]) && (mMemory[ 0x10 + Y ] >= 0x58) && w30 ) {
						A = (mMemory[ 0x2F82 + Y ] ^ 0xFF) & mMemory[ 0x21 ];
						sprite->_rEnabled = false;
					} else {
						// 2F5B
						sprite->_Y = ((char) mMemory[ 0xBD02 + X ]) + 0x32;
						mMemory[ 0x18 + Y ] = mMemory[ 0xBD02 + X ] + 0x32;
						A = mMemory[ 0x21 ] | mMemory[ 0x2F82 + Y ];

						sprite->_rEnabled = true;
					}
				}

				// 2F69
				// Enabled Sprites
				mMemory[ 0x21 ] = A;
				mMemory[ 0xBD05 + X ] = mMemory[ 0xBD06 + X ];
			}
				
		}
s2F72:
		// 2F72
		X += 0x20;
		if( !X )
			break;
	}

}

// 311E
void cCreep::obj_OverlapCheck( byte pX ) {

	byte_31F1 = mMemory[ 0xBD01 + pX ];
	byte_31F2 = byte_31F1 + mMemory[ 0xBD0A + pX ];
	if( (mMemory[ 0xBD01 + pX ] +  mMemory[ 0xBD0A + pX ]) > 0xFF )
		byte_31F1 = 0;

	byte_31F3 = mMemory[ 0xBD02 + pX ];
	byte_31F4 = byte_31F3 + mMemory[ 0xBD0B + pX ];
	if( (mMemory[ 0xBD02 + pX ] +  mMemory[ 0xBD0B + pX ]) > 0xFF )
		byte_31F3 = 0;

	// 3149
	if( !mImageCount )
		return;

	byte_31EF = mImageCount << 3;

	byte Y = 0;

	do {
		byte_31F0 = Y;

		if( !(mMemory[ 0xBF04 + Y ] & byte_83F ))
			if( !(byte_31F2 < mMemory[ 0xBF01 + Y ] ))
				if( !(mMemory[ 0xBF01 + Y ] + mMemory[ 0xBF05 + Y ] < byte_31F1))
					if( !(byte_31F4 < mMemory[ 0xBF02 + Y ]) )
						if( !(mMemory[ 0xBF02 + Y ] + mMemory[ 0xBF06 + Y ] < byte_31F3) ) {
						//318C
							byte_31F5 = 1;
							Y = mMemory[ 0xBD00 + pX ]  << 3;

							if( obj_Actions_Collision( pX, Y ) == true ) {

								if( byte_31F5 == 1 ) 
									mMemory[ 0xBD04 + pX ] |= byte_883;
							}

							Y = mMemory[ 0xBF00 + byte_31F0 ] << 2;
							obj_Actions_InFront( pX, Y );
						} 

		Y = byte_31F0 + 8;

	} while(Y != byte_31EF);
}

bool cCreep::obj_Actions_Collision( byte pX, byte pY ) {
	word func = readWord( &mMemory[ 0x893 + pY ]);
	
	pY = byte_31F0;

	switch( func ) {
		case 0:
			return false;

		case 0x34EF:		// Player In Front
			obj_Player_Collision( pX, pY );
			break;

		case 0x38CE:		// Mummy
			obj_Mummy_Collision( pX, pY );
			break;

		case 0x3940:
			sub_3940( pX, pY );
			break;

		case 0x3A60:		//
			sub_3A60( pX, pY );
			break;

		case 0x3D6E:		// Frankie
			obj_Frankie_Collision( pX, pY );
			break;

		default:
			cout << "obj_Actions_Collision: 0x";
			cout << std::hex << func << "\n";
			break;

	}

	return true;
}

bool cCreep::obj_Actions_InFront( byte pX, byte pY ) {
	word func = readWord( &mMemory[ 0x844 + pY ] );
	
	pY = byte_31F0;

	switch( func ) {
		case 0:
			return false;
			break;
		
		case 0x4075:		// In Front Door
			obj_Door_InFront( pX, pY );
			break;

		case 0x41D8:		// In Front Button
			obj_Door_Button_InFront( pX, pY );
			break;
		
		case 0x44E7:		// In Front Lightning Switch
			obj_Lightning_Switch_InFront( pX, pY );
			break;

		case 0x4647:		// In Front Forcefield Timer
			obj_Forcefield_Timer_InFront( pX, pY );
			break;

		case 0x47A7:		// In Front Mummy Release
			obj_Mummy_Infront( pX, pY );
			break;

		case 0x4990:		// In Front Key
			obj_Key_Infront( pX, pY );
			break;

		case 0x4A68:		// In Front Lock
			obj_Door_Lock_InFront( pX, pY );
			break;
		
		case 0x4D70:		// In Front RayGun Control
			obj_RayGun_Control_InFront( pX, pY );
			break;

		case 0x4EA8:		// In Front Teleport
			obj_Teleport_InFront( pX, pY );
			break;
		
		case 0x548B:		// In Front Conveyor
			obj_Conveyor_InFront( pX, pY );
			break;

		case 0x5611:		// In Front Conveyor Control
			obj_Conveyor_Control_InFront( pX, pY );
			break;

		default:
			cout << "obj_Actions_InFront: 0x";
			cout << std::hex << func << "\n";

			break;

	}

	return true;
}

// 30D9
void cCreep::obj_Actions_Hit( byte pX, byte pY ) {
	
	if( mMemory[ 0xBD04 + pX ] & byte_884 )
		return;

	byte_311D = 1;
	mMemory[ 0x311C ] = pY;
	
	byte Y = mMemory[ 0xBD00 + pX ] << 3;
	word func = readWord( &mMemory[ 0x891 + Y ] );

	switch( func ) {
		case 0:
			break;

		case 0x3534:				//  Hit Player
			obj_Player_Hit( pX, pY );
			break;

		case 0x3682:
			byte_311D = 0;
			break;

		case 0x3DDE:				//  Hit Frankie
			obj_Frankie_Hit( pX, pY );
			break;

		default:
			cout << "obj_Actions_Hit: 0x";
			cout << std::hex << func << "\n";
			break;
	}

	// 3104
	if( byte_311D != 1 )
		return;

	mMemory[ 0xBD04 + pX ] |= byte_883;
}

// 3026
void cCreep::obj_CheckCollisions( byte pX ) {
	byte byte_3115 = pX, byte_3116;
	byte byte_311A, byte_311B, byte_3117, byte_3118, byte_3119;

	byte Y = mMemory[ 0xBD00 + pX ] << 3;
	
	byte A = mMemory[ 0x895 + Y ];
	
	if(!(A & 0x80)) {
		byte_311B = A;
		byte_3117 = mMemory[ 0xBD01 + pX ];
		byte_3118 = byte_3117 + mMemory[ 0xBD0A + pX];
		if( (byte_3117 + mMemory[ 0xBD0A + pX ]) > 0x80 )
			byte_3117 = 0;

		byte_3119 = mMemory[ 0xBD02 + pX ];
		byte_311A = byte_3119 + mMemory[ 0xBD0B + pX ];
		if( (byte_3119 + mMemory[ 0xBD0B + pX ]) > 0x80 )
			byte_3119 = 0;

		Y = 0;
		for(;;) {
			// 3068
			byte_3116 = Y;
			if( byte_3115 != byte_3116 ) {
				A = mMemory[ 0xBD04 + Y];

				if( !(A & byte_889) ) {
					
					if( (A & byte_888) ) {
						Y = mMemory[ 0xBD00 + Y ] << 3;
						A = mMemory[ 0x395 + Y ];

						if( !(A & 0x80 )) {
							if(! (A & byte_311B )) {
								// 808E
								Y = byte_3116;
								A = byte_3118;
								if( A >= mMemory[ 0xBD01 + Y ] ) {
									A = mMemory[ 0xBD01 + Y ] + mMemory[ 0xBD0A + Y ];
									if( A >= byte_3117 ) {
										// 30A5
										if( byte_311A >= mMemory[ 0xBD02 + Y ] ) {
											if( (mMemory[ 0xBD02 + Y ] + mMemory[ 0xBD0B + Y ]) >= byte_3119 ) {
												obj_Actions_Hit( pX, Y);
												pX = byte_3116;
												Y = byte_3115;
												obj_Actions_Hit( pX, Y );
											}
											// 30C5
											pX = byte_3115;
											Y = byte_3116;
										}
									}
								}
							}
						}
					}
				}
			}

			pX = byte_3115;
			Y = byte_3116;
			// 30CB
			Y+= 0x20;
			if(!Y)
				break;
		}
	}
	// 30D5
}

//2F8A
void cCreep::sprite_FlashOnOff( byte pX ) {
	byte A = mMemory[ 0xBD04 + pX ];
	byte Y = 0;

	mScreen->spriteRedrawSet();
	cSprite *sprite = mScreen->spriteGet( pX >> 5 );

	if( !(A & byte_883) ) {
		
		if( !(mMemory[ 0xBD08 + pX ] )) {
			A = mMemory[ 0xBD04 + pX ] ^ byte_884;
		} else
			goto s2FE9;
	
	} else {
		// 2FA3
		A ^= byte_883;
		mMemory[ 0xBD04 + pX ] = A;
		Y = mMemory[ 0xBD00 + pX ] << 3;
		
		A = mMemory[ 0x896 + Y ];
		if(!(A & byte_88E))
			A = mMemory[ 0xBD04 + pX ];
		else
			goto s2FC4;
	}
	// 2FBB
	mMemory[ 0xBD04 + pX ] = A | byte_885;
	return;

s2FC4:;
	mMemory[ 0xBD08 + pX ] = 8;
	Y = pX >> 5;

	// Sprite multicolor mode
	mMemory[ 0xD01C ] = (mMemory[ 0x2F82 + Y ] ^ 0xFF) & mMemory[ 0xD01C ];
	sprite->_rMultiColored = false;
	
	mMemory[ 0xBD04 + pX ] |= byte_884;
	mMemory[ 0xBD06 + pX ] = 1;

s2FE9:;
	if(! (byte_2E36 & 1) ) {
		// Flash On
		// 2FF0
		Y = pX >> 5;
		sprite->_color = 1;

		mMemory[ 0xD027 + Y ] = 1;
		--mMemory[ 0xBD08 + pX ];

		mMemory[ 0x760C ] = mMemory[ 0xBD08 + pX ] << 3;
		sub_21C8(8);
		
	} else {
		// Flash Off
		// 3010
		Y = pX >> 5;
		sprite->_color = 0;
		mMemory[ 0xD027 + Y ] = 0;
	}

	// 301C
	mMemory[ 0xBD05 + pX ] = mMemory[ 0xBD06 + pX ];
}

// Originally this was not a function, but its too big to bother
// implementing in the original location
void cCreep::obj_Actions_Execute( byte pX ) {
	//2ED5
	byte Y =  mMemory[ 0xBD00 + pX ] << 3;
	word func = readWord( &mMemory[ 0x88F + Y ]);
	
	switch(func) {

		case 0x31F6:
			obj_Player_Execute( pX );
			break;
		
		case 0x3639:
			obj_Lightning_Execute( pX );
			break;

		case 0x36B3:
			obj_Forcefield_Execute( pX );
			break;
		
		case 0x379A:
			obj_Mummy_Execute( pX );
			break;

		case 0x3A08:
			obj_RayGun_Laser_Execute( pX );
			break;

		case 0x3AEB:
			obj_Frankie_Execute( pX );
			break;

		default:
			cout << "obj_Actions_Execute: 0x";
			cout << std::hex << func << "\n";
			break;
	}

}

void cCreep::obj_Player_Execute( byte pX ) {
	byte A =  mMemory[ 0xBD04 + pX ];

	// Player leaving room?
	if( A & byte_885 ) {
		A ^= byte_885;
		A |= byte_886;
		mMemory[ 0xBD04 + pX ] = A;

		char Y = mMemory[ 0xBD1C + pX ] << 1;
		
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
		word_30 = readWord( &mMemory[ 0x34E7 + Y ] );
		word_32 = readWord( &mMemory[ 0x34EB + Y ] );

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
	if( A & byte_882 ) {
		A ^= byte_882;
		mMemory[ 0xBD04 + pX ] = A;

		// Current Player
		char Y = mMemory[ 0xBD1C + pX ] << 1;

		if( Y == 0 )
			ftime(&mPlayer1Time);

		if( Y == 2 )
			ftime(&mPlayer2Time);

		// Get Ptr to CIA Timer and store area
		word_32 = readWord( &mMemory[ 0x34E7 + Y ] );
		word_30 = readWord( &mMemory[ 0x34EB + Y ] );

		// Restore CIA Timer
		//for( Y = 3; Y >= 0; --Y ) 
		//	mMemory[ word_32 + Y ] = mMemory[ word_30 + Y ];
		
		Y = mMemory[ 0xBD1C + pX ];
		A = mMemory[ 0x780D + Y ];
		if( A != 6 ) {
			obj_Player_Unk( pX );
			goto s32DB;
		}

	} else {
		// 3269
		byte Y = mMemory[ 0xBD1C + pX ];
		A = mMemory[ 0x780D + Y ];

		if( A == 5 ) {
			//3280
			byte_34D6 = Y;
			Y = mMemory[ 0xBD1B + pX ];
			A = mMemory[ 0x34A4 + Y ];

			if( A != 0xFF ) {
				Y = byte_34D6;
				mMemory[ 0x780D + Y ] = A;
				mMemory[ 0xBD06 + pX ] = 1;
				A = mMemory[ 0x780D + Y ];
				goto s32CB;

			} else {
				// 329E
				mMemory[ 0xBD1B + pX ] += 0x04;
				Y = mMemory[ 0xBD1B + pX ];

				mMemory[ 0xBD01 + pX ] += mMemory[ 0x34A1 + Y ];
				mMemory[ 0xBD02 + pX ] += mMemory[ 0x34A2 + Y ];
			}

		} else if( A == 6 ) {
			mMemory[ 0x780D + Y ] = 5;
		} else
			goto s32CB;
	}
	// 32BC
	mMemory[ 0xBD03 + pX ] = mMemory[ 0x34A3 + mMemory[ 0xBD1B + pX ] ];
	obj_Player_Unk( pX );
	return;
s32CB:;
	
	if( A != 0 ) {
		mMemory[ 0xBD04 + pX ] |= byte_885;
		return;
	}

s32DB:;
	A = mMemory[ 0xBD1A + pX ];
	char a = A;
	if( A != 0xFF )
		if( A != mMemory[ 0xBD19 + pX ] ) {
			sub_526F( a );
			A = a;
		}

	mMemory[ 0xBD19 + pX ] = A;
	mMemory[ 0xBD1A + pX ] = 0xFF;
	sub_5F6B( pX );
	
	byte byte_34D5 = mMemory[ word_3C ] & mMemory[ 0xBD18 + pX ];
	//32FF
	mMemory[ 0xBD18 + pX ] = 0xFF;

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
	KeyboardJoystickMonitor( mMemory[ 0xBD1C + pX ] );
	mMemory[ 0xBD1D + pX ] = byte_5F57;
	mMemory[ 0xBD1E + pX ] = byte_5F56;
	
	byte Y = byte_5F56;
	if( !(Y & 0x80 )) {

		if( mMemory[ 0x2F82 + Y ] & byte_34D5 ) {
			mMemory[ 0xBD1F + pX ] = Y;
			goto s33DE;

		} 

		// 33B2
		A = mMemory[ 0xBD1F + pX ];

		if(!( A & 0x80 )) {
			A += 1;
			A &= 7;
			if( A != byte_5F56 ) {
				A -= 2;
				A &= 7;
				if( A != byte_5F56 )
					goto s33D6;
			}
			
			if( mMemory[ 0x2F82 + mMemory[ 0xBD1F + pX ] ] & byte_34D5 )
				goto s33DE;
		}
	}
s33D6:;
	// 33D6
	mMemory[ 0xBD1F + pX ] = 0x80;
	return;

	// Player Input
	// 33DE
s33DE:;
	A = (mMemory[ 0xBD1F + pX ] & 3);

	if( A == 2 ) {
		mMemory[ 0xBD02 + pX ] -= byte_5FD8;

	} else {
		// 33F4
		if( A == 0 ) {
			mMemory[ 0xBD01 + pX ] -= byte_5FD7;
			++mMemory[ 0xBD01 + pX ];
		}
	}
	// 3405
	Y = mMemory[ 0xBD1F + pX ];
	mMemory[ 0xBD01 + pX ] += mMemory[ 0x34D7 + Y ];
	mMemory[ 0xBD02 + pX ] += mMemory[ 0x34DF + Y ];

	if( !(Y & 3) ) {
		// 3421
		if( byte_34D5 & 1 ) {
			A = mMemory[ 0xBD1F + pX ];
			if( !A )
				++mMemory[ 0xBD03 + pX ];
			else 
				--mMemory[ 0xBD03 + pX ];
			
			// 3436
			// Ladder Movement 
			A = mMemory[ 0xBD03 + pX ];
			if( A >= 0x2E ) {
				// 3445
				// Moving Up Ladder
				if( A >= 0x32 )
					mMemory[ 0xBD03 + pX ] = 0x2E;
			} else {
				// 343D
				// Moving Down Ladder
				mMemory[ 0xBD03 + pX ] = 0x31;
			}

		} else {
			//3451
			// Down Pole
			mMemory[ 0xBD03 + pX ] = 0x26;
		}

	} else {
		// 3459
		// Player Frame
		++mMemory[ 0xBD03 + pX ];

		if( mMemory[ 0xBD1F + pX ] < 4 ) {
			// 3463
			A = mMemory[ 0xBD03 + pX ];
			if( A >= 6 || A < 3 )
				mMemory[ 0xBD03 + pX ] = 3;

		} else {
			// 3476
			// Max frame reached?
			if( mMemory[ 0xBD03 + pX ] >= 3 )
				mMemory[ 0xBD03 + pX ] = 0;
		}
	}

	// 3482
	obj_Player_Unk( pX );

}

// 3488: 
void cCreep::obj_Player_Unk( byte pX ) {
	hw_SpritePrepare( pX );

	byte_34D6 = pX >> 5;

	mMemory[ 0xD027 + byte_34D6 ] = mMemory[ 0x34D3 + mMemory[ 0xBD1C + pX ] ];
	
	cSprite *sprite = mScreen->spriteGet( byte_34D6 );

	sprite->_color = mMemory[ 0x34D3 + mMemory[ 0xBD1C + pX ] ];
	mScreen->spriteRedrawSet();
}

// 3AEB: Frankie Movement
void cCreep::obj_Frankie_Execute( byte pX ) {
	byte Y;
	char A = mMemory[ 0xBD04 + pX ];
	byte byte_3F0B, byte_3F12;
	char byte_3F0A, byte_3F10, byte_3F11;

	if( A & byte_885 ) {
		mMemory[ 0xBD04 + pX ] = (A ^ byte_885) | byte_886;
		return;
	}

	if( A & byte_882 ) 
		mMemory[ 0xBD04 + pX ] ^= byte_882;

	word_40 = word_5748 + mMemory[ 0xBD1F + pX ];
	
	if( !(mMemory[ 0xBD1E + pX ] & byte_574E) ) {
		if( mIntro == 1 )
			return;

		// 3B31
		for(byte_3F0A = 1; byte_3F0A >= 0; --byte_3F0A) {
			Y = byte_3F0A;

			if( mMemory[ 0x780D + Y ] != 0 )
				continue;

			Y = mMemory[ 0x34D1 + Y ];
			A = mMemory[ 0xBD02 + pX ];
			A -= mMemory[ 0xBD02 + Y ];

			// Within 4 on the Y axis, then frank can wake up
			if( (byte) A >= 4 )
				continue;

			// 3B4A
			A = mMemory[ 0xBD01 + pX ];
			A -= mMemory[ 0xBD01 + Y ];

			if( !(mMemory[ 0xBD01 + pX ] >= 0 && A < 0)) {
				// We are behind frank

				A = mMemory[ 0xBD1E + pX ];
				if( !(A & byte_574F) )
					continue;
				else
					goto s3B6E;
			}

			// 3B5E
			A = mMemory[ 0xBD1E + pX ];
			if( !(A & byte_574F)) {
s3B6E:
				A |= byte_574E;
				mMemory[ 0xBD1E + pX ] = A;
				mMemory[ word_40 ] = A;
				mMemory[ 0xBD1D + pX ] = 0x80;

				sub_21C8(0x07);
				break;
			}

		}
	
		if(byte_3F0A < 0 )
			return;

	}
	
	// 3B82
	A = mMemory[ 0xBD1B + pX ];

	if( (byte) A != 0xFF )
		if( A != mMemory[ 0xBD1A + pX ] ) {
			sub_526F(A);
		}

	mMemory[ 0xBD1A + pX ] = A;
	mMemory[ 0xBD1B + pX ] = 0xFF;
	sub_5F6B( pX );

	//3B9C
	A = mMemory[ word_3C ] & mMemory[ 0xBD1C + pX ];
	byte byte_3F13 = A;

	mMemory[ 0xBD1C + pX ] = 0xFF;
	A = byte_3F13;
	if(!A) {
		mMemory[ 0xBD1D + pX ] = 0x80;
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
			mMemory[ 0xBD1D + pX ] = byte_3F0B;
			goto s3CB4;
		}
		if( byte_3F0A == 2 ) {
			byte Y = (byte_3F0B - 4) & 7;
			if( mMemory[ 0x2F82 + Y ] & byte_3F13 ) {
				Y = mMemory[ 0xBD1D + pX ];
				if( !(Y & 0x80 ))
					if( mMemory[ 0x2F82 + Y ] & byte_3F13 )
						goto s3CB4;
			}
		}
		// 3C06
		for(char Y = 3; Y >= 0; --Y) {
			mMemory[ 0x3F0C + Y ] = 0xFF;
		}

		byte_3F0A = 1;

		// 3C15
		for(;;) {
			byte Y = byte_3F0A;
			if( mMemory[ 0x780D + Y ] == 0 ) {
				
				Y = mMemory[ 0x34D1 + Y ];
				A = mMemory[ 0xBD01 + Y ];
				A -= mMemory[ 0xBD01 + pX ];
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

				Y = byte_3F0A;
				Y = mMemory[ 0x34D1 + Y ];
				A = mMemory[ 0xBD02 + Y ];
				A -= mMemory[ 0xBD02 + pX ];
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
			--byte_3F0A;
			if( byte_3F0A < 0 )
				break;
		}

		// 3C67
		byte_3F10 = -1;
		for(;;) {
			byte_3F11 = 0x00;
			byte_3F12 = 0xFF;
			
			for( char Y = 3; Y >= 0; --Y ) {
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
				mMemory[ 0xBD1D + pX ] = 0x80;
				goto s3CB4;
			}

			A = mMemory[ 0x2F82 + (byte_3F12 << 1) ];
			if( A & byte_3F13 ) {
				// 3CB0
				mMemory[ 0xBD1D + pX ] = (byte_3F12 << 1);
				break;
			}

			byte_3F10 = byte_3F11;
		}

	}

	// 3CB4
s3CB4:;
	if( mMemory[ 0xBD1D + pX ] & 2 ) {
		mMemory[ 0xBD02 + pX ] -= byte_5FD8;

		++mMemory[ 0xBD03 + pX ];
		if( mMemory[ 0xBD1D + pX ] != 2 ) {
			// 3ccf
			--mMemory[ 0xBD01 + pX ];
			if( mMemory[ 0xBD03 + pX ] >= 0x87 )
				if( mMemory[ 0xBD03 + pX ] < 0x8A )
					goto s3D4C;
			
			mMemory[ 0xBD03 + pX ] = 0x87;
			
		} else {
			// 3ce5
			++mMemory[ 0xBD01 + pX ];
			if( mMemory[ 0xBD03 + pX ] >= 0x84 )
				if( mMemory[ 0xBD03 + pX ] < 0x87 )
					goto s3D4C;

			mMemory[ 0xBD03 + pX ] = 0x84;

			goto s3D4C;
		}
	} else {
		// 3CFB
		A = mMemory[ 0xBD1D + pX ];
		if(A & 0x80)
			goto s3D4F;

		mMemory[ 0xBD01 + pX ] -= byte_5FD7;
		++mMemory[ 0xBD01 + pX ];
		if( !(mMemory[ word_3C ] & 1) ) {
			// 3d15
			mMemory[ 0xBD03 + pX ] = 0x8A;
			mMemory[ 0xBD02 + pX ] += 2;
			goto s3D4C;
		} else {
			// 3d26
			if( !(mMemory[ 0xBD1D + pX ]) )
				mMemory[ 0xBD02 + pX ] -= 2;
			else
				mMemory[ 0xBD02 + pX ] += 2;

			// 3d40
			mMemory[ 0xBD03 + pX ] = ((mMemory[ 0xBD02 + pX ] & 0x06) >> 1) + 0x8B;
		}
	}
s3D4C:;
	// 3D4C
	hw_SpritePrepare( pX );

s3D4F:;
	mMemory[ word_40 + 6 ] = mMemory[ 0xBD1D + pX ];
	mMemory[ word_40 + 3 ] = mMemory[ 0xBD01 + pX ];
	mMemory[ word_40 + 4 ] = mMemory[ 0xBD02 + pX ];
	mMemory[ word_40 + 5 ] = mMemory[ 0xBD03 + pX ];
}

// 3D6E: Frankie?
void cCreep::obj_Frankie_Collision( byte pX, byte pY ) {
	char A = mMemory[ 0xBD01 + pX ] + mMemory[ 0xBD0C + pX ];
	A -= mMemory[ 0xBF01 + pY ];
	if( A >= 4 ) {
		byte_31F5 = 0;
		return;
	}
	
	// 3d85
	if( mMemory[ 0xBF00 + pY ] != 0x0B ) {
		byte_31F5 = 0;
		if( mMemory[ 0xBF00 + pY ] != 0x0C )
			return;
		mMemory[ 0xBD1B + pX ] = mMemory[ 0xBE00 + pY ];
		return;

	} else {
		// 3DA1
		word_40 = word_5387 + mMemory[ 0xBE00 + pY ];

		if( !(mMemory[ word_40 ] & byte_538A) ) {
			byte_31F5 = 0;
			return;
		}
		
		word_40 = word_5748 + mMemory[ 0xBD1F + pX ];

		A = (byte_574E ^ 0xFF) & mMemory[ word_40 ];
		A |= byte_574D;
		mMemory[ word_40 ] = A;
		mMemory[ 0xBD1E + pX ] = A;
	}
}

// 3DDE: Franky Hit 
void cCreep::obj_Frankie_Hit(byte pX, byte pY) {
	if( mMemory[ 0xBD1E + pX ] & byte_574E ) {
		byte A = mMemory[ 0xBD00 + pY ];

		if( A && A != 2 && A != 3 ) {
			// 3DF3
			if( A != 5 ) {
			
				word_40 = word_5748 + mMemory[ 0xBD1F + pX ];
				mMemory[ word_40 ] = ((byte_574E ^ 0xFF) & mMemory[ word_40 ]) | byte_574D;
				return;

			} else {
				// 3E18
				A = mMemory[ 0xBD03 + pX ];
				if( A >= 0x8A && A < 0x8F ) {
					// 3E23
					A = mMemory[ 0xBD03 + pY ];
					if( A < 0x8A || A >= 0x8F ) {
						byte_311D = 0;
						return;
					}
					// 3E2E
					if( mMemory[ 0xBD02 + pX ] == mMemory[ 0xBD02 + pY ] ) {
						byte_311D = 0;
						return;
					}
					if( mMemory[ 0xBD02 + pX ] < mMemory[ 0xBD02 + pY ] ) {
						mMemory[ 0xBD1C + pX ] &= 0xEF;
						byte_311D = 0;
						return;
					}
					// 3E43
					mMemory[ 0xBD1C + pX ] &= 0xFE;
					byte_311D = 0;
					return;
				}
				// 3E4E
				byte A =  mMemory[ 0xBD03 + pX ];
				byte B = mMemory[ 0xBD03 + pY ];

				// 3E51
				if( A < 0x84 || A >= 0x8A || B < 0x84 || B >= 0x8A ) {
					byte_311D = 0;
					return;
				}
				if( mMemory[ 0xBD01 + pX ] < mMemory[ 0xBD01 + pY ] ) {
					mMemory[ 0xBD1C + pX ] &= 0xFB;
					byte_311D = 0;
					return;
				}
				// 3E77
				mMemory[ 0xBD1C + pX ] &= 0xBF;
			}
		} 	
	}

	// 3E4B
	byte_311D = 0;
	return;
}			

// 3E87: 
void cCreep::obj_Frankie_Add() {
	if( mMemory[ word_3E ] & byte_574D )
		return;

	byte X;
	
	obj_FindFree(X);
	mMemory[ 0xBD00 + X ] = 5;
	mMemory[ 0xBD1F + X ] = byte_574A;
	mMemory[ 0xBD1E + X ] = mMemory[ word_3E ];
	if( !(mMemory[ word_3E ] & byte_574E) ) {
		mMemory[ 0xBD01 + X ] = mMemory[ word_3E + 1 ];
		mMemory[ 0xBD02 + X ] = mMemory[ word_3E + 2 ] + 7;
		mMemory[ 0xBD03 + X ] = 0x8F;
	} else {
		// 3EC8
		mMemory[ 0xBD01 + X ] = mMemory[ word_3E + 3 ];
		mMemory[ 0xBD02 + X ] = mMemory[ word_3E + 4 ];
		mMemory[ 0xBD03 + X ] = mMemory[ word_3E + 5 ];
		mMemory[ 0xBD1D + X ] = mMemory[ word_3E + 6 ];
	}
	// 3ee4
	mMemory[ 0xBD0C + X ] = 3;
	mMemory[ 0xBD0D + X ] = 0x11;
	hw_SpritePrepare(X);
	mMemory[ 0xBD1C + X ] = 0xFF;
	mMemory[ 0xBD1A + X ] = 0xFF;
	mMemory[ 0xBD1B + X ] = 0xFF;
	mMemory[ 0xBD06 + X ] = 2;
	mMemory[ 0xBD05 + X ] = 2;
}

void cCreep::img_Actions() {
	byte X;

	byte_3FD4 = 0;
	
	for(;;) {
		if( byte_3FD4 >= mImageCount ) 
			break;
	
		X = byte_3FD4 << 3;

		byte A = mMemory[ 0xBF04 + X ];
		if(A & byte_840) {
			byte Y = mMemory[ 0xBF00 + X ] << 2;
		
			word func = readWord( &mMemory[ 0x842 + Y ] );
		
			switch( func ) {
				case 0:
					mMemory[ 0xBF04 + X ] ^= byte_840;
					break;
				case 0x3FD5:
					obj_Door_Img_Execute( X );
					break;
				case 0x42AD:
					obj_Lightning_Img_Execute( X );
					break;
				case 0x45E0:	
					obj_Forcefield_Img_Timer_Execute( X );
					break;
				case 0x475E:
					obj_Mummy_Img_Execute( X );
					break;
				case 0x4B1A:
					obj_RayGun_Img_Execute( X );
					break;
				case 0x4E32:
					obj_Teleport_Img_Execute( X );
					break;
				case 0x50D2:
					obj_TrapDoor_Switch_Img_Execute( X );
					break;
				case 0x538B:
					obj_Conveyor_Img_Execute( X );
					break;
				default:
					cout << "img_Actions: 0x";
					cout << std::hex << func << "\n";
					break;
			}

			// 3F93
			A = mMemory[ 0xBF04 + X ];
		}
	
		// 3F96
		// Key picked up
		if( A & byte_841 ) {
			sub_57DF( X );
			// Decrease image count
			--mImageCount;
			A = mImageCount << 3;

			// Last image?
			if( X == A )
				break;

			byte Y = A;
			byte byte_3FD3 = 8;

			// Copy the last image entry, into the keys position
			for(;;) {
				mMemory[ 0xBF00 + X ] = mMemory[ 0xBF00 + Y ];
				mMemory[ 0xBE00 + X ] = mMemory[ 0xBE00 + Y ];
				++X;
				++Y;
				--byte_3FD3;
				if( !byte_3FD3 )
					break;
			}
		}

		// 3FC7
		++byte_3FD4;
	}

	//
	
}

// 3639: 
void cCreep::obj_Lightning_Execute( byte pX ) {
	byte A = mMemory[ 0xBD04 + pX ];
	if( A & byte_885 ) {
		mMemory[ 0xBD04 + pX ] = (A ^ byte_885) | byte_886;
		return;
	}

	if( A & byte_882 ) {
		A ^= byte_882;
		mMemory[ 0xBD04 + pX ] = A;
	}

	A = seedGet();
	A &= 0x03;
	mMemory[ 0xBD06 + pX ] = A;
	++mMemory[ 0xBD06 + pX ];

	A = seedGet();
	A &= 3;

	A += 0x39;
	if( A == mMemory[ 0xBD03 + pX ] ) {
		A += 0x01;
		if( A >= 0x3D )
			A = 0x39;
	}
	// 3679
	mMemory[ 0xBD03 + pX ] = A;
	hw_SpritePrepare( pX );
}

void cCreep::obj_Lightning_Add( byte &pY  ) {
	byte X;

	obj_FindFree( X );
	mMemory[ 0xBD00 + X ] = 1;
	
	mMemory[ 0xBD02 + X ] = mMemory[ 0xBF02 + pY ] + 8;
	mMemory[ 0xBD01 + X ] = mMemory[ 0xBF01 + pY ];
	mMemory[ 0xBD1F + X ] = mMemory[ 0xBE00 + pY ];

}

// 36B3: Forcefield
void cCreep::obj_Forcefield_Execute( byte pX ) {
	byte A = mMemory[ 0xBD04 + pX ];
	byte Y;

	if(A & byte_885 ) {
		A ^= byte_885;

		A |= byte_886;
		mMemory[ 0xBD04 + pX ] = A;
		return;
	}

	if(A & byte_882 ) 
		mMemory[ 0xBD04 + pX ] = (A ^ byte_882);

	Y = mMemory[ 0xBD1F + pX ];

	if( mMemory[ 0x4750 + Y ] == 1 ) {

		if( mMemory[ 0xBD1E + pX ] != 1 ) {
			mMemory[ 0xBD1E + pX ] = 1;
			sub_5F6B( pX );
			
			word_3C -= 2;
			mMemory[ word_3C + 0 ] = mMemory[ word_3C + 0 ] & 0xFB;
			mMemory[ word_3C + 4 ] = mMemory[ word_3C + 4 ] & 0xBF;

			A = 0x35;
		} else {
			// 3709
			if( mMemory[ 0xBD03 + pX ] == 0x35 )
				A = 0x3D;
			else
				A = 0x35;
		}

	} else {
	// 371A
		if( mMemory[ 0xBD1E + pX ] != 1 )
			return;

		mMemory[ 0xBD1E + pX ] = 0;
		sub_5F6B( pX );

		word_3C -= 2;
		mMemory[ word_3C + 0 ] |= 4;
		mMemory[ word_3C + 4 ] |= 0x40;
		A = 0x41;
	}

	// 3746
	mMemory[ 0xBD03 + pX ] = A;

	// Draw the forcefield
	hw_SpritePrepare( pX );
}

// 5FD9
word cCreep::lvlPtrCalculate( byte pCount ) {
	word_42 = pCount;
	
	word_42 <<= 1;
	word_42 <<= 1;
	word_42 <<= 1;
	word_42 += 0x7900;
	if( mIntro )
		word_42 += 0x2000;

	return word_42;
}

// 0D71: 
void cCreep::Game() {

	mMenuReturn = false;
	mPlayer1Seconds = 0;
	mPlayer2Seconds = 0;

	if( byte_24FD == 1 ) {
		// D7D
		byte_24FD = 0;
		mMemory[ 0x7802 ] |= 1;
	} else {
		// D8D
		word_30 = 0x9800;
		word_32 = 0x7800;

		word_34 = readWord( &mMemory[ 0x9800 ] );

		// 
		memcpy( &mMemory[ word_32 ], &mMemory[ word_30 ], word_34 );

		// DC6
		// Which joystick was selected when button press was detected (in intro)
		mMemory[ 0x7812 ] = byte_D10;

		// Clear Stored CIA Timers
		for( char Y = 7; Y >= 0; --Y )
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

				mMemory[ 0x11C9 + X ] = 0;
				X ^= 0x01;
				mMemory[ 0x11C9 + X ] = 1;

			} else {
				mMemory[ 0x11C9 ] = mMemory[ 0x11CA ] = 1;
			}

		} else {
			// E5F
			if( mMemory[ 0x780F ] != 1 ) {
				mMemory[ 0x11CA ] = 1;
				mMemory[ 0x11C9 ] = 0;
			} else {
				// E73
				mMemory[ 0x11C9 ] = 1;
				mMemory[ 0x11CA ] = 0;
			}
		}

		// E7D
		while( mapDisplay() );

		if(mMenuReturn)
			return;

		roomMain();
		screenClear();
		
		mMemory[ 0xF62 ] = 0;

		// E8D
		for( byte X = 0; X < 2; ++X ) {

			if( mMemory[ 0x11C9 + X ] == 1 ) {
				if( mMemory[ 0x780D + X ] != 2 ) {
					if( mMemory[ 0x785D + X ] != 1 )
						continue;

					mMemory[ 0x1AB2 ] = X;
					gameEscapeCastle();

					if(! (mMemory[ 0x7802 ] & 1 )) {
						
						if( mUnlimitedLives != 0xFF ) {
							
							byte A = X << 2;
							
							word_30 = 0x7855 + A;
							for( char Y = 3; Y >= 0; --Y )
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

			word_3E = 0x0F64;		// Game Over
			obj_stringPrint();

			if( mMemory[ 0x7812 ] != 0 ) {
				if( mMemory[ 0x780F ] != 1 ) {
					word_3E = 0x0F72;	// For Player
					obj_stringPrint();
				}
				// F39
				if( mMemory[ 0x7810 ] != 1 ) {
					word_3E = 0x0F83;	// For Player
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

// F94: Display the map/time taken screen
bool cCreep::mapDisplay() {
	byte gfxPosX, gfxPosY;

	screenClear();

	mMemory[ 0xD028 ] = mMemory[ 0xD027 ] = 0;
	mMemory[ 0x11D7 ] = 0;

	// Draw both players Name/Time/Arrows
	// FA9
	for(;;) {

		byte X = mMemory[ 0x11D7 ];

		if( mMemory[ 0x11C9 + X ] == 1 ) {
			// FB6

			byte A = mMemory[ 0x7809 + X ];
			lvlPtrCalculate( A );
			
			mMemory[ word_42 ] |= byte_8C0;
			
			sub_6009( mMemory[ 0x780B + X ] );
			
			mMemory[ 0x11D9 ] = mMemory[ word_40 + 2 ] & 3;
			
			obj_FindFree( X );
			mMemory[ 0x11D8 ] = X >> 5;
			
			word posX;
			
			A = mMemory[ word_42 + 1 ];	// X
			A += mMemory[ word_40 + 5 ];

			byte Y = mMemory[ 0x11D9 ];
			A += mMemory[ 0x11DA + Y ];
			posX = A;

			bool cf = false;

			if( (A - 4) < 0 )
				cf = true;
			A -= 4;
			A <<= 1;

			posX -= 4;
			posX <<= 1;
			
			Y = mMemory[ 0x11D8 ];
			cSprite *sprite = mScreen->spriteGet( Y );

			// Sprite X
			mMemory[ 0x10 + Y ] = A;
			sprite->_X = posX ;

			if( cf ) {
				A = mMemory[ 0x2F82 + Y ] | mMemory[ 0x20 ];
			} else
				A = (mMemory[ 0x2F82 + Y ] ^ 0xFF) & mMemory[ 0x20 ];

			// Sprite X 8bit
			mMemory[ 0x20 ] = A;

			// 100D
			A = mMemory[ word_42 + 2 ];		// Y
			A += mMemory[ word_40 + 6 ];
			A += mMemory[ 0x11DE + mMemory[ 0x11D9 ] ];
			A += 0x32;

			// Sprite Y
			sprite->_Y = A;
			mMemory[ 0x18 + mMemory[ 0x11D8 ] ] = A;
			mMemory[ 0xBD03 + X ] = mMemory[ 0x11E2 + mMemory[0x11D9] ];
			
			// Enable the Arrow sprite
			hw_SpritePrepare( X );
			
			// Sprites Enabled
			mMemory[ 0x21 ] = (mMemory[ 0x2F82 + mMemory[ 0x11D8 ] ] | mMemory[ 0x21 ]);
			sprite->_rEnabled = true;

			// 103C
			Y = mMemory[ 0x11D7 ];
			X = mMemory[ 0x7807 + Y ];
			A = mMemory[ 0x11E5 + X ];

			
			mMemory[ 0x11EF ] = mMemory[ 0x11FA ] = A;
			X = Y << 1;
			
			// Player (One Up / Two Up)
			word_3E = mMemory[ 0x11E9 + X ];
			word_3E += (mMemory[ 0x11EA + X ] << 8);

			obj_stringPrint();

			// 1058
			word_3E = (Y << 2);
			word_3E += 0x7855;

			convertTimerToTime();

			gfxPosX = mMemory[ 0x11D3 + Y ] + 8;
			gfxPosY = 0x10;

			// Draw Time ' : : '
			screenDraw(0, 0x93, gfxPosX, gfxPosY, 0);
		} 
		
		// 1087
		++mMemory[ 0x11D7 ];
		if( mMemory[ 0x11D7 ] == 2 )
			break;
	}
	
	// 1094
	mapRoomDraw();

	mMemory[ 0x11CB ] = 0;
	mMemory[ 0x11CD ] = mMemory[ 0x11CC ] = 1;
	if( mMemory[ 0x11C9 ] != 1 ) {
		if( mMemory[ 0x11CA ] == 1 )
			mMemory[ 0x11CD ] = 0;

	} else {
		// 10BA
		mMemory[ 0x11CC ] = 0;
		if( mMemory[ 0x11CA ] == 1 ) {
			mMemory[ 0x11CD ] = 0;
			if( mMemory[ 0x780B ] == mMemory[ 0x780C ] ) {
				mMemory[ 0x11CB ] = 1;
				mMemory[ 0xD028 ] = mMemory[ 0xD027 ] = 1;
				goto s10EB;
			}
		}
	}

	// 10E3
	mMemory[ 0xD027 ] = mMemory[ 0xD028 ] = 0;

s10EB:;
	mMemory[ 0x11D0 ] = 1;
	byte_B83 = 0;
	mMemory[ 0x11CE ] = mMemory[ 0x11D1 ];
	mMemory[ 0x11CF ] = mMemory[ 0x11D2 ];
	mMemory[ 0x11D7 ] = 0;
	

	// 110E
	for( ;; ) {
		mInterruptCounter = 1;

		byte X = mMemory[ 0x11D7 ];

		// Player Arrow Flash
		if( mMemory[ 0x11CB ] != 1 ) {
			--mMemory[ 0x11CE + X ];
			if(mMemory[ 0x11CE + X ] == 0 ) {
				// 1122
				mMemory[ 0x11CE + X ] = mMemory[ 0x11D1 + X ];
				if( X == 0 || mMemory[ 0x11C9 ] != 1 ) {
					// 1133
					mMemory[ 0xD027 ] ^= 0x01;
					mScreen->spriteGet( 0 )->_color ^= 0x01;
				} else {
					// 113E
					mMemory[ 0xD028 ] ^= 0x01;
					mScreen->spriteGet( 1 )->_color ^= 0x01;
				}
				
				mScreen->spriteRedrawSet();
			}
		}

		// 115A
		KeyboardJoystickMonitor( X );

		// 1146
		if( byte_B83 == 1 ) {
			byte_B83 = 0;
			mMemory[ 0x11D0 ] = 0;
			mMenuReturn = true;
			return false;
		}

		// Check if run/stop was pressed
		if( mRunStopPressed == 1 ) {

			// Save the game
			gamePositionSave();

			return true;
		}

		if( mInput->f2Get() ) {

			// Load a game
			gamePositionLoad();

			return true;
		}

		// 117D
		// Button Press Check
		if( byte_5F57 )
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

	for(;;) {
		hw_Update();

		KeyboardJoystickMonitor(0);
		if( byte_5F57 )
			continue;

		KeyboardJoystickMonitor(1);
		if( byte_5F57 )
			continue;
		
		break;
	}

	sub_21C8( 9 );
	mMemory[ 0x11D0 ] = 0;
	return false;
}

// 34EF
void cCreep::obj_Player_Collision( byte pX, byte pY ) {
	byte A;
	if( mMemory[ 0xBF00 + pY ] == 0x0B ) {
		
		A = mMemory[ 0xBD01 + pX ] + mMemory[ 0xBD0C + pX ];
		A -= mMemory[ 0xBF01 + pY ];

		if( A < 4 ) {
			mMemory[ 0x780D + mMemory[ 0xBD1C + pX ] ] = 2;
			return;
		}

	} 
	// 3505
	byte_31F5 = 0;
	if( mMemory[ 0xBF00 + pY ] != 0x0C ) 
		return;

	A = mMemory[ 0xBD01 + pX ] + mMemory[ 0xBD0C + pX ];
	A -= mMemory[ 0xBF01 + pY ];

	if( A >= 4 )
		return;

	mMemory[ 0xBD1A + pX ] = mMemory[ 0xBE00 + pY ];
	return;
}

// 3534: Hit Player
void cCreep::obj_Player_Hit( byte pX, byte pY ) {
	byte A = mMemory[ 0xBD00 + pY ];

	if( A == 2 ) {
		byte_311D = 0;
		return;
	}

	if( A != 0 ) {
		// 358C
		if( mMemory[ 0x780D + mMemory[ 0xBD1C + pX ] ] != 0 ) {
			byte_311D = 0;
			return;
		}

		mMemory[ 0x780D + mMemory[ 0xBD1C + pX ] ] = 2;
		return;
	}
	// 353F

	A = mMemory[ 0xBD03 + pY ];

	if( A == 0x2E || A == 0x2F || A == 0x30 || A == 0x31 || A == 0x26 ) {
		byte_311D = 0;
		return;
	} 
	
	if( mMemory[ 0xBD02 + pY ] == mMemory[ 0xBD02 + pX ] ) {
		byte_311D = 0;
		return;
	}
	if( mMemory[ 0xBD02 + pY ] >= mMemory[ 0xBD02 + pX ] ) {
		mMemory[ 0xBD18 + pX ] = 0xEF;
		byte_311D = 0;
		return;
	}

	mMemory[ 0xBD18 + pX ] = 0xFE;
	byte_311D = 0;
	return;
}

// 359E
void cCreep::obj_Player_Add( ) {
	byte X;
	obj_FindFree( X );
	
	byte Y = byte_3638;
	mMemory[ 0x34D1 + Y ] = X;		// Set player object number
	
	byte A = mMemory[ 0x780B + Y ];
	A <<= 3;
	word_40 = A + word_41D3;
	
	// 35C0
	if( mMemory[ word_40 + 2 ] & 0x80 ) {
		mMemory[ 0x780D + byte_3638 ] = 6;
		mMemory[ 0xBD01 + X ] = mMemory[ word_40 ] + 0x0B;
		mMemory[ 0xBD02 + X ] = mMemory[ word_40 + 1 ] + 0x0C;
		mMemory[ 0xBD1B + X ] = 0x18;
		mMemory[ 0xBD06 + X ] = 0x03;

	} else {
		// 35F1
		mMemory[ 0x780D + byte_3638 ] = 0;
		mMemory[ 0xBD01 + X ] = mMemory[ word_40 ] + 6;
		mMemory[ 0xBD02 + X ] = mMemory[ word_40 + 1 ] + 0x0F;
	}

	// 360D
	mMemory[ 0xBD0C + X ] = 3;
	mMemory[ 0xBD0D + X ] = 0x11;
	mMemory[ 0xBD1F + X ] = 0x80;
	mMemory[ 0xBD1C + X ] = byte_3638;
	mMemory[ 0xBD03 + X ] = 0;
	mMemory[ 0xBD19 + X ] = mMemory[ 0xBD1A + X ] = mMemory[ 0xBD18 + X ] = 0xFF;
}

void cCreep::roomMain() {

	roomLoad();

	byte X = 0;

	for(;;) {
		
		if( mMemory[ 0x11C9 + X ] == 1 ) {
			byte_3638 = X;
			obj_Player_Add();
		}
		++X;
		if(X >= 2)
			break;
	}

	//14EA
	mMemory[ 0x15D7 ] = 1;
	byte_B83 = 0;

	for(;;) {

		events_Execute();
		hw_Update();

		// Do pause?
		if( mRunStopPressed == 1 ) {
			//150E
			for(;;) {
				interruptWait( 3 );

				KeyboardJoystickMonitor( 0 );						
			
				if( mRunStopPressed == 1 )
					break;
			}

		}
		// 156B
		if( byte_B83 == 1 ) {
			byte_B83 = 0;
			char X = 1;
			for(;;) {
				if( mMemory[ 0x780D + X ] == 0 ) {
					mMemory[ 0x780D + X ] = 2;
					byte Y = mMemory[ 0x34D1 + X ];
					mMemory[ 0xBD04 + Y ] |= byte_883;
				}
				--X;
				if(X < 0)
					break;
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
	if( mMemory[ 0x780D + X ] == 5 )
		continue;
	if( mMemory[ 0x780D + X ] == 6 )
		continue;

	++X;
	if( X < 2 )
		goto s15B4;

		break;
	}

	// 15C4
	mMemory[ 0x15D7] = 0;

	for( byte X = 0x1E; X; --X ) {
		events_Execute();
		hw_Update();
	}
}

void cCreep::sub_6009( byte pA ) {
	byte_603A = pA;

	word_40 = readWord( &mMemory[ word_42 + 4] );
	
	byte A2 = mMemory[ word_40 ];
	
	word_40 += (byte_603A << 3) + 1;

	byte_603A = A2;	
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

	for( char Y = 5; Y >= 0; --Y)
		mMemory[ word_30 + Y] = mTextColor << 4;
	
	//2AFE
	for(;;) {
		byte X = mTextFont & 0x30;
		X >>= 3;

		word_30 = (mMemory[ word_3E ] & 0x7F) << 3;
		word_30 += mMemory[ 0x2BE8 + X ];
		word_30 += (mMemory[ 0x2BE9 + X ] << 8);

		// Copy from Char ROM
		for( char count = 7; count >= 0; --count )
			mMemory[ 0x2BF0 + count ] = charRom( word_30 + count);

		word_30 = 0x73EA;
		X = 0;

		// 2B50
		for(;;) {
			byte Y = mMemory[ 0x2BF0 + X ];
			Y >>= 4;
			Y &= 0x0F;

			word tmp =  mMemory[ 0x2BF8 + Y ];
			Y = mMemory[ 0x2BF0 + X ] & 0xF;
			tmp += (mMemory[ 0x2BF8 + Y] << 8);
			
			*(word*)(&mMemory[ word_30 ]) = tmp;

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
			++X;
			if( X >= 8 )
				break;

		}

		screenDraw( 2, 0x95, gfxPosX, gfxPosY, 0x94 );

		if( ((char) mMemory[ word_3E ]) < 0 )
			break;

		++word_3E;
		gfxPosX += 8;
		mTxtX_0 = gfxPosX;
	}

	// 2BD7
	++word_3E;
}

//2A6D
void cCreep::obj_stringPrint( ) {

	while( (mTextXPos = mMemory[ word_3E ]) ) {
		mTextYPos = mMemory[ word_3E + 1 ];
		mTextColor = mMemory[ word_3E + 2 ];
		mTextFont = mMemory[ word_3E + 3 ];

		word_3E += 0x04;

		stringDraw( );
	}

	++word_3E;
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
		
		word_32 = readWord( &mMemory[ byte_38 ] );
		
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
			char a = 0 - videoPtr0;
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

		if(!A)
			A = pGfxPosY;
		
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
						A ^= 0xFF;
						A &= mMemory[ word_36 + Y];
						mMemory[ word_36 + Y] = A;
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
					if( gfxCurPos < 0x28 ) {
						byte A = mMemory[ word_32 + Y ];
						*memory( byte_36 + Y ) |= A;
					}

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

	if( pGfxPosY & 0x7)
		mMemory[0x5CE5] = 1;
	else
		mMemory[0x5CE5] = 0;

	// 5BA4
	gfxCurrentPosY = gfxDestY;
	word_30 = videoPtr0;
	word_30 += (0xCC + videoPtr1) << 8;

	// 5BBC
	for( ;; ) {
		if( gfxCurrentPosY < 0x19 ) {
			gfxCurPos = gfxDestX2;
		
			byte Y = 0;

			for( ;; ) {
			
				if( gfxCurPos < 0x28 )
					mMemory[ word_30 + Y ] = mMemory[ word_32 + Y ];
			
				++Y;
				if( gfxCurPos == gfxPosRightX )
					break;

				++gfxCurPos;
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
			gfxCurPos = gfxDestX2;
			
			byte Y = 0;

			for(;;) {
				if( gfxCurPos < 0x28 ) 
					mMemory[ word_30 + Y ] = mMemory[ word_32 + Y ];
				
				++Y;
				if( gfxCurPos == gfxPosRightX )
					break;

				++gfxCurPos;
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
	byte byte_13EA, byte_13EB, byte_13EC;
	byte byte_13ED, byte_13EE, byte_13EF;

	word_42 = 0x7900;
	
	byte gfxPosX;
	byte gfxPosY;

	//1210
	for(;;) {
		
		if( mMemory[ word_42 ] & byte_8C1 )
			return;
		
		if( mMemory[ word_42 ] & byte_8C0 ) {
			//1224
			
			mMemory[ 0x63E7 ] = mMemory[ word_42 ] & 0xF;
			byte_13EC = mMemory[ word_42 + 1 ];	// top left x
			byte_13ED = mMemory[ word_42 + 2 ]; // top left y
			byte_13EF = mMemory[ word_42 + 3 ] & 7; // width
			byte_13EE = (mMemory[ word_42 + 3 ] >> 3) & 7; // height

			gfxPosY = byte_13ED;

			byte_13EB = byte_13EF;
			
			// Draw Room Floor Square
			// 1260
			for(;;) {
				byte_13EA = byte_13EE;
				gfxPosX = byte_13EC;
				
				for(;;) {
					screenDraw( 0, 0x0A, gfxPosX, gfxPosY, 0 );
					gfxPosX += 0x04;
					--byte_13EA;
					if(!byte_13EA)
						break;
				}
				gfxPosY += 0x08;
				--byte_13EB;
				if(!byte_13EB)
					break;
			}

			// 128B
			// Top edge of room
			mTxtX_0 = byte_13EC;
			mTxtY_0 = byte_13ED;
			byte_13EA = byte_13EE;

			for(;;) {
				screenDraw(1, 0, 0, 0, 0x0B );
				mTxtX_0 += 0x04;
				--byte_13EA;
				if(!byte_13EA)
					break;
			}

			// 12B8
			// Bottom edge of room
			mTxtX_0 = byte_13EC;
			mTxtY_0 = ((byte_13EF << 3) + byte_13ED) - 3;
			byte_13EA = byte_13EE;

			for(;;) {
				screenDraw(1, 0, 0, 0, 0x0B );
				mTxtX_0 += 0x04;
				--byte_13EA;
				if(!byte_13EA)
					break;
			}

			//12E5
			// Draw Left Edge
			mTxtX_0 = byte_13EC;
			mTxtY_0 = byte_13ED;
			byte_13EA = byte_13EF;

			for(;;) {
				screenDraw(1, 0, 0, 0, 0x0C );
				mTxtY_0 += 0x08;
				--byte_13EA;
				if(!byte_13EA)
					break;
			}

			//130D
			// Draw Right Edge
			mTxtX_0 = ((byte_13EE << 2) + byte_13EC) - 4;
			mTxtY_0 = byte_13ED;
			byte_13EA = byte_13EF;

			for(;;) {
				screenDraw(1, 0, 0, 0, 0x0D );
				mTxtY_0 += 0x08;
				--byte_13EA;
				if(!byte_13EA)
					break;
			}

			// 133E
			sub_6009( 0 );
			byte_13EA = byte_603A;
s1349:;
			if( byte_13EA )
				break;
		} 

		//134E
		word_42 += 0x08;
	}

	//135C
	byte A = mMemory[ word_40 + 2 ];
	A &= 3;

	if( !( A & 3) ) {
		mTxtY_0 = byte_13ED;
	} else {
		// 136D
		if( A == 2 ) {
			// 136F
			mTxtY_0 = (byte_13EF << 3) + byte_13ED;
			mTxtY_0 -= 3;

		} else {
			// 13A0
			mTxtY_0 = byte_13ED + mMemory[ word_40 + 6 ];
			
			if( A != 3 ) {
				mTxtX_0 = ((byte_13EE << 2) + byte_13EC) - 4;
				A = 0x11;
			} else {
				// 13C5
				mTxtX_0 = byte_13EC;
				A = 0x10;
			}
			
			goto s13CD;
		}
	}

	// 1381
	mTxtX_0 = A = byte_13EC + mMemory[ word_40 + 5 ];

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
	*((word*) &mMemory[ 0x6067 ]) = word_3E;
	
	byte gfxDecodeMode = 0, gfxCurrentID = 0x16;

	word_32 = (mMemory[ word_3E + 1 ] - 1) >> 3;
	++word_32;

	byte X =  mMemory[ word_3E ];

	word_30 = 0;
	
	//1B18
	for(;;) {
		if( X == 0 )
			break;

		word_30 += word_32;
		X--;
	}
	// 1B2D
	word_30 <<= 1;
	X = mMemory[ word_3E + 1 ];

	for(;;) {
		if(X == 0 )
			break;

		word_30 += mMemory[ word_3E ];
		--X;
	}
	// 1B4D
	word_30 += 3;

	word_3E += word_30;

	//1B67
	for(;;) {
		byte A = mMemory[ word_3E ];
		if( !A ) {
			
			++word_3E;
			break;

		} else {
			// 1B7D
			screenDraw( gfxDecodeMode, gfxCurrentID, A, mMemory[ word_3E + 1 ], 0 );
			
			word_3E += 2;
		}
	}
}

// 160A: Draw multiples of an object
void cCreep::obj_MultiDraw() {
	byte gfxCurrentID, gfxPosX, gfxPosY;
	char gfxRepeat;

	while( (gfxRepeat = mMemory[ word_3E ]) != 0 ) {

		gfxCurrentID = mMemory[ word_3E + 1 ];
		gfxPosX = mMemory[ word_3E + 2 ];
		gfxPosY = mMemory[ word_3E + 3 ];

		--gfxRepeat;

		for( ; gfxRepeat >= 0; --gfxRepeat ) {
		
			screenDraw( 0, gfxCurrentID, gfxPosX, gfxPosY );
		
			gfxPosX += mMemory[ word_3E + 4 ];
			gfxPosY += mMemory[ word_3E + 5 ];
		}

		word_3E += 0x06;
	}
	
	++word_3E;
}

void cCreep::hw_Update() {
	mScreen->refresh();
	mInput->inputCheck();
}

// 1935: Sleep for X amount of interrupts
void cCreep::hw_IntSleep( byte pA ) {
	byte byte_194F = pA;

	for( byte X = 6; X > 0; --X ) {
		interruptWait( byte_194F );
	}

}

// 1950: Player Escapes from the Castle
void cCreep::gameEscapeCastle() {
	
	screenClear();
	mMemory[ 0x0B72 ] = 6;
	if( mMemory[ 0x7802 ] & 0x80 ) {
		word_3E = readWord( &mMemory[ 0x785F ] );
		roomPrepare();
	}

	byte A = mMemory[ 0x1AB2 ];
	A += 0x31;

	// Set player number in string 'Player Escapes'
	mMemory[ 0x1ABE ] = A;

	word_3E = 0x1AB3;
	obj_stringPrint();
	word_3E = 0x7855 + (mMemory[ 0x1AB2 ] << 2);
	convertTimerToTime();

	screenDraw(0, 0x93, 0x68, 0x18, 0 );
	// 19AF
	
	byte Y = mMemory[ 0x1AB2 ];
	byte X = mMemory[ 0x34D1 + Y ];
	mMemory[ 0xBD02 + X ] = 0x87;
	mMemory[ 0xBD01 + X ] = 0x08;

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
				++mMemory[ 0xBD03 + X];
				A = mMemory[ 0xBD03 + X ];
				if( A >= 0x9B || A < 0x97 ) {
					A = 0x97;
				}
			} else {
				// 1A33
				--mMemory[ 0xBD01 + X];
				++mMemory[ 0xBD03 + X];
				A = mMemory[ 0xBD03 + X ];
				if( A >= 3 )
					A = 0;
			}

		} else {
			// 1A1D
			++mMemory[ 0xBD01 + X];
			++mMemory[ 0xBD03 + X];
			A = mMemory[ 0xBD03 + X ];
			if( A >= 6 || A < 3 )
				A = 0x03;
		}

		// 1A42
		mMemory[ 0xBD03 + X ] = A;
		Y = X >> 5;

		cSprite *sprite = mScreen->spriteGet( Y );

		// 1A4B
		mMemory[ 0x10 + Y ] = ((mMemory[ 0xBD01 + X ] - 0x10) << 1) + 0x18;
		sprite->_X = ((mMemory[ 0xBD01 + X ] - 0x10) << 1) + 0x18;
		
		// 1A72
		sprite->_Y = mMemory[ 0xBD02 + X ] + 0x32;

		hw_SpritePrepare( X );
		sprite->_rEnabled = true;

		if( mMemory[ 0x1AB2 ] )
			A = mMemory[ 0x34D4 ];
		else
			A = mMemory[ 0x34D3 ];

		mMemory[ 0xD027 + Y ] = A;
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
	mMemory[ 0x1CFE ] = 0x0A;
	
	for(;;) {

		for(byte Y = 3;Y != 0; --Y) {
			// 1BC5
			byte A = mMemory[ word_30 + Y ];
			if( A < mMemory[ 0x1CF9 + Y ] ) {
				// 1BD1
				word_30 += 0x06;
				--mMemory[ 0x1CFE ];
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
	byte A = 0x0A - mMemory[ 0x1CFE ];
	A <<= 3;
	A += 0x38;
	mMemory[ 0x2789 ] = A;

	byte X = 0x0A - mMemory[ 0x1CFE ];
	
	mMemory[ 0x278A ] = mMemory[ 0x1E85 + X ];

	*((word*) (&mMemory[ 0x1D03 ])) = word_30 - 2;
	
	for(;;) {
		--mMemory[ 0x1CFE ];
		if( mMemory[ 0x1CFE ] == 0 )
			break;

		mMemory[ 0x1CFF ] = 6;

		for(;;) {
			mMemory[ 0xB806 + Y ] = mMemory[ 0xB800 + Y ];
			--Y;
			if( !Y )
				break;
		}
		//1C40
	}

	// 1C43
	for(Y = 3; Y != 0; --Y )
		mMemory[ word_30 + Y ] = mMemory[ 0x1CF9 + Y ];
	
	// 1C4D
	word_30 = readWord( &mMemory[ 0x1D03 ] );
	mMemory[ word_30 ] = 0;
	gameHighScores();

	// 1C60

	word_3E = 0x1D05;
	X = mMemory[ 0x1CFD ];

	A = mMemory[ 0x1D01 + X ];
	mMemory[ 0x1D10 ] = A;
	obj_stringPrint();

	mMemory[ 0x278C ] = 3;
	mMemory[ 0x278B ] = 1;

	textShow();
	word_30 = readWord( &mMemory[ 0x1D03 ] );

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

	for( char Y = 0x0E; Y >= 0; --Y ) {
		A = mMemory[ word_30 + Y ];

		if( (A & 0x7F) < 0x20 )
			A |= 0x40;

		mMemory[ 0x28D7 + Y ] = A;
	}

	// 1CD8
	mMemory[ 0x28D2 ] = mMemory[ 0xBA03 + X ];
	mMemory[ 0x28D1 ] = 2;
	
	// TODO
	//1CE3 20 20 29                    JSR     InterruptDisableAndReInit

	// Save highscores
	mCastleManager->scoresSave( mCastle->nameGet(), readWord( memory( 0xB800 ) ), memory( 0xB800 ) );

	sub_2973();
}

// 1D42: Display Highscores for this Castle
void cCreep::gameHighScores( ) {
	screenClear();

	mMemory[ 0x2399 ] = mCastle->nameGet().size() - 1;
	mMemory[ 0xBA01 + 0x10 ] = mCastle->nameGet().size() - 1;
	mMemory[ 0xBA10 ] = 0x03;

	byte X = mMemory[ 0x2399 ];
	byte Y = mMemory[ 0xBA01 + X ];
	byte A = mMemory[ 0x5CE6 + Y ];

	word_30 = A + mMemory[ 0xBA00 + X ] + ((mMemory[ 0x5D06 + Y] | 0x04) << 8);

	mMemory[ 0xBA03 + X ] = mCastle->nameGet().size() - 1;

	Y =  mCastle->nameGet().size() - 1;

	// 1D67
	//mMemory[ 0xB87A + Y ] = mMemory[ word_30 + Y ];

	// Convert name
	for( ; (char) Y >= 0; --Y )
		mMemory[ 0xB87A + Y ] = toupper(mCastle->nameGet()[Y]) & 0x3F;

	// Mark end of name
	mMemory[ 0xB87A + (mCastle->nameGet().size()-1) ] |= 0x80;


	word_3E = 0xB87A;
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
		mMemory[ 0xB889 ] = 0;
		mTextYPos = 0x38;
		
		for(;;) {
			
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

			word_3E = 0xB87A; 
			stringDraw();

			if( mMemory[ 0xB802 + X ] != 0xFF ) {
				// 1DF5
				word_3E = (X + 4) + 0xB800;
				convertTimerToTime();
				screenDraw( 0, 0x93, mTextXPos + 0x20, mTextYPos, 0x94 );
			}
				
			// 1E20
			mTextYPos += 0x08;
			X += 0x06;
			++mMemory[ 0xB889 ];
			if( mMemory[ 0xB889 ] >= 0x0A )
				break;
		}
		
		// 1E3B
		if( mTextXPos != 0x18 )
			break;

		mTextXPos = 0x68;
	}

	// Draw BEST TIMES
	// 1E4A
	word_3E = 0x1E5B;
	obj_stringPrint();
}

// 21C8
void cCreep::sub_21C8( char pA ) {
	char byte_2231 = pA;

	if( mIntro == 1 )
		return;

	if( byte_839 == 1 )
		return;

	if( byte_2232 < 0 )
		return;

	byte_2232 = byte_2231;

	byte Y = byte_2232 << 1;
	word_44 = readWord( &mMemory[ 0x7572 + Y ] );

	mSound->sidWrite( 0x04, 0 );
	mSound->sidWrite( 0x0B, 0 );
	mSound->sidWrite( 0x12, 0 );
	mSound->sidWrite( 0x17, 0 );

	mMusicBuffer = memory( readWord(memory( 0x7572 + Y )) );

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
	byte byte_1744, byte_1745, byte_1746;
	byte gfxCurrentID, gfxPosX, gfxPosY;

	for(;;) {
		
		byte_1746 = mMemory[ word_3E ];

		if( ! byte_1746 ) {
			++word_3E;
			return;
		}
		
		gfxPosX = mMemory[ word_3E + 1 ];
		gfxPosY = mMemory[ word_3E + 2 ];
		
		byte_1744 = 1;

		byte_5FD5 = (gfxPosX >> 2);
		byte_5FD5 -= 4;

		byte_5FD6 = (gfxPosY >> 3);
		sub_5FA3();

		// 16A9
		
		for(;;) {
			byte A;

			if( byte_1744 != 1 ) {
				if( byte_1744 != byte_1746 )
					A = 0x1C;
				else
					A = 0x1D;
			} else 
				A = 0x1B;

			// 16C1
			gfxCurrentID = A;
			screenDraw( 0, gfxCurrentID, gfxPosX, gfxPosY );

			byte_1745 = 1;
			
			// 16D1
			for(;;) {
				
				if( byte_1744 != 1 ) {
					
					if( byte_1744 != byte_1746 )
						A = 0x44;

					else {
						// 16EE
						if( byte_1745 == mGfxWidth )
							A = 0x40;
						else
							A = 0x44;
					}

				} else {
					// 16E2
					if( byte_1745 == 1 )
						A = 0x04;
					else
						A = 0x44;
				}

				// 16F8
				A |= mMemory[ word_3C ];
				mMemory[ word_3C ] = A;
				
				++byte_1745;
				word_3C += 2;

				if( byte_1745 > mGfxWidth )
					break;
			}
			
			gfxPosX += (mGfxWidth << 2);
			++byte_1744;

			if( byte_1744 > byte_1746 )
				break;
		}
		// 1732

		word_3E += 3;
	}

}

void cCreep::sub_57DF( byte pX ) {

	if( !(mMemory[ 0xBF04 + pX ] & byte_83F) ) {
		mTxtX_0 = mMemory[ 0xBF01 + pX ];
		mTxtY_0 = mMemory[ 0xBF02 + pX ];
		
		screenDraw( 1, 0, 0, 0, mMemory[ 0xBF03 + pX ] );
		
		mMemory[ 0xBF04 + pX ] |= byte_83F;
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
void cCreep::hw_SpritePrepare( byte &pX ) {
	byte byte_5E8C, byte_5E8D, byte_5E88;
	byte A;

	word word_38 = mMemory[ 0xBD03 + pX ];
	word_38 <<= 1;

	word_38 += 0x603B;
	
	word_30 = readWord( &mMemory[ word_38 ] );
	
	mMemory[ 0xBD09 + pX ] = mMemory[ word_30 + 2 ];
	
	byte_5E8D = mMemory[ word_30 ];
	mMemory[ 0xBD0A + pX ] = byte_5E8D << 2;
	mMemory[ 0xBD0B + pX ] = mMemory[ word_30 + 1 ];
	
	byte_5E88 = pX >> 5;
	byte Y = byte_5E88;
	// 5D72
	
	word_32 = mMemory[ 0x26 + Y ] ^ 8;
	word_32 <<= 6;
	word_32 += 0xC000;

	word_30 += 0x03;
	byte_5E8C = 0;
	
	// 5DB2
	for(;;) {
		Y = 0;

		for(;;) {
			
			if( Y < byte_5E8D )
				A = mMemory[ word_30 + Y ];
			else
				A = 0;

			mMemory[ word_32 + Y ] = A;
			++Y;
			if( Y >= 3 )
				break;
		}

		++byte_5E8C;
		if( byte_5E8C == 0x15 )
			break;

		if( byte_5E8C < mMemory[ 0xBD0B + pX ] ) 
			word_30 += byte_5E8D;
		else 
			word_30 = 0x5E89;
		
		// 5DED
		word_32 += 0x03;
	}

	// 5DFB
	Y = byte_5E88;
	
	cSprite *sprite = mScreen->spriteGet(Y);

	word dataSrc = mMemory[ 0x26 + Y ] ^ 8;
	dataSrc <<= 6;
	dataSrc += 0xC000;

	mMemory[ 0x26 + Y ] = mMemory[ 0x26 + Y ] ^ 8;

	// Sprite Color
	mMemory[ 0xD027 + Y ]  = mMemory[ 0xBD09 + pX ] & 0x0F;
	sprite->_color = mMemory[ 0xD027 + Y ];

	if( !(mMemory[ 0xBD09 + pX ] & byte_88A )) {
		A = (mMemory[ 0x2F82 + Y ] ^ 0xFF) & mMemory[ 0xD01D ];
		sprite->_rDoubleWidth = false;
	} else {
		A = (mMemory[ 0xD01D ] | mMemory[ 0x2F82 + Y ]);
		mMemory[ 0xBD0A + pX ] <<= 1;
		sprite->_rDoubleWidth = true;
	}

	// Sprite X Expansion
	mMemory[ 0xD01D ] = A;
	
	// 5E2D
	if( !(mMemory[ 0xBD09 + pX ] & byte_88B )) {
		A = (mMemory[ 0x2F82 + Y ] ^ 0xFF) & mMemory[ 0xD017 ];
		sprite->_rDoubleHeight = false;
	} else {
		A = (mMemory[ 0xD017 ] | mMemory[ 0x2F82 + Y ]);
		mMemory[ 0xBD0B + pX ] <<= 1;
		sprite->_rDoubleHeight = true;
	}

	// Sprite Y Expansion
	mMemory[ 0xD017 ] = A;

	// 5E4C
	if( !(mMemory[ 0xBD09 + pX ] & byte_88C )) {
		A = mMemory[ 0xD01B ] | mMemory[ 0x2F82 + Y ];
		sprite->_rPriority = true;
	} else {
		A = (mMemory[ 0x2F82 + Y ] ^ 0xFF) & mMemory[ 0xD01B ]; 
		sprite->_rPriority = false;
	}

	// Sprite data priority
	mMemory[ 0xD01B ] = A;

	// 5E68
	if(! (mMemory[ 0xBD09 + pX ] & byte_88D )) {
		A = mMemory[ 0xD01C ] | mMemory[ 0x2F82 + Y ];
		sprite->_rMultiColored = true;
	} else {
		A = (mMemory[ 0x2F82 + Y ] ^ 0xFF) & mMemory[ 0xD01C ];
		sprite->_rMultiColored = false;
	}
		
	// MultiColor Enable
	mMemory[ 0xD01C ] = A;

	sprite->streamLoad( &mMemory[ dataSrc ] );
	mScreen->spriteRedrawSet();
}

// 25B8
void cCreep::gamePositionFilenameGet( bool pLoading ) {
	
	screenClear();
	
	word_3E = 0x2633;
	roomPrepare();
	
	if( pLoading )
		word_3E = 0x261F;
	 else
		word_3E = 0x2609;

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
	
	gamePositionFilenameGet( true );
	if(!mStrLength)
		return;

	string filename = string( (char*) &mMemory[ 0x278E ], mStrLength );
	
	if( mCastleManager->positionLoad( filename, memory( 0x7800 ) ) == true) {
		byte_24FD = 1;
	}

	sub_2973();
}

// 24FF
void cCreep::gamePositionSave() {
	
	gamePositionFilenameGet( false );
	if(!mStrLength)
		return;

	string filename = string( (char*) &mMemory[ 0x278E ], mStrLength );
	// Save from 0x7800

	word saveSize = readWord( memory( 0x7800 ) );
	
	if( mCastleManager->positionSave( filename, saveSize, memory( 0x7800 ) ) == false) {

		word_3E = 0x25AA;	// IO ERROR
		screenClear();
		obj_stringPrint();

		hw_Update();
		hw_IntSleep(0x23);
	} else
		sub_2973();

}

void cCreep::sub_5F6B( byte &pX ) {
	byte_5FD5 = mMemory[ 0xBD01 + pX ] + mMemory[ 0xBD0C + pX ];
	
	byte_5FD7 = byte_5FD5 & 3;
	byte_5FD5 = (byte_5FD5 >> 2) - 4;

	byte_5FD6 = mMemory[ 0xBD02 + pX ] + mMemory[ 0xBD0D + pX ];
	byte_5FD8 = byte_5FD6 & 7;
	byte_5FD6 >>= 3;
	
	sub_5FA3();
}

void cCreep::sub_5FA3() {
	// 5fa6
	word_3C = mMemory[ 0x5CE6 + byte_5FD6 ];
	word_3C += mMemory[ 0x5D06 + byte_5FD6 ] << 8;

	word_3C <<= 1;
	word_3C += 0xC000;

	byte A = byte_5FD5 << 1;
	word_3C += A;
}

void cCreep::obj_SlidingPole_Prepare() {
	byte byte_17ED;
	byte A, gfxPosX, gfxPosY;

	for(;;) {
	
		byte_17ED = mMemory[ word_3E ];

		if( ! byte_17ED ) {
			++word_3E;
			return;
		}

		gfxPosX = mMemory[ word_3E + 1 ];
		gfxPosY = mMemory[ word_3E + 2 ];
		
		byte_5FD5 = (gfxPosX >> 2) - 0x04;
		byte_5FD6 = (gfxPosY >> 3);

		sub_5FA3();

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

			--byte_17ED;
			if( !byte_17ED ) {
				word_3E += 0x03;
				break;
			}
			
			gfxPosY += 0x08;
			word_3C += 0x50;
		}
	}
}

void cCreep::obj_Ladder_Prepare() {
	byte byte_18E3, gfxPosX, gfxPosY;
	
	for(;;) {
	
		byte_18E3 = mMemory[ word_3E ];
		if( byte_18E3 == 0 ) {
			++word_3E;
			return;
		}

		// 1800
		gfxPosX = mMemory[ word_3E + 1 ];
		gfxPosY = mMemory[ word_3E + 2 ];

		byte A = gfxPosX >> 2;
		A -= 0x04;

		byte_5FD5 = A;
		A = (gfxPosY >> 3);
		byte_5FD6 = A;

		sub_5FA3();

		// 1828
		for(;;) {
			if( (mMemory[ word_3C ] & 0x44) == 0 ) {
				byte  gfxCurrentID;

				if( byte_18E3 != 1 ) 
					gfxCurrentID = 0x28;
				else
					gfxCurrentID = 0x2B;

				screenDraw( 0, gfxCurrentID, gfxPosX, gfxPosY, 0 );

			} else {
				// 184C
				if( byte_18E3 == 1 ) {
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
			if( byte_18E3 != mMemory[ word_3E ] )
				mMemory[ word_3C ] = ( mMemory[ word_3C ] | 1);
			
			--byte_18E3;
			if( byte_18E3 == 0 ) { 
			
				word_3E += 3;
				break;
			}

			mMemory[ word_3C ] |= 0x10;
			gfxPosY += 8;
		
			word_3C += 0x50;
		}
	}

}

// 3FD5: Door Opening
void cCreep::obj_Door_Img_Execute( byte pX ) {

	if( mMemory[ 0xBE01 + pX ] == 0 ) {
		mMemory[ 0xBE01 + pX ] = 1;
		mMemory[ 0xBE02 + pX ] = 0x0E;

		word_40 = (mMemory[ 0xBE00 + pX ] << 3) + word_41D3;
		
		mMemory[ word_40 + 2 ] |= 0x80;
		byte A = mMemory[ word_40 + 4 ];

		lvlPtrCalculate( mMemory[ word_40 + 3 ] );
		sub_6009( A );
		mMemory[ word_40 + 2 ] |= 0x80;
	}
	// 4017
	byte A = 0x10;
	A -= mMemory[ 0xBE02 + pX ];
	mMemory[ 0x75B7 ] = A;

	sub_21C8( 3 );
	A = mMemory[ 0xBE02 + pX ];

	if( A ) {
		--mMemory[ 0xBE02 + pX ];
		A += mMemory[ 0xBF02 + pX ]; 
		mTxtY_0 = A;
		mTxtX_0 = mMemory[ 0xBF01 + pX ];
		screenDraw( 1, 0, 0, 0, 0x7C );
		return;
	}
	mMemory[ 0xBF04 + pX ] ^= byte_840;
	for(char Y = 5; Y >= 0; --Y ) 
		mMemory[ 0x6390 + Y ] = mMemory[ 0xBE03 + pX ];

	img_Update( 0x08, mMemory[ 0xBF01 + pX ], mMemory[ 0xBF02 + pX ], 0, pX );
}

// 4075: In Front Door
void cCreep::obj_Door_InFront( byte pX, byte pY ) {
	byte byte_41D5 = pY;
	if( mMemory[ 0xBE01 + pY ] == 0 )
		return;
	if( mMemory[ 0xBD00 + pX ] )
		return;

	// 4085
	if( mMemory[ 0xBD1E + pX ] != 1 )
		return;
	
	pY = mMemory[ 0xBD1C + pX ];

	byte A = mMemory[ 0x780D + pY ];
	if( A != 0 )
		return;

	// Enter the door
	mMemory[ 0x780D + pY ] = 6;
	mMemory[ 0xBD1B + pX ] = 0;
	mMemory[ 0xBD06 + pX ] = 3;
	
	A = mMemory[ 0xBE00 + byte_41D5 ];
	A <<= 3;

	word_40 = word_41D3 + A;

	// 40BB

	mMemory[ 0xBD02 + pX ] = mMemory[ word_40 + 1 ] + 0x0F;
	mMemory[ 0xBD01 + pX ] = mMemory[ word_40 ] + 0x06;
	if( mMemory[ word_40 + 7 ] != 0 ) {
		mMemory[ 0x785D + mMemory[ 0xBD1C + pX ] ] = 1;
	}

	//40DD
	word word_41D6 = readWord( &mMemory[ word_40 + 3 ] );
	lvlPtrCalculate( (word_41D6 & 0xFF) );
	
	mMemory[ word_42 ] |= byte_8C0;

	pY = mMemory[ 0xBD1C + pX ];

	// Set player room / door
	mMemory[ 0x7809 + pY ] = word_41D6 & 0xFF;
	mMemory[ 0x780B + pY ] = (word_41D6 & 0xFF00) >> 8;
}

// 4F5C: Load the rooms' Teleports
void cCreep::obj_Teleport_Prepare() {
	mTxtX_0 = mMemory[ word_3E ];
	mTxtY_0 = mMemory[ word_3E + 1 ];

	byte byte_50D0 = 3;
	for(;;) {
		screenDraw( 1, 0, 0, 0, 0x1C );
		
		mTxtX_0 += 0x04;
		--byte_50D0;
		if( byte_50D0 == 0 )
			break;
	}

	byte gfxPosX = mMemory[ word_3E ];
	byte gfxPosY = mMemory[ word_3E + 1 ];

	// Draw the teleport unit
	screenDraw( 0, 0x6F, gfxPosX, gfxPosY, 0 );

	//4fad
	gfxPosX += 0x0C;
	gfxPosY += 0x18;
	screenDraw( 0, 0x1C, gfxPosX, gfxPosY, 0 );

	byte X;
	img_FindFree( X );
	
	mMemory[ 0xBF00 + X ] = 0x0A;
	gfxPosX = mMemory[ word_3E ] + 4;
	gfxPosY = mMemory[ word_3E + 1 ] + 0x18;

	mMemory[ 0xBE00 + X ] = (word_3E & 0xFF);
	mMemory[ 0xBE01 + X ] = (word_3E & 0xFF00) >> 8;
	
	img_Update( 0x70, gfxPosX, gfxPosY, 0, X );

	obj_Teleport_unk( (mMemory[ word_3E + 2 ] + 2), X );
	
	byte_50D0 = 0x20;

	while( mMemory[ word_3E + 3 ] ) {
		byte A = byte_50D0;
		mMemory[ 0x6E95 ] = mMemory[ 0x6E96 ] = mMemory[ 0x6E97 ] = mMemory[ 0x6E98 ] = A;
		gfxPosX = mMemory[ word_3E + 3 ];
		gfxPosY = mMemory[ word_3E + 4 ];

		screenDraw( 0, 0x72, gfxPosX, gfxPosY, 0 );
		
		word_3E += 2;

		byte_50D0 += 0x10;
	}

	word_3E += 0x04;
}

// 475E: Mummy Releasing
void cCreep::obj_Mummy_Img_Execute( byte pX ) {
	if( byte_2E36 & 3 )
		return;

	byte A;

	--mMemory[ 0xBE01 + pX ];
	if( mMemory[ 0xBE01 + pX ] == 0 ) {
		mMemory[ 0xBF04 + pX ] ^= byte_840;
		A = 0x66;
	} else {
		// 4776
		if( mMemory[ 0xBE02 + pX ] == 0x66 )
			A = 0x11;
		else
			A = 0x66;
	}
	
	// 4784
	for(char Y = 5; Y >= 0; --Y)
		mMemory[ 0x68F0 + Y ] = A;

	// 478C
	mMemory[ 0xBE02 + pX ] = A;
	img_Update( mMemory[ 0xBF03 + pX ], mMemory[ 0xBF01 + pX ], mMemory[ 0xBF02 + pX ], 0, pX );
}

// 4B1A: 
void cCreep::obj_RayGun_Img_Execute( byte pX ) {
	byte gfxPosX = 0;
	byte gfxPosY = 0;
	byte Y = 0;

	if( byte_2E36 & 3 )
		return;

	word_40 = word_4D5B + mMemory[ 0xBE00 + pX ];

	byte A = mMemory[ 0xBF04 + pX ];
	if(!( A & byte_83F )) {
		if( mIntro == 1 )
			return;

		// 4B46
		if(!(mMemory[ word_40 ] & byte_4D62) ) {
			byte_4D5D = 0xFF;
			byte_4D5E = 0x00;
			byte_4D5F = 0x01;

			for(;;) {

				if( mMemory[ 0x780D + byte_4D5F ] == 0 ) {
					byte Y = mMemory[ 0x34D1 + byte_4D5F ];
					char A = mMemory[ 0xBD02 + Y ];
					A -= mMemory[ 0xBF02 + pX ];
					if( A < 0 )
						A = (A ^ 0xFF) + 1;

					if( A < byte_4D5D ) {
						byte_4D5D = A;
						byte A = mMemory[ 0xBD02 + Y ];

						if( A >= 0xC8 || A < mMemory[ 0xBF02 + pX ] ) {
							byte_4D5E = byte_4D65;	// Will Move Up
						} else {
							byte_4D5E = byte_4D66;	// Will Move Down
						}
					}
				}
				//4B9C
				--byte_4D5F;
				if( (char) byte_4D5F < 0 )
					break;
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
			if( A >= mMemory[ 0xBE01 + pX ] )
				goto s4BD9;

			mMemory[ word_40 + 4 ] = A + 1;
			obj_RayGun_Control_Update( 0xC2 );
		}	
	}
	// 4BF4
	gfxPosX = mMemory[ 0xBF01 + pX ];
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
	img_Update( mMemory[ 0x4D68 + Y ], gfxPosX, gfxPosY, 0, pX );

s4C27:;
	A = mMemory[ word_40 ];

	if( A & byte_4D62 ) {
		A ^= byte_4D62;
		mMemory[ word_40 ] = A;

		if( !(A & byte_4D64 ))
			return;
	} else {
		// 4C3D
		if( byte_4D5D >= 5 )
			return;
	}

	// Fire Weapon

	// 4C44
	A = mMemory[ word_40 ];
	if( (A & byte_4D61) )
		return;

	obj_RayGun_Laser_Add( pX );
	A |= byte_4D61;
	mMemory[ word_40 ] = A;
}

// 4E32: Teleport?
void cCreep::obj_Teleport_Img_Execute( byte pX ) {
	if( byte_2E36 & 1 )
		return;

	byte A = seedGet();
	A &= 0x3F;
	
	mMemory[ 0x75CD + 2 ] = A;
	sub_21C8( 0x04 );
	if( byte_2E36 & 3  )
		A = 1;
	else
		A = mMemory[ 0xBE02 + pX ];

	A <<= 4;
	mMemory[ 0x6E95 ] = mMemory[ 0x6E96 ] = mMemory[ 0x6E97 ] = mMemory[ 0x6E98 ] = A;
	byte gfxPosX = mMemory[ 0xBE04 + pX ];
	byte gfxPosY = mMemory[ 0xBE05 + pX ];
	screenDraw( 0, 0x72, gfxPosX, gfxPosY, 0 );

	if( byte_2E36 & 3 ) 
		A = 0;
	else
		A = mMemory[ 0xBE02 + pX ];

	obj_Teleport_unk(A, pX);
	if( byte_2E36 & 3 )
		return;
	
	--mMemory[ 0xBE03 + pX ];
	if( mMemory[ 0xBE03 + pX ] )
		return;

	mMemory[ 0xBF04 + pX ] ^= byte_840;
}

// 4C58: Load the rooms' Ray Guns
void cCreep::obj_RayGun_Prepare() {

	word_4D5B = word_3E;
	byte_4D5D = 0;

	for(;;) {
		if( mMemory[ word_3E ] & byte_4D60 ) {
			++word_3E;
			break;
		}
		// 4C7E

		mMemory[ word_3E ] &=( 0xFF ^ byte_4D61);
		byte gfxPosX = mMemory[ word_3E + 1 ];
		byte gfxPosY = mMemory[ word_3E + 2 ];
		byte gfxCurrentID;

		if( mMemory[ word_3E ] & byte_4D67 )
			gfxCurrentID = 0x5F;
		else
			gfxCurrentID = 0x60;

		byte_4D5E = mMemory[ word_3E + 3 ];
		for(;;) {
			if(!byte_4D5E)
				break;
			
			screenDraw( 0, gfxCurrentID, gfxPosX, gfxPosY, 0 );
			gfxPosY += 0x08;
			--byte_4D5E;
		}
		// 4CCB
		if(!( mMemory[ word_3E ] & byte_4D63 )) {
			byte X;

			img_FindFree( X );
			
			mMemory[ 0xBF00 + X ] = 8;
			mMemory[ 0xBE00 + X ] = byte_4D5D;
			mMemory[ 0xBF04 + X ] |= byte_840;
			
			byte A = mMemory[ word_3E + 3 ];
			A <<= 3;
			A += mMemory[ word_3E + 2 ];
			A -= 0x0B;
			mMemory[ 0xBE01 + X ] = A;
			// 4D01
			if( !(mMemory[ word_3E ] & byte_4D67) ) {
				A = mMemory[ word_3E + 1 ];
				A += 4;
			} else {
				A = mMemory[ word_3E + 1 ];
				A -= 8;
			}
			mMemory[ 0xBF01 + X ] = A;
		} 
		
		// 4D1A
		byte X;

		img_FindFree( X );
		mMemory[ 0xBF00 + X ] = 9;
		gfxPosX = mMemory[ word_3E + 5 ];
		gfxPosY = mMemory[ word_3E + 6 ];

		img_Update( 0x6D, gfxPosX, gfxPosY, 0, X );

		mMemory[ 0xBE00 + X ] = byte_4D5D;

		word_3E += 0x07;
		byte_4D5D += 0x07;
	}
}

// 49F8: Load the rooms' Keys
void cCreep::obj_Key_Load() {
	word_4A65 = word_3E;

	byte_4A64 = 0;
	
	for(;;) {
		if( mMemory[ word_3E ] == 0 ) {
			++word_3E;
			break;
		}

		if( mMemory[ word_3E + 1 ] != 0 ) {
			byte X;
			
			img_FindFree(X);
			mMemory[ 0xBF00 + X ] = 6;

			byte gfxPosX = mMemory[ word_3E + 2 ];
			byte gfxPosY = mMemory[ word_3E + 3 ];
			byte gfxCID = mMemory[ word_3E + 1 ];

			mMemory[ 0xBE00 + X ] = byte_4A64;

			img_Update( gfxCID, gfxPosX, gfxPosY, 0, X );
		}

		// 4A47
		byte_4A64 += 0x04;
		word_3E += 0x04;
	}

}

void cCreep::obj_Door_Lock_Prepare() {
	
	byte X, gfxPosX, gfxPosY;

	for(;;) {
		
		if( mMemory[ word_3E ] == 0 )
			break;

		img_FindFree( X );
		
		mMemory[ 0xBF00 + X ] = 7;
		gfxPosX = mMemory[ word_3E + 3 ];
		gfxPosY = mMemory[ word_3E + 4 ];
		
		byte A = (mMemory[ word_3E ] << 4);
		A |= mMemory[ word_3E ];

		for( char Y = 8; Y >= 0; --Y )
			mMemory[ 0x6C53 + Y ] = A;

		mMemory[ 0xBE00 + X ] = *level( word_3E );
		mMemory[ 0xBE01 + X ] = *level( word_3E + 2 );
		img_Update( 0x58, gfxPosX, gfxPosY, 0, X );

		word_3E += 0x05;
	}

	++word_3E;
}

void cCreep::obj_Door_Prepare() {
	byte byte_41D0 = *level(word_3E++);
	word_41D3 = word_3E;
	
	byte X, gfxCurrentID, gfxPosX, gfxPosY;

	for( byte count = 0; count != byte_41D0; ++count) {
		X = *level(word_3E + 7);
		
		gfxCurrentID  = mMemory[ 0x41D1 + X ];
		gfxPosX = *level( word_3E + 0 );
		gfxPosY = *level( word_3E + 1 );

		screenDraw( 0, gfxCurrentID, gfxPosX, gfxPosY );
		// 4159

		img_FindFree( X );
		
		gfxPosX += 0x04;
		gfxPosY += 0x10;

		mMemory[ 0xBE00 + X ] = count;
		mMemory[ 0xBF00 + X ] = 0;

		lvlPtrCalculate( *level( word_3E + 3 ) );
		
		byte A =  (*level( word_42 ) & 0xF);

		mMemory[ 0xBE03 + X ] = A;
		A <<= 4;

		A |= mMemory[ 0xBE03 + X ];
		mMemory[ 0xBE03 + X ] = A;
		
		A = *level( word_3E + 2 );
		if(A & 0x80) {
			mMemory[ 0xBE01 + X ] = 1;
			A = mMemory[ 0xBE03 + X ];

			byte Y = 5;
			while(Y) {
				mMemory[ 0x6390 + Y ] = A;
				--Y;
			}

			A = 0x08;
		} else
			A = 0x07;

		// 41B2
		gfxCurrentID = A;
		img_Update( gfxCurrentID, gfxPosX, gfxPosY, 0, X );

		word_3E += 0x08;
	}

}

// 42AD: Lightning Machine pole movement
void cCreep::obj_Lightning_Img_Execute( byte pX ) {
	byte gfxPosX, gfxPosY;

	byte byte_43E2, byte_43E3;
	word_40 = word_45DB + mMemory[ 0xBE00 + pX ];
	byte Y = 0;

	if( mMemory[ 0xBE01 + pX ] != 1 ) {
		mMemory[ 0xBE01 + pX ] = 1;

		obj_Lightning_Add( pX );

	} else {
		// 42CF
		byte A = mMemory[ word_40 + Y ];
		if( !(A & byte_45DE )) {
			// 42D8
			mMemory[ 0xBE01 + pX ] = 0;
			mMemory[ 0xBF04 + pX ] ^= byte_840;
			mMemory[ 0x66E6 ] = mMemory[ 0x66E7 ] = 0x55;

			gfxPosX = mMemory[ word_40 + 1 ];
			gfxPosY = mMemory[ word_40 + 2 ];
			byte_43E2 = mMemory[ word_40 + 3 ];
			
			for(;;) {
				if( !byte_43E2 )
					break;
				
				screenDraw( 0, 0x34, gfxPosX, gfxPosY, 0 );
				gfxPosY += 0x08;

				--byte_43E2;
			}

			Y = 0;

			// 4326
			for(;;) {
				if( mMemory[ 0xBD00 + Y] == 1 ) {
					if( !(mMemory[ 0xBD04 + Y ] & byte_889) )
						if( mMemory[ 0xBD1F + Y ] == mMemory[ 0xBE00 + pX ] )
							break;
				}

				Y+= 0x20;
			}
			// 4345
			mMemory[ 0xBD04 + Y ] |= byte_885;
			return;

		} else {
			// 4351
			if( byte_2E36 & 3 )
				return;
		}
	}
	// 435B
	++mMemory[ 0xBE02 + pX ];
	byte A = mMemory[ 0xBE02 + pX];
	if( A >= 3 ) {
		A = 0;
		mMemory[ 0xBE02 + pX ] = A;
	}
	
	byte_43E3 = A;
	
	gfxPosX = mMemory[ word_40 + 1 ];
	gfxPosY = mMemory[ word_40 + 2 ];
	byte_43E2 = mMemory[ word_40 + 3 ];

	for(;;) {
		if( !byte_43E2 )
			break;

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
		--byte_43E2;
	}

}

void cCreep::obj_Door_Button_Prepare() {
	byte gfxCurrentID, gfxPosX, gfxPosY;
	byte byte_42AB;

	byte_42AB = mMemory[ word_3E ];
	++word_3E;
	byte X = 0;

	for( ;; ) {

		if( byte_42AB == 0 )
			return;

		img_FindFree(X);
		mMemory[ 0xBF00 + X ] = 1;

		gfxPosX = mMemory[word_3E];
		gfxPosY = mMemory[word_3E+1];
		gfxCurrentID = 0x09;
		mMemory[ 0xBE00 + X ] = mMemory[ word_3E+2 ];
		
		byte A = 0;
		char Y = 0;

		for(;;) {
			if( mMemory[ 0xBF00 + Y ] == 0 ) {

				if( mMemory[ 0xBE00 + Y ] == mMemory[ 0xBE00 + X ] ) {
					A = mMemory[ 0xBE03 + Y ];
					break;
				}
			}	

			Y += 8;
		}

		Y = 8;
		while( Y >= 0 ) {
			mMemory[ 0x63D2 + Y ] = A;
			--Y;
		}

		A >>= 4;
		A |= 0x10;

		mMemory[ 0x63D6 ] = A;
		img_Update( gfxCurrentID, gfxPosX, gfxPosY, 0, X);
		word_3E += 0x03;

		--byte_42AB;
	}

}

// 44E7: Lightning Switch
void cCreep::obj_Lightning_Switch_InFront( byte pX, byte pY ) {
	if( mMemory[ 0xBD00 + pX ] )
		return;

	if( (mMemory[ 0xBD01 + pX ] + mMemory[ 0xBD0C + pX ]) - mMemory[ 0xBF01 + pY ] >= 4 )
		return;

	if( mMemory[ 0xBD1E + pX ] != 0 && mMemory[ 0xBD1E + pX ] != 4 )
		return;

	// 4507
	byte byte_45D7 = 0;
	word_30 = word_45DB + mMemory[ 0xBE00 + pY ];
	byte byte_45D8 = pY;

	if( !(mMemory[ word_30 ] & byte_45DE )) {
		if( mMemory[ 0xBD1E + pX ] )
			return;
	} else {
		if( !(mMemory[ 0xBD1E + pX ]) )
			return;
	}
	
	// 4535
	mMemory[ word_30 ] ^= byte_45DE;
	for(;;) {
		if( byte_45D7 >= 4 )
			break;

		byte A = mMemory[ word_30 + (byte_45D7 + 4) ];
		if( A == 0xFF )
			break;

		// 4553
		byte byte_45DA = A;
		word_32 = word_45DB + A;
		
		mMemory[ word_32 ] ^= byte_45DE;
		byte Y;

		for(Y = 0;; Y += 8 ) {
			
			if( mMemory[ 0xBF00 + Y ] != 2 )
				continue;

			if( mMemory[ 0xBE00 + Y ] == byte_45DA )
				break;
		}

		// 4585
		mMemory[ 0xBF04 + Y ] |= byte_840;
		++byte_45D7;
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

	img_Update( A, mMemory[ 0xBF01 + byte_45D8 ], mMemory[ 0xBF02 + byte_45D8 ], 0, byte_45D8 );

	sub_21C8(0x06);
}

// 45E0: Forcefield Timer
void cCreep::obj_Forcefield_Img_Timer_Execute( byte pX ) {
	--mMemory[ 0xBE01 + pX ];
	if( mMemory[ 0xBE01 + pX ] != 0 )
		return;

	--mMemory[ 0xBE02 + pX ];
	byte Y = mMemory[ 0xBE02 + pX ];
	byte A = mMemory[ 0x4756 + Y ];

	mMemory[ 0x75AB ] = A;
	sub_21C8( 2 );

	for( Y = 0; Y < 8; ++Y ) {
		if( Y >= mMemory[ 0xBE02 + pX ] )
			A = 0x55;
		else
			A = 0;

		mMemory[ 0x6889 + Y ] = A;
	}

	screenDraw( 0, 0x40, mMemory[ 0xBF01 + pX ], mMemory[ 0xBF02 + pX ], 0 );
	if( mMemory[ 0xBE02 + pX ] != 0 ) {
		mMemory[ 0xBE01 + pX ] = 0x1E;
		return;
	} 

	// 4633
	mMemory[ 0xBF04 + pX ] ^= byte_840;
	mMemory[ 0x4750 + mMemory[ 0xBE00 + pX ] ] = 1;
}

// Lightning Machine Setup
void cCreep::obj_Lightning_Prepare() {
	byte	byte_44E5, byte_44E6, gfxPosX, gfxPosY;

	word_45DB = word_3E;
	byte_44E5 = 0;

	byte X = 0, A;

	for(;;) {
		
		if( mMemory[ word_3E ] & byte_45DF ) {
			++word_3E;
			break;
		}

		img_FindFree( X );

		mMemory[ 0xBE00 + X ] = byte_44E5;
		if( mMemory[ word_3E ] & byte_45DD ) {
			// 441C
			gfxPosX = mMemory[ word_3E + 1 ];
			gfxPosY = mMemory[ word_3E + 2 ];
			screenDraw( 0, 0x36, gfxPosX, gfxPosY, 0 );
			
			gfxPosX += 0x04;
			gfxPosY += 0x08;

			mMemory[ 0xBF00 + X ] = 3;
			if( mMemory[ word_3E ] & byte_45DE )
				A = 0x37;
			else
				A = 0x38;

			img_Update( A, gfxPosX, gfxPosY, 0, X );

		} else {
			// 4467
			mMemory[ 0xBF00 + X ] = 2;
			gfxPosX = mMemory[ word_3E + 1 ];
			gfxPosY = mMemory[ word_3E + 2 ];

			byte_44E6 = mMemory[ word_3E + 3 ];
			mMemory[ 0xBE03 + X ] = mMemory[ word_3E + 3 ];

			for(;; ) {
				if( !byte_44E6 ) 
					break;

				screenDraw( 0, 0x32, gfxPosX, gfxPosY, 0 );
				gfxPosY += 0x08;
				--byte_44E6;
			}

			gfxPosX -= 0x04;

			img_Update( 0x33, gfxPosX, gfxPosY, 0, X );
			if( mMemory[ word_3E ] & byte_45DE )
				mMemory[ 0xBF04 + X ] |= byte_840;
		}

		// 44C8
		byte_44E5 += 0x08;
		word_3E += 0x08;
	}
}

// 46AE: Forcefield
void cCreep::obj_Forcefield_Prepare() {
	byte X = 0;
	byte gfxPosX, gfxPosY;

	byte_474F = 0;
	
	for(;;) {
		
		if( mMemory[ word_3E ] == 0 ) {
			++word_3E;
			return;
		}

		img_FindFree( X );

		mMemory[ 0xBF00 + X ] = 4;

		gfxPosX = mMemory[ word_3E ];
		gfxPosY = mMemory[ word_3E + 1 ];

		// Draw outside of timer
		screenDraw( 0, 0x3F, gfxPosX, gfxPosY, 0 );

		// 46EA
		gfxPosX += 4;
		gfxPosY += 8;

		for( char Y = 7; Y >= 0; --Y ) 
			mMemory[ 0x6889 + Y ] = 0x55;

		// Draw inside of timer
		img_Update( 0x40, gfxPosX, gfxPosY, 0, X );

		mMemory[ 0xBE00 + X ] = byte_474F;
		mMemory[ 0x4750 + byte_474F ] = 1;

		obj_Forcefield_Create( );

		gfxPosX = mMemory[ word_3E + 2 ];
		gfxPosY = mMemory[ word_3E + 3 ];

		// Draw top of forcefield
		screenDraw( 0, 0x3E, gfxPosX, gfxPosY, 0 );
		
		++byte_474F;
		word_3E += 0x04;
	}

}

// 4872 : Load the rooms' Mummys
void cCreep::obj_Mummy_Prepare( ) {
	byte	byte_498D = 0, byte_498E, byte_498F;
	byte	X;
	byte	gfxCurrentID;

	word_498B = word_3E;

	for(;;) {
		
		if( mMemory[ word_3E ] == 0 ) {
			++word_3E;
			return;
		}

		img_FindFree( X );
		
		mMemory[ 0xBF00 + X ] = 5;

		byte gfxPosX = mMemory[ word_3E + 1 ];
		byte gfxPosY = mMemory[ word_3E + 2 ];
		gfxCurrentID = 0x44;

		mMemory[ 0xBE00 + X ] = byte_498D;
		mMemory[ 0xBE02 + X ] = 0x66;
		for( char Y = 5; Y >= 0; --Y )
			mMemory[ 0x68F0 + Y ] = 0x66;

		img_Update( gfxCurrentID, gfxPosX, gfxPosY, 0, X );
		byte_498E = 3;
		gfxPosY = mMemory[ word_3E + 4 ];
		gfxCurrentID = 0x42;

		while(byte_498E) {
			byte_498F = 5;
			gfxPosX = mMemory[ word_3E + 3 ];

			while(byte_498F) {
				screenDraw( 0, gfxCurrentID, gfxPosX, gfxPosY, 0 );
				gfxPosX += 4;
				--byte_498F;
			}

			gfxPosY += 8;
			--byte_498E;
		}

		if( mMemory[ word_3E ] != 1 ) {
			// 4911
			mTxtX_0 = mMemory[ word_3E + 3 ] + 4;
			mTxtY_0 = mMemory[ word_3E + 4 ] + 8;
			byte_498E = 3;

			while( byte_498E ) {
				screenDraw( 1, gfxCurrentID, gfxPosX, gfxPosY, 0x42 );
				mTxtX_0 += 4;
				--byte_498E;
			}
			
			gfxPosX = mTxtX_0 - 0x0C;
			gfxPosY = mTxtY_0;
			screenDraw( 0, 0x43, gfxPosX, gfxPosY, 0x42 );
			
			if( mMemory[ word_3E ] == 2 ) {
				obj_Mummy_Add(0xFF, X);
			}
		}
		// 496E
		word_3E += 0x07;
		byte_498D += 0x07;
	}
}

// 517F : Load the rooms' Trapdoors
void cCreep::obj_TrapDoor_Prepare( ) {
	byte	byte_5381;
	word_5387 = word_3E;

	byte X;

	byte_5381 = 0;
	for(;;) {
	
		if( (mMemory[ word_3E ] & byte_5389) ) {
			++word_3E;
			return;
		}
		
		img_FindFree( X );
		mMemory[ 0xBF00 + X ] = 0x0B;
		mMemory[ 0xBE00 + X ] = byte_5381;
		if( !(mMemory[ word_3E ] & byte_538A) ) {
			// 51BC
			mMemory[ 0x6F2E ] = 0xC0;
			mMemory[ 0x6F30 ] = 0x55;
		} else {
			// 51c9
			mTxtX_0 = mMemory[ word_3E + 1 ];
			mTxtY_0 = mMemory[ word_3E + 2 ];
			screenDraw( 1, 0, 0, 0, 0x7B );
			
			byte gfxPosX, gfxPosY;

			gfxPosX = mTxtX_0 + 4;
			gfxPosY = mTxtY_0;

			img_Update( 0x79, gfxPosX, gfxPosY, 0x7B, X );

			mMemory[ 0x6F2E ] = 0x20;
			mMemory[ 0x6F30 ] = 0xCC;
			byte_5FD5 = mMemory[ word_3E + 1 ] >> 2;
			byte_5FD5 -= 4;
			
			byte_5FD6 = mMemory[ word_3E + 2 ] >> 3;
			sub_5FA3();

			mMemory[ word_3C ] = mMemory[ word_3C ] & 0xFB;
			mMemory[ word_3C + 4 ] = mMemory[ word_3C + 4 ] & 0xBF;
		}

		// 522E
		img_FindFree( X );
		mMemory[ 0xBF00 + X ] = 0x0C;
		
		byte gfxPosX = mMemory[ word_3E + 3 ];
		byte gfxPosY = mMemory[ word_3E + 4 ];
		
		mMemory[ 0xBE00 + X ] = byte_5381;
		img_Update( 0x7A, gfxPosX, gfxPosY, 0, X );
		
		byte_5381 += 0x05;
		word_3E += 0x05;
	}

}

// 50D2: Floor Switch
void cCreep::obj_TrapDoor_Switch_Img_Execute( byte pX ) {
	
	word_40 = word_5387 + mMemory[ 0xBE00 + pX ];
	if( mMemory[ 0xBE01 + pX ] ) {
		mTxtX_0 = mMemory[ word_40 + 1 ];
		mTxtY_0 = mMemory[ word_40 + 2 ];

		byte A = mMemory[ 0xBE02 + pX ];

		sub_5171( A );
		screenDraw( 1, 0, 0, 0, A );
		if( mMemory[ 0xBE02 + pX ] != 0x78 ) {
			// 515F
			++mMemory[ 0xBE02 + pX ];
			return;
		}
		
		img_Update( 0x79, mMemory[ word_40 + 1 ] + 4, mMemory[ word_40 + 2 ], 0, pX );
		
	} else {
		// 5129
		if( mMemory[ 0xBE02 + pX ] == 0x78 )
			sub_57DF( pX );
		
		byte A = mMemory[ 0xBE02 + pX ];

		sub_5171( A );
		screenDraw( 0, A, mMemory[ word_40 + 1 ], mMemory[ word_40 + 2 ], 0 );
		if( mMemory[ 0xBE02 + pX ] != 0x73 ) {
			--mMemory[ 0xBE02 + pX ];
			return;
		}
	}
	// 5165
	mMemory[ 0xBF04 + pX ] ^= byte_840;
}

// 538B: Conveyor
void cCreep::obj_Conveyor_Img_Execute( byte pX ) {
	
	word_40 = word_564B + mMemory[ 0xBE00 + pX ];
	byte A = mMemory[ word_40 ];

	// 539F
	if( (A & byte_5646) )
		if( !(A & byte_5644) )
			goto s53B3;
	
	if( (A & byte_5645) && !(A & byte_5643) ) {
		// 53B3
s53B3:;
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

			sub_21C8(0xA);
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
		A = byte_2E36 & 1;
		if( A )
			return;

		byte gfxCurrentID = mMemory[ 0xBF03 + pX ];

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
		byte gfxPosX = mMemory[ 0xBF01 + pX ];
		byte gfxPosY = mMemory[ 0xBF02 + pX ];
		
		img_Update( gfxCurrentID, gfxPosX, gfxPosY, 0, pX );
	}
}

// 47A7: In Front Mummy Release
void cCreep::obj_Mummy_Infront( byte pX, byte pY ) {
	byte byte_4870 = pY;
	
	if( mMemory[ 0xBD00 + pX ] )
		return;

	byte A = mMemory[ 0xBD01 + pX ] + mMemory[ 0xBD0C + pX ];
	A -= mMemory[ 0xBF01 + pY ];

	if( A >= 8 )
		return;

	word_40 = word_498B + mMemory[ 0xBE00 + pY ];

	// 47D1
	if( mMemory[ word_40 ] != 1 )
		return;

	mMemory[ word_40 ] = 2;
	mMemory[ word_40 + 5 ] = mMemory[ word_40 + 3 ] + 4;
	// 47ED
	mMemory[ word_40 + 6 ] = mMemory[ word_40 + 4 ] + 7;
	
	// 47FB
	mMemory[ 0xBF04 + pY ] |= byte_840;

	mMemory[ 0xBE01 + pY ] = 8;
	mMemory[ 0xBE02 + pY ] = 0x66;
	mTxtX_0 = mMemory[ word_40 + 3 ] + 4;
	mTxtY_0 = mMemory[ word_40 + 4 ] + 8;
	
	byte byte_4871 = 3;
	
	for(;;) {
		screenDraw( 1, 0, 0, 0, 0x42 );
		mTxtX_0 += 4;
		--byte_4871;
		if( !byte_4871 )
			break;
	}

	// 4842
	byte gfxPosX = mTxtX_0 - 0x0C;
	byte gfxPosY = mTxtY_0;

	screenDraw( 0, 0x43, gfxPosX, gfxPosY, 0 );
	obj_Mummy_Add(0, byte_4870 );
}

// 564E: Load the rooms' frankensteins
void cCreep::obj_Frankie_Load() {
	word_5748 = word_3E;
	byte byte_574B;

	byte_574A = 0;

	for(;;) {

		if( mMemory[ word_3E ] & byte_574C ) {
			++word_3E;
			return;
		}

		mTxtX_0 = mMemory[ word_3E + 1 ];
		mTxtY_0 = mMemory[ word_3E + 2 ] + 0x18;
		screenDraw( 1, 0, 0, 0, 0x92 );

		byte_5FD5 = (mTxtX_0 >> 2) - 4;
		byte_5FD6 = (mTxtY_0 >> 3);

		sub_5FA3();
		byte A;

		if( ( mMemory[ word_3E ] & byte_574F )) {
			word_3C -= 2;
			A = 0xFB;
		} else
			A = 0xBF;

		// 56C4
		byte_574B = A;
		
		for( char Y = 4; Y >= 0; Y -= 2 ) {
			mMemory[ word_3C + Y ] &= byte_574B;
		}
		byte X;
		img_FindFree(X);
		mMemory[ 0xBF00 + X ] = 0x0F;
		byte gfxPosX = mMemory[ word_3E + 1 ];
		byte gfxPosY = mMemory[ word_3E + 2 ];
		if( !(mMemory[ word_3E ] & byte_574F ))
			A = 0x90;
		else
			A = 0x91;

		img_Update( A, gfxPosX, gfxPosY, 0, X );

		//5700
		if(!( mMemory[ word_3E ]  & byte_574F )) {
			gfxPosX += 4;
			gfxPosY += 0x18;
			screenDraw( 0, 0x1C, gfxPosX, gfxPosY, 0 );
		}
		obj_Frankie_Add();

		word_3E += 0x07;
		byte_574A += 0x07;
	}

}

// 5501: Load the rooms' Conveyors
void cCreep::obj_Conveyor_Prepare() {
	word_564B = word_3E;

	byte byte_5649 = 0, gfxPosX = 0, gfxPosY = 0;
	
	byte A, X;

	for(;;) {
		if( mMemory[ word_3E ] & byte_5642 ) {
			++word_3E;
			break;
		}
		
		//5527
		A = 0xFF;

		A ^= byte_5646;
		A ^= byte_5645;
		A ^= byte_5644;
		A ^= byte_5643;

		A &= mMemory[ word_3E ];
		mMemory[ word_3E ] = A;

		img_FindFree( X );

		mMemory[ 0xBF00 + X ] = 0x0D;
		mMemory[ 0xBE00 + X ] = byte_5649;

		mMemory[ 0xBF04 + X ] = (mMemory[ 0xBF04 + X ] | byte_840);
		
		mTxtX_0 = mMemory[ word_3E + 1 ];
		mTxtY_0 = mMemory[ word_3E + 2 ];

		screenDraw( 1, 0x7D, gfxPosX, gfxPosY, 0x7D );
		gfxPosX = mTxtX_0;
		gfxPosY = mTxtY_0;
		
		img_Update( 0x7E, gfxPosX, gfxPosY, 0x7D, X );
		img_FindFree(X);

		mMemory[ 0xBF00 + X ] = 0x0E;
		mMemory[ 0xBE00 + X ] = byte_5649;
		
		gfxPosX = mMemory[ word_3E + 3 ];
		gfxPosY = mMemory[ word_3E + 4 ];
		byte gfxCurrentID = 0x82;
		byte gfxDecodeMode = 0;

		if( (mMemory[ word_3E ] & byte_5648) == 0 ) {
			mMemory[ 0x70A6 ] = 0xC0;
			mMemory[ 0x70A8 ] = 0xC0;
		} else {
			if( (mMemory[ word_3E ] & byte_5647) == 0 ) {
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
		gfxPosX = mMemory[ word_3E + 3] + 0x04;
		gfxPosY = mMemory[ word_3E + 4] + 0x08;

		img_Update( 0x83, gfxPosX, gfxPosY, 0, X );
		byte_5649 += 0x05;
		word_3E += 0x05;
	}
}

void cCreep::obj_Forcefield_Create() {
	byte X = 0;

	obj_FindFree( X );

	mMemory[ 0xBD00 + X ] = 2;
	mMemory[ 0xBD01 + X ] = mMemory[ word_3E + 2 ];
	mMemory[ 0xBD02 + X ] = mMemory[ word_3E + 3 ] + 2;
	mMemory[ 0xBD03 + X ] = 0x35;

	mMemory[ 0xBD1F + X ] = byte_474F;
	mMemory[ 0xBD1E + X ] = 0;
	mMemory[ 0xBD06 + X ] = 4;
	mMemory[ 0xBD0C + X ] = 2;
	mMemory[ 0xBD0D + X ] = 0x19;
}

// 38CE: Mummy ?
void cCreep::obj_Mummy_Collision( byte pX, byte pY ) {
	byte byte_3A07 = pY;
	if( mMemory[ 0xBF00 + pY ] == 0x0B ) {
		
		char A = mMemory[ 0xBD01 + pX ] + mMemory[ 0xBD0C + pX ];
		A -= mMemory[ 0xBF01 + pY ];
		if( A < 4 ) {
			
			word_40 = word_5387 + mMemory[ 0xBE00 + pY ];
			// 38F7
			if( mMemory[ word_40 ] & byte_538A ) {
				// 3900
				word_40 = word_498B + mMemory[ 0xBD1D + pX ];
				mMemory[ word_40 ] = 3;
				return;
			}
		}
	} 

	// 3919
	pY = byte_3A07;
	byte_31F5 = 0;
	if( mMemory[ 0xBF00 + pY ] != 0x0C )
		return;

	char A = mMemory[ 0xBD01 + pX ] + mMemory[ 0xBD0C + pX ];
	A -= mMemory[ 0xBF01 + pY ];
	if( A >= 4 )
		return;

	mMemory[ 0xBD1C + pX ] = mMemory[ 0xBE00 + pY ];
}

// 3940: 
void cCreep::sub_3940( byte pX, byte pY ) {
	byte A = mMemory[ 0xBD00 + pY ];
	if( A == 0 || A == 5 ) {
		byte_311D = 0;
		return;
	}

	word_40 = word_498B + mMemory[ 0xBD1D + pX ];

	mMemory[ word_40 ] = 3;
}

// 3A60:  
void cCreep::sub_3A60( byte pX, byte pY ) {
	byte A = mMemory[ 0xBF00 + pY ] ;
	
	if( A == 2 )
		return;
	if( A == 0x0F )
		return;
	if( A == 8 ) {
		if( mMemory[ 0xBD1E + pX ] != mMemory[ 0xBE00 + pY ] )
			return;
	}

	byte_31F5 = 0;
}

void cCreep::obj_RayGun_Laser_Add( byte pX ) {
	byte Y = pX;

	byte A = mMemory[ 0xBE00 + Y ] + 0x07;
	A |= 0xF8;
	A >>= 1;
	A += 0x2C;
	
	mMemory[ 0x7591 + 2 ] = A;

	sub_21C8( 0 );

	byte X;
	obj_FindFree( X );
	mMemory[ 0xBD00 + X ] = 4;
	mMemory[ 0xBD01 + X ] = mMemory[ 0xBF01 + Y ];
	mMemory[ 0xBD02 + X ] = mMemory[ 0xBF02 + Y ] + 0x05;
	mMemory[ 0xBD03 + X ] = 0x6C;
	mMemory[ 0xBD1E + X ] = mMemory[ 0xBE00 + Y ];

	if( mMemory[ word_40 ] & byte_4D67 ) {
		mMemory[ 0xBD01 + X ] -= 0x08;
		mMemory[ 0xBD1F + X ] = 0xFC;
	} else {
		// 3AD4	
		mMemory[ 0xBD01 + X ] += 0x08;
		mMemory[ 0xBD1F + X ] = 4;
	}

	hw_SpritePrepare( X );
}

// 396A: 
void cCreep::obj_Mummy_Add( byte pA, byte pX ) {
	byte byte_39EE = pA;
	byte X;
	byte Y = pX;

	obj_FindFree( X );
	
	mMemory[ 0xBD00 + X ] = 3;
	mMemory[ 0xBD1B + X ] = 0xFF;
	mMemory[ 0xBD1C + X ] = 0xFF;
	mMemory[ 0xBD1D + X ] = mMemory[ 0xBE00 + Y ];
	
	word_40 = word_498B + mMemory[ 0xBD1D + X ];
	//3998

	mMemory[ 0xBD0C + X ] = 5;
	mMemory[ 0xBD0D + X ] = 0x11;
	mMemory[ 0xBD03 + X ] = 0xFF;
	if( byte_39EE == 0 ) {
		mMemory[ 0xBD1E + X ] = 0;
		mMemory[ 0xBD1F + X ] = 0xFF;
		mMemory[ 0xBD06 + X ] = 4;
		
		mMemory[ 0xBD01 + X ] = mMemory[ word_40 + 3 ] + 0x0D;
		mMemory[ 0xBD02 + X ] = mMemory[ word_40 + 4 ] + 0x08;
	} else {
		// 39D0
		mMemory[ 0xBD1E + X ] = 1;
		mMemory[ 0xBD01 + X ] = mMemory[ word_40 + 5 ];
		mMemory[ 0xBD02 + X ] = mMemory[ word_40 + 6 ];
		mMemory[ 0xBD06 + X ] = 2;
	}

	// 39E8
}

// 379A: Mummy
void cCreep::obj_Mummy_Execute( byte pX ) {
	byte A = mMemory[ 0xBD04 + pX ];

	if( A & byte_885 ) {
		mMemory[ 0xBD04 + pX ] ^= byte_885;
		mMemory[ 0xBD04 + pX ] |= byte_886;
		return;
	}

	if( A & byte_882 ) {
		A ^= byte_882;
		mMemory[ 0xBD04 + pX ] = A;
		if( mMemory[ 0xBD1E + pX ] ) {
			mMemory[ 0xBD03 + pX ] = 0x4B;
			hw_SpritePrepare( pX );
		}
	}
	// 37C6
	char AA = mMemory[ 0xBD1C + pX ];
	if( AA != -1 ) {
		if( AA != mMemory[ 0xBD1B + pX ] )
			sub_526F( AA );
	}
	// 37D5
	mMemory[ 0xBD1B + pX ] = AA;
	mMemory[ 0xBD1C + pX ] = 0xFF;
	word_40 = word_498B + mMemory[ 0xBD1D + pX ];
	if( mMemory[ 0xBD1E + pX ] == 0 ) {
		++mMemory[ 0xBD1F + pX ];
		byte Y = mMemory[ 0xBD1F + pX ];
		A = mMemory[ 0x39EF + Y ];

		if( A != 0xFF ) {
			mMemory[ 0xBD03 + pX ] = A;
			mMemory[ 0xBD01 + pX ] += mMemory[ 0x39F7 + Y ];

			mMemory[ 0xBD02 + pX ] += mMemory[ 0x39FF + Y ];
			
			mMemory[ 0x7630 ] = (mMemory[ 0xBD1F + pX ] << 2) + 0x24;
			sub_21C8( 0x0B );
			hw_SpritePrepare( pX );
			return;
		}
		// 3828
		mMemory[ 0xBD1E + pX ] = 0x01;
		mMemory[ 0xBD01 + pX ] = mMemory[ word_40 + 3 ] + 4;
		mMemory[ 0xBD02 + pX ] = mMemory[ word_40 + 4 ] + 7;
		mMemory[ 0xBD06 + pX ] = 2;
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
	sub_5F6B( pX );

	AA = mMemory[ 0xBD01 + pX ];
	AA -= mMemory[ 0xBD01 + Y ];
	if( AA < 0 ) {
		AA ^= 0xFF;
		++AA;
	}
	// 3872
	if( AA < 2 )
		return;

	// Frame
	++mMemory[ 0xBD03 + pX ];
	if( mMemory[ 0xBD01 + pX ] < mMemory[ 0xBD01 + Y ] ) {
		// Walking Right
		// 3881
		if( !(mMemory[ word_3C ] & 0x04) )
			return;
		
		// 3889
		++mMemory[ 0xBD01 + pX ];
		A = mMemory[ 0xBD03 + pX ];
		if( A < 0x4E || A >= 0x51 )
				mMemory[ 0xBD03 + pX ] = 0x4E;
	
	} else {
		// Walking Left
		// 389F
		if( !(mMemory[ word_3C ] & 0x40) )
			return;

		--mMemory[ 0xBD01 + pX ];
		A = mMemory[ 0xBD03 + pX ];
		if( A < 0x4B || A >= 0x4E)
			mMemory[ 0xBD03 + pX ] = 0x4B;
	}

	// 38BA
	mMemory[ word_40 + 5 ] = mMemory[ 0xBD01 + pX ];
	mMemory[ word_40 + 6 ] = mMemory[ 0xBD02 + pX ];

	hw_SpritePrepare( pX );
}

// 3A08
void cCreep::obj_RayGun_Laser_Execute( byte pX ) {
	byte A = mMemory[ 0xBD04 + pX ];

	mScreen->spriteRedrawSet();

	if( A & byte_885 ) {

		A ^= byte_885;
		A |= byte_886;
		mMemory[ 0xBD04 + pX ] = A;
		
		word_40 = word_4D5B + mMemory[ 0xBD1E + pX ];
		mMemory[ word_40 ] = (0xFF ^ byte_4D61) & mMemory[ word_40 ];

		return;
	} else {
		if( A & byte_882 )
			mMemory[ 0xBD04 + pX ] = A ^ byte_882;

		// 3A42
		A = mMemory[ 0xBD01 + pX ] + mMemory[ 0xBD1F + pX ];
		mMemory[ 0xBD01 + pX ] = A;

		if( A < 0xB0 )
			if( A >= 8 )
				return;
		
		mMemory[ 0xBD04 + pX ] |= byte_885;
	}
}

// 3F14: Find a free object position, and clear it
void cCreep::obj_FindFree( byte &pX ) {
	
	pX = 0;
	byte A;

	for( ;; ) {
		A = mMemory[ 0xBD04 + pX ];
		if( A & byte_889 )
			break;

		pX += 0x20;
		if( pX == 0 )
			return;
	}

	for( byte Y = 0x20; Y > 0; --Y ) {
		mMemory[ 0xBD00 + pX ] = 0;
		++pX;
	}
	
	pX -= 0x20;
	mMemory[ 0xBD04 + pX ] = byte_882;
	mMemory[ 0xBD05 + pX ] = 1;
	mMemory[ 0xBD06 + pX ] = 1;
}

void cCreep::img_Update( byte pGfxID, byte pGfxPosX, byte pGfxPosY, byte pTxtCurrentID, byte pX ) {
	//5783
	byte gfxDecodeMode;

	byte A = mMemory[ 0xBF04 + pX ];
	if( !(A & byte_83F) ) {
		gfxDecodeMode = 2;
		mTxtX_0 = mMemory[ 0xBF01 + pX ];
		mTxtY_0 = mMemory[ 0xBF02 + pX ];
		pTxtCurrentID = mMemory[ 0xBF03 + pX ];

	} else {
		gfxDecodeMode = 0;
	}

	screenDraw( gfxDecodeMode, pGfxID, pGfxPosX, pGfxPosY, pTxtCurrentID );
	//57AE
	mMemory[ 0xBF04 + pX ] = ((byte_83F ^ 0xFF) & mMemory[ 0xBF04 + pX]);
	mMemory[ 0xBF03 + pX ] = pGfxID;
	mMemory[ 0xBF01 + pX ] = pGfxPosX;
	mMemory[ 0xBF02 + pX ] = pGfxPosY;
	mMemory[ 0xBF05 + pX ] = mGfxWidth;
	mMemory[ 0xBF06 + pX ] = mGfxHeight;

	mMemory[ 0xBF05 + pX ] <<= 2;
}

// 41D8: In Front Button?
void cCreep::obj_Door_Button_InFront( byte pX, byte pY ) {

	if( mMemory[ 0xBD00 + pX ] )
		return;

	if( mMemory[ 0xBD1D + pX ] == 0 )
		return;

	byte A = mMemory[ 0xBD01 + pX ] + mMemory[ 0xBD0C + pX ];

	A -= mMemory[ 0xBF01 + pY ];
	if( A >= 0x0C )
		return;

	pX =  mMemory[ 0xBD1C + pX ];
	if( mMemory[ 0x780D + pX ] != 0 )
		return;
	
	for(pX = 0;; pX += 0x08 ) {
		if( (mMemory[ 0xBF00 + pX ] ))
			continue;
		
		if( mMemory[ 0xBE00 + pX ] == mMemory[ 0xBE00 + pY ] )
			break;
	}
	//4216
	if( mMemory[ 0xBE01 + pX ] )
		return;

	mMemory[ 0xBF04 + pX ] |= byte_840;
}

// 4647: In Front Forcefield Timer
void cCreep::obj_Forcefield_Timer_InFront( byte pX, byte pY ) {
	if(mMemory[ 0xBD00 + pX ])
		return;

	if(!mMemory[ 0xBD1D + pX ])
		return;

	mMemory[ 0x75AB ] = 0x0C;
	sub_21C8( 0x02 );
	
	mMemory[ 0xBF04 + pY ] |= byte_840;
	mMemory[ 0xBE01 + pY ] = 0x1E;
	mMemory[ 0xBE02 + pY ] = 0x08;
	mMemory[ 0x6889 ] = mMemory[ 0x688A ] = mMemory[ 0x688B ] = 0x55;
	mMemory[ 0x688C ] = mMemory[ 0x688D ] = mMemory[ 0x688E ] = 0x55;
	mMemory[ 0x688F ] = mMemory[ 0x6890 ] = 0x55;

	mTxtX_0 = mMemory[ 0xBF01 + pY ];
	mTxtY_0 = mMemory[ 0xBF02 + pY ];
	screenDraw( 1, 0, 0, 0, mMemory[ 0xBF03 + pY ]);

	mMemory[ 0x4750 + mMemory[ 0xBE00 + pY ] ] = 0;
}

// 4990: In front of key
void cCreep::obj_Key_Infront( byte pX, byte pY ) {

	if( mMemory[ 0xBD00 + pX ] )
		return;

	if( mMemory[ 0x780D + mMemory[ 0xBD1C + pX ] ] != 0 )
		return;
	
	if( mMemory[ 0xBD1D + pX ] == 0 )
		return;

	sub_21C8( 0x0C );
	mMemory[ 0xBF04 + pY ] |= byte_841;

	word_40 = word_4A65 + mMemory[ 0xBE00 + pY ];

	mMemory[ word_40 + 1 ] = 0;
	byte_4A64 = mMemory[ word_40 ];

	if( mMemory[ 0xBD1C + pX ] ) {
		// 49DA
		mMemory[ 0x7835 + mMemory[ 0x7814 ] ] = byte_4A64;
		++mMemory[ 0x7814 ];
		
	} else {
		mMemory[ 0x7815 + mMemory[ 0x7813 ] ] = byte_4A64;
		++mMemory[ 0x7813 ];
	}
}

// 4A68: In Front Lock
void cCreep::obj_Door_Lock_InFront( byte pX, byte pY ) {
	byte byte_4B19 = pX;

	if( mMemory[ 0xBD00 + pX ] )
		return;

	pX = mMemory[ 0xBD1C + pX ];
	if( mMemory[ 0x780D + pX ] != 0 )
		return;

	pX = byte_4B19;
	if(mMemory[ 0xBD1D + pX ] == 0)
		return;

	if( sub_5E8E( mMemory[ 0xBE00 + pY ], pX, pY ) == true )
		return;

	for( pX = 0;; pX += 0x08 ) {
		if( mMemory[ 0xBF00 + pX ] )
			continue;
		if( mMemory[ 0xBE00 + pX ] == mMemory[ 0xBE01 + pY ] )
			break;
	}
	// 4AA2
	if( mMemory[ 0xBE01 + pX ] )
		return;

	mMemory[ 0xBF04 + pX ] |= byte_840;
}

bool cCreep::sub_5E8E( byte pA, byte pX, byte pY ) {
	byte byte_5ED3, byte_5ED4 = pA;
	
	if( mMemory[ 0xBD1C + pX ] != 0 ) {
		byte_5ED3 = mMemory[ 0x7814 ];
		
		word_30 = 0x7835;
	} else {
		// 5EAA
		byte_5ED3 = mMemory[ 0x7813 ];
		word_30 = 0x7815;
	}

	//5EB8
	for( pY = 0;; ++pY ) {
		if( pY == byte_5ED3 )
			return true;

		if( mMemory[ word_30 + pY ] == byte_5ED4 )
			return false;
	}
}

// 4D70: In Front RayGun Control
void cCreep::obj_RayGun_Control_InFront( byte pX, byte pY ) {
	byte byte_4E30 = pY;

	if(mMemory[ 0xBD00 + pX ])
		return;

	byte A = mMemory[ 0xBD01 + pX ] + mMemory[ 0xBD0C + pX ];
	A -= mMemory[ 0xBF01 + pY ];

	if( A >= 8 )
		return;

	if( mMemory[ 0x780D + mMemory[ 0xBD1C + pX ] ] != 0)
		return;

	pY = byte_4E30;

	word_40 = word_4D5B + mMemory[ 0xBE00 + pY ];
	A = 0xFF;
	A ^= byte_4D65;
	A ^= byte_4D66;
	A &= mMemory[ word_40 ];

	pY = mMemory[ 0xBD1E + pX ];
	if( !pY ) 
		A |= byte_4D65;
	else {
		if( pY == 4 )
			A |= byte_4D66;
		else
			if(pY != 0x80 )
				return;
	}

	// 4DC9
	A |= byte_4D62;

	mMemory[ word_40 ] = A;
	if( mMemory[ 0xBD1D + pX ] )
		A = mMemory[ word_40 ] | byte_4D64;
	else
		A = (0xFF ^ byte_4D64) & mMemory[ word_40 ];

	mMemory[ word_40 ] = A;
}


// 4EA8: Teleport?
void cCreep::obj_Teleport_InFront( byte pX, byte pY ) {
	byte byte_50CE, byte_50CF;
	
	if( mMemory[ 0xBF04 + pY ] & byte_840 )
		return;

	if( mMemory[ 0xBD00 + pX ] )
		return;

	// 4EB5
	byte_50CF = pY;
	if( mMemory[ 0x780D + mMemory[ 0xBD1C + pX ] ] != 0 )
		return;

	// 4EC5
	word_40 = readWord( &mMemory[ 0xBE00 + pY ] );
	if(! (mMemory[ 0xBD1D + pX ]) ) {
		// 4ED4
		if( (mMemory[ 0xBD1E + pX ]) )
			return;

		if( byte_2E36 & 0x0F )
			return;
		
		// Change teleport destination
		byte A = mMemory[ word_40 + 2 ];
		A += 1;
		mMemory[ word_40 + 2 ] = A;
		A <<= 1;
		A += 3;
		pY = A;

		if( !(mMemory[ word_40 + pY ]) )
			mMemory[ word_40 + 2 ] = 0;

		// 4EF7
		mMemory[ 0x75DB ] = mMemory[ word_40 + 2 ] + 0x32;

		sub_21C8(5);
		A = mMemory[ word_40 + 2 ] + 2;

		byte_50CE = pX;
		obj_Teleport_unk( A, byte_50CF );
		pX = byte_50CE;
		
		return;
	} else {
		// 4F1A
		// Use Teleport
		pY = byte_50CF;
		mMemory[ 0xBF04 + pY ] |= byte_840;
		mMemory[ 0xBE03 + pY ] = 8;
		
		byte A = mMemory[ word_40 + 2 ] + 0x02;
		// 4F35
		mMemory[ 0xBE02 + pY ] = A;

		A = mMemory[ word_40 + 2 ] << 1;
		A += 0x03;
		pY = A;

		byte A2 = mMemory[ word_40 + pY ];
		++pY;
		//4F44
		mMemory[ 0xBE05 + byte_50CF ] = mMemory[ word_40 + pY ];

		// Set player new X/Y
		pY = byte_50CF;
		mMemory[ 0xBD02 + pX ] = mMemory[ 0xBE05 + pY ] + 0x07;
		mMemory[ 0xBE04 + pY ] = mMemory[ 0xBD01 + pX ] = A2;
	}

}

// 4DE9: 
void cCreep::obj_RayGun_Control_Update( byte pA ) {
	byte byte_4E31 = pA;
	
	mMemory[ 0x6DBF ] = pA;
	mMemory[ 0x6DC0 ] = pA;

	byte gfxPosX = mMemory[ word_40 + 5 ];
	byte gfxPosY = mMemory[ word_40 + 6 ];

	screenDraw( 0, 0x6E, gfxPosX, gfxPosY, 0 );
	mMemory[ 0x6DBF ] = byte_4E31 << 4;
	mMemory[ 0x6DC0 ] = byte_4E31 << 4;

	gfxPosY += 0x10;
	screenDraw( 0, 0x6E, gfxPosX, gfxPosY, 0 );
}

// 505C: 
void cCreep::obj_Teleport_unk( byte pA, byte pX ) {

	byte A =  (pA << 4) | 0x0A;

	mMemory[ 0x6E70 ] = mMemory[ 0x6E71 ] = mMemory[ 0x6E72 ] = A;
	mMemory[ 0x6E73 ] = mMemory[ 0x6E74 ] = mMemory[ 0x6E75 ] = 0x0F;

	word_40 = readWord( &mMemory[ 0xBE00 + pX ] );

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
void cCreep::sub_5171( byte pA ) {
	
	mMemory[ 0x759F ] = pA - 0x48;
	sub_21C8(1);

}

// 526F: 
void cCreep::sub_526F( char &pA ) {
	byte byte_5382;
	word word_5383, word_5385;

	byte_5382 = (byte) pA;

	word_5383 = word_40;
	word_5385 = word_3C;

	word_40 = word_5387 + byte_5382;
	// 529B
	mMemory[ word_40 ] ^= byte_538A;
	byte X;

	for( X = 0 ;;X+=8) {
		if( mMemory[ 0xBF00 + X ] != 0x0B )
			continue;
		if( mMemory[ 0xBE00 + X ] == byte_5382 )
			break;
	}

	//52bd
	mMemory[ 0xBF04 + X ] |= byte_840;
	
	if( !(mMemory[ word_40 ] & byte_538A) ) {
		// 52cf
		mMemory[ 0xBE01 + X ] = 0;
		mMemory[ 0xBE02 + X ] = 0x78;
		mMemory[ 0x6F2E ] = 0xC0;
		mMemory[ 0x6F30 ] = 0x55;

		byte_5FD5 = (mMemory[ word_40 + 1 ] >> 2) - 4;
		byte_5FD6 = mMemory[ word_40 + 2 ] >> 3;

		sub_5FA3();

		mMemory[ word_3C ] |= 0x04;
		mMemory[ word_3C + 4 ] |= 0x40;

	} else {
		// 530F
		mMemory[ 0xBE01 + X ] = 1;
		mMemory[ 0xBE02 + X ] = 0x73;
		mMemory[ 0x6F2E ] = 0x20;
		mMemory[ 0x6F30 ] = 0xCC;
		
		byte_5FD5 = (mMemory[ word_40 + 1 ] >> 2) - 4;
		byte_5FD6 = mMemory[ word_40 + 2 ] >> 3;

		sub_5FA3();

		mMemory[ word_3C ] &= 0xFB;
		mMemory[ word_3C + 4 ] &= 0xBF;
	}

	// 534c
	byte gfxPosX = mMemory[ word_40 + 3 ];
	byte gfxPosY = mMemory[ word_40 + 4 ];

	screenDraw( 0, 0x7A, gfxPosX, gfxPosY, 0 );

	word_40 = word_5383;
	word_3C = word_5385;
}

// 548B: In Front Conveyor
void cCreep::obj_Conveyor_InFront( byte pX, byte pY ) {
	byte byte_564D;
	word_40 = word_564B + mMemory[ 0xBE00 + pY ];

	byte_564D = pY;

	if( !(mMemory[ word_40 ] & byte_5648 ))
		return;
	
	byte A = mMemory[ 0xBD00 + pX ];

	if( !A ) {
		// 54B7
		if( mMemory[ 0xBD03 + pX ] >= 6 )
			return;

	} else
		if( A != 3 && A != 5 ) 
			return;
	
	// 54BE
	A = mMemory[ 0xBD01 + pX ] + mMemory[ 0xBD0C + pX ];
	if( A >= 0 && (A - mMemory[ 0xBF01 + byte_564D ]) < 0 )
		return;

	A -= mMemory[ 0xBF01 + byte_564D ];
	if( A >= 0x20 )
		return;

	if( mMemory[ word_40 ] & byte_5647 )
		A = 0xFF;
	else
		A = 0x01;

	// 54E2
	byte byte_564A = A;
	
	if( mMemory[ 0xBD00 + pX ] == 0 ) {
		A = byte_2E36 & 7;
		if( A )
			goto s54F4;
	}
	
	byte_564A <<= 1;
s54F4:;

	mMemory[ 0xBD01 + pX ] += byte_564A;
}

// 5611: In Front Conveyor Control
void cCreep::obj_Conveyor_Control_InFront( byte pX, byte pY ) {
	if( mMemory[ 0xBD00 + pX ] )
		return;

	if( !mMemory[ 0xBD1D + pX ] )
		return;

	word_40 = word_564B + mMemory[ 0xBE00 + pY ];
	byte A;

	if( mMemory[ 0xBD1C + pX ] )
		A = byte_5645;
	else
		A = byte_5646;

	mMemory[ word_40 ] |= A;
}

bool cCreep::img_FindFree( byte &pX ) {

	if( mImageCount == 0x20 )
		return true;

	pX = mImageCount++;
	pX <<= 3;
	
	char Y = 8;

	while( Y ) {
		mMemory[ 0xBF00 + pX ] = 0;
		mMemory[ 0xBE00 + pX ] = 0;
		++pX;
		--Y;
	}

	pX -= 8;
	mMemory[ 0xBF04 + pX ] = byte_83F;

	return false;
}