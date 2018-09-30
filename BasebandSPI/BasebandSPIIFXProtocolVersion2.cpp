#include <sys/param.h>

#include <IOKit/IOBufferMemoryDescriptor.h>

#include "BasebandSPIIFXProtocolVersion2.h"
#include "BasebandSPICommand.h"
#include "BasebandSPIController.h"
#include "BasebandSPILogger.h"

class BasebandSPICommandVersion2: public BasebandSPICommand
{
	OSDeclareDefaultStructors( BasebandSPICommandVersion2 );

	friend class BasebandSPILogger;
	friend class BasebandSPICommand;
	
public:
	static const unsigned kSPIHeaderDataSizeBits		= 12;
	static const unsigned kSPIHeaderCreditBits			= 12; 
	static const unsigned kSPIHeaderMaxDataSize			= ( ( 1UL << kSPIHeaderDataSizeBits ) - 1 );

	typedef struct SPIHeader {
		UInt curr_size:kSPIHeaderDataSizeBits;
		UInt ext_more:1;
		UInt rx_err:1;
		UInt credit_reset:1;
		UInt rsvd2:1;
		UInt credits:kSPIHeaderCreditBits;
		UInt ri:1;
		UInt dcd:1;
		UInt rsvd:1;
		UInt dsr_dtr:1;
	};

public:
	static BasebandSPICommandVersion2 * create( IODirection direction );

	virtual IOByteCount writeData( UInt8 * source, IOByteCount amount, IOByteCount dataSize );

	virtual void reset( void );
	
protected:
	virtual IOReturn init( IODirection direction );
	
public:
	inline void finalize( IOByteCount framePayloadSize, IOByteCount frameSize, UInt32 creditsGranted );
	
	inline void clearHeader( void )
	{
		SPI_ASSERT( sizeof( SPIHeader ), ==, sizeof( UInt32 ) );
		
		UInt32 * p = (UInt32 *)getHeader();
		
		*p = 0;
	};
	
	inline void assertMoreToTransfer( void )
	{
		getHeader()->ext_more = 1;
	}
	
	inline bool indicatesMoreToTransfer( void ) const
	{
		return getHeader()->ext_more;
	}
	
	inline void assertCreditReset( void )
	{
		getHeader()->credit_reset = 1;
	}
	
	inline bool indicatesRxError( void ) const
	{
		return getHeader()->rx_err;
	};
	
	inline IOByteCount getPayloadLength( void ) const
	{
		return getHeader()->curr_size;
	}
	
	inline UInt32 getGrantedCredits( void ) const
	{
		return getHeader()->credits;
	}
	
public:
	static const char * getDebugNameStatic( void )
	{
		return "BasebandSPICommandVersion2";
	};

	virtual const char * getDebugName( void ) const
	{
		return getDebugNameStatic();
	};
	
	void dumpFrameInternal( int logLevel ) const;
	
private:
	inline void dumpHeader( int logLevel ) const;
	
private:
	inline SPIHeader * getHeader( void ) const
	{
		return (SPIHeader *)fBuffer;
	}
	
	inline UInt8 * getPayload( void ) const
	{
		return fBuffer + sizeof( SPIHeader );
	};
	
private:
	IOByteCount		fWriteOffset;

};

OSDefineMetaClassAndStructors( BasebandSPICommandVersion2, BasebandSPICommand );

/*static */ BasebandSPICommandVersion2 * BasebandSPICommandVersion2::create( IODirection direction )
{
	BasebandSPICommandVersion2 * command = new BasebandSPICommandVersion2();
	
	IOReturn ret = command->init( direction );
	
	if ( ret != kIOReturnSuccess )
	{
		SPI_LOG_STATIC( DEBUG_CRITICAL, "Command initialization failed with error %p\n", ret );
		
		command->release();
		command = NULL;
	}
	
	return command;
}

IOReturn BasebandSPICommandVersion2::init( IODirection direction )
{
	IOReturn ret = BasebandSPICommand::init( direction );
	
	if ( ret != kIOReturnSuccess )
	{
		return ret;
	}
	
	fWriteOffset = 0;
	
	return kIOReturnSuccess;
}

