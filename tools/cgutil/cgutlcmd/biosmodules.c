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
 * $Header:   S:/CG/archives/CGTOOLS/CGUTIL/W32DOSX/CGUTLCMD/BiosModules.c-arc   1.6   Sep 06 2016 16:28:24   congatec  $
 *
 * Contents: Congatec MPFA modification command line module.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/CGUTIL/W32DOSX/CGUTLCMD/BiosModules.c-arc  $
 * 
 *    Rev 1.6   Sep 06 2016 16:28:24   congatec
 * Added BSD header.
 * MOD001: Corrected usage description, especially for module creation.
 * Display information what type of module has been created and
 * actively confirm success as well not only report error in failure case.
 * 
 *    Rev 1.5   May 31 2012 15:31:56   gartner
 * Updated variable definitions to ease 64bit porting.
 * 
 *    Rev 1.4   Nov 09 2006 12:49:34   gartner
 * Fixed info string about how to disable bios write and update protection.
 * 
 *    Rev 1.3   Oct 30 2006 14:33:28   gartner
 * Added support for build switches. Changed some description strings. Added support for BIOS write protection feature. Adaptions to reduce Linux warnings.
 * 
 *    Rev 1.2   Mar 21 2006 15:06:48   gartner
 * Added support for xxx version. Added module compare function.
 * 
 *    Rev 1.1   Jan 27 2006 12:44:24   gartner
 * Fixed and extended return code handling. Adapted to new parameters of MPFA functions. Avoid error message after update of operating target, which was only due to the unnecessary re-inint of the MPFA.
 * 
 *    Rev 1.0   Oct 04 2005 13:16:30   gartner
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
#include "cgbmod.h"


/*--------------
 * Externs used
 *--------------
 */
extern _TCHAR *g_lpszBiosFilename;


/*--------------------
 * Local definitions
 *--------------------
 */
// Commands
#define CMD_ADD_MOD         0
#define CMD_DEL_MOD         1
#define CMD_SAVE_MOD        2
#define CMD_DISPLAY_LIST    3
#define CMD_SAVE_LIST       4
#define CMD_CREATE_MOD      5
#define CMD_INFO            6
#define CMD_SET_OEM         7
#define CMD_CMP_MOD         8

/*-------------------------
 * Module global variables
 *-------------------------
 */
static _TCHAR szBiosFilename[256], szInpFilename[256], szOutpFilename[256];
static UINT32 command;
static _TCHAR szOemBiosVersion[256] = {0};
   
static CG_MPFA_MODULE_HEADER localMpfaHeader = {CG_MPFA_MOD_HDR_ID,     //hdrID
                                                0,                      //modSize
                                                0,                      //modType
                                                0,                      //modSubType
                                                0xFFFF,                 //modFlags
                                                0xFFFFFFFF,             //modParamFlags
                                                0,                      //modRev
                                                0,                      //modLoadTime
                                                0,                      //modExecTime
                                                0xFF,                   //modParam0
                                                0,                      //modLoadAddr
                                                0,                      //modEntryOff
                                                0xFFFFFFFF,             //modParam1
                                                0xFFFFFFFF,             //modParam2
                                                0,                      //modID
                                                0                       //modChecksum
                                                };    

static CG_MPFA_MODULE_END localMpfaEnd = {CG_MPFA_MOD_END_ID};

/*---------------------------------------------------------------------------
 * Name: ShowUsage
 * Desc: Display parameters for this module.
 * Inp:  none
 * Outp: none
 *---------------------------------------------------------------------------
 */
