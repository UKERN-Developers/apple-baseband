#include "BasebandSPIProtocol.h"
#include "BasebandSPIDevice.h"
#include "BasebandSPIController.h"
#include "BasebandSPICommandQueue.h"
#include "BasebandSPIIFXProtocolVersion1.h"
#include "BasebandSPIIFXProtocolVersion2.h"

#define super OSObject

OSDefineMetaClassAndAbstractStructors( BasebandSPIProtocol, super );

/* static */ BasebandSPIProtocol * BasebandSPIProtocol::create( BasebandSPIController * controller, BasebandSPIDevice * device )
{
	UInt32 protocolVersion;
	
	if ( !getUInt32PropertyWithProvider( controller->getProvider(), "protocol-version", &protocolVersion ) )
	{
		return NULL;
	}

	BasebandSPIProtocol * protocol;
	
	switch( protocolVersion )
	{
	case 1:
		protocol = new BasebandSPIIFXProtocolVersion1();
		break;
		
	case 2:
		protocol = new BasebandSPIIFXProtocolVersion2();
		break;
	
	default:
		SPI_LOG_STATIC( DEBUG_CRITICAL, "unrecognized version %u\n", (unsigned)protocolVersion );
		return NULL;
	}
	
	if ( !protocol->init( controller, device ) )
	{
		delete protocol;
		protocol = NULL;
	}
	
	return protocol;
}

bool BasebandSPIProtocol::init( BasebandSPIController * controller, BasebandSPIDevice * device )
{
	SPI_TRACE( DEBUG_SPEW );

	fController			= NULL;
	fDevice				= NULL;
	fCommandQueue		= NULL;
	fFramePayloadSize	= NULL;
	fRxBufferCount		= 0;
	fTxBufferCount		= 0;
	fRxFreeCommandPool	= NULL;
	fRxDMACommandPool	= NULL;
	fTxDMACommandPool	= NULL;
	
	fInLowPower			= false;
	
	if ( !super::init() )
	{
		return false;
	}

	fController			= controller;
	fDevice				= device;
	
	fCommandQueue = new BasebandSPICommandQueue();
	
	fRxFreeCommandPool = IOCommandPool::withWorkLoop( fController->getWorkLoop() );
	if ( !fRxFreeCommandPool )
	{
		return false;
	}
	
	fRxDMACommandPool = IOCommandPool::withWorkLoop( fController->getWorkLoop() );
	if ( !fRxDMACommandPool )
	{
		return false;
	}
	
	fTxDMACommandPool = IOCommandPool::withWorkLoop( fController->getWorkLoop() );
	if ( !fTxDMACommandPool )
	{
		return false;
	}
	
	if ( !getUInt32PropertyFromController( "max-data-size", (UInt32 *)&fFramePayloadSize ) )
	{
		return false;
	}
	
	if ( !getUInt32PropertyFromController( "rx-buffer-count", &fRxBufferCount ) )
	{
		return false;
	}

	if ( !getUInt32PropertyFromController( "tx-buffer-count", &fTxBufferCount ) )
	{
		return false;
	}
	
	fFrameSize = fFramePayloadSize + getHeaderAndFooterSize();

	return true;
}

void BasebandSPIProtocol::free( void )
{
	SPI_TRACE( DEBUG_SPEW );

	if ( fRxFreeCommandPool )
	{
		fRxFreeCommandPool->release();
		fRxFreeCommandPool = NULL;
	}
	
	if ( fRxDMACommandPool )
	{
		fRxDMACommandPool->release();
		fRxDMACommandPool = NULL;
	}

	if ( fTxDMACommandPool )
	{
		fTxDMACommandPool->release();
		fTxDMACommandPool = NULL;
	}

	if ( fCommandQueue )
	{
		delete fCommandQueue;
		fCommandQueue = NULL;
	}
	
	super::free();
}

