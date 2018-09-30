#include <stdio.h>
#include <stdlib.h>
#if defined(UNIX_BUILD)
#include <memory.h>
#else 
#include <mem.h>
#include <vcl.h> 
#endif

#include <string.h>
#include "IFWD_StringList.h"
#include "Control.h"
#include "PrgSequencer.h"
#include "coverage.h"

#if defined(UNIX_BUILD)
char* extractfilename_unix(const char*path, char* filename);
char filename_extracted[MAX_FILENAME_LENGTH];
#endif

#define LOAD_MAGIC 0x01FEDABE

//Local variables:
//Static elements from PRG sequencer:
GenericHeaderElementStructType* GenericElementPtr;
ToolVersionElementStructType* ToolElementPtr;
ToolVersionElementStructType* FirstToolElementPtr;

//Dynamic elements from PRG sequencer:
TocElementStructType* TocElementPtr;
SecurityElementStructType* SecurityElements[MAX_LOADMAP_ENTRIES];
DownloadDataElementStructType* DownloadElements[MAX_LOADMAP_ENTRIES];
bool NewElementEnabled;

TocElementStructType* InjectTocElementPtr;
SecurityElementStructType* InjectSecurityElement;
VersionDataElementStructType* InjectVersionElementPtr;                                
VersionDataElementStructType* InjectPsiVersionElementPtr;                                
VersionDataElementStructType* InjectEblVersionElementPtr;                                
DownloadDataElementStructType* InjectDownloadDataPtr;
InjectionElementStructType* PsiInjectStruct;                                
InjectionElementStructType* EblInjectStruct;                                

                                
//Local structures:
TocElementStructType* FirstTocElementPtr;
TocEntryStructType* ThisTocDataPtr;
TocEntryStructType FirstTocData[50];
TocEntryStructType TocData[50];
ulong32 CurrentTocUid;
ulong32 NoOfSecStructs;
ulong32 NoOfDownloadStructs;
char* LocalDynamic[MAX_DYN_ENTRIES];
ulong32 DynamicCounter;
ulong32 PrgHash[5];
PRGSequencer FirstFile;
PRGSequencer NextFile;
PRGSequencer PSI;
PRGSequencer EBL;


LocalMemoryMapEntryStructType LocalLoadmap[MAX_LOADMAP_ENTRIES];


/*
   Call back function from PRG sequencer - provides pointers to the
   elements read from the file.
*/

void NewElement(void* ElementPtr)
{
ulong32 i;
   if(NewElementEnabled)
   switch(((ElementHeaderStructType*)ElementPtr)->Type)
   {
      case GENERIC_HEADER_ELEM_TYPE:
        COV;
        GenericElementPtr= (GenericHeaderElementStructType*)ElementPtr;
      break;
      case FLSSIGN_TOOL_ELEM_TYPE:
        COV;
        ToolElementPtr = (ToolVersionElementStructType*)ElementPtr;
      break;
      case TOC_ELEM_TYPE:
        COV;
        TocElementPtr= (TocElementStructType*)ElementPtr;
        for(i=0;i<TocElementPtr->Data.NoOfEntries;i++)
           ThisTocDataPtr[i]=TocElementPtr->Data.Data[i];
        TocElementPtr->Data.Data=ThisTocDataPtr;// point to local array
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
      case INJECTED_PSI_ELEM_TYPE:
        COV;
        PsiInjectStruct=(InjectionElementStructType*)ElementPtr;                                
      break;   
      case INJECTED_EBL_ELEM_TYPE:
        COV;
        EblInjectStruct=(InjectionElementStructType*)ElementPtr;            
      break;
      case INJECTED_PSI_VERSION_ELEM_TYPE:
        COV;
        InjectPsiVersionElementPtr=(VersionDataElementStructType*)ElementPtr;                                
      break;
      case INJECTED_EBL_VERSION_ELEM_TYPE:
        COV;
        InjectEblVersionElementPtr=(VersionDataElementStructType*)ElementPtr;                                
      break;
   }
}

