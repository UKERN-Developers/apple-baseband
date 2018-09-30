//---------------------------------------------------------------------------


#pragma hdrstop

//#include <stdlib.h>
#include "PrgSequencer.h"
#include "IFWD_MemoryStream.h"
#include "PrgHandler.h"
#include "c_sha1.h"
#include "string.h"
#include <stdlib.h>

#pragma package(smart_init)



//---------------------------------------------------------------------------

bool PRGSequencer::AddToGlobalStruct(char* Mem)
{
  if(ElemCount >= MAX_NUM_ELEMENTS )
    return false;

  ElementHeaderStructType *E = (ElementHeaderStructType *)Mem;
  AllocateAndCopy(Mem,PRGH_GetLongFromLong(E->Type));
  return true;
}

bool PRGSequencer::AllocateAndCopy(char* Mem, ulong32 ElemType)
{
  bool found = true;
  int StructSize = 0;
  switch(ElemType)
  {
    case GENERIC_HEADER_ELEM_TYPE:
      ElemPtrs[ElemCount] = ( GenericHeaderElementStructType *)malloc(sizeof(GenericHeaderElementStructType));
      StructSize = sizeof(GenericHeaderElementStructType);
      break;
    case PRGHANDLER_TOOL_ELEM_TYPE:
    case PRGSEQUENCER_TOOL_ELEM_TYPE:
    case MAKEPRG_TOOL_ELEM_TYPE:
    case HEXTOFLS_TOOL_ELEM_TYPE:
    case FLSSIGN_TOOL_ELEM_TYPE:
    case DWDTOHEX_TOOL_ELEM_TYPE:
    case FLASHTOOL_TOOL_ELEM_TYPE:
    case STORAGETOOL_TOOL_ELEM_TYPE:
		case FLSTOHEADER_TOOL_ELEM:
    case BOOT_CORE_TOOL_ELEM:
    case EBL_TOOL_ELEM:
		case FLSTONAND_TOOL_ELEM:
			ElemPtrs[ElemCount] = ( ToolVersionElementStructType *)malloc(sizeof(ToolVersionElementStructType));
      StructSize = sizeof(ToolVersionElementStructType);
      break;
    case MEMORYMAP_ELEM_TYPE:
      ElemPtrs[ElemCount] = ( MemoryMapElementStructType *)malloc(sizeof(MemoryMapElementStructType));
      StructSize = sizeof(MemoryMapElementStructType);
      break;
    case DOWNLOADDATA_ELEM_TYPE:
      ElemPtrs[ElemCount] = ( DownloadDataElementStructType *)malloc(sizeof(DownloadDataElementStructType));
      StructSize = sizeof(DownloadDataElementStructType);
      break;
    case INDIRECT_DOWNLOADDATA_ELEM_TYPE:
      ElemPtrs[ElemCount] = ( IndirectDownloadDataElementStructType *)malloc(sizeof(IndirectDownloadDataElementStructType));
      StructSize = sizeof(IndirectDownloadDataElementStructType);
      break;
    case HARDWARE_ELEM_TYPE:
      ElemPtrs[ElemCount] = ( HardwareElementStructType *)malloc(sizeof(HardwareElementStructType));
      StructSize = sizeof(HardwareElementStructType);
      break;
    case NANDPARTITION_ELEM_TYPE:
      ElemPtrs[ElemCount] = ( NandPartitionElementStructType *)malloc(sizeof(NandPartitionElementStructType));
      StructSize = sizeof(NandPartitionElementStructType);
      break;
    case SECURITY_ELEM_TYPE:
      ElemPtrs[ElemCount] = ( SecurityElementStructType *)malloc(sizeof(SecurityElementStructType));
      StructSize = sizeof(SecurityElementStructType);
      break;
    case TOC_ELEM_TYPE:
      ElemPtrs[ElemCount] = ( TocElementStructType *)malloc(sizeof(TocElementStructType));
      StructSize = sizeof(TocElementStructType);
      break;
    case SAFE_BOOT_CODEWORD_ELEM_TYPE:
      ElemPtrs[ElemCount] = ( SafeBootCodewordElementStructType *)malloc(sizeof(SafeBootCodewordElementStructType));
      StructSize = sizeof(SafeBootCodewordElementStructType);
      break;
    case GENERIC_VERSION_ELEM_TYPE:
    case INJECTED_PSI_VERSION_ELEM_TYPE:
    case INJECTED_EBL_VERSION_ELEM_TYPE:
      ElemPtrs[ElemCount] = ( VersionDataElementStructType *)malloc(sizeof(VersionDataElementStructType));
      StructSize = sizeof(VersionDataElementStructType);
      break;
    case INJECTED_PSI_ELEM_TYPE:
    case INJECTED_EBL_ELEM_TYPE:
      ElemPtrs[ElemCount] = ( InjectionElementStructType *)malloc(sizeof(InjectionElementStructType));
      StructSize = sizeof(InjectionElementStructType);
      break;
    case ROOT_DISK_ELEM_TYPE:
    case CUST_DISK_ELEM_TYPE:
    case USER_DISK_ELEM_TYPE:
      ElemPtrs[ElemCount] = ( StorageDataElementStructType *)malloc(sizeof(StorageDataElementStructType));
      StructSize = sizeof(StorageDataElementStructType);
      break;
    default:
      found = false;
      break;
  }

  if(found)
  {
    if(Mem !=NULL)
    {
      memcpy(ElemPtrs[ElemCount],Mem,StructSize);
      TamperDataAfterRead(ElemType,Mem);
      if(MachEndian == BIG_ENDIAN_MACHINE)
        PRGH_SwapElementEndian(ElemPtrs[ElemCount],ElemType);
    }
    else
    {
      InitializeElement(ElemPtrs[ElemCount],ElemType,StructSize);
      ((ElementHeaderStructType*)ElemPtrs[ElemCount])->Type = ElemType;
      ((ElementHeaderStructType*)ElemPtrs[ElemCount])->Size = StructSize;
    }
    if(NewElementFunc != NULL)
      NewElementFunc(ElemPtrs[ElemCount]);
    ElemCount++;
  }

  return found;
}

