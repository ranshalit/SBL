/*---------------------------------------------------------------------------
 *
 * Copyright (c) 2021, congatec GmbH. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the BSD 2-clause license which 
 * accompanies this distribution. 
 *
 * This program is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. 
 * See the BSD 2-clause license for more details.
 *
 * The full text of the license may be found at:        
 * http://opensource.org/licenses/BSD-2-Clause   
 *
 *---------------------------------------------------------------------------
 */

/*---------------------------------------------------------------------------
 *
 * $Header:   S:/CG/archives/CGTOOLS/CGUTIL/CGUTLCMN/cgbmod.h-arc   1.7   Sep 08 2016 09:53:04   congatec  $
 *
 * Contents: Common MPFA configuration implementation module definitions.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/CGUTIL/CGUTLCMN/cgbmod.h-arc  $
 * 
 *    Rev 1.7   Sep 08 2016 09:53:04   congatec
 * MOD003: Adapted for correct Linux build.
 * 
 *    Rev 1.6   Sep 07 2016 12:28:28   congatec
 * MOD002: Added definitions for FV alignment. Make sure the UID is only redefined for non-UEFI builds.
 * 
 *    Rev 1.5   Sep 06 2016 15:49:44   congatec
 * Added BSD header.
 * MOD001: Added support definitions for SMBIOS MPFA module, Menu Layout MPFA module and FV MPFA module. 
 * 
 *    Rev 1.4   May 31 2012 15:45:34   gartner
 * Updated variable definition to ease 64bit porting.
 * 
 *    Rev 1.3   Oct 30 2006 15:11:46   gartner
 * Modified and added new function prototypes.
 * 
 *    Rev 1.2   Mar 21 2006 14:40:50   gartner
 * Added further error code and module compare function prototype.
 * 
 *    Rev 1.1   Jan 27 2006 12:35:30   gartner
 * Redefined some return codes. Adapted function prototypes for new parameters. Added functions to get/set the OEM and system BIOS version.
 * 
 *    Rev 1.0   Oct 04 2005 13:14:14   gartner
 * Initial revision.
 * 
 *    Rev 1.0   Sep 30 2005 16:31:18   gartner
 * Initial revision.
 *
 *---------------------------------------------------------------------------
 */

#ifndef _INC_CGBMOD

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#endif 

#include "cgmpfa.h"

//+---------------------------------------------------------------------------	//MOD001 v
//       SMBIOS DMS input data file description / sample
//+---------------------------------------------------------------------------
/*
[BIOS]
Vendor = BiosVendor
Version = BiosVersion
ReleaseDate = 01/01/2016

[System]
Manufacturer = SystemManufacturer
Product = SystemProduct
Version = SystemVersion
SerialNum = SystemSerialNum
SKU = SystemSKU
Family = SystemFamily
UUID = 5cbf7824d4b049ed9f2e1981d8cded04

[BaseBoard]
Manufacturer = BaseBoardManufacturer
Product = BaseBoardProduct
Version = BaseBoardVersion
SerialNum = BaseBoardSerialNum
TagNum = BaseBoardAssetTag

[Chassis]
Manufacturer = ChassisManufacturer
Version = ChassisVersion
SerialNum = ChassisSerialNum
TagNum = ChassisAssetTag
ChassisType = 83
ChassisOEM = 00C0DE00

[OemString]
String = congatec AG
String = http://www.congatec.de
String = We simplify the use of embedded computing

[SysConfigOptions]
String = There are no jumpers
String = or any other system controls
String = on this module
*/

//+---------------------------------------------------------------------------	
//       Setup menu layout file (MLF) header definition
//+---------------------------------------------------------------------------
typedef struct 
{
		UINT32			Signature;          // CGML signature
		UINT32			TotalSize;          // Size of file, including this header
		UINT32			OriginalSize;       // Size of file, without overrides
		unsigned char	BiosVersion[8];     // BIOS version tag, i.e TR33R000
} CG_SETUP_MENU_FILE;															

#define CGML_SIGNATURE	0x4C4D4743U		// CGML	

//+---------------------------------------------------------------------------	
//       UEFI Firmware Volume support definitions 
//+---------------------------------------------------------------------------

// NOTE: Correct UEFI FV alignment in BIOS is essential as otherwise the FV 
//       would not be recognized by the BIOS or in worst case might corrupt
//       the BIOS and prevent booting.

#define FIRMWARE_VOLUME_MIN_ALIGN	0x00000040	// Supported minimal FV volume alignment		
#define FIRMWARE_VOLUME_MAX_ALIGN	0x00008000	// Supported maximal FV volume alignment	MOD002		

///
/// 128 bit buffer containing a unique identifier value.
/// Unless otherwise specified, aligned on a 64 bit boundary.
///
#ifdef LINUX																				//MOD002
typedef unsigned char UINT8;																//MOD003
typedef struct {
  UINT32  Data1;
  UINT16  Data2;
  UINT16  Data3;
  UINT8   Data4[8];
} EFI_GUID;
#endif																						//MOD002 v
#ifdef WIN32
typedef struct {
  UINT32  Data1;
  UINT16  Data2;
  UINT16  Data3;
  UINT8   Data4[8];
} EFI_GUID;
#endif																						//MOD002 ^