static void ShowUsage(void)
{
    UINT32 i;

    PRINTF(_T("\nUsage:\n\n"));
    PRINTF(_T("MODULE /OT:[target] /[command] /IF:<input file> /OF:<output file> [parameters]\n\n"));	//MOD001
    PRINTF(_T("/OT:  - Specify the operation target.\n"));
    PRINTF(_T("        BOARD  - All actions result in direct hardware accesses to the\n"));
    PRINTF(_T("                 respective board.\n"));
    PRINTF(_T("        All other strings are interpreted as name of the BIOS file\n"));
    PRINTF(_T("        to be used as operation target.\n"));
    PRINTF(_T("/IF:  - Specify the input data file (depends on command).\n"));      
    PRINTF(_T("/OF:  - Specify the output data file (depends on command).\n"));      

    PRINTF(_T("\nPress ENTER to continue...\n"));
    getch();

    PRINTF(_T("\nCommands:\n\n"));
    PRINTF(_T("/SAVE    - Find specified module and save it to the output data file.\n"));
    PRINTF(_T("/DSAVE   - Find specified module and save it's data block.\n"));
    PRINTF(_T("/CMP     - Find and compare BIOS module with the module input file.\n"));
    PRINTF(_T("/DCMP    - Find specified module and compare module data with\n"));
    PRINTF(_T("           data in input file.\n"));
    PRINTF(_T("/DEL     - Find specified module and delete it.\n"));
    PRINTF(_T("/ADD     - Add a new BIOS module to the operation target.\n"));
    PRINTF(_T("/LIST    - Generate a list of all BIOS modules found on the\n"));
    PRINTF(_T("           operation target and display it.\n"));
    PRINTF(_T("/SLIST   - Generate a list of all BIOS modules found on the\n"));
    PRINTF(_T("           operation target and save it to the output data file.\n"));
    PRINTF(_T("/CREATE  - Create a new BIOS module using input file data\n"));
    PRINTF(_T("           and store the result in the specified output file.\n"));
    PRINTF(_T("/INFO    - Display the OEM BIOS version (if assigned) and information\n"));
    PRINTF(_T("           about the free space left in the module storage area.\n"));
    PRINTF(_T("/OEM:xxx - Assign OEM BIOS version (eight characters max.).\n"));

    PRINTF(_T("\nPress ENTER to continue...\n"));
    getch();

    PRINTF(_T("\nBIOS Module Parameters:\n\n"));
    PRINTF(_T("/T:  - BIOS module type (BYTE). Has to be specified!\n"));
    PRINTF(_T("/ID: - BIOS module ID (WORD). Default: 0\n"));
    PRINTF(_T("/R:  - BIOS module revision (BYTE). Default: 0\n"));
    PRINTF(_T("/LA: - BIOS module physical load address (DWORD, OEM code/data only).Default: 0\n"));
    PRINTF(_T("       Special addresses:\n"));
    PRINTF(_T("       00000000: Load module at any free upper memory block address.\n"));
    PRINTF(_T("       FFFFFFFE: Load module at any free conventional memory address.\n"));
    PRINTF(_T("       FFFFFFFF: Load module at any free extended memory address.\n"));
    PRINTF(_T("/LT: - BIOS module load time (BYTE, OEM code/data only).Default: 0\n"));
    PRINTF(_T("/ET: - BIOS module execution time (BYTE, OEM code/data only).Default: 0\n"));
    PRINTF(_T("/O:  - BIOS module entry offset (DWORD, OEM code/data only).Default: 0\n"));
    
    PRINTF(_T("\nPress ENTER to continue...\n"));
    getch();

    PRINTF(_T("\nBIOS Module Types:\n\n"));
    for(i=0; i < g_nNoMpfaTypes; i++)
    {
        PRINTF("%x", g_MpfaTypeList[i].type);
        PRINTF(_T("\t"));
        PRINTF(_T("%s"), g_MpfaTypeList[i].typeDescription);
        // PRINTF(("%s", g_MpfaTypeList[i].typeShortQualifier));
        PRINTF(_T("\n"));
    }
    
    PRINTF(_T("\nPress ENTER to continue...\n"));
    getch();

    PRINTF(_T("\nExecution/Load Time Selectors:\n\n"));
    for(i=0; i < g_nNoPostHooks; i++)
    {
        PRINTF(_T("%x"), g_MpfaPostHookList[i].hook);
        PRINTF(_T("\t"));
        PRINTF(_T("%s"), g_MpfaPostHookList[i].hookDescription);
        //PRINTF(("%s", g_MpfaPostHookList[i].hookShortQualifier));
        PRINTF(_T("\n"));
    }

    PRINTF(_T("\n\nNOTE 1: All numeric parameters are interpreted as hexadecimals!\n"));
    PRINTF(_T("NOTE 2: When the 'BIOS Update & Write Protection' feature is enabled\n"));
    PRINTF(_T("        in BIOS setup, commands that try to write to the BIOS flash will fail.\n"));    
    PRINTF(_T("        In order to deactivate the BIOS write protection until next boot\n"));
#ifdef CONGA
    PRINTF(_T("        please use: 'CGUTLCMD BFLASH /BP:[password]'.\n"));
#endif
#ifdef COMMON
    PRINTF(_T("        please use: 'CGUTLCMD BFLASH /BP:[password]'.\n"));
#endif
#ifdef BANDR
    PRINTF(_T("        please use: 'XCPBSVC BFLASH /BP:[password]'.\n"));
#endif
}


