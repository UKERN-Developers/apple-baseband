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

#ifndef __APPLES5L8900XBASEBANDSPICONTROLLER_H
#define __APPLES5L8900XBASEBANDSPICONTROLLER_H


#define	kS5L8900XSPCLKCON			(0x000)
#define		kS5L8900XSPCLKCONFlushRx	(1 << 3)
#define		kS5L8900XSPCLKCONFlushTx	(1 << 2)
#define		kS5L8900XSPCLKCONReady		(1 << 1)
#define		kS5L8900XSPCLKCONOn		(1 << 0)
#define	kS5L8900XSPCON				(0x004)
#define     kS5L8900XSPCONDataSwap    (1 << 16)
#define     kS5L8900XSPCONDMAReqOne   (1 << 15)
#define		kS5L8900XSPCONWordSizeShift	(13)
#define		kS5L8900XSPCONClkSelNClk	(1 << 12)
#define		kS5L8900XSPCONBitOrderLSB	(1 << 11)
#define		kS5L8900XSPCONTransReady	(1 << 8)
#define		kS5L8900XSPCONRxDone		(1 << 7)
#define		kS5L8900XSPCONSPIModePoll	(0 << 5)
#define		kS5L8900XSPCONSPIModeInt	(1 << 5)
#define		kS5L8900XSPCONSPIModeDMA	(2 << 5)
#define		kS5L8900XSPCONClkEnable		(1 << 4)
#define		kS5L8900XSPCONMasterMode	(1 << 3)
#define		kS5L8900XSPCONSlaveMode		(0 << 3)
#define		kS5L8900XSPCONClkActiveLow	(1 << 2)
#define		kS5L8900XSPCONClkMissingLast	(1 << 1)
#define		kS5L8900XSPCONFullDuplex	(0 << 0)
#define		kS5L8900XSPCONAGDMode		(1 << 0)
#define	kS5L8900XSPSTA				(0x008)
#define		kS5L8900XSPSTARxFIFOShift	(8)
#define		kS5L8900XSPSTATxFIFOShift	(4)
#define		kS5L8900XSPSTAFIFOMask		(0x00F)
#define		kS5L8900XSPSTADCError		(1 << 3)
#define		kS5L8900XSPSTAMMError		(1 << 2)
#define		kS5L8900XSPSTATransReady	(1 << 1)
#define		kS5L8900XSPSTARxDone		(1 << 0)
#define		kS5L8900XSPSTAIntAll		(0x00F)
#define	kS5L8900XSPPIN				(0x00C)
#define	kS5L8900XSPTDAT				(0x010)
#define	kS5L8900XSPRDAT				(0x020)
#define	kS5L8900XSPPRE				(0x030)
#define	kS5L8900XSPCNT				(0x034)
#define	kS5L8900XSPIDD				(0x038)

#define S5L8900XSPITxFIFOLevel(x)		(((x) >> kS5L8900XSPSTATxFIFOShift) & kS5L8900XSPSTAFIFOMask)
#define S5L8900XSPIRxFIFOLevel(x)		(((x) >> kS5L8900XSPSTARxFIFOShift) & kS5L8900XSPSTAFIFOMask)

#include "AppleS5LFamilyBasebandSPIController.h"

class AppleS5L8900XBasebandSPIController : public AppleS5LFamilyBasebandSPIController
{
	OSDeclareDefaultStructors(AppleS5L8900XBasebandSPIController);
	
public:
	virtual bool start(IOService *provider);
	
protected:
	virtual bool validateDeviceTypeAndChip( const char * deviceType, UInt32 chipRevision ) const;
	virtual IOReturn loadConfiguration( void );

protected:
	// controller state manipulation
	virtual IOReturn preflightSetupHardware( void );
	virtual IOReturn configureHardware( UInt32 dmaFlags, IOByteCount size );
	virtual IOReturn unconfigureHardware( void );
	virtual IOReturn startHardware( void );

	
protected:
	// debugging	
	const char * getDebugName( void ) const
	{
		return "AppleS5L8900XBasebandSPIController";
	}
	
	virtual IOReturn takeSnapshot( const char * comment, 
		BasebandSPIControllerSnapshot::PinState resetDetectState, BasebandSPIControllerSnapshot ** snapshot );

private:
	virtual void dumpRegistersInternal( void );

	UInt32 _spiWordTime;
	UInt32 _spiWordSize;
	UInt32 _spiClockPeriod;
		
	// register shadows
	UInt32 _spiClkConReg;
	UInt32 _spiConReg;
	UInt32 _spiPreReg;
	UInt32 _spiIDDReg;
	
};

#endif