typedef struct _EFI_FIRMWARE_VOLUME_HEADER {
    UINT8                  ZeroVector[16];
    EFI_GUID               FileSystemGuid;
    UINT32                 FvLength1;														//MOD003
	UINT32                 FvLength2;														//MOD003	
    UINT32                 Signature;
    UINT32                 Attributes;
    UINT16                 HeaderLength;
    UINT16                 Checksum;
    UINT16                 ExtHeaderOffset;  //Reserved in Revision 1
    UINT8                  Reserved;
    UINT8                  Revision;
    //EFI_FV_BLOCK_MAP_ENTRY FvBlockMap[1];
} EFI_FIRMWARE_VOLUME_HEADER;

// FV alignment calculation:
// 
// UINT32 alignment = (UINT32)pow(2.0, (INT32)(volumeHeader->Attributes & 0x001F0000) >> 16); 
																				//MOD001 ^

#define EFI_FVB2_ALIGNMENT_1        0x00000000									//MOD002 v
#define EFI_FVB2_ALIGNMENT_2        0x00010000
#define EFI_FVB2_ALIGNMENT_4        0x00020000
#define EFI_FVB2_ALIGNMENT_8        0x00030000
#define EFI_FVB2_ALIGNMENT_16       0x00040000
#define EFI_FVB2_ALIGNMENT_32       0x00050000
#define EFI_FVB2_ALIGNMENT_64       0x00060000
#define EFI_FVB2_ALIGNMENT_128      0x00070000
#define EFI_FVB2_ALIGNMENT_256      0x00080000
#define EFI_FVB2_ALIGNMENT_512      0x00090000
#define EFI_FVB2_ALIGNMENT_1K       0x000A0000
#define EFI_FVB2_ALIGNMENT_2K       0x000B0000
#define EFI_FVB2_ALIGNMENT_4K       0x000C0000
#define EFI_FVB2_ALIGNMENT_8K       0x000D0000
#define EFI_FVB2_ALIGNMENT_16K      0x000E0000
#define EFI_FVB2_ALIGNMENT_32K      0x000F0000
#define EFI_FVB2_ALIGNMENT_64K      0x00100000									//MOD002 ^


//+---------------------------------------------------------------------------
//       MPFA sub-type descriptor structure
//+---------------------------------------------------------------------------
typedef struct
{
        unsigned char subType;
        _TCHAR subTypeDescription[80];
        _TCHAR subTypeShortQualifier[40];
} CG_MPFA_STYPE;

//+---------------------------------------------------------------------------
//       MPFA type descriptor structure
//+---------------------------------------------------------------------------
typedef struct
{
        unsigned char type;
        _TCHAR typeDescription[80];
        _TCHAR typeShortQualifier[40];
        UINT32 sectionType;
        UINT32 paramLockFlags;
        CG_MPFA_STYPE* subTypeLstPtr;
} CG_MPFA_TYPE;

//+---------------------------------------------------------------------------
//       MPFA POST hook descriptor structure
//+---------------------------------------------------------------------------
typedef struct
{
        unsigned char hook;
        _TCHAR hookDescription[80];
        _TCHAR hookShortQualifier[40];
} CG_MPFA_POST_HOOK;

//+---------------------------------------------------------------------------
//       MPFA module section info structure
//+---------------------------------------------------------------------------
typedef struct
{
        UINT32 sectionType;
        UINT32 sectionSize;
        UINT32 sectionBlockSize;
        UINT32 physAccess;
        unsigned char *pSectionBuffer;
        UINT32 addIndex;
} CG_MPFA_SECTION_INFO;


//+---------------------------------------------------------------------------
//       MPFA module return codes
//+---------------------------------------------------------------------------
#define CG_MPFARET_OK           0x00    // Success
#define CG_MPFARET_ERROR        0x01    // General error
#define CG_MPFARET_INTRF_ERROR  0x02    // Interface access/init. error
#define CG_MPFARET_INCOMP       0x03    // Module incompatibility error
#define CG_MPFARET_NOTFOUND     0x04    // Module not found error
#define CG_MPFARET_INV          0x05    // Invalid MPFA module (frame or data)
#define CG_MPFARET_ERROR_SIZE   0x06    // Section size exceeded
#define CG_MPFARET_ERROR_DUP    0X07    // Duplicate module exists error
#define CG_MPFARET_ERROR_FILE   0x08    // File processing error
#define CG_MPFARET_NOTALLOWED   0x09    // Operation not allowed
#define CG_MPFARET_INV_DATA     0x0A    // Invalid MPFA module data
#define CG_MPFARET_INV_PARM     0x0B    // Invalid MPFA module parameters
#define CG_MPFARET_NOMATCH      0x0C    // Module data does not match

