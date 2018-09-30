#ifndef _BASEBANDSPICONTROLLER_H_
#define _BASEBANDSPICONTROLLER_H_

#include "BasebandSPIDebug.h"
#include "BasebandSPIConfig.h"
#include "BasebandSPIReturn.h"
#include "BasebandSPIDevice.h"

#include <IOKit/IOLib.h>

class IODMAEventSource;
class IODMACommand;
class AppleARMIODevice;

class BasebandSPIDiagnostics;
class BasebandSPIProtocol;

class BasebandSPIController: public IOService
{
	OSDeclareAbstractStructors(BasebandSPIController);
	
public:
	typedef enum
	{
		kNone		 = 0,
		kRxDMA		 = 1UL << 0,
		kTxDMA		 = 1UL << 1,
	}
	DMAFlags;

private:
	typedef enum
	{
		kStateActive,
		kStateInactive,
		kStateDeactivating	
	} State;
	
	typedef enum
	{
		kFlagHardwareActive		= 1UL << 0,
		kFlagTransferActive		= 1UL << 1,
		kFlagError				= 1UL << 2
	}
	VolatileHardwareStateFlags;
	
	typedef enum
	{
		kOpen = 0,
		kBlocked = 1,
		kBlockedAndRejected = 2
	}
	InterruptGuardState;
	
public:
	virtual IOService * probe( IOService * provider, SInt32 * score );
	virtual bool init( OSDictionary * dict );
	virtual bool start( IOService *provider );
	virtual void stop( IOService *provider );
	virtual IOWorkLoop * getWorkLoop( void ) const;
	
private:
	inline IOReturn createEventSources( void );
	inline IOReturn releaseEventSources( void );
	
	IOReturn validateHardware( void );
		
	void dmaNotifyAction(IODMAEventSource * es, IODMACommand * command, IOReturn status, IOByteCount byteCount);
	void dmaCompleteAction(IODMAEventSource * es, IODMACommand *command, IOReturn status, IOByteCount byteCount);
	
	void transferCompleteTimeoutAction(IOTimerEventSource *es);
	
	inline void startTransferCompleteTimer( void );
	inline void cancelTransferCompleteTimer( void );
	
protected:
	UInt32 getOverrideClockPeriod( void ) const;
	
private:
	// Generic helpers
	inline void changeState( State newState );
	IOReturn waitForStateChange( State expectedState );

private:
	// DMA helpers
	IOReturn handleDMACompleteOnError( IODMAEventSource * eventSource, IODMACommand * command, IOReturn status, IOByteCount byteCount );
	IOReturn handleDMACompleteOnActive( IODMAEventSource * eventSource, IODMACommand * command, IOReturn status, IOByteCount byteCount );
	IOReturn handleDMACompleteOnCancel( IODMAEventSource * eventSource, IODMACommand * command, IOReturn status, IOByteCount byteCount );
	
	inline IOReturn queueDMAWithCommand( IODMAEventSource * es, IODirection direction, IODMACommand * command, IOByteCount size );
	
	inline IOReturn cancelDMA( IODMAEventSource * es );

	inline IOReturn handleCancelledTxDMACommand( IODMACommand * command );
	inline IOReturn handleCancelledRxDMACommand( IODMACommand * command );
	
	IOReturn configureDMA( IODMACommand * command, IOByteCount size, DMAFlags what );
	IOReturn waitForDMACompletion( void );
	
protected:
	// debugging
	static const char * stateAsString( State state );
	
	const char * getDebugName( void ) const
	{
		return "BasebandSPIController";
	}
	
	void dumpRegisters( int level = DEBUG_SPEW )
	{
		if ((level) <= DEBUG_LEVEL)
		{
			dumpRegistersInternal();
		}
	}
	virtual void dumpRegistersInternal( void ) = 0;
	
	const char * eventSourceAsString( IODMAEventSource * es ) const;
		
	IOReturn getNewSnapshot( const char * reason );

