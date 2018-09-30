#ifndef __UTILFUNCTIONS_H__
#define __UTILFUNCTIONS_H__

#include "errorcodes.h"
#include "as_defs.h"

#define UNKNOWN_ERROR "UNKNOWN_ERROR"

void AS_GetErrorString(unsigned char *errorstring, unsigned char error_code);
void copy_bytes(unsigned char* dest, unsigned char *src,int len);
void printlong(char *name,uint32 *var,int len);
void printbytes(char *name,unsigned char *var,int len);
void AS_printf(const char* format, ...);
as_ret_type compare_long(uint32 *A, uint32 *B,int len);

#endif 

