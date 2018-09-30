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
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHA NTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */
 
#include <sys/param.h>
#include <IOKit/IODeviceTreeSupport.h>

#include "BasebandSPIController.h"
#include "BasebandSPIDebug.h"
#include "BasebandSPIConfig.h"
#include "BasebandSPIDevice.h"
#include "BasebandSPIProtocol.h"

#undef super
#define super IOService

OSDefineMetaClassAndStructors(BasebandSPIDevice, super);

static const char * kInvalidState = "Invalid state %s\n";

static const unsigned kNumPowerStates = 2;

static IOPMPowerState	sPowerStates[kNumPowerStates];

BasebandSPIControllerSnapshot::BasebandSPIControllerSnapshot( const char * comment, PinState resetDetectState )
	: fResetDetectState( resetDetectState )
{
	strncpy( fComment, comment, kMaxCommentSize );
}

/* static */ const char * BasebandSPIControllerSnapshot::pinStateAsString( PinState state )
{
	switch ( state )
	{
	case kHigh:
		return "High";
	case kLow:
		return "Low";
	case kDunno:
	default:
		return "Dunno";
	}
}

IOService * BasebandSPIDevice::probe( IOService * provider, SInt32 * score )
{
	SPI_TRACE( DEBUG_SPEW );
	
	*score = 5000;
	
	return super::probe( provider, score );
}

bool BasebandSPIDevice::init( OSDictionary * dict )
{
	SPI_TRACE( DEBUG_SPEW );

	bool ret = super::init( dict );
	
	SPI_ASSERT( ret, ==, true );
	
	fState			= kStateInactive;
	
	fController		= NULL;
	fProtocol		= NULL;
	
	fCompletionTarget		= NULL;
	fCompletionAction		= NULL;
	
	fLowPowerFlags = 0;
								
	return true;
}

bool BasebandSPIDevice::start( IOService * provider )
{
	SPI_TRACE( DEBUG_SPEW );
	
	//PE_enter_debugger( "blah" );
	
	bool ret = super::start( provider );
	SPI_ASSERT( ret, ==, true );
		
	fController = OSDynamicCast(BasebandSPIController, provider);
	if (!fController){
		SPI_LOG( DEBUG_CRITICAL, "Controller seems invalid\n" );
		return false;
	}
	
		fCommandGate = IOCommandGate::commandGate( this );
	if ( fCommandGate == NULL )
	{
		SPI_LOG( DEBUG_CRITICAL, "Failed to create command gate\n" );
		return false;
	}
	
	SPI_ASSERT( getWorkLoop(), !=, NULL );
	
	IOReturn result;
	
	result = getWorkLoop()->addEventSource( fCommandGate );
	
	if ( result != kIOReturnSuccess )
	{
		fCommandGate->release();
		fCommandGate = NULL;
		SPI_LOG( DEBUG_CRITICAL, "Failed to add command gate to workloop, ret = %p\n", result );
		return false;
	}
	
	fProtocol = BasebandSPIProtocol::create( fController, this );
	if ( !fProtocol )
	{
		SPI_LOG( DEBUG_CRITICAL, "Creating protocol failed\n" );
		return false;
	}
	
	result = initPM();
	if ( result != kIOReturnSuccess )
	{
		SPI_LOG( DEBUG_CRITICAL, "Couldn't initialize PM\n" );
	}
	
	registerService();
								
	return true;
}

void BasebandSPIDevice::stop( IOService * provider )
{
	SPI_TRACE( DEBUG_SPEW );

	(void)provider;

	if ( fProtocol )
	{
		fProtocol->release();
		fProtocol = NULL;
	}
	
	stopPM();
	
	if ( fCommandGate  )
	{
		getWorkLoop()->removeEventSource( fCommandGate );
		fCommandGate->release();
		fCommandGate = NULL;
	}

	super::stop( provider );
}

IOByteCount BasebandSPIDevice::getFrameSize()
{
	return fProtocol->getFrameSize();
}

IOReturn BasebandSPIDevice::registerRxBuffer( IOMemoryDescriptor *md, UInt8 * virtualAddress, void *cookie )
{
	SPI_TRACE( DEBUG_SPEW );
	return fProtocol->registerRxBuffer( md, virtualAddress, cookie );
}

