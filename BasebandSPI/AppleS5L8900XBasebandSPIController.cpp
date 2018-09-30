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
 

#include "AppleS5L8900XBasebandSPIController.h"
#include "BasebandSPIDebug.h"
#include "BasebandSPIConfig.h"
#include "BasebandSPIProtocol.h"

#undef super
#define super AppleS5LFamilyBasebandSPIController

OSDefineMetaClassAndStructors(AppleS5L8900XBasebandSPIController, super);


class AppleS5L8900XBasebandSPIControllerSnapshot: public BasebandSPIControllerSnapshot
{
private:
	UInt32 fSPCLKCON;
	UInt32 fSPCON;
	UInt32 fSPSTA;
	UInt32 fSPCNT;

public:
	AppleS5L8900XBasebandSPIControllerSnapshot( const char * comment, PinState resetDetectState,
	    UInt32 spclkcon,
		UInt32 spcon,
		UInt32 spsta,
		UInt32 spcnt
	   )
		: BasebandSPIControllerSnapshot( comment, resetDetectState ),
		fSPCLKCON( spclkcon ),
		fSPCON( spcon ),
		fSPSTA( spsta ),
		fSPCNT( spcnt )
	{
	}

	virtual ~AppleS5L8900XBasebandSPIControllerSnapshot()
	{
	}
	
	virtual IOByteCount copyAsString( char * dest, IOByteCount capacity ) const
	{
		IOByteCount written = 0;
		written += SPIsnprintf( dest + written, capacity - written, "%s: ", fComment );
		written += SPIsnprintf( dest + written, capacity - written, "RESET_DET: %s, ",
						pinStateAsString( fResetDetectState ) );
		written += SPIsnprintf( dest + written, capacity - written, "SPCLKCON %p, SPCON %p, SPSTA %p, SPCNT %p", 
						fSPCLKCON, fSPCON, fSPSTA, fSPCNT );
						
		if ( fProtocolSnapshot )
		{
			written += fProtocolSnapshot->copyAsString( dest + written, capacity - written );
		}
						
		return written;
	}
};


// Public

bool AppleS5L8900XBasebandSPIController::start(IOService *provider)
{
	SPI_TRACE( DEBUG_INFO );

	bool ret = super::start( provider );
	
	_spiClockPeriod = 0;
	
	if ( !ret )
	{
		SPI_LOG( DEBUG_CRITICAL, "Starting failed\n" );
		return ret;
	}
	
	registerService();

	return true;
}

IOReturn AppleS5L8900XBasebandSPIController::preflightSetupHardware( void )
{
	SPI_TRACE( DEBUG_INFO );
	
	// configure the clock registers
	writeReg32(kS5L8900XSPCON, _spiConReg);
	writeReg32(kS5L8900XSPCLKCON, _spiClkConReg);
	
	return kIOReturnSuccess;
}

IOReturn AppleS5L8900XBasebandSPIController::loadConfiguration( void )
{
	SPI_TRACE( DEBUG_SPEW );
	
	IOReturn ret;
	ret = super::loadConfiguration();
	if ( ret != kIOReturnSuccess )
	{
		return ret;
	}
	
	_spiConReg = kS5L8900XSPCONClkSelNClk | kS5L8900XSPCONClkEnable | kS5L8900XSPCONMasterMode;

	if (fConfiguration.spiClockPolarity) _spiConReg |= kS5L8900XSPCONClkActiveLow;
	if (fConfiguration.spiClockPhase) _spiConReg |= kS5L8900XSPCONClkMissingLast;
	if (!fConfiguration.spiBitFirst) _spiConReg |= kS5L8900XSPCONBitOrderLSB;
	
	UInt32 sclkPeriod = fConfiguration.spiClockPeriod;
	{
		UInt32 period = getOverrideClockPeriod();
		if ( period != 0 )
		{
			sclkPeriod = period;
			SPI_LOG( DEBUG_CRITICAL, "Overriding SCLK to %uns\n", (unsigned)period );
		}
		else
		{
			SPI_LOG( DEBUG_SPEW, "SCLK period %uns\n", (unsigned)sclkPeriod );
		}
	}

	SPI_LOG( DEBUG_SPEW, "polarity %p, phase %p\n", 
		(void *)fConfiguration.spiClockPolarity, (void *)fConfiguration.spiClockPhase );

	switch ( fConfiguration.spiWordSize )
	{
	case 8:
		_spiWordSize = 0;
		break;
	case 16:
		_spiWordSize = 1;
		break;
	case 32:
		_spiWordSize = 2;
		break;
	default:
		SPI_ASSERT( fConfiguration.spiWordSize, ==, 8 ); 
	}
	
	_spiConReg |= _spiWordSize << kS5L8900XSPCONWordSizeShift;
	_spiWordSize = 1 << _spiWordSize;

#if SPI_WORD32
	// Request in the unit of every one buffer. The documentation
	// states one byte however it is wrong.
	// TODO Is it worse to poll the last 4 buffers
	_spiConReg |= kS5L8900XSPCONDMAReqOne | kS5L8900XSPCONDataSwap;
#endif

	// SPI Clock Value = Cell Clock / prescale
	_spiPreReg = (((UInt64)fNCLKFrequency * sclkPeriod) + 1000000000ULL - 1) / 1000000000ULL;
	SPI_LOG( DEBUG_SPEW, "NCLK %p\t Prescaler %p\n", (void *)fNCLKFrequency, (void *)_spiPreReg );

	// SPI Inter-byte Delay = PCLK period * delay cycles
	 _spiIDDReg = (((UInt64)fPCLKFrequency * fConfiguration.spiWordDelay) + 1000000000ULL - 1) / 1000000000ULL;
	 
	 SPI_LOG( DEBUG_SPEW, "SPCON: %p\tWord Size: %p\n", (void *)_spiConReg, (void *)_spiWordSize );
	 SPI_LOG( DEBUG_SPEW, "SPPRE: %p\tSPIDD: %p\n", (void *)_spiPreReg, (void *)_spiIDDReg );
	 
	return kIOReturnSuccess;	
}

