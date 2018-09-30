/*
 *  PowerStateCycler.cpp
 *  RegisterAccessor
 *
 *  Created by Arjuna Sivasithambaresan on 2/15/08.
 *  Copyright 2008 Apple Inc. All rights reserved.
 *
 */
 
#include "RegisterAccessorLib.h"

#include <stdio.h>
#include <unistd.h>

static RAInterface * gInterface = NULL;

static void configureGPIO( unsigned value )
{
	// Port 6, Pin 7, Function 1 is GPIO3 - WLAN_RESET
	static const unsigned kPort = 6;
	static const unsigned kPin = 7;
	static const unsigned kFunction = 1;
	
	static const unsigned kPDAT0 = 0x3E400000;
	static const unsigned kFSEL  = 0x3E400320;
	
	value &= 1;
	
	if ( kFunction == 1 )
	{
		value	= ( kPort << 16 )
						| ( kPin << 8 )
						| 0xE 
						| value; 
						
		gInterface->write( kFSEL, value ); 
	}
	
	if ( kFunction == 1 )
	{
		value	= ( kPort << 16 )
						| ( kPin << 8 )
						| kFunction; 
		gInterface->write( kFSEL, value ); 
		gInterface->writeMasked( kPDAT0 + 0x20*kPort, value < kPin, 1UL << kPin );
	}
};

static void configureHCLK( unsigned state )
{
	static const unsigned kPLL = 1;
	static const unsigned kPLL0Reg = 0x3C500020;
	static const unsigned kCLKCON1 = 0x3C500004;
	static const unsigned kNumStates = 4;
	
	static const unsigned kParamTable[kNumStates][3]= 
	{
		{ 0x67, 0, 2 },
		{ 0x67, 1, 2 },
		{ 0x67, 2, 1 },
		{ 0x64, 3, 2 }
	};
	
	if ( state >= kNumStates )
	{
		throw RAInterface::Exception( "Invalid state %d\n",  state );
	}
	
	unsigned m;
	unsigned s;
	unsigned div;
	
	m = kParamTable[state][0];
	s = kParamTable[state][1];
	div = kParamTable[state][2];
	
	gInterface->writeMasked( kCLKCON1, ( div | ( div << 2 ) ) << 20, 0xF << 20 );
	gInterface->writeMasked( kPLL0Reg + 4*kPLL, ( m << 8 ) | s, ( 0xFF << 8 ) | 0x7 );
}

int main( int argc, const char * argv[] )
{
	if ( argc < 3 )
	{
		fprintf( stderr, "You must provide at least one state\n" );
		exit( 0 );
	}
	
	unsigned numStates = argc-1;
	
	if ( numStates & 1 )
	{
		fprintf( stderr, "Every state must have a delay\n" );
		exit( 1 );
	}
	
	numStates /= 2;
	
	unsigned * states = new unsigned[numStates];
	unsigned * delays = new unsigned[numStates];
	
	fprintf( stderr, "Using sequence with %d states:\n\t", numStates );
	
	for( unsigned i = 0; i < numStates; i++ )
	{
		states[i] = strtoul( argv[2*i + 1], NULL, 10 );
		delays[i] = strtoul( argv[2*i + 1 + 1], NULL, 10 )*1000;
		
		fprintf( stderr, "%d-%d", states[i], delays[i] );
		
		if ( i < numStates-1 )
		{
			fprintf( stderr, ", " );
		}
		else
		{
			fprintf( stderr, "\n" );
		}
	}

	gInterface = new RAInterface();
	
	gInterface->connect();
	
	gInterface->write( 0x38100008, 0x1081 );
	gInterface->write( 0x38100404, 0xB );
	
	{
		unsigned i = 0;

		while ( 1 )
		{
			printf( "state %d\n", states[i] );
			configureGPIO( i & 1 );
			configureHCLK( states[i] );
			configureGPIO( 0 );
			
			usleep( delays[i] );
				
			i++;
			if ( i == numStates )
			{
				i = 0;
			}
		}
	}
	
	gInterface->disconnect();
	
	delete gInterface;
	
	delete []states;
	delete []delays;

}