IOReturn BasebandSPIDevice::copyTxBuffer( void *buffer, IOByteCount length, IOByteCount *remaining, bool txNow )
{
	SPI_TRACE( DEBUG_SPEW );
	return fProtocol->copyTxBuffer( (UInt8 *)buffer, length, remaining, txNow );
}

IOReturn BasebandSPIDevice::getControllerSnapshot( BasebandSPIControllerSnapshot ** snapshot )
{
	return fController->getSnapshot( snapshot );
}

UInt32 BasebandSPIDevice::getMaxRxBufferCount()
{
	return fProtocol->getMaxRxBufferCount();
}

const char * BasebandSPIDevice::stateAsString( State state )
{
	switch ( state )
	{
		case kStateInactive:	return "inactive";
		case kStateActive:		return "active";
		case kStateError:		return "error";
		case kStateLowPower:	return "lowpower";
		default:				return "???";
	}
}

IOReturn BasebandSPIDevice::activate( void * target, BasebandSPICompletionAction action )
{
	SPI_TRACE( DEBUG_INFO );
	
	// while this flag is set, we can't activate. We could get in here
	// if we had previuosly failed to complete a proper low power sequence
	// and then CommCenter or someone re-opened us while the system was still
	// in the process of entering sleep
	while( fLowPowerFlags & kFlagSystemInLowPower )
	{
		IOReturn ret;
		ret = fCommandGate->commandSleep( &fLowPowerFlags, THREAD_UNINT );
		
		if ( ret != kIOReturnSuccess )
		{
			SPI_LOG( DEBUG_CRITICAL, "Couldn't wait for system wake\n" );
			return ret;
		}
	}

	switch ( fState )
	{
	case kStateInactive:
		{		
			IOReturn ret;
			
			ret = fController->activate( this, fProtocol );
			SPI_ASSERT_SUCCESS( ret );
			
			fState = kStateActive;
			
			fCompletionTarget	= target;
			fCompletionAction	= action;
						
			fProtocol->activate();
			SPI_ASSERT_SUCCESS( ret );
			
			return kIOReturnSuccess;
		}
		
	default:
		SPI_LOG(DEBUG_CRITICAL, kInvalidState, stateAsString( fState ) );
		return kIOReturnNotPermitted;
	}
}

IOReturn BasebandSPIDevice::deactivate( void )
{
	SPI_TRACE( DEBUG_INFO );
	
	IOReturn ret;

	switch ( fState )
	{
	case kStateLowPower:
		{
			ret = fProtocol->exitLowPower();
			SPI_ASSERT_SUCCESS( ret );
			
		}
		// fall through
				
	case kStateActive:
	case kStateError:
		{
			ret = fProtocol->predeactivate();
			SPI_ASSERT_SUCCESS( ret );
			
			ret = fController->deactivate();
			SPI_ASSERT_SUCCESS( ret );
		}
		break;
		
	default:
		SPI_LOG(DEBUG_CRITICAL, kInvalidState, stateAsString( fState ) );
		return kIOReturnNotPermitted;	
	}
	
	ret = fProtocol->deactivate();
	SPI_ASSERT_SUCCESS( ret );

	fState = kStateInactive;
	
	fLowPowerFlags &= ~ kFlagLowPowerAllowed;
				
	//BasebandSPILogger::report();
	//BasebandSPILogger::clear();
	
	return kIOReturnSuccess;

}

