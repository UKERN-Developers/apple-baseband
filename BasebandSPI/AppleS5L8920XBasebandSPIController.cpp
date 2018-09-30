/*
 *  AppleS5L8920XBasebandSPIController.cpp
 *  BasebandSPI
 *
 *  Created by Arjuna Sivasithambaresan on 6/16/08.
 *  Copyright 2008 Apple Inc. All rights reserved.
 *
 */

#include "AppleS5L8920XBasebandSPIController.h"

#include <IOKit/platform/AppleARMIODevice.h>

#include "BasebandSPIDebug.h"
#include "BasebandSPIConfig.h"
#include "BasebandSPIProtocol.h"

#undef super
#define super AppleS5LFamilyBasebandSPIController

OSDefineMetaClassAndStructors(AppleS5L8920XBasebandSPIController, super);

class AppleS5L8920XBasebandSPIControllerSnapshot: public BasebandSPIControllerSnapshot
{
private:
	UInt32 fSPCLKCON;
	UInt32 fSPCON;
	UInt32 fSPSTA;
	UInt32 fSPCNT;
	UInt32 fSPTDCNT;

public:
	AppleS5L8920XBasebandSPIControllerSnapshot( const char * comment, PinState resetDetectState,
	    UInt32 spclkcon,
		UInt32 spcon,
		UInt32 spsta,
		UInt32 spcnt,
		UInt32 sptdcnt
	   )
		: BasebandSPIControllerSnapshot( comment, resetDetectState ),
		fSPCLKCON( spclkcon ),
		fSPCON( spcon ),
		fSPSTA( spsta ),
		fSPCNT( spcnt ),
		fSPTDCNT( sptdcnt )
	{
	}

	virtual ~AppleS5L8920XBasebandSPIControllerSnapshot()
	{
	}
	
	virtual IOByteCount copyAsString( char * dest, IOByteCount capacity ) const
	{
		IOByteCount written = 0;
		written += SPIsnprintf( dest + written, capacity - written, "%s: ", fComment );
		written += SPIsnprintf( dest + written, capacity - written, "RESET_DET: %s, ",
						pinStateAsString( fResetDetectState ) );
		written += SPIsnprintf( dest + written, capacity - written, "SPCLKCON %p, SPCON %p, SPSTA %p, SPCNT %p, SPTDCNT %p", 
						fSPCLKCON, fSPCON, fSPSTA, fSPCNT, fSPTDCNT );
						
		if ( fProtocolSnapshot )
		{
			written += fProtocolSnapshot->copyAsString( dest + written, capacity - written );
		}
						
		return written;
	}
};

// Public

bool AppleS5L8920XBasebandSPIController::start(IOService *provider)
{
	SPI_TRACE( DEBUG_INFO );
	
	bool ret = super::start( provider );
	
	if ( !ret )
	{
		return ret;
	}

	_spiClockPeriod = 0;
	
	registerService();
	
	return true;
}

IOReturn AppleS5L8920XBasebandSPIController::preflightSetupHardware( void )
{
	SPI_TRACE( DEBUG_CRITICAL );

	// configure the clock registers
	writeReg32(kS5L8920XSPCON, _spiConReg);
	writeReg32(kS5L8920XSPCLKCON, _spiClkConReg);
	
	return kIOReturnSuccess;
}

