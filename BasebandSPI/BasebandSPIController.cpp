
#include <sys/param.h>

#include <IOKit/platform/AppleARMFunction.h>
#include <IOKit/platform/AppleARMIODevice.h>
#include <IOKit/IODMAEventSource.h>
#include <IOKit/IOTimerEventSource.h>

#include <IOKit/AppleBaseband.h>
#include <IOKit/AppleBasebandKeys.h>

#include "BasebandSPIProtocol.h"
#include "BasebandSPIController.h"
#include "BasebandSPIReturn.h"
#include "BasebandSPIDiagnostics.h"

#define super IOService
OSDefineMetaClassAndAbstractStructors(BasebandSPIController, super);

// TODO: move this somewhere else
static const unsigned kTransferCompleteTimeoutInterval = 1000;

/* static */ const char * BasebandSPIController::stateAsString( State state )
{
	switch ( state )
	{
	case kStateInactive:		return "Inactive";
	case kStateActive:			return "Active";
	case kStateDeactivating:	return "Deactivating";
	default:					return "???";
	}
}

const char * BasebandSPIController::eventSourceAsString( IODMAEventSource * es ) const
{
	bool bSRDYHigh = (fProtocol) ? fProtocol->isSRDYStateHigh() : false;
	if ( es == fTxDMAES )
	{
		if ( bSRDYHigh )
		{
			return ( "Tx DMA (SRDYHI)" );
		}
		else
		{
			return ( "Tx DMA (SRDYLO)" );
		}
	}
	else if ( es == fRxDMAES )
	{
		if ( bSRDYHigh )
		{
			return ( "Rx DMA (SRDYHI)" );
		}
		else
		{
			return ( "Rx DMA (SRDYLO)" );
		}
	}
	else
	{
		return "Dunno";
	}
}

void BasebandSPIController::changeState( State newState )
{
	SPI_LOG( DEBUG_INFO, "State goes from %s to %s\n",
		stateAsString( fState ), stateAsString( newState ) );

	fState = newState;
	fCommandGate->commandWakeup( &fState );
}

IOReturn BasebandSPIController::waitForStateChange( State fromState )
{
	while ( fState == fromState )
	{
		IOReturn ret;

		SPI_LOG( DEBUG_INFO, "Current state %s, from %s",
			stateAsString( fState ), stateAsString( fromState ) );

		ret = fCommandGate->commandSleep( &fState, THREAD_UNINT );
		if ( ret != kIOReturnSuccess )
		{
			SPI_LOG( DEBUG_CRITICAL, "Sleeping failed, ret = %p\n", ret );
			return ret;
		}
	}
	
	return kIOReturnSuccess;
}

bool BasebandSPIController::init( OSDictionary * dict )
{
	SPI_TRACE( DEBUG_INFO );

	fState					= kStateInactive;
	
	fHardwareStateFlags				= 0;
	fVolatileHardwareStateFlags		= 0;

	fProvider				= NULL;
	
	fCommandGate			= NULL;
	
	fPendingReceiveCommand	= NULL;
	fPendingSendCommand		= NULL;
	
	fCurrentReceiveCommand	= NULL;
	fCurrentSendCommand		= NULL;
	
	fTransferCompleteTimeout		= NULL;
	
	fTxDMAES						= NULL;
	fRxDMAES						= NULL;
	
	fPendingDMAs			= kNone;
	fInLowPower				= false;
	fEnteringLowPower			= false;
	
	fDiags					= NULL;
	
	fSavedSnapshot			= NULL;
	
	fInterruptGuard			= kOpen;
	
	fDevice					= NULL;
	fProtocol				= NULL;
	
	bool ret = super::init( dict );

	if ( !ret )
	{
		return false;
	}
	
	return true;
}

IOService * BasebandSPIController::probe( IOService * provider, SInt32 * score )
{
	SPI_TRACE( DEBUG_INFO );
	
	const char * providerName = provider->getName();
	
	*score = 5000;
    
    SPI_LOG( DEBUG_INFO, "provider = 0x%08x, providerName = %s, score = %d, \n", provider, providerName , *score );
	
	return super::probe( provider, score );
}


