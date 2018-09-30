#include "BasebandSPILogger.h"

#include <sys/param.h>
#include <pexpert/pexpert.h>

#include <mach/mach_time.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOMemoryDescriptor.h>

#include "BasebandSPIConfig.h"
#include "BasebandSPIIFXProtocol.h"
#include "BasebandSPICommand.h"


BasebandSPILogger BasebandSPILogger::fsBasebandSPILogger;

static unsigned dumpBinary( const char * buffer, int length, char * dest, unsigned capacity )
{
	static const unsigned kBytesPerLine = 16;
	static const char kHexDigits[] = "0123456789ABCDEF";

	if (length <= 0)
	{
		dest[0] = 0;
		return 0;
	}

	unsigned written = 0;

	for (int lineStart = 0; lineStart < length; lineStart += kBytesPerLine)
	{	
		int thisLine = length - lineStart;
		if (thisLine > kBytesPerLine)
			thisLine = kBytesPerLine;
			
		char line[80] = ""; // 3 * 16 + 1 + 16 + 1 = 66
		int offset = 0;
		int byteOffset;
		for (byteOffset = 0; byteOffset < thisLine; byteOffset++)
		{
			unsigned char c = ((const unsigned char*) buffer)[lineStart + byteOffset];
			line[offset++] = kHexDigits[c >> 4];
			line[offset++] = kHexDigits[c & 0xf];
			line[offset++] = ' ';
		}

		// Pad with space
		memset(line + offset, ' ', (kBytesPerLine + 1 - byteOffset) * 3);
		offset += (kBytesPerLine + 1 - byteOffset) * 3;
	
		for (byteOffset = 0; byteOffset < thisLine; byteOffset++)
		{
			char c = ((const unsigned char*) buffer)[lineStart + byteOffset];
			if (c >= 0x20 && c <= 0x7e)
				line[offset++] = c;		// Printable
			else
				line[offset++] = '.';	// Non-printable
		}

		line[offset++] = '\r';
		line[offset++] = '\n';
		line[offset++] = '\0';
		
		written += SPIsnprintf( dest + written, capacity - written, "\t%04x  %s", lineStart, line);
	}

	return written;
}


BasebandSPILogger::BasebandSPILogger()
	: fCanHasDebugger( PE_i_can_has_debugger( NULL ) )
{
	clear();
	
#if SPI_USE_REAL_ISR_FOR_DEBUG
	bzero( &fSRDYAssertedTime, sizeof(fSRDYAssertedTime) );
	fNumSRDYs = 0;
	fLastSampledNumSRDYs = 0;
#endif

	if ( isEnabled() )
	{
		IOLog( ">>>> SPI Log is located at %p\n", fLog );
	}
}

BasebandSPILogger::~BasebandSPILogger()
{
}

unsigned BasebandSPILogger::getDelta( unsigned now, unsigned before )
{
	if ( !isEnabled() )
	{
		return 0;
	}
	
	if ( now < before )
	{
		return now + 0xFFFFFFFF - before;
	}
	else
	{
		return now - before;
	}
}

void BasebandSPILogger::clearInternal( void )
{
	fHead = fTail = fSize = 0;
}

BasebandSPILogger::LogEntry & BasebandSPILogger::getEntry( unsigned offset )
{
	offset = offset & ( kBufferSize - 1);
	return fLog[offset];
}

void BasebandSPILogger::addEntry( unsigned type, int arg, const char * msg, unsigned messageSize, bool state )
{
	if ( !isEnabled() )
	{
		return;
	}


	clock_sec_t s;
	clock_usec_t us;
	LogEntry * entry = &getEntry( fHead );
	
#if SPI_USE_REAL_ISR_FOR_DEBUG
	// If a new SRDY was asserted from the time the last entry was
	// added, add a new entry for the SRDY interrupt assertion
	if (fNumSRDYs != fLastSampledNumSRDYs)
	{
		fLastSampledNumSRDYs = fNumSRDYs;

		s = fSRDYAssertedTime.tv_sec;
		us = fSRDYAssertedTime.tv_usec;

		entry->timestamp		= s*1000*1000 + us;
		entry->type			= kTypeSRDYInterrupt;
		entry->arg			= fLastSampledNumSRDYs;

		fHead++;
		entry = &getEntry( fHead );
		if ( fSize < kBufferSize )
		{
			fSize++;
		}
		else
		{
			fTail++;
		}
	}
#endif
	
	clock_get_system_microtime( &s, &us );
	
	entry->timestamp		= s*1000*1000 + us;
	entry->type			= type;
	entry->arg			= arg;
#if SPI_SRDY_DEBUG
	entry->srdy_debug   = state;
#endif

	if  ( msg )
	{
		memcpy( entry->message, msg, MIN( kMaxMessageSize, messageSize ) );
	}
	else
	{
		entry->message[0] = 0;
	}

	fHead++;
	if ( fSize < kBufferSize )
	{
		fSize++;
	}
	else
	{
		fTail++;
	}
}

