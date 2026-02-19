/*---------------------------------------------------------------------------
 *
 * Copyright (c) 2023, congatec GmbH. All rights reserved.
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
 * $Header:   S:/CG/archives/CGTOOLS/CGUTIL/CGUTLCMN/BIOSFLSH.H-arc   1.7   Sep 06 2016 15:44:48   congatec  $
 *
 * Contents: Congatec BIOS flash common implemetation module definitions.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/CGUTIL/CGUTLCMN/BIOSFLSH.H-arc  $
 * 
 *    Rev 1.7   Sep 06 2016 15:44:48   congatec
 * Added BSD header.
 * MOD006: Added flag to control immediate/auto off-on cycle for BIOS unlock. Mark old BB update control flag as obsolete. 
 * 
 *    Rev 1.6   Aug 06 2015 13:26:38   gartner
 * MOD005: UPdated for new MAC address recovery mechanism.
 * 
 *    Rev 1.5   Aug 08 2014 13:47:42   gartner
 * MOD004: Added flags for separate extended lock handling.
 * 
 *    Rev 1.4   May 31 2012 15:45:34   gartner
 * Updated variable definition to ease 64bit porting.
 * 
 *    Rev 1.3   Jun 22 2011 13:47:16   gartner
 * MOD003: Added new flash for flash update control.
 * 
 *    Rev 1.2   Dec 06 2010 11:59:40   gartner
 * MOD002: Added support for extended/full flash update handling.
 * 
 *    Rev 1.1   Dec 19 2005 13:26:42   gartner
 * MOD001: Added BIOS update prepare routine.
 * 
 *    Rev 1.0   Oct 04 2005 13:14:14   gartner
 * Initial revision.
 * 
 *    Rev 1.0   Sep 30 2005 16:31:18   gartner
 * Initial revision.
 *
 *---------------------------------------------------------------------------
 */

#ifndef _INC_BIOSFLSH

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#endif 

// Make sure we use right packing rules											//MOD005
#pragma pack(push,1)

//----------------------------
// BIOS flash parameter flags
//----------------------------
#define CG_BFFLAG_DELCMOS       0x01
#define CG_BFFLAG_UPDBB         0x02	// MOD006: Obsolete. Do not use anymore !
#define CG_BFFLAG_FORCE         0x04
#define CG_BFFLAG_SAVE          0x08
#define CG_BFFLAG_ASK           0x10
#define CG_BFFLAG_EXTD			0x20	// Flash extended (e.g. full SPI flash update)
#define CG_BFFLAG_FEXTD			0x40	// Force extended flash update although not required due to matching compatibility IDs
#define CG_BFFLAG_MANUF			0x80	// Indicate manufacturing mode. // MOD003
#define CG_BFFLAG_ISLOCKED      0x0100	// Check whether (extended) BIOS flash area is locked	//MOD004
#define CG_BFFLAG_UNLOCK		0x0200	// Unlock (extended) BIOS flash area					//MOD004
#define CG_BFFLAG_LOCK			0x0400	// Lock (extended) BIOS flash area						//MOD004
#define CG_BFFLAG_AUTO_OFFON	0x0800	// Perform immediate/auto off-on cycle for BIOS unlock.	//MOD006
#define CG_BFFLAG_PRESERVE		0x1000	// Preserve pre-defined NVRAM settings (e.g. PASSWORD)	//MOD007
#define CG_BFFLAG_KEEP_LANAREAS 0x2000  // Save LAN CTRL 0 and LAN CTRL 1 areas (DSAC)          //MOD008

//-------------------------
// BIOS flash return codes
//-------------------------
#define CG_BFRET_OK           0x00    // Success
#define CG_BFRET_ERROR        0x01    // General error
#define CG_BFRET_INTRF_ERROR  0x02    // Interface access/init. error
#define CG_BFRET_INCOMP       0x03    // BIOS incompatibility error
#define CG_BFRET_INVALID      0x04    // Invalid BIOS file
#define CG_BFRET_ERROR_SIZE   0x05    // Size mismatch
#define CG_BFRET_ERROR_FILE   0x06    // Error processing file
#define CG_BFRET_ERROR_NOEXTD 0x07    // Error no extended flash update possible					//MOD002
#define CG_BFRET_INCOMP_EXTD  0x08    // BIOS extended information incompatibility					//MOD002
#define	CG_BFRET_ERROR_UNLOCK_EXTD	0x09	// Failed to unlock flash for extended update			//MOD002
#define	CG_BFRET_ERROR_LOCK_EXTD	0x0A	// Failed to lock flash after extended update			//MOD002
#define CG_BFRET_NOTCOMP_EXTD 0x0B		// Extend update not (yet) completed						//MOD002


