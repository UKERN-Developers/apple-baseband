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

#include <string.h>
#include <IOKit/IOLib.h>
#include <sys/param.h>

#include "BasebandSPILogger.h"
#include "BasebandSPIDebug.h"

static const char *kHexDigits = "0123456789ABCDEF";
static const int kBytesPerLine = 16;

void hexdump(const void *buffer, int length)
{
	int i, thisLine, offset, byteOffset;
	unsigned char c;
	char line[80] = "";

	for (i = 0; i < length; i += kBytesPerLine) {
		offset = 0;
		thisLine = length - i;

		if (thisLine > kBytesPerLine) thisLine = kBytesPerLine;

		for (byteOffset = 0; byteOffset < thisLine; byteOffset++) {
			c = ((const unsigned char *)buffer)[i + byteOffset];
			line[offset++] = kHexDigits[c >> 4];
			line[offset++] = kHexDigits[c & 0xf];
			line[offset++] = ' ';
		}

		memset(line + offset, ' ', (kBytesPerLine + 1 - byteOffset) * 3);
		offset += (kBytesPerLine + 1 - byteOffset) * 3;

		for (byteOffset = 0; byteOffset < thisLine; byteOffset++) {
			c = ((const unsigned char *)buffer)[i + byteOffset];
			if (c >= 0x20 && c <= 0x7e) line[offset++] = c;
			else line[offset++] = '.';
		}

		line[offset++] = '\r';
		line[offset++] = '\n';
		line[offset++] = '\0';

		IOLog("%s",line);
	}
}

void _BasebandSPITriggerAssertion( unsigned long left, unsigned long right, const char * leftStr,
	const char * operatorStr, const char * rightStr
#if SPI_ASSERT_LINE_NUMBERS
		, const char * file, unsigned line
#endif	
	  )
{
	char buff[256];

	BasebandSPILogger::report();
	
#if !SPI_ASSERT_LINE_NUMBERS
	unsigned line = 0;
	const char * file = "";
#endif

	snprintf( buff, sizeof( buff ), "BasebandSPI Assertion failure: %s %s %s. Left operand %p, Right operand %p (%s:%d)\n",
		leftStr, operatorStr, rightStr, left, right, file, line );
		
	IOLog( buff );
	PE_enter_debugger( buff );
}

unsigned SPIsnprintf( char * p, unsigned capacity, const char * fmt, ... )
{
	va_list al;
	va_start( al, fmt );
	unsigned ret = vsnprintf( p, capacity, fmt, al );
	va_end( al );
	
	return MIN( ret, capacity );
}

