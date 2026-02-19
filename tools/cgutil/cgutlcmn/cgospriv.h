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

// CgosPriv.h
// CGOS API private declarations
// {G)U(2} 2005.05.22

/*
 * $Log:   S:/CG/archives/CGTOOLS/INC/CgosPriv.h-arc  $
 * 
 *    Rev 1.9   Sep 06 2016 15:54:32   congatec
 * Added BSD header.
 * 
 *    Rev 1.8   Jul 08 2013 10:02:58   ess
 * MOD005: added more CgbcSetControl parameters
 * 
 *    Rev 1.7   May 31 2012 15:45:48   gartner
 * Updated variable definition to ease 64bit porting.
 * 
 *    Rev 1.6   Dec 06 2010 12:11:54   gartner
 * MOD004: Added support for new extended MPFa flash region.
 * 
 *    Rev 1.5   Dec 18 2008 15:56:46   ess
 * MOD003: added support for ATmega325P board controller
 * 
 *    Rev 1.4   Aug 26 2008 21:47:24   ess
 * MOD002: added support for ATmega165P board controller
 * 
 *    Rev 1.3   Jul 02 2007 12:03:58   gartner
 * Updated for CGOS version 1.03.016 (no change).
 * 
 *    Rev 1.2   Oct 26 2006 19:02:12   gartner
 * Updated for CGOS version 1.02.014.
 * 
 *    Rev 1.1   Mar 09 2006 18:36:32   ess
 * MOD001: defined storage area types for BIOS parameter EEPROM and BIOS
 *         parameter RAM
 */

//***************************************************************************

#ifndef _CGOSPRIV_H_
#define _CGOSPRIV_H_

#ifdef __cplusplus
extern "C" {
#endif

//***************************************************************************
//
// CgbcGetInfo
//

typedef struct {
  unsigned int size;
  unsigned int type;
  unsigned int flags;
  unsigned int flashSize;
  unsigned int eepromSize;
  unsigned int ramSize;
  unsigned int firmwareRevision;
  } CGOSBCINFO, CGEB_BC_INFO;

// BC (sub) type flags

#define CGEB_BC_TYPE_NONE             0x00000000
#define CGEB_BC_TYPE_UNKNOWN          0x00010000
#define CGEB_BC_TYPE_EMU              0x00020000
#define CGEB_BC_TYPE_ATMEL            0x00030000
#define CGEB_BC_TYPE_ATMEL_ATMEGA     0x00030001
#define CGEB_BC_TYPE_ATMEL_ATMEGA48   0x00030002
#define CGEB_BC_TYPE_ATMEL_ATMEGA88   0x00030003
#define CGEB_BC_TYPE_ATMEL_ATMEGA168  0x00030004
#define CGEB_BC_TYPE_ATMEL_ATMEGA165P 0x00030005                     //MOD002
#define CGEB_BC_TYPE_ATMEL_ATMEGA325P 0x00030006                     //MOD003
#define CGEB_BC_TYPE_PIC              0x00040000

// BC flags

#define CGEB_BC_FLAG_I2C   0x00000001
#define CGEB_BC_FLAG_WD    0x00000002
#define CGEB_BC_FLAG_EEP   0x00000004

// CgbcSetControl flags

#define CGEB_BC_CONTROL_SS 0
#define CGEB_BC_CONTROL_RESET 1
#define CGEB_BC_CONTROL_WDTRIG 2                                     //MOD005
#define CGEB_BC_CONTROL_SCICHK 3                                     //MOD005

#ifndef NOCGOSAPI

//***************************************************************************

//
// Board Controller functions
//
cgosret_bool CgosCgbcGetInfo(HCGOS hCgos, unsigned int dwType, CGOSBCINFO *pInfo);
cgosret_bool CgosCgbcSetControl(HCGOS hCgos, unsigned int dwLine, unsigned int dwSetting);
cgosret_bool CgosCgbcReadWrite(HCGOS hCgos, unsigned char bDataByte, unsigned char *pDataByte,
  unsigned int dwClockDelay, unsigned int dwByteDelay);
cgosret_bool CgosCgbcHandleCommand(HCGOS hCgos, unsigned char *pBytesWrite, unsigned int dwLenWrite,
  unsigned char *pBytesRead, unsigned int dwLenRead, unsigned int *pdwStatus);

//***************************************************************************

//
// Backdoor functions
//

cgosret_bool CgosCgeb(HCGOS hCgos, unsigned char *pBytes, unsigned int dwLen);
CGOSDLLAPI unsigned char *CGOSAPI CgosCgebTransAddr(HCGOS hCgos, unsigned char *pBytes);
cgosret_bool CGOSAPI CgosCgebDbgLevel(HCGOS hCgos, unsigned int dwLevel);

//***************************************************************************

//
// Reserved functions
//

cgosret_bool CgosBoardGetOption(HCGOS hCgos, unsigned int dwOption, unsigned int *pdwSetting);
cgosret_bool CgosBoardSetOption(HCGOS hCgos, unsigned int dwOption, unsigned int dwSetting);
cgosret_bool CgosBoardGetBootErrorLog(HCGOS hCgos, unsigned int dwType, unsigned int *pdwLogType, unsigned char *pBytes, unsigned int *pdwLen);
cgosret_bool CgosVgaEndDarkBoot(HCGOS hCgos, unsigned int dwReserved);

//***************************************************************************

//
// Obsolete functions
//

cgosret_ulong CgosWDogGetTriggerCount(HCGOS hCgos, unsigned int dwType);
cgosret_bool CgosWDogSetTriggerCount(HCGOS hCgos, unsigned int dwType, unsigned int cnt);

//***************************************************************************


#define CG32_STORAGE_FLASH              0x00020000      // User flash
#define CG32_STORAGE_MPFA_DYNAMIC       0x40020000      // Dynamic MPFA
#define CG32_STORAGE_MPFA_STATIC        0x40020100      // Static MPFA
#define CG32_STORAGE_MPFA_ALL           0x80020000      // BIOS ROM part (comprises _STATIC, _USER, _DYNAMIC).	//MOD004 v
#define CG32_STORAGE_MPFA_EXTD			0x81020000      // Whole flash part (e.g. complete SPI 
														// contents including descriptor, ME, BIOS,
														// ethernet configuration,...).
														// For non-descriptor BIOS identical to CG_MPFA_ALL		//MOD004 ^                                       

                                                                     //MOD001v
#define CG32_STORAGE_EEPROM_BIOS        0x40010000      // BIOS parameter EEP
#define CG32_STORAGE_RAM_BIOS           0x40040000      // BIOS parameter RAM
                                                                     //MOD001^
#endif // NOCGOSAPI

#ifdef __cplusplus
}
#endif

#endif // _CGOSPRIV_H_