void PRGSequencer::InitializeElement(void *Mem, ulong32 ElemType,int size)
{
  memset(Mem,0x00,size);
  switch(ElemType)
  {
    case MEMORYMAP_ELEM_TYPE:
      for(int i=0;i<MAX_MAP_ENTRIES;i++)
        (( MemoryMapElementStructType *)Mem)->Data.Entry[i].Start = 0xFFFFFFFF;
      break;
    /* In case of Tool Elements, when a new element is created, the Required and Used versions */
    /* should be updated to what is given in ToolVersions.h. This is because, when a new element */
    /* is created, user should know the versions which are going to be written to the file, when he */
    /* saves the file, ( rather then setting initial values are Zeros */
    case PRGHANDLER_TOOL_ELEM_TYPE:
      ((ToolVersionElementStructType*)Mem)->Data.Required = PRGHANDLER_TOOL_REQ_VER;
      ((ToolVersionElementStructType*)Mem)->Data.Used = PRG_HANDLER_VERSION;
      break;
    case PRGSEQUENCER_TOOL_ELEM_TYPE:
      ((ToolVersionElementStructType*)Mem)->Data.Required = PRGSEQUENCER_TOOL_REQ_VER;
      ((ToolVersionElementStructType*)Mem)->Data.Used = PRG_SEQUENCER_VERSION;
      break;
    case MAKEPRG_TOOL_ELEM_TYPE:
      ((ToolVersionElementStructType*)Mem)->Data.Required = MAKEPRG_TOOL_REQ_VER;
      ((ToolVersionElementStructType*)Mem)->Data.Used = MAKEPRG_TOOL_REQ_VER;;
      break;
    case HEXTOFLS_TOOL_ELEM_TYPE:
      ((ToolVersionElementStructType*)Mem)->Data.Required = HEXTOFLS_TOOL_REQ_VER;
      (( ToolVersionElementStructType *)Mem)->Data.Used = 0x00;
      break;
    case FLSSIGN_TOOL_ELEM_TYPE:
      ((ToolVersionElementStructType*)Mem)->Data.Required = FLSSIGN_TOOL_REQ_VER;
      (( ToolVersionElementStructType *)Mem)->Data.Used = 0x00;
      break;
    case DWDTOHEX_TOOL_ELEM_TYPE:
      ((ToolVersionElementStructType*)Mem)->Data.Required = DWDTOHEX_TOOL_REQ_VER;
      (( ToolVersionElementStructType *)Mem)->Data.Used = 0x00;
      break;
    case FLASHTOOL_TOOL_ELEM_TYPE:
      ((ToolVersionElementStructType*)Mem)->Data.Required = FLASHTOOL_TOOL_REQ_VER;
      (( ToolVersionElementStructType *)Mem)->Data.Used = 0x00;
      break;
    case STORAGETOOL_TOOL_ELEM_TYPE:
      ((ToolVersionElementStructType*)Mem)->Data.Required = STORAGETOOL_TOOL_REQ_VER;
      (( ToolVersionElementStructType *)Mem)->Data.Used = 0x00;
      break;
		case FLSTOHEADER_TOOL_ELEM:
			((ToolVersionElementStructType*)Mem)->Data.Required = FLSTOHEADER_TOOL_REQ_VER;
			(( ToolVersionElementStructType *)Mem)->Data.Used = 0x00;
			break;
		case BOOT_CORE_TOOL_ELEM:
			((ToolVersionElementStructType*)Mem)->Data.Required = BOOT_CORE_TOOL_REQ_VER;
			(( ToolVersionElementStructType *)Mem)->Data.Used = 0x00;
			break;
		case EBL_TOOL_ELEM:
			((ToolVersionElementStructType*)Mem)->Data.Required = EBL_TOOL_REQ_VER;
			(( ToolVersionElementStructType *)Mem)->Data.Used = 0x00;
			break;
		case FLSTONAND_TOOL_ELEM:
			((ToolVersionElementStructType*)Mem)->Data.Required = FLSTONAND_TOOL_REQ_VER;
			(( ToolVersionElementStructType *)Mem)->Data.Used = 0x00;
			break;
		/* End of Tool element updation */
	}
}

