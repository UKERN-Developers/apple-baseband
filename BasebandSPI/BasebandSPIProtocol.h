#ifndef _BASEBANDSPIPROTOCOL_H_
#define _BASEBANDSPIPROTOCOL_H_

#include "BasebandSPIConfig.h"
#include "BasebandSPIDebug.h"
#include "BasebandSPILogger.h"
#include "BasebandSPICommand.h"
#include "BasebandSPICommandQueue.h"

#include <IOKit/IOMemoryDescriptor.h>
#include <IOKit/IOCommandPool.h>

class BasebandSPIController;
class BasebandSPIDevice;

class BasebandSPIProtocolSnapshot
{
protected:
	BasebandSPIProtocolSnapshot()
	{
	}
	
public:
	virtual ~ BasebandSPIProtocolSnapshot()
	{
	}

public:
	virtual IOByteCount copyAsString( char * dest, IOByteCount capacity ) const = 0;
};

class BasebandSPIProtocol : public OSObject
{
	OSDeclareAbstractStructors( BasebandSPIProtocol );
	
protected:
	typedef enum
	{
		kPrepareInitiate	= 1UL << 0,
		kPrepareStart		= 1UL << 1,
		kPrepareRxRegistered = 1UL << 2
	}
	PrepareFlags;

public:
	static BasebandSPIProtocol * create( BasebandSPIController * controller, BasebandSPIDevice * device );
	
	virtual bool isSRDYStateHigh ( void ) = 0;

protected:	
	virtual bool init( BasebandSPIController * controller, BasebandSPIDevice * device );
	virtual void free( void );
	
protected:
	static const char * getDebugNameStatic( void )
	{
		return "BasebandSPIProtocol";
	};
	virtual const char * getDebugName( void ) const = 0;
		
	IOWorkLoop * getWorkLoop( void ) const;
	
	inline void getTime (uint64_t * time) const
	{
		*time = mach_absolute_time();
	}
	
	unsigned getRemainingTimeLeftMicroSec ( const uint64_t end, const uint64_t start, unsigned differenceMicroseconds);
	
public:
	virtual BasebandSPIProtocolSnapshot * getSnapshot( void ) const = 0;
	
public:
	IOReturn copyTxBuffer( UInt8 * source, IOByteCount length, IOByteCount * remaining, bool transmitNow );
	IOReturn registerRxBuffer( IOMemoryDescriptor *md, UInt8 * virtualAddress, void * cookie );	
	
	inline IOByteCount getFrameSize( void ) const
	{
		return fFrameSize;
	}
	
	inline IOByteCount getFramePayloadSize( void ) const
	{
		return fFramePayloadSize;
	};
	
	inline IOByteCount getMaxRxBufferCount( void ) const
	{
		return fRxBufferCount;
	}
	
	virtual IOReturn activate( void );
	virtual IOReturn predeactivate( void );
	virtual IOReturn deactivate( void );
	virtual IOReturn unregisterAllRxBuffers( void );
	
	virtual IOReturn handleTransferComplete( IODMACommand * receivedCommand, IODMACommand * sendCommand ) = 0;
	virtual IOReturn handleTransferCancelled( void ) = 0;
	
	virtual IOReturn handleCancelledRxCommand( IODMACommand * command ) = 0;
	virtual IOReturn handleCancelledTxCommand( IODMACommand * command ) = 0;
	
	virtual IOReturn enterLowPower( void );
	virtual IOReturn exitLowPower( void );
	
private:
	inline IOReturn allocateCommands( void );
	inline IOReturn freeCommands( void );
	
	inline IOReturn flushTxBuffers( void );
		
protected:
	virtual IOByteCount getHeaderAndFooterSize( void ) const = 0;
	
	virtual BasebandSPICommand * allocateTxCommand( void ) const = 0;
	virtual BasebandSPICommand * allocateRxCommand( void ) const = 0;
	
	virtual void prepareTransferIfNeeded( UInt32 prepareFlags ) = 0;
	
protected:
	IOReturn addEventSource( IOEventSource ** es, const char * name );

	inline BasebandSPICommand * getTxPendingCommand( void ) const
	{
#if SPI_PEDANTIC_ASSERT
		SPI_ASSERT( fCommandQueue, !=, NULL );
#endif	
		BasebandSPICommand * command = fCommandQueue->peekTail();
		if ( command )
		{
			if ( command->isFinalized() )
			{
				command = NULL;
			}
		}
		
		return command;
	};
	
	inline BasebandSPICommand * getTxUnusedCommand( void )
	{
		if ( fTxAvailableBufferCount )
		{
			fTxAvailableBufferCount--;
			return (BasebandSPICommand *)fTxDMACommandPool->getCommand( false );
		}
		else
		{
			return NULL;
		}

	};
	
	inline BasebandSPICommand * getRxFreeCommand( void )
	{
		return (BasebandSPICommand *)fRxFreeCommandPool->getCommand( false );
	};
	
	inline BasebandSPICommand * getRxUnusedCommand( void )
	{
		if ( fRxAvailableBufferCount )
		{
			fRxAvailableBufferCount--;
			return (BasebandSPICommand *)fRxDMACommandPool->getCommand( false );
		}
		else
		{
			return NULL;
		}
	};
	
	inline void addTxUnusedCommand( BasebandSPICommand * command )
	{
		// reset the command so we can reuse it
		command->reset();
		fTxAvailableBufferCount++;
		SPI_LOG( DEBUG_SPEW, "Increase to %u\n", (unsigned)fTxAvailableBufferCount );
		fTxDMACommandPool->returnCommand( command );
	}
	
	inline void addRxFreeCommand( BasebandSPICommand * command )
	{
		fRxFreeCommandPool->returnCommand( command );
	};
	
	inline void addRxUnusedCommand( BasebandSPICommand * command )
	{
		// reset the command so we can reuse it
		command->reset();
		fRxAvailableBufferCount++;
		SPI_LOG( DEBUG_SPEW, "Increase to %u\n", (unsigned)fRxAvailableBufferCount );
		fRxDMACommandPool->returnCommand( command );
	};
	
	inline UInt32 getNumTxUnusedCommands( void ) const
	{
		return fTxAvailableBufferCount;
	};
	
	inline UInt32 getNumRxUnusedCommands(void) const
	{
		return fRxAvailableBufferCount;
	};
	
	bool getUInt32PropertyFromController( const char * name, UInt32 * device );
	
	static bool getUInt32PropertyWithProvider( IOService * provider, const char * name, UInt32 * device );
	
protected:
	BasebandSPIController			* fController;
	BasebandSPIDevice				* fDevice;
	BasebandSPICommandQueue			* fCommandQueue;
	
	bool				fInLowPower;

	IOByteCount			fFramePayloadSize;
	UInt32				fRxBufferCount;
	UInt32				fTxBufferCount;
	
	IOByteCount			fFrameSize;
	
	UInt32				fRxAvailableBufferCount;
	UInt32				fTxAvailableBufferCount;
	
	IOCommandPool		* fRxFreeCommandPool;
	IOCommandPool		* fRxDMACommandPool;
	IOCommandPool		* fTxDMACommandPool;
};

#endif