void BasebandSPILogger::logFrame( unsigned type, int size, BasebandSPICommand * cmd )
{
	if ( !isEnabled() )
	{
		return;
	}

	if ( cmd )
	{
		size = MIN( size, kMaxMessageSize );
		
		addEntry( type, size, (const char *)cmd->fBuffer, size );
	}
}

void BasebandSPILogger::logFrameReceivedInternal( int size, BasebandSPICommand * cmd )
{
	if ( !isEnabled() )
	{
		return;
	}

	logFrame( kTypeFrameReceived, size, cmd );
}

void BasebandSPILogger::logFrameSentInternal( int size, BasebandSPICommand * cmd )
{
	if ( !isEnabled() )
	{
		return;
	}

	logFrame( kTypeFrameSent, size, cmd );
}

void BasebandSPILogger::logMRDYInternal( int level, const char * msg )
{
	if ( !isEnabled() )
	{
		return;
	}

	size_t length = msg?(strlen( msg ) + 1):0;
	addEntry( kTypeMRDY, level, msg, length );
}

void BasebandSPILogger::logSRDYInternal( int level, const char * msg )
{
	if ( !isEnabled() )
	{
		return;
	}

	size_t length = msg?(strlen( msg ) + 1):0;
	addEntry( kTypeSRDY, level, msg, length );
}

void BasebandSPILogger::logGeneralInternal( const char * msg, int arg, bool state )
{
	if ( !isEnabled() )
	{
		return;
	}

	size_t length = strlen( msg ) + 1;
	addEntry( kTypeGeneral, arg, msg, length, state );
}

void BasebandSPILogger::logReceiveCallbackBeginInternal( const char * msg )
{
	if ( !isEnabled() )
	{
		return;
	}

	size_t length = msg?(strlen( msg ) + 1):0;
	addEntry( kTypeReceiveCallback, 0, msg, length );
}

void BasebandSPILogger::logReceiveCallbackEndInternal( const char * msg )
{
	if ( !isEnabled() )
	{
		return;
	}

	size_t length = msg?(strlen( msg ) + 1):0;
	addEntry( kTypeReceiveCallback, 1, msg, length );
}

void BasebandSPILogger::logDMACompleteInternal( const char * msg )
{
	if ( !isEnabled() )
	{
		return;
	}

	size_t length = msg?(strlen( msg ) + 1):0;
	addEntry( kTypeDMAComplete, 0, msg, length );
}

void BasebandSPILogger::logDMACancelInternal( const char * msg )
{
	if ( !isEnabled() )
	{
		return;
	}

	size_t length = msg?(strlen( msg ) + 1):0;
	addEntry( kTypeDMACancel, 0, msg, length );
}

void BasebandSPILogger::logDMAErrorInternal( const char * msg )
{
	if ( !isEnabled() )
	{
		return;
	}

	size_t length = msg?(strlen( msg ) + 1):0;
	addEntry( kTypeDMAError, 0, msg, length );
}

#if SPI_USE_REAL_ISR_FOR_DEBUG
void BasebandSPILogger::logSRDYAssertedInternal( void )
{
	clock_get_system_microtime( &(fSRDYAssertedTime.tv_sec),
				    &(fSRDYAssertedTime.tv_usec) );
	fNumSRDYs++;
}
#endif

