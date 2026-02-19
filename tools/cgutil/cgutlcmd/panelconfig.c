/*---------------------------------------------------------------------------
 *
 * Copyright (c) 2016, congatec AG. All rights reserved.
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
 * $Header:   S:/CG/archives/CGTOOLS/CGUTIL/W32DOSX/CGUTLCMD/PanelConfig.c-arc   1.8   Sep 06 2016 16:21:20   congatec  $
 *
 * Contents: Congatec panel configuration command line module.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/CGUTIL/W32DOSX/CGUTLCMD/PanelConfig.c-arc  $
 * 
 *    Rev 1.8   Sep 06 2016 16:21:20   congatec
 * Added BSD header.
 * 
 *    Rev 1.7   May 31 2012 15:35:28   gartner
 * Updated variable definitions to ease 64bit porting.
 * 
 *    Rev 1.6   Jul 02 2007 12:56:52   gartner
 * Added option to delete OEM EPI data set.
 * 
 *    Rev 1.5   Nov 09 2006 13:07:10   gartner
 * Fixed description about how to disable the BIOS update and write protection.
 * 
 *    Rev 1.4   Oct 30 2006 14:43:26   gartner
 * Added support for build switches. Added support for BIOS write protection. Added support to display and read standard EPI module datasets and to selectively update more than one OEM EPI dataset.
 * 
 *    Rev 1.3   Mar 21 2006 15:15:16   gartner
 * Added support for xxx version.
 * 
 *    Rev 1.2   Jan 27 2006 12:40:20   gartner
 * Fixed and extended retrun code handling. Adapted to new function parameters used by functions in cgepi.c.
 * 
 *    Rev 1.1   Dec 19 2005 13:24:06   gartner
 * MOD001: Added support to clear EPI EEPROM. Added support 
 * to control backlight.
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
#include "cgpanel.h"

/*--------------
 * Externs used
 *--------------
 */

// Commands
#define CMD_SAVE_EPI        0
#define CMD_LOAD_EPI        1
#define CMD_SAVE_OEM        2
#define CMD_LOAD_OEM        3
#define CMD_SAVE_EEP        4
#define CMD_LOAD_EEP        5
#define CMD_CLEAR_EEP       6
#define CMD_SET_BLVAL       7
#define CMD_GET_BLVAL       8
#define CMD_SET_BLSTATE     9
#define CMD_GET_BLSTATE     10
#define CMD_SAVE_STD        11
#define CMD_DUMP_PL         12
#define CMD_DEL_OEM         13

/*-------------------------
 * Module global variables
 *-------------------------
 */
static _TCHAR szBiosFilename[256], szIOFilename[256];
static _TCHAR szEPIDataSetDesc[128];
static UINT32 parameter; 
static UINT32 command;
static UINT16 bValReq = FALSE;
static UINT16 bFileReq = FALSE;

/*---------------------------------------------------------------------------
 * Name: ShowUsage
 * Desc: Display parameters for this module.
 * Inp:  none
 * Outp: none
 *---------------------------------------------------------------------------
 */
