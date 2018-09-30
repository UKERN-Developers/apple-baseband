#include "BasebandSPICommandQueue.h"
#include "BasebandSPICommand.h"
#include "BasebandSPIDebug.h"
#include "BasebandSPILogger.h"


BasebandSPICommandQueue::BasebandSPICommandQueue()
	: fSize( 0 ), fHead( NULL ), fTail( NULL )
{
}

BasebandSPICommandQueue::~BasebandSPICommandQueue()
{
	SPI_ASSERT( fSize, ==, 0 );
	SPI_ASSERT( fHead, ==, NULL );
	SPI_ASSERT( fTail, ==, NULL );
}

BasebandSPICommand * BasebandSPICommandQueue::dequeueHead( void )
{
	if ( isEmpty() )
	{
		return NULL;
	}
	
#if SPI_PEDANTIC_ASSERT
	SPI_ASSERT( fHead, !=, NULL );
	SPI_ASSERT( fTail, !=, NULL );
#endif

	BasebandSPICommand * ret = fHead;

	fSize--;

	if ( fSize )
	{
		fHead = fHead->getNext();
		ret->setNext( NULL );
	}
	else
	{
		fHead = NULL;
		fTail = NULL;
	}
	
	BasebandSPILogger::logGeneral( "DQ", fSize );
	
	SPI_LOG( DEBUG_SPEW, "queue size is now %u\n", fSize );

	return ret;
};

void BasebandSPICommandQueue::enqueueTail( BasebandSPICommand * command )
{
	if ( isEmpty() )
	{
		fHead = command;
	}
	else
	{
		fTail->setNext( command );
	}
	fTail = command;
	fSize++;
	
	BasebandSPILogger::logGeneral( "NQ", fSize );

	SPI_LOG( DEBUG_SPEW, "queue size is now %u\n", fSize );
}
