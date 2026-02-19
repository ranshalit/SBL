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
 * $Header:   S:/CG/archives/CGTOOLS/CGUTIL/W32DOSX/CGUTLCMD/boardinfo.c-arc   1.2   Sep 06 2016 16:41:04   congatec  $
 *
 * Contents: Congatec board/BIOS information command line module.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/CGUTIL/W32DOSX/CGUTLCMD/boardinfo.c-arc  $
 * 
 *    Rev 1.2   Sep 06 2016 16:41:04   congatec
 * Added BSD header.
 * MOD001:
 * Added option to CGINFO module that allows to compare a given BC firmware file with the currently programmed BC firmware off a module and thus e.g. simplify the check whether a BC firmware update is required or not.
 * 
 *    Rev 1.1   May 31 2012 15:33:16   gartner
 * Updated variable definitions to ease 64bit porting.
 * 
 *    Rev 1.0   Oct 30 2006 14:35:38   gartner
 * Initial revision.
 * 
 *---------------------------------------------------------------------------
 */

/*---------------
 * Include files
 *---------------
 */
#include "cgutlcmn.h"
#include "cginfo.h"

/*--------------
 * Externs used
 *--------------
 */

// Commands
#define CMD_INFO_DUMP       0
#define CMD_INFO_SAVE       1
#define CMD_INFO_CHECK      2
#define CMD_INFO_FWCHECK	3													//MOD001


/*-------------------------
 * Module global variables
 *-------------------------
 */
static _TCHAR szBiosFilename[256], szIOFilename[256];
static UINT32 parameter; 
static UINT32 command;
static UINT16 bValReq = FALSE;
static UINT16 bFileReq = FALSE;
static UINT16 bSilent = FALSE;
static CG_INFO_STRUCT CgInfoStruct1 = {{0xFF}};
static CG_INFO_STRUCT CgInfoStruct2 = {{0xFF}};
static FILE *fpOutDatafile = NULL;
static FILE *fpInDatafile = NULL;												//MOD001
static UINT32 infoFlags = 0;



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
#ifdef BANDR
    PRINTF(_T("CBINFO /OT:[target] /[command] [parm] [opts...]\n\n"));
#else
    PRINTF(_T("CGINFO /OT:[target] /[command] [parm] [opts...]\n\n"));
#endif
    PRINTF(_T("/OT:   - Specify the operation target.\n"));
    PRINTF(_T("         BOARD  - All actions result in direct hardware accesses to the\n"));
    PRINTF(_T("                  respective board.\n"));
    PRINTF(_T("         All other strings are interpreted as name of the BIOS file\n"));
    PRINTF(_T("         to be used as operation target.\n"));
    PRINTF(_T("[parm] - Command parameter (if required). E.g. input/output file name.\n"));
    PRINTF(_T("[opts] - Information selection option(s). One or more options to select\n"));
    PRINTF(_T("         the information that should be returned by the DUMP or SAVE command.\n"));
    
    PRINTF(_T("\nPress ENTER to continue...\n"));
    getch();

    PRINTF(_T("\nCommands:\n\n"));
    PRINTF(_T("/DUMP    - Dump information on screen.\n"));
    PRINTF(_T("/SAVE    - Save information to file.\n"));
    PRINTF(_T("/BFCHK   - Check whether operating target BIOS version matches the\n"));
    PRINTF(_T("           version of the specified BIOS file.\n"));
#ifdef BANDR																	//MOD001 v
	PRINTF(_T("/FWCHK   - Check whether operating target CPU-board controller firmware\n"));
    PRINTF(_T("           version matches the version of the specified BIOS file.\n"));					
#else
	PRINTF(_T("/FWCHK   - Check whether operating target board controller firmware version\n"));
    PRINTF(_T("           matches the version of the specified firmware file.\n"));					
#endif																			//MOD001 ^
    PRINTF(_T("\nInformation Selection Options:\n\n"));
    PRINTF(_T("/SILENT  - Information displayed/saved without description text.\n"));
    PRINTF(_T("/BIOS    - Return system and OEM BIOS version.\n"));
