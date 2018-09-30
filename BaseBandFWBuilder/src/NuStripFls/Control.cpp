#define UNIX_BUILD
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include <string.h>
//#include "string.h"
#include "IFWD_StringList.h"
#include "Control.h"
//#include "Interpret.h"
#include "PrgSequencer.h"
#include "coverage.h"

#if defined(UNIX_BUILD)
char* extractfilename_unix(const char*path, char* filename);
char filename_extracted[MAX_FILENAME_LENGTH];
#endif

#define LOAD_MAGIC 0x01FEDABE
#define LINE_LENGTH 32

//Local variables:
//Static elements from PRG sequencer:
GenericHeaderElementStructType* GenericElementPtr;
MemoryMapElementStructType* MemoryMapElementPtr;
HardwareElementStructType* HardwareElementPtr;

//Dynamic elements from PRG sequencer:
TocElementStructType* TocElementPtr;
SecurityElementStructType* SecurityElements[MAX_DYN_ENTRIES];
DownloadDataElementStructType* DownloadElements[MAX_DYN_ENTRIES];
                                
//Local structures:
TocEntryStructType TocData[MAX_DYN_ENTRIES];
ulong32 CurrentTocUid;
ulong32 NoOfSecStructs;
ulong32 NoOfDownloadStructs;
char* LocalDynamic[MAX_DYN_ENTRIES];
ulong32 DynamicCounter;
ulong32 PrgHash[5];

PRGSequencer PRG;


LocalMemoryMapEntryStructType LocalLoadmap[MAX_LOADMAP_ENTRIES];

char ANSI_FileExists(char *filename)
{
  FILE *in;
  COV;
  if((in = fopen(filename,"rb")) == NULL)
  {
    COV_ERR;
    return 0;
  }
  fclose(in);
  return 1;
}


/*
   Call back function from PRG sequencer - provides pointers to the
   elements read from the file.
*/

void NewElement(void* ElementPtr)
{
ulong32 i;
   COV;
   switch(((ElementHeaderStructType*)ElementPtr)->Type)
   {
      case GENERIC_HEADER_ELEM_TYPE:
         COV;
         GenericElementPtr= (GenericHeaderElementStructType*)ElementPtr;
      break;
      case HARDWARE_ELEM_TYPE:
         HardwareElementPtr=(HardwareElementStructType*)ElementPtr;
      break;
      case MEMORYMAP_ELEM_TYPE:
         MemoryMapElementPtr=(MemoryMapElementStructType*)ElementPtr;
      break;
      case TOC_ELEM_TYPE:
         COV;
         TocElementPtr= (TocElementStructType*)ElementPtr;
         for(i=0;i<TocElementPtr->Data.NoOfEntries;i++)
            TocData[i]=TocElementPtr->Data.Data[i];
         TocElementPtr->Data.Data=TocData;// point to local array
      //printf("TOC\n");
      break;          
      case DOWNLOADDATA_ELEM_TYPE:
         COV;
         DownloadElements[NoOfDownloadStructs++]=(DownloadDataElementStructType*)ElementPtr;
      break;    
      case SECURITY_ELEM_TYPE:
         COV;
         SecurityElements[NoOfSecStructs++]=(SecurityElementStructType*)ElementPtr;
      //printf("SEC\n");
      break;
   }
}



void ClearLocalLoadmap(void)
{
int i;
   COV;
   for(i=0;i<MAX_LOADMAP_ENTRIES;i++)
   {
      LocalLoadmap[i].Region.StartAddr=0;
      LocalLoadmap[i].Region.TotalLength=0;
      LocalLoadmap[i].Region.UsedLength=0;
      LocalLoadmap[i].Region.ImageFlags=0;
      LocalLoadmap[i].MemoryPtr=0;
      LocalLoadmap[i].ElementPtr=0;
      LocalLoadmap[i].MemoryClass=0xFFFFFFFF;
   }
}