//---------------------
// Function prototypes
//---------------------
extern UINT16 CG_BiosFlash( _TCHAR* lpszBiosFile, UINT32 nFlags);
extern UINT16 CG_BiosSave( _TCHAR* lpszBiosFile);
extern UINT16 CgBfGetBiosInfoBoard(_TCHAR *lpszBiosVersion);
extern UINT16 CgBfGetBiosInfoRomfile(FILE *fpBiosRomfile, _TCHAR *lpszBiosVersion);
extern UINT16 CgBfGetFlashSize(UINT32 *pFlashSize, UINT32 *pExtdFlashSize, UINT32 *pFlashBlockSize);	//MOD002
extern UINT16 CgBfGetFileSize(FILE *fpFile, UINT32 *pFileSize);
extern UINT16 CG_BiosFlashPrepare(void);
extern UINT16 CgBfCheckExtendedCompatibility(FILE *fpBiosRomfile, UINT32 nRomfileSize);	//MOD002
extern UINT16 CgBfGetBiosInfoFlash(void);										//MOD002 


																				//MOD005 v
//----------------------------------
// SPI flash descriptor definitions
//----------------------------------

// Flash descriptor signature
#define FLASH_DESCRIPTOR_SIGNATURE 0x0FF0A55A
#define FLMAP0_OFFSET	0x14
#define FLREG0_OFFSET   0x00 // MOD008
#define FLREG3_OFFSET	0x0C
                             // MOD008 v
#define FLREG11_OFFSET  0x2C
#define FLREG12_OFFSET  0x30
                              // MOD008 ^

// MOD010 v

//---------------------------------------------------
// Icelake definitions (MAC address / Lan recovery
//---------------------------------------------------

#define PFA_POINTER 0x40
#define  MACADDRESSES 8


//MOD010 ^

// MOD008 v 

//---------------------------------------------------
// DSAC definitions (MAC address / LAN area recovery)
//---------------------------------------------------

#define MAC_0_2_OFFSET 0x202
#define MAC_1_3_OFFSET 0x212
#define X550_CHECKSUM_OFFSET 0x3F
#define X550_SIZE 0x41
#define X550_PCIE_ANALOG_POINTER 0x02
#define X550_PHY_POINTER 0x04
#define X550_OPTION_ROM_POINTER 0x05
#define X550_PCIE_GENERAL_POINTER 0x06
#define X550_PCIE_GENERAL_SIZE 0x24
#define X550_PCIE_CONFIG0_POINTER 0x07
#define X550_PCIE_CONFIG1_POINTER 0x08
#define X550_PCIE_CONFIG_SIZE 0x08
#define X550_FW_POINTER 0x0F

// MOD008 ^

//MOD009 v

/*
 * EHL GbE Region Layout:
 * 
 * GUID: 16 Bytes
 * FFS Header: 12 Bytes
 * Version: 4 Bytes
 * Number of Ports: 4 Bytes
 * 
 * For each MAC address (see number of ports):
 * BDF: 4 Bytes
 * MAC Address Low: 4 Bytes
 * MAC Address High: 4 Bytes
 * 
 */

#define EHL_GBE_REGION_MAXNUM_PORTS 3 // there should not be more than 3 mac address entries, otherwise we have a problem 

#define EHL_GBE_REGION_VERSION_SIZE 0x04
#define EHL_GBE_REGION_NUMPORTS_SIZE 0x04
#define EHL_GBE_REGION_MAC_SIZE 0x0C // Size of a MAC address entry 4 Bytes BDF + 4 Bytes MAC Address Low + 4 Bytes MAC Address High = 12 = 0x0C

#define EHL_GBE_REGION_VERSION_OFFSET 0x1C //16 Bytes GUID + 12 Bytes FFS Header = 28 = 1C
#define EHL_GBE_REGION_NUMPORTS_OFFSET 0x20 // 16 Bytes GUID + 12 Bytes FFS Header + 4 Bytes Version = 32 = 0x20
#define EHL_GBE_REGION_MAC_OFFSET 0x24 // Offset to first MAC address (EHL_GBE_REGION_NUMPORTS_OFFSET + 4 Bytes Number of ports)

//MOD009 ^
                                                                            
// Restore previous packing rules
#pragma pack(pop)																//MOD005 ^

#ifdef __cplusplus
}
#endif 

#define _INC_BIOSFLSH
#endif 