IOReturn BasebandSPIDevice::preEnterLowPower( void )
{
	fLowPowerFlags |= kFlagLowPowerPreEnter;
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIDevice::allowLowPower( void )
{
	fLowPowerFlags |= kFlagLowPowerAllowed;
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIDevice::enterLowPower( void )
{
	SPI_TRACE( DEBUG_INFO );

	fLowPowerFlags &= ~kFlagLowPowerPreEnter;
	switch ( fState )
	{
	case kStateActive:
		{
			if ( !( fLowPowerFlags & kFlagLowPowerAllowed ) )
			{
				fController->reportError( kBasebandSPIReturnPMError, "child driver did not allow low power" );
			}
			else
			{	
				fLowPowerFlags &= ~ kFlagLowPowerAllowed;
			}
		}
		// fall through
	case kStateInactive:
		{
			IOReturn ret = fController->enterLowPower();
			SPI_ASSERT_SUCCESS( ret );
						
			ret = fProtocol->enterLowPower();
			SPI_ASSERT_SUCCESS( ret );
			
			fStateBeforeLowPower	= fState;
			fState					= kStateLowPower;
			
			
			fLowPowerFlags			|= kFlagSystemInLowPower;
		}
		SPI_LOG( DEBUG_INFO, "In Low Power\n" );
		return kIOReturnSuccess;
		
	default:
		SPI_LOG(DEBUG_CRITICAL, kInvalidState, stateAsString( fState ) );
		return kIOReturnNotPermitted;
	}
}

IOReturn BasebandSPIDevice::exitLowPower( void )
{
	SPI_TRACE( DEBUG_INFO );
	
	// always clear the flags
	fLowPowerFlags	&= ~ kFlagSystemInLowPower;	
	fCommandGate->commandWakeup( &fLowPowerFlags );

	switch ( fState )
	{
	case kStateLowPower:
		{
			IOReturn ret = fController->exitLowPower( this, fProtocol );
			SPI_ASSERT_SUCCESS( ret );

			ret = fProtocol->exitLowPower();
			SPI_ASSERT_SUCCESS( ret );

			fState			= fStateBeforeLowPower;
			
			SPI_LOG( DEBUG_INFO, "Exited Low Power\n" );
		}
		return kIOReturnSuccess;
		
	default:
		SPI_LOG( DEBUG_CRITICAL, kInvalidState, stateAsString( fState ) );
		
		return kIOReturnNotPermitted;
	}
}

IOReturn BasebandSPIDevice::initPM( void )
{
	SPI_TRACE( DEBUG_INFO );
	
	static const unsigned kIOPMPowerOff = 0;
	
	bzero( &sPowerStates, sizeof( sPowerStates ) );
	
	sPowerStates[0].version					= kIOPMPowerStateVersion1;
	sPowerStates[0].capabilityFlags			= kIOPMPowerOff;
	sPowerStates[0].outputPowerCharacter	= kIOPMPowerOff;
	sPowerStates[0].inputPowerRequirement	= kIOPMPowerOff;
	
	
	sPowerStates[1].version					= kIOPMPowerStateVersion1;
	sPowerStates[1].capabilityFlags			= kIOPMPowerOn;
	sPowerStates[1].outputPowerCharacter	= kIOPMPowerOn;
	sPowerStates[1].inputPowerRequirement	= kIOPMPowerOn;	

	PMinit();
	
	fController->joinPMtree( this );
	
	IOReturn ret = registerPowerDriver( this, sPowerStates, kNumPowerStates );
	
	if ( ret != kIOReturnSuccess )
	{
		SPI_LOG( DEBUG_CRITICAL, "Couldn't register as power driver, ret = %p\n", 
			(void *)ret );
	}
	
	return ret;
}

IOReturn BasebandSPIDevice::stopPM( void )
{
	SPI_TRACE( DEBUG_INFO );

	PMstop();
	
	return kIOReturnSuccess;
}


IOReturn BasebandSPIDevice::setPowerState( unsigned long whichState, IOService * whatDevice )
{
	IOCommandGate::Action action = OSMemberFunctionCast( IOCommandGate::Action, this,
									&BasebandSPIDevice::setPowerStateGated );
									
	return fCommandGate->runAction( action, (void *)whichState, whatDevice );
}

IOReturn BasebandSPIDevice::setPowerStateGated( unsigned long whichState, IOService * whatDevice )
{
	typedef enum
	{
		kSelectorOff,
		kSelectorOn
	}
	Selector;

	SPI_TRACE( DEBUG_INFO );
	
	IOReturn ret;
	
	switch ( whichState )
	{
	case kSelectorOff:
		SPI_LOG( DEBUG_INFO, "sleeping\n" );
		ret = enterLowPower();
		SPI_ASSERT_SUCCESS( ret );
		break;
		
	case kSelectorOn:
	default:
		SPI_LOG( DEBUG_INFO, "waking\n" );
		exitLowPower();
		break;
	}
	
	return kIOReturnSuccess;
}

bool BasebandSPIDevice::getLogEntry( char * buffer, IOByteCount capacity )
{
	return BasebandSPILogger::popLogEntryAsString( buffer, capacity );
}



BasebandSPIControllerSnapshot::~BasebandSPIControllerSnapshot()
{
	if ( fProtocolSnapshot )
	{
		delete fProtocolSnapshot;
		fProtocolSnapshot = NULL;
	}
}
