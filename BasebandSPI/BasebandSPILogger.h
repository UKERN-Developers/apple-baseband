#ifndef _BasebandSPILogger_H_
#define _BasebandSPILogger_H_

#include "BasebandSPIConfig.h"

class IOMemoryDescriptor;
class BasebandSPICommand;

class BasebandSPILogger
{
private:
	static const unsigned			kBufferSizePower = 10;
	static const unsigned			kBufferSize = 1UL << kBufferSizePower;
	
	static const unsigned			kMaxMessageSize = 32;
	
	static const bool				kEnabledMaster = SPI_GM_TASK;
	
	typedef enum
	{
		kTypeMRDY,
		kTypeSRDY,
		kTypeGeneral,
		kTypeFrameReceived,
		kTypeFrameSent,
		kTypeReceiveCallback,
#if SPI_USE_REAL_ISR_FOR_DEBUG
		kTypeSRDYInterrupt,
#endif
		kTypeDMAComplete,
		kTypeDMACancel,
		kTypeDMAError
	};

	typedef struct
	{
		unsigned 		timestamp;
		unsigned short		type;
#if SPI_SRDY_DEBUG
		bool	srdy_debug;
#endif
		unsigned		arg;
		char			message[kMaxMessageSize];
	} LogEntry;

private:
	const bool			fCanHasDebugger;

	LogEntry			fLog[kBufferSize];

	unsigned			fHead;
	unsigned			fTail;
	unsigned			fSize;

	static BasebandSPILogger	fsBasebandSPILogger;

#if SPI_USE_REAL_ISR_FOR_DEBUG
	struct {
		clock_sec_t		tv_sec;
		clock_usec_t	tv_usec;
	} fSRDYAssertedTime;
	UInt32 fNumSRDYs;
	UInt32 fLastSampledNumSRDYs;
#endif

private:
	LogEntry & getEntry( unsigned offset );
	void addEntry( unsigned type, int arg, const char * msg, unsigned messageSize, bool state=false );
	
	void logFrame( unsigned type, int size, BasebandSPICommand * cmd );
	
	static bool isEnabled( void )
	{
		return kEnabledMaster && fsBasebandSPILogger.fCanHasDebugger;
	}

public:
	BasebandSPILogger();
	~BasebandSPILogger();

	static unsigned getDelta( unsigned now, unsigned before );
	
	static void clear( void )
	{
		if ( isEnabled() )
		{
			fsBasebandSPILogger.clearInternal();
		}
	};
	
#if SPI_USE_REAL_ISR_FOR_DEBUG
	static void logSRDYAsserted( void )
	{
		if ( isEnabled() )
		{
			fsBasebandSPILogger.logSRDYAssertedInternal();
		}
	};
#endif

	static void logMRDY( int level, const char * msg = 0 )
	{
		if ( isEnabled() )
		{
			fsBasebandSPILogger.logMRDYInternal( level, msg );
		}
	};
	static void logSRDY( int level, const char * msg = 0 )
	{
		if ( isEnabled() )
		{
			fsBasebandSPILogger.logSRDYInternal( level, msg );
		}
	};
	
	static void logGeneral( const char * msg, int arg = 0, bool srdy_state=false )
	{
		if ( isEnabled() )
		{
			fsBasebandSPILogger.logGeneralInternal( msg, arg, srdy_state );
		}
	}

	static void logFrameReceived( int size, BasebandSPICommand * cmd )
	{
		if ( isEnabled() )
		{
			fsBasebandSPILogger.logFrameReceivedInternal( size, cmd );
		}
	};

	static void logFrameSent( int size, BasebandSPICommand * cmd )
	{
		if ( isEnabled() )
		{
			fsBasebandSPILogger.logFrameSentInternal( size, cmd );
		}
	}
	static void logReceiveCallbackBegin( const char * msg = 0 )
	{
		if ( isEnabled() )
		{
			fsBasebandSPILogger.logReceiveCallbackBeginInternal( msg );
		}
	}
	static void logReceiveCallbackEnd( const char * msg = 0 )
	{
		if ( isEnabled() )
		{
			fsBasebandSPILogger.logReceiveCallbackEndInternal( msg );
		}
	}
	
	static void logDMAComplete( const char * msg = 0 )
	{
		if ( isEnabled() )
		{
			fsBasebandSPILogger.logDMACompleteInternal( msg );
		}
	}
	
	static void logDMACancel( const char * msg = 0 )
	{
		if ( isEnabled() )
		{
			fsBasebandSPILogger.logDMACancelInternal( msg );
		}
	}
	
	static void logDMAError( const char * msg = 0 )
	{
		if ( isEnabled() )
		{
			fsBasebandSPILogger.logDMAErrorInternal( msg );
		}
	}
	
	static void report( void )
	{
		if ( isEnabled() )
		{
			fsBasebandSPILogger.reportInternal();
		}
	}


	static bool popLogEntryAsString( char * buffer, unsigned capacity )
	{
		if ( isEnabled() )
		{
			return fsBasebandSPILogger.popLogEntryAsStringInternal( buffer, capacity );
		}
		else
		{
			return false;
		}
	}


private:
	void clearInternal( void );
	void logMRDYInternal( int level, const char * msg );
	void logSRDYInternal( int level, const char * msg );
	void logGeneralInternal( const char * msg, int arg, bool state );
	void logFrameReceivedInternal( int size, BasebandSPICommand * cmd );
	void logFrameSentInternal( int size, BasebandSPICommand * cmd );
	void logReceiveCallbackBeginInternal( const char * msg );
	void logReceiveCallbackEndInternal( const char * msg );
	void logDMACompleteInternal( const char * msg );
	void logDMACancelInternal( const char * msg );
	void logDMAErrorInternal( const char * msg );
#if SPI_USE_REAL_ISR_FOR_DEBUG
	void logSRDYAssertedInternal( void );
#endif

	void reportInternal( void );
	bool popLogEntryAsStringInternal( char * buffer, unsigned capacity );
};

#endif