void AddLocalDynamicData(char* ThisPtr)
{
   COV;
   LocalDynamic[DynamicCounter++]=ThisPtr;
}
/*
ulong32 CreateLocalLoadmap(SecurityElementStructType* Sec)
{
ulong32 i,j,ReturnValue;
   COV;
   ReturnValue=0;
   ClearLocalLoadmap();// clear data
   for(j=0;j<MAX_LOADMAP_ENTRIES;j++)// copy from sec
   {
      LocalLoadmap[j].Region.StartAddr=Sec->Data.LoadMap[j].StartAddr;
      LocalLoadmap[j].Region.TotalLength=Sec->Data.LoadMap[j].TotalLength;
      LocalLoadmap[j].Region.UsedLength=Sec->Data.LoadMap[j].UsedLength;
      LocalLoadmap[j].Region.ImageFlags=Sec->Data.LoadMap[j].ImageFlags;
   }
   for (i=0;i<NoOfDownloadStructs;i++)//create buffers and
   {
      COV;
      if(DownloadElements[i]->Header.UID==Sec->Header.UID)
      {
// comment: maybe we need to allocate according to SEC and not to present elements??
         COV;
         LocalLoadmap[DownloadElements[i]->Data.LoadMapIndex].ElementPtr=DownloadElements[i];
         LocalLoadmap[DownloadElements[i]->Data.LoadMapIndex].MemoryClass=Sec->Data.Type;
         LocalLoadmap[DownloadElements[i]->Data.LoadMapIndex].MemoryPtr=
            (char*)malloc(LocalLoadmap[DownloadElements[i]->Data.LoadMapIndex].Region.TotalLength);
         memset(LocalLoadmap[DownloadElements[i]->Data.LoadMapIndex].MemoryPtr,
                0xff,
                LocalLoadmap[DownloadElements[i]->Data.LoadMapIndex].Region.TotalLength);
         memcpy(LocalLoadmap[DownloadElements[i]->Data.LoadMapIndex].MemoryPtr,
                DownloadElements[i]->Data.Data,LocalLoadmap[DownloadElements[i]->Data.LoadMapIndex].Region.UsedLength);
         LocalLoadmap[DownloadElements[i]->Data.LoadMapIndex].ElementPtr->Data.Data=
         (unsigned char*)(LocalLoadmap[DownloadElements[i]->Data.LoadMapIndex].MemoryPtr);
         // add the buffer pointer to a list so it can be free'ed later:
         AddLocalDynamicData(LocalLoadmap[DownloadElements[i]->Data.LoadMapIndex].MemoryPtr);
      }
   }
  return ReturnValue;
}*/

ulong32 CreateLocalLoadmap(SecurityElementStructType* Sec)
{
int i,j,ReturnValue;
   COV;
   ReturnValue=0;
   ClearLocalLoadmap();// clear data
   for(j=0;j<MAX_LOADMAP_ENTRIES;j++)// copy from sec
   {
      LocalLoadmap[j].Region.StartAddr=Sec->Data.LoadMap[j].StartAddr;
      LocalLoadmap[j].Region.TotalLength=Sec->Data.LoadMap[j].TotalLength;
      LocalLoadmap[j].Region.UsedLength=Sec->Data.LoadMap[j].UsedLength;
      LocalLoadmap[j].Region.ImageFlags=Sec->Data.LoadMap[j].ImageFlags;
   }
   for (i=0;i<NoOfDownloadStructs;i++)//Assign pointers and memory class
   {
      COV;
      if(DownloadElements[i]->Header.UID==Sec->Header.UID)
      {
         COV;
         LocalLoadmap[DownloadElements[i]->Data.LoadMapIndex].ElementPtr=DownloadElements[i];
         LocalLoadmap[DownloadElements[i]->Data.LoadMapIndex].MemoryClass=Sec->Data.Type;
         LocalLoadmap[DownloadElements[i]->Data.LoadMapIndex].MemoryPtr=
            (char *)(LocalLoadmap[DownloadElements[i]->Data.LoadMapIndex].ElementPtr->Data.Data);
			//(char*)malloc(LocalLoadmap[DownloadElements[i]->Data.LoadMapIndex].Region.TotalLength);
      }
   }
  return ReturnValue;
}

void DoMakeHeader(char* Fin, char* Fout)
{
	char* tempLine;
	char tempCh[20];
	IFWD_MemoryStream* pms1 = new IFWD_MemoryStream();
	IFWD_StringList* hdrFile = new IFWD_StringList();
	unsigned long Length;
	unsigned char* InPtr;
	unsigned char CheckSum=0;
	unsigned int i;
	char*  fileName;
	if (ANSI_FileExists(Fin))
	{
		pms1->LoadFromFile(Fin);
		Length=pms1->Size;
		InPtr=(unsigned char*)pms1->Memory;
		hdrFile->Add(" ");
		hdrFile->Add((char *)(("#define FLS_HEADER_LENGTH %d", Length)));

		hdrFile->Add("const unsigned char FLS_HEADER[FLS_HEADER_LENGTH] = {");
		tempLine = "";
		for (i=0; i<Length-1; i++)
		{
			if ((i & 0xf) == 0)
			{
				if (tempLine != "")
				{
					hdrFile->Add((char *)tempLine);
				}
				tempLine = "  ";
		}
			sprintf(tempCh,"0x%02x, ",*InPtr);
			CheckSum^=*InPtr++;
			tempLine = strcat(tempLine, (char *)tempCh);
		}
		sprintf(tempCh,"0x%02x",*InPtr);
		CheckSum^=*InPtr++;
		tempLine = strcat(tempLine, tempCh);
		hdrFile->Add((char *)tempLine);
		hdrFile->Add("  };");
		hdrFile->Add(" ");
		hdrFile->SaveToFile(Fout);
	}
	else
	{
		printf("Cannot open %s\n",Fin);
		exit(0);
	}
	delete hdrFile;
	delete pms1;
}


