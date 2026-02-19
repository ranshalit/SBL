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
 * $Header:   S:/CG/archives/CGTOOLS/INC/CGMPFA.H-arc   1.9   Sep 06 2016 15:57:52   congatec  $
 *
 * Contents: MPFA definitions
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/INC/CGMPFA.H-arc  $
 * 
 *    Rev 1.9   Sep 06 2016 15:57:52   congatec
 * Added BSD header.
 * MOD004: Added support for new MPFA module types.
 * 
 *    Rev 1.8   Aug 06 2015 13:28:04   gartner
 * MOD003: Add new PAD module type.
 * 
 *    Rev 1.7   Nov 21 2014 16:35:32   gartner
 * MOD002: Added support for new MPFA modules: 
 *  POST Logo Left (standard small POST logo in uper left corner)
 *  POST Logo Right (addon small POST logo in upper right corner)
 *  UEFI FW Volume (UEFI Firmware Volume with UEFI DXE drivers and or UEFI bootloaders)
 *  OA 3.0 Module (OEM activation 3.0 module)   
 * 
 *    Rev 1.6   May 31 2012 15:45:46   gartner
 * Updated variable definition to ease 64bit porting.
 * 
 *    Rev 1.5   Dec 06 2010 12:07:30   gartner
 * MOD001: Add definitions for new module types and new extended flash region.
 * 
 *    Rev 1.4   Jul 16 2010 13:52:58   gartner
 * Added support for OEM SLIC modules.
 * 
 *    Rev 1.3   Apr 16 2009 12:56:04   gartner
 * Added support for OEM HDA verb table modules.
 * 
 *    Rev 1.2   Jul 02 2007 13:05:24   gartner
 * Add OEM video mode module type definition.
 * 
 *    Rev 1.1   Oct 30 2006 15:22:02   gartner
 * Added OEM setup, OEM string and fixed CMOS module types.
 * 
 *    Rev 1.0   Sep 30 2005 16:29:08   gartner
 * Initial revision.
 *
 *---------------------------------------------------------------------------
 */

#ifndef _INC_CGMPFA

#pragma pack (1)

//+---------------------------------------------------------------------------
//       BIOS MPFA information structure
//+---------------------------------------------------------------------------

#define CG_MPFA_INFO_ID_L       0x49504D24      // $MPINFO$
#define CG_MPFA_INFO_ID_H       0x244F464E           
#define CG_MPFA_INFO_REV        2               // revision of MPFA info structure	//MOD001
#define CG_MPFA_REVISION        1               // Revision of MPFA structure
#define CG_MPFA_ID_IGNORE       0xFFFF          // Don't care module ID.        

typedef struct
{
    UINT32			infoIDLow;      // MPFA info low ID
    UINT32			infoIDHigh;     // MPFA info high ID
    unsigned char   infoRev;        // MPFA info structure revision
    unsigned char   mpfaRev;        // MPFA structure revision (defines MPFA block layout)
    UINT16			mpfaBlkSize;    // MPFA flash block size in KB   
    unsigned char   mpfaBSize;      // BIOS MPFA section size in no. of MPFA blocks
    UINT32			mpfaBStart;     // BIOS MPFA section physical start address
    UINT32			mpfaBEnd;       // BIOS MPFA section physical end address
    unsigned char   mpfaMSize;      // Module MPFA section size in no. of MPFA blocks
    UINT32			mpfaMStart;     // Module MPFA section physical start address
    UINT32			mpfaMEnd;       // Module MPFA section physical end address
    unsigned char   mpfaUSize;      // User MPFA section size in no. of MPFA blocks
    UINT32			mpfaUStart;     // User MPFA section physical start address
    UINT32			mpfaUEnd;       // User MPFA section physical end address
    UINT32			flashSize;      // BIOS ROM flash size in bytes.                //MOD001
	UINT32			extFlashSize;   // Total flash part size in bytes.              //MOD001
} CG_MPFA_INFO ;


