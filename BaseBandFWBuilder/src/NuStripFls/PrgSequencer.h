//---------------------------------------------------------------------------

#ifndef PrgSequencerH
#define PrgSequencerH

#include "Header.h"
#include "IFWD_MemoryStream.h"
//#if defined(UNIX_BUILD)
#include <memory.h>
//#else
//#include <malloc.h>
//#include <mem.h>
//#endif

//---------------------------------------------------------------------------

#define PRG_SEQUENCER_VERSION PRGSEQUENCER_TOOL_REQ_VER

#define MAX_NUM_ELEMENTS 200

class PRGSequencer
{
  private:
		IFWD_MemoryStream *Input;
		IFWD_MemoryStream *Output;
		void (*NewElementFunc)(void* ElementPtr);
		ulong32 FileHash[5];

		bool AddToGlobalStruct(char* Mem);
		bool AllocateAndCopy(char* Mem, ulong32 ElemType);
		void TamperDataAfterRead(ulong32 ElemType,char*Mem);
		bool WriteElementToFile(IFWD_MemoryStream* Output, ulong32 element_type,ulong32 UID,char* Mem);
		void InitializeElement(void *mem, ulong32 ElemType,int size);
		ulong32 WriteToolElementToFile(ulong32 ToolElemType, ulong32 ReqVer, ulong32 UsedVer);

	public:
    void* ElemPtrs[MAX_NUM_ELEMENTS];
    int ElemCount;
		ulong32 MachEndian;
		ulong32 CallingTool;
		ulong32 AddingNewElement;
		ulong32 CurrentFileType;
		__fastcall PRGSequencer();
		__fastcall ~PRGSequencer();
		bool IsDynamicElement(ulong32 ElemType);
		void SetCallBackFunc(void (*NewElementF)(void* ElementPtr));
		ulong32 ReadFile(char* Filename, ulong32 ElementType, ulong32 ElementVersion,void (*NewElementF)(void* ElementPtr));
		ulong32 WriteFile(char* Filename);
		void* CreateElement(ulong32 ElementType);
		bool RemoveElement(void *element_ptr);
		void CleanUp();
		ulong32 CompareHash();
		void TamperToolDataBeforeWrite(ulong32 MakePrgVersion);
		char* FindElementStructure(ulong32 element_type,ulong32 UID);

    // Only for Storage Tool
    void* CreateStorageElement(ulong32 ElementType, char*data_ptr, ulong32 data_len);
    ulong32 WriteStorageFile(char* Filename,ulong32 version);
};
#endif