void PRGSequencer::TamperDataAfterRead(ulong32 ElemType,char*Mem)
{
	// Data pointer has to be adjusted in case of special elements, eg: DownloadData
	switch(ElemType)
	{
		case DOWNLOADDATA_ELEM_TYPE:
			(( DownloadDataElementStructType *)ElemPtrs[ElemCount])->Data.Data = (unsigned char*) (Mem+sizeof(DownloadDataElementStructType));
      break;
    case TOC_ELEM_TYPE:
      (( TocElementStructType *)ElemPtrs[ElemCount])->Data.Data = (TocEntryStructType*)(Mem+sizeof(TocElementStructType));
      break;
    case GENERIC_VERSION_ELEM_TYPE:
    case INJECTED_PSI_VERSION_ELEM_TYPE:
    case INJECTED_EBL_VERSION_ELEM_TYPE:
      (( VersionDataElementStructType *)ElemPtrs[ElemCount])->Data.Data = (unsigned char*) (Mem+sizeof(VersionDataElementStructType));
      break;
    case INJECTED_PSI_ELEM_TYPE:
    case INJECTED_EBL_ELEM_TYPE:
      (( InjectionElementStructType *)ElemPtrs[ElemCount])->Data.Data = (unsigned char*)(Mem+sizeof(InjectionElementStructType));
      break;
    case ROOT_DISK_ELEM_TYPE:
    case CUST_DISK_ELEM_TYPE:
    case USER_DISK_ELEM_TYPE:
      (( StorageDataElementStructType *)ElemPtrs[ElemCount])->Data.Data = (unsigned char*)(Mem+sizeof(StorageDataElementStructType));
      break;
  }
}

bool PRGSequencer::IsDynamicElement(ulong32 ElemType)
{
  if(ElemType == DOWNLOADDATA_ELEM_TYPE ||
    ElemType == INDIRECT_DOWNLOADDATA_ELEM_TYPE ||
    ElemType == SECURITY_ELEM_TYPE ||
    ElemType == TOC_ELEM_TYPE ||
    ElemType == GENERIC_VERSION_ELEM_TYPE ||
    ElemType == INJECTED_PSI_VERSION_ELEM_TYPE ||
    ElemType == INJECTED_EBL_VERSION_ELEM_TYPE ||
    ElemType == INJECTED_PSI_ELEM_TYPE ||
    ElemType == INJECTED_EBL_ELEM_TYPE ||
    ElemType == ROOT_DISK_ELEM_TYPE ||
    ElemType == CUST_DISK_ELEM_TYPE ||
    ElemType == USER_DISK_ELEM_TYPE
  )
    return true;
  else
    return false;
}

