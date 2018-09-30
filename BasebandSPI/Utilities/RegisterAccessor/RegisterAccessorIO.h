/*
 *  RegisterAccessorIO.h
 *  RegisterAccessor
 *
 *  Created by Arjuna Sivasithambaresan on 2/15/08.
 *  Copyright 2008 Apple Inc. All rights reserved.
 *
 */

#ifndef _REGISTERACCESSORIO_H_
#define _REGISTERACCESSORIO_H_
 
#include <sys/ioccom.h>

typedef struct
{
	unsigned	addr;
	unsigned	value;
}
RAWriteInfo;

typedef struct
{
	unsigned	addr;
	unsigned	mask;
	unsigned	value;
}
RAMaskedWriteInfo;

#define RAWRITE				_IOW( 'x', 10, RAWriteInfo )
#define RAREAD				_IOWR( 'x', 20, unsigned )
#define RAMASKEDWRITE		_IOW( 'x', 30, RAMaskedWriteInfo )

#endif



