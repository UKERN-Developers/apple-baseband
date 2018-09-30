/* add your code here */

#include "RegisterAccessor.h"
#include "RegisterAccessorIO.h"


#include <sys/conf.h>
#include <sys/errno.h>
#include <miscfs/devfs/devfs.h>

#define super IOService

static const char * kClassName = "RegisterAccessor";


struct cdevsw RegisterAccessor::fsDeviceSwitch = 
{
	RegisterAccessor::devopen,					// open
	RegisterAccessor::devclose,					// close
	eno_rdwrt,									// read
	eno_rdwrt,									// write
	RegisterAccessor::devioctl,					// ioctl
	eno_stop,									// stop
	eno_reset,									// reset	
	0,											// ttys
	eno_select,									// select
	eno_mmap,									// mmap
	eno_strat,									// strategy
	eno_getc,									// getc	
	eno_putc,									// putc
	0											// type	
};

RegisterAccessor::MappingParameters RegisterAccessor::fsParams[RegisterAccessor::kNumMappings] =
{
	{ 0x3C500000, RegisterAccessor::kMappingSize, 0 },
	{ 0x3E400000, RegisterAccessor::kMappingSize, 0 },
	{ 0x38100000, RegisterAccessor::kMappingSize, 0 },
	{ 0x3D200000, RegisterAccessor::kMappingSize, 0 }
};
	


// ===========
static const UInt32 kDebugLevel = 99;


class Tracer
{
private:
	char fFunc[64];
	UInt32 fLvl;
	
public:
	Tracer( const char * func, UInt32 lvl )
		: fLvl( lvl )
	{
		if ( fLvl <= kDebugLevel )
		{
			strncpy( fFunc, func, sizeof( fFunc ) );
			IOLog( "%s::%s: ENTER\n", kClassName, fFunc );
		}
	}
	
	~Tracer()
	{
		if ( fLvl <= kDebugLevel )
		{
			IOLog( "%s::%s: EXIT\n", kClassName, fFunc );
		}
	}
};

#define TRACE( lvl ) Tracer __tracer( __FUNCTION__, lvl )
#define LOG( lvl, ... ) if ( lvl <= kDebugLevel ) IOLog( __VA_ARGS__ )

// ===========

RegisterAccessor * RegisterAccessor::fsInstance = NULL;

OSDefineMetaClassAndStructors( RegisterAccessor, IOService );

bool RegisterAccessor::init( OSDictionary * dictionary )
{
	TRACE( 0 );

	bool result = super::init( dictionary );
	
	if ( result == false )
	{
		return false;
	}
	
	fMajor		= 1;
	fDevfsNode	= NULL;
		
	return true;
}

bool RegisterAccessor::attach( IOService * provider )
{
	TRACE( 0 );
	
	return super::attach( provider );
}

void RegisterAccessor::detach( IOService * provider )
{
	TRACE( 0 );
	
	super::detach( provider );
}

IOService * RegisterAccessor::probe( IOService * provider, SInt32 * score )
{
	TRACE( 0 );
	
	return super::probe( provider, score );
}

bool RegisterAccessor::start( IOService * provider )
{
	TRACE( 0 );

	bool result = super::start( provider );
	
	if ( result == false )
	{
		return false;
	}
	
	fsInstance = this;
	
	fMajor = cdevsw_add( -1, &fsDeviceSwitch );
	if ( fMajor < 0 )
	{
		LOG( 0, "Creating major failed\n" );
		return false;
	}

	dev_t device = makedev( fMajor, 0 );
	
	fDevfsNode = devfs_make_node( device, DEVFS_CHAR, UID_ROOT, GID_WHEEL, 0666, "RegisterAccess" );
	if ( fDevfsNode == NULL )
	{
		LOG( 0, "Making devfs node failed\n" );
		return false;
	}
	
	IOOptionBits bits = kIOMapInhibitCache;
	
	for( UInt32 i = 0; i < kNumMappings; i++ )
	{
		fDeviceMemory[i] = IODeviceMemory::withRange( fsParams[i].address, fsParams[i].length );
		fsParams[i].map = fDeviceMemory[i]->map( bits );
		
		fsParams[i].vaddress = fsParams[i].map->getVirtualAddress();
		
		LOG( 0, "Mapped %p to %p, size %p\n", fsParams[i].address, fsParams[i].vaddress, fsParams[i].length );		
	}
	
	return true;
}

void RegisterAccessor::stop( IOService * provider )
{
	TRACE( 0 );

	fsInstance = NULL;
	
	for ( UInt32 i = 0; i < kNumMappings; i++ )
	{
		fsParams[i].map->release();
		fDeviceMemory[i]->release();
	}
	
	cdevsw_remove( -1, &fsDeviceSwitch );
	fMajor = NULL;
	
	if( fDevfsNode )
	{
		devfs_remove( fDevfsNode );
		fDevfsNode = NULL;
	}

	super::stop( provider );
}

void RegisterAccessor::free( void )
{
	TRACE( 0 );
	
	super::free();
}

/* static */ int RegisterAccessor::devopen( dev_t dev, int flags, int devtype, 
							 struct proc *p )
{
	if ( fsInstance )
	{
		// fsInstance->open();
		return 0;
	}
	else
	{
		return ENXIO;
	}
}

/* static */ int RegisterAccessor::devclose( dev_t dev, int flags, int devtype, 
							  struct proc *p )
{
	if ( fsInstance )
	{
		//fsInstance->open();
		return 0;
	}
	else
	{
		return ENXIO;
	}
}

/* static */ int RegisterAccessor::devioctl( dev_t dev, u_long cmd, caddr_t data, 
						  int fflag, struct proc *p )
{
	if ( fsInstance )
	{
		return fsInstance->ioctl( cmd, data );
	}
	else
	{
		return ENXIO;
	}
}

int RegisterAccessor::ioctl( u_long cmd, caddr_t data )
{
	Register reg = NULL;

	switch ( cmd )
	{
	case RAWRITE:
		{
			RAWriteInfo * info = (RAWriteInfo *)data;
			
			reg = getVirtualAddress( (Register)info->addr );
			*reg = info->value;
			
			return 0;
		}
	
	case RAREAD:
		{
			reg = getVirtualAddress( *(Register *)data );
			UInt32 * out = (UInt32 *)data;
			
			*out = *reg;
			
			return 0;
		}
	
	case RAMASKEDWRITE:
		{
			RAMaskedWriteInfo * info = (RAMaskedWriteInfo *)data;
			
			reg = getVirtualAddress( (Register)info->addr );
			*reg = ( *reg & ~info->mask )  | ( info->value & info->mask );
					
			return 0;
		}
		
	default:
		return EINVAL;
	}
}