char* PRGSequencer::FindElementStructure(ulong32 element_type,ulong32 UID)
{

	for(int i=0; i<ElemCount && i < MAX_NUM_ELEMENTS;i++)
	{
		if(ElemPtrs[i] != NULL)
		{
			ElementHeaderStructType *E = (ElementHeaderStructType *)ElemPtrs[i];
			if(E->Type == element_type && E->UID == UID)
			{
				return (char*)ElemPtrs[i];
			}
		}
	}
	return NULL;
}

bool PRGSequencer::WriteElementToFile(IFWD_MemoryStream* Output, ulong32 element_type,ulong32 uid,char* Mem)
{
  bool valid_elem=true;
  int payload_size;

  switch(element_type)
  {
    case GENERIC_HEADER_ELEM_TYPE:
      payload_size = sizeof(GenericHeaderDataStructType);
      break;
    case PRGHANDLER_TOOL_ELEM_TYPE:
    case PRGSEQUENCER_TOOL_ELEM_TYPE:
    case MAKEPRG_TOOL_ELEM_TYPE:
    case HEXTOFLS_TOOL_ELEM_TYPE:
    case FLSSIGN_TOOL_ELEM_TYPE:
    case DWDTOHEX_TOOL_ELEM_TYPE:
    case FLASHTOOL_TOOL_ELEM_TYPE:
    case STORAGETOOL_TOOL_ELEM_TYPE:
		case FLSTOHEADER_TOOL_ELEM:
		case BOOT_CORE_TOOL_ELEM:
		case EBL_TOOL_ELEM:
		case FLSTONAND_TOOL_ELEM:
			payload_size = sizeof(ToolVersionStructType);
      break;
    case MEMORYMAP_ELEM_TYPE:
      payload_size = sizeof(MemoryMapStructType);
      break;
    case DOWNLOADDATA_ELEM_TYPE:
      payload_size = sizeof(DownloadDataStructType);
      break;
    case INDIRECT_DOWNLOADDATA_ELEM_TYPE:
      payload_size = sizeof(IndirectDownloadDataStructType);
      break;
    case HARDWARE_ELEM_TYPE:
      payload_size = sizeof(HardwareStructType);
      break;
    case NANDPARTITION_ELEM_TYPE:
      payload_size = sizeof(NandPartitionDataStructType);
      break;
    case SECURITY_ELEM_TYPE:
      payload_size = sizeof(SecurityStructType);
      break;
    case TOC_ELEM_TYPE:
      payload_size = sizeof(TocStructType);
      break;
    case SAFE_BOOT_CODEWORD_ELEM_TYPE:
      payload_size = sizeof(CodewordStructType);
      break;
    case GENERIC_VERSION_ELEM_TYPE:
    case INJECTED_PSI_VERSION_ELEM_TYPE:
    case INJECTED_EBL_VERSION_ELEM_TYPE:
      payload_size = sizeof(VersionDataStructType);
      break;
    case INJECTED_PSI_ELEM_TYPE:
    case INJECTED_EBL_ELEM_TYPE:
      payload_size = sizeof(InjectionDataStructType);
      break;
    case ROOT_DISK_ELEM_TYPE:
    case CUST_DISK_ELEM_TYPE:
    case USER_DISK_ELEM_TYPE:
      payload_size = sizeof(StorageDataStructType);
      break;
    default:
      valid_elem = false;
    break;
  }

  if(valid_elem)
  {
    //TamperDataBeforeWrite(element_type,Mem);
    PRGH_WriteElement(Output,
                      element_type,
                      uid,
                      Mem+sizeof(ElementHeaderStructType),
                      payload_size);
  }

  return valid_elem;
}

ulong32 PRGSequencer::WriteToolElementToFile(ulong32 ToolElemType, ulong32 ReqVer, ulong32 UsedVer)
{
  ToolVersionStructType T;
  ulong32 result;

  //Write Tool Element
  T.Required = ReqVer;
  T.Used = UsedVer;
  result=PRGH_WriteElement(Output,ToolElemType,GENERIC_UID,(char*)&T,sizeof(T));

  return result;
}


