#ifndef __HEADER_H__
#define __HEADER_H__


#include "ToolVersions.h"

#if defined(UNIX_BUILD)
#define __fastcall 
#endif

#define ulong32 unsigned
#define ushort16 unsigned short
#define ubyte unsigned char



#define PRG_MAGIC_NUMBER 0x693729F1
#define GENERIC_UID 0 // To start with, all the elements will have same UID as 0

#define E_GOLD_CHIPSET_V2     0
#define E_GOLD_LITE_CHIPSET   1
#define S_GOLD_CHIPSET_V1     2
#define S_GOLD_LITE_CHIPSET   3
#define E_GOLD_CHIPSET_V3     4
#define S_GOLD_CHIPSET_V2     5
#define S_GOLD_CHIPSET_V3     6
#define E_GOLD_RADIO_V1       7
#define E_GOLD_VOICE_V1       8
#define S_GOLD_RADIO_V1       9

#define NUM_CHIPSELECT_OPTIONS 10

#define NOR_FLASH    0
#define NAND_FLASH   1


#define LOAD_MAGIC 0x01FEDABE

#define MAX_FILENAME_LENGTH 128

typedef enum
{
  PRG_FILE_TYPE = 1,
  FLS_FILE_TYPE,
  EEP_FILE_TYPE,
  DFFS_FILE_TYPE,
  DISK_FILE_TYPE,
  NUM_FILE_TYPES // This should be the last
} prg_file_type;

typedef enum
{
  GENERIC_HEADER_ELEM_TYPE = 1000,//0x3E8
  END_MARKER_ELEM_TYPE=2,
  PRGHANDLER_TOOL_ELEM_TYPE, // All the tool version element types
  PRGSEQUENCER_TOOL_ELEM_TYPE,
  MAKEPRG_TOOL_ELEM_TYPE,
  HEXTOFLS_TOOL_ELEM_TYPE,
  FLSSIGN_TOOL_ELEM_TYPE,
  DWDTOHEX_TOOL_ELEM_TYPE,
  FLASHTOOL_TOOL_ELEM_TYPE,
  STORAGETOOL_TOOL_ELEM_TYPE,
  MEMORYMAP_ELEM_TYPE,
  DOWNLOADDATA_ELEM_TYPE,
  HARDWARE_ELEM_TYPE,
  NANDPARTITION_ELEM_TYPE,
  SECURITY_ELEM_TYPE,
  TOC_ELEM_TYPE,
  SAFE_BOOT_CODEWORD_ELEM_TYPE,
  INJECTED_PSI_ELEM_TYPE,
  INJECTED_EBL_ELEM_TYPE,
  GENERIC_VERSION_ELEM_TYPE,
  INJECTED_PSI_VERSION_ELEM_TYPE,
  INJECTED_EBL_VERSION_ELEM_TYPE,
  ROOT_DISK_ELEM_TYPE,
  CUST_DISK_ELEM_TYPE,
  USER_DISK_ELEM_TYPE,
	FLSTOHEADER_TOOL_ELEM,
	BOOT_CORE_TOOL_ELEM,
	EBL_TOOL_ELEM,
	INDIRECT_DOWNLOADDATA_ELEM_TYPE,
	FLSTONAND_TOOL_ELEM,
	NUM_ELEM_TYPES // This should be the last
} prg_element_type;

// --------------------------
// Memory Class defines
//---------------------------
#define NOT_USED              0
#define BOOT_CORE_PSI_CLASS   1
#define BOOT_CORE_SLB_CLASS   2
#define BOOT_CORE_EBL_CLASS   3
#define CODE_CLASS            4
#define CUST_CLASS            5
#define DYN_EEP_CLASS         6
#define STAT_EEP_CLASS        7
#define SWAP_BLOCK_CLASS      8
#define SEC_BLOCK_CLASS       9
#define EXCEP_LOG_CLASS       10
#define STAT_FFS_CLASS        11
#define DYN_FFS_CLASS         12
#define DSP_SW_CLASS          13
#define ROOT_DISK_CLASS       14
#define CUST_DISK_CLASS       15
#define USER_DISK_CLASS       16
#define HEX_EXTRACT_CLASS     17