IOWorkLoop * BasebandSPIProtocol::getWorkLoop( void ) const
{
	return fController->getWorkLoop();
}


unsigned BasebandSPIProtocol::getRemainingTimeLeftMicroSec ( const uint64_t end, const uint64_t start, unsigned differenceMicroseconds)
{
	uint64_t diff = end - start;
	uint64_t left = 0;
	uint64_t delay;
	
	nanoseconds_to_absolutetime((differenceMicroseconds * 1000), (AbsoluteTime *)&delay);
	
	if (diff < delay) 
	{
		uint64_t absDiff = delay - diff;
		absolutetime_to_nanoseconds(*(AbsoluteTime *)&absDiff, &left);
		
		left /= 1000;
	}
	
	return ( (unsigned) left);
	
}


IOReturn BasebandSPIProtocol::activate( void )
{
	SPI_TRACE( DEBUG_SPEW );
	
	IOReturn ret;
	
	ret = allocateCommands();
	if ( ret != kIOReturnSuccess )
	{
		return ret;
	}
		
	return kIOReturnSuccess;
}

IOReturn BasebandSPIProtocol::predeactivate( void )
{
	return kIOReturnSuccess;
}

IOReturn BasebandSPIProtocol::deactivate( void )
{
	SPI_TRACE( DEBUG_SPEW );

	IOReturn ret;
	
	ret = unregisterAllRxBuffers();
	SPI_ASSERT_SUCCESS( ret );
	
	ret = flushTxBuffers();
	SPI_ASSERT_SUCCESS( ret );
	
	ret = freeCommands();	
	if ( ret != kIOReturnSuccess )
	{
		return ret;
	}
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIProtocol::enterLowPower( void )
{
	IOReturn ret;
	ret = flushTxBuffers();
	SPI_ASSERT_SUCCESS( ret );
	
	SPI_ASSERT( fInLowPower, ==, false );
	fInLowPower = true;
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIProtocol::exitLowPower( void )
{
	SPI_ASSERT( fInLowPower, ==, true );
	fInLowPower = false;
	
	fDevice->invokeCompletionCallback( NULL, kBasebandSPIReturnTxReady, 0, 0 );
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIProtocol::unregisterAllRxBuffers( void )
{
	SPI_TRACE( DEBUG_SPEW );

	UInt32 count = 0;
	while ( 1 )
	{
		BasebandSPICommand * command = getRxUnusedCommand();
		
		if ( command )
		{
			SPI_LOG( DEBUG_SPEW, "Returning memory descriptor %p with cookie %p\n", 
				command->getMemoryDescriptor(), command->getCookie() );
				
			fDevice->invokeCompletionCallback( command->getCookie(), kBasebandSPIReturnRxFreed, 0, 0 );
			count++;
			command->clearMemoryDescriptor();
			addRxFreeCommand( command );
		}
		else
		{
			break;
		}
	}
	
	SPI_ASSERT( fRxDMACommandPool->getCommand( false ), ==, NULL );
	
	SPI_LOG( DEBUG_INFO, "Freed %u commands\n", (unsigned)count );
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIProtocol::flushTxBuffers( void )
{
	SPI_TRACE( DEBUG_SPEW );
	
	UInt32 size = fCommandQueue->getSize();
	
	SPI_LOG( DEBUG_INFO, "Have %u buffers to flush\n", (unsigned)size );
	
	while ( size )
	{
		BasebandSPICommand * command = fCommandQueue->dequeueHead();
		SPI_ASSERT( command, !=, NULL );
		
		addTxUnusedCommand( command );
		
		size--;
	}
	
	SPI_ASSERT( fCommandQueue->isEmpty(), ==, true );
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIProtocol::addEventSource( IOEventSource ** es, const char * name )
{
	if ( !( *es ) )
	{
		SPI_LOG( DEBUG_CRITICAL, "Couldn't get %s event source\n", name );
		return kIOReturnError;
	}
	
	IOReturn ret;
	
	ret = fDevice->getWorkLoop()->addEventSource( *es );
	
	if ( ret != kIOReturnSuccess )
	{
		( *es )->release();
		( *es ) = NULL;
		
		SPI_LOG( DEBUG_CRITICAL, "Couldn't add source for %s, ret = %p\n", name, (void *)ret );
		return ret;
	}
	
	return kIOReturnSuccess;
}

/* static */ bool BasebandSPIProtocol::getUInt32PropertyWithProvider( IOService * provider, const char * name, UInt32 * dest )
{
	OSData * data;
	
	data = OSDynamicCast( OSData, provider->getProperty( name ) );
	if ( !data )
	{
		SPI_LOG_STATIC( DEBUG_CRITICAL, "Property '%s' not found\n", name );
		return false;
	}
	
	*dest = *( (UInt32 *)data->getBytesNoCopy() );

	return true;
}

bool BasebandSPIProtocol::getUInt32PropertyFromController( const char * name, UInt32 * dest )
{
	return getUInt32PropertyWithProvider( fController->getProvider(), name, dest );
}

IOReturn BasebandSPIProtocol::allocateCommands( void )
{
	SPI_TRACE( DEBUG_SPEW );

	for ( unsigned i = 0; i < fRxBufferCount; i++ )
	{
		BasebandSPICommand * command = allocateRxCommand();
		if ( !command )
		{
			SPI_LOG( DEBUG_CRITICAL, "Failed to allocate Rx command %u\n", i );
			return kIOReturnNoMemory;
		}
		
		addRxFreeCommand( command );
	}
	
	SPI_LOG( DEBUG_INFO, "Allocated Rx %u commands\n", (unsigned)fRxBufferCount );
	
	for ( unsigned i = 0; i < fTxBufferCount; i++ )
	{
		BasebandSPICommand * command = allocateTxCommand();
		if ( !command )
		{
			SPI_LOG( DEBUG_CRITICAL, "Failed to allocate Tx command %u\n", i );
			return kIOReturnNoMemory;
		}
		
		addTxUnusedCommand( command );
	}
	
	SPI_LOG( DEBUG_INFO, "Allocated Tx %u commands\n", (unsigned)fRxBufferCount );
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIProtocol::freeCommands( void )
{
	SPI_TRACE( DEBUG_SPEW );

	SPI_ASSERT( getRxUnusedCommand(), ==, NULL );
	
	unsigned i;
	for ( i = 0; ; i++ )
	{
		BasebandSPICommand * command = getRxFreeCommand();
		if ( command )
		{
			command->release();
		}
		else
		{
			break;
		}
	}
	
	SPI_ASSERT( fRxFreeCommandPool->getCommand( false ), ==, NULL );
	SPI_ASSERT( i, ==, fRxBufferCount );
	
	for ( i = 0; ; i++ )
	{
		BasebandSPICommand * command = getTxUnusedCommand();
		if ( command )
		{
			command->release();
		}
		else
		{
			break;
		}
	}

	SPI_ASSERT( fTxDMACommandPool->getCommand( false ), ==, NULL );	
	SPI_ASSERT( i, ==, fTxBufferCount );
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIProtocol::copyTxBuffer( UInt8 * source, IOByteCount length, IOByteCount * remaining, bool transmitNow )
{
	SPI_TRACE( DEBUG_SPEW );

#if SPI_PEDANTIC_ASSERT
	SPI_ASSERT( length, !=, 0 );
	SPI_ASSERT( source, !=, NULL );
	
	SPI_ASSERT( getWorkLoop()->inGate(), ==, true );
#endif

	if ( fInLowPower )
	{
		*remaining = length;
		return kIOReturnSuccess;
	}

	BasebandSPICommand * command = getTxPendingCommand();

#if SPI_SRDY_DEBUG
	BasebandSPILogger::logGeneral( "CTx", 0, isSRDYStateHigh() );
#else
	BasebandSPILogger::logGeneral( "CTx", 0 );
#endif
	
	bool isPending;
	
	if ( command )
	{
		isPending = true;
	}
	else
	{
		if ( getNumTxUnusedCommands() < 2 )
		{
			*remaining = length;
			return kIOReturnSuccess;
		}
	
		isPending = false;
		command = getTxUnusedCommand();
		
#if SPI_PEDANTIC_ASSERT
		SPI_ASSERT( command, !=, NULL );
#endif
	
#if SPI_SRDY_DEBUG
		BasebandSPILogger::logGeneral( "CTX", 0x9 | ( getNumTxUnusedCommands() << 8 ), isSRDYStateHigh() );
#else
		BasebandSPILogger::logGeneral( "CTX", 0x9 | ( getNumTxUnusedCommands() << 8 ) );
#endif
	}
	
	while ( 1 )
	{
#if SPI_SRDY_DEBUG	
		BasebandSPILogger::logGeneral( "CTx", 1, isSRDYStateHigh() );
#else
		BasebandSPILogger::logGeneral( "CTx", 1 );
#endif
		IOByteCount written = command->writeData( (UInt8 *)source, length, fFramePayloadSize );
		
		if ( isPending )
		{
			isPending = false;
		}
		else
		{
#if SPI_SRDY_DEBUG
			BasebandSPILogger::logGeneral( "CTx", 2  | ( fCommandQueue->getSize() << 8 ), isSRDYStateHigh() );
#else
			BasebandSPILogger::logGeneral( "CTx", 2  | ( fCommandQueue->getSize() << 8 ) );
#endif
			
			fCommandQueue->enqueueTail( command );
		}
		
		source += written;
		length -= written;
		
		if ( length )
		{
			if ( getNumTxUnusedCommands() < 2 )
			{
				break;
			}

			command = getTxUnusedCommand();
			
#if SPI_PEDANTIC_ASSERT
			SPI_ASSERT( command, !=, NULL );
#endif
			
		}
		else
		{
			break;
		}
	}
	
	*remaining = length;
	if ( length || transmitNow )
	{
		prepareTransferIfNeeded( kPrepareInitiate );
	}
	
	return kIOReturnSuccess;
}

IOReturn BasebandSPIProtocol::registerRxBuffer( IOMemoryDescriptor *md, UInt8 * virtualAddress, void * cookie )
{
	SPI_TRACE( DEBUG_SPEW );
	
#if SPI_PEDANTIC_ASSERT
	SPI_ASSERT( getWorkLoop()->inGate(), ==, true );
#endif

	BasebandSPICommand * command = getRxFreeCommand();
	
	if ( !command )
	{
		SPI_LOG( DEBUG_CRITICAL, "Registered too many commands\n" );
		return kIOReturnNotPermitted;
	}
	
	SPI_LOG( DEBUG_SPEW, "Setting memory descriptor %p with cookie %p\n",
		md, cookie );
	
	IOReturn ret = command->setMemoryDescriptor( md, virtualAddress, cookie );
	
	if ( ret != kIOReturnSuccess )
	{
		addRxFreeCommand( command );
		return ret;
	}
	
#if SPI_PEDANTIC_ASSERT
	SPI_ASSERT( command->getMemoryDescriptor(), ==, md );
	SPI_ASSERT( command->getCookie(), ==, cookie );
#endif

#if SPI_SRDY_DEBUG
	BasebandSPILogger::logGeneral( "RRx", 0, isSRDYStateHigh() );
#else
	BasebandSPILogger::logGeneral( "RRx", 0 );
#endif
	
	addRxUnusedCommand( command );
	
	if ( !fInLowPower )
	{
		prepareTransferIfNeeded( kPrepareRxRegistered );
	}
	
	return kIOReturnSuccess;
}