//#################### Interface functions ################################
__fastcall PRGSequencer::PRGSequencer()
{
  Input = new IFWD_MemoryStream;

  CallingTool = 0;
  AddingNewElement = false;

  for(int i=0; i < MAX_NUM_ELEMENTS; i++)
    ElemPtrs[i] = NULL;

  ElemCount=0;

  NewElementFunc = NULL;
  memset(FileHash,0x00,sizeof(FileHash));
}

__fastcall PRGSequencer::~PRGSequencer()
{
  CleanUp();
  if(Input)
  {
    delete Input;
    Input = NULL;
  }
}

void PRGSequencer::CleanUp()
{
  Input->Clear();
  for(int i=0; i < ElemCount; i++)
  {
    if(ElemPtrs[i] != NULL)
      free(ElemPtrs[i]);
    ElemPtrs[i] = NULL;
  }
  ElemCount=0;
  memset(FileHash,0x00,sizeof(FileHash));
}

void PRGSequencer::SetCallBackFunc(void (*NewElementF)(void* ElementPtr))
{
  NewElementFunc = NewElementF;
}

ulong32 PRGSequencer::ReadFile(char* Filename, ulong32 ElementType, ulong32 ElementVersion,void (*NewElementF)(void* ElementPtr))
{
  ulong32 FileType,ReqVer;
  ulong32 element_index;
  ulong32 uid = GENERIC_UID;
  char *Mem;
  ulong32 retvalue=PRG_OK;

  Input->Clear();
  Input->LoadFromFile(Filename);

  MachEndian = PRGH_GetEndian((char*)Input->Memory);

  FileType=PRGH_FileCheck((char*)Input->Memory);
  if(FileType == INVALID_FILE_TYPE)
    return FileType;

  CallingTool = ElementType;
  CurrentFileType = FileType;

  if(FileType == DISK_FILE_TYPE)
  {
    retvalue = 0;
  }
  else
  {
    ReqVer = PRGH_VersionCheck((char*)Input->Memory,ElementType,ElementVersion);
    if(ReqVer != 0 ) // zero means okay.
    {
      if( CallingTool == MAKEPRG_TOOL_ELEM_TYPE &&
          ( ((ElementVersion >>16) & 0x0000FFFF) > ((ReqVer >>16) & 0x0000FFFF)) &&
          CurrentFileType == PRG_FILE_TYPE
        )
        retvalue=ReqVer;
      else
       return ReqVer;
    }
  }

  NewElementFunc = NewElementF;

  Mem = (char*)Input->Memory;
  if(PRGH_GetElementIndex(Mem,GENERIC_HEADER_ELEM_TYPE,uid,&element_index))
  {
    memcpy(FileHash,((GenericHeaderElementStructType*)(Mem+element_index))->Data.Sha1Digest,sizeof(FileHash));
    AddToGlobalStruct(&Mem[element_index]);
    while(PRGH_GetNextElementIndex(Mem,&element_index))
    {
      AddToGlobalStruct(&Mem[element_index]);
    }
  }

  return retvalue;
}

