/*
 *  RegisterAccessorLib.cpp
 *  RegisterAccessor
 *
 *  Created by Arjuna Sivasithambaresan on 2/15/08.
 *  Copyright 2008 Apple Inc. All rights reserved.
 *
 */

#include "RegisterAccessorLib.h"

#include <sys/fcntl.h>
#include <errno.h>

const std::string  RAInterface::kDevicePath = "/dev/RegisterAccess";


RAInterface::Exception::Exception( const char * fmt, ... )
	: std::exception()
{
	va_list args;
	va_start( args, fmt );
	vsnprintf( fMessage, sizeof( fMessage ), fmt, args );
	va_end( args );
}

const char * RAInterface::Exception::what( void ) const throw()
{
	return fMessage;
}

RAInterface::RAInterface()
	: fFD( -1 )
{
}

RAInterface::~RAInterface()
{
	if( fFD != -1 )
	{
		disconnect();
	}
}

void RAInterface::connect( void )
{
	if ( fFD != -1 )
	{
		throw Exception( "FD is %d\n", fFD );
	}
	
	fFD = ::open( kDevicePath.c_str(), O_RDWR );
	if ( fFD < 0 )
	{
		throw Exception( "Opening %s failed\n", kDevicePath.c_str() );
	}
}

void RAInterface::disconnect( void )
{
	if ( fFD == -1 )
	{
		throw Exception( "Cannot disconnect when not open, FD is %d\n", fFD );
	}
	
	
	::close( fFD );
	
	fFD = -1;
}

void RAInterface::ioctl( unsigned what, void * data ) const
{
	int ret = ::ioctl( fFD, what, data );
	
	if ( ret != 0 )
	{
		int err = errno;
		throw Exception( "ioctl failed with ret %d, errno %d\n", ret, err );
	}
}

