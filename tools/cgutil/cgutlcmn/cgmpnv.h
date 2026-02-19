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
 * $Header:   S:/CG/archives/CGTOOLS/INC/CGMPNV.H-arc   1.5   Sep 06 2016 15:56:16   congatec  $
 *
 * Contents: Multi Purpose NVRAM definitions.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/INC/CGMPNV.H-arc  $
 * 
 *    Rev 1.5   Sep 06 2016 15:56:16   congatec
 * Added BSD header.
 * 
 *    Rev 1.4   May 31 2012 15:45:46   gartner
 * Updated variable definition to ease 64bit porting.
 * 
 *    Rev 1.3   Jun 29 2011 16:43:40   ess
 * MOD003: added EEPROM cycle count in secure data area
 * 
 *    Rev 1.2   Mar 09 2006 18:34:44   ess
 * MOD002: added structure of the BIOS parameter EEPROM
 * 
 *    Rev 1.1   Jan 04 2006 15:34:00   ess
 * MOD001: updated structure of secure data area for board controller
 *         oscillator calibration byte
 * 
 *    Rev 1.0   Oct 04 2005 13:14:16   gartner
 * Initial revision.
 * 
 *    Rev 1.0   Sep 30 2005 16:31:18   gartner
 * Initial revision.
 *
 *---------------------------------------------------------------------------
 */

#ifndef _INC_CGMPNV

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#endif 

#pragma pack (1)

//+---------------------------------------------------------------------------
//
//       MPNV region definitions
//
#define CG_MPNV_SECURE          0x00
#define CG_MPNV_BIOS_EEP        0x01
#define CG_MPNV_BIOS_RAM        0x02
#define CG_MPNV_USER_EEP        0x03
#define CG_MPNV_USER_RAM        0x04



//+---------------------------------------------------------------------------
//
//       Layout of MPNV regions
//
typedef struct {
  unsigned char         cgTotalSize;            // total secure data area size in bytes
  unsigned char         cgCheckSize;            // size of the checksummed area
  unsigned char         cgChecksum;             // checksum correction byte
  unsigned char         cgSerialNumber[6];      // board serial number (BCD)
  unsigned char         cgPartNumber[16];       // board part number (ASCII)
  unsigned char         cgEanCode[7];           // board EAN-13 code (BCD)
  unsigned char         cgProductRevMaj;        // major product rev (ASCII)
  unsigned char         cgProductRevMin;        // minor product rev (ASCII)
  unsigned char         cgMfgDay;               // day of manufacturing (BCD 01h-31h)
  unsigned char         cgMfgMonth;             // month of manufacturing (BCD 01h-12h)
  unsigned char         cgMfgYear;              // year of manufacturing (BCD 05h-99h)
  unsigned char         cgRepairDay;            // day of last repair (BCD 01h-31h)
  unsigned char         cgRepairMonth;          // month of last repair (BCD 01h-12h)
  unsigned char         cgRepairYear;           // year of last repair (BCD 05h-99h)
  unsigned char         cgManufacturerId;       // manufacturer ID (hex)
  unsigned char         cgProjectId[4];         // project ID (ASCII)
  unsigned char         cgRepairCounter;        // repair counter (hex)
  unsigned char         cgSecureDataRes[9];     // spare      //MOD001 MOD003
  unsigned char         cgEepromCycles[3];      // EEPROM cycle count//MOD003
  unsigned char         cgCalibration;          // osc. calibration  //MOD001
  unsigned char         cgBootCounter[3];       // board boot counter (hex)
  UINT16				cgRunningTime;          // board running time in hours (hex)
} CG_SECURE_DATA;


                                                                     //MOD002v
typedef struct {
  unsigned char         cgBiosCfg1;             // BIOS configuration byte 1
  unsigned char         cgBiosEepDataRes[31];   // reserved
} CG_BIOS_EEP_DATA;

// CG_BIOS_EEP_DATA.cgBiosCfg1 format
//   bits 7-4: reserved
//   bits 3-0: BIOS OEM mode

#define  CG_OEM_MODE_MASK     0x0F
#define  CG_OEM_MODE_STANDARD 0x0F     // board configured for congatec mode
                                                                     //MOD002^


#pragma pack()

#ifdef __cplusplus
}
#endif 

#define _INC_CGMPNV
#endif 