ulong32 PRGSequencer::WriteFile(char* Filename)
{
  ulong32 uid = GENERIC_UID;
  ulong32 ret;
  char *Mem;

  Output = new IFWD_MemoryStream;
  Output->Clear();

  // first write the static elements
  for(int i=0; i<ElemCount && i < MAX_NUM_ELEMENTS;i++)
  {
    if(ElemPtrs[i] != NULL)
    {
      ElementHeaderStructType *E = (ElementHeaderStructType *)ElemPtrs[i];
      if(!IsDynamicElement(E->Type))
    	  WriteElementToFile(Output,E->Type,E->UID,(char*)ElemPtrs[i]);
    }
  }

  // Write only static elements incase of MakePrg
  if(CallingTool != MAKEPRG_TOOL_ELEM_TYPE)
  {
    // Now write the dynamic elements
    for(int i=0; i<ElemCount && i < MAX_NUM_ELEMENTS;i++)
    {
      if(ElemPtrs[i] != NULL)
      {
        ElementHeaderStructType *E = (ElementHeaderStructType *)ElemPtrs[i];
        if(IsDynamicElement(E->Type))
          WriteElementToFile(Output,E->Type,E->UID,(char*)ElemPtrs[i]);
      }
    }
  }

  PRGH_WriteElement(Output,END_MARKER_ELEM_TYPE,uid,NULL,0);

  // If the calling tool is Make PRG, the file hash has to be taken
  if(CallingTool == MAKEPRG_TOOL_ELEM_TYPE)
  {
    SHA1Context filehash;
    Mem = (char*)(Output->Memory);
    int len_to_hash = Output->Size - sizeof(GenericHeaderElementStructType);

    SHA1Reset(&filehash);

    SHA1Input(&filehash,(unsigned char*)(Mem+sizeof(GenericHeaderElementStructType)),len_to_hash);
    if(!SHA1Result(&filehash))
    {
      if(Output)
      {
        delete Output;
        Output = NULL;
      }
      return PRG_HASHING_ERROR; // Error in Hashing
    }

    GenericHeaderElementStructType G;
    ulong32 element_index;
    if(PRGH_GetElementIndex(Mem,GENERIC_HEADER_ELEM_TYPE,uid,&element_index))
    {
      // Get the element
      memcpy(&G,&Mem[element_index],sizeof(GenericHeaderElementStructType));
      // Copy the hash
      for(int i=0; i<5;i++)
        G.Data.Sha1Digest[i]=filehash.Message_Digest[i];

      // Mark as derived if needed
      if(G.Data.FileType != PRG_FILE_TYPE)
      {
        // Make it PRG type
        G.Data.FileType = PRG_FILE_TYPE;
        // Then mark it as derived
        G.Data.FileType = G.Data.FileType | 0x01000000;
      }

      ulong32 ret=PRGH_ReplaceElement(Output,G.Header.Type,G.Header.UID,(char*)&(G.Data),sizeof(G.Data));
      if(ret != PRG_OK)
      {
        if(Output)
        {
          delete Output;
          Output = NULL;
        }
        return ret;
      }
    }

    // Copy the file hash to current file hash
    memcpy(FileHash,filehash.Message_Digest,sizeof(FileHash));
  }

  if(!Output->SaveToFile(Filename))
    ret = PRG_FILE_WRITE_ERROR;
  else
    ret = PRG_OK;

  if(Output)
  {
    delete Output;
    Output = NULL;
  }

  return ret;
}

void* PRGSequencer::CreateElement(ulong32 ElementType)
{
  if(AllocateAndCopy(NULL,ElementType))
    return ElemPtrs[ElemCount - 1];
  else
    return NULL;
}

bool PRGSequencer::RemoveElement(void *element_ptr)
{
  bool found = false;

  for(int i=0; i<ElemCount && i < MAX_NUM_ELEMENTS;i++)
  {
    if((char*)ElemPtrs[i] == (char*)element_ptr)
    {
      found = true;
      if(ElemPtrs[i] != NULL)
        free(ElemPtrs[i]);
      ElemPtrs[i] = NULL;
      break;
    }
  }
  return found;
}

void* PRGSequencer::CreateStorageElement(ulong32 ElementType, char*data_ptr, ulong32 data_len)
{
  StorageDataElementStructType *S;

  if(ElementType != ROOT_DISK_ELEM_TYPE &&
    ElementType != CUST_DISK_ELEM_TYPE &&
    ElementType != USER_DISK_ELEM_TYPE
    )
    return NULL;

  S = (StorageDataElementStructType*) CreateElement(ElementType);
  if( S == NULL) return NULL;

  S->Header.UID = GENERIC_UID;

  S->Data.Data = (unsigned char*)data_ptr;
  S->Data.DataLength = data_len;
  S->Data.DataOffset = 0x00;

  return S;
}

