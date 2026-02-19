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
 * Contents: Congatec board/BIOS information module.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * MOD001: Added support for MEC1706 based cBC.
 * 
 *    Rev 1.2   Sep 06 2016 15:52:02   congatec
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

/*---------------
 * Include files
 *---------------
 */
#include "cgutlcmn.h"
#include "cginfo.h"
#include "cgbmod.h"
#include "cgbc.h"

/*--------------
 * Externs used
 *--------------
 */

/*--------------------
 * Local definitions
 *--------------------
 */


/*-------------------------
 * Module global variables
 *-------------------------
 */

/*---------------------------------------------------------------------------
 * Name: 
 * Desc: 
 * Inp:  
 *       
 * Outp: 
 *---------------------------------------------------------------------------
 */
UINT16 CgInfoGetInfo
(
    CG_INFO_STRUCT *pCgInfoStruct,
    UINT32 flags
 )
{
    UINT16 retVal;
    unsigned char CgbcCmdWriteBuf[32] = {0};
    unsigned char CgbcCmdReadBuf[32] = {0};
    UINT32 CgbcCmdStatus = 0;
    unsigned char CgbcType = 0; //MOD001

    // Prepare for infomration gathering
    if ((retVal = CgMpfaStart(FALSE)) != CG_MPFARET_OK)
    {
        return CG_RET_FAILED;
    }

    // Gather required information

    // Get system and OEM BIOS versions
    if(flags & CG_FLAG_INFO_BIOS)
    {
        CgMpfaGetSysBiosVersion( (_TCHAR*) (&(pCgInfoStruct->BaseBiosVersion)),NULL);
        CgMpfaGetOEMBiosVersion( (_TCHAR*) (&(pCgInfoStruct->OEMBiosVersion)));
    }

    // More information only available on board level
    if(g_nOperationTarget == OT_BOARD)
    {
        // Get CGOS version information
        if(flags & CG_FLAG_INFO_CGOS)
        {
            pCgInfoStruct->CgosAPIVersion = CgosLibGetVersion();
            pCgInfoStruct->CgosDrvVersion = CgosLibGetDrvVersion();
        }
        if(flags & CG_FLAG_INFO_CGBC)
        {
            CgbcCmdWriteBuf[0] = CGBC_CMD_INFO_1; //MOD001 v 
            if(CgosCgbcHandleCommand(hCgos, &CgbcCmdWriteBuf[0], 1, &CgbcCmdReadBuf[0], 14, &CgbcCmdStatus))
            {
                CgbcType = CgbcCmdReadBuf[0]; // Byte 0 of CGBC_CMD_INFO_1 contains the Board Controller Type
                                              // This is needed for generating the version String. All CGBCs lower
                                              // than Gen5 had a version name starting with CGBCP, the version names for
                                              // the BC Gen5 start with GEN5P
                CgbcCmdWriteBuf[0] = CGBC_CMD_GET_FW_REV;
                //MOD001 ^
                if(CgosCgbcHandleCommand(hCgos, &CgbcCmdWriteBuf[0], 1, &CgbcCmdReadBuf[0], 3, &CgbcCmdStatus))
                {
#ifndef BANDR
                    //MOD001 v 
                    //sprintf((char*)(&(pCgInfoStruct->FirmwareVersion)),"CGBCP%c%c%c",CgbcCmdReadBuf[0], CgbcCmdReadBuf[1],CgbcCmdReadBuf[2]);
                    if(CgbcType == CGBC_MEC170x)                   /* MOD001 */
                    {
                        sprintf((char*)(&(pCgInfoStruct->FirmwareVersion)),"GEN5P%c%c%c",CgbcCmdReadBuf[0], CgbcCmdReadBuf[1],CgbcCmdReadBuf[2]);
                    }
                    else
                    {
                        sprintf((char*)(&(pCgInfoStruct->FirmwareVersion)),"CGBCP%c%c%c",CgbcCmdReadBuf[0], CgbcCmdReadBuf[1],CgbcCmdReadBuf[2]);
                    }
                    //MOD001 ^
#else
                    sprintf((char*)(&(pCgInfoStruct->FirmwareVersion)),"v%c%c%c",CgbcCmdReadBuf[0], CgbcCmdReadBuf[1],CgbcCmdReadBuf[2]);
#endif
                }
            } //MOD001
        }
        if(flags & CG_FLAG_INFO_MANU)
        {
            pCgInfoStruct->CgosBoardInfo.dwSize = sizeof(pCgInfoStruct->CgosBoardInfo);
            CgosBoardGetInfo(hCgos, &(pCgInfoStruct->CgosBoardInfo));
        }
        if(flags & CG_FLAG_INFO_BCNT)
        {
            CgosBoardGetBootCounter(hCgos, &(pCgInfoStruct->BootCount));
        }
        if(flags & CG_FLAG_INFO_RTIM)
        {
            CgosBoardGetRunningTimeMeter(hCgos, &(pCgInfoStruct->RunningTime));
        }
        // Also store state of the BIOS update protection
        pCgInfoStruct->BupState = CgMpfaCheckBUPActive();
    }

    // Perform cleanup
    if ((retVal = CgMpfaEnd()) != CG_MPFARET_OK)
    {
        return CG_RET_FAILED;
    }
    return CG_RET_OK;

}