/*---------------------------------------------------------------------------
 * Name: CgMpfaListModules
 * Desc: Write all valid and used modules to specified output       
 * Inp:  fpOutDatafile - pointer to output file for the module list
 * Outp: return code:
 *       CG_MPFARET_INTRF_ERROR  - Interface access error 
 *       CG_MPFARET_ERROR        - Execution error
 *       CG_MPFARET_OK           - Success
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaListModules
(
    FILE *fpOutDatafile
)
{
    unsigned char *pCurrent;
    UINT32 nTempOffset, nSectionCount;
    CG_MPFA_SECTION_INFO* pSectionInfo;
    
    if(!fpOutDatafile)
    {
        return CG_MPFARET_INTRF_ERROR;
    }

    fprintf(fpOutDatafile,_T("\nBIOS Module Overview\n\n"));
    fprintf(fpOutDatafile,_T("Space available for new modules:0x%X Bytes\n"),(CgMpfaStaticInfo.sectionSize - CgMpfaStaticInfo.addIndex));
    fprintf(fpOutDatafile,_T("\nDetected BIOS Modules:\n\n"));

    if(fpOutDatafile == stdout)
    {
        PRINTF(_T("Press ENTER to continue...\n"));
        getch();
    }

    for(nSectionCount = 0; nSectionCount < g_nNoMpfaSections; nSectionCount++)
    {
        pSectionInfo = g_MpfaSectionList[nSectionCount];
        pCurrent = pSectionInfo->pSectionBuffer;
        
        if((pSectionInfo->sectionType == CG_MPFA_STATIC) || (pSectionInfo->sectionType == CG_MPFA_DYNAMIC))
        {
            do
            {
                if (((CG_MPFA_MODULE_HEADER *)pCurrent)->hdrID != CG_MPFA_MOD_HDR_ID)
                {
                    // No more modules found.
                    break;
                }
                //Module header found, now check for module end structure.
                nTempOffset = ((CG_MPFA_MODULE_HEADER *)pCurrent)->modSize - sizeof(localMpfaEnd);
                if (((CG_MPFA_MODULE_END *)(pCurrent + nTempOffset))->endID != CG_MPFA_MOD_END_ID)
                {
                    break;
                }
                // We have found a valid module entry, now check whether it is USED;
                // if not, we continue with the next module.            
                if ((((CG_MPFA_MODULE_HEADER *)pCurrent)->modFlags & CG_MOD_ENTRY_USED) && (((CG_MPFA_MODULE_HEADER *)pCurrent)->modType != CG_MPFA_TYPE_ROOT))
                {
                    // Save module header to output file.
                    fprintf(fpOutDatafile,_T("\nModule Type: %Xh"), ((CG_MPFA_MODULE_HEADER *)pCurrent)->modType);
                    fprintf(fpOutDatafile,_T("\nModule Sub-Type: %Xh"), ((CG_MPFA_MODULE_HEADER *)pCurrent)->modSubType);
                    fprintf(fpOutDatafile,_T("\nModule ID: %Xh"), ((CG_MPFA_MODULE_HEADER *)pCurrent)->modID);               
                    fprintf(fpOutDatafile,_T("\nModule Flags: %Xh"), ((CG_MPFA_MODULE_HEADER *)pCurrent)->modFlags);
                    fprintf(fpOutDatafile,_T("\nModule Parameter Flags: %Xh"), ((CG_MPFA_MODULE_HEADER *)pCurrent)->modParamFlags);
                    fprintf(fpOutDatafile,_T("\nModule Revision: %Xh"), ((CG_MPFA_MODULE_HEADER *)pCurrent)->modRev);
                    fprintf(fpOutDatafile,_T("\nModule Load Time: %Xh"), ((CG_MPFA_MODULE_HEADER *)pCurrent)->modLoadTime);
                    fprintf(fpOutDatafile,_T("\nModule Execution Time: %Xh"), ((CG_MPFA_MODULE_HEADER *)pCurrent)->modExecTime);
                    fprintf(fpOutDatafile,_T("\nModule Paramter 0: %Xh"), ((CG_MPFA_MODULE_HEADER *)pCurrent)->modParam0);
                    fprintf(fpOutDatafile,_T("\nModule Load Address: %Xh"), ((CG_MPFA_MODULE_HEADER *)pCurrent)->modLoadAddr);
                    fprintf(fpOutDatafile,_T("\nModule Entry Offset: %Xh"), ((CG_MPFA_MODULE_HEADER *)pCurrent)->modEntryOff);
                    fprintf(fpOutDatafile,_T("\nModule Parameter 1: %Xh"), ((CG_MPFA_MODULE_HEADER *)pCurrent)->modParam1);
                    fprintf(fpOutDatafile,_T("\nModule Parameter 2: %Xh"), ((CG_MPFA_MODULE_HEADER *)pCurrent)->modParam2);
                    fprintf(fpOutDatafile,_T("\nModule Data Checksum: %Xh"), ((CG_MPFA_MODULE_HEADER *)pCurrent)->modChkSum);
                    fprintf(fpOutDatafile,_T("\nTotal Module Size: %Xh"), ((CG_MPFA_MODULE_HEADER *)pCurrent)->modSize);
                    fprintf(fpOutDatafile,_T("\n\n"));

                    if(fpOutDatafile == stdout)
                    {
                        PRINTF(_T("\nPress ENTER to continue...\n"));
                        getch();
                    }
                }
                // Set current pointer to next module in list.
                pCurrent = pCurrent + ((CG_MPFA_MODULE_HEADER *)pCurrent)->modSize;
                    
            }while(pCurrent < pSectionInfo->pSectionBuffer + pSectionInfo->sectionSize);
        }
    }
            
    return CG_MPFARET_OK;
}

/*---------------------------------------------------------------------------
 * Name: HandleBiosModules
 * Desc: Main BIOS MPFA module interface handler.
 * Inp:  argc   - Number of command line arguments passed
 *       argv[] - Array of pointers to command line parameters
 * Outp: none       
 *---------------------------------------------------------------------------
 */
