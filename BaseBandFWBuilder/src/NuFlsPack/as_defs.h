#ifndef __AS_DEFS_H__
#define __AS_DEFS_H__

#ifndef uint16
#define uint16 unsigned short
#endif

#ifndef uint32
#define uint32 unsigned int
#endif

#ifndef uint64
//#define uint64 unsigned long long
#ifdef __BORLANDC__
#define uint64 unsigned __int64
#else
#define uint64 unsigned long long
#endif
#endif

#define KEY_LENGTH 		256

#define KEY_MARKER 0xAABBCCDD

#define MAX_MODULUS_LEN (2048/8)

#define MAX_ERROR_CODE_LEN 50

#endif