bool BasebandSPIController::start(IOService *provider)
{
	SPI_TRACE( DEBUG_INFO );
	
	fProvider = OSDynamicCast( AppleARMIODevice, provider );
	if (!fProvider) return false;
	
	
	/*
	{
		mach_timespec_t ts = {0, 20*1000*1000};
		
		IOService * baseband = IOService::waitForService( serviceMatching( "baseband" ), &ts );
		
		if ( baseband == NULL )
		{
			SPI_LOG( DEBUG_CRITICAL, "baseband doesn't exist\n" );
			return false;
		}
	}
	*/
	
	fProvider->retain();
	
	fWorkLoop = IOWorkLoop::workLoop();
	if ( !fWorkLoop ) 
	{
		SPI_LOG( DEBUG_CRITICAL, "Failed to create work loop\n" );
		return false;
	}
	
	fCommandGate = IOCommandGate::commandGate( this );
	if ( fCommandGate == NULL )
	{
		SPI_LOG( DEBUG_CRITICAL, "Failed to create command gate\n" );
		return false;
	}

	IOReturn ret = getWorkLoop()->addEventSource( fCommandGate );
	
	if ( ret != kIOReturnSuccess )
	{
		fCommandGate->release();
		fCommandGate = NULL;
		SPI_LOG( DEBUG_CRITICAL, "Failed to add command gate to workloop, ret = %p\n", ret );
		return false;
	}
	
	ret = createEventSources();
	if ( ret != kIOReturnSuccess )
	{
		return false;
	}
	
	ret = validateHardware();
	if ( ret != kIOReturnSuccess )
	{
		return false;
	}
	
	ret = loadConfiguration();
	if ( ret != kIOReturnSuccess )
	{
		return false;
	}
	
	fDiags = BasebandSPIDiagnostics::withProvider( fProvider );
	if ( !fDiags )
	{
		SPI_LOG( DEBUG_CRITICAL, "Failed to set up diags\n" );
		return false;
	}
	
	SPI_LOG( DEBUG_SPEW, "All OK, registering service\n" );
		
	return true;
}

IOWorkLoop * BasebandSPIController::getWorkLoop( void ) const
{
	return fWorkLoop;
}

IOReturn BasebandSPIController::validateHardware( void )
{
	static const char * kPlatformDeviceName = "arm-io";

	IOService * service = IOService::waitForService( nameMatching( kPlatformDeviceName ) );
	
	OSObject * object;
	

	object = service->getProperty( "chip-revision" );
	SPI_ASSERT( object, !=, NULL );
		
	OSData * data = OSDynamicCast( OSData, object );
	SPI_ASSERT( data, !=, NULL );
		
	UInt32 chipRevision = *( (UInt32 *)data->getBytesNoCopy() );

	
	object = service->getProperty( "device_type" );
	SPI_ASSERT( object, !=, NULL );
		
	OSData * str = OSDynamicCast( OSData, object );
	SPI_ASSERT( str, !=, NULL );
		
	char deviceType[32];
	strncpy( deviceType, (const char *)str->getBytesNoCopy(), MIN( str->getLength(), sizeof( deviceType ) ) );
	
	
	bool ret = validateDeviceTypeAndChip( deviceType, chipRevision );
	
	if ( ret )
	{
		SPI_LOG( DEBUG_SPEW, "Hardware is OK\n" );
		return kIOReturnSuccess;
	}
	else
	{
		SPI_LOG( DEBUG_SPEW, "Hardware is not compatible\n" );
		return kBasebandSPIReturnHardwareIncompatible;
	}
}

IOReturn BasebandSPIController::loadConfiguration( void )
{
	OSData * data = OSDynamicCast(OSData, getProvider()->getProperty("config"));
	if (!data)
	{
		SPI_LOG( DEBUG_CRITICAL, "config property not found\n" );
		return kIOReturnError;
	}
	
	if (data->getLength() != sizeof(BasebandSPIConfig))
	{
		SPI_LOG( DEBUG_CRITICAL, "data size %d is less than config size %d\n",
					data->getLength(), sizeof(BasebandSPIConfig) );
		return kIOReturnError;
	}
	
	{
		BasebandSPIConfig * config = (BasebandSPIConfig *)data->getBytesNoCopy();
		memcpy( &fConfiguration, config, sizeof( BasebandSPIConfig ) );
	}
	
	return kIOReturnSuccess;
}

void BasebandSPIController::stop(IOService *provider)
{
	SPI_TRACE( DEBUG_INFO );
	
	if ( fSavedSnapshot )
	{
		delete fSavedSnapshot;
		fSavedSnapshot = NULL;
	}
	
	if ( fDiags )
	{
		fDiags->release();
		fDiags = NULL;
	}
	
	releaseEventSources();
	
	if ( fCommandGate  )
	{
		getWorkLoop()->removeEventSource( fCommandGate );
		fCommandGate->release();
		fCommandGate = NULL;
	}
	
	if ( fWorkLoop )
	{
		fWorkLoop->release();
		fWorkLoop = NULL;
	}

	if ( fProvider )
	{
		fProvider->release();
		fProvider = NULL;
	}	
}