void HandleBiosModules(INT32 argc, _TCHAR* argv[])
{
    _TCHAR	cTemp;
    INT32	i, parStart, exitState = 0;
    UINT32	retVal, modType, modRev, modLoadTime, modExecTime,
                        modLoadAddr, modEntryOff, modID, nCompFlags,
                        nSaveFlags, j;											//MOD001
    UINT16	bOutpFileRequired, bInpFileRequired ,
			bModParRequired, bApplyChangeReq,bModTypeFound;						//MOD001
    FILE	*fpOutDatafile;
        
    PRINTF(_T("BIOS Module Modification Module\n"));
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
        else if (STRNCMP(&szBiosFilename[0], _T("NONE"), 4) == 0)
        {
            g_nOperationTarget = OT_NONE;
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
    if (STRNCMP(argv[2], _T("/ADD"), 4) == 0)
	{
        command = CMD_ADD_MOD;
        bInpFileRequired = TRUE;
        bOutpFileRequired = FALSE;
        bModParRequired = FALSE;
    }
    else if (STRNCMP(argv[2], _T("/DEL"),4) == 0)
	{
        command = CMD_DEL_MOD;
        bInpFileRequired = FALSE;
        bOutpFileRequired = FALSE;
        bModParRequired = TRUE;
    }
    else if (STRNCMP(argv[2], _T("/CMP"),4) == 0)
	{
        command = CMD_CMP_MOD;
        bInpFileRequired = TRUE;
        bOutpFileRequired = FALSE;
        bModParRequired = FALSE;
        nCompFlags = CG_MPFACMP_ALL;
    }
    else if (STRNCMP(argv[2], _T("/DCMP"),5) == 0)
	{
        command = CMD_CMP_MOD;
        bInpFileRequired = TRUE;
        bOutpFileRequired = FALSE;
        bModParRequired = TRUE;
        nCompFlags = 0;         // Will be set later on.
    }
    else if (STRNCMP(argv[2], _T("/SAVE"),5) == 0)
	{
        command = CMD_SAVE_MOD;
        nSaveFlags = CG_MPFASFL_MOD;
        bInpFileRequired = FALSE;
        bOutpFileRequired = TRUE;
        bModParRequired = TRUE;
    }
    else if (STRNCMP(argv[2], _T("/DSAVE"),6) == 0)
	{
        command = CMD_SAVE_MOD;
        nSaveFlags = CG_MPFASFL_DATA;
        bInpFileRequired = FALSE;
        bOutpFileRequired = TRUE;
        bModParRequired = TRUE;
    }
    else if (STRNCMP(argv[2], _T("/LIST"),5) == 0)
	{
        command = CMD_DISPLAY_LIST;
        bInpFileRequired = FALSE;
        bOutpFileRequired = FALSE;
        bModParRequired = FALSE;
    }
    else if (STRNCMP(argv[2], _T("/SLIST"),6) == 0)
	{
        command = CMD_SAVE_LIST;
        bInpFileRequired = FALSE;
        bOutpFileRequired = TRUE;
        bModParRequired = FALSE;
    }
    else if (STRNCMP(argv[2], _T("/CREATE"),7) == 0)
	{
        command = CMD_CREATE_MOD;
        bInpFileRequired = TRUE;
        bOutpFileRequired = TRUE;
        bModParRequired = TRUE;
    }
    else if (STRNCMP(argv[2], _T("/INFO"),5) == 0)
	{
        command = CMD_INFO;
        bInpFileRequired = FALSE;
        bOutpFileRequired = FALSE;
        bModParRequired = FALSE;
    }
    else if (STRNCMP(argv[2], _T("/OEM:"),5) == 0)
	{
        command = CMD_SET_OEM;
        bInpFileRequired = FALSE;
        bOutpFileRequired = FALSE;
        bModParRequired = FALSE;
        if ((SSCANF(argv[2], _T("/OEM:%8s%c"), &szOemBiosVersion[0], &cTemp) != 1) &&
            (SSCANF(argv[2], _T("/oem:%8s%c"), &szOemBiosVersion[0], &cTemp) != 1))
	    {
            PRINTF(_T("ERROR: Invalid OEM version specified (min. 1, max. 8 characters)!\n"));          
            exit(1);
        }
    }
    else
    {
        PRINTF(_T("ERROR: Unknown command!\n"));
        exit(1);
    }

    if((g_nOperationTarget == OT_NONE) &&(command != CMD_CREATE_MOD))
    {
        PRINTF(_T("ERROR: Only CREATE command is supported with operation target NONE!\n"));
        exit(1);
    }

    // Minimum set of module parameters
    parStart = 3;

    // Select input/output data file name
    if (bInpFileRequired)
    {
        if(argc < parStart + 1)
        {
            PRINTF(_T("ERROR: You have to specify an input file!\n"));
            exit(1);
        }
        if (STRNCMP(argv[parStart], _T("/IF:"), 4) != 0)
	    {
            PRINTF(_T("ERROR: You have to specify an input file!\n"));
            exit(1);
        }
        if ((SSCANF(argv[parStart], _T("/IF:%s%c"), &szInpFilename[0], &cTemp) != 1) &&
            (SSCANF(argv[parStart], _T("/if:%s%c"), &szInpFilename[0], &cTemp) != 1))
	    {
            PRINTF(_T("ERROR: You have to specify an input file!\n"));
            exit(1);
        }
        parStart = parStart + 1;
    }
    if (bOutpFileRequired)
    {
        if(argc < parStart + 1)
        {
            PRINTF(_T("ERROR: You have to specify an output file!\n"));
            exit(1);
        }
        if (STRNCMP(argv[parStart], _T("/OF:"), 4) != 0)
	    {
            PRINTF(_T("ERROR: You have to specify an output file!\n"));
            exit(1);
        }
        if ((SSCANF(argv[parStart], _T("/OF:%s%c"), &szOutpFilename[0], &cTemp) != 1) &&
            (SSCANF(argv[parStart], _T("/of:%s%c"), &szOutpFilename[0], &cTemp) != 1))
	    {
            PRINTF(_T("ERROR: You have to specify an output file!\n"));
            exit(1);
        }
        parStart = parStart + 1;
    }
    
    if(bModParRequired == TRUE)
    {
        if(argc < parStart + 1)
        {
            PRINTF(_T("ERROR: At least a module type has to be specified!\n"));
            exit(1);
        }
        // Scan for parameters and store them in a MPFA header structure.
        nCompFlags = 0;
        for(i = parStart; i < argc ; i++)
        {
            if (STRNCMP(argv[i], _T("/T:"), 3) == 0)
		    {
                // Set module type parameter.
                if ((SSCANF(argv[i], _T("/T:%x%c"), &modType, &cTemp) != 1) &&
                    (SSCANF(argv[i], _T("/t:%x%c"), &modType, &cTemp) != 1))
			    {
                    PRINTF(_T("ERROR: Type parameter parse error!\n"));
                    exit(1);
                }
                localMpfaHeader.modType = (unsigned char) modType;
                nCompFlags = nCompFlags | CG_MPFACMP_TYPE;
		    }
		    else if (STRNCMP(argv[i], _T("/ID:"), 4) == 0)
		    {
                // Set module ID parameter.
                if ((SSCANF(argv[i], _T("/ID:%x%c"), &modID, &cTemp) != 1) &&
                    (SSCANF(argv[i], _T("/id:%x%c"), &modID, &cTemp) != 1))
			    {
                    PRINTF(_T("ERROR: ID parameter parse error!\n"));
                    exit(1);
                }
                localMpfaHeader.modID = (UINT16)modID;
                nCompFlags = nCompFlags | CG_MPFACMP_ID;
		    }
            else if (STRNCMP(argv[i], _T("/R:"), 3) == 0)
		    {
                // Set module revision parameter.
                if ((SSCANF(argv[i], _T("/R:%x%c"), &modRev, &cTemp) != 1) &&
                    (SSCANF(argv[i], _T("/r:%x%c"), &modRev, &cTemp) != 1))
			    {
                    PRINTF(_T("ERROR: Revision parameter parse error!\n"));
                    exit(1);
                }
                localMpfaHeader.modRev = (unsigned char) modRev;
                nCompFlags = nCompFlags | CG_MPFACMP_REV;
		    }
            else if (STRNCMP(argv[i], _T("/LA:"), 4) == 0)
		    {
                // Set module load address parameter.
                if ((SSCANF(argv[i], _T("/LA:%X%c"), &modLoadAddr, &cTemp) != 1) &&
                    (SSCANF(argv[i], _T("/la:%X%c"), &modLoadAddr, &cTemp) != 1))
			    {
                    PRINTF(_T("ERROR: Load address parameter parse error!\n"));
                    exit(1);
                }
                localMpfaHeader.modLoadAddr = modLoadAddr;
                nCompFlags = nCompFlags | CG_MPFACMP_LOADADDR;
		    }
            else if (STRNCMP(argv[i], _T("/LT:"), 4) == 0)
		    {
                // Set module load time parameter.
                if ((SSCANF(argv[i], _T("/LT:%x%c"), &modLoadTime, &cTemp) != 1) &&
                    (SSCANF(argv[i], _T("/lt:%x%c"), &modLoadTime, &cTemp) != 1))
			    {
                    PRINTF(_T("ERROR: Load time parameter parse error!\n"));
                    exit(1);
                }
                localMpfaHeader.modLoadTime = (unsigned char)modLoadTime;
                nCompFlags = nCompFlags | CG_MPFACMP_LOADTIME;
		    }
            else if (STRNCMP(argv[i], _T("/ET:"), 4) == 0)
		    {
                // Set module execution time parameter.
                if ((SSCANF(argv[i], _T("/ET:%x%c"), &modExecTime, &cTemp) != 1) &&
                    (SSCANF(argv[i], _T("/et:%x%c"), &modExecTime, &cTemp) != 1))
			    {
                    PRINTF(_T("ERROR: Execution time parameter parse error!\n"));
                    exit(1);
                }
                localMpfaHeader.modExecTime = (unsigned char)modExecTime;
                nCompFlags = nCompFlags | CG_MPFACMP_EXECTIME;
		    }
            else if (STRNCMP(argv[i], _T("/O:"), 3) == 0)
		    {
                // Set module entry offset parameter.
                if ((SSCANF(argv[i], _T("/O:%x%c"), &modEntryOff, &cTemp) != 1) &&
                    (SSCANF(argv[i], _T("/o:%x%c"), &modEntryOff, &cTemp) != 1))
			    {
                    PRINTF(_T("ERROR: Entry offset parameter parse error!\n"));
                    exit(1);
                }
                localMpfaHeader.modEntryOff = modEntryOff;
                nCompFlags = nCompFlags | CG_MPFACMP_ENTRYOFF;
		    }
            else
            {
                PRINTF(_T("ERROR: Unknown parameter!\n"));
                exit(1);
            }
        }
    }

    // Ensure that at least a module type has been specified.
    if((localMpfaHeader.modType == 0) && (bModParRequired == TRUE))
    {
        PRINTF(_T("ERROR: You have to specify a module type!\n"));
        exit(1);
    }

    //
    // Begin command execution.
    //
    if((retVal = CgMpfaStart(FALSE)) != CG_MPFARET_OK)
    {
        if(retVal == CG_MPFARET_INTRF_ERROR)
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
        bApplyChangeReq = FALSE;
        switch(command)
        {
        case CMD_ADD_MOD:
            PRINTF(_T("Adding module..."));
            if ((retVal = CgMpfaAddModule(&szInpFilename[0],g_nAccessLevel,FALSE)) != CG_MPFARET_OK)
            {
                PRINTF(_T("ERROR\n"));
                PRINTF(_T("ERROR: Failed to add selected module!\n"));
                if(retVal == CG_MPFARET_INCOMP)
                {
                    PRINTF(_T("ERROR: Module not compatible to selected target!\n"));
                }
                else if(retVal == CG_MPFARET_INV)
                {
                    PRINTF(_T("ERROR: Invalid module file specified!\n"));
                }
                else if(retVal == CG_MPFARET_INV_DATA)
                {
                    PRINTF(_T("ERROR: Invalid module data!\n"));
                }
                else if(retVal == CG_MPFARET_INV_PARM)
                {
                    PRINTF(_T("ERROR: Invalid module parameters!\n"));
                }
                else if(retVal == CG_MPFARET_ERROR_FILE)
                {
                    PRINTF(_T("ERROR: Failed to access input file!\n"));
                }
                else if(retVal == CG_MPFARET_ERROR_SIZE)
                {
                    PRINTF(_T("ERROR: Module size exceeds available module storage size!\n"));
                }
                else if(retVal == CG_MPFARET_NOTALLOWED)
                {
                    PRINTF(_T("ERROR: Operation not allowed!\n"));
                }
                else if(retVal == CG_MPFARET_INTRF_ERROR)
                {
                    PRINTF(_T("ERROR: Failed to access operating target!\n"));
                }
                else if(retVal == CG_MPFARET_ERROR)
                {
                    PRINTF(_T("ERROR: Internal processing error!\n"));
                }
                exitState = 1;
            }
            else
            {
                bApplyChangeReq = TRUE;
                PRINTF(_T("DONE\n"));
            }
            break;
        case CMD_DEL_MOD:
            PRINTF(_T("Deleting module..."));
            if ((retVal = CgMpfaDelModule(&localMpfaHeader, nCompFlags, g_nAccessLevel)) != CG_MPFARET_OK)
            {
                PRINTF(_T("ERROR\n"));
                PRINTF(_T("ERROR: Failed to delete selected module!\n"));
                if(retVal == CG_MPFARET_NOTFOUND)
                {
                    PRINTF(_T("ERROR: Specified module not found!\n"));
                }
                else if(retVal == CG_MPFARET_NOTALLOWED)
                {
                    PRINTF(_T("ERROR: Operation not allowed!\n"));
                }            
                else
                {
                    PRINTF(_T("ERROR: Internal processing error!\n"));
                }
                exitState = 1;
            }
            else
            {
                bApplyChangeReq = TRUE;
                PRINTF(_T("DONE\n"));
            }
            break;

        case CMD_CMP_MOD:
            PRINTF(_T("Comparing module..."));
            if ((retVal = CgMpfaCmpModule(&szInpFilename[0], &localMpfaHeader, nCompFlags, g_nAccessLevel)) != CG_MPFARET_OK)
            {
                PRINTF(_T("ERROR\n"));
                PRINTF(_T("ERROR: Module compare failed!\n"));
                if(retVal == CG_MPFARET_NOTFOUND)
                {
                    PRINTF(_T("ERROR: Specified module not found!\n"));
                }             
                else if(retVal == CG_MPFARET_NOMATCH)
                {
                    PRINTF(_T("ERROR: Module data does not match!\n"));
                }
                else if(retVal == CG_MPFARET_INV)
                {
                    PRINTF(_T("ERROR: Invalid module file specified!\n"));
                }
                else if(retVal == CG_MPFARET_INV_DATA)
                {
                    PRINTF(_T("ERROR: Invalid module data!\n"));
                }
                else if(retVal == CG_MPFARET_INV_PARM)
                {
                    PRINTF(_T("ERROR: Invalid module parameters!\n"));
                }
                else if(retVal == CG_MPFARET_ERROR_FILE)
                {
                    PRINTF(_T("ERROR: Failed to access input file!\n"));
                }
                else if(retVal == CG_MPFARET_ERROR_SIZE)
                {
                    PRINTF(_T("ERROR: Module size exceeds available module storage size!\n"));
                }
                else if(retVal == CG_MPFARET_NOTALLOWED)
                {
                    PRINTF(_T("ERROR: Operation not allowed!\n"));
                }
                else if(retVal == CG_MPFARET_INTRF_ERROR)
                {
                    PRINTF(_T("ERROR: Failed to access operating target!\n"));
                }
                else if(retVal == CG_MPFARET_ERROR)
                {
                    PRINTF(_T("ERROR: Internal processing error!\n"));
                }
                exitState = 1;
            }
            else
            {
                bApplyChangeReq = FALSE;
                PRINTF(_T("DONE\n"));
            }
            break;

        case CMD_SAVE_MOD:
            PRINTF(_T("Saving module..."));
            if ((retVal = CgMpfaSaveModule(&localMpfaHeader, &szOutpFilename[0],nCompFlags,nSaveFlags)) != CG_MPFARET_OK)
            {
                PRINTF(_T("ERROR\n"));
                PRINTF(_T("ERROR: Failed to save selected module!\n"));
                if(retVal == CG_MPFARET_NOTFOUND)
                {
                    PRINTF(_T("ERROR: Specified module not found!\n"));
                }
                else if(retVal == CG_MPFARET_INTRF_ERROR)
                {
                    PRINTF(_T("ERROR: Failed to access operating target!\n"));
                }
                else if(retVal == CG_MPFARET_ERROR_FILE)
                {
                    PRINTF(_T("ERROR: Failed to access output file!\n"));
                }
                else if(retVal == CG_MPFARET_ERROR)
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

        case CMD_SAVE_LIST:
            // Open output data file
            if (!(fpOutDatafile = fopen(&szOutpFilename[0], "wt")))
            {
                PRINTF(_T("ERROR: Failed to open output file!\n"));
                break;
            }
            CgMpfaListModules(fpOutDatafile);
            fclose(fpOutDatafile);
            break;

        case CMD_DISPLAY_LIST:
            CgMpfaListModules(stdout);
            break;

        case CMD_INFO:
            if (CgMpfaGetOEMBiosVersion(&szOemBiosVersion[0]) == CG_MPFARET_OK)
            {
                PRINTF(_T("OEM BIOS version                  : %s\n"), &szOemBiosVersion[0]);
            }
            PRINTF(_T("Space available for new modules   : 0x%X bytes\n"), (CgMpfaStaticInfo.sectionSize - CgMpfaStaticInfo.addIndex));
            break;

        case CMD_CREATE_MOD:
			//MOD001 v
			// Get type description string for module beeing created.
			// Also check immediately for supported module type.
			bModTypeFound = FALSE;
			PRINTF(_T("Creating module "));	
			for(j=0; j < g_nNoMpfaTypes; j++)
			{
				if (g_MpfaTypeList[j].type == localMpfaHeader.modType)
				{
					PRINTF(_T("%s"), g_MpfaTypeList[j].typeDescription);
					bModTypeFound = TRUE;
				}
			}
			PRINTF(_T("..."));
			if(bModTypeFound == FALSE)
			{				
				PRINTF(_T("ERROR\n"));									
                PRINTF(_T("ERROR: Unknown module type selected!\n"));
				exitState = 1;
				break;
			}
			//MOD001 ^
            if ((retVal = CgMpfaCreateModule(&localMpfaHeader, &szInpFilename[0], &szOutpFilename[0], g_nAccessLevel,FALSE)) != CG_MPFARET_OK)
            {
				PRINTF(_T("ERROR\n"));											//MOD001
                PRINTF(_T("ERROR: Failed to create selected module!\n"));
                if(retVal == CG_MPFARET_INCOMP)
                {
                    PRINTF(_T("ERROR: Module not compatible to selected target!\n"));
                }
                else if(retVal == CG_MPFARET_INV)
                {
                    PRINTF(_T("ERROR: Invalid module file specified!\n"));
                }
                else if(retVal == CG_MPFARET_INV_DATA)
                {
                    PRINTF(_T("ERROR: Invalid module data!\n"));
                }
                else if(retVal == CG_MPFARET_INV_PARM)
                {
                    PRINTF(_T("ERROR: Invalid module parameters!\n"));
                }
                else if(retVal == CG_MPFARET_ERROR_FILE)
                {
                    PRINTF(_T("ERROR: Failed to access input/output file!\n"));
                }
                else if(retVal == CG_MPFARET_NOTALLOWED)
                {
                    PRINTF(_T("ERROR: Operation not allowed!\n"));
                }
                else if(retVal == CG_MPFARET_ERROR)
                {
                    PRINTF(_T("ERROR: Internal processing error!\n"));
                }
                exitState = 1;
            }
			else																//MOD001 v
            {
                PRINTF(_T("DONE\n"));
            }																	//MOD001 ^ 
            break;
        
        case CMD_SET_OEM:
            PRINTF(_T("Assigning OEM version..."));
            if ((retVal = CgMpfaSetOEMBiosVersion(&szOemBiosVersion[0])) != CG_MPFARET_OK)
            {
                PRINTF(_T("ERROR\n"));
                PRINTF(_T("ERROR: Failed to assign OEM version!\n"));
                exitState = 1;
            }
            else
            {
                bApplyChangeReq = TRUE;
                PRINTF(_T("DONE\n"));
            }
            break;

        default:
            exitState = 1;
            break;
        }
        if(bApplyChangeReq == TRUE)
        {
            PRINTF(_T("Applying changes to operation target..."));
            // First check whether BIOS write protection is enabled
            if(CgMpfaCheckBUPActive())
            {
                //BUP activate
                PRINTF(_T("FAILED!\n"));
                PRINTF(_T("\nBIOS write protection is active!\n"));
                PRINTF(_T("Please use the BIOS update module (BFLASH) to deactivate the protection.\n"));
                exitState = 1;       
            }
            else if (CgMpfaApplyChanges(FALSE) != CG_MPFARET_OK)
            {
                PRINTF(_T("FAILED!\n"));
                exitState = 1;
            }
            else
            {
                PRINTF(_T("DONE!\n"));
            }
        }
    }
    if (CgMpfaEnd() != CG_MPFARET_OK)
    {
            PRINTF(_T("ERROR: Failed to perform module cleanup!\n"));
            exitState = 1;
    }
    exit(exitState);
}

