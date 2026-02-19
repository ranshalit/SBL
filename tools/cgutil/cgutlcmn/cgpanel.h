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
 * $Header:   S:/CG/archives/CGTOOLS/CGUTIL/CGUTLCMN/cgpanel.h-arc   1.6   Sep 06 2016 15:54:44   congatec  $
 *
 * Contents: Common panel configuration implementation module definitions.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/CGUTIL/CGUTLCMN/cgpanel.h-arc  $
 * 
 *    Rev 1.6   Sep 06 2016 15:54:44   congatec
 * Added BSD header.
 * 
 *    Rev 1.5   May 31 2012 15:45:48   gartner
 * Updated variable definition to ease 64bit porting.
 * 
 *    Rev 1.4   Jul 02 2007 13:12:32   gartner
 * Added definition for function to delete OEM EPI data set entry.
 * 
 *    Rev 1.3   Oct 30 2006 15:29:14   gartner
 * Added new function prototypes.
 * 
 *    Rev 1.2   Jan 27 2006 12:31:34   gartner
 * Changed return code definitions. Added support for the restart parameter to the MPFA related EPI functions.
 * 
 *    Rev 1.1   Dec 19 2005 13:28:48   gartner
 * MOD001: Added routine to clear EPI EEPROM; added backlight control routines.
 * 
 *    Rev 1.0   Oct 04 2005 13:14:16   gartner
 * Initial revision.
 * 
 *    Rev 1.0   Sep 30 2005 16:31:18   gartner
 * Initial revision.
 *
 *---------------------------------------------------------------------------
 */

#ifndef _INC_CGPANEL

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#endif 

#include "cgbmod.h"
#include "cgepi.h"

//+---------------------------------------------------------------------------
//      Interface functions
//+---------------------------------------------------------------------------
extern UINT16 CgEpiStart(void);
extern UINT16 CgEpiEnd(void);

extern UINT16 CgEpiReadEpiFromMpfa(_TCHAR *pOutputFilename);
extern UINT16 CgEpiWriteEpiToMpfa(_TCHAR *pInputFilename, UINT16 bRestart);

extern UINT16 CgEpiReadEdidFromEEP(_TCHAR *pOutputFilename);
extern UINT16 CgEpiWriteEdidToEEP(_TCHAR *pInputFilename);
extern UINT16 CgEpiClearEEP(void);

extern UINT16 CgEpiReadOEMEdidFromMpfa(_TCHAR *pOutputFilename, UINT16 nOEMEdidNo);
extern UINT16 CgEpiReadStdEdidFromMpfa(_TCHAR *pOutputFilename, UINT16 nStdEdidNo);
extern UINT16 CgEpiWriteOEMEdidToMpfa(_TCHAR *pInputFilename, UINT16 nOEMEdidNo, UINT16 bRestart);
extern UINT16 CgEpiDelOEMEdidFromMpfa(UINT16 nOEMEdidNo, UINT16 bRestart);
extern UINT16 CgEpiGetEpiDataSetDesc(_TCHAR *lpszEpiDesc, UINT16 nStdEdidNo,UINT16 bOemSelect);

extern UINT16 CgEpiSetBLValue(UINT32 valueBL);
extern UINT16 CgEpiGetBLValue(UINT32 *pValueBL);
extern UINT16 CgEpiSetBLEnable(UINT32 stateBL);
extern UINT16 CgEpiGetBLEnable(UINT32 *pStateBL);
//+---------------------------------------------------------------------------
//       EPI module return codes
//+---------------------------------------------------------------------------
#define CG_EPIRET_OK            CG_MPFARET_OK           // Success
#define CG_EPIRET_ERROR         CG_MPFARET_ERROR        // General error
#define CG_EPIRET_INTRF_ERROR   CG_MPFARET_INTRF_ERROR  // Interface access/init. error
#define CG_EPIRET_INCOMP        CG_MPFARET_INCOMP       // Data incompatibility error
#define CG_EPIRET_NOTFOUND      CG_MPFARET_NOTFOUND     // Data not found error
#define CG_EPIRET_INV           CG_MPFARET_INV          // Invalid data
#define CG_EPIRET_ERROR_SIZE    CG_MPFARET_ERROR_SIZE   // Size exceeded
#define CG_EPIRET_ERROR_DUP     CG_MPFARET_ERROR_DUP    // Duplicate module exists error
#define CG_EPIRET_ERROR_FILE    CG_MPFARET_ERROR_FILE   // File processing error
#define CG_EPIRET_NOTALLOWED    CG_MPFARET_NOTALLOWED   // Operation not allowed
#define CG_EPIRET_INV_DATA      CG_MPFARET_INV_DATA     // Invalid MPFA module data
#define CG_EPIRET_INV_PARM      CG_MPFARET_INV_PARM     // Invalid MPFA module parameters


#ifdef __cplusplus
}
#endif 

#define _INC_CGPANEL
#endif 


