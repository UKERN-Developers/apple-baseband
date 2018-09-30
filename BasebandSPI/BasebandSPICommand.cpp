/*
 * Copyright (c) 2007 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 *
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

#include "BasebandSPICommand.h"
#include "BasebandSPIDebug.h"
#include "BasebandSPIConfig.h"

#undef super
#define super IODMACommand

OSDefineMetaClassAndAbstractStructors(BasebandSPICommand, super);

IOReturn BasebandSPICommand::init( IODirection direction )
{
	if ( !super::initWithSpecification(kIODMACommandOutputHost32, 32, 0) )
	{
		SPI_LOG( DEBUG_CRITICAL, "Failed initialising DMA command\n" );
		return kIOReturnError;
	}
	
	if ( direction != kIODirectionIn && direction != kIODirectionOut )
	{
		SPI_LOG( DEBUG_CRITICAL, "direction must be kIODirectionIn or kIODirectionOut\n" );
		return kIOReturnError;
	}
	
	fCookie			= NULL;
	fDirection		= direction;
	fBuffer			= NULL;
	fIsFinalized	= false;
	fNext			= NULL;
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPICommand::setMemoryDescriptor( IOMemoryDescriptor * md, UInt8 * virtualAddress, void * cookie )
{
	IOReturn ret = super::setMemoryDescriptor( md, false );
	if ( ret != kIOReturnSuccess )
	{
		return ret;
	}
	
	fMemoryDescriptor = md;
	fBuffer		= virtualAddress;
	fCookie		= cookie;
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPICommand::clearMemoryDescriptor( void )
{
	fBuffer				= NULL;
	fMemoryDescriptor	= NULL;
	
	return super::clearMemoryDescriptor( false );
}

void BasebandSPICommand::free( void )
{
	if ( fMemoryDescriptor )
	{
		super::setMemoryDescriptor( NULL, false );
		fMemoryDescriptor->release();
		fMemoryDescriptor = NULL;
	}
	
	super::free();
}