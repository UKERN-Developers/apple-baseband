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

#ifndef __BASEBANDSPIDEBUG_H
#define __BASEBANDSPIDEBUG_H

#include <IOKit/IOLib.h>

#include "BasebandSPILogger.h"

#define SPI_ASSERT_GPIO_ON_ERROR	0

#define SPI_SAVE_TRANSFERS_AMOUNTS 	0

#define TDEBUG                0
#define SPI_WORD32            1
#define GPIO_MEASURE          0
#define RELIABLE_DEBUG        0

#define DEBUG_CRITICAL        0
#define DEBUG_INFO            1
#define DEBUG_SPEW            2
#define DEBUG_FRAME           3
#define DEBUG_PROTOCOL		  2


#ifdef RELEASE
	#ifndef DEBUG_LEVEL
		#define DEBUG_LEVEL DEBUG_CRITICAL
	#endif
#else
	#ifndef DEBUG_LEVEL
		#define DEBUG_LEVEL DEBUG_SPEW
//		#define DEBUG_LEVEL DEBUG_INFO
	#endif
#endif

#ifdef ASSERT
	#define assert(ex) { \
		if (!(ex)) { \
			IOLog(__FILE__ ":%d :" __LINE__); \
			Debugger("assert(" #ex ") failed"); \
		} \
	}
#define _KERN_ASSERT_H_
#endif

#define SPI_ASSERT_LINE_NUMBERS		1


#if !SPI_ASSERT_LINE_NUMBERS
#define SPI_ASSERT( left, operator, right ) \
	if ( ! ( ( left ) operator ( right ) ) ) \
	{\
		_BasebandSPITriggerAssertion( (unsigned long)( left ), (unsigned long)( right ), # left, # operator, #right ); \
	}
#else
#define SPI_ASSERT( left, operator, right ) \
	if ( ! ( ( left ) operator ( right ) ) ) \
	{\
		_BasebandSPITriggerAssertion( (unsigned long)( left ), (unsigned long)( right ), # left, # operator, #right,  __FILE__, __LINE__ ); \
	}
#endif

#define SPI_ASSERT_SUCCESS( ret )  SPI_ASSERT( ret, ==, kIOReturnSuccess )
	
#define SPI_TRACE( level ) \
	BasebandSPITracer __tr( level, getDebugName(), __FUNCTION__ )

#define SPI_LOG_CONDITION(level) \
	((level) <= DEBUG_LEVEL	)
	
#define SPI_LOG(level, x... ) \
	do { if ((level) <= DEBUG_LEVEL) { IOLog( "%s::%s: ", getDebugName(), __FUNCTION__ ); IOLog(x); } } while(0)
	
#define SPI_LOG_STATIC(level, x... ) \
	do { if ((level) <= DEBUG_LEVEL) { IOLog( "%s::%s: ", getDebugNameStatic(), __FUNCTION__ ); IOLog(x); } } while(0)

#define SPI_LOG_PLAIN Dbg_IOLog

#define Dbg_IOLog(level, x...) \
	do { if ((level) <= DEBUG_LEVEL) { IOLog(x); } } while(0)

#define Dbg_hexdump(level, ptr, len) \
	do {if ((level) <= DEBUG_LEVEL) { hexdump(ptr, len); } } while(0)

void hexdump(const void *buffer, int length);

unsigned SPIsnprintf( char * p, unsigned capacity, const char * fmt, ... );

void _BasebandSPITriggerAssertion( unsigned long left, unsigned long right, const char * leftStr,
	const char * operatorStr, const char * rightStr
#if SPI_ASSERT_LINE_NUMBERS
		, const char * file, unsigned line
#endif	
	  );


class BasebandSPITracer
{
public:
	const char * fClass;
	const char * fFunction;
	unsigned fLevel;
	
public:
	BasebandSPITracer( unsigned level, const char * cls, const char * function )
		: fClass( cls ), fFunction( function ), fLevel( level )
	{
		SPI_LOG_PLAIN( fLevel, "%s::%s: Enter\n", fClass, fFunction );
	}
	
	~BasebandSPITracer()
	{
		SPI_LOG_PLAIN( fLevel, "%s::%s: Exit\n", fClass, fFunction );
	}
};

#endif