#define NUM_MEM_CLASSES    18

#if defined(INCLUDE_MEM_CLASS_NAME_ARRAY)
const char mem_class_name[NUM_MEM_CLASSES][20] = {
  "NOT USED",
  "BOOT CORE PSI",
  "BOOT CORE SLB",
  "BOOT CORE EBL",
  "CODE",
  "CUST",
  "DYN EEP",
  "STAT EEP",
  "SWAP BLOCK",
  "SEC BLOCK",
  "EXCEPTION LOG",
  "STATIC FFS",
  "DYNAMIC_FFS",
  "DSP SW",
  "ROOT DISK",
  "CUST DISK",
  "USER DISK",
  "HEX EXTRACT"
 };
#endif

// --------------------------
// Load Map Flags
//---------------------------
#define BOOT_CORE_HASH_MARKER   1
#define ALLOW_SEC_BOOT_UPDATE   2
#define DONT_TOUCH_REGION       4
#define SKIP_VERSION_CHECK      8
#define ERASE_TOTAL_LENGTH      16
#define ERASE_ONLY              32
#define COMPRESSED_IMAGE        64
#define COMPRESSED_TRANSFER     128
#define USE_DRIVER_FROM_FILE    256
#define NAND_SKIP_PT_CHECK      512  /* Written by download DLL in case of binary upload from NAND */

#define NUM_LOADMAP_FLAGS       4 // Only first 4 options are used for the time being

// --------------------------
// Element Header Structure
//---------------------------
typedef struct {
  ulong32 Type;
  ulong32 Size;
  ulong32 UID;
}ElementHeaderStructType;

// --------------------------
// Generic Header Structures
//---------------------------
typedef struct {
  ulong32 FileType; // prg,fls,eep,dffs ......
  ulong32 Marker; // magic number
  ulong32 Sha1Digest[5];
}GenericHeaderDataStructType;

typedef struct {
 ElementHeaderStructType Header;
 GenericHeaderDataStructType Data;
}__attribute__((packed)) GenericHeaderElementStructType ;

// --------------------------
// Safe boot codeword Structure
//---------------------------
typedef struct {
  ulong32 Enabled;
  ulong32 Addr;
  ulong32 Value1;
  ulong32 Value2;
}CodewordStructType;

typedef struct {
  ElementHeaderStructType Header;
  CodewordStructType Data;
}__attribute__((packed)) SafeBootCodewordElementStructType;


// --------------------------
// Tool Structures
//---------------------------
typedef struct {
  ulong32 Required;
  ulong32 Used;
}ToolVersionStructType;

typedef struct {
  ElementHeaderStructType Header;
  ToolVersionStructType Data;
}__attribute__((packed)) ToolVersionElementStructType;

// --------------------------
// End Marker Structure
//---------------------------
typedef struct {
  ElementHeaderStructType Header;
}EndElementStructType;

// --------------------------
// Version data Structure
//---------------------------
typedef struct {
  ulong32 DataLength;
  ubyte *Data;
  ulong32 DataOffset;
}VersionDataStructType;

typedef struct {
  ElementHeaderStructType Header;
  VersionDataStructType Data;
}__attribute__((packed)) VersionDataElementStructType;

// --------------------------
// Storage Structure
//---------------------------
typedef struct {
  ulong32 DataLength;
  ubyte *Data;
  ulong32 DataOffset;
}__attribute__((packed)) StorageDataStructType;

typedef struct {
  ElementHeaderStructType Header;
  VersionDataStructType Data;
}__attribute__((packed)) StorageDataElementStructType;

// --------------------------
// Injection data Structure
//---------------------------
typedef struct {
  ulong32 CRC_16;
  ulong32 CRC_8;
  ulong32 DataLength;
  ubyte *Data;
  ulong32 DataOffset;
}InjectionDataStructType;

