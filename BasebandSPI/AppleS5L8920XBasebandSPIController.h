/*
 *  AppleS5L8920XBasebandSPIController.h
 *  BasebandSPI
 *
 *  Created by Arjuna Sivasithambaresan on 6/16/08.
 *  Copyright 2008 Apple Inc. All rights reserved.
 *
 */
 
#ifndef __APPLES5L8920XBASEBANDSPICONTROLLER_H__
#define __APPLES5L8920XBASEBANDSPICONTROLLER_H__
 
#define	kS5L8920XSPCLKCON			(0x000)
#define		kS5L8920XSPCLKCONFlushRx	(1 << 3)
#define		kS5L8920XSPCLKCONFlushTx	(1 << 2)
#define		kS5L8920XSPCLKCONEnable		(1 << 0)
// ------------
#define	kS5L8920XSPCON				(0x004)
#define		kS5L8920XSPCONDelaySelectShift		(26)
#define		kS5L8920XSPCONTxDone				(1 << 21)
#define		kS5L8920XSPCONRxDataCaptureDelay	(1 << 20)
#define     kS5L8920XSPCONDataSwap				(1 << 19)
#define     kS5L8920XSPCONDMAReqOne				(2 << 17)
#define		kS5L8920XSPCONWordSizeShift			(15)
#define		kS5L8920XSPCONClkSelNClk			(1 << 14)
#define		kS5L8920XSPCONBitOrderLSB			(1 << 13)
#define		kS5L8920XSPCONRxDataLeftOverTimeout (1 << 12)
#define		kS5L8920XSPCONSPIHangError			(1 << 11)
#define		kS5L8920XSPCONDataCollisionError	(1 << 10)
#define		kS5L8920XSPCONMultiMasterError		(1 << 9)
#define		kS5L8920XSPCONTransReady			(1 << 8)
#define		kS5L8920XSPCONRxDone				(1 << 7)

#define		kS5L8920XSPCONSPIModePoll			(0 << 5)
#define		kS5L8920XSPCONSPIModeInt			(1 << 5)
#define		kS5L8920XSPCONSPIModeDMA			(2 << 5)

#define		kS5L8920XSPCONClkEnable				(1 << 4)

#define		kS5L8920XSPCONMasterMode			(1 << 3)
#define		kS5L8920XSPCONSlaveMode				(0 << 3)

#define		kS5L8920XSPCONClkActiveLow			(1 << 2)
#define		kS5L8920XSPCONClkActiveHigh			(0 << 2)

#define		kS5L8920XSPCONClkMissingLast		(1 << 1)
#define		kS5L8920XSPCONClkMissingFirst		(0 << 1)

#define		kS5L8920XSPCONFullDuplexMode		(0 << 0)
#define		kS5L8920XSPCONRxOnlyMode			(1 << 0)
// ------------
#define	kS5L8920XSPSTA				(0x008)
#define		kS5L8920XSPSTARxFIFOShift			(11)
#define		kS5L8920XSPSTATxFIFOShift			(6)
#define		kS5L8920XSPSTAFIFOMask				(kS5L8920XSPSTAIntAll)

#define		kS5L8920XSPSTARxDataLeftoverTimeout	(1 << 5)
#define		kS5L8920XSPSTASPIHangError			(1 << 4)
#define		kS5L8920XSPSTADCError				(1 << 3)
#define		kS5L8920XSPSTAMMError				(1 << 2)
#define		kS5L8920XSPSTATransReady			(1 << 1)
#define		kS5L8920XSPSTARxDone				(1 << 0)

#define		kS5L8920XSPSTAIntAll				((1 << 5) - 1)
// ------------
#define	kS5L8920XSPPIN				(0x00C)
#define	kS5L8920XSPTDAT				(0x010)
#define	kS5L8920XSPRDAT				(0x020)
#define	kS5L8920XSPPRE				(0x030)
#define	kS5L8920XSPCNT				(0x034)
#define	kS5L8920XSPIDD				(0x038)
#define kS5L8920XSPIRTO				(0x03C)
#define kS5L8920XSPIHANGD			(0x040)
#define kS5L8920XSPIRST				(0x044)
#define kS5L8920XSPIVER				(0x048)
#define kS5L8920XSPTDCNT			(0x04C)
// ------------
#define S5L8920XSPITxFIFOLevel(x)		(((x) >> kS5L8920XSPSTATxFIFOShift) & kS5L8920XSPSTAFIFOMask)
#define S5L8920XSPIRxFIFOLevel(x)		(((x) >> kS5L8920XSPSTARxFIFOShift) & kS5L8920XSPSTAFIFOMask)

#include "AppleS5LFamilyBasebandSPIController.h"

class AppleS5L8920XBasebandSPIController: public AppleS5LFamilyBasebandSPIController
{
	OSDeclareDefaultStructors(AppleS5L8920XBasebandSPIController);

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
		return "AppleS5L8920XBasebandSPIController";
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
