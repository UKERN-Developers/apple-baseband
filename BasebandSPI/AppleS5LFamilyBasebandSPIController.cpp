/*
 *  AppleS5LFamilyBasebandSPIController.cpp
 *  BasebandSPI
 *
 *  Created by Arjuna Sivasithambaresan on 6/16/08.
 *  Copyright 2008 Apple Inc. All rights reserved.
 *
 */
 
#include <libkern/OSAtomic.h>

#include <IOKit/platform/AppleARMFunction.h>
#include <IOKit/platform/AppleARMIODevice.h>

#include "AppleS5LFamilyBasebandSPIController.h"
#include "BasebandSPIDiagnostics.h"

#define super BasebandSPIController

OSDefineMetaClassAndAbstractStructors( AppleS5LFamilyBasebandSPIController, super );

bool AppleS5LFamilyBasebandSPIController::init( OSDictionary * dict )
{
	fMemoryMap						= NULL;
	fBaseAddress					= 0;
	
	fPCLKFrequency					= 0;
	fNCLKFrequency					= 0;
	
	bool ret = super::init( dict );
	
	if ( !ret )
	{
		return ret;
	}
	
	return true;
}

bool AppleS5LFamilyBasebandSPIController::start( IOService * provider )
{
	SPI_TRACE( DEBUG_INFO );

	if ( !super::start(provider) ) 
	{
		return false;
	}
	
	IOReturn ret;
	
	ret = configurePins();
	if ( ret != kIOReturnSuccess )
	{
		return false;
	}
		
	ret = createFunctions();
	if ( ret != kIOReturnSuccess )
	{
		return false;
	}
				
	return true;
}

void AppleS5LFamilyBasebandSPIController::stop(IOService *provider)
{
	releaseFunctions();
	
	super::stop( provider );
}

IOReturn AppleS5LFamilyBasebandSPIController::configurePins( void )
{
	AppleARMFunction * function;

	// Reconfigure IPC_MOSI. iBoot sets it as input
	function = AppleARMFunction::withProvider(fProvider, "function-mosi");
	if (!function)
	{
		SPI_LOG( DEBUG_CRITICAL, "Couldn't get function-mosi\n" );
		return kIOReturnError;
	}
	
	function->callFunction();
	
	function->release();
	
	return kIOReturnSuccess;
}

IOReturn AppleS5LFamilyBasebandSPIController::createFunctions( void )
{
	return kIOReturnSuccess;
}

IOReturn AppleS5LFamilyBasebandSPIController::releaseFunctions( void )
{
	return kIOReturnSuccess;
}

IOReturn AppleS5LFamilyBasebandSPIController::loadConfiguration( void )
{
	IOReturn ret = super::loadConfiguration();
	
	if ( ret != kIOReturnSuccess )
	{
		return ret;
	}

	if ( !fConfiguration.spiClockPeriod )
	{
		SPI_LOG( DEBUG_CRITICAL, "SPI clock period == 0\n" );
		return kIOReturnBadArgument;
	}
	
	if ((((fNCLKFrequency * fConfiguration.spiClockPeriod) + 1000000000ULL - 1) / 1000000000ULL) > 2048) {
		SPI_LOG( DEBUG_CRITICAL, "Invalid SPI clock period %u\n", fConfiguration.spiClockPeriod);
		return kIOReturnBadArgument;
	}
		
	// S5L8920X supports only 8, 16 & 32 bit words
	switch ( fConfiguration.spiWordSize )
	{
		case 8:
		case 16:
		case 32:
			break;
		default:
			SPI_LOG( DEBUG_CRITICAL, "Invalid SPI word size %u\n", fConfiguration.spiWordSize);
			return kIOReturnBadArgument;
	}
	
		SPI_TRACE( DEBUG_SPEW );
	
	// get our clock frequencies
	fPCLKFrequency = fProvider->getClockFrequency("pclk");
	if (!fPCLKFrequency) return false;

	fNCLKFrequency = fProvider->getClockFrequency("nclk");
	if (!fNCLKFrequency) return false;

	// get our memory map and base address
	fMemoryMap = fProvider->mapDeviceMemoryWithIndex(0);
	if (!fMemoryMap) return false;
	fBaseAddress = fMemoryMap->getVirtualAddress();

	SPI_LOG( DEBUG_INFO, "baseAddress %p, physical %p\n", fBaseAddress,
		fMemoryMap->getPhysicalAddress() );
	
	return kIOReturnSuccess;
}