ulong32 PRGSequencer::WriteStorageFile(char* Filename,ulong32 version)
{
  ulong32 uid = GENERIC_UID;
  char *Mem;
  ulong32 result;
  GenericHeaderDataStructType Gen;

  Output = new IFWD_MemoryStream;
  Output->Clear();

  //Write Generic Header
  Gen.FileType = DISK_FILE_TYPE;
  Gen.Marker = PRG_MAGIC_NUMBER;
  memset((char*)Gen.Sha1Digest,0xDD,sizeof(Gen.Sha1Digest));
  result=PRGH_WriteElement(Output,GENERIC_HEADER_ELEM_TYPE,GENERIC_UID,(char*)&Gen,sizeof(Gen));
  if(result != PRG_OK)
  {
    if(Output)
    {
      delete Output;
      Output = NULL;
    }
    return result;
  }

  WriteToolElementToFile(PRGHANDLER_TOOL_ELEM_TYPE,PRGHANDLER_TOOL_REQ_VER,PRG_HANDLER_VERSION);
  WriteToolElementToFile(PRGSEQUENCER_TOOL_ELEM_TYPE,PRGSEQUENCER_TOOL_REQ_VER,PRG_SEQUENCER_VERSION);
  WriteToolElementToFile(MAKEPRG_TOOL_ELEM_TYPE,MAKEPRG_TOOL_REQ_VER,0x00);
  WriteToolElementToFile(HEXTOFLS_TOOL_ELEM_TYPE,HEXTOFLS_TOOL_REQ_VER,0x00);
  WriteToolElementToFile(FLSSIGN_TOOL_ELEM_TYPE,FLSSIGN_TOOL_REQ_VER,0x00);
  WriteToolElementToFile(DWDTOHEX_TOOL_ELEM_TYPE,DWDTOHEX_TOOL_REQ_VER,0x00);
  WriteToolElementToFile(FLASHTOOL_TOOL_ELEM_TYPE,FLASHTOOL_TOOL_REQ_VER,0x00);
  WriteToolElementToFile(STORAGETOOL_TOOL_ELEM_TYPE,STORAGETOOL_TOOL_REQ_VER,version);
	WriteToolElementToFile(FLSTOHEADER_TOOL_ELEM,FLSTOHEADER_TOOL_REQ_VER,0x00);
	WriteToolElementToFile(BOOT_CORE_TOOL_ELEM,BOOT_CORE_TOOL_REQ_VER,0x00);
	WriteToolElementToFile(EBL_TOOL_ELEM,EBL_TOOL_REQ_VER,0x00);
	WriteToolElementToFile(FLSTONAND_TOOL_ELEM,FLSTONAND_TOOL_REQ_VER,0x00);

  // Write the storage elements
  for(int i=0; i<ElemCount && i < MAX_NUM_ELEMENTS;i++)
  {
    if(ElemPtrs[i] != NULL)
    {
      ElementHeaderStructType *E = (ElementHeaderStructType *)ElemPtrs[i];
      WriteElementToFile(Output,E->Type,E->UID,(char*)ElemPtrs[i]);
    }
  }
  PRGH_WriteElement(Output,END_MARKER_ELEM_TYPE,uid,NULL,0);

  if(!Output->SaveToFile(Filename))
    result = PRG_FILE_WRITE_ERROR;
  else
    result = PRG_OK;

  if(Output)
  {
    delete Output;
    Output = NULL;
  }

  return result;
}

// This is used only by MakePrg
ulong32 PRGSequencer::CompareHash()
{
  ulong32 uid = GENERIC_UID;
  char *Mem;
  prg_handler_ret_type ret;

  Output = new IFWD_MemoryStream;
  Output->Clear();

  //write the static elements
  for(int i=0; i<ElemCount && i < MAX_NUM_ELEMENTS;i++)
  {
    if(ElemPtrs[i] != NULL)
    {
      ElementHeaderStructType *E = (ElementHeaderStructType *)ElemPtrs[i];
      if(!IsDynamicElement(E->Type))
        WriteElementToFile(Output,E->Type,E->UID,(char*)ElemPtrs[i]);
    }
  }
  PRGH_WriteElement(Output,END_MARKER_ELEM_TYPE,uid,NULL,0);

  SHA1Context filehash;
  Mem = (char*)(Output->Memory);
  int len_to_hash = Output->Size - sizeof(GenericHeaderElementStructType);

  SHA1Reset(&filehash);
  SHA1Input(&filehash,(unsigned char*)(Mem+sizeof(GenericHeaderElementStructType)),len_to_hash);
  if(Output)
  {
    delete Output;
    Output = NULL;
  }
  if(!SHA1Result(&filehash))
    return PRG_HASHING_ERROR; // Error in Hashing

  if(0 == memcmp(filehash.Message_Digest,FileHash,sizeof(FileHash)))
    ret = PRG_OK;
  else
    ret = PRG_FAIL;

  return ret;
}

