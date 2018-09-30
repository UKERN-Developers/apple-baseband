/*
 *  BasebandSPIDiagnostics.h
 *  BasebandSPI
 *
 *  Created by Arjuna Sivasithambaresan on 6/30/08.
 *  Copyright 2008 Apple Inc. All rights reserved.
 *
 */
 
#include "BasebandSPIDebug.h"
#include "BasebandSPIConfig.h"

#include <IOKit/IOLib.h>
#include <IOKit/IOService.h>
#include <IOKit/platform/AppleARMFunction.h>


class BasebandSPIDiagnostics: public OSObject
{
	OSDeclareAbstractStructors(BasebandSPIDiagnostics);

private:
	static const bool kEnabled = false;
	
	static const char * getDebugName( void ) 
	{
		return "BasebandSPIDiagnostics";
	}
	

public:
	static BasebandSPIDiagnostics * withProvider( IOService * provider );
	
private:
	virtual bool init( IOService * provider );
	virtual void free( void );

	inline bool enabled( void ) const
	{
		return kEnabled && fGPIOFunction;
	}
	
private:
	AppleARMFunction * fGPIOFunction;
	UInt32			fGPIOLevel;
	
public:
	void markRaiseMRDYMasterInitiated( void )
	{
		if ( enabled() )
		{
			markRaiseMRDYMasterInitiatedInternal();
		}
	}
	
	void markRaiseMRDYSlaveInitiated( void )
	{
		if ( enabled() )
		{
			markRaiseMRDYSlaveInitiatedInternal();
		}
	}
	
	void markSRDYInterrupt( void )
	{
		if ( enabled() )
		{
			markSRDYInterruptInternal();
		}
	}
	
	void markDMACompletion( void )
	{
		if ( enabled() )
		{
			markDMACompletionInternal();
		}
	}
	
	void markRxHandlingComplete( void )
	{
		if ( enabled() )
		{
			markRxHandlingCompleteInternal();
		}

	}
	
private:
	void markRaiseMRDYMasterInitiatedInternal( void );
	void markRaiseMRDYSlaveInitiatedInternal( void );
	void markSRDYInterruptInternal( void );
	void markDMACompletionInternal( void );
	void markRxHandlingCompleteInternal( void );
	
	void setGPIO( UInt32 high );
	void toggleGPIO( void ) 
	{
		fGPIOLevel = fGPIOLevel ? 0 : 1;
		setGPIO( fGPIOLevel );
	};
};
