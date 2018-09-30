#ifndef __TOOL_VERSIONS_H__
#define __TOOL_VERSIONS_H__

/* Tool Versions for all the tools
   1st Nibble : Major version
   2nd Nibble : Minor Version
   Eg: 0x00000001 means version 0.1
*/
#define PRGHANDLER_TOOL_REQ_VER    0x00020002
#define PRGSEQUENCER_TOOL_REQ_VER  0x00020003
#define MAKEPRG_TOOL_REQ_VER       0x00020003
#define HEXTOFLS_TOOL_REQ_VER      0x00020000
#define FLSSIGN_TOOL_REQ_VER       0x00020000
#define DWDTOHEX_TOOL_REQ_VER      0x00020000
#define FLASHTOOL_TOOL_REQ_VER     0x00040000
#define STORAGETOOL_TOOL_REQ_VER   0x00010000 // Removed from Version Check
#define FLSTOHEADER_TOOL_REQ_VER   0x00020000
#define BOOT_CORE_TOOL_REQ_VER     0x00020000
#define EBL_TOOL_REQ_VER           0x00020000
#define FLSTONAND_TOOL_REQ_VER     0x00020000

#endif
 

