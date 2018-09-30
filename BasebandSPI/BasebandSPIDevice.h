/*
 * Copyright (c) 2007 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 *
 * The contents of this file constitute Original Code as defined in and
 * are subject to the Apple Public Source License Version 1.1 (the
 * "License").  You may not use this file except in compliance with the
 * License.  Please obtain a copy of the License at
 * http://www.apple.com/publicsource and read it before using this file.
 *
 * This Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 * Please see the License for the specific language governing rights and
 * limitations under the License.
 *
 * @APPLE_LICENSE_HEADER_END@
 */

#ifndef __BASEBANDSPIDEVICE_H
#define __BASEBANDSPIDEVICE_H

#include <IOKit/IOService.h>
#include <IOKit/IOCommandGate.h>

// Errors
#define kBasebandSPIReturnRxReady          (1 << 0)
#define kBasebandSPIReturnTxReady          (1 << 1)
#define kBasebandSPIReturnHWError          (1 << 2)
#define kBasebandSPIReturnProtocolError    (1 << 3)
#define kBasebandSPIReturnRxFreed          (1 << 4)
#define kBasebandSPIReturnValidFrame	   (1 << 5)
#define kBasebandSPIReturnPMError		   (1 << 6)

#define kBasebandSPIReturnCRCMismatch		iokit_vendor_specific_err( 0x00 )

typedef UInt32 BasebandSPIReturn;

// Callback for when data is received, or there is an error.
// target - Target of the callback
// cookie - The cookie passed in to with registerRxBuffer to identify the
//          complete transfer, or NULL if indicating an error
// status - A status from the SPI driver
// offset - Offset in the IOMemoryDescriptor where valid data begins
// length - Length of valid data
typedef void (*BasebandSPICompletionAction)(void *target, void *cookie, BasebandSPIReturn status, IOByteCount offset, IOByteCount length);

class BasebandSPIController;
class BasebandSPIProtocol;

class BasebandSPIProtocolSnapshot;

// class for storing a snapshot of the SPI driver
class BasebandSPIControllerSnapshot
{
private:
	static const IOByteCount kMaxCommentSize = 64;
	
public:
	typedef enum
	{
		kLow = 0,
		kHigh = 1,
		kDunno = 2
	}
	PinState;
	
private:
	BasebandSPIControllerSnapshot( BasebandSPIControllerSnapshot * other ) 
		: fProtocolSnapshot( NULL )
	{};
protected:
	BasebandSPIControllerSnapshot( const char * comment, PinState resetDetectState );
	
public:
	virtual ~BasebandSPIControllerSnapshot();
	
public:
	virtual IOByteCount copyAsString( char * dest, IOByteCount capacity ) const = 0;
	
	void injectProtocolSnapshot( BasebandSPIProtocolSnapshot * snapshot )
	{
		fProtocolSnapshot = snapshot;
	}
	
	bool resetDetectIsHigh( void ) const
	{
		return fResetDetectState == kHigh;
	}
	
	static const char * pinStateAsString( PinState state );
	
protected:
	char				fComment[kMaxCommentSize];
	PinState			fResetDetectState;
	BasebandSPIProtocolSnapshot	* fProtocolSnapshot;
};

class BasebandSPIDevice : public IOService
{
	OSDeclareDefaultStructors(BasebandSPIDevice);

public:
	static const IOByteCount		kErrorStringSize = 128;
	
private:

	typedef enum
	{
		kStateInactive,
		kStateActive,
		kStateLowPower,
		kStateError
	} State;
		
	typedef enum
	{
		kFlagLowPowerAllowed = 1U << 0,
		kFlagSystemInLowPower = 1U << 1,
		kFlagLowPowerPreEnter = 1U << 2
	}
	LowPowerFlags;
	
private:
	static const char * getDebugName( void )
	{
		return "BasebandSPIDevice";
	}
	
private:
	/// Utility method for string representation of the state
	/// @param state The state
	/// @returns the string representation of state
	static const char * stateAsString( State state );
	
private:
	virtual IOService * probe( IOService * provider, SInt32 * score );
	virtual bool init( OSDictionary * dict );
	virtual bool start( IOService *provider );
	virtual void stop( IOService *provider );
	