void InjectionNewElement(void* ElementPtr)
{

   switch(((ElementHeaderStructType*)ElementPtr)->Type)
   {
      case TOC_ELEM_TYPE:
         COV;
      //printf("INJ TOC\n");
         InjectTocElementPtr=(TocElementStructType*)ElementPtr;
      break;          
      case DOWNLOADDATA_ELEM_TYPE:
         COV;
      //printf("INJ DL\n");
         InjectDownloadDataPtr=(DownloadDataElementStructType*)ElementPtr;
      break;                                    
      case SECURITY_ELEM_TYPE:
         COV;
      //printf("INJ SEC\n");
         InjectSecurityElement=(SecurityElementStructType*)ElementPtr;
      break;
      case GENERIC_VERSION_ELEM_TYPE:
         COV;
      //printf("INJ Version\n");
         InjectVersionElementPtr=(VersionDataElementStructType*)ElementPtr;
      break;
   }
}

/*
   Handle list of localy allocated memory - used to ease delete process
*/

void FreeLocalDynamicData(void)
{
ulong32 i;
   COV;
   for(i=0;i<DynamicCounter;i++)
   {
      COV;
      if(LocalDynamic[i])
      {
         COV;
         free(LocalDynamic[i]);
         LocalDynamic[i]=0;
      }
   }
   DynamicCounter=0;
}
void AddLocalDynamicData(char* ThisPtr)
{
   COV;
   LocalDynamic[DynamicCounter++]=ThisPtr;
}
void InitLocalDynamicData(void)
{
int i;
   COV;
   for(i=0;i<MAX_DYN_ENTRIES;i++)
   {
      LocalDynamic[i]=0;
   }
   DynamicCounter=0;
}       

/*
   Local load map handling
*/

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
}

/*
   Check existence of a given file
*/
static char ANSI_FileExists(char *filename)
{
  FILE *in;
  if((in = fopen(filename,"rb")) == NULL)
  {
    return 0;
  }
  fclose(in);
  return 1;
}

/*
   Overall functionality
*/

