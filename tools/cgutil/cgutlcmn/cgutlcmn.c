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
 
/*    Rev 1.5   Sep 06 2016 15:55:52   congatec
 * Added BSD header.
 * 
 *    Rev 1.4   May 31 2012 15:45:48   gartner
 * Updated variable definition to ease 64bit porting.
 * 
 *    Rev 1.3   Nov 09 2006 13:13:46   gartner
 * Added read-only flag for ROMFILE operation.
 * 
 *    Rev 1.2   Oct 30 2006 15:30:08   gartner
 * Performed cleanup; removed obsolete functions and header files.
 * 
 *    Rev 1.1   Jan 27 2006 12:30:02   gartner
 * Added support for access level handling.
 * 
 *    Rev 1.0   Oct 04 2005 13:14:16   gartner
 * Initial revision.
 * 
 *    Rev 1.0   Sep 30 2005 16:31:18   gartner
 * Initial revision.
 *
 *---------------------------------------------------------------------------
 */

/*---------------
 * Include files
 *---------------
 */
#include "cgutlcmn.h"

/*--------------------
 * Local definitions
 *--------------------
 */

/*------------------
 * Global variables
 *------------------
 */
UINT16 g_nOperationTarget;
HCGOS hCgos = 0;
FILE *g_fpBiosRomfile = NULL;
UINT16 g_nAccessLevel = CGUTL_ACC_LEV_USER;
UINT16 g_nBiosReadOnly = FALSE;

_TCHAR *g_lpszBiosFilename = NULL;
_TCHAR g_szBiosVersion[] = "PROJRxxx";
_TCHAR g_szFirmwareVersion[] = "CGBCPxxx";


/*---------------------------------------------------------------------------
 * Name:        CgosOpen
 * Desc:        Try to open and initalize the CGOS interface.
 * Inp:         none
 * Outp:        Status:
 *              FALSE   - Error
 *              TRUE    - Success
 *---------------------------------------------------------------------------
 */
UINT16 CgosOpen(void)
{        
    if (!CgosLibInitialize()) 
    {
        if (!CgosLibInstall(1)) 
        {                        
            return FALSE;
        }
        if (!CgosLibInitialize()) 
        {
            return FALSE;
        }
    }
        
    if (!CgosBoardOpen(0,0,0,&hCgos)) 
    {
        return FALSE;
    }
    return TRUE;          
}

/*---------------------------------------------------------------------------
 * Name:        CgosClose
 * Desc:        Try to close and un-initalize the CGOS interface.
 * Inp:         none
 * Outp:        Status:
 *              FALSE   - Error
 *              TRUE    - Success
 *---------------------------------------------------------------------------
 */
UINT16 CgosClose(void)
{        
    if (hCgos) 
    {
        CgosBoardClose(hCgos);
        hCgos = 0;
    }
    CgosLibUninitialize();
    return TRUE;          
}

/*---------------------------------------------------------------------------
 * Name:        CgutlGetAccessLevel
 * Desc:        Set the access level for the utility. Depending on the 
 *              selected access level several functions might be blocked.
 *              The access level is set by putting different access level
 *              definition files into the same directory as the utility
 *              executable.
 * Inp:         none
 * Outp:        Status:
 *              FALSE   - Error
 *              TRUE    - Success
 *---------------------------------------------------------------------------
 */
UINT16 CgutlGetAccessLevel(void)
{       
    FILE *fpCtlFile = NULL; 
    // Check for control level selection files. The files must be queried 
    // in the order of the control level they select, starting with the 
    // highest priority.
    if ((fpCtlFile = fopen("CONGATEC.CNG", "rb")))
    {
        g_nAccessLevel = CGUTL_ACC_LEV_CONGA;
    }
    else if ((fpCtlFile = fopen("MANUF.CNG", "rb")))
    {
        g_nAccessLevel = CGUTL_ACC_LEV_MANUF;
    }
    else if ((fpCtlFile = fopen("OEM.CNG", "rb")))
    {
        g_nAccessLevel = CGUTL_ACC_LEV_OEM;
    }
    else
    {
        g_nAccessLevel = CGUTL_ACC_LEV_USER;
    }
    if(fpCtlFile)
    {
        fclose(fpCtlFile);
    }

    return TRUE;          
}

#ifdef WIN32
/*---------------------------------------------------------------------------
 * Name: ClearScreen
 * Desc: Clear the Windows console screen.
 * Inp:  none
 * Outp: none
 *---------------------------------------------------------------------------
 */
void CgClearScreen(void) 
{ 
    COORD coordScreen = { 0, 0 }; 
    DWORD cCharsWritten; 
    CONSOLE_SCREEN_BUFFER_INFO csbi; 
    DWORD dwConSize; 
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE); 
    
    GetConsoleScreenBufferInfo(hConsole, &csbi); 
    dwConSize = csbi.dwSize.X * csbi.dwSize.Y; 
    FillConsoleOutputCharacter(hConsole, TEXT(' '), dwConSize, 
                               coordScreen, &cCharsWritten); 
    GetConsoleScreenBufferInfo(hConsole, &csbi); 
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, 
                               coordScreen, &cCharsWritten); 
    SetConsoleCursorPosition(hConsole, coordScreen); 
}
#else
void CgClearScreen(void) 
{ 
    char *args[]={NULL};
    execv("clear", args ); 
}
#endif


