/*
 *  AppleS5LFamilyBasebandSPIController.h
 *  BasebandSPI
 *
 *  Created by Arjuna Sivasithambaresan on 6/16/08.
 *  Copyright 2008 Apple Inc. All rights reserved.
 *
 */
 
#ifndef __APPLES5LFAMILYBASEBANDSPICONTROLLER_H__
#define __APPLES5LFAMILYBASEBANDSPICONTROLLER_H__
 
#include "BasebandSPIController.h"
#include "BasebandSPIConfig.h"

#include <IOKit/platform/AppleARMIODevice.h>

class AppleS5LFamilyBasebandSPIController: public BasebandSPIController
{
	OSDeclareAbstractStructors(AppleS5LFamilyBasebandSPIController);

private:		
	IOMemoryMap						* fMemoryMap;
	IOLogicalAddress				fBaseAddress;
	
	bool							fClockEnabled;
	
protected:
	UInt64							fPCLKFrequency;
	UInt64							fNCLKFrequency;
	
protected:
	const char * getDebugName( void ) const
	{
		return "AppleS5LFamilyBasebandSPIController";
	}
	
	virtual IOReturn takeSnapshot( const char * comment, BasebandSPIControllerSnapshot::PinState resetDetectState, BasebandSPIControllerSnapshot ** snapshot ) = 0;
			
protected:
	UInt32 readReg32(UInt32 offset);
	void writeReg32(UInt32 offset, UInt32 data);

private:
	// setup and teardown
	inline IOReturn configurePins( void );
	
	inline IOReturn createFunctions( void );
	inline IOReturn releaseFunctions( void );
	
protected:
	virtual IOReturn loadConfiguration( void );
	
	void enableClock( bool enable )
	{
		SPI_LOG( DEBUG_SPEW, "Enable Clock from %u to %u\n", fClockEnabled, enable );
		fProvider->enableDeviceClock( enable );
		fClockEnabled = enable;
	}
	
	bool clockEnabled( void ) const
	{
		SPI_LOG( DEBUG_SPEW, "Clock enabled %u\n", fClockEnabled );
		return fClockEnabled;
	}
	
public:
	virtual bool init( OSDictionary * dict );
	virtual bool start( IOService *provider );
	virtual void stop( IOService *provider );	
};


inline UInt32 AppleS5LFamilyBasebandSPIController::readReg32(UInt32 offset)
{
	return *(volatile UInt32 *)( fBaseAddress + offset);
}

inline void AppleS5LFamilyBasebandSPIController::writeReg32(UInt32 offset, UInt32 data)
{
	*(volatile UInt32 *)( fBaseAddress + offset) = data;
}


#endif