typedef struct {
  ElementHeaderStructType Header;
  InjectionDataStructType Data;
}__attribute__((packed)) InjectionElementStructType;

// --------------------------
// Download data Structure
//---------------------------
typedef struct {
  ulong32 LoadMapIndex;
  ulong32 CompressionAlgorithm;
  ulong32 CompressedLength;
  ulong32 CRC;
  ulong32 DataLength;
  ubyte *Data;
  ulong32 DataOffset;
}DownloadDataStructType;

typedef struct {
  ElementHeaderStructType Header;
  DownloadDataStructType Data;
}__attribute__((packed)) DownloadDataElementStructType;

// --------------------------
// Indirect Download data Structure
//---------------------------
typedef struct {
  ulong32 LoadMapIndex;
  ulong32 CompressionAlgorithm;
  ulong32 CompressedLength;
  ulong32 CRC;
  ulong32 DataLength;
  ubyte *Data;
  ulong32 DataOffset;
}IndirectDownloadDataStructType;

typedef struct {
  ElementHeaderStructType Header;
  IndirectDownloadDataStructType Data;
}__attribute__((packed)) IndirectDownloadDataElementStructType;

// --------------------------
// Hardware Structure
//---------------------------
//SG1,SGL,SG2
typedef struct {
  ulong32 ChipSelectNumber;
  ulong32 Addrsel;
  ulong32 Buscon;
  ulong32 Busap;
}ChipSelectStructType;
//SG3
typedef struct {
  ulong32 ChipSelectNumber;
  ulong32 BUSRCON;
  ulong32 BUSWCON;
  ulong32 BUSRP;
  ulong32 BUSWP;
}SG3ChipSelectStructType;
// all E-gold's
typedef struct
{
  unsigned short AreaStartAddress;  // High word, no area is less than 64Kbyte anyway
  unsigned short A21_Setup;      // Setup A21 for EGold ver 2 eller free extra GPIO for Ver 3
  unsigned short A22_Setup;      // Setup A22 as Addr or Gpio constant H or L
  unsigned short A23_Setup;      // Setup A23 as Addr or Gpio constant H or L
  unsigned short Ext_Gpio_Setup;    // Setup extra Gpio constant H or L
  unsigned short CS_Number;      // CS number to use in this area
  unsigned short CS_AddrSel;      // ADDRSEL for CS
  unsigned short CS_BusCon;      // BUSCON for CS
} AddrSetupStruct;    // Size is 4 dword.

typedef struct {
  ulong32 Platform;
  ulong32 SystemSize;
  ulong32 BootSpeed;
  ulong32 HandshakeMode;
  ulong32 UsbCapable;
  ulong32 FlashTechnology;
  union {
    ChipSelectStructType CS[4];
    SG3ChipSelectStructType CS3[3];
    AddrSetupStruct CE[4];
  }ChipSelect;
  unsigned char ProjectName[20];
  unsigned char DepTemplate[32];
  unsigned char CfgTemplate[32];
}HardwareStructType;

typedef struct {
  ElementHeaderStructType Header;
  HardwareStructType Data;
}__attribute__((packed)) HardwareElementStructType;

// --------------------------
// NAND Partition Structure
//---------------------------
#if defined (NAND_PRESENT)
  #define ulong unsigned long
  #include <NAND_spare_area.h>  /* This is the root-definitions of the NAND partition table */
#else

#define MAX_PARTITIONS 28

typedef struct
{
  ulong32 ID;                         //ID of partition which is specified in this entry.
  ulong32 StartBlock;                 //Physical start address of region, in which the partition is located.
  ulong32 EndBlock;                   //Physical end address of region, in which the partition is located.
  ulong32 MaxBlocksInSubPartition;    //Number of blocks in partition.
  ulong32 StartReservoirBlock;        //Physical start address of the reservoir that the partition is using.
  ulong32 EndReservoirBlock;          //Physiacl end address of the reservoir that the partition is using.
  ulong32 ImageType;                  //Type specification of the partition
  ulong32 Options;                    //Specific options for this image
  ulong32 Reserved;                   //Reserved for future usage
}PartitionEntryStructType;

