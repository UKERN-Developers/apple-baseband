/*
 *  RegisterAccess.cpp
 *  RegisterAccessor
 *
 *  Created by Arjuna Sivasithambaresan on 2/15/08.
 *  Copyright 2008 Apple Inc. All rights reserved.
 *
 */

#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>

#include "RegisterAccessorLib.h"

int main( int argc, const char * argv[] )
{
	RAInterface interface;
	
	interface.connect();

	
	int ret;

	switch ( argc-1 )
	{
	case 1:
		// read
		{
			int reg = strtoul( argv[1], NULL, 16 );
			ret = printf( "0x%08X", interface.read( reg ) );
		}
		break;
		
	case 2:
		// write
		{
			interface.write( strtoul( argv[1], NULL, 16 ), strtoul( argv[2], NULL, 16 ) );
		}
		break;
		
	case 3:
		// masked write
		{
			interface.writeMasked( strtoul( argv[1], NULL, 16 ),
				strtoul( argv[2], NULL, 16 ),
				strtoul( argv[3], NULL, 16 ) );
		}
		break;
	default:
		{
			printf( "Usage %s [parameters]\n", argv[0] );
			printf( "\t Parameters can be:\n" );
			printf( "\t <address>                    : Do a register read\n" );
			printf( "\t <address> <value>            : Do a register write\n" );
			printf( "\t <address> <value> <mask>     : Do a register masked write\n" );
		}
	}
	
	interface.disconnect();
	
			
	return 0;

}