void Control(char* Output, char* HeaderFile,char* BinHeaderFile, IFWD_StringList* FileList)
{
ulong32 i,LoadmapCounter,Result,FileCounter,TocCounter;
ulong32 CurrentMemClass;
ulong32 LoadAddr,LoadLen;
ulong32 DownloadIndex;
DownloadDataElementStructType* ThisIndirectElement;
IFWD_MemoryStream* OutputFile;
IFWD_MemoryStream* HdrFile;
char* MemPtr;
   COV;
   if(FileList->Count>1)
   {
    COV_ERR;
    printf("\nOnly one input file is allowed!\n");
    exit (0);
   }
//1) existence check of files:
   for(FileCounter=0;FileCounter<FileList->Count;FileCounter++)
   {
      COV;
      if(!ANSI_FileExists(FileList->Strings[FileCounter]))
      {
         COV_ERR;
         printf("\nCannot open input file (%s)!\n",FileList->Strings[FileCounter]);
         exit (0);
      }
   }

// 2) Create empty output file(s):
   OutputFile = new IFWD_MemoryStream;
   
   
// 2) process input files one by one:
   for(FileCounter=0;FileCounter<FileList->Count;FileCounter++)
   {
      //clear dyn pointers:
      COV;
      for(i=0;i<MAX_LOADMAP_ENTRIES;i++)
      {
         SecurityElements[i]=0;
         DownloadElements[i]=0;
      }
      NoOfSecStructs=0;
      NoOfDownloadStructs=0;
      PRG.CleanUp();  
      //Read the file:
      printf("Processing file: %s\n",FileList->Strings[FileCounter]);
      
      Result=PRG.ReadFile(FileList->Strings[FileCounter],DWDTOHEX_TOOL_ELEM_TYPE,MY_VERSION,NewElement);
      if (Result!=0)   
      {
         COV_ERR;
         if (Result!=0xFFFFFFFF)   
         {
            COV_ERR;
            // Not invalid file so...
            if((Result>>16)<MY_VERSION_MAJOR)
            {
               COV_ERR;
               // The input is too old because MAJOR of input is smaller than ours
               printf("\nError: Input file too old - use StripFls_E2 %d.%d !\n",Result>>16,Result&0xFFFF);
               exit (0);
            }
            else
            {
               COV_ERR;
               // The input is too new
               printf("\nError: StripFls_E2 version %d.%d - must be used!\n",Result>>16,Result&0xFFFF);
               exit (0);
            }
         }    
         else
         {
            COV_ERR;
            printf("\nInvalid file format!\n");
            printf("File: %s\n",FileList->Strings[FileCounter]);
            exit (0);            
         }                             
      }
      else
      {
         COV;
         if((GenericElementPtr->Data.FileType&0x00FFFFFF)!=FLS_FILE_TYPE)
         {
            COV_ERR;
            printf("\nInvalid input file type!\n");
            printf("File: %s\n",FileList->Strings[FileCounter]);
            exit (0);            
         }
         else
         {
            COV;
            if(GenericElementPtr->Data.FileType!=FLS_FILE_TYPE)
            {
               COV;
               printf("\nWARNING:\n");
               printf("File: %s was made with a derived PRG file!\n\n",FileList->Strings[FileCounter]);
            }
         }        
      }
      // hash comparison:
      if(FileCounter!=0)
      {
         COV;
         for(i=0;i<5;i++)
         {
            if(PrgHash[i]!=GenericElementPtr->Data.Sha1Digest[i])
            {
               COV;
               printf("\nWARNING:\n");
               printf("File: %s was generated with a different PRG file than %s!\n\n",FileList->Strings[FileCounter],FileList->Strings[FileCounter-1]);
               break;
            }
         }             
      }
      else
      {
         OutputFile->SetSize(HardwareElementPtr->Data.SystemSize);
         memset(OutputFile->Memory,0xFF,HardwareElementPtr->Data.SystemSize);
      }    
      for(i=0;i<5;i++)PrgHash[i]=GenericElementPtr->Data.Sha1Digest[i];
      
      // process each TOC element entry in the file
      for(TocCounter=0;TocCounter<TocElementPtr->Data.NoOfEntries;TocCounter++)
      {
         // find SEC with current UID:
         COV;
         for(i=0;i<NoOfSecStructs;i++)
         {
            COV;
            if(SecurityElements[i]->Header.UID==TocData[TocCounter].UID)
            {
               // found so create load map
               COV;
               CreateLocalLoadmap(SecurityElements[i]);
            }
         }
         // process each used load map entry
         for(LoadmapCounter=0;LoadmapCounter<MAX_LOADMAP_ENTRIES;LoadmapCounter++)  
         {
            COV;
            if(LocalLoadmap[LoadmapCounter].Region.UsedLength)
            {
               COV;
               // inform user about what we are doing and initialize variables
               printf("Processing Memory class: ");
               switch (LocalLoadmap[LoadmapCounter].MemoryClass)
               {
                  case BOOT_CORE_PSI_CLASS:
                     printf("BOOT CORE PSI\n");
                  break;     
                  case BOOT_CORE_SLB_CLASS:
                     printf("BOOT CORE SLB\n");
                  break;  
                  case BOOT_CORE_EBL_CLASS:
                     printf("BOOT CORE EBL\n");
                  break;  
                  case CODE_CLASS:
                     printf("CODE\n");
                  break;  
                  case CUST_CLASS:
                     printf("CUST\n");
                  break;  
                  case DYN_EEP_CLASS:
                     printf("DYN EEP\n");
                  break;  
                  case STAT_EEP_CLASS:
                     printf("STAT EEP\n");
                  break;  
                  case SWAP_BLOCK_CLASS:
                     printf("SWAP BLOCK\n");
                  break;  
                  case SEC_BLOCK_CLASS:
                     printf("SEC BLOCK\n");
                  break;  
                  case EXCEP_LOG_CLASS:
                     printf("EXCEP LOG\n");
                  break;  
                  case STAT_FFS_CLASS:
                     printf("STAT FFS\n");
                  break;  
                  case DYN_FFS_CLASS:
                     printf("DYN FFS\n");
                  break;  
                  case DSP_SW_CLASS:
                     printf("DSP SW\n");
                  break;  
                  case ROOT_DISK_CLASS:
                     printf("ROOT DISK\n");
                  break;  
                  case CUST_DISK_CLASS:
                     printf("CUST DISK\n");
                  break;  
                  case USER_DISK_CLASS:
                     printf("USER DISK\n");
                  break;  
               }
               LoadAddr=LocalLoadmap[LoadmapCounter].Region.StartAddr;
               LoadLen=LocalLoadmap[LoadmapCounter].Region.UsedLength;
               printf("Addr:   %08X \n",LoadAddr);
               printf("Length: %08X \n\n",LoadLen);
               MemPtr=(char*)OutputFile->Memory;
               // copy data into output:
               memcpy(&MemPtr[LoadAddr-MemoryMapElementPtr->Data.FlashStartAddr],
                       LocalLoadmap[LoadmapCounter].MemoryPtr,
                       LoadLen);
               // Save the Offset in the download element
               // - this is to help header generation:
               LocalLoadmap[LoadmapCounter].ElementPtr->Data.DataOffset=
                  LoadAddr-MemoryMapElementPtr->Data.FlashStartAddr;
            }
         }
      }
   }
   COV;
   if ((HeaderFile[0]!=0)||(BinHeaderFile[0]!=0) )
   {
   // generate header file:
      for(DownloadIndex=0;DownloadIndex<NoOfDownloadStructs;DownloadIndex++)
      {
        // Create an INDIRECT_DOWNLOADDATA_ELEM_TYPE:
        ThisIndirectElement=(DownloadDataElementStructType*)
          PRG.CreateElement(INDIRECT_DOWNLOADDATA_ELEM_TYPE);
        // Copy "static" data from Download element:
        ThisIndirectElement->Data=DownloadElements[DownloadIndex]->Data;
        // Remove download element:
        PRG.RemoveElement((void*)DownloadElements[DownloadIndex]);
      }
      // write the file:
      if(BinHeaderFile[0]!=0)
      {
        PRG.WriteFile(BinHeaderFile);
        if(HeaderFile[0]!=0)
        {
          DoMakeHeader(BinHeaderFile,HeaderFile);  
        }
      }
      else
      {
        PRG.WriteFile(HeaderFile);
        DoMakeHeader(HeaderFile,HeaderFile);  
      }
   }
   // clean up and close output file   
   PRG.CleanUp();  
   OutputFile->SaveToFile(Output);
   delete OutputFile;         
}
 