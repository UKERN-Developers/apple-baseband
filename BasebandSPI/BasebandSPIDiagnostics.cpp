/*
 *  BasebandSPIDiagnostics.cpp
 *  BasebandSPI
 *
 *  Created by Arjuna Sivasithambaresan on 6/30/08.
 *  Copyright 2008 Apple Inc. All rights reserved.
 *
 */

#include "BasebandSPIDiagnostics.h"

#define super OSObject

OSDefineMetaClassAndStructors( BasebandSPIDiagnostics, super );

BasebandSPIDiagnostics * BasebandSPIDiagnostics::withProvider( IOService * provider )
{
	BasebandSPIDiagnostics * me = new BasebandSPIDiagnostics();
	
	bool ret = me->init( provider );
	
	if ( !ret )
	{
		delete me;
		me = NULL;
	}
	
	return me;
}

bool BasebandSPIDiagnostics::init( IOService * provider )
{
	static const char * kGPIOKey = "function-diag_gpio1";

	if ( !kEnabled )
	{
		fGPIOFunction = NULL;
		return true;
	}

	fGPIOFunction = AppleARMFunction::withProvider(provider, kGPIOKey );
	
	if ( !fGPIOFunction )
	{
		SPI_LOG( DEBUG_CRITICAL, "Warning: Couldn't find function for '%s'\n", kGPIOKey );
	}
	
	fGPIOLevel = 0;
	
	return true;
}

void BasebandSPIDiagnostics::setGPIO( UInt32 high )
{
	IOReturn ret = fGPIOFunction->callFunction( &high );
	if ( ret != kIOReturnSuccess )
	{
		SPI_LOG( DEBUG_CRITICAL, "Failed to toggle gpio\n" );
	}
}

void BasebandSPIDiagnostics::markRaiseMRDYMasterInitiatedInternal( void )
{
	toggleGPIO();
}

void BasebandSPIDiagnostics::markRaiseMRDYSlaveInitiatedInternal( void )
{
	toggleGPIO();
}

void BasebandSPIDiagnostics::markSRDYInterruptInternal( void )
{
	toggleGPIO();
}

void BasebandSPIDiagnostics::markDMACompletionInternal( void )
{
	toggleGPIO();
}

void BasebandSPIDiagnostics::markRxHandlingCompleteInternal( void )
{
	toggleGPIO();
}


void BasebandSPIDiagnostics::free( void )
{
	fGPIOFunction->release();
	fGPIOFunction = NULL;
}