//+---------------------------------------------------------------------------
//       BIOS MPFA types or sections
//+---------------------------------------------------------------------------
#define CG_MPFA_STATIC  0x00            // Area not modified during POST.
                                        // Main area for feature modules added
                                        // at build time or using congatec MPFA
                                        //patch utility.
#define CG_MPFA_USER    0x01            // User area. No MPFA structures required.
                                        // Not checked or touched by the BIOS 
                                        // at all.
#define CG_MPFA_DYNAMIC 0x02            // Area dynamically modified during POST.
                                        // (E.g. used for CMOS backup)

#define CG_MPFA_ALL     0x03            // BIOS ROM part (comprises _STATIC, _USER, _DYNAMIC).	//MOD001 v

#define CG_MPFA_EXTD	0x04            // Whole flash part (e.g. complete SPI 
										// contents including descriptor, ME, BIOS,
										// ethernet configuration,...).
										// For non-descriptor BIOS identical 
										// to CG_MPFA_ALL
																								//MOD001 ^

//+---------------------------------------------------------------------------
//       MPFA module header
//+---------------------------------------------------------------------------
#define CG_MPFA_MOD_HDR_ID 0x5244484D   //MHDR

typedef struct
{
// CG_MPFA_MOD_CHAIN                                                
    UINT32			hdrID;          // MPFA module header ID
    UINT32			modSize;        // total module size in bytes (header + data + end)
    unsigned char   modType;        // module type        
    unsigned char   modSubType;     // module subtype
    UINT16			modFlags;       // module flags
//CG_MPFA_MOD_PARAM
    UINT32			modParamFlags;  // module parameter flags
    unsigned char   modRev;         // module revision
    unsigned char   modLoadTime;    // module load time
    unsigned char   modExecTime;    // module execution time
    unsigned char   modParam0;      // module parameter
    UINT32			modLoadAddr;    // physical module load address
    UINT32			modEntryOff;    // entry offset within module
    UINT32			modParam1;      // module parameter (e.g. NVRAM control value)
    UINT32			modParam2;      // module parameter
    UINT16			modID;          // module defined ID
    UINT16			modChkSum;      // module data checksum
} CG_MPFA_MODULE_HEADER ;


//+---------------------------------------------------------------------------
//       MPFA module end structure
//+---------------------------------------------------------------------------
#define CG_MPFA_MOD_END_ID      0x444E454D      //MEND

typedef struct
{                                                
    UINT32       endID;                  // MPFA module end struct. ID
} CG_MPFA_MODULE_END ;


//+---------------------------------------------------------------------------
//       MPFA module types
//+---------------------------------------------------------------------------
#define CG_MPFA_TYPE_IGNORE             0xFF    // Don't care.
#define CG_MPFA_TYPE_ROOT               0x00    // Root module
#define CG_MPFA_TYPE_CMOS_BACKUP        0x01    // CMOS backup map
#define CG_MPFA_TYPE_CMOS_DEFAULT       0x02    // CMOS default map
#define CG_MPFA_TYPE_UC                 0x03    // Usercode
#define CG_MPFA_TYPE_BOOT               0x04    // boot loader
#define CG_MPFA_TYPE_PDA                0x05    // panel data area
#define CG_MPFA_TYPE_LOGO               0x06    // BIOS LOGO data
#define CG_MPFA_TYPE_IP_CONF            0x07    // IP configuration block
#define CG_MPFA_TYPE_SETUP              0x08    // SETUPDB override             //MOD001
#define CG_MPFA_TYPE_CMOS_FIXED         0x09    // Fixed CMOS settings map      //MOD001
#define CG_MPFA_TYPE_STRING             0x0A    // OEM string table             //MOD001
#define CG_MPFA_TYPE_OEMVMODE           0x0B    // OEM video mode                
#define CG_MPFA_TYPE_OEMHDAVERB         0x0C    // OEM HDA verb table            
#define CG_MPFA_TYPE_OEMSLIC			0x0D    // OEM SLIC table			
#define CG_MPFA_TYPE_OEMSLP			    0x0E    // OEM SLP 1.0 module					//MOD001
#define CG_MPFA_TYPE_VBIOS_STD		    0x0F    // Standard/fixed VIDEO BIOS module		//MOD001
#define CG_MPFA_TYPE_VBIOS_OEM		    0x10    // OEM (exchangeable) VIDEO BIOS module	//MOD001
#define CG_MPFA_TYPE_SMALL_LOGO         0x11    // Small boot logo data (top-left)      //MOD002
#define CG_MPFA_TYPE_OEM_LOGO           0x12    // OEM boot logo data (top-right)       //MOD002
#define CG_MPFA_TYPE_FIRMWARE_VOLUME    0x13    // UEFI volume with user files          //MOD002
#define CG_MPFA_TYPE_OA30				0x14    // OEM activation 3.0 module            //MOD002
#define CG_MPFA_TYPE_SETUP_MENU_SETTINGS 0x15	// OEM BIOS setup control module		//MOD004
#define CG_MPFA_TYPE_OEM_SMBIOS_DATA	0x16    // OEM SMBIOS data						//MOD004 