void Control(char* Output, 
             IFWD_StringList* FileList, 
             char* Psi,
             char* Ebl,
             ulong32 RemoveFlag,
			 ulong32 LocalSigning,
			 char* HsmPassword)
{
ulong32 i,j,Result,FileCounter,TocCounter,SecCounter;
ulong32 CurrentMemClass;
ulong32 CRC;
void* ThisElementPtr;
ulong32 InjectResult;
bool InsertFileName;
bool Done;
   UNIT_COV_INIT;
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
// 2) process input files one by one:
   InitLocalDynamicData();
   if(FileList->Count>1)
   {
     // more than one input file, so insert file names in TOC
     InsertFileName=true;
   }
   else
   {
     // only one input file, so keep file names in TOC
     InsertFileName=false;           
   }
   for(FileCounter=0;FileCounter<FileList->Count;FileCounter++)
   {
      //clear dyn pointers:
      COV;
      for(i=0;i<MAX_LOADMAP_ENTRIES;i++)
      {
         SecurityElements[i]=0;
         DownloadElements[i]=0;
      }
      NoOfDownloadStructs=0;
      NoOfSecStructs=0;
      //Read the file(s):
      
      printf("\nProcessing file: %s\n",FileList->Strings[FileCounter]);
      if(FileCounter==0)
      {
         COV;
         InjectPsiVersionElementPtr=0;
         InjectEblVersionElementPtr=0;
         PsiInjectStruct=0;
         EblInjectStruct=0;
         ThisTocDataPtr=FirstTocData;
		 NewElementEnabled=true;
         Result=FirstFile.ReadFile(FileList->Strings[FileCounter],FLSSIGN_TOOL_ELEM_TYPE,MY_VERSION,NewElement);
		 NewElementEnabled=false;
         //save pointers from the first file for later use
         FirstTocElementPtr=TocElementPtr;
         FirstToolElementPtr=ToolElementPtr;
         //remove injected files from input:
         if (InjectPsiVersionElementPtr||
             InjectEblVersionElementPtr||
             PsiInjectStruct||
             EblInjectStruct)
         {
            COV;
            printf("\nRemoving previously injected elements from input...\n");
            FirstFile.RemoveElement(InjectPsiVersionElementPtr);
            FirstFile.RemoveElement(InjectEblVersionElementPtr);
            FirstFile.RemoveElement(PsiInjectStruct);
            FirstFile.RemoveElement(EblInjectStruct);
         }
      }
      else
      {
         COV;
         NextFile.CleanUp();  
         ThisTocDataPtr=TocData;
		 NewElementEnabled=true;
         Result=NextFile.ReadFile(FileList->Strings[FileCounter],FLSSIGN_TOOL_ELEM_TYPE,MY_VERSION,NewElement);
         // Prevent the element creation functions call to NewElement
         // to insert elements in our SEC and DL element lists by
         // setting NewElementEnabled to false.
         NewElementEnabled=false; 
      }
      // Check the result of the file read operation
      if (Result!=0)   
      {
         COV_ERR;
         // Something wrong
         if (Result!=0xFFFFFFFF)   
         {
            COV_ERR;
            // Not invalid file so...
            if((Result>>16)<MY_VERSION_MAJOR)
            {
               // The input is too old because MAJOR of input is smaller than ours
               printf("\nError: Input file too old - use NuFlsSign %d.%d !\n",Result>>16,Result&0xFFFF);
               exit (0);
            }
            else
            {
               // The input is too new
               printf("\nError: NuFlsSign version %d.%d - must be used!\n",Result>>16,Result&0xFFFF);
               exit (0);
            }
         }    
         else
         {
            COV_ERR;
            // Invalid file format
            printf("\nInvalid file format!\n");
            exit (0);
         }                             
      }
      else
      {
         COV;
         // Input file ok.
         if((GenericElementPtr->Data.FileType&0x00FFFFFF)!=FLS_FILE_TYPE)
         {
            COV_ERR;
            printf("\nInvalid input file type!\n");
            printf("File: %s\n",FileList->Strings[FileCounter]);
            exit (0);            
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
               COV_ERR;
               printf("WARNING: %s was generated with a different PRG file than %s!\n\n",FileList->Strings[FileCounter],FileList->Strings[FileCounter-1]);
               break;
            }
         }             
      }     
      for(i=0;i<5;i++)PrgHash[i]=GenericElementPtr->Data.Sha1Digest[i];
      
      // process each TOC element entry in the file
      for(TocCounter=0;TocCounter<TocElementPtr->Data.NoOfEntries;TocCounter++)
      { 
         // find SEC with current UID:
         COV;
         for(SecCounter=0;SecCounter<NoOfSecStructs;SecCounter++)
         {
            COV;
            if(SecurityElements[SecCounter]->Header.UID==ThisTocDataPtr[TocCounter].UID)
            {
               // found so create load map:
               COV;
               CreateLocalLoadmap(SecurityElements[SecCounter]);
               // add boot core and ebl versions:
               SecurityElements[SecCounter]->Data.BootCoreVersion=BOOT_CORE_TOOL_REQ_VER;
               SecurityElements[SecCounter]->Data.EblVersion=EBL_TOOL_REQ_VER;
               // and exit loop so we preserve SecCounter:
               break;
            }
         }
         if(FileCounter!=0)
         {
            // put the data from NextFile into FirstFile.
            // Update TOC element:
            COV;
            FirstTocData[FirstTocElementPtr->Data.NoOfEntries]=TocData[TocCounter];
            FirstTocData[FirstTocElementPtr->Data.NoOfEntries].UID=FirstTocElementPtr->Data.NoOfEntries;
            if(InsertFileName)
            {
                #if defined(UNIX_BUILD)
                strcpy((char*)FirstTocData[FirstTocElementPtr->Data.NoOfEntries].FileName,
                      extractfilename_unix(FileList->Strings[FileCounter],filename_extracted));
                #else										
                strcpy(FirstTocData[FirstTocElementPtr->Data.NoOfEntries].FileName,
                      ExtractFileName(FileList->Strings[FileCounter]).c_str());
                #endif
            }
            //Create SEC
            ThisElementPtr=FirstFile.CreateElement(SECURITY_ELEM_TYPE);
            // find SEC with current UID:
            for(SecCounter=0;SecCounter<NoOfSecStructs;SecCounter++)
            {
               COV;
               if(SecurityElements[SecCounter]->Header.UID==TocData[TocCounter].UID)
               {
                  // found so copy
                  COV;
                  *((SecurityElementStructType*)ThisElementPtr)=*SecurityElements[SecCounter];
                  ((SecurityElementStructType*)ThisElementPtr)->Header.UID=FirstTocElementPtr->Data.NoOfEntries;
               }
            }
            // now create and copy download elements:
            for(i=0;i<MAX_LOADMAP_ENTRIES;i++)
            {
               if(LocalLoadmap[i].Region.UsedLength)
               {
                  COV;
                  ThisElementPtr=FirstFile.CreateElement(DOWNLOADDATA_ELEM_TYPE);
                  *((DownloadDataElementStructType*)ThisElementPtr)=*LocalLoadmap[i].ElementPtr;
                  ((DownloadDataElementStructType*)ThisElementPtr)->Header.UID=FirstTocElementPtr->Data.NoOfEntries;
               }                    
            }
            // update TOC entry counter
            FirstTocElementPtr->Data.NoOfEntries++;
         }
      }
   }
