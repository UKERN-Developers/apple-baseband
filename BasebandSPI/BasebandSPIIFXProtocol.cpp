#include <sys/param.h>
#include <IOKit/platform/AppleARMFunction.h>

#include "BasebandSPIIFXProtocol.h"
#include "BasebandSPIController.h"


#define super BasebandSPIProtocol

OSDefineMetaClassAndAbstractStructors( BasebandSPIIFXProtocol, super );



bool BasebandSPIIFXProtocol::init( BasebandSPIController * controller, BasebandSPIDevice * device )
{
	SPI_TRACE( DEBUG_INFO );

	fMRDYFunction	 = NULL;
	fSRDYFunction	 = NULL;
	
	fSRDYEventSource			= NULL;
	fSRDYTimeoutEventSource		= NULL;
	
	clearSRDYMissCount();
	
	fMRDYModificationTime = 0;
	

	bool ret = super::init( controller, device );
	
	if ( !ret )
	{
		return ret;
	}
			
	IOReturn result;
	
	result = createFunctions();
	if ( result != kIOReturnSuccess )
	{
		return false;
	}
	
	result = createEventSources();
	if ( result != kIOReturnSuccess )
	{
		return false;
	}
	
	return true;
}

void BasebandSPIIFXProtocol::free( void )
{
	SPI_TRACE( DEBUG_INFO );

	releaseEventSources();
	
	releaseFunctions();

	super::free();
}