// This is used only by MakePrg
void PRGSequencer::TamperToolDataBeforeWrite(ulong32 MakePrgVersion)
{
  // For all the tool elements, except for PRG Handler, PRG Sequencer and MakePrg,
  // the Used value has to be set to 0x00.
  for(int i=0; i<ElemCount && i < MAX_NUM_ELEMENTS;i++)
  {
    if(ElemPtrs[i] != NULL)
    {
      ElementHeaderStructType *E = (ElementHeaderStructType *)ElemPtrs[i];
      char *Mem = (char*)ElemPtrs[i];
      ulong32 ElemType = E->Type;
      switch(ElemType)
      {
        case PRGHANDLER_TOOL_ELEM_TYPE:
          ((ToolVersionElementStructType*)Mem)->Data.Required = PRGHANDLER_TOOL_REQ_VER;
          ((ToolVersionElementStructType*)Mem)->Data.Used = PRG_HANDLER_VERSION;
          break;
        case PRGSEQUENCER_TOOL_ELEM_TYPE:
          ((ToolVersionElementStructType*)Mem)->Data.Required = PRGSEQUENCER_TOOL_REQ_VER;
          ((ToolVersionElementStructType*)Mem)->Data.Used = PRG_SEQUENCER_VERSION;
          break;
        case MAKEPRG_TOOL_ELEM_TYPE:
          ((ToolVersionElementStructType*)Mem)->Data.Required = MAKEPRG_TOOL_REQ_VER;
          ((ToolVersionElementStructType*)Mem)->Data.Used = MakePrgVersion;
          break;
        case HEXTOFLS_TOOL_ELEM_TYPE:
          ((ToolVersionElementStructType*)Mem)->Data.Required = HEXTOFLS_TOOL_REQ_VER;
          (( ToolVersionElementStructType *)Mem)->Data.Used = 0x00;
          break;
        case FLSSIGN_TOOL_ELEM_TYPE:
          ((ToolVersionElementStructType*)Mem)->Data.Required = FLSSIGN_TOOL_REQ_VER;
          (( ToolVersionElementStructType *)Mem)->Data.Used = 0x00;
          break;
        case DWDTOHEX_TOOL_ELEM_TYPE:
          ((ToolVersionElementStructType*)Mem)->Data.Required = DWDTOHEX_TOOL_REQ_VER;
          (( ToolVersionElementStructType *)Mem)->Data.Used = 0x00;
          break;
        case FLASHTOOL_TOOL_ELEM_TYPE:
          ((ToolVersionElementStructType*)Mem)->Data.Required = FLASHTOOL_TOOL_REQ_VER;
          (( ToolVersionElementStructType *)Mem)->Data.Used = 0x00;
          break;
        case STORAGETOOL_TOOL_ELEM_TYPE:
          ((ToolVersionElementStructType*)Mem)->Data.Required = STORAGETOOL_TOOL_REQ_VER;
          (( ToolVersionElementStructType *)Mem)->Data.Used = 0x00;
          break;
				case FLSTOHEADER_TOOL_ELEM:
					((ToolVersionElementStructType*)Mem)->Data.Required = FLSTOHEADER_TOOL_REQ_VER;
					(( ToolVersionElementStructType *)Mem)->Data.Used = 0x00;
					break;
				case BOOT_CORE_TOOL_ELEM:
					((ToolVersionElementStructType*)Mem)->Data.Required = BOOT_CORE_TOOL_REQ_VER;
					(( ToolVersionElementStructType *)Mem)->Data.Used = 0x00;
					break;
				case EBL_TOOL_ELEM:
					((ToolVersionElementStructType*)Mem)->Data.Required = EBL_TOOL_REQ_VER;
					(( ToolVersionElementStructType *)Mem)->Data.Used = 0x00;
					break;
				case FLSTONAND_TOOL_ELEM:
					((ToolVersionElementStructType*)Mem)->Data.Required = FLSTONAND_TOOL_REQ_VER;
					(( ToolVersionElementStructType *)Mem)->Data.Used = 0x00;
					break;
			}
		}
	}
}

ulong32 PRGSequencer::GetFileLength()
{
  ulong32 start_index,end_index;
  ulong32 uid = GENERIC_UID;
  char *Mem;

  Mem = (char*)Input->Memory;
  if(Mem == NULL)
    return 0;
    
  if( PRGH_GetElementIndex(Mem,GENERIC_HEADER_ELEM_TYPE,uid,&start_index) &&
      PRGH_GetElementIndex(Mem,END_MARKER_ELEM_TYPE,uid,&end_index))
  {
    return (end_index - start_index + sizeof(EndElementStructType));
  }
  else
    return 0;
}