IOReturn BasebandSPIController::activate( BasebandSPIDevice * device, BasebandSPIProtocol * protocol )
{
	SPI_ASSERT( getWorkLoop()->inGate(), ==, true );

	switch ( fState )
	{
	case kStateInactive:
		break;
		
	default:
		SPI_LOG( DEBUG_CRITICAL, "Can't activate when state is %s\n",
			stateAsString( fState ) );
		return kIOReturnNotPermitted;
	}

#if SPI_SRDY_DEBUG
	BasebandSPILogger::logGeneral( "AC", 0, (protocol) ? protocol->isSRDYStateHigh() : false );
#else
	BasebandSPILogger::logGeneral( "AC", 0 );
#endif
	
	changeState( kStateActive );
	
	fDevice			= device;
	fProtocol		= protocol;
		
	return kIOReturnSuccess;
}

IOReturn BasebandSPIController::deactivate( void )
{
	SPI_ASSERT( getWorkLoop()->inGate(), ==, true );

	switch ( fState )
	{
	case kStateActive:
		break;
		
	default:
		SPI_LOG( DEBUG_CRITICAL, "State is %s, clearing low power state\n",
			stateAsString( fState ) );
			
		// we must clear this anyway, if we are in low power, we
		// are in the inactive state, but the fInLowPower flag is true
		// so we need to clear it since we may close down SPI before we
		// get the exit low power call from the system
		fInLowPower = false;
		return kIOReturnSuccess;
	}
#if SPI_SRDY_DEBUG
	BasebandSPILogger::logGeneral( "DE", 0, fProtocol->isSRDYStateHigh() );
#else
	BasebandSPILogger::logGeneral( "DE", 0 );
#endif	
	beginBlockInterrupt();
	
	IOReturn ret;
	
	ret = unconfigure();
	if ( ret != kIOReturnSuccess )
	{
		SPI_LOG( DEBUG_CRITICAL, "Unconfiguring failed, ret = %p\n", ret );
	}
	
	endBlockInterrupt();
	
	changeState( kStateDeactivating );
	
	//BasebandSPILogger::report();
	//BasebandSPILogger::clear();
	
	ret = cancelAll();
	SPI_ASSERT_SUCCESS( ret );
	
	ret = waitForDMACompletion();
	SPI_ASSERT_SUCCESS( ret );	
	
	changeState( kStateInactive );
	
	fDevice			= NULL;
	fProtocol		= NULL;

	fInLowPower		= false;
		
	return ret;
}

