#ifndef _BASEBANDSPICOMMANDQUEUE_H_
#define _BASEBANDSPICOMMANDQUEUE_H_

#include "BasebandSPIConfig.h"
#include "BasebandSPILogger.h"

class BasebandSPICommand;


class BasebandSPICommandQueue
{
public:
	BasebandSPICommandQueue();
	~BasebandSPICommandQueue();
	
private:
	static const char * getDebugName( void )
	{
		return "BasebandSPICommandQueue"; 
	}

public:
	void enqueueTail( BasebandSPICommand * command );
	BasebandSPICommand * dequeueHead( void );
	BasebandSPICommand * dequeueTail( void );
	BasebandSPICommand * peekHead( void ) const
	{
		return fHead;
	};
	
	BasebandSPICommand * peekTail( void ) const
	{
		return fTail;
	};

	UInt32 getSize( void ) const
	{
		return fSize;
	};

	bool isEmpty( void ) const
	{
		return getSize() == 0;
	};

private:
	UInt32					fSize;
	BasebandSPICommand		* fTail;
	BasebandSPICommand		* fHead;
};

#endif