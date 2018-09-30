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

#ifndef __BASEBANDSPICOMMAND_H
#define __BASEBANDSPICOMMAND_H

#include <IOKit/IODMACommand.h>

#include "BasebandSPIDevice.h"
#include "BasebandSPILogger.h"
#include "BasebandSPIDebug.h"

class BasebandSPICommand : public IODMACommand
{
	OSDeclareAbstractStructors(BasebandSPICommand);
	
	friend class BasebandSPILogger;

public:
	// interface for memory descriptor related operations
	IOReturn setMemoryDescriptor( IOMemoryDescriptor * md, UInt8 * buffer, void * cookie );
	virtual IOReturn clearMemoryDescriptor( void );
	
public:
	// interface for writing, reading, etc.
	
	/// Writes some data into this command
	/// @param source The buffer representing the source of the data to write
	/// @param amount The amount of data to write from the buffer
	/// @param dataSize The maximum data that can be written to this command (in total,
	///                 including what's already written)
	/// @returns the amount written
	virtual IOByteCount writeData( UInt8 * source, IOByteCount amount, IOByteCount dataSize ) = 0;
		
public:
	// state management
	virtual void reset( void ) = 0;
			
	void * getCookie( void ) const
	{
		return fCookie;
	}
	
	bool isFinalized( void ) const
	{
		return fIsFinalized;
	}
	
	void setNext( BasebandSPICommand * command )
	{
		fNext = command;
	};
	
	BasebandSPICommand * getNext( void ) const
	{
		return fNext;
	};

protected:
	// debugging
	virtual const char * getDebugName( void ) const = 0;
	virtual void dumpFrameInternal( int logLevel ) const = 0;
	
public:
	void dumpFrame( int level ) const
	{
		if ( level <= DEBUG_LEVEL )
		{
			dumpFrameInternal( level );
		}
	}
	
protected:	
	virtual IOReturn init( IODirection direction );
	virtual void free( void );
	
protected:
	void setFinalized( void )
	{
		fIsFinalized = true;
	};
	
	void clearFinalized( void )
	{
		fIsFinalized = false;
	};
	
protected:
	void			* fCookie;
	IODirection		fDirection;
	UInt8			* fBuffer;
	bool			fIsFinalized;
	
	// lame that I need to keep a copy, but the one in
	// IODMACommand is const
	IOMemoryDescriptor * fMemoryDescriptor;
	
	BasebandSPICommand * fNext;
};

#endif