//NOTE: if MaxBlocksInSubPartition is 0x0, the Partition entry does not specify any partition.
typedef struct
{
  ulong32 PartitionMarker;/* Magic number to identify possible valid data*/
  PartitionEntryStructType Partition[MAX_PARTITIONS];
  ulong32 Reserved[3];
  //ulong32 PartitionCheckSum;
}PartitionStructType;
#endif
typedef struct
{
  ulong32 Table[MAX_PARTITIONS];
}LoadAddrToPartitionTableStruct;

typedef struct
{
  PartitionStructType Partition;
  LoadAddrToPartitionTableStruct LoadToPartition;
}__attribute__((packed)) NandPartitionDataStructType;

typedef struct {
  ElementHeaderStructType Header;
  NandPartitionDataStructType Data;
}__attribute__((packed)) NandPartitionElementStructType;


// --------------------------
// Secure Structure
//---------------------------
#define MAX_MODULUS_LEN (2048/8)

typedef struct {
  ulong32 StartAddr;
  ulong32 TotalLength;
  ulong32 UsedLength;
  ulong32 ImageFlags;
}RegionStructType;

typedef struct {
  ulong32 BootVersion;
  ulong32 InvBootVersion;
  ulong32 EblVersion;
  ulong32 InvEblVersion;
}ExtendedSecStructType;

typedef struct {
  ulong32 BootStartAddr;
  ulong32 BootLength;
  ulong32 BootSignature[5];
}OldStructType;

typedef union {
   OldStructType Old;
   ExtendedSecStructType Data;
}__attribute__((packed)) ExtendSecUnionType;

typedef struct {
  unsigned char SecureBlock[2][MAX_MODULUS_LEN];
  ulong32 Type;
  // NOTE: this change has impact on EBL SW version handling!!!!
  unsigned char DataBlock[2048-(2*MAX_MODULUS_LEN)-8-128-20-sizeof(PartitionStructType)-MAX_PARTITIONS*4-9*4];
  //***********************
  ulong32 BootCoreVersion;
  ulong32 EblVersion;
  ExtendSecUnionType SecExtend;
  //***********************
  ulong32 FileHash[5];
  PartitionStructType Partition;
  ulong32 LoadAddrToPartition[MAX_PARTITIONS];
  ulong32 LoadMagic;
  RegionStructType LoadMap[8];
}SecurityStructType;

typedef struct {
  ElementHeaderStructType Header;
  SecurityStructType Data;
}__attribute__((packed)) SecurityElementStructType;
// --------------------------
// Memory map Structure
//---------------------------
#define MAX_MAP_ENTRIES 50

typedef struct {
  ulong32 Start;
  ulong32 Length;
  ulong32 Class;
  ulong32 Flags;
  ubyte FormatSpecify[16];
}MemoryMapEntryStructType;

typedef struct {
  MemoryMapEntryStructType Entry[MAX_MAP_ENTRIES];
  ulong32 EepRevAddrInSw;
  ulong32 EepRevAddrInEep;
  ulong32 EepStartModeAddr;
  ulong32 NormalModeValue;
  ulong32 TestModeValue;
  ulong32 SwVersionStringLocation;
  ulong32 FlashStartAddr;
}MemoryMapStructType;

typedef struct {
  ElementHeaderStructType Header;
  MemoryMapStructType Data;
}__attribute__((packed)) MemoryMapElementStructType;

// --------------------------
// TOC Structure
//---------------------------
typedef struct {
  ulong32 UID;
  ulong32 MemoryClass;
  ulong32 Reserved[2];
  ubyte   FileName[MAX_FILENAME_LENGTH];
}TocEntryStructType;  

typedef struct {
  ulong32 NoOfEntries;
  TocEntryStructType* Data;
  ulong32 DataOffset;
} TocStructType;

typedef struct {
  ElementHeaderStructType Header;
  TocStructType Data;
} __attribute__((packed)) TocElementStructType;

#endif
