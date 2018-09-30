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

#ifndef __RELIABLEBASEBANDSPIPROTOCOL_H
#define __RELIABLEBASEBANDSPIPROTOCOL_H

#include <IOKit/IOReturn.h>

#include "BasebandSPICommand.h"
#include "BasebandSPIProtocol.h"

// Only checking CRC for now

class ReliableBasebandSPIProtocol : public BasebandSPIProtocol
{
	OSDeclareDefaultStructors(ReliableBasebandSPIProtocol);

public:
	virtual void finalizeTxCommand(BasebandSPICommand *txCommand);
	virtual IOReturn handleRxFrame(BasebandSPICommand *rxCommand, bool isFlowControlFrame, bool notifyTx, bool *rxConsumed);

protected:
	virtual IOByteCount getTrailerSize() const;

	virtual void dumpTrailer(int logLevel, void *buffer, int length);

private:
	UInt16 _cachedCRCFlowOn;
	UInt16 _cachedCRCFlowOff;
};

#endif