IOReturn AppleS5L8920XBasebandSPIController::loadConfiguration( void )
{
	IOReturn ret;
	ret = super::loadConfiguration();
	if ( ret != kIOReturnSuccess )
	{
		return ret;
	}

	_spiConReg = kS5L8920XSPCONClkSelNClk | kS5L8920XSPCONClkEnable | kS5L8920XSPCONMasterMode;

	if (fConfiguration.spiClockPolarity) _spiConReg |= kS5L8920XSPCONClkActiveLow;
	if (fConfiguration.spiClockPhase) _spiConReg |= kS5L8920XSPCONClkMissingLast;
	if (!fConfiguration.spiBitFirst) _spiConReg |= kS5L8920XSPCONBitOrderLSB;
	
	UInt32 sclkPeriod = fConfiguration.spiClockPeriod;
	{
		UInt32 period = getOverrideClockPeriod();
		if ( period != 0 )
		{
			sclkPeriod = period;
			SPI_LOG( DEBUG_CRITICAL, "Overriding SCLK to %uns\n", period );
		}
	}
	
	SPI_LOG( DEBUG_SPEW, "polarity %p, phase %p\n", fConfiguration.spiClockPolarity, fConfiguration.spiClockPhase );
	
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

	_spiConReg |= _spiWordSize << kS5L8920XSPCONWordSizeShift;
	_spiWordSize = 1 << _spiWordSize;

#if SPI_WORD32
	// Request in the unit of every one buffer. The documentation
	// states one byte however it is wrong.
	// TODO Is it worse to poll the last 4 buffers
	_spiConReg |= kS5L8920XSPCONDMAReqOne;
#endif

	if ( !fConfiguration.spiBigEndian )
	{
		_spiConReg |= kS5L8920XSPCONDataSwap;
	}

	// SPI Clock Value = Cell Clock / prescale
	UInt64 tmp = (((UInt64)fNCLKFrequency * (UInt64)sclkPeriod) + 1000000000ULL - 1) / 1000000000ULL;
	
	_spiPreReg = tmp;
	
	SPI_LOG( DEBUG_CRITICAL, "NCLK Frequency %u, Prescaler %u\n", (unsigned)fNCLKFrequency, _spiPreReg );

	// SPI Inter-byte Delay = PCLK period * delay cycles
	 _spiIDDReg = (((UInt64)fPCLKFrequency * fConfiguration.spiWordDelay) + 1000000000ULL - 1) / 1000000000ULL;
	 
	 SPI_LOG( DEBUG_INFO, "SPCON: %p\tWord Size: %p\n", (void *)_spiConReg, (void *)_spiWordSize );
	 SPI_LOG( DEBUG_INFO, "SPPRE: %p\tSPIDD: %p\n", (void *)_spiPreReg, (void *)_spiIDDReg );
	 
	 return kIOReturnSuccess;
}

IOReturn AppleS5L8920XBasebandSPIController::configureHardware( UInt32 dmaFlags, IOByteCount size )
{
	enableClock(true);        
	// Reset & Clear Interrupts
	_spiClkConReg = kS5L8920XSPCLKCONEnable;

	// disable the clock
	writeReg32(kS5L8920XSPCLKCON, kS5L8920XSPCLKCONFlushTx | kS5L8920XSPCLKCONFlushRx);
	writeReg32(kS5L8920XSPPIN, 0);
	
	// Configure controller mode
	writeReg32(kS5L8920XSPCNT, 0);
	writeReg32(kS5L8920XSPTDCNT, 0);
	
	writeReg32(kS5L8920XSPPRE, _spiPreReg);
	writeReg32(kS5L8920XSPIDD, _spiIDDReg);
	writeReg32(kS5L8920XSPIRTO, 0xffffffff);
	writeReg32(kS5L8920XSPIHANGD, 0);
	writeReg32(kS5L8920XSPSTA, kS5L8920XSPSTAIntAll);
	
	size = size >> (_spiWordSize >> 1);
	writeReg32( kS5L8920XSPCNT, size );
		
	UInt32 spiConReg = _spiConReg | kS5L8920XSPCONSPIModeDMA;
	if ( dmaFlags & kTxDMA )
	{
		writeReg32( kS5L8920XSPTDCNT, size );
		spiConReg |= kS5L8920XSPCONFullDuplexMode;
	}
	else
	{
		spiConReg |= kS5L8920XSPCONRxOnlyMode;
	}
	
	// set the SPI controller register
	writeReg32(kS5L8920XSPCON, spiConReg);
	
	return kIOReturnSuccess;
}