// Handle injections:   
   
   // handle psi injection
   if(Psi[0]!=0)
   {
      COV;
      if(!ANSI_FileExists(Psi))
      {
         COV_ERR;
         printf("\nCannot open psi file (%s)!\n",Psi);
         exit (0);
      }
      InjectVersionElementPtr=0;//clear pointer so we know if version is present in file
      InjectResult=PSI.ReadFile(Psi,FLSSIGN_TOOL_ELEM_TYPE,MY_VERSION,InjectionNewElement);
      if (InjectResult!=0)   
      {
         COV_ERR;
         if (InjectResult!=0xFFFFFFFF)   
         {
            COV_ERR;
            // Not invalid file so...
            if((Result>>16)<MY_VERSION_MAJOR)
            {
               // The input is too old because MAJOR of input is smaller than ours
               printf("\nError: Input file too old - use NuFlsSign %d.%d !\n",Result>>16,Result&0xFFFF);
               exit (0);
            }
            else
            {
               // The input is too new
               printf("\nPSI injection error: NuFlsSign version %d.%d - must be used!\n",Result>>16,Result&0xFFFF);
               exit (0);
            }
         }    
         else
         {
            COV_ERR;
            printf("\nInvalid PSI format!\n");
            exit (0);
         }                             
      }
      if(InjectTocElementPtr->Data.NoOfEntries==1)
      {
         COV;
         if(InjectDownloadDataPtr->Data.LoadMapIndex==0)
         {
            COV;
            printf("Injecting psi from: %s\n",Psi);
            PsiInjectStruct=(InjectionElementStructType*)FirstFile.CreateElement(INJECTED_PSI_ELEM_TYPE);
            PsiInjectStruct->Data.CRC_16=InjectDownloadDataPtr->Data.CRC;
            PsiInjectStruct->Data.CRC_8=
               (InjectDownloadDataPtr->Data.CRC>>8)^(InjectDownloadDataPtr->Data.CRC&0xFF);
            PsiInjectStruct->Data.DataLength=InjectDownloadDataPtr->Data.DataLength;
            PsiInjectStruct->Data.Data=InjectDownloadDataPtr->Data.Data;
            if(InjectVersionElementPtr)
            {
               COV;
               InjectPsiVersionElementPtr=(VersionDataElementStructType*)FirstFile.CreateElement(INJECTED_PSI_VERSION_ELEM_TYPE);
               InjectPsiVersionElementPtr->Data=InjectVersionElementPtr->Data;
            }
            else
            {
               COV;
               printf("WARNING: PSI file contains no version info!\n");
            }     
         }      
      }
      else
      {
         COV_ERR;
         printf("ERROR: PSI file contains more than one load region!\n");
         exit (0);
      }       
   }
   // handle ebl injection
   if(Ebl[0]!=0)
   {
      COV;
      if(!ANSI_FileExists(Ebl))
      {
         COV_ERR;
         printf("\nCannot open ebl file (%s)!\n",Ebl);
         exit (0);
      }
      InjectVersionElementPtr=0;//clear pointer so we know if version is present in file
      InjectResult=EBL.ReadFile(Ebl,FLSSIGN_TOOL_ELEM_TYPE,MY_VERSION,InjectionNewElement);
      if (InjectResult!=0)   
      {
         COV_ERR;
         if (InjectResult!=0xFFFFFFFF)   
         {
            COV_ERR;
            // Not invalid file so...
            if((Result>>16)<MY_VERSION_MAJOR)
            {
               // The input is too old because MAJOR of input is smaller than ours
               printf("\nError: Input file too old - use NuFlsSign %d.%d !\n",Result>>16,Result&0xFFFF);
               exit (0);
            }
            else
            {
               // The input is too new
               printf("\nPSI injection error: NuFlsSign version %d.%d - must be used!\n",Result>>16,Result&0xFFFF);
               exit (0);
            }
         }    
         else
         {
            COV_ERR;
            printf("\nInvalid EBL format!\n");
            exit (0);
         }                             
      }
      if(InjectTocElementPtr->Data.NoOfEntries==1)
      {
         COV;
         if(InjectDownloadDataPtr->Data.LoadMapIndex==0)
         {
            COV;
            printf("Injecting ebl from: %s\n",Ebl);
            EblInjectStruct=(InjectionElementStructType*)FirstFile.CreateElement(INJECTED_EBL_ELEM_TYPE);
            EblInjectStruct->Data.CRC_16=InjectDownloadDataPtr->Data.CRC;
            EblInjectStruct->Data.CRC_8=
               (InjectDownloadDataPtr->Data.CRC>>8)^(InjectDownloadDataPtr->Data.CRC&0xFF);
            EblInjectStruct->Data.DataLength=InjectDownloadDataPtr->Data.DataLength;
            EblInjectStruct->Data.Data=InjectDownloadDataPtr->Data.Data;
            if(InjectVersionElementPtr)
            {
               COV;
               InjectEblVersionElementPtr=(VersionDataElementStructType*)FirstFile.CreateElement(INJECTED_EBL_VERSION_ELEM_TYPE);
               InjectEblVersionElementPtr->Data=InjectVersionElementPtr->Data;
            }
            else
            {
               COV;
               printf("WARNING: EBL file contains no version info!\n");
            }     
         }      
      }
      else
      {
         COV_ERR;
         printf("ERROR: EBL file contains more than one load region!\n");
         exit (0);
      }           
    }  
   // update tool version:
   FirstToolElementPtr->Data.Used=MY_VERSION;
   GenericElementPtr->Data.FileType&=0xFF000000;
   GenericElementPtr->Data.FileType|=FLS_FILE_TYPE;
   // save file:
   FirstFile.WriteFile(Output);
   
   // Free buffers:
   FreeLocalDynamicData();
   FirstFile.CleanUp();
   NextFile.CleanUp();  
   PSI.CleanUp();  
   EBL.CleanUp();

   // Insert boot core removal here:
  if(RemoveFlag)
  {
    printf("Removing boot core items\n");
    
    for(i=0;i<MAX_LOADMAP_ENTRIES;i++)
    {
       SecurityElements[i]=0;
       DownloadElements[i]=0;
    }
    NoOfDownloadStructs=0;
    NoOfSecStructs=0;
    COV;
    InjectPsiVersionElementPtr=0;
    InjectEblVersionElementPtr=0;
    PsiInjectStruct=0;
    EblInjectStruct=0;
    ThisTocDataPtr=FirstTocData;
    NewElementEnabled=true;
    Result=FirstFile.ReadFile(Output,FLSSIGN_TOOL_ELEM_TYPE,MY_VERSION,NewElement);
    NewElementEnabled=false;
    FirstTocElementPtr=TocElementPtr;
   
     // search the TOC for boot core class
     // Search for the sec and download element
     // remove toc entry + update counter
     // remove the sec element
     // remove the download element
     //
       // find TOC entry with code class BOOT_CORE:
        for(TocCounter=0;TocCounter<FirstTocElementPtr->Data.NoOfEntries;TocCounter++)
        {
          if((FirstTocElementPtr->Data.Data[TocCounter].MemoryClass==BOOT_CORE_PSI_CLASS)
           ||(FirstTocElementPtr->Data.Data[TocCounter].MemoryClass==BOOT_CORE_SLB_CLASS)
           ||(FirstTocElementPtr->Data.Data[TocCounter].MemoryClass==BOOT_CORE_EBL_CLASS))
          {
            for(SecCounter=0;SecCounter<NoOfSecStructs;SecCounter++)
            {
              COV;
              if(SecurityElements[SecCounter]->Header.UID==FirstTocElementPtr->Data.Data[TocCounter].UID)
              {
                // found
                for (i=0;i<NoOfDownloadStructs;i++)//create buffers and
                {
                  COV;
                  if(DownloadElements[i]->Header.UID==SecurityElements[SecCounter]->Header.UID)
                  {
                    FirstFile.RemoveElement(DownloadElements[i]);
                  }
                }
                FirstFile.RemoveElement(SecurityElements[SecCounter]);
              }
            }
          }
          COV;
        }
        i=0;
     Done=false;
     while(!Done)
     {
        for(TocCounter=0;TocCounter<FirstTocElementPtr->Data.NoOfEntries;TocCounter++)
        {
          Done=true;
          if((FirstTocElementPtr->Data.Data[TocCounter].MemoryClass==BOOT_CORE_PSI_CLASS)
           ||(FirstTocElementPtr->Data.Data[TocCounter].MemoryClass==BOOT_CORE_SLB_CLASS)
           ||(FirstTocElementPtr->Data.Data[TocCounter].MemoryClass==BOOT_CORE_EBL_CLASS))
          {
            for(j=TocCounter;j<FirstTocElementPtr->Data.NoOfEntries;j++)
            {
              FirstTocElementPtr->Data.Data[j]=FirstTocElementPtr->Data.Data[j+1];
            }
            FirstTocElementPtr->Data.NoOfEntries--;
            Done=false;
          }
        }
     }
    //now correct all UID's:
    for(TocCounter=0;TocCounter<FirstTocElementPtr->Data.NoOfEntries;TocCounter++)
    {
      if(FirstTocElementPtr->Data.Data[TocCounter].UID!=TocCounter)
      {
        for (i=0;i<NoOfDownloadStructs;i++)
        {
          if(DownloadElements[i]->Header.UID==FirstTocElementPtr->Data.Data[TocCounter].UID)
          {
            DownloadElements[i]->Header.UID=TocCounter;
          }
        }
        for (i=0;i<NoOfSecStructs;i++)
        {
          if(SecurityElements[i]->Header.UID==FirstTocElementPtr->Data.Data[TocCounter].UID)
          {
            SecurityElements[i]->Header.UID=TocCounter;
          }
        }
        FirstTocElementPtr->Data.Data[TocCounter].UID=TocCounter;
      }
    }
     
     FirstFile.WriteFile(Output);
     // Free buffers:
     FreeLocalDynamicData();
     FirstFile.CleanUp();
  }     
       
}

#if defined(UNIX_BUILD)
char* extractfilename_unix(const char*path, char* filename)
{
  int len=strlen(path);
  const char* ind;
  int i;

  for(i=len-1;i>=0;i--)
    if(path[i]=='/') break;

  ind = &path[i+1];
  strcpy(filename,ind);

  return filename;
}
#endif

 
