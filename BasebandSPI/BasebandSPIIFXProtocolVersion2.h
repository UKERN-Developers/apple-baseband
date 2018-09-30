#ifndef _BasebandSPIIFXProtocolVersion2_H_
#define _BasebandSPIIFXProtocolVersion2_H_

#include "BasebandSPIConfig.h"
#include "BasebandSPIIFXProtocol.h"

class BasebandSPICommandQueue;

class BasebandSPIIFXProtocolVersion2 : public BasebandSPIIFXProtocol
{
	OSDeclareDefaultStructors( BasebandSPIIFXProtocolVersion2 );

	friend class BasebandSPIProtocol;

private:
	typedef enum
	{
		kFlagTxActivated	= 1UL << 0,
		kFlagRxActivated	= 1UL << 1,
		kFlagActive			= 1UL << 2,
		kFlagMissedSRDY		= 1UL << 3
	} TransferFlags;
	
	typedef enum
	{
		kFlagMasterMoreAsserted				= 1UL << 1,
		kFlagMasterCreditReset				= 1UL << 2,
		kFlagSlaveMoreAsserted				= 1UL << 17
	} StateFlags;
	
	static const UInt16		kMasterStateFlagMask	= 0xFF;
	static const UInt16		kSlaveStateFlagMask		= 0xFF << 8;
	
	static const UInt32		kRxCreditDifferenceThreshold	= 8;

private:
	virtual const char * getDebugName( void ) const
	{
		return "BasebandSPIIFXProtocolVersion2";
	};

protected:
	virtual bool init( BasebandSPIController * controller, BasebandSPIDevice * device );
	virtual void free( void );
	
public:
	virtual IOReturn activate( void );
	virtual IOReturn predeactivate( void );
	virtual IOReturn unregisterAllRxBuffers( void );

	virtual IOReturn handleTransferComplete( IODMACommand * receivedCommand, IODMACommand * sendCommand );
	virtual IOReturn handleTransferCancelled( void );
	
	virtual IOReturn handleCancelledRxCommand( IODMACommand * command );
	virtual IOReturn handleCancelledTxCommand( IODMACommand * command );
	
	virtual IOReturn enterLowPower( void );
	virtual IOReturn exitLowPower( void );
	
protected:
	virtual BasebandSPICommand * allocateTxCommand( void ) const;
	virtual BasebandSPICommand * allocateRxCommand( void ) const;
	
	virtual IOByteCount getHeaderAndFooterSize( void ) const;
	
	virtual void prepareTransferIfNeeded( UInt32 prepareFlags );
	
protected:
	virtual IOInterruptEventAction getSRDYAction( void ) const;
#if ( SPI_USE_REAL_ISR || SPI_USE_REAL_ISR_FOR_DEBUG )
	virtual IOFilterInterruptAction getSRDYFilterAction( void ) const;
#endif
	
private:
#if ( SPI_USE_REAL_ISR || SPI_USE_REAL_ISR_FOR_DEBUG )
	bool handleSRDYFilterInterruptAction( IOFilterInterruptEventSource * filterES );
#endif

	void handleSRDYInterruptAction(IOInterruptEventSource *es, int count);		
	bool handleSRDY( void );


	BasebandSPICommand * allocateCommand( IODirection direction ) const;
		
private:
	volatile UInt32		fTransferFlags;
	UInt16				fStateFlags;
	UInt16				fLastStateFlags;
	
	UInt32				fCurrentRxCredits;
	UInt32				fSlaveRxCredits;
	
	UInt32				fCurrentTxCredits;	
};

#endif