	virtual IOReturn takeSnapshot( const char * comment, 
		BasebandSPIControllerSnapshot::PinState resetDetectState,
		BasebandSPIControllerSnapshot ** snapshot ) = 0;

public:
	IOReturn activate( BasebandSPIDevice * device, BasebandSPIProtocol * protocol );
	IOReturn deactivate( void );
	
	IOReturn enterLowPower( void );
	IOReturn exitLowPower( BasebandSPIDevice * device, BasebandSPIProtocol * protocol );
	
	bool reportError( BasebandSPIReturn err, const char * details );
		
public:
	IOReturn configure( IODMACommand * receiveCommand, IODMACommand * transmitCommand, IOByteCount size );
	
	IOReturn startTransfer( void );
	
	IOReturn unconfigure( void );
	
	IOReturn cancelAll( void );

	inline bool isEnteringLowPower(void) const
	{
		return fEnteringLowPower;
	};
	
	bool canStartTransfer( void ) const
	{
		UInt32 flags = fVolatileHardwareStateFlags;
		
		return ( flags & kFlagHardwareActive ) 
			&& !( flags & kFlagTransferActive )
			&& !fEnteringLowPower;
	}
	
	IOReturn getSnapshot( BasebandSPIControllerSnapshot ** snapshot );
	
protected:
	// stuff for hardware specific drivers to implement
	
	/// Parses through the configuration and extract necessary information
	virtual IOReturn loadConfiguration( void );
	
	/// Validates the device type and chip revision
	/// @param deviceTyp the Device type
	/// @param chipRevision the chip revision
	/// @returns True if OK, false if not compatible
	virtual bool validateDeviceTypeAndChip( const char * deviceType, UInt32 chipRevision ) const = 0;
		
	/// Do a preflight setup of the hardware
	virtual IOReturn preflightSetupHardware( void ) = 0;
	
	/// Power up the hardware 
	/// @param dmaFlags	Flags saying which DMAs we want
	/// @param size The transfer size
	virtual IOReturn configureHardware( UInt32 dmaFlags, IOByteCount size ) = 0;
	
	/// Power down the hardware
	virtual IOReturn unconfigureHardware( void ) = 0;
	
	/// Start the hardware for the transfer
	virtual IOReturn startHardware( void ) = 0;
	
public:
	/// Disallow SPI-specific interrupts
	void beginBlockInterrupt( void );
	
	/// Allow SPI-specific interrupts to fisre again
	/// @returns True if we missed an interrupt while we blocked it
	bool endBlockInterrupt( void );
	
	/// Ask if we can interrupt, this is typically called from ISR context
	/// @returns True if we can interrupt
	bool intendToInterrupt( void )
	{
#if SPI_USE_REAL_ISR
		if ( fInterruptGuard == kOpen )
		{
			return true;
		}
		else
		{
			fInterruptGuard = kOpen;
			return false;
		}
#else
		return true;
#endif
	}

	
protected:
	IODMACommand		* fPendingReceiveCommand;
	IODMACommand		* fPendingSendCommand;

	IODMACommand		* fCurrentReceiveCommand;
	IODMACommand		* fCurrentSendCommand;
	
	AppleARMIODevice	* fProvider;
	
	IOCommandGate		* fCommandGate;
	IOWorkLoop			* fWorkLoop;
	
	State				fState;
	State				fStateBeforeLowPower;
	UInt32				fHardwareStateFlags;
	volatile UInt32		fVolatileHardwareStateFlags;
	
	UInt32				fPendingDMAs;
	bool				fInLowPower;
	bool				fEnteringLowPower;
	
	IODMAEventSource	* fTxDMAES;
	IODMAEventSource	* fRxDMAES;

	IOTimerEventSource	* fTransferCompleteTimeout;
	
	BasebandSPIDevice	* fDevice;
	BasebandSPIProtocol * fProtocol;
	BasebandSPIConfig	fConfiguration;
	
	BasebandSPIDiagnostics * fDiags;
	
	BasebandSPIControllerSnapshot	* fSavedSnapshot;
	
	volatile InterruptGuardState fInterruptGuard;
};

#endif
