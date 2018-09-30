#include <sys/param.h>
#include <IOKit/IOInterruptEventSource.h>
#include <IOKit/IOFilterInterruptEventSource.h>

#include <IOKit/IOBufferMemoryDescriptor.h>

#include "BasebandSPIIFXProtocolVersion1.h"
#include "BasebandSPICommand.h"
#include "BasebandSPIController.h"
#include "BasebandSPILogger.h"

class BasebandSPICommandVersion1: public BasebandSPICommand
{
	OSDeclareDefaultStructors( BasebandSPICommandVersion1 );

	friend class BasebandSPILogger;
	friend class BasebandSPICommand;
	
public:
	static const unsigned kSPIHeaderDataSizeBits		= 12;
	static const unsigned kSPIHeaderMaxDataSize			= ( ( 1UL << kSPIHeaderDataSizeBits ) - 1 );

	typedef struct SPIHeader {
		UInt curr_data_size:kSPIHeaderDataSizeBits;
		UInt more:1;
		UInt crc_err:1;
		UInt pkt_id:2;
		UInt next_data_size:kSPIHeaderDataSizeBits;
		UInt ri:1;
		UInt dcd:1;
		UInt cts_rts:1;
		UInt dsr_dtr:1;
	};

public:
	static BasebandSPICommandVersion1 * create( IODirection direction );

	virtual IOByteCount writeData( UInt8 * source, IOByteCount amount, IOByteCount dataSize );
	virtual void reset( void );
	
protected:
	virtual IOReturn init( IODirection direction );
	
public:
	inline void finalize( IOByteCount framePayloadSize, IOByteCount frameSize );
	
	inline void clearHeader( void )
	{
		SPI_ASSERT( sizeof( SPIHeader ), ==, sizeof( UInt32 ) );
		
		UInt32 * p = (UInt32 *)getHeader();
		
		*p = 0;
	};
	
	inline void assertFlowControl( void )
	{
		getHeader()->cts_rts = 1;
	}
	
	inline void assertMoreToTransfer( void )
	{
		getHeader()->more = 1;
	}
	
	inline bool indicatesFlowControl( void ) const
	{
		return getHeader()->cts_rts;
	};
	
	inline bool indicatesMoreToTransfer( void ) const
	{
		return getHeader()->more;
	}
	
