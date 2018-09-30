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

#include <IOKit/IOMemoryDescriptor.h>

#include "crc16_ccitt.h"

#include "BasebandSPIDebug.h"
#include "ReliableBasebandSPIProtocol.h"

#define BSWAP16(n) (((n & 0xff) << 8) | (n >> 8))

#if SPI_WORD32
#define PADDING    2
#else
#define PADDING    0
#endif

#undef super
#define super BasebandSPIProtocol

OSDefineMetaClassAndStructors(ReliableBasebandSPIProtocol, super);

// Public

void ReliableBasebandSPIProtocol::finalizeTxCommand(BasebandSPICommand *txCommand)
{
	assert(txCommand->_buffer);
	assert(txCommand->_writeOffset <= _dataSize);

	super::finalizeTxCommand(txCommand);

	assert(txCommand->finalized);

	IOByteCount length = _dataSize + getHeaderSize() + PADDING;
	UInt16 crc = crc16_ccitt((const unsigned char *)txCommand->_buffer, length);
	*((UInt16 *)((char *)txCommand->_buffer + length)) = BSWAP16(crc);
}

IOReturn ReliableBasebandSPIProtocol::handleRxFrame(BasebandSPICommand *rxCommand, bool isFlowControlFrame, bool notifyTx, bool *rxConsumed)
{
#if 1
	IOMemoryMap *map = NULL;
	char *buffer;
	if (rxCommand->_buffer) {
		buffer = (char *)rxCommand->_buffer;
	} else {
		// TODO need to unmap when done?
		IOMemoryDescriptor *md = (IOMemoryDescriptor *)rxCommand->getMemoryDescriptor();
		map = md->map();
		buffer = (char *)map->getVirtualAddress();
	}

	// Invalid frame, do not calculate CRC.
	if (*((UInt32 *)buffer) == 0xffffffff) 
	{
		if ( map )
		{
			map->release();
		}
		return kIOReturnSuccess;
	}

	int length = _dataSize + getHeaderSize() + PADDING;
	UInt16 crc = crc16_ccitt((const unsigned char *)buffer, length);
	UInt16 rx_crc = *((UInt16 *)(buffer + length));
	rx_crc = BSWAP16(rx_crc);

	if (map) map->release();

	if (crc != rx_crc) {
		Dbg_IOLog(DEBUG_INFO, "%s: CRC check failed! CRC = %#x, rx CRC = %#x\n", __PRETTY_FUNCTION__, crc, rx_crc);
		dumpFrame(DEBUG_INFO, rxCommand);

		return kIOReturnSuccess;
	} else {
		return super::handleRxFrame(rxCommand, isFlowControlFrame, notifyTx, rxConsumed);
	}
#else
	return super::handleRxFrame(rxCommand, isFlowControlFrame, notifyTx, rxConsumed);
#endif
}

// Protected

inline
IOByteCount ReliableBasebandSPIProtocol::getTrailerSize() const
{
	return sizeof(UInt16) + PADDING;
}

void ReliableBasebandSPIProtocol::dumpTrailer(int logLevel, void *buffer, int length)
{
	if (length < sizeof(UInt16)) return;
	UInt16 crc = *((UInt16 *)((char *)buffer + PADDING));
	Dbg_IOLog(logLevel, "crc: %#x\n", crc);
}
