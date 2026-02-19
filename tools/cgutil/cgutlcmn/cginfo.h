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
 * $Header:   S:/CG/archives/CGTOOLS/CGUTIL/CGUTLCMN/cginfo.h-arc   1.2   Sep 06 2016 15:54:00   congatec  $
 *
 * Contents: Congatec board/BIOS information module header file.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/CGUTIL/CGUTLCMN/cginfo.h-arc  $
 * 
 *    Rev 1.2   Sep 06 2016 15:54:00   congatec
 * Added BSD header.
 * 
 *    Rev 1.1   May 31 2012 15:45:36   gartner
 * Updated variable definition to ease 64bit porting.
 * 
 *    Rev 1.0   Oct 30 2006 15:04:52   gartner
 * Initial revision.
 * 
 * 
 *
 *---------------------------------------------------------------------------
 */

#ifndef _INC_CGINFO

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#endif 

#include "cgmpnv.h"
#pragma pack (1)

//+---------------------------------------------------------------------------
//       congatec system information structure
//+---------------------------------------------------------------------------
typedef struct
{
    unsigned char       BaseBiosVersion[9]; // 9 bytes for base BIOS version and zero termination.  
    unsigned char       OEMBiosVersion[9];  // 9 bytes for OEM BIOS version and zero termination.  
    unsigned char       FirmwareVersion[9]; // 9 bytes for CGBC firmware revision and zero termination.  
    UINT32       CgosAPIVersion;     // CGOS library/API version.
    UINT32       CgosDrvVersion;     // CGOS driver version.
    UINT32       BootCount;          // Boot counter
    UINT32       RunningTime;        // Running time
    CGOSBOARDINFO       CgosBoardInfo;      // CGOS board information structure
    UINT16      BupState;           // BIOS update protection active state
} CG_INFO_STRUCT ;

//+---------------------------------------------------------------------------
//       info query flags
//+---------------------------------------------------------------------------
#define CG_FLAG_INFO_BIOS   0x00000001      // System and OEM BIOS versions
#define CG_FLAG_INFO_CGOS   0x00000002      // CGOS API and driver versions
#define CG_FLAG_INFO_MANU   0x00000004      // Manufacturing info
#define CG_FLAG_INFO_CGBC   0x00000008      // Board controller info
#define CG_FLAG_INFO_BCNT   0x00000010      // Boot counter
#define CG_FLAG_INFO_RTIM   0x00000020      // Running time meter
#define CG_FLAG_INFO_BUP    0x00000040      // BIOS update protection

#pragma pack()

//+---------------------------------------------------------------------------
//       exported funtions
//+---------------------------------------------------------------------------
extern UINT16 CgInfoGetInfo( CG_INFO_STRUCT *pCgInfoStruct, UINT32 flags);

#ifdef __cplusplus
}
#endif 

#define _INC_CGINFO
#endif 