	inline IOByteCount getPayloadLength( void ) const
	{
		return getHeader()->curr_data_size;
	}
	
public:
	static const char * getDebugNameStatic( void )
	{
		return "BasebandSPICommandVersion1";
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

OSDefineMetaClassAndStructors( BasebandSPICommandVersion1, BasebandSPICommand );

/*static */ BasebandSPICommandVersion1 * BasebandSPICommandVersion1::create( IODirection direction )
{
	BasebandSPICommandVersion1 * command = new BasebandSPICommandVersion1();
	
	IOReturn ret = command->init( direction );
	
	if ( ret != kIOReturnSuccess )
	{
		SPI_LOG_STATIC( DEBUG_CRITICAL, "Command initialization failed with error %p\n", ret );
		
		command->release();
		command = NULL;
	}
	
	return command;
}

IOReturn BasebandSPICommandVersion1::init( IODirection direction )
{
	IOReturn ret = BasebandSPICommand::init( direction );
	
	if ( ret != kIOReturnSuccess )
	{
		return ret;
	}
	
	fWriteOffset = 0;
	
	return kIOReturnSuccess;
}

void BasebandSPICommandVersion1::reset( void )
{
	fWriteOffset = 0;
	clearFinalized();
}

void BasebandSPICommandVersion1::finalize( IOByteCount framePayloadSize, IOByteCount frameSize )
{
	SPIHeader * header = getHeader();
	
	// set our size fields
	header->curr_data_size = fWriteOffset;
	header->next_data_size = framePayloadSize;
		
	// store the data from cache to memory
	fMemoryDescriptor->performOperation( kIOMemoryIncoherentIOStore, 0, frameSize );
	
	// mark this as finalized
	setFinalized();
}

IOByteCount BasebandSPICommandVersion1::writeData( UInt8 * source, IOByteCount amount, IOByteCount dataSize )
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

void BasebandSPICommandVersion1::dumpFrameInternal( int logLevel ) const
{
	IOMemoryDescriptor * md = (IOMemoryDescriptor *)getMemoryDescriptor();

	IOMemoryMap * map = md->map();

	SPI_LOG_PLAIN( logLevel, "--------------------------------------------------\n" );
	SPI_LOG_PLAIN( logLevel, "Direction: %s\tHeader: %p(%p), %d\tMD: %p\n", fDirection == kIODirectionOut ? "Tx" : "Rx",
		fBuffer, map->getPhysicalAddress(), sizeof( struct BasebandSPICommandVersion1::SPIHeader ), md );
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


	if (valid && ( getHeader()->curr_data_size != kSPIHeaderMaxDataSize ) ) {
	
		IOByteCount size = MIN( md->getLength(), getHeader()->curr_data_size );

		Dbg_hexdump( logLevel, getPayload(), size );
	}
	
	SPI_LOG_PLAIN( logLevel, "--------------------------------------------------\n" );
}

void BasebandSPICommandVersion1::dumpHeader( int logLevel ) const
{
	Dbg_hexdump(logLevel, getHeader(), sizeof(struct SPIHeader));
	
	SPIHeader * header = getHeader();

	SPI_LOG_PLAIN( logLevel, "\tcurr_data_size: %#3lx\tnext_data_size: %#3lx\n", header->curr_data_size, header->next_data_size );
	SPI_LOG_PLAIN( logLevel, "\tri: %d\tcts: %d\tdcd: %d\tdsr: %d\tmore: %d\n", header->ri, header->cts_rts, header->dcd, header->dsr_dtr, header->more );
	SPI_LOG_PLAIN( logLevel, "\trx_err: %d\tpkt_id: %#x\n", header->crc_err, header->pkt_id );
}


// ------------------------------------------------------------------------------------------

#define super BasebandSPIIFXProtocol
OSDefineMetaClassAndStructors( BasebandSPIIFXProtocolVersion1, BasebandSPIIFXProtocol );


bool BasebandSPIIFXProtocolVersion1::init( BasebandSPIController * controller, BasebandSPIDevice * device )
{
	SPI_TRACE( DEBUG_INFO );
	
	fTransferFlags	= 0;

	bool ret = super::init( controller, device );
	
	if ( !ret )
	{
		return ret;
	}
		
	return true;
}

void BasebandSPIIFXProtocolVersion1::free( void )
{
	SPI_TRACE( DEBUG_INFO );

	super::free();
}

IOReturn BasebandSPIIFXProtocolVersion1::activate( void )
{
	SPI_TRACE( DEBUG_INFO );
	
	fController->beginBlockInterrupt();

	fStateFlags		= 0;
	fLastStateFlags = 0;

	IOReturn ret = super::activate();
	SPI_ASSERT_SUCCESS( ret );
	
	SPI_ASSERT( fTransferFlags, ==, 0 );
	
	fController->endBlockInterrupt();
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIIFXProtocolVersion1::predeactivate( void )
{
	SPI_TRACE( DEBUG_INFO );
	
	fController->beginBlockInterrupt();

	fTransferFlags = 0;
	
	IOReturn ret = super::predeactivate();	
	SPI_ASSERT_SUCCESS( ret );
	
	fController->endBlockInterrupt();
	
	return kIOReturnSuccess;
}

IOInterruptEventAction BasebandSPIIFXProtocolVersion1::getSRDYAction( void ) const
{
	return OSMemberFunctionCast(IOInterruptEventAction, this, &BasebandSPIIFXProtocolVersion1::handleSRDYInterruptAction);
}

#if ( SPI_USE_REAL_ISR || SPI_USE_REAL_ISR_FOR_DEBUG )
IOFilterInterruptAction BasebandSPIIFXProtocolVersion1::getSRDYFilterAction( void ) const
{
	return OSMemberFunctionCast( IOFilterInterruptAction, this, &BasebandSPIIFXProtocolVersion1::handleSRDYFilterInterruptAction );
}
#endif

BasebandSPICommand * BasebandSPIIFXProtocolVersion1::allocateCommand( IODirection direction ) const
{
	return BasebandSPICommandVersion1::create( direction );
}

BasebandSPICommand * BasebandSPIIFXProtocolVersion1::allocateTxCommand( void ) const
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

BasebandSPICommand * BasebandSPIIFXProtocolVersion1::allocateRxCommand( void ) const
{
	return allocateCommand( kIODirectionIn );
}

#if ( SPI_USE_REAL_ISR || SPI_USE_REAL_ISR_FOR_DEBUG )
bool BasebandSPIIFXProtocolVersion1::handleSRDYFilterInterruptAction( IOFilterInterruptEventSource * es )
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

void BasebandSPIIFXProtocolVersion1::handleSRDYInterruptAction(IOInterruptEventSource *es, int count)
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

bool BasebandSPIIFXProtocolVersion1::handleSRDY( void )
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
		SPI_LOG( DEBUG_SPEW, "Deferring SRDY\n" );
		if ( fTransferFlags & kFlagActive )
		{
			fTransferFlags |= kFlagMissedSRDY;
		}
		return true;
	}
}

IOByteCount BasebandSPIIFXProtocolVersion1::getHeaderAndFooterSize( void ) const
{
	return sizeof( BasebandSPICommandVersion1::SPIHeader );
}

void BasebandSPIIFXProtocolVersion1::prepareTransferIfNeeded( UInt32 prepareFlags )
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
	