#ifdef BANDR
    PRINTF(_T("/BCFW    - Return CPU-board controller firmware version.\n"));
#else
    PRINTF(_T("/BCFW    - Return board controller firmware version.\n"));
    PRINTF(_T("/CGOS    - Return CGOS interface/driver version.\n"));
#endif    
    PRINTF(_T("/BCNT    - Return boot counter.\n"));
    PRINTF(_T("/RTIM    - Return running time.\n"));
    PRINTF(_T("/MANU    - Return CPU-board manufacturing data.\n"));    
    PRINTF(_T("/BUP     - Return BIOS update/write protection state.\n"));    
    PRINTF(_T("\nIf no selection options are passed, every available information \n"));    
    PRINTF(_T("will be displayed or saved.\n"));    
}

/*---------------------------------------------------------------------------
 * Name: WriteInfo
 * Desc: Displays selected information on screen or writes them to file.
 * Inp:  none
 * Outp: none
 *---------------------------------------------------------------------------
 */
static void WriteInfo(void)
{
    if(!fpOutDatafile)
    {
        return;
    }

    if(bSilent == TRUE)
    {
        // Pure data information
        if(infoFlags & CG_FLAG_INFO_BIOS)
        {
            fprintf(fpOutDatafile,_T("%s\n"), &(CgInfoStruct1.BaseBiosVersion[0]));
            fprintf(fpOutDatafile,_T("%s\n"), &(CgInfoStruct1.OEMBiosVersion[0]));        
        }
        if(g_nOperationTarget == OT_BOARD)
        {
            // Following information only available in board mode
            if(infoFlags & CG_FLAG_INFO_CGBC)
            {
                fprintf(fpOutDatafile,_T("%s\n"), &(CgInfoStruct1.FirmwareVersion[0]));
            }                        
            if(infoFlags & CG_FLAG_INFO_MANU)
            {
                fprintf(fpOutDatafile,_T("%s\n"), CgInfoStruct1.CgosBoardInfo.szBoard);
                fprintf(fpOutDatafile,_T("%s\n"), CgInfoStruct1.CgosBoardInfo.szBoardSub);
                fprintf(fpOutDatafile,_T("%c.%c\n"), CgInfoStruct1.CgosBoardInfo.wProductRevision>>8,
                                                                        CgInfoStruct1.CgosBoardInfo.wProductRevision & 0xFF);         
                fprintf(fpOutDatafile,_T("%s\n"), CgInfoStruct1.CgosBoardInfo.szPartNumber);
                fprintf(fpOutDatafile,_T("%s\n"), CgInfoStruct1.CgosBoardInfo.szEAN);
                fprintf(fpOutDatafile,_T("%s\n"), CgInfoStruct1.CgosBoardInfo.szSerialNumber);            
                fprintf(fpOutDatafile,_T("%04d.%02d.%02d\n"), CgInfoStruct1.CgosBoardInfo.stManufacturingDate.wYear,
                                                                        CgInfoStruct1.CgosBoardInfo.stManufacturingDate.wMonth,
                                                                        CgInfoStruct1.CgosBoardInfo.stManufacturingDate.wDay);  
                fprintf(fpOutDatafile,_T("%04d.%02d.%02d\n"), CgInfoStruct1.CgosBoardInfo.stLastRepairDate.wYear,
                                                                        CgInfoStruct1.CgosBoardInfo.stLastRepairDate.wMonth,
                                                                        CgInfoStruct1.CgosBoardInfo.stLastRepairDate.wDay);  
            }
            if(infoFlags & CG_FLAG_INFO_BCNT)
            {
                fprintf(fpOutDatafile,_T("%d\n"), CgInfoStruct1.BootCount);
            }
            if(infoFlags & CG_FLAG_INFO_RTIM)
            {
                fprintf(fpOutDatafile,_T("%d\n"), CgInfoStruct1.RunningTime);
            }
#ifndef BANDR
            if(infoFlags & CG_FLAG_INFO_CGOS)
            {
                fprintf(fpOutDatafile,_T("0x%08X\n"), CgInfoStruct1.CgosAPIVersion);
                fprintf(fpOutDatafile,_T("0x%08X\n"), CgInfoStruct1.CgosDrvVersion);        
            }
#endif
            if(infoFlags & CG_FLAG_INFO_BUP)
            {
                /* Also store state of the BIOS update protection */
                if(CgInfoStruct1.BupState)
                {
                    fprintf(fpOutDatafile,_T("Active\n"));        
                }
                else
                {
                    fprintf(fpOutDatafile,_T("Inactive\n"));                        
                }
            }
        }
    }
    else
    {
        // Human readable addon information
        if(infoFlags & CG_FLAG_INFO_BIOS)
        {
            fprintf(fpOutDatafile,_T("System BIOS Version:        %s\n"), &(CgInfoStruct1.BaseBiosVersion[0]));
            fprintf(fpOutDatafile,_T("OEM BIOS Version:           %s\n"), &(CgInfoStruct1.OEMBiosVersion[0]));        
        }
        if(g_nOperationTarget == OT_BOARD)
        {
            // Following information only available in board mode
            if(infoFlags & CG_FLAG_INFO_CGBC)
            {
                fprintf(fpOutDatafile,_T("BC Firmware Version:        %s\n"), &(CgInfoStruct1.FirmwareVersion[0]));
            }                        
            if(infoFlags & CG_FLAG_INFO_MANU)
            {
                fprintf(fpOutDatafile,_T("Board Name:                 %s\n"), CgInfoStruct1.CgosBoardInfo.szBoard);
                fprintf(fpOutDatafile,_T("Board Sub Name:             %s\n"), CgInfoStruct1.CgosBoardInfo.szBoardSub);
                fprintf(fpOutDatafile,_T("Product Revision:           %c.%c (0x%04X)\n"), CgInfoStruct1.CgosBoardInfo.wProductRevision>>8,
                                                                        CgInfoStruct1.CgosBoardInfo.wProductRevision & 0xFF,
                                                                        CgInfoStruct1.CgosBoardInfo.wProductRevision);         
                fprintf(fpOutDatafile,_T("Part Number:                %s\n"), CgInfoStruct1.CgosBoardInfo.szPartNumber);
                fprintf(fpOutDatafile,_T("EAN Code:                   %s\n"), CgInfoStruct1.CgosBoardInfo.szEAN);
                fprintf(fpOutDatafile,_T("Serial Number:              %s\n"), CgInfoStruct1.CgosBoardInfo.szSerialNumber);            
                fprintf(fpOutDatafile,_T("Manufacturing Date:         %04d.%02d.%02d\n"), CgInfoStruct1.CgosBoardInfo.stManufacturingDate.wYear,
                                                                        CgInfoStruct1.CgosBoardInfo.stManufacturingDate.wMonth,
                                                                        CgInfoStruct1.CgosBoardInfo.stManufacturingDate.wDay);  
                fprintf(fpOutDatafile,_T("Last Repair Date:           %04d.%02d.%02d\n"), CgInfoStruct1.CgosBoardInfo.stLastRepairDate.wYear,
                                                                        CgInfoStruct1.CgosBoardInfo.stLastRepairDate.wMonth,
                                                                        CgInfoStruct1.CgosBoardInfo.stLastRepairDate.wDay);  
            }
            if(infoFlags & CG_FLAG_INFO_BCNT)
            {
                fprintf(fpOutDatafile,_T("Boot Counter:               %d\n"), CgInfoStruct1.BootCount);
            }
            if(infoFlags & CG_FLAG_INFO_RTIM)
            {
                fprintf(fpOutDatafile,_T("Running Time:               %d hours\n"), CgInfoStruct1.RunningTime);
            }
#ifndef BANDR
            if(infoFlags & CG_FLAG_INFO_CGOS)
            {
                fprintf(fpOutDatafile,_T("CGOS API Version:           0x%08X\n"), CgInfoStruct1.CgosAPIVersion);
                fprintf(fpOutDatafile,_T("CGOS Driver Version:        0x%08X\n"), CgInfoStruct1.CgosDrvVersion);        
            }
#endif
            if(infoFlags & CG_FLAG_INFO_BUP)
            {
                /* Also store state of the BIOS update protection */
                if(CgInfoStruct1.BupState)
                {
                    fprintf(fpOutDatafile,_T("BIOS Write Protection:      Active\n"));        
                }
                else
                {
                    fprintf(fpOutDatafile,_T("BIOS Write Protection:      Inactive\n"));                        
                }
            }
        }
    }
    return;
}