	virtual IOReturn setPowerState( unsigned long whichState, IOService * whatDevice );
	
	IOReturn setPowerStateGated( unsigned long whichState, IOService * whatDevice );
	
private:
	IOReturn initPM( void );
	IOReturn stopPM( void );

	/// Prepares driver for low power
	/// @returns The result of the operation
	IOReturn enterLowPower( void );
	
	/// Brings driver out of low power
	/// @returns The result of the operation
	IOReturn exitLowPower( void );
			
public:	
	// activation/deactivation/power

	/// Activates the controller
	/// @param target The target of the callback
	/// @param action The callback function
	/// @returns The result of activation
	IOReturn activate( void *target, BasebandSPICompletionAction action );
	
	/// Deactivates the controller
	/// @returns The result of deactivation
	IOReturn deactivate( void );
	
	/// Notify the SPI driver that it's OK to go to low ower
	/// @returns The result
	IOReturn allowLowPower( void );

	/// Notify the SPI driver that Mux is attempting to enter low power
	/// @returns The result
	IOReturn preEnterLowPower( void );
public:
	// transfer related

	// Copy buffer into an internal buffer in the driver for transfer.
	// buffer - The source buffer
	// length - Bytes to copy
	// remaining - Bytes not yet copied.
	IOReturn copyTxBuffer(void *buffer, IOByteCount length, IOByteCount *remaining, bool txNow );
	
	// Register a buffer for received data.
	// md - Memory descriptor for the buffer
	// virtualAddress - Virtual address of the buffer, 
	// The passed in memory descriptor sould have at least getFrameSize() bytes of room
	IOReturn registerRxBuffer(IOMemoryDescriptor *md, UInt8 * virtualAddress, void *cookie);
	
	// Gets the error string
	// buff - The buffer to copy into
	// capacity - The capacity of the buffer
	IOReturn getControllerSnapshot( BasebandSPIControllerSnapshot ** snapshot );

public:
	/// Debugging and logging
	/// @param buffer The buffer to copy into
	/// @param capacity The capacity of the buffer
	/// @returns true if something was copied, false otherwise
	/// @notes This will be a string representation of the entry
	bool getLogEntry( char * buffer, IOByteCount capacity );
	
public:
	void invokeCompletionCallback( void * cookie,
		BasebandSPIReturn status, IOByteCount offset, IOByteCount length )
	{
		if ( fCompletionAction )
		{
			fCompletionAction( fCompletionTarget, cookie, status, offset, length );
		}
	}
	
	void reportError( BasebandSPIReturn error )
	{
		invokeCompletionCallback( NULL, error, 0, 0 );
	}
		
public:
	static BasebandSPIDevice *withRegistryEntry(IORegistryEntry *from, IOService *provider);

	/// Indicate if Mux is in the process of entering low power
	/// @returns The result
	bool isMuxEnteringLowPower( void )
	{
		return ( (fLowPowerFlags & kFlagLowPowerPreEnter) != 0 );
	}

	// Query the device for the frame size before registering any Rx buffers
	// with it.
	virtual IOByteCount getFrameSize();

	
	// Query the device for the maximum number of Rx buffers that can
	// be registered.
	virtual UInt32 getMaxRxBufferCount();


protected:	
	State					fState;
	State					fStateBeforeLowPower;
	
    IOCommandGate			* fCommandGate;

private:
	BasebandSPIController	* fController;
	BasebandSPIProtocol		* fProtocol;
	
	void							* fCompletionTarget;
	BasebandSPICompletionAction		fCompletionAction;
	
	UInt32					fLowPowerFlags;
};

#endif