	if ( !( currentTransferFlags & kFlagActive ) )
	{
		// Only start touching our state if a transfer is not in flight
		
		bool changedFlags = false;
		
		BasebandSPICommandVersion1 * txCommand = NULL;

#if SPI_SRDY_DEBUG
		BasebandSPILogger::logGeneral( "STI0", 0, isSRDYStateHigh() );
#else
		BasebandSPILogger::logGeneral( "STI0", 0 );
#endif
		
		// Do not assert/deassert flow control if Mux is in process of entering low power
		if ( !fDevice->isMuxEnteringLowPower() )
		{
			if ( fLastStateFlags & kFlagMasterFlowControlAsserted )
			{
				// If we asserted flow control, lower it if we have enough frames now
				// and mark that we want to start a transfer to tell the slave that
				// we can receive normally again
				// add 1 because we need to account for the Rx command that would need to be queued for the transfer
		
				SPI_LOG( DEBUG_SPEW, "Rx Commands = %u\n",
					 getNumRxUnusedCommands() );
				if ( getNumRxUnusedCommands() >= 2 + 1 )
				{
#if SPI_SRDY_DEBUG
					BasebandSPILogger::logGeneral( "STI0", 5, isSRDYStateHigh() );
#else	
					BasebandSPILogger::logGeneral( "STI0", 5 );
#endif
					fStateFlags &= ~ kFlagMasterFlowControlAsserted;
					prepareFlags |= kPrepareInitiate;
					changedFlags = true;
				}
			}
			else
			{
				// If flow control is not asserted, if we have run low on buffers
				// ( <= 1, as we need one spare to receive header-only frames)
				// then assert flow control and mark that we want to start a transfer
				// to notify the slave
				// add 1 because we need to account for the Rx command that would need to be queued for the transfer
		
				SPI_LOG( DEBUG_SPEW, "Rx Commands = %u\n",
					 getNumRxUnusedCommands() );

				if ( getNumRxUnusedCommands() <= 1 + 1 )
				{
#if SPI_SRDY_DEBUG
					BasebandSPILogger::logGeneral( "STI0", 1, isSRDYStateHigh() );
#else
					BasebandSPILogger::logGeneral( "STI0", 1 );
#endif	
					fStateFlags |= kFlagMasterFlowControlAsserted;
					prepareFlags |= kPrepareInitiate;
					changedFlags = true;
				}
			}
		}

		if ( !( fTransferFlags & kFlagTxActivated ) )
		{
			// Don't touch Tx if it's already set up
		
			BasebandSPICommandVersion1 * command = NULL;
			
			SPI_LOG( DEBUG_SPEW, "Tx Queue Size %u\n", fCommandQueue->getSize() );
			
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
					command = OSDynamicCast( BasebandSPICommandVersion1, getTxUnusedCommand() );
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
					prepareFlags |= kPrepareInitiate;
					changedFlags = true;
				}
				else if ( fStateFlags & kFlagMasterMoreAsserted )
				{
					// if we have only one left, clear the MORE flag
					// and indicate that we want to start a transfer to notify the slave
#if SPI_SRDY_DEBUG
					BasebandSPILogger::logGeneral( "STI0", 7, isSRDYStateHigh() );
#else
					BasebandSPILogger::logGeneral( "STI0", 7 );
#endif	
					fStateFlags &= ~ kFlagMasterMoreAsserted;
					prepareFlags |= kPrepareInitiate;					
					changedFlags = true;
				}

				if ( fStateFlags & kFlagSlaveFlowControlAsserted )  
				{
					// We need to enter this case everytime and than check if we want to initiate a transfer.
					// Otherwise we would run into the else-condition.
					if ( prepareFlags & kPrepareInitiate ) {
						// if the slave has asserted fow conrol on us, but we want to start a transfer anyway
						// (to send a header-only frame) then get an empty command

#if SPI_SRDY_DEBUG
						BasebandSPILogger::logGeneral( "STI0", 6, isSRDYStateHigh() );
#else
						BasebandSPILogger::logGeneral( "STI0", 6 );
#endif	
						command = OSDynamicCast( BasebandSPICommandVersion1, getTxUnusedCommand() );
#if SPI_PEDANTIC_ASSERT
						SPI_ASSERT( command, !=, NULL );
#endif
					}
				}
				else
				{
					// otherwise just get the command
					BasebandSPICommand * tmp = fCommandQueue->dequeueHead();
			
					command = OSDynamicCast( BasebandSPICommandVersion1, tmp );
#if SPI_PEDANTIC_ASSERT
					SPI_ASSERT( command, !=, NULL );
#endif
					prepareFlags |= kPrepareInitiate;
					
				}
			}
			txCommand = command;
		}
		
		BasebandSPICommandVersion1 * rxCommand	= NULL;
		
		if ( !( currentTransferFlags & kFlagRxActivated ) )
		{
		
			// if we're initiating a transfer then we need to queue up an Rx
			if ( prepareFlags & kPrepareInitiate )
			{
				rxCommand = OSDynamicCast( BasebandSPICommandVersion1, getRxUnusedCommand() );
#if SPI_PEDANTIC_ASSERT
				SPI_ASSERT( rxCommand, !=, NULL );
#endif
						
				// indicate that Rx is activated in our copy, and set the flag to update the field
				currentTransferFlags |= kFlagRxActivated;
#if SPI_SRDY_DEBUG
				BasebandSPILogger::logGeneral( "STIRx", fLastStateFlags, isSRDYStateHigh() );
#else
				BasebandSPILogger::logGeneral( "STIRx", fLastStateFlags );		
#endif
			}
		}
		
		if ( txCommand )
		{
			// we have something to tx (either a real frame or header-only) depending on the code above
			
			// clear the header
			txCommand->clearHeader();
			
			// start filling in bits of the header
			if ( fStateFlags & kFlagMasterFlowControlAsserted )
			{
				txCommand->assertFlowControl();
			}
			
			if (  fStateFlags & kFlagMasterMoreAsserted )
			{
				txCommand->assertMoreToTransfer();
			}
			
			// finalize the command by writing some last minute fields and 
			// flushing the buffer 
			txCommand->finalize( getFramePayloadSize(), getFrameSize() );
			
			BasebandSPILogger::logFrameSent( getFrameSize(), txCommand );

			// set the last state flags to the current state, this keeps track of
			// what the slave thinks our state is
			fLastStateFlags = fStateFlags;		
			
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
#if SPI_SRDY_DEBUG
			BasebandSPILogger::logGeneral( "STI0", 9, isSRDYStateHigh() );
#else
			BasebandSPILogger::logGeneral( "STI0", 9 );
#endif		
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
			startSRDYTimeoutTimer();
			raiseMRDY();
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


IOReturn BasebandSPIIFXProtocolVersion1::handleTransferCancelled( )
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


IOReturn BasebandSPIIFXProtocolVersion1::handleTransferComplete( IODMACommand * receivedCommand, IODMACommand * sendCommand )
{
	SPI_TRACE( DEBUG_SPEW );
	
	BasebandSPICommandVersion1 * command;
	
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
	
		command = OSDynamicCast( BasebandSPICommandVersion1, sendCommand );
		
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
		
		// TODO: this might be redundant
		if ( fCommandQueue->getSize() )
		{
			prepareFlags |= kPrepareInitiate;
		}
				
		addTxUnusedCommand( command );
	}
	
	{
		// we always go in here since we always prepare for an Rx
	
		command = OSDynamicCast( BasebandSPICommandVersion1, receivedCommand );
		
		SPI_ASSERT( command, !=, NULL );
		
		command->dumpFrame( DEBUG_SPEW );
		
		IOByteCount payloadLength = command->getPayloadLength();
		
		if ( payloadLength > getFramePayloadSize() )
		{
			if ( payloadLength < BasebandSPICommandVersion1::kSPIHeaderMaxDataSize )
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
				prepareFlags |= kPrepareInitiate;
			}
			
			if ( command->indicatesFlowControl() )
			{
#if SPI_SRDY_DEBUG
				BasebandSPILogger::logGeneral( "SFLOW", 1, isSRDYStateHigh() );
#else
				BasebandSPILogger::logGeneral( "SFLOW", 1 );
#endif
				fStateFlags |= kFlagSlaveFlowControlAsserted;
			}
			
			SPI_LOG( DEBUG_SPEW, "State flags %p\n", fStateFlags );
			
			// ramp up the controller again
			// we do this here because if there was another pending SRDY, it can transfer
			// while the client processes the frame we received
			prepareTransferIfNeeded( prepareFlags );
			
			if ( payloadLength )
			{
				// if there was an actual payload, mark that this is a real Rx
				callbackFlags |= kBasebandSPIReturnRxReady;
				
				BasebandSPILogger::logFrameReceived( payloadLength + sizeof( BasebandSPICommandVersion1::SPIHeader ), command ); 	
				
				// clear the memory descriptor of the command and put it back in the free pool
				command->clearMemoryDescriptor();
				addRxFreeCommand( command );
				
				// do the actual callback
				BasebandSPILogger::logReceiveCallbackBegin();
				fDevice->invokeCompletionCallback( command->getCookie(), callbackFlags,
					sizeof( BasebandSPICommandVersion1::SPIHeader ), payloadLength );
				BasebandSPILogger::logReceiveCallbackEnd();
			}
			else
			{
				// this was a header-only frame, recycle the command and use it for another receive
				BasebandSPILogger::logFrameReceived( sizeof( BasebandSPICommandVersion1::SPIHeader ), command ); 
			
				addRxUnusedCommand( command );
			}
		}
	}

	return kIOReturnSuccess;
}