/*---------------------------------------------------------------------------
 * Name:
 * Desc:
 * Inp:         
 * Outp:        
 *---------------------------------------------------------------------------
 */
void HandleInfo(INT32 argc, _TCHAR* argv[])
{
    _TCHAR cTemp;
    INT32 exitState = 0;
    UINT32 retVal;
    UINT16 i, optStart;
	unsigned char FirmwareVersion[9];											//MOD001
#ifdef BANDR
    PRINTF(_T("CPU-Board/BIOS Information Module\n"));
#else
    PRINTF(_T("Board/BIOS Information Module\n"));
#endif
    PRINTF(_T("\n"));
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
    if (STRNCMP(argv[2], _T("/DUMP"), 5) == 0)
	{
        command = CMD_INFO_DUMP;
        bValReq = FALSE;
        bFileReq = FALSE;
    }
    else if (STRNCMP(argv[2], _T("/SAVE"),5) == 0)
	{
        command = CMD_INFO_SAVE;
        bValReq = FALSE;
        bFileReq = TRUE;
    }
    else if (STRNCMP(argv[2], _T("/BFCHK"),6) == 0) 
	{
        command = CMD_INFO_CHECK;
        bValReq = FALSE;
        bFileReq = TRUE;
    }
	else if (STRNCMP(argv[2], _T("/FWCHK"),6) == 0)		//MOD001 v
	{
        command = CMD_INFO_FWCHECK;
        bValReq = FALSE;
        bFileReq = TRUE;
    }													//MOD001 ^
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
        optStart = 4;
        if (SSCANF(argv[3], _T("%s%c"), &szIOFilename[0], &cTemp) != 1)
        {
            PRINTF(_T("ERROR: You have to specify an input/output file!\n"));
            exit(1);
        }
    }
    else if(bValReq == TRUE)
    {
        optStart = 4;
        if (SSCANF(argv[3], _T("%d%c"), &parameter, &cTemp) != 1)
        {
            PRINTF(_T("ERROR: You have to specify a parameter value!\n"));
            exit(1);
        }
    }
    else
    {
        optStart = 3;
    }

    // By default no information is selected
    infoFlags = 0x00000000;
    

    // Get information selection options
    for(i=optStart; i < argc; i++)
    {
        if (STRNCMP(argv[i], _T("/SILENT"),7) == 0)
        {
            bSilent = TRUE;
        }
        else if (STRNCMP(argv[i], _T("/BIOS"),5) == 0)
        {
            infoFlags = infoFlags | CG_FLAG_INFO_BIOS; 
        }
        else if (STRNCMP(argv[i], _T("/BCFW"),5) == 0)
        {
            infoFlags = infoFlags | CG_FLAG_INFO_CGBC; 
        }
        else if (STRNCMP(argv[i], _T("/CGOS"),5) == 0)
        {
            infoFlags = infoFlags | CG_FLAG_INFO_CGOS; 
        }
        else if (STRNCMP(argv[i], _T("/BCNT"),5) == 0)
        {
            infoFlags = infoFlags | CG_FLAG_INFO_BCNT; 
        }
        else if (STRNCMP(argv[i], _T("/RTIM"),5) == 0)
        {
            infoFlags = infoFlags | CG_FLAG_INFO_RTIM; 
        }
        else if (STRNCMP(argv[i], _T("/MANU"),5) == 0)
        {
            infoFlags = infoFlags | CG_FLAG_INFO_MANU; 
        }
        else if (STRNCMP(argv[i], _T("/BUP"),5) == 0)
        {
            infoFlags = infoFlags | CG_FLAG_INFO_BUP; 
        }        
    }

    // If no special information is selected everything is returned
    if(infoFlags == 0x00000000)
    {
        infoFlags = 0xFFFFFFFF;
    }


    //
    // Begin command execution.
    //    
    exitState = 0;
    if ((retVal =  CgInfoGetInfo(&CgInfoStruct1, 0xFFFFFFFF)) != CG_RET_OK)
    {
        PRINTF(_T("ERROR: Failed to get system information!\n"));
        exit(1);
    }
    
    switch(command)
    {

    case CMD_INFO_DUMP:
        fpOutDatafile = stdout;
        WriteInfo();
        break;        

    case CMD_INFO_SAVE:
        PRINTF(_T("Write information to file..."));
        if (!(fpOutDatafile = fopen(&szIOFilename[0], "wt")))
        {
            PRINTF(_T("\nERROR: Failed to open output file!\n"));
            break;
        }
        WriteInfo();
        fclose(fpOutDatafile);
        PRINTF(_T("DONE\n"));
        break;        
    
    case CMD_INFO_CHECK:
        g_lpszBiosFilename = &szIOFilename[0];
        g_nOperationTarget = OT_ROMFILE;

        PRINTF(_T("Check system and OEM BIOS versions...\n"));
        if ((retVal =  CgInfoGetInfo(&CgInfoStruct2, infoFlags)) != CG_RET_OK)
        {
            PRINTF(_T("ERROR: Failed to get system information!\n"));
            exitState = 1;
            break;
        }
        for(i=0; i < 8; i++)
        {
            if(CgInfoStruct2.BaseBiosVersion[i] != CgInfoStruct1.BaseBiosVersion[i])
            {
                PRINTF(_T("ERROR: System BIOS versions do not match!\n"));
                exit(1);
            }
        }
        for(i=0; i < 8; i++)
        {
            if(CgInfoStruct2.OEMBiosVersion[i] != CgInfoStruct1.OEMBiosVersion[i])
            {
                PRINTF(_T("ERROR: OEM BIOS versions do not match!\n"));
                exit(1);
            }
        }
        PRINTF(_T("SUCCESS: System and OEM BIOS versions match.\n"));
        break;  

	case CMD_INFO_FWCHECK:														//MOD001 v
		// Read firmware information from firmware file.
		// First line in each firmware file looks like this:
		// ;CGBCP424.DAT CID 21
		PRINTF(_T("Check board controller firmware version...\n"));

		if (!(fpInDatafile = fopen(&szIOFilename[0], "rb")))
        {
            PRINTF(_T("\nERROR: Failed to open firmware file!\n"));
            exitState = 1;
            break;
        }

		fread(&FirmwareVersion[0], 1, 9, fpInDatafile );
		if( ferror( fpInDatafile ) )      
		{			
			PRINTF(_T("\nERROR: Failed to read firmware file!\n"));
			fclose(fpInDatafile);
            exitState = 1;
            break;
		}        

        for(i=0; i < 3; i++)
        {
#ifndef BANDR
			if(CgInfoStruct1.FirmwareVersion[i+5] != FirmwareVersion[i+6])
#else
			if(CgInfoStruct1.FirmwareVersion[i+1] != FirmwareVersion[i+6])
#endif
            {
                PRINTF(_T("ERROR: Board controller firmware version does not match!\n"));
                fclose(fpInDatafile);
				exit(1);
            }
        }
        PRINTF(_T("SUCCESS: Board controller firmware version matches.\n"));
		fclose(fpInDatafile);
        break;																	//MOD001 ^

    default:
        exitState = 1;
        break;
    }

    exit(exitState);
}
