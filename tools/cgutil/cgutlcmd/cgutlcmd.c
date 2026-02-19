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
 * $Header:   S:/CG/archives/CGTOOLS/CGUTIL/cgutillx/cgutlcmd/cgutlcmd.c-arc   1.9   Oct 17 2016 15:03:34   congatec  $
 *
 * Contents: congatec system utility main module (command line).
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/CGUTIL/cgutillx/cgutlcmd/cgutlcmd.c-arc  $
 * 
 *    Rev 1.9   Oct 17 2016 15:03:34   congatec
 * Increased build number to 7
 * 
 *    Rev 1.8   Sep 08 2016 11:29:24   congatec
 * Fixed build number
 * 
 *    Rev 1.7   Sep 07 2016 12:34:22   congatec
 * Updated copyright year.
 * 
 *    Rev 1.6   Aug 06 2015 13:47:44   gartner
 * Updated build number and copyright year.
 * 
 *    Rev 1.5   Nov 21 2014 16:50:06   gartner
 * Updated build number.
 * 
 *    Rev 1.4   Aug 11 2014 15:58:26   gartner
 * Updated for v 1.5.3
 * 
 *    Rev 1.3   Jul 16 2013 12:26:58   gartner
 * Updated for v1.5.2
 * 
 *    Rev 1.2   Jul 09 2013 16:58:00   gartner
 * Updated copyright year.
 * 
 *    Rev 1.1   Jul 09 2013 16:56:20   gartner
 * Updated for v1.5.1
 * 
 *    Rev 1.0   May 30 2012 11:05:14   gartner
 * Initial revision.
 * 
 *    Rev 1.5   Jan 10 2012 15:37:42   gartner
 * Updated build number to 7.
 * 
 *    Rev 1.4   Jun 22 2011 20:18:00   gartner
 * Updated build number to 6.
 * 
 *    Rev 1.3   Feb 07 2011 18:10:54   gartner
 * Updated build number and copyright year.
 * 
 *    Rev 1.2   Feb 15 2010 18:13:48   gartner
 * Show build number as well.
 * 
 *    Rev 1.1   Feb 15 2010 15:45:54   gartner
 * Updated copyright year and build number for v1.4.1
 * 
 *    Rev 1.0   Sep 09 2009 11:22:06   gartner
 * Initial revision.
 * 
 *
 *---------------------------------------------------------------------------
 */

/*---------------
 * Include files
 *---------------
 */
#include "cgutlcmn.h"
#include "cgutil.h"

/*--------------
 * Externs used
 *--------------
 */
#ifdef __cplusplus
extern "C" {
#endif
extern void HandleBiosUpdate(INT32 argc, _TCHAR* argv[]);
extern void HandleFirmwareUpdate(INT32 argc, _TCHAR* argv[]);
extern void HandleStorageArea(INT32 argc, _TCHAR* argv[]);
extern void HandleBiosModules(INT32 argc, _TCHAR* argv[]);
extern void HandleReportGeneration(INT32 argc, _TCHAR* argv[]);
extern void HandlePanelConfiguration(INT32 argc, _TCHAR* argv[]);
extern void HandleCgosTest(INT32 argc, _TCHAR* argv[]);
extern void HandleInfo(INT32 argc, _TCHAR* argv[]);

#ifdef __cplusplus
}
#endif

/*--------------------
 * Local definitions
 *--------------------
 */
#define BUILD_NUMBER            2 //MODGMA 

#define CGEVAL                  0
// Major and minor version number are defined in cgutlcmn.h

typedef struct
{
    _TCHAR modSelector[9];
    _TCHAR modDescription[48];
    void (*fpModEntry) (INT32 argc, _TCHAR* argv[]);
} CG_UTIL_MODULE;

/*-------------------------
 * Module global variables
 *-------------------------
 */
static _TCHAR signonMes[] = "\n"
                            "congatec System Configuration Utility ---  Version %d.%d.%d\n"                            
                            "(C) Copyright 2005-2023 congatec GmbH\n\n";

static CG_UTIL_MODULE moduleList[] =   {
                                {"BCPROG", "Board Controller Firmware Update Module", HandleFirmwareUpdate},
                                {"BFLASH", "System BIOS Update Module", HandleBiosUpdate},
                                {"CPANEL", "Panel Configuration Module", HandlePanelConfiguration},
                                {"MODULE", "BIOS Module Modification Module", HandleBiosModules},
                                {"CGINFO", "Board/BIOS Information Module", HandleInfo},
                                };

/*---------------------------------------------------------------------------
 * Name: ShowSignon
 * Desc: Clear screen and display CGUTLCMD signon message.
 * Inp:  none
 * Outp: none
 *---------------------------------------------------------------------------
 */
void ShowSignon(void)
{
    // ClearScreen();
    PRINTF(_T(signonMes),MAJOR_VERSION_NUMBER, MINOR_VERSION_NUMBER, BUILD_NUMBER );        
}

/*---------------------------------------------------------------------------
 * Name: ShowUsage
 * Desc: Display usage and exit with error.
 * Inp:  none
 * Outp: none
 *---------------------------------------------------------------------------
 */
void ShowUsage(void)
{
    INT32 i;

    PRINTF(_T("\nUsage:\n\n"));
    PRINTF(_T("CGUTLCMD [MODULE] [...module specific parameters...]\n\n"));
    PRINTF(_T("\n"));
    PRINTF(_T("Supported Modules:\n\n"));
    for(i=0; i < (sizeof moduleList / sizeof moduleList[0]); i++)
    {
        PRINTF(_T("%s"), moduleList[i].modSelector);
        PRINTF(_T("\t"));
        PRINTF(_T("%s"), moduleList[i].modDescription);
        PRINTF(_T("\n"));
    }
    PRINTF(_T("\n\n"));
    PRINTF(_T("Passing a valid module selector without any additional parameters will\n"));
    PRINTF(_T("display the module specific help or usage screen.\n\n"));    
    exit(1);
}



/*---------------------------------------------------------------------------
 * Name: CGUTILMAIN
 * Desc: CGUTLCMD entry and main dispatch routine.
 * Inp:  argc   - number of command line parameters passed
 *       argv[] - array of pointers to command line parameter strings
 * Outp: exit code (0=success ; 1=error)
 *---------------------------------------------------------------------------
 */
INT32 CGUTILMAIN(INT32 argc, _TCHAR* argv[])
{
    INT32     i;
    
    //Clear screen and display signon message.
    ShowSignon();

    // Set the access level
    CgutlGetAccessLevel();

    if(argc < 2)
    {
        // Display usage description.
        ShowUsage();
        return 1;
    }

    // Parse through module list and check whether the respective module has been selected.  
    for(i=0; i < (sizeof moduleList / sizeof moduleList[0]); i++)
    {
        if (STRNCMP(argv[1],  moduleList[i].modSelector, 6) == 0)
        {
            if (moduleList[i].fpModEntry != NULL)
            {
                (*moduleList[i].fpModEntry) (argc - 1, &argv[1]);        
            }
            break;
        }
        if(i == (sizeof moduleList / sizeof moduleList[0]) -1)
        {
            // Reached end of module list -> error.
            PRINTF(_T("ERROR: You have to select a valid module!\n"));
            ShowUsage();
        }
    }    
	exit(0);
}