IOReturn BasebandSPIController::waitForDMACompletion( void )
{
	SPI_LOG( DEBUG_INFO, "Pending DMAs = %p\n", fPendingDMAs );
	
	while ( fPendingDMAs )
	{
		IOReturn ret;
		ret = fCommandGate->commandSleep( &fPendingDMAs, THREAD_UNINT );
		if ( ret != kIOReturnSuccess )
		{
			SPI_LOG( DEBUG_CRITICAL, "Sleeping failed, ret = %p\n", ret );
			return ret;
		}
		
		//BasebandSPILogger::report();
		//BasebandSPILogger::clear();
		
		SPI_LOG( DEBUG_INFO, "Updated Pending DMAs = %p\n", fPendingDMAs );
	}
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIController::enterLowPower( void )
{
	SPI_TRACE( DEBUG_INFO );
	
	SPI_ASSERT( fInLowPower, ==, false );
	fEnteringLowPower = true;
	
	switch( fState )
	{
	case kStateActive:
		fStateBeforeLowPower = fState;
		deactivate();
		break;
		
	case kStateDeactivating:
		{
			IOReturn ret = waitForDMACompletion();
			SPI_ASSERT_SUCCESS( ret );
			
			ret = waitForStateChange( kStateDeactivating );
			SPI_ASSERT_SUCCESS( ret );
			
			// in the very unlikely event we went active again, we need to tear down
			// and break
			if ( fState == kStateActive )
			{
				fStateBeforeLowPower = fState;
				deactivate();
				break;
			}
		}
		// fall through
		
	case kStateInactive:
		fStateBeforeLowPower = fState;
		break;	

	}

	fEnteringLowPower 			= false;
	fInLowPower				= true;	
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIController::exitLowPower( BasebandSPIDevice * device, BasebandSPIProtocol * protocol )
{
	SPI_TRACE( DEBUG_INFO );
	
	SPI_ASSERT( fInLowPower, ==, true );
	fInLowPower	= false;
	
	switch( fStateBeforeLowPower )
	{
	case kStateActive:
		{
			IOReturn ret = activate( device, protocol );
			SPI_ASSERT_SUCCESS( ret );
			
			fState	= fStateBeforeLowPower;
		}
		break;
	default:
		break;
	}
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIController::configureDMA( IODMACommand * command, IOByteCount size, DMAFlags what )
{
	SPI_TRACE( DEBUG_SPEW );
	
	if ( fPendingDMAs & what )
	{
		return kBasebandSPIReturnAlreadyQueued;
	}

	fPendingDMAs |= what;
	
	IODMAEventSource * es;
	IODirection direction;
	
	switch ( what )
	{
	case kTxDMA:
		fPendingSendCommand			= command;
		es							= fTxDMAES;
		direction					= kIODirectionOut;
		break;
		
	case kRxDMA:
	default:
		fPendingReceiveCommand		= command;
		es							= fRxDMAES;
		direction					= kIODirectionIn;
		break;
	}
#if SPI_SRDY_DEBUG				
	BasebandSPILogger::logGeneral( "PREP", 1 | ( what << 8 ), ((fProtocol) ? fProtocol->isSRDYStateHigh() : false) );
#else
	BasebandSPILogger::logGeneral( "PREP", 1 | ( what << 8 ) );
#endif
	IOReturn ret;
	ret = queueDMAWithCommand( es, direction, command, size );
	
	return ret;
}

IOReturn BasebandSPIController::configure( IODMACommand * receiveCommand, IODMACommand * transmitCommand, IOByteCount size )
{
	SPI_TRACE( DEBUG_SPEW );
	
#if SPI_PEDANTIC_ASSERT
	SPI_ASSERT( fState, ==, kStateActive );
#endif
	
	UInt32 currentVolatileFlags = fVolatileHardwareStateFlags;
	
	SPI_LOG( DEBUG_SPEW, "currentVolatile = %p, hardware state = %p, rx = %p, tx = %p\n",
		currentVolatileFlags, fHardwareStateFlags, receiveCommand, transmitCommand );
#if SPI_SRDY_DEBUG	
	BasebandSPILogger::logGeneral( "PSTAT", ( currentVolatileFlags <<  16 )  | fHardwareStateFlags, ((fProtocol) ? fProtocol->isSRDYStateHigh() : false) );
#else
	BasebandSPILogger::logGeneral( "PSTAT", ( currentVolatileFlags <<  16 )  | fHardwareStateFlags );
#endif
	IOReturn ret;
	
	if ( currentVolatileFlags & kFlagTransferActive )
	{
		// don't allow it because we're currently in the middle of a transfer
		ret = kBasebandSPIReturnTransferActive;
#if SPI_SRDY_DEBUG
		BasebandSPILogger::logGeneral( "PREP", 4, ((fProtocol) ? fProtocol->isSRDYStateHigh() : false) );
#else
		BasebandSPILogger::logGeneral( "PREP", 4 );
#endif
	}
	else
	{
		if ( transmitCommand )
		{
			ret = configureDMA( transmitCommand, size, kTxDMA );
#if SPI_PEDANTIC_ASSERT
			SPI_ASSERT_SUCCESS( ret );
#else
			(void)ret;
#endif
		}

		if ( receiveCommand )
		{
			ret = configureDMA( receiveCommand, size, kRxDMA );
#if SPI_PEDANTIC_ASSERT
			SPI_ASSERT_SUCCESS( ret );
#else		
			(void)ret;
#endif

		}
		
#if SPI_SRDY_DEBUG
		BasebandSPILogger::logGeneral( "PREP", 2, ((fProtocol) ? fProtocol->isSRDYStateHigh() : false) );
#else
		BasebandSPILogger::logGeneral( "PREP", 2 );
#endif	
		
		ret = configureHardware( fPendingDMAs, size );
		if ( ret != kIOReturnSuccess )
		{
			goto bail;
		}
		
		currentVolatileFlags |= kFlagHardwareActive;
	}
	
#if SPI_SRDY_DEBUG
	BasebandSPILogger::logGeneral( "PREP", 3, ((fProtocol) ? fProtocol->isSRDYStateHigh() : false) );
#else
	BasebandSPILogger::logGeneral( "PREP", 3 );
#endif
	
	// Set the volatile flags to the current one
	fVolatileHardwareStateFlags	= currentVolatileFlags;
bail:
	dumpRegisters( DEBUG_SPEW );
	return ret;
}

IOReturn BasebandSPIController::unconfigure( void )
{
	SPI_TRACE( DEBUG_SPEW );

#if SPI_SRDY_DEBUG
	BasebandSPILogger::logGeneral( "DPREP", ( fVolatileHardwareStateFlags <<  16 )  | fHardwareStateFlags, ((fProtocol) ? fProtocol->isSRDYStateHigh() : false) );
#else
	BasebandSPILogger::logGeneral( "DPREP", ( fVolatileHardwareStateFlags <<  16 )  | fHardwareStateFlags );
#endif
	
	dumpRegisters( DEBUG_SPEW );
	
	IOReturn ret;
	
	ret = unconfigureHardware();
	
	if ( ret != kIOReturnSuccess )
	{
		goto bail;
	}
	
	fVolatileHardwareStateFlags &= ~ ( kFlagHardwareActive | kFlagTransferActive );
	
bail:
	
	return ret;
}

IOReturn BasebandSPIController::startTransfer( void )
{
	SPI_TRACE( DEBUG_SPEW );
	
	fVolatileHardwareStateFlags |= kFlagTransferActive;

	IOReturn ret = startHardware();
	
	dumpRegisters( DEBUG_SPEW );
	
	return ret;
}

IOReturn BasebandSPIController::cancelAll( void )
{
	SPI_TRACE( DEBUG_SPEW );
	
	IOReturn finalRet = kIOReturnSuccess;
	IOReturn ret;
	
	if ( fPendingDMAs & kRxDMA )
	{
		ret = cancelDMA( fRxDMAES );
		if ( ret != kIOReturnSuccess )
		{
			finalRet = ret;
		}
	}
	
	if ( fPendingDMAs & kTxDMA )
	{
		ret = cancelDMA( fTxDMAES );
		if ( ret != kIOReturnSuccess )
		{
			finalRet = ret;
		}
	}
	
	
	return finalRet;
}

IOReturn BasebandSPIController::createEventSources( void )
{
	IOReturn ret;
		
	IODMAEventAction notifyAction;
	
#if SPI_HANDLE_DMA_NOTIFY
	notifyAction = OSMemberFunctionCast( IODMAEventAction, this, &BasebandSPIController::dmaNotifyAction );
#else
	notifyAction = NULL;
#endif

	// Only operate in DMA mode
	fTxDMAES = IODMAEventSource::dmaEventSource(this, fProvider, OSMemberFunctionCast(IODMAEventAction, this,
		&BasebandSPIController::dmaCompleteAction), notifyAction, 0);
	if ( !fTxDMAES )
	{
		SPI_LOG( DEBUG_CRITICAL, "Couldn't get TxDMA Event source\n" );
		return kIOReturnError;
	}
	ret = getWorkLoop()->addEventSource( fTxDMAES );
	
	if ( ret != kIOReturnSuccess )
	{
		fTxDMAES->release();
		fTxDMAES = NULL;
		
		SPI_LOG( DEBUG_CRITICAL, "Couldn't add source for TxDMA, ret = %p\n", ret );
		return ret;
	}
	
	fRxDMAES = IODMAEventSource::dmaEventSource(this, fProvider, OSMemberFunctionCast(IODMAEventAction, this,
		&BasebandSPIController::dmaCompleteAction), notifyAction, 1);
	if ( !fRxDMAES )
	{
		SPI_LOG( DEBUG_CRITICAL, "Couldn't get RxDMA Event source\n" );
		return kIOReturnError;
	}
	ret = getWorkLoop()->addEventSource( fRxDMAES );
	
	if ( ret != kIOReturnSuccess )
	{
		fRxDMAES->release();
		fRxDMAES = NULL;

		SPI_LOG( DEBUG_CRITICAL, "Couldn't add source for RxDMA, ret = %p\n", ret );
		return ret;
	}

	fTransferCompleteTimeout = IOTimerEventSource::timerEventSource(this, OSMemberFunctionCast(IOTimerEventSource::Action,
		this, &BasebandSPIController::transferCompleteTimeoutAction ));
	if ( !fTransferCompleteTimeout )
	{
		SPI_LOG( DEBUG_CRITICAL, "Couldn't get Transfer complete timeout event source\n" );
		return kIOReturnError;
	}
	
	ret = getWorkLoop()->addEventSource( fTransferCompleteTimeout );
	
	if ( ret != kIOReturnSuccess )
	{
		fTransferCompleteTimeout->release();
		fTransferCompleteTimeout = NULL;
		
		SPI_LOG( DEBUG_CRITICAL, "Couldn't add source for SRDY Timeout, ret = %p\n", ret );
		return ret;
	}
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIController::releaseEventSources( void )
{
	SPI_TRACE( DEBUG_SPEW );

	if ( fTxDMAES )
	{
		getWorkLoop()->removeEventSource( fTxDMAES );
		fTxDMAES->release();
		fTxDMAES = NULL;
	}
	
	if ( fRxDMAES )
	{
		getWorkLoop()->removeEventSource( fRxDMAES );
		fRxDMAES->release();
		fRxDMAES = NULL;
	}
	
	if ( fTransferCompleteTimeout )
	{
		getWorkLoop()->removeEventSource( fTransferCompleteTimeout );
		fTransferCompleteTimeout->release();
		fTransferCompleteTimeout = NULL;
	}
	
	return kIOReturnSuccess;
}


void BasebandSPIController::dmaNotifyAction(IODMAEventSource *es, 
	IODMACommand *command, IOReturn status, IOByteCount byteCount)
{
	SPI_TRACE( DEBUG_SPEW );
		
	SPI_LOG( DEBUG_SPEW, "es = %p (%s), command = %p, status = %p, byteCount = %p,  pending DMAs = %p\n",
		es, eventSourceAsString( es ), command, status, byteCount, fPendingDMAs );

}

IOReturn BasebandSPIController::handleDMACompleteOnCancel( IODMAEventSource * eventSource,
	IODMACommand * command, IOReturn status, IOByteCount byteCount )
{
	SPI_TRACE( DEBUG_INFO );

	if ( eventSource == fTxDMAES )
	{
		if ( command )
		{
			handleCancelledTxDMACommand( command );
			fCurrentSendCommand = NULL; // We cancelled the Tx so we have to mark it as not available as well otherwise it could be reused in handleTransferComplete
		}
		if ( fCurrentReceiveCommand )
		{
			handleCancelledRxDMACommand( fCurrentReceiveCommand );
			fCurrentReceiveCommand = NULL;
		}
	}
	else
	{
		if ( command )
		{
			handleCancelledRxDMACommand( command );
			fCurrentReceiveCommand = NULL; // We cancelled the Rx so we have to mark it as not available as well otherwise it could be reused in handleTransferComplete
		}
		
		if ( fCurrentSendCommand )
		{
			handleCancelledTxDMACommand( fCurrentSendCommand );
			fCurrentSendCommand = NULL;
		}
	}
	if ( fPendingDMAs == kNone )
	{
		cancelTransferCompleteTimer();
		fCommandGate->commandWakeup( &fPendingDMAs );

		fProtocol->handleTransferCancelled();
	}
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIController::handleDMACompleteOnError( IODMAEventSource * eventSource,
	IODMACommand * command, IOReturn status, IOByteCount byteCount )
{
	IOReturn ret = handleDMACompleteOnCancel( eventSource, command, status, byteCount );
	if ( ret != kIOReturnSuccess )
	{
		return ret;
	}
		
	// If we've gotten all the DMA callbacks we need, we can now throw up the error
	if ( fPendingDMAs == kNone )
	{
		static const char * reason;
		
		UInt32 flags = fVolatileHardwareStateFlags;
		
		// TODO: this should go in the protocol
		
		if ( flags & kFlagTransferActive )
		{
			reason = "DMA Timeout";
		}
		else
		{
			reason = "SRDY Timeout";
		}
						
		// all commands are returned, we can now report the error
		reportError( kBasebandSPIReturnHWError, reason );		
	}
	return kIOReturnSuccess;
}

IOReturn BasebandSPIController::handleDMACompleteOnActive( IODMAEventSource * eventSource,
	IODMACommand * command, IOReturn status, IOByteCount byteCount )
{
	SPI_TRACE( DEBUG_SPEW );

	if ( eventSource == fRxDMAES )
	{
		fCurrentReceiveCommand = command;
	}
	else
	{
		fCurrentSendCommand = command;
	}
	
	if ( fPendingDMAs == kNone )
	{
		fDiags->markDMACompletion();
		
		cancelTransferCompleteTimer();
		
#if SPI_PEDANTIC_ASSERT
		SPI_ASSERT( fProtocol, !=, NULL );
#endif
		fProtocol->handleTransferComplete( fCurrentReceiveCommand, fCurrentSendCommand );
		
		fCurrentReceiveCommand = NULL;
		fCurrentSendCommand = NULL;
		
		fDiags->markRxHandlingComplete();
	}
	
	return kIOReturnSuccess;
}

void BasebandSPIController::dmaCompleteAction(IODMAEventSource *es,
	IODMACommand *command, IOReturn status, IOByteCount byteCount)
{
	SPI_TRACE( DEBUG_SPEW );
	
	const char * eventSourceStr = eventSourceAsString( es );

	SPI_LOG( DEBUG_SPEW, "es = %p, command = %p, status = %p, bytecount = %p, pending DMAs %p, (%s)\n", 
		es, command, status, byteCount, fPendingDMAs, eventSourceStr );
		
	dumpRegisters();
	
	if ( es == fRxDMAES )
	{
		fPendingDMAs &= ~ kRxDMA;
	}
	else
	{
		fPendingDMAs &= ~ kTxDMA;
	}
	
	SPI_LOG( DEBUG_SPEW, "Pending DMAs = %p\n", fPendingDMAs );
	
	UInt32 flags = fVolatileHardwareStateFlags;
	
	if ( flags & kFlagTransferActive )
	{
		BasebandSPILogger::logDMAComplete( eventSourceStr );
	
		handleDMACompleteOnActive( es, (IODMACommand *)command, status, byteCount );
	}
	else if ( flags & kFlagError )
	{
		SPI_LOG( DEBUG_SPEW, "In DMA Error\n" );
		BasebandSPILogger::logDMAError( eventSourceStr );
	
		handleDMACompleteOnError( es, (IODMACommand *)command, status, byteCount );
	}
	else
	{
		SPI_LOG( DEBUG_INFO, "Received cancelled DMA command\n" );
		BasebandSPILogger::logDMACancel( eventSourceStr );
	
		handleDMACompleteOnCancel( es, (IODMACommand *)command, status, byteCount );
	}
}


IOReturn BasebandSPIController::queueDMAWithCommand( IODMAEventSource * es, IODirection direction, IODMACommand * command, IOByteCount size )
{
	SPI_TRACE( DEBUG_SPEW );
	
	SPI_LOG( DEBUG_SPEW, "Start DMA on ES %s, MD %p, size %u\n",
		eventSourceAsString( es ), command->getMemoryDescriptor(), size );

	IOReturn ret = es->startDMACommand( command, direction, size, 0 );
	
	if ( ret != kIOReturnSuccess )
	{
		SPI_LOG( DEBUG_CRITICAL, "Failed to start DMA with %s, ret = %p\n", 
			eventSourceAsString( es ), ret );
		return ret;
	}
		
	return ret;
}

IOReturn BasebandSPIController::cancelDMA( IODMAEventSource * es )
{
	SPI_TRACE( DEBUG_INFO );

	es->stopDMACommand( false );
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIController::handleCancelledTxDMACommand( IODMACommand * command )
{
	SPI_TRACE( DEBUG_INFO );

	return fProtocol->handleCancelledTxCommand( command );
}

IOReturn BasebandSPIController::handleCancelledRxDMACommand( IODMACommand * command )
{
	SPI_TRACE( DEBUG_INFO );

	return fProtocol->handleCancelledRxCommand( command );
}

void BasebandSPIController::startTransferCompleteTimer( void )
{

	fTransferCompleteTimeout->cancelTimeout();
	fTransferCompleteTimeout->setTimeoutMS( kTransferCompleteTimeoutInterval );
	fTransferCompleteTimeout->enable();	
}

void BasebandSPIController::cancelTransferCompleteTimer( void )
{
	fTransferCompleteTimeout->disable();
	fTransferCompleteTimeout->cancelTimeout();
}

void BasebandSPIController::transferCompleteTimeoutAction(IOTimerEventSource *es)
{
	SPI_TRACE( DEBUG_CRITICAL );
	
	SPI_LOG( DEBUG_CRITICAL, "Pending DMA's %d, HW status %p, volatile HW status %p\n",
		fPendingDMAs, fHardwareStateFlags, fVolatileHardwareStateFlags );
	
	beginBlockInterrupt();
	unconfigure();
	
	fVolatileHardwareStateFlags = ( fVolatileHardwareStateFlags & ~ kFlagTransferActive ) | kFlagError;
	
	endBlockInterrupt();
	
	cancelAll();
	
}

bool BasebandSPIController::reportError( BasebandSPIReturn err, const char * details )
{	
	switch( fState )
	{
	case kStateInactive:
	case kStateDeactivating:
		// if we're not enabled, then don't throw up an error.
		return false;
		
	default:
		break;
	}
	
	{
		char buff[256];
		
		SPIsnprintf( buff, sizeof( buff ), "%s/%p", details, err );
		
		IOReturn ret = getNewSnapshot( buff );
		
		if ( ret != kIOReturnSuccess )
		{
			SPI_LOG( DEBUG_CRITICAL, "Failed to create new snapshot, ret = %p\n", ret );
		}
		else
		{
			SPI_ASSERT( fSavedSnapshot, !=, NULL );
			
			fSavedSnapshot->copyAsString( buff, sizeof( buff ) );
			
			SPI_LOG( DEBUG_CRITICAL, "Error: %s\n", buff );
		}
	}
	
	bool ret;
	if ( fSavedSnapshot )
	{
		if ( fDevice )
		{
			fDevice->reportError( err );
		}
		
		ret = true;
	}
	else
	{
		// Delete the snapshot if the error is not being reported to upper layers
		if ( fSavedSnapshot )
		{
			delete fSavedSnapshot;
			fSavedSnapshot = NULL;
		}
		ret = false;
	}

#if SPI_ASSERT_GPIO_ON_ERROR
	AppleARMFunction * function = AppleARMFunction::withProvider(fProvider, "function-fail_gpio");
	
	if ( function )
	{
		UInt32 value;
		IOReturn ret;
		
		SPI_LOG( DEBUG_CRITICAL, "Triggering GPIO\n" );
		
		value = 1;
		ret = function->callFunction( &value );
		SPI_ASSERT( kIOReturnSuccess, ==, ret );

		IOSleep( 20 );
		
		value = 0;
		ret = function->callFunction( &value );
		SPI_ASSERT( kIOReturnSuccess, ==, ret );

		IOSleep( 100 );		
		
		value = 1;
		ret = function->callFunction( &value );
		SPI_ASSERT( kIOReturnSuccess, ==, ret );

				
bail:
		function->release();
	}
#endif
	
	return ret;
}


void BasebandSPIController::beginBlockInterrupt( void )
{
#if SPI_USE_REAL_ISR
	// assumption: assignment is atomic here
	fInterruptGuard = kBlocked;
	BasebandSPILogger::logGeneral( "DSRDY", 0 );
#endif
}

bool BasebandSPIController::endBlockInterrupt( void )
{
#if SPI_USE_REAL_ISR
	// OSCompareAndSwap will return false if the guard isn't 1, in which case we know an SRDY interrupt came by and was turned away
	bool pending = !OSCompareAndSwap( kBlocked, kOpen, (volatile UInt32 *)&fInterruptGuard );
	BasebandSPILogger::logGeneral( "ESRDY", pending );
	return pending;
#else
	return false;
#endif
}

UInt32 BasebandSPIController::getOverrideClockPeriod( void ) const
{
	static const char * kSCLKPeriodOverrideKey = "baseband-spi-sclk-period";
	UInt32 period = 0;
	
	PE_parse_boot_argn( kSCLKPeriodOverrideKey, &period, sizeof( period ) );
	
	return period;
}

IOReturn BasebandSPIController::getSnapshot( BasebandSPIControllerSnapshot ** snapshot )
{
	if ( !fSavedSnapshot )
	{
		IOReturn ret = getNewSnapshot( "snapshot" );
		
		if ( ret != kIOReturnSuccess )
		{
			SPI_LOG( DEBUG_CRITICAL, "Failed to create new snapshot, ret = %p\n", ret );
			*snapshot = NULL;
			return ret;
		}
	}
	
	*snapshot		= fSavedSnapshot;
	fSavedSnapshot	= NULL;
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIController::getNewSnapshot( const char * reason )
{
	IOReturn ret;
	
	// We will always report unknown for the reset state with <rdar://problem/9457884>
	BasebandSPIControllerSnapshot::PinState resetDetectState = BasebandSPIControllerSnapshot::kDunno;	
	ret = takeSnapshot( reason, resetDetectState, &fSavedSnapshot );
	if ( ret == kIOReturnSuccess )
	{
		if ( fProtocol )
		{
			fSavedSnapshot->injectProtocolSnapshot( fProtocol->getSnapshot() );
		}
	}
	return ret;
}

