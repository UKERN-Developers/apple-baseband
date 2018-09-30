#ifndef __PRGHANDLER_H__
#define __PRGHANDLER_H__

#include "Header.h"
#ifndef FLASHTOOL_E2
  #include "IFWD_MemoryStream.h"
#else
  #include "IFWD_std_types.h"
#endif
#include <stdio.h>

#ifndef FLASHTOOL_E2
  ulong32 PRGH_GetEndian(char *Memory);
  ulong32 PRGH_FileCheck(char *Memory);
  ulong32 PRGH_VersionCheck(char *Memory,ulong32 ToolElementType, ulong32 CurrentVersion);
  bool PRGH_GetElementIndex(char *Memory,ulong32 ElementType, ulong32 UID, ulong32* element_index);
  bool PRGH_GetNextElementIndex(char *Memory,ulong32* current_element_index);
  ulong32 PRGH_GetPrgHandlerVersion();
  ulong32 PRGH_WriteElement(IFWD_MemoryStream *Output,ulong32 Type,ulong32 UID,char* PayloadPtr,ulong32 PayloadSize);
  ulong32 PRGH_ReplaceElement(IFWD_MemoryStream *Output,ulong32 Type,ulong32 UID,char* PayloadPtr,ulong32 PayloadSize);
#else
  #ifdef __cplusplus
	extern "C" {
  #endif
	U32 PRGH_VersionCheck(char *Memory,U32 ToolElementType, U32 CurrentVersion);
	IFX_BOOL PRGH_GetElementIndex(char *Memory,U32 ElementType, U32 UID, U32* element_index);
	IFX_BOOL PRGH_GetNextElementIndex(char *Memory,U32* current_element_index);
	U32 PRGH_GetPrgHandlerVersion();

	// Functions only for FlashTool
	U32 PRGH_FileCheck_File(IFX_FILE *fp);
	U32 PRGH_VersionCheck_File(IFX_FILE *fp,U32 ToolElementType, U32 CurrentVersion);
	IFX_BOOL PRGH_GetElementIndex_File(IFX_FILE *fp,U32 ElementType, U32 UID, U32 *element_index);
	IFX_BOOL PRGH_GetNextElementIndex_File(IFX_FILE *fp,U32* current_element_index);
  #ifdef __cplusplus
  }
  #endif
#endif
ulong32 PRGH_SwapElementEndian(void *Memory,ulong32 ElemType);
ulong32 PRGH_GetLongFromLong(ulong32 value);

typedef enum
{
  PRG_OK,
  PRG_FAIL,
  PRG_ELEMENT_NOT_FOUND,
  PRG_ELEMENT_SIZE_DIFFER,
  PRG_HASHING_ERROR,
  PRG_FILE_WRITE_ERROR,
  NUM_ERROR_CODES // This should be the last
} prg_handler_ret_type;

#define PRG_HANDLER_VERSION PRGHANDLER_TOOL_REQ_VER 

#define INVALID_TOOL_VERSION 0xFFFFFFFF
#define INVALID_ELEMENT_INDEX 0xFFFFFFFF
#define INVALID_FILE_TYPE 0xFFFFFFFF

#define FILE_TYPE_MASK 0x000000FF

#define UNKNOWN_ENDIAN_MACHINE 0
#define LITTLE_ENDIAN_MACHINE 1
#define BIG_ENDIAN_MACHINE 2

#endif
