#ifndef _BASEBANDSPIIFXPROTOCOL_H_
#define _BASEBANDSPIIFXPROTOCOL_H_

#include "BasebandSPIProtocol.h"

#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/IOFilterInterruptEventSource.h>
#include <IOKit/IOTimerEventSource.h>

#define SPI_PROTOCOL_RERAISE_MRDY		0

class AppleARMFunction;


class BasebandSPIIFXProtocol: public BasebandSPIProtocol
{
	OSDeclareAbstractStructors( BasebandSPIIFXProtocol );
	
protected:
#if SPI_PROTOCOL_RERAISE_MRDY
	static const UInt32		kSRDYTimeoutInterval			= 500;
#else
	static const UInt32		kSRDYTimeoutInterval			= 2*1000;
#endif
	
public:
	virtual bool init( BasebandSPIController * controller, BasebandSPIDevice * device );
	virtual void free( void );
	
public:
	virtual BasebandSPIProtocolSnapshot * getSnapshot( void ) const;
	
public:
	virtual IOReturn activate( void );
	virtual IOReturn predeactivate( void );

	virtual bool isSRDYStateHigh ( void );
	
	virtual IOReturn exitLowPower( void );

protected:
	virtual IOReturn createEventSources( void );
	virtual IOReturn releaseEventSources( void );
	
	virtual IOReturn createFunctions( void );
	virtual IOReturn releaseFunctions( void );
	
protected:
	virtual IOInterruptEventAction getSRDYAction( void ) const = 0;
#if ( SPI_USE_REAL_ISR || SPI_USE_REAL_ISR_FOR_DEBUG )
	virtual IOFilterInterruptAction getSRDYFilterAction( void ) const = 0;
#endif

	virtual void handleSRDYInterruptAction(IOInterruptEventSource *es, int count) = 0;


protected:
	void setMRDY( UInt32 value ); 
	
	inline void raiseMRDY( void )
	{
		setMRDY( 1 );
	}
	inline void lowerMRDY( void )
	{
		setMRDY( 0 );
	}
	
	inline void startSRDYTimeoutTimer( void )
	{
		fSRDYTimeoutEventSource->setTimeoutMS( kSRDYTimeoutInterval );
	}
	
	inline void stopSRDYTimeoutTimer( void )
	{
		clearSRDYMissCount();
		fSRDYTimeoutEventSource->cancelTimeout();
	}
	
	bool canHandleSRDY( void ) const
	{
		return fAllowSRDY && !fInLowPower && fRxAvailableBufferCount;
	}
	
	void handleSRDYTimeoutAction( IOTimerEventSource * es );
	
protected:
	void clearSRDYMissCount( void )
	{
#if SPI_PROTOCOL_RERAISE_MRDY
		fSRDYMissCount = 0;
#endif
	}
	
#if SPI_PROTOCOL_RERAISE_MRDY
	UInt32 fSRDYMissCount;
#endif
	
protected:
	AppleARMFunction	* fMRDYFunction;
	AppleARMFunction	* fSRDYFunction;
	
	IOInterruptEventSource	* fSRDYEventSource;
	IOTimerEventSource		* fSRDYTimeoutEventSource;
	
	uint64_t			fMRDYModificationTime;
	
	UInt32				fMRDYState;
	
private:
	bool				fAllowSRDY;

};

#endif