IOReturn AppleS5L8920XBasebandSPIController::unconfigureHardware( void )
{
	SPI_TRACE( DEBUG_SPEW );

	if ( clockEnabled() )
	{
		writeReg32(kS5L8920XSPCON, _spiConReg);
		writeReg32(kS5L8920XSPCLKCON, 0);
		enableClock(false);
	}
	
	return kIOReturnSuccess;
}

void AppleS5L8920XBasebandSPIController::dumpRegistersInternal()
{
	if ( !clockEnabled() )
	{
		SPI_LOG( DEBUG_CRITICAL, "Clock is off\n" );
		return;
	}

	SPI_LOG_PLAIN( DEBUG_SPEW, "Register dump\n" );
	SPI_LOG_PLAIN( DEBUG_SPEW, "\tSPCLKCON: %#8lx\n", readReg32(kS5L8920XSPCLKCON) );
	SPI_LOG_PLAIN( DEBUG_SPEW, "\t   SPCON: %#8lx\n", readReg32(kS5L8920XSPCON) );
	SPI_LOG_PLAIN( DEBUG_SPEW, "\t   SPSTA: %#8lx\n", readReg32(kS5L8920XSPSTA) );
	SPI_LOG_PLAIN( DEBUG_SPEW, "\t   SPIDD: %#8lx\n", readReg32(kS5L8920XSPIDD) );
	SPI_LOG_PLAIN( DEBUG_SPEW, "\t   SPPRE: %#8lx\n", readReg32(kS5L8920XSPPRE) );
	SPI_LOG_PLAIN( DEBUG_SPEW, "\t   SPCNT: %#8lx\n", readReg32(kS5L8920XSPCNT) );
	SPI_LOG_PLAIN( DEBUG_SPEW, "\t   SPTDCNT: %#8lx\n", readReg32(kS5L8920XSPTDCNT) );
}

IOReturn AppleS5L8920XBasebandSPIController::takeSnapshot( const char * comment, 
	BasebandSPIControllerSnapshot::PinState resetDetectState, BasebandSPIControllerSnapshot ** snapshot )
{	
	bool wasClockEnabled = clockEnabled();
	// Enable the SPI clock temporarily for snapshot, if not already enabled
	if (!wasClockEnabled)
	{
		enableClock(true);
	}

	*snapshot = new AppleS5L8920XBasebandSPIControllerSnapshot( comment,
						resetDetectState, 
						readReg32(kS5L8920XSPCLKCON), 
						readReg32(kS5L8920XSPCON), 
						readReg32(kS5L8920XSPSTA),
						readReg32(kS5L8920XSPCNT),
						readReg32(kS5L8920XSPTDCNT) );

	// Disable SPI clock temporarily for snapshot, if enabled just for duration of snapshot
	if (!wasClockEnabled)
	{
		enableClock(false);
	}
	
	return kIOReturnSuccess;
}

IOReturn AppleS5L8920XBasebandSPIController::startHardware( void )
{
	if ( clockEnabled() )
	{
		writeReg32( kS5L8920XSPCLKCON, _spiClkConReg );
	}
	else
	{
		SPI_LOG( DEBUG_CRITICAL, "AppleS5L8920XBasebandSPIController::startHardware: clock not enabled\n" );
	}
	
	return kIOReturnSuccess;
}

bool AppleS5L8920XBasebandSPIController::validateDeviceTypeAndChip( const char * deviceType, UInt32 chipRevision ) const
{
#if 0  // Disable Chip ID check and always return true
	(void)deviceType;
	if ( chipRevision == 0 )
	{
		for ( unsigned i = 0; i < 3; i++ )
		{
			SPI_LOG( DEBUG_CRITICAL, "!!!! Chip revision %u is not supported !!!!\n", chipRevision );
			IOSleep( 1000 );
		}
		return false;
	}
	else
	{
		return true;
	}
#else
	return true;
#endif
}