IOReturn AppleS5L8900XBasebandSPIController::configureHardware( UInt32 dmaFlags, IOByteCount size )
{
	SPI_TRACE( DEBUG_SPEW );
	
	SPI_LOG( DEBUG_SPEW, "Flags = %p, size = %u\n", (void *)dmaFlags, (unsigned)size );

	enableClock(true);
	// Reset & Clear Interrupts
	_spiClkConReg = kS5L8900XSPCLKCONOn;

	// disable the clock and flush 
	writeReg32(kS5L8900XSPCLKCON, kS5L8900XSPCLKCONFlushTx | kS5L8900XSPCLKCONFlushRx);
	
	// clear the pin control register
	writeReg32(kS5L8900XSPPIN, 0);
	// set the inter-word data delay 
 	writeReg32(kS5L8900XSPIDD, _spiIDDReg);
	
	// clear the count
	writeReg32(kS5L8900XSPCNT, 0);
	
	// set the baud rate prescaler
	writeReg32(kS5L8900XSPPRE, _spiPreReg);

	// clear all interrupts
	writeReg32(kS5L8900XSPSTA, kS5L8900XSPSTAIntAll);
	
	// set SPCON
	writeReg32(kS5L8900XSPCON, _spiConReg );
	
	size = size >> (_spiWordSize >> 1);
	
	// write the count register
	writeReg32(kS5L8900XSPCNT, size);
	
	UInt32 spiConReg = _spiConReg | kS5L8900XSPCONSPIModeDMA;
	if ( dmaFlags & kTxDMA )
	{
		spiConReg |= kS5L8900XSPCONFullDuplex;
	}
	else
	{
		spiConReg |= kS5L8900XSPCONAGDMode;
	}
	// enable DMA and set garbage tx frame mode (if needed)
	writeReg32(kS5L8900XSPCON, spiConReg );
	
	return kIOReturnSuccess;
}
	
IOReturn AppleS5L8900XBasebandSPIController::unconfigureHardware( void )
{
	SPI_TRACE( DEBUG_SPEW );
	
	if ( clockEnabled() )
	{
		writeReg32(kS5L8900XSPCON, _spiConReg);
		writeReg32(kS5L8900XSPCLKCON, 0);
		enableClock(false);
	}
	
	return kIOReturnSuccess;
}

void AppleS5L8900XBasebandSPIController::dumpRegistersInternal( void )
{
	if ( !clockEnabled() )
	{
		SPI_LOG( DEBUG_CRITICAL, "Clock is off\n" );
		return;
	}

	SPI_LOG_PLAIN( DEBUG_SPEW, "Register dump\n" );
	SPI_LOG_PLAIN( DEBUG_SPEW, "\tSPCLKCON: %#8lx\n", readReg32(kS5L8900XSPCLKCON) );
	SPI_LOG_PLAIN( DEBUG_SPEW, "\t   SPCON: %#8lx\n", readReg32(kS5L8900XSPCON) );
	SPI_LOG_PLAIN( DEBUG_SPEW, "\t   SPSTA: %#8lx\n", readReg32(kS5L8900XSPSTA) );
	SPI_LOG_PLAIN( DEBUG_SPEW, "\t   SPIDD: %#8lx\n", readReg32(kS5L8900XSPIDD) );
	SPI_LOG_PLAIN( DEBUG_SPEW, "\t   SPPRE: %#8lx\n", readReg32(kS5L8900XSPPRE) );
	SPI_LOG_PLAIN( DEBUG_SPEW, "\t   SPCNT: %#8lx\n", readReg32(kS5L8900XSPCNT) );
}

IOReturn AppleS5L8900XBasebandSPIController::takeSnapshot( const char * comment, 
	BasebandSPIControllerSnapshot::PinState resetDetectState, BasebandSPIControllerSnapshot ** snapshot )
{	
	bool wasClockEnabled = clockEnabled();
	// Enable the SPI clock temporarily for snapshot, if not already enabled
	if (!wasClockEnabled)
	{
		enableClock(true);
	}

	*snapshot = new AppleS5L8900XBasebandSPIControllerSnapshot( comment,
						resetDetectState,
						readReg32(kS5L8900XSPCLKCON), 
						readReg32(kS5L8900XSPCON), 
						readReg32(kS5L8900XSPSTA),
						readReg32(kS5L8900XSPCNT) );
						
	// Disable SPI clock temporarily for snapshot, if enabled just for duration of snapshot
	if (!wasClockEnabled)
	{
		enableClock(false);
	}
	
	return kIOReturnSuccess;
}

IOReturn AppleS5L8900XBasebandSPIController::startHardware( void )
{
	SPI_TRACE( DEBUG_SPEW );
	
	if ( clockEnabled() )
	{
		writeReg32( kS5L8900XSPCLKCON, _spiClkConReg );
	}
	else
	{
		SPI_LOG( DEBUG_CRITICAL, "AppleS5L8900XBasebandSPIController::startHardware: clock not enabled\n" );	
	}
	
	return kIOReturnSuccess;
}

bool AppleS5L8900XBasebandSPIController::validateDeviceTypeAndChip( const char * deviceType, UInt32 chipRevision ) const
{
	(void)deviceType;
	(void)chipRevision;
	
	return true;
}