static void ShowUsage(void) 
{
    PRINTF(_T("\nUsage:\n\n"));
    PRINTF(_T("CPANEL /OT:[target] /[command] [parm]\n\n"));
    PRINTF(_T("/OT:   - Specify the operation target.\n"));
    PRINTF(_T("         BOARD  - All actions result in direct hardware accesses to the\n"));
    PRINTF(_T("                  respective board.\n"));
    PRINTF(_T("         All other strings are interpreted as name of the BIOS file\n"));
    PRINTF(_T("         to be used as operation target.\n"));
    PRINTF(_T("[parm] - Command parameter (if required). E.g. input/output file name.\n"));
    
    PRINTF(_T("\nPress ENTER to continue...\n"));
    getch();

    PRINTF(_T("\nCommands:\n\n"));
    PRINTF(_T("/SEPI     - Find EPI module and save it.\n"));
    PRINTF(_T("/LEPI     - Add/replace EPI module.\n"));
    PRINTF(_T("/SSTD:xxx - Find standard EPI EDID data set number xxx (hex) and save it.\n"));
    PRINTF(_T("/SOEM:xxx - Find OEM EDID data set number xxx (hex) and save it.\n"));
    PRINTF(_T("/LOEM:xxx - Add/replace OEM EDID data set number xxx (hex)\n"));
    PRINTF(_T("/DOEM:xxx - Delete OEM EDID data set number xxx (hex)\n"));
    PRINTF(_T("/SEEP     - Save EPI EEPROM contents.\n"));
    PRINTF(_T("/LEEP     - Write data to EPI EEPROM.\n"));    
    PRINTF(_T("/CEEP     - Clear EPI EEPROM.\n"));   
#ifndef BANDR
    PRINTF(_T("/SBLV     - Set backlight control value (0-100, decimal).\n"));    
    PRINTF(_T("/GBLV     - Get current backlight control value.\n"));    
    PRINTF(_T("/SBLS     - Set backlight enable state (0: OFF 1: ON).\n"));    
    PRINTF(_T("/GBLS     - Get backlight enable state (0: OFF 1: ON).\n")); 
#endif
    PRINTF(_T("/DPL      - Display list of EPI panel data sets included in BIOS.\n")); 

    PRINTF(_T("\n\n")); 
    PRINTF(_T("NOTE : When the 'BIOS Update & Write Protection' feature is enabled\n"));
    PRINTF(_T("       in BIOS setup, commands that try to write to the BIOS flash will fail.\n"));    
    PRINTF(_T("       In order to deactivate the BIOS write protection until next boot\n"));
#ifdef CONGA
    PRINTF(_T("       please use: 'CGUTLCMD BFLASH /BP:[password]'.\n"));
#endif
#ifdef COMMON
    PRINTF(_T("       please use: 'CGUTLCMD BFLASH /BP:[password]'.\n"));
#endif
#ifdef BANDR
    PRINTF(_T("       please use: 'XCPBSVC BFLASH /BP:[password]'.\n"));
#endif
 
}


/*---------------------------------------------------------------------------
 * Name:
 * Desc:
 * Inp:         
 * Outp:        
 *---------------------------------------------------------------------------
 */
