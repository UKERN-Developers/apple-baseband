#ifndef __CONTROL_H__
#define __CONTROL_H__

#define ulong32 unsigned
#define ubyte unsigned char

#include "Header.h"

#define MY_VERSION_MAJOR (0x0002)
#define MY_VERSION_MINOR (0x0004)
#define MY_VERSION ((MY_VERSION_MAJOR<<16)|(MY_VERSION_MINOR))





#define MAX_LOADMAP_ENTRIES 8
#define MAX_DYN_ENTRIES 100

//LOCAL STRUCTURES:
typedef struct {
        RegionStructType Region;
        char* MemoryPtr;
        DownloadDataElementStructType* ElementPtr;
        ulong32 MemoryClass;
}LocalMemoryMapEntryStructType;

void Control(char* Output, 
             IFWD_StringList* FileList, 
             char* Psi,
             char* Ebl,
             ulong32 RemoveFlag,
			 ulong32 LocalSigning,
			 char* HsmPassword);


#endif 