void BasebandSPICommandVersion2::reset( void )
{
	fWriteOffset = 0;
	clearFinalized();
}

void BasebandSPICommandVersion2::finalize( IOByteCount framePayloadSize, IOByteCount frameSize, UInt32 grantedCredits )
{
	SPIHeader * header = getHeader();
	
	
	// set our size fields
	header->curr_size = fWriteOffset;		
	
	header->credits = grantedCredits;

#if 0
	if ( fWriteOffset == 0 )
	{
		UInt8 * p = getPayload();
		UInt32 remaining = frameSize - fWriteOffset - sizeof( SPIHeader );
		
		for ( UInt32 i = 0; i < remaining; i++ )
		{
			switch ( i & 0x3 )
			{
			case 0:
				p[i] = 0x00;
				break;
				
			case 1:
				p[i] = 0x01;
				break;
				
			case 2:
				p[i] = 0x01;
				break;
				
			case 3:
			default:
				p[i] = 0x03;
				break;
			}
		}
	}
#endif
	
	// store the data from cache to memory
	fMemoryDescriptor->performOperation( kIOMemoryIncoherentIOStore, 0, frameSize );
	
	// mark this as finalized
	setFinalized();
}

IOByteCount BasebandSPICommandVersion2::writeData( UInt8 * source, IOByteCount amount, IOByteCount dataSize )
{
	IOByteCount remaining = dataSize - fWriteOffset;
	
	
	// if we have less room than allows, then only write for how much room we have
	if ( remaining < amount )
	{
		amount = remaining;
	}
	
	// copy the data over
	memcpy( getPayload() + fWriteOffset, source, amount );
	
	// increase the write offset
	fWriteOffset += amount;
	 
	 return amount;
}

void BasebandSPICommandVersion2::dumpFrameInternal( int logLevel ) const
{
	IOMemoryDescriptor * md = (IOMemoryDescriptor *)getMemoryDescriptor();

	IOMemoryMap * map = md->map();

	SPI_LOG_PLAIN( logLevel, "--------------------------------------------------\n" );
	SPI_LOG_PLAIN( logLevel, "Direction: %s\tHeader: %p(%p), %d\tMD: %p\n", fDirection == kIODirectionOut ? "Tx" : "Rx",
		fBuffer, map->getPhysicalAddress(), sizeof( struct BasebandSPICommandVersion2::SPIHeader ), md );
	SPI_LOG_PLAIN( logLevel, "..................................................\n" );

	map->release();
		
	bool valid = fBuffer != NULL;

	if ( valid )
	{
		dumpHeader( logLevel );
	}
	else
	{
		SPI_LOG_PLAIN( logLevel, "INVALID FRAME\n" );
	}
	
	SPI_LOG_PLAIN( logLevel, "..................................................\n" );


	if (valid && ( getHeader()->curr_size != kSPIHeaderMaxDataSize ) ) {
	
		IOByteCount size = MIN( md->getLength(), getHeader()->curr_size );
		if ( size == 0 )
		{
			size = 1024;
		}

		Dbg_hexdump( logLevel, getPayload(), size );
	}
	
	SPI_LOG_PLAIN( logLevel, "--------------------------------------------------\n" );
}

void BasebandSPICommandVersion2::dumpHeader( int logLevel ) const
{
	Dbg_hexdump(logLevel, getHeader(), sizeof(struct SPIHeader));
	
	SPIHeader * header = getHeader();

	SPI_LOG_PLAIN( logLevel, "\tcurr_size: %p\tcredits: %p\n", header->curr_size, header->credits );
	SPI_LOG_PLAIN( logLevel, "\text_more: %p\trx_err: %p\tri: %p\n", header->ext_more, header->rx_err, header->ri );
	SPI_LOG_PLAIN( logLevel, "\tdcd: %p\tdsr_dtr: %p\n", header->dcd, header->dsr_dtr );
}


// ------------------------------------------------------------------------------------------

#define super BasebandSPIIFXProtocol
OSDefineMetaClassAndStructors( BasebandSPIIFXProtocolVersion2, BasebandSPIIFXProtocol );


