#ifndef _REGISTERACCESSOR_H_
#define _REGISTERACCESSOR_H_

#include <IOKit/IOService.h>
#include <IOKit/IOLib.h>
#include <IOKit/IOCommandGate.h>
#include <IOKit/IODeviceMemory.h>

class RegisterAccessor: public IOService
{
	OSDeclareDefaultStructors( RegisterAccessor );
	
private:
	static const UInt32			kNumMappings = 4;
	static const IOByteCount	kMappingSize = 64*1024;
	
	typedef struct
	{
		IOPhysicalAddress		address;
		IOPhysicalLength		length;
		IOVirtualAddress		vaddress;
		IOMemoryMap				* map;
	}
	MappingParameters;
	
	typedef volatile UInt32 * Register;

private:
	static RegisterAccessor				* fsInstance;
	static struct cdevsw				fsDeviceSwitch;
	static MappingParameters			fsParams[kNumMappings];
	
private:
	int										fMajor;
	void *									fDevfsNode;
	
	IODeviceMemory							* fDeviceMemory[kNumMappings];
	
	
public:
	// device switch methods 
	static int			devopen( dev_t dev, int flags, int devtype, 
								 struct proc *p );

    static int			devclose( dev_t dev, int flags, int devtype, 
								  struct proc *p );

	static int			devioctl( dev_t dev, u_long cmd, caddr_t data, 
							  int fflag, struct proc *p );

public:
	virtual bool init( OSDictionary * dictionary );
	virtual bool attach( IOService * provider );
	virtual void detach( IOService * provider );
	virtual IOService * probe( IOService * provider, SInt32 * score );
	virtual bool start( IOService * provider );
	virtual void stop( IOService * provider );
	virtual void free( void );

private:
	int ioctl( u_long cmd, caddr_t data );
	
	inline Register getVirtualAddress( Register r ) const
	{
		UInt32 first = ((UInt32)r & 0xFFFF0000);
		
		for( UInt32 i = 0; i < kNumMappings; i++ )
		{
			if ( first == fsParams[i].address )
			{
				return (Register)( fsParams[i].vaddress + ( (UInt32)r & 0xFFFF ) );
			}
		}
		
		return 0;
	};
	
	/*
	inline Register getVirtualAddress( Register r ) const
	{
		return r;
	}
	*/

};

#endif