void BasebandSPILogger::reportInternal( void )
{
	if ( !isEnabled() )
	{
		return;
	}

	unsigned lastTimestamp = 0;
	
	IOLog( "--- BEGIN SPI LOG ---\n" );
	
	for( UInt32 i = 0; i < fSize; i++ )
	{

		LogEntry * entry = &getEntry( fTail + i );
		
		UInt32 delta = getDelta( entry->timestamp, lastTimestamp );
		lastTimestamp = entry->timestamp;
		
		IOLog( "%010u:%010u: ", lastTimestamp, delta );
		
		switch ( entry->type )
		{
		case kTypeMRDY:
			IOLog( "MRDY: %d - %s\n", entry->arg, entry->message );
			break;
			
		case kTypeSRDY:
			IOLog( "SRDY: %d - %s\n", entry->arg, entry->message );
			break;
		
#if SPI_USE_REAL_ISR_FOR_DEBUG
		case kTypeSRDYInterrupt:
			IOLog( "SRDYA: %d\n", entry->arg );
			break;
#endif

		case kTypeGeneral:
			IOLog( "EVENT: %s, %p\n", entry->message, entry->arg );
			break;
			
		case kTypeFrameReceived:
		case kTypeFrameSent:
			{
				char buff[256];
				IOLog( "Frame %s(%u):\n", entry->type == kTypeFrameReceived? "Rx": "Tx", entry->arg );
				dumpBinary( entry->message, entry->arg, buff, sizeof( buff ) );
				IOLog( "%s", buff );
			}
			break;
			
		case kTypeReceiveCallback:
			IOLog( "Rx Callback: %d - %s\n", entry->arg, entry->message );
			break;		
			
		case kTypeDMAComplete:
			IOLog( "DMA Complete: %s\n", entry->message );
			break;
		}
		
	}
	
	IOLog( "--- END SPI LOG ---\n" );
}

bool BasebandSPILogger::popLogEntryAsStringInternal( char * buffer, unsigned capacity )
{
	if ( !isEnabled() )
	{
		return false;
	}

	if ( fSize == 0 )
	{
		return false;
	}
	else
	{

		static UInt32 lastTimestamp = 0;

		LogEntry * entry = &getEntry( fTail );

		fSize--;
		fTail++;
		
		IOByteCount copied = 0;
		UInt32 delta = getDelta( entry->timestamp, lastTimestamp );
		lastTimestamp = entry->timestamp;
		
		copied += SPIsnprintf( buffer + copied, capacity - copied, "%010u:%010u: ", lastTimestamp, delta );
		
		switch ( entry->type )
		{
		case kTypeMRDY:
			copied += SPIsnprintf( buffer + copied, capacity - copied, "MRDY: %d - %s", entry->arg, entry->message );
			break;
			
		case kTypeSRDY:
			copied += SPIsnprintf( buffer + copied, capacity - copied, "SRDY: %d - %s", entry->arg, entry->message );
			break;
			
#if SPI_USE_REAL_ISR_FOR_DEBUG
		case kTypeSRDYInterrupt:
			copied += SPIsnprintf( buffer + copied, capacity - copied, "SRDYA: %d", entry->arg );
			break;
#endif

		case kTypeGeneral:
#if SPI_SRDY_DEBUG
				copied += SPIsnprintf( buffer + copied, capacity - copied, "EVENT: %s, %p, (%d)", entry->message, entry->arg,( entry->srdy_debug ? 1 : 0 ) );
#else
			copied += SPIsnprintf( buffer + copied, capacity - copied, "EVENT: %s, %p", entry->message, entry->arg );
#endif
			break;
			
		case kTypeFrameReceived:
		case kTypeFrameSent:
			copied += SPIsnprintf( buffer + copied, capacity - copied, "Frame %s:", entry->type == kTypeFrameReceived? "Rx": "Tx" );
			copied += dumpBinary( entry->message, entry->arg, buffer + copied, capacity - copied );
			break;
			
		case kTypeReceiveCallback:
			copied += SPIsnprintf( buffer + copied, capacity - copied, "Rx Callback: %d - %s", entry->arg, entry->message );
			break;		
			
		case kTypeDMAComplete:
			copied += SPIsnprintf( buffer + copied, capacity - copied, "DMA Complete: %s", entry->message );
			break;
			
		case kTypeDMACancel:
			copied += SPIsnprintf( buffer + copied, capacity - copied, "DMA Cancel: %s", entry->message );
			break;
			
		case kTypeDMAError:
			copied += SPIsnprintf( buffer + copied, capacity - copied, "DMA Error: %s", entry->message );
			break;
		}

		return true;
	}
}