bool BasebandSPIIFXProtocolVersion2::init( BasebandSPIController * controller, BasebandSPIDevice * device )
{
	SPI_TRACE( DEBUG_INFO );
	
	fTransferFlags	= 0;
	
	fCurrentRxCredits	= 0;
	fSlaveRxCredits		= fCurrentRxCredits;
	
	fCurrentTxCredits	= 0;

	bool ret = super::init( controller, device );
	
	if ( !ret )
	{
		return ret;
	}
	
	return true;
}

void BasebandSPIIFXProtocolVersion2::free( void )
{
	super::free();
}

IOInterruptEventAction BasebandSPIIFXProtocolVersion2::getSRDYAction( void ) const
{
	return OSMemberFunctionCast(IOInterruptEventAction, this, &BasebandSPIIFXProtocolVersion2::handleSRDYInterruptAction);
}

#if ( SPI_USE_REAL_ISR || SPI_USE_REAL_ISR_FOR_DEBUG )
IOFilterInterruptAction BasebandSPIIFXProtocolVersion2::getSRDYFilterAction( void ) const
{
	return OSMemberFunctionCast( IOFilterInterruptAction, this, &BasebandSPIIFXProtocolVersion2::handleSRDYFilterInterruptAction );
}
#endif

IOReturn BasebandSPIIFXProtocolVersion2::activate( void )
{
	SPI_TRACE( DEBUG_INFO );
	
	fController->beginBlockInterrupt();

	IOReturn ret = super::activate();
	SPI_ASSERT_SUCCESS( ret );
	
	fStateFlags		= kFlagMasterCreditReset;
	fLastStateFlags = kFlagMasterCreditReset;
	
	SPI_ASSERT( fTransferFlags, ==, 0 );
	
	fController->endBlockInterrupt();
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIIFXProtocolVersion2::predeactivate( void )
{
	SPI_TRACE( DEBUG_INFO );
	
	fController->beginBlockInterrupt();

	fTransferFlags		= 0;
	fCurrentRxCredits	= 0;
	
	IOReturn ret = super::predeactivate();
	SPI_ASSERT_SUCCESS( ret );
	
	fController->endBlockInterrupt();
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIIFXProtocolVersion2::unregisterAllRxBuffers( void )
{
	SPI_TRACE( DEBUG_INFO );
	
	IOReturn ret = super::unregisterAllRxBuffers();
	SPI_ASSERT_SUCCESS( ret );
	
	fCurrentRxCredits	= 0;
	fSlaveRxCredits		= 0;
	fCurrentTxCredits	= 0;
	
	return kIOReturnSuccess;
}

BasebandSPICommand * BasebandSPIIFXProtocolVersion2::allocateCommand( IODirection direction ) const
{
	return BasebandSPICommandVersion2::create( direction );
}

BasebandSPICommand * BasebandSPIIFXProtocolVersion2::allocateTxCommand( void ) const
{
	BasebandSPICommand * command = allocateCommand( kIODirectionOut );
	if ( command )
	{
		IOBufferMemoryDescriptor * bmd = IOBufferMemoryDescriptor::withOptions( kIOMapDefaultCache, getFrameSize(), 4 );
		if ( bmd )
		{
			command->setMemoryDescriptor( bmd, (UInt8 *)bmd->getBytesNoCopy(), NULL );
		}
		else
		{
			command->release();
			command = NULL;
		}
	}
	
	return command;
}

BasebandSPICommand * BasebandSPIIFXProtocolVersion2::allocateRxCommand( void ) const
{
	return allocateCommand( kIODirectionIn );
}

#if ( SPI_USE_REAL_ISR || SPI_USE_REAL_ISR_FOR_DEBUG )
bool BasebandSPIIFXProtocolVersion2::handleSRDYFilterInterruptAction( IOFilterInterruptEventSource * es )
{
	(void)es;
	
#if SPI_USE_REAL_ISR_FOR_DEBUG
	BasebandSPILogger::logSRDYAsserted();
	return true;
#else
	return handleSRDY();
#endif
}
#endif

void BasebandSPIIFXProtocolVersion2::handleSRDYInterruptAction(IOInterruptEventSource *es, int count)
{
	(void)es;
	(void)count;

	SPI_TRACE( DEBUG_SPEW );
#if !SPI_USE_REAL_ISR
	bool powerUp = handleSRDY();
	if ( !powerUp )
	{
		return;
	}
#endif

	if ( !canHandleSRDY() || fController->isEnteringLowPower() )
	{
		BasebandSPILogger::logGeneral( "SRDYI", 1 );
		return;
	}

	prepareTransferIfNeeded( kPrepareInitiate | kPrepareStart );
	
}

bool BasebandSPIIFXProtocolVersion2::handleSRDY( void )
{
	bool allowed = fController->intendToInterrupt();
	
	BasebandSPILogger::logGeneral( "SRDYH", 0 );
	
	if ( !allowed )
	{
		return false;
	}
	
	if ( fController->canStartTransfer() )
	{
		BasebandSPILogger::logGeneral( "SRDYH", 1 );

		fTransferFlags |= kFlagActive;
		IOReturn ret = fController->startTransfer();
		SPI_ASSERT_SUCCESS( ret );
		return false;
	}
	else
	{
		BasebandSPILogger::logGeneral( "SRDYH", 2 );
	
		SPI_LOG( DEBUG_SPEW, "Deferring SRDY\n" );
		if ( fTransferFlags & kFlagActive )
		{
			fTransferFlags |= kFlagMissedSRDY;
		}
		return true;
	}
}

IOByteCount BasebandSPIIFXProtocolVersion2::getHeaderAndFooterSize( void ) const
{
	return sizeof( BasebandSPICommandVersion2::SPIHeader );
}

void BasebandSPIIFXProtocolVersion2::prepareTransferIfNeeded( UInt32 prepareFlags )
{
	SPI_TRACE( DEBUG_SPEW );
	
	fController->beginBlockInterrupt();

	UInt32 currentTransferFlags = fTransferFlags;
	
	SPI_LOG( DEBUG_SPEW, "prepare = %p, transferFlags = %p, state flags = %p\n", prepareFlags, currentTransferFlags, fStateFlags );

#if SPI_SRDY_DEBUG
	BasebandSPILogger::logGeneral( "STI", currentTransferFlags, isSRDYStateHigh() );
#else
	BasebandSPILogger::logGeneral( "STI", currentTransferFlags );
#endif
	
	if ( prepareFlags & kPrepareRxRegistered )
	{
		// if a new buffer was registered, then increment the credit count
		fCurrentRxCredits++;
		SPI_LOG( DEBUG_SPEW, "Increased credits to %u\n", fCurrentRxCredits );

#if SPI_SRDY_DEBUG
		BasebandSPILogger::logGeneral( "CRDIRx", ( fSlaveRxCredits << 16 ) | fCurrentRxCredits, isSRDYStateHigh() );
#else
		BasebandSPILogger::logGeneral( "CRDIRx", ( fSlaveRxCredits << 16 ) | fCurrentRxCredits );
#endif
	}

	if ( !( currentTransferFlags & kFlagActive ) )
	{
		// Only start touching our state if a transfer is not in flight
		
		bool changedFlags = false;
		BasebandSPICommandVersion2 * txCommand = NULL;	
#if SPI_SRDY_DEBUG
		BasebandSPILogger::logGeneral( "STI0", 0, isSRDYStateHigh() );
#else
		BasebandSPILogger::logGeneral( "STI0", 0 );
#endif		
		UInt32 creditsGranted;
				
		if ( !( fTransferFlags & kFlagTxActivated ) )
		{
			// Don't touch Tx if it's already set up
		
			BasebandSPICommandVersion2 * command = NULL;
			
			SPI_LOG( DEBUG_SPEW, "Tx Queue Size %u\n", fCommandQueue->getSize() );
										
			SPI_LOG( DEBUG_SPEW, "Slave credits %u vs current %u\n", fSlaveRxCredits, fCurrentRxCredits );
			
			creditsGranted	= fCurrentRxCredits - fSlaveRxCredits;
			
			UInt32 minimumDifference = MIN( kRxCreditDifferenceThreshold, fSlaveRxCredits );

			// Do not send out a credit update if Mux is in process of entering low power
			if ( ( creditsGranted > minimumDifference ) &&  !( fDevice->isMuxEnteringLowPower() ) )
			{
				// if the number of credits the baseband thinks we have is
				// different from what we really have then we need to notify BB
				prepareFlags |= kPrepareInitiate;
				changedFlags = true;
			}
			
			if ( fStateFlags & kFlagMasterMoreAsserted )
			{
				// Clear the MORE flag unconditionally
				// and indicate that we want to start a transfer to notify the slave
				// In case we have more data to send this flag will be set again below. 
				// But in case we ran out we should signal the BB that no further frames are expected
				fStateFlags &= ~ kFlagMasterMoreAsserted;
				prepareFlags |= kPrepareInitiate;					
				changedFlags = true;
			}
			
			if ( fCommandQueue->isEmpty() )
			{			
#if SPI_SRDY_DEBUG
				BasebandSPILogger::logGeneral( "STI0", 3, isSRDYStateHigh() );
#else
				BasebandSPILogger::logGeneral( "STI0", 3 );
#endif
				if ( changedFlags )
				{
					// if we have no real payload to send, but we want to start a transfer anyway
					// (to send a header-only frame) then get an empty command
#if SPI_SRDY_DEBUG
					BasebandSPILogger::logGeneral( "STI0", 4, isSRDYStateHigh() );
#else
					BasebandSPILogger::logGeneral( "STI0", 4 );
#endif
					command = OSDynamicCast( BasebandSPICommandVersion2, getTxUnusedCommand() );
				}
			}
			else
			{
				if ( fCurrentTxCredits == 0 ) 
				{
					
					// Only get a Unused commands if the flags have changed and we want to initiate a transfer
					// kPrepareInitiate is always set when we want to Tx or if we receive a SRDY interrupt
					if ( changedFlags && ( prepareFlags & kPrepareInitiate ) ) {
						// if we can't send real data, then we need to send a
						// header-only frame then get an empty command
#if SPI_SRDY_DEBUG
						BasebandSPILogger::logGeneral( "STI0", 6, isSRDYStateHigh() );
#else
						BasebandSPILogger::logGeneral( "STI0", 6 );
#endif
						command = OSDynamicCast( BasebandSPICommandVersion2, getTxUnusedCommand() );
					}
				}
				else
				{
					
					if ( fCommandQueue->getSize() > 1 )
					{
						// If there is more than command in the queue, then mark the MORE flag
						// and indicate that we want to start a transfer to notify the slave
#if SPI_SRDY_DEBUG
						BasebandSPILogger::logGeneral( "STI0", 2, isSRDYStateHigh() );
#else
						BasebandSPILogger::logGeneral( "STI0", 2 );
#endif
						fStateFlags |= kFlagMasterMoreAsserted;
						changedFlags = true;
					}
					
					// otherwise just get the command
					BasebandSPICommand * tmp = fCommandQueue->dequeueHead();
					
					command = OSDynamicCast( BasebandSPICommandVersion2, tmp );
#if SPI_PEDANTIC_ASSERT
					SPI_ASSERT( command, !=, NULL );
					SPI_ASSERT( fCurrentTxCredits, >, 0 );
#endif

#if SPI_SRDY_DEBUG
					BasebandSPILogger::logGeneral( "CRDDTx", fCurrentTxCredits, isSRDYStateHigh() );
#else
					BasebandSPILogger::logGeneral( "CRDDTx", fCurrentTxCredits );
#endif					
					// This is a real payload, we need to decrement our credit count
					fCurrentTxCredits--;
					
					prepareFlags |= kPrepareInitiate;
				}
				
			}
		
			txCommand = command;
		}
		
		BasebandSPICommandVersion2 * rxCommand = NULL; 

		
		if ( !( currentTransferFlags & kFlagRxActivated ) )
		{
			// only start touching Rx if there isn't already one active
			
			if ( prepareFlags & kPrepareInitiate )
			{
				rxCommand = OSDynamicCast( BasebandSPICommandVersion2, getRxUnusedCommand() );
#if SPI_PEDANTIC_ASSERT
				SPI_ASSERT( rxCommand, !=, NULL );
#endif
	
#if SPI_SRDY_DEBUG
				BasebandSPILogger::logGeneral( "STIRx", fLastStateFlags, isSRDYStateHigh() );
#else
				BasebandSPILogger::logGeneral( "STIRx", fLastStateFlags );
#endif
				
				// indicate that Rx is activated in our copy, and set the flag to update the field
				currentTransferFlags |= kFlagRxActivated;
				
				
				// <rdar://problem/8629724> CoreDump generated on 8E89 -- SPI Frame 0xBFFFFFFF uplink [Case Number : 00007223]
				if ( txCommand == NULL )
				{
					txCommand = OSDynamicCast( BasebandSPICommandVersion2, getTxUnusedCommand() );					
				}
			}
		}
								
		if ( txCommand )
		{
			// we have something to tx (either a real frame or header-only) depending on the code above
			
			// clear the header
			txCommand->clearHeader();
			
			// start filling in bits of the header				
			if (  fStateFlags & kFlagMasterMoreAsserted )
			{
#if SPI_SRDY_DEBUG
				BasebandSPILogger::logGeneral( "MMORE", 1, isSRDYStateHigh() );
#else
				BasebandSPILogger::logGeneral( "MMORE", 1 );
#endif	
				txCommand->assertMoreToTransfer();
			}
			
			if ( fStateFlags & kFlagMasterCreditReset )
			{
#if SPI_SRDY_DEBUG
				BasebandSPILogger::logGeneral( "CRDRST", 0, isSRDYStateHigh() );
#else
				BasebandSPILogger::logGeneral( "CRDRST", 0 );
#endif
				fStateFlags &= ~ kFlagMasterCreditReset;
				txCommand->assertCreditReset();
			}
			
							
			fSlaveRxCredits	= fCurrentRxCredits;
			SPI_ASSERT( (SInt32)creditsGranted, >=, 0 );
			
			SPI_LOG( DEBUG_SPEW, "Granting credits %u\n", creditsGranted );

#if SPI_SRDY_DEBUG
			BasebandSPILogger::logGeneral( "CRDGRx", creditsGranted, isSRDYStateHigh() );
#else
			BasebandSPILogger::logGeneral( "CRDGRx", creditsGranted );
#endif
			
			// finalize the command by writing some last minute fields and 
			// flushing the buffer 
			
			txCommand->finalize( getFramePayloadSize(), getFrameSize(), creditsGranted );
			
			BasebandSPILogger::logFrameSent( getFrameSize(), txCommand );

			// set the last state flags to the current state, this keeps track of
			// what the slave thinks our state is
			fLastStateFlags		= fStateFlags;	
			
			// mark that Tx is active in the local copy
			currentTransferFlags |= kFlagTxActivated;

#if SPI_SRDY_DEBUG
			BasebandSPILogger::logGeneral( "STITx", fLastStateFlags, isSRDYStateHigh() );
#else
			BasebandSPILogger::logGeneral( "STITx", fLastStateFlags );
#endif
		}
		
		if ( rxCommand || txCommand )
		{
			// configure the hardware
			IOReturn ret = fController->configure( rxCommand, txCommand, getFrameSize() );
			SPI_ASSERT_SUCCESS( ret );
		}
				
		if ( prepareFlags & kPrepareStart )
		{
			// if we want to actually start the transfer, start it
			// this implies the precondition that the slave has already
			// raised SRDY and we deferred handling it
			fController->startTransfer();
			
			// mark that the transfer is active in our local copy
			currentTransferFlags	|= kFlagActive; 
		}
		else if ( prepareFlags & kPrepareInitiate )
		{
			// if we want to initiate the transfer, raise MRDY
			raiseMRDY();
			startSRDYTimeoutTimer();
		}
		
		// if we touched the flags, copy it over
		fTransferFlags = currentTransferFlags;
		
		SPI_LOG( DEBUG_SPEW, "Current State flags %p\n", fStateFlags );
	}
	
	bool missed = fController->endBlockInterrupt();
	
	if ( missed )
	{
		// if we missed an SRDY while we were blocking it, handle it now
		handleSRDY();
	}
}

IOReturn BasebandSPIIFXProtocolVersion2::handleTransferCancelled( )
{
	SPI_TRACE( DEBUG_SPEW );

	lowerMRDY();
	stopSRDYTimeoutTimer();
	
	fController->beginBlockInterrupt();
	fController->unconfigure();
	fController->endBlockInterrupt();
	
	// We need to clean up the transfer flags as well
	// Commands are already placed back to the queue
	fTransferFlags = 0;

	return kIOReturnSuccess;
}


IOReturn BasebandSPIIFXProtocolVersion2::handleTransferComplete( IODMACommand * receivedCommand, IODMACommand * sendCommand )
{
	SPI_TRACE( DEBUG_SPEW );
	
	BasebandSPICommandVersion2 * command;
	
	SPI_LOG( DEBUG_SPEW, "Transfer flags = %p\n", fTransferFlags );
	
	// if we didn't indicate MORE last time, lower MRDY
	if ( !( fLastStateFlags & kFlagMasterMoreAsserted ) )
	{
		lowerMRDY();
	}
	
	stopSRDYTimeoutTimer();
	
	fController->beginBlockInterrupt();
	fController->unconfigure();
	
	bool missed = fController->endBlockInterrupt();
		
	UInt32 prepareFlags;
	
	// if we missed an SRDY while we blocked the interupt 
	// or we missed it because our transfer was active, mark that we
	// want to initiate and start a transfer
	if ( missed || ( fTransferFlags & kFlagMissedSRDY )  )
	{
		prepareFlags = kPrepareInitiate | kPrepareStart;
	}
	else
	{
		prepareFlags = 0;
	}
	
	fTransferFlags = 0;
	
	UInt32 callbackFlags = 0;
	
	if ( sendCommand )
	{
		// only go in here if we sent a valid frame (either with real payload or header-only)
	
		command = OSDynamicCast( BasebandSPICommandVersion2, sendCommand );
		
#if SPI_PEDANTIC_ASSERT
		SPI_ASSERT( command, !=, NULL );
#endif
		
		command->dumpFrame( DEBUG_SPEW );

		// If we were low on Tx commands, mark that we want to notify our
		// client that a Tx buffer is now free
		if ( getNumTxUnusedCommands() == 1 )
		{
			callbackFlags |= kBasebandSPIReturnTxReady;
		}
		
		
		// <rdar://problem/8730644> AP might initiate Tx transfers unnecessarily
		/*
		if ( fCommandQueue->getSize() )
		{
			prepareFlags |= kPrepareInitiate;
		}
		*/
		
		addTxUnusedCommand( command );		
	}
	
	{
		// we always go in here since we always prepare for an Rx
	
		command = OSDynamicCast( BasebandSPICommandVersion2, receivedCommand );
		
		SPI_ASSERT( command, !=, NULL );
		
		command->dumpFrame( DEBUG_SPEW );
		
		IOByteCount payloadLength = command->getPayloadLength();
		
		if ( payloadLength > getFramePayloadSize() )
		{
			if ( payloadLength < BasebandSPICommandVersion2::kSPIHeaderMaxDataSize )
			{
				// the payload length is greater than what we expected, but the field isn't all 1's (in binary)
				// so dump the frame as we shouldn't get here
				SPI_LOG( DEBUG_CRITICAL, "Recieved frame of payload size %u, frame size %u\n",
					payloadLength, getFramePayloadSize() );
					
				command->dumpFrame( DEBUG_CRITICAL );
			}
			else
			{
				// the frame was all 1's (in binary) and thus the slave had nothing to send us
			}
			
			addRxUnusedCommand( command );
			
			
			// ramp up the controller agin
			prepareTransferIfNeeded( prepareFlags );
		}
		else
		{
			// add the number of credits we have
			fCurrentTxCredits += command->getGrantedCredits();			
#if SPI_SRDY_DEBUG
			BasebandSPILogger::logGeneral( "CRDGTx", (command->getGrantedCredits() << 16 ) | fCurrentTxCredits, isSRDYStateHigh() );
#else
			BasebandSPILogger::logGeneral( "CRDGTx", (command->getGrantedCredits() << 16 ) | fCurrentTxCredits );
#endif
		
			// set our state to only the master flags as we be reading in the new slave flags
			fStateFlags = fStateFlags & kMasterStateFlagMask;
			
			// go through what we received, set flags in our state accordingly
			if ( command->indicatesMoreToTransfer() )
			{
#if SPI_SRDY_DEBUG
				BasebandSPILogger::logGeneral( "SMORE", 1, isSRDYStateHigh() );
#else
				BasebandSPILogger::logGeneral( "SMORE", 1 );
#endif	
				fStateFlags |= kFlagSlaveMoreAsserted;
				prepareFlags |= kPrepareInitiate | kPrepareStart;
			}
			
			// if there is an error reported, throw up an error
			if ( command->indicatesRxError() )
			{
				fController->reportError( kBasebandSPIReturnProtocolError, "Rx Error" );
			}
							
			SPI_LOG( DEBUG_SPEW, "State flags %p\n", fStateFlags );
			
			// ramp up the controller again
			// we do this here because if there was another pending SRDY, it can transfer
			// while the client processes the frame we received
			prepareTransferIfNeeded( prepareFlags );
			
			if ( payloadLength )
			{
#if SPI_PEDANTIC_ASSERT
				SPI_ASSERT( fSlaveRxCredits, >, 0 );
#endif

				// we received real data, so the baseband consumed a credit
				fSlaveRxCredits--;
				fCurrentRxCredits--;
#if SPI_SRDY_DEBUG
				BasebandSPILogger::logGeneral( "CRDDRx", ( fSlaveRxCredits << 16 ) | fCurrentRxCredits, isSRDYStateHigh() );
#else
				BasebandSPILogger::logGeneral( "CRDDRx", ( fSlaveRxCredits << 16 ) | fCurrentRxCredits );
#endif				
				SPI_LOG( DEBUG_SPEW, "Slave consumed %u credits\n", fCurrentRxCredits );
							
				// if there was an actual payload, mark that this is a real Rx
				callbackFlags |= kBasebandSPIReturnRxReady;
				
				BasebandSPILogger::logFrameReceived( payloadLength + sizeof( BasebandSPICommandVersion2::SPIHeader ), command ); 	
				
				// clear the memory descriptor of the command and put it back in the free pool
				command->clearMemoryDescriptor();
				addRxFreeCommand( command );
				
				// do the actual callback
				BasebandSPILogger::logReceiveCallbackBegin();
				fDevice->invokeCompletionCallback( command->getCookie(), callbackFlags,
					sizeof( BasebandSPICommandVersion2::SPIHeader ), payloadLength );
				BasebandSPILogger::logReceiveCallbackEnd();
			}
			else
			{
				// this was a header-only frame, recycle the command and use it for another receive
				BasebandSPILogger::logFrameReceived( sizeof( BasebandSPICommandVersion2::SPIHeader ), command ); 
			
				addRxUnusedCommand( command );
			}
		}
	}

	return kIOReturnSuccess;
}

IOReturn BasebandSPIIFXProtocolVersion2::handleCancelledRxCommand( IODMACommand * command )
{
	SPI_ASSERT( fRxAvailableBufferCount, <, fRxBufferCount );
	BasebandSPICommandVersion2 * c = OSDynamicCast( BasebandSPICommandVersion2, command );
	SPI_ASSERT( c, !=, NULL );
	addRxUnusedCommand( c );
	return kIOReturnSuccess;
}

IOReturn BasebandSPIIFXProtocolVersion2::handleCancelledTxCommand( IODMACommand * command )
{
	SPI_ASSERT( fTxAvailableBufferCount, <, fTxBufferCount );
	BasebandSPICommandVersion2 * c = OSDynamicCast( BasebandSPICommandVersion2, command );
	SPI_ASSERT( c, !=, NULL );
	addTxUnusedCommand( c );
	return kIOReturnSuccess;
}

IOReturn BasebandSPIIFXProtocolVersion2::enterLowPower( void )
{
	SPI_TRACE( DEBUG_INFO );
	
	IOReturn ret = super::enterLowPower();
	SPI_ASSERT_SUCCESS( ret );
	
	fController->beginBlockInterrupt();
	
	fTransferFlags &= ~ ( kFlagTxActivated | kFlagRxActivated );
	
	fController->endBlockInterrupt();
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIIFXProtocolVersion2::exitLowPower( void )
{
	SPI_TRACE( DEBUG_INFO );
	
	IOReturn ret = super::exitLowPower();
	SPI_ASSERT_SUCCESS( ret );

	return kIOReturnSuccess;
}