IOReturn BasebandSPIIFXProtocolVersion1::handleCancelledRxCommand( IODMACommand * command )
{
	SPI_ASSERT( fRxAvailableBufferCount, <, fRxBufferCount );
	BasebandSPICommandVersion1 * c = OSDynamicCast( BasebandSPICommandVersion1, command );
	SPI_ASSERT( c, !=, NULL );
	addRxUnusedCommand( c );
	return kIOReturnSuccess;
}

IOReturn BasebandSPIIFXProtocolVersion1::handleCancelledTxCommand( IODMACommand * command )
{
	SPI_ASSERT( fTxAvailableBufferCount, <, fTxBufferCount );
	BasebandSPICommandVersion1 * c = OSDynamicCast( BasebandSPICommandVersion1, command );
	SPI_ASSERT( c, !=, NULL );
	addTxUnusedCommand( c );
	return kIOReturnSuccess;
}

IOReturn BasebandSPIIFXProtocolVersion1::enterLowPower( void )
{
	SPI_TRACE( DEBUG_INFO );
	
	IOReturn ret = super::enterLowPower();
	SPI_ASSERT_SUCCESS( ret );
	
	fController->beginBlockInterrupt();
	
	fTransferFlags &= ~ ( kFlagTxActivated | kFlagRxActivated );
	
	fController->endBlockInterrupt();
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIIFXProtocolVersion1::exitLowPower( void )
{
	SPI_TRACE( DEBUG_INFO );
	
	IOReturn ret = super::exitLowPower();
	SPI_ASSERT_SUCCESS( ret );
	
	return kIOReturnSuccess;
}