void HandlePanelConfiguration(INT32 argc, _TCHAR* argv[])
{ 
    _TCHAR cTemp;
    INT32 exitState = 0;
    UINT32 retVal;
    UINT16 nOEMEdidNo = 0;    
    UINT16 nStdEdidNo = 0;    
        
    PRINTF(_T("Panel Configuration Module\n"));
    if(argc < 2)
    {
        ShowUsage();
        exit(1);
    }
    // Select operation target / operation mode.
    if (STRNCMP(argv[1], _T("/OT:"), 4) != 0)
	{
        PRINTF(_T("ERROR: You have to select an operation target!\n"));
        exit(1);
    }
    if ((SSCANF(argv[1], _T("/OT:%s%c"), &szBiosFilename[0], &cTemp) == 1) ||
        (SSCANF(argv[1], _T("/ot:%s%c"), &szBiosFilename[0], &cTemp) == 1))
	{
	    if(STRNCMP(&szBiosFilename[0], _T("BOARD"), 5) == 0)
        {
            g_nOperationTarget = OT_BOARD;
        }
        else
        {
            g_nOperationTarget = OT_ROMFILE;
            g_lpszBiosFilename = &szBiosFilename[0];
        }
    }
	else
	{
        PRINTF(_T("ERROR: You have to select an operation target!\n"));
        exit(1);
	}


    if(argc < 3)
    {
        PRINTF(_T("ERROR: You have to pass a command!\n"));
        exit(1);
    }

	// Select command to be processed
    if (STRNCMP(argv[2], _T("/SEPI"), 5) == 0)
	{
        command = CMD_SAVE_EPI;
        bValReq = FALSE;
        bFileReq = TRUE;
    }
    else if (STRNCMP(argv[2], _T("/LEPI"),5) == 0)
	{
        command = CMD_LOAD_EPI;
        bValReq = FALSE;
        bFileReq = TRUE;
    }
    else if (STRNCMP(argv[2], _T("/SSTD:"),6) == 0)
	{
        command = CMD_SAVE_STD;
        bValReq = FALSE;
        bFileReq = TRUE;
        if (!((SSCANF(argv[2], _T("/SSTD:%02hx%c"), &nStdEdidNo, &cTemp) == 1)  ||
            (SSCANF(argv[2], _T("/sstd:%02hx%c"), &nStdEdidNo, &cTemp) == 1)  ||
            (SSCANF(argv[2], _T("/SSTD:0x%02hx%c"), &nStdEdidNo, &cTemp) == 1)  ||
            (SSCANF(argv[2], _T("/sstd:0x%02hx%c"), &nStdEdidNo, &cTemp) == 1)))
        {
            PRINTF(_T("ERROR: You have to specify a valid data set number!\n"));
            exit(1);
        }
        if(nStdEdidNo < 1)
        {
            PRINTF(_T("ERROR: You have to specify a valid data set number!\n"));
            exit(1);
        }
        else
        {
            nStdEdidNo = nStdEdidNo -1;
        }
    }
    else if (STRNCMP(argv[2], _T("/SOEM:"),6) == 0)
	{
        command = CMD_SAVE_OEM;
        bValReq = FALSE;
        bFileReq = TRUE;
        if (!((SSCANF(argv[2], _T("/SOEM:%02hx%c"), &nOEMEdidNo, &cTemp) == 1)  ||
            (SSCANF(argv[2], _T("/soem:%02hx%c"), &nOEMEdidNo, &cTemp) == 1)  ||
            (SSCANF(argv[2], _T("/SOEM:0x%02hx%c"), &nOEMEdidNo, &cTemp) == 1)  ||
            (SSCANF(argv[2], _T("/soem:0x%02hx%c"), &nOEMEdidNo, &cTemp) == 1)))
        {
            PRINTF(_T("ERROR: You have to specify a valid data set number!\n"));
            exit(1);
        }
        if(nOEMEdidNo < 1)
        {
            PRINTF(_T("ERROR: You have to specify a valid data set number!\n"));
            exit(1);
        }
        else
        {
            nOEMEdidNo = nOEMEdidNo -1;
        }
    }
    else if (STRNCMP(argv[2], _T("/LOEM:"),6) == 0)
	{
        command = CMD_LOAD_OEM;
        bValReq = FALSE;
        bFileReq = TRUE;
        if (!((SSCANF(argv[2], _T("/LOEM:%02hx%c"), &nOEMEdidNo, &cTemp) == 1)  ||
            (SSCANF(argv[2], _T("/loem:%02hx%c"), &nOEMEdidNo, &cTemp) == 1)  ||
            (SSCANF(argv[2], _T("/LOEM:0x%02hx%c"), &nOEMEdidNo, &cTemp) == 1)  ||
            (SSCANF(argv[2], _T("/loem:0x%02hx%c"), &nOEMEdidNo, &cTemp) == 1)))
        {
            PRINTF(_T("ERROR: You have to specify a valid data set number!\n"));
            exit(1);
        }
        if(nOEMEdidNo < 1)
        {
            PRINTF(_T("ERROR: You have to specify a valid data set number!\n"));
            exit(1);
        }
        else
        {
            nOEMEdidNo = nOEMEdidNo -1;
        }
    }
    else if (STRNCMP(argv[2], _T("/DOEM:"),6) == 0)
	{
        command = CMD_DEL_OEM;
        bValReq = FALSE;
        bFileReq = FALSE;
        if (!((SSCANF(argv[2], _T("/DOEM:%02hx%c"), &nOEMEdidNo, &cTemp) == 1)  ||
            (SSCANF(argv[2], _T("/doem:%02hx%c"), &nOEMEdidNo, &cTemp) == 1)  ||
            (SSCANF(argv[2], _T("/DOEM:0x%02hx%c"), &nOEMEdidNo, &cTemp) == 1)  ||
            (SSCANF(argv[2], _T("/doem:0x%02hx%c"), &nOEMEdidNo, &cTemp) == 1)))
        {
            PRINTF(_T("ERROR: You have to specify a valid data set number!\n"));
            exit(1);
        }
        if(nOEMEdidNo < 1)
        {
            PRINTF(_T("ERROR: You have to specify a valid data set number!\n"));
            exit(1);
        }
        else
        {
            nOEMEdidNo = nOEMEdidNo -1;
        }
    }
    else if (STRNCMP(argv[2], _T("/SEEP"),5) == 0)
	{
        command = CMD_SAVE_EEP;
        bValReq = FALSE;
        bFileReq = TRUE;
    }
    else if (STRNCMP(argv[2], _T("/LEEP"),5) == 0)
	{
        command = CMD_LOAD_EEP;
        bValReq = FALSE;
        bFileReq = TRUE;
    }
    else if (STRNCMP(argv[2], _T("/CEEP"),5) == 0)
	{
        command = CMD_CLEAR_EEP;
        bValReq = FALSE;
        bFileReq = FALSE;
    }
#ifndef BANDR
    else if (STRNCMP(argv[2], _T("/SBLV"),5) == 0)
	{
        command = CMD_SET_BLVAL;
        bValReq = TRUE;
        bFileReq = FALSE;
    }
    else if (STRNCMP(argv[2], _T("/GBLV"),5) == 0)
	{
        command = CMD_GET_BLVAL;
        bValReq = FALSE;
        bFileReq = FALSE;
    }
    else if (STRNCMP(argv[2], _T("/SBLS"),5) == 0)
	{
        command = CMD_SET_BLSTATE;
        bValReq = TRUE;
        bFileReq = FALSE;
    }
    else if (STRNCMP(argv[2], _T("/GBLS"),5) == 0)
	{
        command = CMD_GET_BLSTATE;
        bValReq = FALSE;
        bFileReq = FALSE;
    }
#endif
    else if (STRNCMP(argv[2], _T("/DPL"), 4) == 0)
	{
        command = CMD_DUMP_PL;
        bValReq = FALSE;
        bFileReq = FALSE;
    }
    else
    {
        PRINTF(_T("ERROR: Unknown command!\n"));
        exit(1);
    }


    if(((bValReq == TRUE) || (bFileReq == TRUE)) && (argc < 4))
    {
        PRINTF(_T("ERROR: You have to specify an input/output file or parameter!\n"));
        exit(1);
    }
    
    if(bFileReq == TRUE)
    {
        if (SSCANF(argv[3], _T("%s%c"), &szIOFilename[0], &cTemp) != 1)
        {
            PRINTF(_T("ERROR: You have to specify an input/output file!\n"));
            exit(1);
        }
    }
    else if(bValReq == TRUE)
    {
        if (SSCANF(argv[3], _T("%d%c"), &parameter, &cTemp) != 1)
        {
            PRINTF(_T("ERROR: You have to specify a parameter value!\n"));
            exit(1);
        }
    }

    //
    // Begin command execution.
    //
    if((retVal = CgEpiStart()) != CG_EPIRET_OK)
    {
        if(retVal == CG_EPIRET_INTRF_ERROR)
        {
            if(g_nOperationTarget == OT_ROMFILE)
            {
                PRINTF(_T("ERROR: Failed to access BIOS file!\n"));
            }
            else
            {
                PRINTF(_T("ERROR: Failed to access system interface!\n"));
            }       
        }
        else
        {
            PRINTF(_T("ERROR: Failed to perform module initialisation!\n"));
        }
    }
    else
    {
        exitState = 0;
        switch(command)
        {
        case CMD_SAVE_EPI:
            PRINTF(_T("Saving EPI module..."));
            if ((retVal = CgEpiReadEpiFromMpfa(&szIOFilename[0])) != CG_EPIRET_OK)
            {
                PRINTF(_T("ERROR\n"));
                PRINTF(_T("ERROR: Failed to save EPI module!\n"));
                if(retVal == CG_EPIRET_NOTFOUND)
                {
                    PRINTF(_T("ERROR: EPI module not found!\n"));
                }
                else if(retVal == CG_EPIRET_INTRF_ERROR)
                {
                    PRINTF(_T("ERROR: Failed to access operating target!\n"));
                }
                else if(retVal == CG_EPIRET_ERROR_FILE)
                {
                    PRINTF(_T("ERROR: Failed to access output file!\n"));
                }
                else if(retVal == CG_EPIRET_ERROR)
                {
                    PRINTF(_T("ERROR: Internal processing error!\n"));
                }
                exitState = 1;                
            }
            else
            {
                PRINTF(_T("DONE\n"));
            }
            break;

        case CMD_LOAD_EPI:
            PRINTF(_T("Adding/replacing EPI module..."));
            
            // First check state of BIOS update protection
            if(CgMpfaCheckBUPActive())
            {
                //BUP activate
                PRINTF(_T("FAILED!\n"));
                PRINTF(_T("\nBIOS write protection is active!\n"));
                PRINTF(_T("Please use the BIOS update module (BFLASH) to deactivate the protection.\n"));
                exitState = 1;       
                break;
            }
            else if ((retVal = CgEpiWriteEpiToMpfa(&szIOFilename[0],FALSE)) != CG_EPIRET_OK)
            {
                PRINTF(_T("ERROR\n"));
                PRINTF(_T("ERROR: Failed to add EPI module!\n"));

                if(retVal == CG_EPIRET_INCOMP)
                {
                    PRINTF(_T("ERROR: Module not compatible to selected target!\n"));
                }
                else if((retVal == CG_EPIRET_INV)||(retVal == CG_EPIRET_INV_DATA) ||(retVal == CG_EPIRET_INV_PARM))
                {
                    PRINTF(_T("ERROR: Selected file is not a valid EPI module!\n"));
                }
                else if(retVal == CG_EPIRET_ERROR_FILE)
                {
                    PRINTF(_T("ERROR: Failed to access input file!\n"));
                }
                else if(retVal == CG_EPIRET_ERROR_SIZE)
                {
                    PRINTF(_T("ERROR: Module size exceeds available module storage size!\n"));
                }
                else if(retVal == CG_EPIRET_NOTALLOWED)
                {
                    PRINTF(_T("ERROR: Operation not allowed!\n"));
                }
                else if(retVal == CG_EPIRET_INTRF_ERROR)
                {
                    PRINTF(_T("ERROR: Failed to access operating target!\n"));
                }
                else if(retVal == CG_EPIRET_ERROR)
                {
                    PRINTF(_T("ERROR: Internal processing error!\n"));
                }
                exitState = 1;                
            }
            else
            {
                PRINTF(_T("DONE\n"));
            }
            break;

        case CMD_SAVE_STD:
            PRINTF(_T("Saving standard EPI EDID data set..."));
            if ((retVal = CgEpiReadStdEdidFromMpfa(&szIOFilename[0],nStdEdidNo)) != CG_EPIRET_OK)
            {
                PRINTF(_T("ERROR\n"));
                PRINTF(_T("ERROR: Failed to save standard EPI EDID data set!\n"));

                if(retVal == CG_EPIRET_NOTFOUND)
                {
                    PRINTF(_T("ERROR: Standard EPI EDID data set not found!\n"));
                }
                else if(retVal == CG_EPIRET_INTRF_ERROR)
                {
                    PRINTF(_T("ERROR: Failed to access operating target!\n"));
                }
                else if(retVal == CG_EPIRET_ERROR_FILE)
                {
                    PRINTF(_T("ERROR: Failed to access output file!\n"));
                }
                else if(retVal == CG_EPIRET_ERROR)
                {
                    PRINTF(_T("ERROR: Internal processing error!\n"));
                }
                exitState = 1;                
            }
            else
            {
                PRINTF(_T("DONE\n"));
            }
            break;

        case CMD_SAVE_OEM:
            PRINTF(_T("Saving OEM EDID data set..."));
            if ((retVal = CgEpiReadOEMEdidFromMpfa(&szIOFilename[0],nOEMEdidNo)) != CG_EPIRET_OK)
            {
                PRINTF(_T("ERROR\n"));
                PRINTF(_T("ERROR: Failed to save OEM EDID data set!\n"));

                if(retVal == CG_EPIRET_NOTFOUND)
                {
                    PRINTF(_T("ERROR: OEM EDID data set not found!\n"));
                }
                else if(retVal == CG_EPIRET_INTRF_ERROR)
                {
                    PRINTF(_T("ERROR: Failed to access operating target!\n"));
                }
                else if(retVal == CG_EPIRET_ERROR_FILE)
                {
                    PRINTF(_T("ERROR: Failed to access output file!\n"));
                }
                else if(retVal == CG_EPIRET_ERROR)
                {
                    PRINTF(_T("ERROR: Internal processing error!\n"));
                }
                exitState = 1;                
            }
            else
            {
                PRINTF(_T("DONE\n"));
            }
            break;

        case CMD_DEL_OEM:            
            PRINTF(_T("Removing OEM EDID data set..."));
            
            // First check state of BIOS update protection
            if(CgMpfaCheckBUPActive())
            {
                //BUP activate
                PRINTF(_T("FAILED!\n"));
                PRINTF(_T("\nBIOS write protection is active!\n"));
                PRINTF(_T("Please use the BIOS update module (BFLASH) to deactivate the protection.\n"));
                exitState = 1;       
                break;
            }            
            else if ((retVal = CgEpiDelOEMEdidFromMpfa(nOEMEdidNo,FALSE)) != CG_EPIRET_OK)
            {
                PRINTF(_T("ERROR\n"));
                PRINTF(_T("ERROR: Failed to remove OEM EDID module!\n"));
                
                if(retVal == CG_EPIRET_ERROR_FILE)
                {
                    PRINTF(_T("ERROR: Failed to access input file!\n"));
                }
                else if(retVal == CG_EPIRET_INTRF_ERROR)
                {
                    PRINTF(_T("ERROR: Failed to access operating target!\n"));
                }
                else if(retVal == CG_EPIRET_ERROR_SIZE)
                {
                    PRINTF(_T("ERROR: Resulting module size exceeds available module storage size!\n"));
                }            
                else if(retVal == CG_EPIRET_NOTFOUND)
                {
                    PRINTF(_T("ERROR: Could not find EPI module or OEM EDID entry!\n"));
                }            
                else if(retVal == CG_EPIRET_NOTALLOWED)
                {
                    PRINTF(_T("ERROR: Operation not allowed!\n"));
                }            
                else if(retVal == CG_EPIRET_ERROR)
                {
                    PRINTF(_T("ERROR: Internal processing error!\n"));
                }            
                exitState = 1;                
            }
            else
            {
                PRINTF(_T("DONE\n"));
            }
            break;

        case CMD_LOAD_OEM:
            PRINTF(_T("Adding/replacing OEM EDID data set..."));
            
            // First check state of BIOS update protection
            if(CgMpfaCheckBUPActive())
            {
                //BUP activate
                PRINTF(_T("FAILED!\n"));
                PRINTF(_T("\nBIOS write protection is active!\n"));
                PRINTF(_T("Please use the BIOS update module (BFLASH) to deactivate the protection.\n"));
                exitState = 1;       
                break;
            }            
            else if ((retVal = CgEpiWriteOEMEdidToMpfa(&szIOFilename[0],nOEMEdidNo,FALSE)) != CG_EPIRET_OK)
            {
                PRINTF(_T("ERROR\n"));
                PRINTF(_T("ERROR: Failed to add OEM EDID module!\n"));
                
                if(retVal == CG_EPIRET_INCOMP)
                {
                    PRINTF(_T("ERROR: Data not compatible to selected target!\n"));
                }
                else if(retVal == CG_EPIRET_INV)
                {
                    PRINTF(_T("ERROR: Invalid EDID file specified!\n"));
                }
                else if(retVal == CG_EPIRET_INV_PARM)
                {
                    PRINTF(_T("ERROR: OEM EDID index to high!\n"));
                }                
                else if(retVal == CG_EPIRET_ERROR_FILE)
                {
                    PRINTF(_T("ERROR: Failed to access input file!\n"));
                }
                else if(retVal == CG_EPIRET_INTRF_ERROR)
                {
                    PRINTF(_T("ERROR: Failed to access operating target!\n"));
                }
                else if(retVal == CG_EPIRET_ERROR_SIZE)
                {
                    PRINTF(_T("ERROR: Resulting module size exceeds available module storage size!\n"));
                }            
                else if(retVal == CG_EPIRET_NOTFOUND)
                {
                    PRINTF(_T("ERROR: Could not find EPI module or OEM EDID entry!\n"));
                }            
                else if(retVal == CG_EPIRET_NOTALLOWED)
                {
                    PRINTF(_T("ERROR: Operation not allowed!\n"));
                }            
                else if(retVal == CG_EPIRET_ERROR)
                {
                    PRINTF(_T("ERROR: Internal processing error!\n"));
                }            
                exitState = 1;                
            }
            else
            {
                PRINTF(_T("DONE\n"));
            }
            break;
            
       
        case CMD_SAVE_EEP:
            PRINTF(_T("Saving EPI EEPROM contents to file..."));
            if ((retVal = CgEpiReadEdidFromEEP(&szIOFilename[0])) != CG_EPIRET_OK)
            {
                PRINTF(_T("ERROR\n"));
                PRINTF(_T("ERROR: Failed to save EPI EEPROM contents to file!\n"));

                if(retVal == CG_EPIRET_INTRF_ERROR)
                {
                    PRINTF(_T("ERROR: Failed to read data from EPI EEPROM!\n"));
                }
                else if(retVal == CG_EPIRET_INV)
                {
                    PRINTF(_T("ERROR: EPI EEPROM contents invalid!\n"));
                }
                else if(retVal == CG_EPIRET_ERROR_FILE)
                {
                    PRINTF(_T("ERROR: Failed to access output file!\n"));
                }
                else if(retVal == CG_EPIRET_ERROR)
                {
                    PRINTF(_T("ERROR: Internal processing error!\n"));
                }
                exitState = 1;                
            }
            else
            {
                PRINTF(_T("DONE\n"));
            }
            break;

        case CMD_LOAD_EEP:
            PRINTF(_T("Writing EPI EEPROM data..."));
            if ((retVal = CgEpiWriteEdidToEEP(&szIOFilename[0])) != CG_EPIRET_OK)
            { 
                PRINTF(_T("ERROR\n"));
                PRINTF(_T("ERROR: Failed to write data to EPI EEPROM!\n"));
                if(retVal == CG_EPIRET_INV)
                {
#ifdef CONGA
                    PRINTF(_T("ERROR: Data file is not a valid EDID 1.3/2.0 or congatec panel selection file!\n"));
#else
                    PRINTF(_T("ERROR: Data file is not a valid EDID 1.3/2.0 or panel selection file!\n"));
#endif
                }
                else if(retVal == CG_EPIRET_INTRF_ERROR)
                {
                    PRINTF(_T("ERROR: Failed to access EPI EEPROM!\n"));
                }
                else if(retVal == CG_EPIRET_ERROR_FILE)
                {
                    PRINTF(_T("ERROR: Failed to access input file!\n"));
                }
                else if(retVal == CG_EPIRET_ERROR)
                {
                    PRINTF(_T("ERROR: Internal processing error!\n"));
                }
                exitState = 1;                
            }
            else
            {
                PRINTF(_T("DONE\n"));
            }
            break;

        case CMD_CLEAR_EEP:
            PRINTF(_T("Clear EPI EEPROM..."));
            if ((retVal = CgEpiClearEEP()) != CG_EPIRET_OK)
            { 
                PRINTF(_T("ERROR\n"));
                PRINTF(_T("ERROR: Failed to clear EPI EEPROM!\n"));
                if(retVal == CG_EPIRET_INTRF_ERROR)
                {
                    PRINTF(_T("ERROR: Failed to access EPI EEPROM!\n"));
                }
                exitState = 1;                
            }
            else
            {
                PRINTF(_T("DONE\n"));
            }
            break;

        case CMD_SET_BLVAL:
            PRINTF(_T("Set backlight control value..."));
            if ((retVal = CgEpiSetBLValue(parameter)) != CG_EPIRET_OK)
            { 
                PRINTF(_T("ERROR\n"));
                PRINTF(_T("ERROR: Failed to set backlight control!\n"));
                if(retVal == CG_EPIRET_INV)
                {
                    PRINTF(_T("ERROR: Invalid parameter passed!\n"));
                }
                else if(retVal == CG_EPIRET_INTRF_ERROR)
                {
                    PRINTF(_T("ERROR: Failed to access backlight control device!\n"));
                }
                exitState = 1;                
            }
            else
            {
                PRINTF(_T("DONE\n"));
            }
            break;

        case CMD_GET_BLVAL:
            PRINTF(_T("Current backlight control value :"));
            if ((retVal = CgEpiGetBLValue(&parameter)) != CG_EPIRET_OK)
            { 
                PRINTF(_T("ERROR\n"));
                PRINTF(_T("ERROR: Failed to get current backlight control value!\n"));
                if(retVal == CG_EPIRET_INTRF_ERROR)
                {
                    PRINTF(_T("ERROR: Failed to access backlight control device!\n"));
                }
                exitState = 1;                
            }
            else
            {
                PRINTF(_T("%d\n"), parameter);
            }
            break;

        case CMD_SET_BLSTATE:
            PRINTF(_T("Set backlight enable state..."));
            if ((retVal = CgEpiSetBLEnable(parameter)) != CG_EPIRET_OK)
            { 
                PRINTF(_T("ERROR\n"));
                PRINTF(_T("ERROR: Failed to set backlight enable state!\n"));
                if(retVal == CG_EPIRET_INV)
                {
                    PRINTF(_T("ERROR: Invalid parameter passed!\n"));
                }
                else if(retVal == CG_EPIRET_INTRF_ERROR)
                {
                    PRINTF(_T("ERROR: Failed to access backlight enable control!\n"));
                }
                exitState = 1;                
            }
            else
            {
                PRINTF(_T("DONE\n"));
            }
            break;

        case CMD_GET_BLSTATE:
            PRINTF(_T("Current backlight enable state :"));
            if ((retVal = CgEpiGetBLEnable(&parameter)) != CG_EPIRET_OK)
            { 
                PRINTF(_T("ERROR\n"));
                PRINTF(_T("ERROR: Failed to get current backlight enable state!\n"));
                if(retVal == CG_EPIRET_INTRF_ERROR)
                {
                    PRINTF(_T("ERROR: Failed to access backlight enable control!\n"));
                }
                exitState = 1;                
            }
            else
            {
                PRINTF(_T("%d\n"), parameter);
            }
            break;

        case CMD_DUMP_PL:
            PRINTF(_T("Panel data sets included in BIOS:\n\n"));
            for(nOEMEdidNo = 0; nOEMEdidNo < 8; nOEMEdidNo++)
            {
                if(CgEpiGetEpiDataSetDesc(&szEPIDataSetDesc[0], nOEMEdidNo, TRUE) == CG_EPIRET_OK)
                {
                    PRINTF(_T("%s\n"), &szEPIDataSetDesc[0]);
                    szEPIDataSetDesc[0] = 0x00;
                }
                else
                {
                    break;
                }
            }
            for(nStdEdidNo = 0; nStdEdidNo < 128; nStdEdidNo++)
            {
                if(CgEpiGetEpiDataSetDesc(&szEPIDataSetDesc[0], nStdEdidNo, FALSE) == CG_EPIRET_OK)
                {
                    PRINTF(_T("%s\n"), &szEPIDataSetDesc[0]);
                    szEPIDataSetDesc[0] = 0x00;
                }
                else
                {
                    break;
                }
            }
            break;

        default:
            exitState = 1;
            break;
        }
    }
    if (CgEpiEnd() != CG_EPIRET_OK)
    {
            PRINTF(_T("ERROR: Failed to perform module cleanup!\n"));
            exitState = 1;
    }
    exit(exitState);
}