IOReturn BasebandSPIIFXProtocol::createEventSources( void )
{
	IOReturn ret;
	
	SPI_TRACE( DEBUG_INFO );
	
	IOInterruptEventAction interruptAction = getSRDYAction();
	
#if ( SPI_USE_REAL_ISR || SPI_USE_REAL_ISR_FOR_DEBUG )
	IOFilterInterruptAction filterInterruptAction = getSRDYFilterAction(); 

	fSRDYEventSource = IOFilterInterruptEventSource::filterInterruptEventSource( this,
		interruptAction,
		filterInterruptAction,
		fController->getProvider(), 0 );
#else
	fSRDYEventSource = IOInterruptEventSource::interruptEventSource(this, 
		interruptAction,
		fController->getProvider(), 0);
#endif

	ret = addEventSource( (IOEventSource **)&fSRDYEventSource, "SRDY Interrupt" );
	if ( ret != kIOReturnSuccess )
	{
		return ret;
	}
	
	IOTimerEventSource::Action action;
	
	action = OSMemberFunctionCast( IOTimerEventSource::Action, this,
											&BasebandSPIIFXProtocol::handleSRDYTimeoutAction );
	fSRDYTimeoutEventSource = IOTimerEventSource::timerEventSource( this, action );
	
	ret = addEventSource( (IOEventSource **)&fSRDYTimeoutEventSource, "SRDY Timeout" );
	if ( ret != kIOReturnSuccess )
	{
		return ret;
	}
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIIFXProtocol::releaseEventSources( void )
{
	SPI_TRACE( DEBUG_INFO );

	if ( fSRDYEventSource )
	{
		fSRDYEventSource->release();
		fSRDYEventSource = NULL;
	}
	
	if ( fSRDYTimeoutEventSource )
	{
		fSRDYTimeoutEventSource->release();
		fSRDYTimeoutEventSource = NULL;
	}	
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIIFXProtocol::createFunctions( void )
{
	SPI_TRACE( DEBUG_INFO );

	fMRDYFunction = AppleARMFunction::withProvider( fController->getProvider(), "function-mrdy");
	if (!fMRDYFunction)
	{
		SPI_LOG( DEBUG_CRITICAL, "Couldn't get function-mrdy\n" );
		return kIOReturnError;
	}
	
	fMRDYState = 3;
	
	// Reconfigure IPC_SRDY. iBoot sets it as UART1_NCTS
	fSRDYFunction = AppleARMFunction::withProvider( fController->getProvider(), "function-srdy");

	if ( !fSRDYFunction )
	{
		SPI_LOG( DEBUG_CRITICAL, "Couldn't get function-srdy\n" );
		return kIOReturnError;
	}
		
	return kIOReturnSuccess;
}

IOReturn BasebandSPIIFXProtocol::releaseFunctions( void )
{
	SPI_TRACE( DEBUG_INFO );

	if ( fMRDYFunction )
	{
		fMRDYFunction->release();
		fMRDYFunction = NULL;
	}
	
	if ( fSRDYFunction )
	{
		fSRDYFunction->release();
		fSRDYFunction = NULL;
	}
	
	return kIOReturnSuccess;
}


void BasebandSPIIFXProtocol::setMRDY( UInt32 value )
{
	bool bSRDYHigh = isSRDYStateHigh();
	
	SPI_LOG( DEBUG_SPEW, "To %u\n", value );
	
	if ( fMRDYState != value )
	{
		fMRDYState = value;
		uint64_t now;
		
		getTime(&now);
		
		// only have to delay if raising MRDY
		if ( value )
		{
			UInt32 delta = getRemainingTimeLeftMicroSec( now, fMRDYModificationTime, 200 );
			if ( delta )
			{
#if SPI_SRDY_DEBUG
				BasebandSPILogger::logGeneral( "MRDYDFR", delta, isSRDYStateHigh() );
#else
				BasebandSPILogger::logGeneral( "MRDYDFR", delta );
#endif
				IODelay( delta );
				getTime(&fMRDYModificationTime);
			}
			else {
				fMRDYModificationTime = now;
			}
		}
		else {
			fMRDYModificationTime = now;
		}
		
		if ( bSRDYHigh )
		{
			BasebandSPILogger::logMRDY( value , "SRDYHI" );
		}
		else
		{
			BasebandSPILogger::logMRDY( value , "SRDYLO" );
		}
		IOReturn ret = fMRDYFunction->callFunction( &fMRDYState );
		SPI_ASSERT_SUCCESS( ret );
	}
	else
	{
		if ( bSRDYHigh )
		{
			BasebandSPILogger::logMRDY( value, "same (SRDYHI)" );
		}
		else
		{
			BasebandSPILogger::logMRDY( value, "same (SRDYLO)" );
		}
	}
}

void BasebandSPIIFXProtocol::handleSRDYTimeoutAction( IOTimerEventSource * es )
{
	(void)es;

	SPI_TRACE( DEBUG_CRITICAL );

#if SPI_SRDY_DEBUG
	BasebandSPILogger::logGeneral( "SRDYT", 0, isSRDYStateHigh() );
#else
	BasebandSPILogger::logGeneral( "SRDYT", 0 );
#endif
	
#if SPI_PROTOCOL_RERAISE_MRDY
	if ( fSRDYMissCount++ < 5 )
	{
		SPI_LOG( DEBUG_CRITICAL, "SRDY Timeout, retry number %u\n", fSRDYMissCount );
		lowerMRDY();
		raiseMRDY();
		startSRDYTimeoutTimer();
		return;
	}

#endif
	fController->reportError( kBasebandSPIReturnProtocolError, "SRDY Timeout" );

}

IOReturn BasebandSPIIFXProtocol::activate( void )
{
	SPI_TRACE( DEBUG_INFO );

	IOReturn ret = super::activate();	
	SPI_ASSERT_SUCCESS( ret );
	
	clearSRDYMissCount();
	
	fAllowSRDY = true;
	fSRDYEventSource->enable();
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIIFXProtocol::predeactivate( void )
{
	SPI_TRACE( DEBUG_INFO );

	fSRDYEventSource->disable();
	fAllowSRDY = false;
	
	lowerMRDY();
	stopSRDYTimeoutTimer();
	
	IOReturn ret = super::predeactivate();
	
	SPI_ASSERT_SUCCESS( ret );
	
	return kIOReturnSuccess;
}

class BasebandSPIIFXProtocolSnapshot: public BasebandSPIProtocolSnapshot
{
public:
	BasebandSPIIFXProtocolSnapshot( BasebandSPIControllerSnapshot::PinState mrdyState, BasebandSPIControllerSnapshot::PinState srdyState )
		: BasebandSPIProtocolSnapshot(), fMRDYState( mrdyState ), fSRDYState( srdyState )
	{
	}
	
	virtual ~ BasebandSPIIFXProtocolSnapshot()
	{
	}
	
	virtual IOByteCount copyAsString( char * dest, IOByteCount capacity ) const
	{
		IOByteCount consumed = SPIsnprintf( dest, capacity, " - Protocol: MRDY %s, SRDY %s", 
			BasebandSPIControllerSnapshot::pinStateAsString( fMRDYState ),
			BasebandSPIControllerSnapshot::pinStateAsString( fSRDYState ) );
			
		return consumed;
	}
	
private:
	BasebandSPIControllerSnapshot::PinState fMRDYState;
	BasebandSPIControllerSnapshot::PinState fSRDYState;

};


BasebandSPIProtocolSnapshot * BasebandSPIIFXProtocol::getSnapshot( void ) const
{
	BasebandSPIControllerSnapshot::PinState mrdyState = BasebandSPIControllerSnapshot::kDunno;
	
	if ( fMRDYState == 1 )
	{
		mrdyState = BasebandSPIControllerSnapshot::kHigh;
	}
	else if ( fMRDYState == 0 )
	{
		mrdyState = BasebandSPIControllerSnapshot::kLow;
	}
	
	BasebandSPIControllerSnapshot::PinState srdyState = BasebandSPIControllerSnapshot::kDunno;
	
	if ( fSRDYFunction )
	{
		UInt32 dest = 0;
		fSRDYFunction->callFunction(NULL, &dest );
		
		if ( dest == 1)
		{
			srdyState = BasebandSPIControllerSnapshot::kHigh;
		}
		else if ( dest == 0 )
		{
			srdyState = BasebandSPIControllerSnapshot::kLow;
		}
	}

	return new BasebandSPIIFXProtocolSnapshot( mrdyState, srdyState );
};


bool BasebandSPIIFXProtocol::isSRDYStateHigh ( void )
{
	UInt32 dest = 0;
	if ( fSRDYFunction )
	{
		fSRDYFunction->callFunction(NULL, &dest );
		
	}
	return ( dest == 1 );
}

IOReturn BasebandSPIIFXProtocol::exitLowPower( void )
{
	IOReturn ret = super::exitLowPower();
	if ( ret != kIOReturnSuccess )
	{
		return ret;
	}
	
	if ( isSRDYStateHigh() )
	{
		handleSRDYInterruptAction( fSRDYEventSource, 1 );
	}
	
	return kIOReturnSuccess;
}
