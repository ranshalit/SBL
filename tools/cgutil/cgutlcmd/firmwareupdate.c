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
 * $Header:   S:/CG/archives/CGTOOLS/CGUTIL/W32DOSX/CGUTLCMD/FirmwareUpdate.c-arc   1.3   Sep 06 2016 16:21:46   congatec  $
 *
 * Contents: Congatec board controller firmware update command line module.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/CGUTIL/W32DOSX/CGUTLCMD/FirmwareUpdate.c-arc  $
 * 
 *    Rev 1.3   Sep 06 2016 16:21:46   congatec
 * Added BSD header.
 * 
 *    Rev 1.2   May 31 2012 15:35:20   gartner
 * Updated variable definitions to ease 64bit porting.
 * 
 *    Rev 1.1   Oct 30 2006 14:41:14   gartner
 * Perform cleanup. Added support for build switch.
 * 
 *    Rev 1.0   Oct 04 2005 13:16:32   gartner
 * Initial revision.
 * 
 *    Rev 1.0   Sep 30 2005 16:25:06   gartner
 * Initial revision.
 *
 *---------------------------------------------------------------------------
 */

/*---------------
 * Include files
 *---------------
 */
#include "cgutlcmn.h"

/*--------------
 * Externs used
 *--------------
 */
extern INT32 BcprgcmdMain( INT32 argc, _TCHAR* argv[] );

/*--------------------
 * Local definitions
 *--------------------
 */
#define BCPRG_HW_EMULATION 0

/*------------------
 * Global variables
 *------------------
 */

/*---------------------------------------------------------------------------
 * Name:
 * Desc:
 * Inp:         
 * Outp:        
 *---------------------------------------------------------------------------
 */
void HandleFirmwareUpdate(INT32 argc, _TCHAR* argv[])
{
    INT32 retCode;
#ifdef BANDR
    PRINTF(_T("CPU-Board Controller Firmware Update Module\n\n"));
#else
    PRINTF(_T("Board Controller Firmware Update Module\n\n"));
#endif

#if BCPRG_HW_EMULATION
    hCgos = 1;
#else
    if (!CgosOpen())
    {
        PRINTF(_T("\nERROR: Failed to initialize system interface!\n\n"));
        exit(1);
    }
#endif
    retCode = BcprgcmdMain( argc, argv );

#if BCPRG_HW_EMULATION

#else
    CgosClose();
#endif
    exit (retCode);
}