//+---------------------------------------------------------------------------
//       MPFA module compare flags / parameter selection flags
//+---------------------------------------------------------------------------
#define CG_MPFACMP_SIZE         0x00000001
#define CG_MPFACMP_TYPE         0x00000002
#define CG_MPFACMP_SUBTYPE      0x00000004
#define CG_MPFACMP_FLAGS        0x00000008
#define CG_MPFACMP_PARAMFLAGS   0x00000010
#define CG_MPFACMP_REV          0x00000020
#define CG_MPFACMP_LOADTIME     0x00000040
#define CG_MPFACMP_EXECTIME     0x00000080
#define CG_MPFACMP_PARAM0       0x00000100
#define CG_MPFACMP_LOADADDR     0x00000200
#define CG_MPFACMP_ENTRYOFF     0x00000400
#define CG_MPFACMP_PARAM1       0x00000800
#define CG_MPFACMP_PARAM2       0x00001000
#define CG_MPFACMP_ID           0x00002000
#define CG_MPFACMP_ALL          0xFFFFFFFF
    
//+---------------------------------------------------------------------------
//       MPFA save module flags
//+---------------------------------------------------------------------------
#define CG_MPFASFL_MOD          0x00000000  //Save whole module
#define CG_MPFASFL_DATA         0x00000001  //Save module data only

//+---------------------------------------------------------------------------
//      Interface functions
//+---------------------------------------------------------------------------
extern UINT16 CgMpfaStart(UINT16 bIncMPFA_ALL);
extern UINT16 CgMpfaEnd(void);
extern UINT16 CgMpfaCreateSectionInfo(void);
extern UINT16 CgMpfaBufferInit(UINT16 bIncMPFA_ALL);
extern UINT16 CgMpfaBufferCleanup(void);
extern UINT16 CgMpfaApplyChanges(UINT16 bRestart);
extern UINT16 CgMpfaAddModule(_TCHAR *pInputFilename,
                                      UINT16 nAccessLevel,
                                      UINT16 bSkipDataCheck);
extern UINT16 CgMpfaCreateModule(CG_MPFA_MODULE_HEADER *pMpfaHeader, 
                                          _TCHAR *pInputFilename, 
                                          _TCHAR *pOutputFilename,
                                          UINT16 nAccessLevel,
                                          UINT16 bSkipDataCheck);
extern UINT16 CgMpfaDelModule(CG_MPFA_MODULE_HEADER *pMpfaHeader,
                                      UINT32 nSearchFlags,
                                      UINT16 nAccessLevel);
extern UINT16 CgMpfaSaveModule(CG_MPFA_MODULE_HEADER *pMpfaHeader,
                                       _TCHAR *pOutputFilename,
                                       UINT32 nSearchFlags, 
                                       UINT32 nSaveFlags );
extern UINT16 CgMpfaRebuildSection(CG_MPFA_SECTION_INFO *pSectionInfo);
UINT16 CgMpfaFindModule(CG_MPFA_SECTION_INFO *pSectionInfo, 
                                CG_MPFA_MODULE_HEADER *pMpfaHeader,
                                UINT32 nStartIndex,
                                UINT32 *pFoundIndex,
                                UINT32 nSearchFlags);
extern UINT16 CgMpfaGetSysBiosVersion( _TCHAR* lpszBiosVersion, unsigned char *pBiosType); 
extern UINT16 CgMpfaSetSysBiosVersion( _TCHAR* lpszBiosVersion); 
extern UINT16 CgMpfaGetOEMBiosVersion( _TCHAR* lpszBiosVersion); 
extern UINT16 CgMpfaSetOEMBiosVersion( _TCHAR* lpszBiosVersion); 
extern UINT16 CgMpfaCmpModule(_TCHAR *pInputFilename,
                                      CG_MPFA_MODULE_HEADER *pMpfaHeader,
                                      UINT32 nSearchFlags,
                                      UINT16 nAccessLevel);
extern UINT16 CgMpfaCreateStringTableFile( _TCHAR* lpszStringTableFile, UINT16 usStrModId); 
extern UINT16 CgMpfaCreateSetupTableFile( _TCHAR* lpszSetupTableFile, UINT16 usSetModId); 
extern UINT16 CgMpfaCheckBUPActive(void); 
extern UINT16 CgMpfaSetBUPInactive( _TCHAR* lpszPassword); 

//+---------------------------------------------------------------------------
//      Global data
//+---------------------------------------------------------------------------
extern CG_MPFA_TYPE g_MpfaTypeList[];
extern CG_MPFA_POST_HOOK g_MpfaPostHookList[];
extern CG_MPFA_SECTION_INFO* g_MpfaSectionList[];
extern CG_MPFA_SECTION_INFO CgMpfaStaticInfo;
extern CG_MPFA_SECTION_INFO CgMpfaUserInfo;
extern CG_MPFA_SECTION_INFO CgMpfaDynamicInfo;
extern CG_MPFA_SECTION_INFO CgMpfaAllInfo;

extern UINT32 g_nNoMpfaTypes;
extern UINT32 g_nNoPostHooks;
extern UINT32 g_nNoMpfaSections;


#pragma pack()

#ifdef __cplusplus
}
#endif 

#define _INC_CGBMOD
#endif 