#define CG_MPFA_TYPE_PAD			    0xE0    // dummy pad module						//MOD003

//+---------------------------------------------------------------------------
//       MPFA module sub-types
//+---------------------------------------------------------------------------
#define CG_MPFA_STYPE_IGNORE            0xFF    // Don't care.

// TYPE_UC sub types
#define CG_MPFA_STYPE_OPROM             0x01    // 55AA OPROM format
#define CG_MPFA_STYPE_BIN               0x02    // free binary format
#define CG_MPFA_STYPE_COM               0x03    // COM file (TINY model)
#define CG_MPFA_STYPE_EXE               0x04    // EXE file

//+---------------------------------------------------------------------------
//       MPFA module flags
//+---------------------------------------------------------------------------
#define CG_MOD_ENTRY_USED       0x8000          // module entry used
#define CG_MOD_ENTRY_MODIFIED   0x4000          // If this bit is clear, the
                                                // module has been modified.
#define CG_MOD_EXTENDED         0x0001          // If bit is CLEAR first extended module
                                                // header is available.
//+---------------------------------------------------------------------------
//       MPFA module parameter flags
//+---------------------------------------------------------------------------
#define CG_MOD_COMPRESSED       0x00000001      // If this bit is CLEAR, the module is compressed.
                                                
#define CG_MOD_MPNV_CONT        0x00000002      // If this bit is CLEAR, the module load/execution/
                                                // access is controlled by a MPNV value.
                                                // modParam1 specifies the MPNV area and index, 
                                                // modParam0 specifies the the value to write to 
                                                // this storage to prevent module access/load/execution. 

//+---------------------------------------------------------------------------
//       load/execution time codes / POST progress codes
//+---------------------------------------------------------------------------
#define CG_MPFA_POST_NO_LOAD                    0
#define CG_MPFA_POST_NO_EXECUTION               0
#define CG_MPFA_POST_AFTER_MEMORY_INIT          1
#define CG_MPFA_POST_BEFORE_CMOS_TEST           2
#define CG_MPFA_POST_BEFORE_VIDEO_INIT          3
#define CG_MPFA_POST_AFTER_VIDEO_INIT           4
#define CG_MPFA_POST_BEFORE_OPROM_SCAN          5
#define CG_MPFA_POST_AFTER_OPROM_SCAN           6
#define CG_MPFA_POST_BEFORE_SETUP               7
#define CG_MPFA_POST_SETUP_EXTENSION            8
#define CG_MPFA_POST_BEFORE_BOOT                9

//+---------------------------------------------------------------------------
//       Load address indicators
//+---------------------------------------------------------------------------
#define CG_MPFA_LOAD_ANY_UMB    0x00000000		// Load at any address in UMB area.
#define CG_MPFA_LOAD_ANY_CONV   0xFFFFFFFE      // Load at any address in RAM <640k.
#define CG_MPFA_LOAD_ANY_EXT    0xFFFFFFFF      // Load at any address in RAM >1MB.

#pragma pack()

#define _INC_CGMPFA
#endif 

