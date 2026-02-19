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
 * $Header:   S:/CG/archives/CGTOOLS/CGUTIL/CGUTLCMN/CGUTLCMN.h-arc   1.10   Sep 06 2016 15:55:38   congatec  $
 *
 * Contents: Common congatec system utility definitions.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/CGUTIL/CGUTLCMN/CGUTLCMN.h-arc  $
 * 
 *    Rev 1.10   Sep 06 2016 15:55:38   congatec
 * Added BSD header. 
 * 
 *    Rev 1.9   Aug 08 2014 13:49:40   gartner
 * Updated to revision 1.5.3.
 * 
 *    Rev 1.8   Apr 24 2013 18:43:48   gartner
 * Updated for new minor revision number.
 * 
 *    Rev 1.7   May 31 2012 15:45:48   gartner
 * Updated variable definition to ease 64bit porting.
 * 
 *    Rev 1.6   Apr 16 2009 12:57:48   gartner
 * Updated version number.
 * 
 *    Rev 1.5   Nov 09 2006 13:14:46   gartner
 * Added read-only flag for ROMFILE operation.
 * 
 *    Rev 1.4   Oct 30 2006 15:30:58   gartner
 * Added all standard CGUTIL includes. Added req. definitions.
 * 
 *    Rev 1.3   Mar 21 2006 14:52:56   gartner
 * Updated MINOR version number.
 * 
 *    Rev 1.2   Jan 27 2006 12:29:32   gartner
 * Updated MINOR version number. Added support for access level handling.
 * 
 *    Rev 1.1   Dec 19 2005 13:29:22   gartner
 * Updated MAJOR version number.
 * 
 *    Rev 1.0   Oct 04 2005 13:14:16   gartner
 * Initial revision.
 * 
 *    Rev 1.0   Sep 30 2005 16:31:18   gartner
 * Initial revision.
 *
 *---------------------------------------------------------------------------
 */

#ifndef _CGUTLCMN_H_
#define _CGUTLCMN_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>
#include <malloc.h>
#include <conio.h>
#else
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#endif

#include "cgos.h"
#define UINT32	unsigned int
#define	INT32	int
#define UINT16	unsigned short
#define INT16	short


#ifdef WIN32
#define CGUTILMAIN _tmain
#else
#define CGUTILMAIN main
#define _TCHAR char
#define _T(s) s
#define getch getchar
#define Sleep(n) usleep(n*1000)
#endif

//-------------------
// Common defintions
//-------------------
#define FALSE   0
#define TRUE    1


//-------------------
// operation targets
//-------------------
#define OT_ROMFILE      0
#define OT_BOARD        1
#define OT_NONE         2


//--------------
// Return codes
//--------------
#define CG_RET_OK       0x00
#define CG_RET_FAILED   0x01

//------------------
// Global variables
//------------------

extern UINT16 g_nOperationTarget;
extern HCGOS hCgos;
extern FILE *g_fpBiosRomfile;
extern _TCHAR *g_lpszBiosFilename;
extern UINT16 g_nAccessLevel;
extern UINT16 g_nBiosReadOnly;

#ifdef _UNICODE

#define PRINTF wprintf
#define TOLOWER towlower
#define STRCMP wcscmp
#define FOPEN _wfopen
#define SSCANF swscanf
#define STRNCMP _wcsnicmp
#define TOLOWER towlower
#define SPRINTF wsprintf

#else //UNICODE

#define PRINTF printf
#define TOLOWER tolower
#define STRCMP strcmp
#ifdef WIN32
#define STRNCMP _strnicmp
#else
#define STRNCMP strncasecmp
#endif
#define FOPEN fopen
#define SSCANF sscanf
#define TOLOWER tolower
#define SPRINTF sprintf

#endif //_UNICODE

//---------------------
// Function prototypes
//---------------------
UINT16 CgosClose(void);
UINT16 CgosOpen(void);
UINT16 CgutlGetAccessLevel(void);
void CgClearScreen(void);

//---------------------
// Version definition
//---------------------
#define MAJOR_VERSION_NUMBER    1 //GMA
#define MINOR_VERSION_NUMBER    6 //GMA

//---------------------------
// Access level definitions
//---------------------------
#define CGUTL_ACC_LEV_USER      0x00    // Default access level
#define CGUTL_ACC_LEV_OEM       0x01    // Access level for trusted OEMs (OEM.CNG)
#define CGUTL_ACC_LEV_MANUF     0x02    // Access level for manufacturers (MANUF.CNG)
#define CGUTL_ACC_LEV_CONGA     0x80    // Unlimited functionality (CONGATEC.CNG file must be present)
    
#include "cgospriv.h"

#ifdef __cplusplus
}
#endif

#endif // _CGUTLCMN_H_
