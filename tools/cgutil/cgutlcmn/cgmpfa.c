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
 * $Header:   S:/CG/archives/CGTOOLS/CGUTIL/CGUTLCMN/CGMPFA.C-arc   1.18   Sep 07 2016 12:29:38   congatec  $
 *
 * Contents: Congatec MPFA modification common implementation module.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/CGUTIL/CGUTLCMN/CGMPFA.C-arc  $
 * 
 *    Rev 1.18   Sep 07 2016 12:29:38   congatec
 * Fix type conversion warnings.
 * 
 *    Rev 1.17   Sep 06 2016 16:06:06   congatec
 * Added BSD header.
 * MOD008:
 * Added support for SMBIOS/DMI MPFA module and new setup menu layout and string control MPFA module.
 * Ensure correct FV module data alignment for 64byte to 32kByte alignment requirements.
 * 
 *    Rev 1.16   Aug 06 2015 16:59:14   gartner
 * MOD007: Corrected padding.
 * 
 *    Rev 1.15   Aug 06 2015 13:27:38   gartner
 * MOD006: Ensure 8 byte alignment of OEM UEFI FV MPFA modules.
 * 
 *    Rev 1.14   Nov 21 2014 16:34:06   gartner
 * MOD005: Added support for new MPFA modules: 
 * POST Logo Left (standard small POST logo in uper left corner)
 * POST Logo Right (addon small POST logo in upper right corner)
 * UEFI FW Volume (UEFI Firmware Volume with UEFI DXE drivers and or UEFI bootloaders)
 * OA 3.0 Module (OEM activation 3.0 module)   
 * 
 *    Rev 1.13   Apr 24 2013 18:41:38   gartner
 * MOD004: Allow INTERN CGUTIL version to replace the Standard Video BIOS module as well.
 * 
 *    Rev 1.12   May 31 2012 15:47:58   gartner
 * MOD003:  Changed settings/backup module compare algorithm to support new UEFI settings module hanlding without causing a diff-mismatch each time. Important for manufacturing / tester !
 * 
 *    Rev 1.11   Jun 22 2011 13:52:06   gartner
 * MOD002: Do not allow to add Standard Video BIOS modules.
 * Added verification and retry handling to module flash updates.
 * 
 *    Rev 1.10   Feb 07 2011 18:03:12   gartner
 * Do not force logo module check.
 * 
 *    Rev 1.9   Dec 06 2010 12:05:48   gartner
 * MOD001: Added support for OEM SLP 1.0, OEM and standard VBIOS modules and extended full flash file patch capabilities.
 * 
 *    Rev 1.8   Jul 16 2010 13:52:10   gartner
 * Added support to patch full SPI BIN files as well as ROM file contents only. Added support for OEM SLIC modules. Added support to handle EFI settings (backup) and default maps.
 * 
 *    Rev 1.7   Apr 16 2009 12:55:04   gartner
 * Added support for OEM HDA verb table modules.
 * 
 *    Rev 1.6   Jul 02 2007 13:04:32   gartner
 * Add support for OEM video mode modules. Fix bug that caused problems with OEM setup module generation.
 * 
 *    Rev 1.5   Nov 09 2006 13:12:58   gartner
 * Fix bug that occurred when switching from ROMFILE mode to BOARD mode due to overwritten section infos.
 * 
 *    Rev 1.4   Oct 30 2006 15:18:14   gartner
 * Added support for OEM setup and string modules. Added support to retrieve original AMIBIOS setup and string tables. 
 * 
 *    Rev 1.3   Mar 21 2006 14:51:14   gartner
 * Added module (data) compare routine. Make a two staged scan for the BIOS information structure in flash. First a 64byte aligned scan is perfromed which should succeed on current BIOS versions. To support older BIOS versions that could not ensure this alignment an second scan stage is added that searches on DWORD boundaries. Only if this also fails, the function returns an error.
 * 
 *    Rev 1.2   Jan 30 2006 15:02:22   gartner
 * Added break's to end of switch cases. Return UNKNOWN as OEM version if the value is still set to build time 0xFFFFFFFF.
 * 
 *    Rev 1.1   Jan 27 2006 12:33:16   gartner
 * Added lots of check functions to avoid inclusion of invalid modules. Basic restructuring and return code rework.
 * 
 *    Rev 1.0   Oct 04 2005 13:14:14   gartner
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
#include "cgbmod.h"
#include "cgbinfo.h"
#include "cgepi.h"
#ifdef WIN32
#include "amiimage.h"
#endif
#include "dmstobin.h"															//MOD008
#include <math.h>																//MOD008

/*--------------
 * Externs used
 *--------------
 */

extern UINT16 g_nOperationTarget;
extern HCGOS hCgos;
extern FILE *g_fpBiosRomfile;
extern _TCHAR *g_lpszBiosFilename;
extern UINT16 CheckEdid13Data(unsigned char *pbDataBuffer,UINT32 ulDataSize);

/*--------------------
 * Local definitions
 *--------------------
 */
#define CG_MPFA_RTC_SKIP_SIZE   16      // 16 bytes of RTC data can be skipped in CMOS maps
#define MAX_MODULE_FLASH_RETRIES	10	// Max. retries when trying to update parts o the flash MOD002

/*------------------
 * Global variables
 *------------------
 */
CG_MPFA_TYPE g_MpfaTypeList[] =   {  
{CG_MPFA_TYPE_CMOS_BACKUP,  "Current Setup Settings",  "CURRENT_SETTINGS",  CG_MPFA_DYNAMIC,	//MOD009
 CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF|CG_MPFACMP_PARAMFLAGS|CG_MPFACMP_PARAM0|CG_MPFACMP_PARAM1|CG_MPFACMP_PARAM2,
 NULL},
{CG_MPFA_TYPE_CMOS_DEFAULT, "Default Setup Settings", "DEFAULT_SETTINGS", CG_MPFA_STATIC,		//MOD009
 CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF|CG_MPFACMP_PARAMFLAGS|CG_MPFACMP_PARAM0|CG_MPFACMP_PARAM1|CG_MPFACMP_PARAM2,
 NULL},
{CG_MPFA_TYPE_UC,           "OEM Code/Data",         "USERCODE",     CG_MPFA_STATIC,
 0,
 NULL},
//GWETODO {CG_MPFA_TYPE_BOOT,         "Bootloader",       "BOOTLDR",      CG_MPFA_STATIC,
// CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF|CG_MPFACMP_PARAMFLAGS|CG_MPFACMP_PARAM0|CG_MPFACMP_PARAM1|CG_MPFACMP_PARAM2,
// NULL},
{CG_MPFA_TYPE_PDA,          "EPI Panel Data",		"EPI_PDA",		CG_MPFA_STATIC,
 CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF|CG_MPFACMP_PARAMFLAGS|CG_MPFACMP_PARAM0|CG_MPFACMP_PARAM1|CG_MPFACMP_PARAM2,
 NULL},
{CG_MPFA_TYPE_LOGO,         "BIOS Boot Logo",		"LOGO",         CG_MPFA_STATIC,
 CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF,
 NULL},
{CG_MPFA_TYPE_SETUP,        "OEM Setup (Core8 BIOS only !)",			"OEM_SETUP",	CG_MPFA_STATIC,
 CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF,
 NULL},																				
{CG_MPFA_TYPE_CMOS_FIXED,   "Fixed Setup Settings",	"FIXED_SETTINGS",	CG_MPFA_STATIC,		//MOD901
 CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF,
 NULL},
//{CG_MPFA_TYPE_STRING,       "OEM String Table",		"OEM_STRING",	CG_MPFA_STATIC,	//MOD008: Defined but never used.
// CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF,
// NULL},
 {CG_MPFA_TYPE_OEMVMODE,    "OEM Video Mode",		"OEM_VMODE",	CG_MPFA_STATIC,
 CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF,
 NULL},
{CG_MPFA_TYPE_OEMHDAVERB,   "OEM HDA Verb Table",   "OEM_HDAVERB",	CG_MPFA_STATIC,
 CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF,
 NULL},
{CG_MPFA_TYPE_OEMSLIC,		"OEM SLIC Table",		"OEM_SLIC",		CG_MPFA_STATIC,
 CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF,
 NULL},																			//MOD001 v
{CG_MPFA_TYPE_OEMSLP,		"OEM SLP 1.0 Module",	"OEM_SLP10",	CG_MPFA_STATIC,
 CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_ENTRYOFF,
 NULL},
{CG_MPFA_TYPE_VBIOS_STD,	"Standard VBIOS",		"STD_VBIOS",	CG_MPFA_STATIC,
 CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF,
 NULL},																			
{CG_MPFA_TYPE_VBIOS_OEM,	"OEM VBIOS",			"OEM_VBIOS",	CG_MPFA_STATIC,
 CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF,
 NULL},																			//MOD001 ^ MOD005 v 																			
{CG_MPFA_TYPE_SMALL_LOGO,	"POST Logo Left",		"POST_LOGO_L",	CG_MPFA_STATIC,
 CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF,
 NULL},
{CG_MPFA_TYPE_OEM_LOGO,		"POST Logo Right",		"POST_LOGO_R",	CG_MPFA_STATIC,
 CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF,
 NULL},
{CG_MPFA_TYPE_FIRMWARE_VOLUME,	"UEFI FW Volume",	"FW_VOLUME",	CG_MPFA_STATIC,
 CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF,
 NULL},
{CG_MPFA_TYPE_OA30,			"OA 3.0 Module",		"OA30",			CG_MPFA_STATIC,
 CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF,
 NULL},																			//MOD005 ^ 
 {CG_MPFA_TYPE_SETUP_MENU_SETTINGS,	"OEM Setup Menu Control", "OEM_SETUP_CONTROL",	CG_MPFA_STATIC,	//MOD008 v 
 CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF,
 NULL},
 {CG_MPFA_TYPE_OEM_SMBIOS_DATA,		"OEM SMBIOS Data",		"OEM_SMBIOS",	CG_MPFA_STATIC,
 CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF,		
 NULL},																								
{CG_MPFA_TYPE_PAD,			"Pad Module",			"PAD",			CG_MPFA_STATIC,		
 CG_MPFACMP_LOADTIME|CG_MPFACMP_EXECTIME|CG_MPFACMP_LOADADDR|CG_MPFACMP_ENTRYOFF|CG_MPFACMP_PARAMFLAGS|CG_MPFACMP_PARAM0|CG_MPFACMP_PARAM1|CG_MPFACMP_PARAM2,
 NULL},																								//MOD008 ^
};																				


CG_MPFA_POST_HOOK g_MpfaPostHookList[] =   {
{CG_MPFA_POST_NO_LOAD,          "Do not load/execute module.",                  "NONE"},
//{CG_MPFA_POST_NO_EXECUTION,     "Do not execute module.",                     "NO_EXECUTION"},
//{CG_MPFA_POST_AFTER_MEMORY_INIT,"Load/execute after memory initialisation.",    "AFTER_MEMORY_INIT"},
{CG_MPFA_POST_BEFORE_CMOS_TEST, "Load/execute before CMOS test.",               "BEFORE_CMOS_TEST"},
{CG_MPFA_POST_BEFORE_VIDEO_INIT,"Load/execute before video BIOS initialisation.","BEFORE_VIDEO_INIT"},
{CG_MPFA_POST_AFTER_VIDEO_INIT, "Load/execute after video BIOS initialisation.","AFTER_VIDEO_INIT"},
{CG_MPFA_POST_BEFORE_OPROM_SCAN,"Load/execute before option ROM scan.",         "BEFORE_OPROM_SCAN"},
{CG_MPFA_POST_AFTER_OPROM_SCAN, "Load/execute after option ROM scan.",          "AFTER_OPROM_SCAN"},
{CG_MPFA_POST_BEFORE_SETUP,     "Load/execute on setup screen start.",          "BEFORE_SETUP"},
{CG_MPFA_POST_SETUP_EXTENSION,  "Load/execute as setup screen extension.",      "SETUP_EXTENSION"},
{CG_MPFA_POST_BEFORE_BOOT,      "Load/execute at end of BIOS POST.",            "BEFORE_BOOT"}};

CG_MPFA_SECTION_INFO CgMpfaStaticInfo = {CG_MPFA_STATIC,
                                                0,
                                                0,
                                                CG32_STORAGE_MPFA_STATIC,
                                                NULL,
                                                0xFFFFFFFF};

CG_MPFA_SECTION_INFO CgMpfaUserInfo =   {CG_MPFA_USER,
                                                0,
                                                0,
                                                CG32_STORAGE_FLASH,
                                                NULL,
                                                0xFFFFFFFF};

CG_MPFA_SECTION_INFO CgMpfaDynamicInfo ={CG_MPFA_DYNAMIC,
                                                0,
                                                0,
                                                CG32_STORAGE_MPFA_DYNAMIC,
                                                NULL,
                                                0xFFFFFFFF};

CG_MPFA_SECTION_INFO CgMpfaAllInfo =    {CG_MPFA_ALL,
                                                0,
                                                0,
                                                CG32_STORAGE_MPFA_ALL,
                                                NULL,
                                                0xFFFFFFFF};
																				//MOD001 v
CG_MPFA_SECTION_INFO CgMpfaExtdInfo =    {CG_MPFA_EXTD,
                                                0,
                                                0,
                                                CG32_STORAGE_MPFA_EXTD,
                                                NULL,
                                                0xFFFFFFFF};
																				// MOD001 ^
CG_MPFA_SECTION_INFO* g_MpfaSectionList[] = {
		&CgMpfaExtdInfo,		//MUST BE FIRST ENTRY TO EASE OUTPUT CREATION IN ROMFILE MODE !	//MOD001
        &CgMpfaAllInfo,																			//MOD001		
        &CgMpfaDynamicInfo,
        &CgMpfaStaticInfo,
        &CgMpfaUserInfo};

UINT32 g_nNoMpfaTypes = sizeof g_MpfaTypeList / sizeof g_MpfaTypeList[0];
UINT32 g_nNoPostHooks = sizeof g_MpfaPostHookList / sizeof g_MpfaPostHookList[0];
UINT32 g_nNoMpfaSections = sizeof g_MpfaSectionList / sizeof g_MpfaSectionList[0];

static CG_MPFA_MODULE_END localMpfaEnd = {CG_MPFA_MOD_END_ID};

// This module header is only used to dereive the size of a MPFA header
// and to ease search for the MPFA ROOT modules !
// Never save any values in this structure !!!!
static CG_MPFA_MODULE_HEADER localMpfaHdr = {CG_MPFA_MOD_HDR_ID,     //hdrID
                                                0,                      //modSize
                                                CG_MPFA_TYPE_ROOT,      //modType
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
// Storage location for BIOS information
CG_BIOS_INFO CgMpfaBiosInfo = {0};

																				//MOD008 v
/*---------------------------------------------------------------------------
 * Name: CgSetupMenuDataExtract
 * Desc: As input for this type of module an MLF file is used which contains 
 *		 the complete setup information (layout, strings, ...) and a change / 
 *		 modification addon package.For the MPFA module only the changes are 
 *		 needed and have to be extracted.
 *		 This routine parses the current MLF setup information input file 
 *		 stores the modification data only in a new file called "tmp.bin".
 *
 * Inp: pMpfaHeader			- pointer to MPFA header of the target module  
 *		FILE* fpInDatafile	- Input data file pointer
 *		 
 *
 * Outp: return code:
 *      CG_MPFARET_OK               - Success
 *		CG_MPFARET_INV              - Invalid input data
 *      CG_MPFARET_ERROR            - Execution error
 *      CG_MPFARET_ERROR_FILE       - Input file processing error
 *
 * MLF file header format:
 *	typedef struct 
 *	{
 *		UINT32			Signature;          // CGML signature
 *		UINT32			TotalSize;          // Size of file, including this header
 *		UINT32			OriginalSize;       // Size of file, without overrides
 *		unsigned char	BiosVersion[8];     // BIOS version tag, i.e TR33R000
 *	} CG_SETUP_MENU_FILE;	
 *---------------------------------------------------------------------------
 */
UINT16 CgSetupMenuDataExtract
(
	CG_MPFA_MODULE_HEADER *pMpfaHeader,
	FILE* fpInDatafile
)
{
	INT32  FileSize;
	UINT32 DataSize;
    UINT32 *pTempUINT32;
	FILE* fpOutfile = NULL;
	unsigned char *pTempBuffer;
	CG_SETUP_MENU_FILE CgMlfHdr;

	// Read MLF file header to local structure
	fread(&CgMlfHdr, sizeof(CgMlfHdr), 1, fpInDatafile );
	if( ferror( fpInDatafile ) )      
    {
		return CG_MPFARET_ERROR_FILE;
	}

	// Check file signature
	if (CgMlfHdr.Signature != CGML_SIGNATURE)
	{
		return CG_MPFARET_INV;
	}

	// Compare total size to original size. If they are identical no change info is present
	// and no module should be generated.
	if (CgMlfHdr.TotalSize == CgMlfHdr.OriginalSize)
	{
		return CG_MPFARET_INV;
	}

	// Get input file size and check whether it matches the total file size header entry.

	// Set file pointer to end of file
    if(!fseek(fpInDatafile,0, SEEK_END))
    {
        // Get position at end of file = file length
        if((FileSize = ftell(fpInDatafile)) >= 0)
        {
            // Set file pointer back to start of file
            if(!fseek(fpInDatafile,0, SEEK_SET))
            {
				// Save temporary data size            
                DataSize = (UINT32) FileSize;
			}
            else
            {
                return CG_MPFARET_ERROR_FILE;
            }
        }
        else
        {
           return CG_MPFARET_ERROR_FILE;
        }
    }
    else
    {
        return CG_MPFARET_ERROR_FILE;
    }

	// Check whether file size matches the total file size header entry.
	if (CgMlfHdr.TotalSize != DataSize)
	{
		return CG_MPFARET_INV;
	}

	// Read all bytes from OriginalSize to TotalSize (overrides area) and 
	// store this info in a temporary file to be used as actual 
	// MPFA module data input file.
	DataSize = CgMlfHdr.TotalSize - CgMlfHdr.OriginalSize;

	// Allocate temporary buffer to hold the override data.
    pTempBuffer = (unsigned char*)malloc(DataSize);
    if(pTempBuffer == NULL)
    {
        return CG_MPFARET_ERROR;
    }


    // Load data to buffer starting from OriginalSize file index.
	// Set file pointer to start of override data
    if(!fseek(fpInDatafile,CgMlfHdr.OriginalSize, SEEK_SET))
    {
		fread(pTempBuffer, DataSize, 1, fpInDatafile );
		if( ferror( fpInDatafile ) )      
		{
			free(pTempBuffer);
			return CG_MPFARET_ERROR_FILE;
		}        
    }
    else
    {
        return CG_MPFARET_ERROR_FILE;
    }

	
	// Write data to output file whcih will be used as new MPFA module imput file.
	fpOutfile = fopen("tmp.bin", "wb");
	if (fpOutfile == NULL) 
	{
		free(pTempBuffer);
		return CG_MPFARET_ERROR_FILE;
    } 
	if(fwrite(pTempBuffer, sizeof(unsigned char),DataSize, fpOutfile ) != DataSize)
	{
		free(pTempBuffer);
		return CG_MPFARET_ERROR_FILE;
	}

	// Close output file and free temporary data buffer.
	fclose(fpOutfile);
	free(pTempBuffer);


	// Now copy BIOS version from MLF file to new MPFA module header.
	pTempUINT32 = (UINT32*)&(CgMlfHdr.BiosVersion);
	pMpfaHeader->modLoadAddr = *pTempUINT32;
	pMpfaHeader->modEntryOff = *(pTempUINT32 +1);

	return CG_MPFARET_OK;
}																				//MOD008 ^		
						

/*---------------------------------------------------------------------------
 * Name: CgMpfaGetOEMBiosVersion     
 * Desc: Gets the OEM BIOS version in the ROOT module header. 
 * Inp:  lpszBiosVersion    - Pointer to zero terminated 8 string that holds 
 *                            the OEM BIOS version. Max. 8 characters plus 
 *                            one character for termination.
 *       
 * Outp: return code:
 *       CG_MPFARET_OK      - Success
 *       CG_MPFARET_ERROR   - Error, or no OEM version defined 
 *
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaGetOEMBiosVersion( _TCHAR* lpszBiosVersion) 
{
    UINT32 nFoundIndex;
    unsigned char *pTempSectionBuffer;

    // Find the ROOT module
    localMpfaHdr.modType = CG_MPFA_TYPE_ROOT;
    if(CgMpfaFindModule(&CgMpfaStaticInfo,&localMpfaHdr, 0, &nFoundIndex, CG_MPFACMP_TYPE) == CG_MPFARET_OK)
    {
        // Check whether we have an OEM BIOS
        pTempSectionBuffer = (CgMpfaStaticInfo.pSectionBuffer + nFoundIndex);
        if((((CG_MPFA_MODULE_HEADER *)pTempSectionBuffer)->modFlags) &CG_MOD_ENTRY_MODIFIED)
        {
            // BIOS not modified
            return CG_MPFARET_ERROR;
        }
        
        // Get the OEM BIOS version from the ROOT module header
        // (we simply use modLoadAddr and modEntryOff fields which are not used by the ROOT module)
        if((((CG_MPFA_MODULE_HEADER *)pTempSectionBuffer)->modLoadAddr) == 0xFFFFFFFF)
        {
            *((UINT32 *)lpszBiosVersion) = (UINT32) 'NKNU'; 
            *((UINT32 *)lpszBiosVersion + 1) = (UINT32) ' NWO';
        }
        else
        {
            *((UINT32 *)lpszBiosVersion) = ((CG_MPFA_MODULE_HEADER *)pTempSectionBuffer)->modLoadAddr; 
            *((UINT32 *)lpszBiosVersion + 1) = ((CG_MPFA_MODULE_HEADER *)pTempSectionBuffer)->modEntryOff;    
        }
        *(lpszBiosVersion + 8) = 0x00;
    }
    else
    {
        return CG_MPFARET_ERROR;
    }
    return CG_MPFARET_OK;
}

/*---------------------------------------------------------------------------
 * Name: CgMpfaSetOEMBiosVersion     
 * Desc: Sets the OEM BIOS version in the ROOT module header. 
 * Inp:  lpszBiosVersion    - Pointer to zero terminated string that holds 
 *                            the OEM BIOS version. Max. 8 characters plus 
 *                            one character for termination.
 *       
 * Outp: return code:
 *       CG_MPFARET_OK      - Success
 *       CG_MPFARET_ERROR   - Error
 *
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaSetOEMBiosVersion( _TCHAR* lpszBiosVersion) 
{
    
    UINT32 nFoundIndex;
    unsigned char *pTempSectionBuffer;

    // Find the ROOT module
    localMpfaHdr.modType = CG_MPFA_TYPE_ROOT;
    if(CgMpfaFindModule(&CgMpfaStaticInfo,&localMpfaHdr, 0, &nFoundIndex, CG_MPFACMP_TYPE) == CG_MPFARET_OK)
    {
        // As we set an OEM BIOS version we will also mark the BIOS as modified
        pTempSectionBuffer = (CgMpfaStaticInfo.pSectionBuffer + nFoundIndex);
        ((CG_MPFA_MODULE_HEADER *)pTempSectionBuffer)->modFlags = 
            ((CG_MPFA_MODULE_HEADER *)pTempSectionBuffer)->modFlags & (~CG_MOD_ENTRY_MODIFIED);
        
        // Insert the OEM BIOS version into the ROOT module header
        // (we simply use modLoadAddr and modEntryOff fields which are not used by the ROOT module)
        ((CG_MPFA_MODULE_HEADER *)pTempSectionBuffer)->modLoadAddr = *((UINT32 *)lpszBiosVersion);
        ((CG_MPFA_MODULE_HEADER *)pTempSectionBuffer)->modEntryOff = *((UINT32 *)lpszBiosVersion + 1);    
    }
    else
    {
        return CG_MPFARET_ERROR;
    }

    return CG_MPFARET_OK;
}

/*---------------------------------------------------------------------------
 * Name: CgMpfaGetSysBiosVersion     
 * Desc: Gets the system BIOS version. 
 * Inp:  lpszBiosVersion    - Pointer to zero terminated string that holds 
 *                            the system BIOS version. Max. 8 characters plus 
 *                            one character for termination.
 *       pBiosType          - Pointer to hold BIOS type. 
 *       
 *       In case one of the pointers is set to zero the respective info part
 *       is not returnd !
 *
 * Outp: return code:
 *       CG_MPFARET_OK      - Success
 *
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaGetSysBiosVersion
( 
    _TCHAR* lpszBiosVersion, 
    unsigned char *pBiosType
) 
{
    UINT16 nCount;

    if(lpszBiosVersion != NULL)
    {
        // The system BIOS version is derived from the BIOS info structure
        for (nCount = 0; nCount < 8; nCount++)
        {
            *(lpszBiosVersion + nCount) = CgMpfaBiosInfo.biosVersion[nCount];
        }    
        *(lpszBiosVersion + 8) = 0x00;
    }

    if(pBiosType != NULL)
    {
        *pBiosType = CgMpfaBiosInfo.biosType;
    }

    return CG_MPFARET_OK;
}

/*---------------------------------------------------------------------------
 * Name: CgMpfaSetSysBiosVersion     
 * Desc: Sets the system BIOS version. 
 * Inp:  lpszBiosVersion    - Pointer to zero terminated string that holds 
 *                            the system BIOS version. Max. 8 characters plus 
 *                            one character for termination.
 *       
 * Outp: return code:
 *       CG_MPFARET_OK      - Success
 *       CG_MPFARET_ERROR   - Error
 *
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaSetSysBiosVersion( _TCHAR* lpszBiosVersion) 
{
    // Function currently not supported (probably never will be)!
    return CG_MPFARET_ERROR;
}

/*---------------------------------------------------------------------------
 * Name: CgMpfaGetBiosInfoRomfile     
 * Desc: Retrieves the complete BIOS information structure from the operating
 *       target ROM file and stores it in the global BIOS info structure.
 * Inp:  none
 *       
 * Outp: return code:
 *       CG_MPFARET_OK      - Success, BIOS info found
 *       CG_MPFARET_ERROR   - Error, BIOS info not found
 *
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaGetBiosInfoRomfile(void) 
{         
 
    UINT16 infoFound = FALSE;
    UINT16 retVal;
   
    if(!g_fpBiosRomfile)
    {
        return CG_MPFARET_ERROR;
    }
    // Set file pointer to start of file
    if(fseek(g_fpBiosRomfile,0, SEEK_SET))
    {
        return CG_MPFARET_ERROR;
    }

    while( !feof( g_fpBiosRomfile ) )
    {
        fread( &CgMpfaBiosInfo.infoIDLow, sizeof(CgMpfaBiosInfo.infoIDLow), 1, g_fpBiosRomfile );
        if( ferror( g_fpBiosRomfile ) )      
        {
            return CG_MPFARET_ERROR;
        }
        else if(CgMpfaBiosInfo.infoIDLow == CG_SYS_BIOS_INFO_ID_L)
        {
            fread( &CgMpfaBiosInfo.infoIDHigh, sizeof(CgMpfaBiosInfo.infoIDHigh), 1, g_fpBiosRomfile );
            if( ferror( g_fpBiosRomfile ) )      
            {
                return CG_MPFARET_ERROR;
            }
            else if(CgMpfaBiosInfo.infoIDHigh == CG_SYS_BIOS_INFO_ID_H)
            {
                // BIOS info structure found, now copy whole structure
                fread( &CgMpfaBiosInfo.infoLen, sizeof(CgMpfaBiosInfo) - 8, 1, g_fpBiosRomfile );
                infoFound = TRUE;
                break;
            }                       
        }
    }
   
    if(infoFound == TRUE)
    {
        retVal = CG_MPFARET_OK; 
    }
    else
    {
        // No info means this cannot be a congatec BIOS file!
        retVal = CG_MPFARET_ERROR;
    }

    return retVal;
}

/*---------------------------------------------------------------------------
 * Name: CgMpfaGetBiosInfoFlash
 * Desc: Retrieves the complete BIOS information structure from the operating
 *       target flash part and stores it in the global BIOS info structure.
 * Inp:  none
 *       
 * Outp: return code:
 *       CG_MPFARET_OK      - Success, BIOS info found
 *       CG_MPFARET_ERROR   - Error, BIOS info not found
 *
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaGetBiosInfoFlash(void) 
{         
 
    UINT16 infoFound = FALSE;
    UINT16 retVal;
    UINT32 nFlashSize, nIndex;
   
    // Get flash size 
    if((nFlashSize = CgosStorageAreaSize(hCgos, CG32_STORAGE_MPFA_ALL)) < 1)
    {
        return CG_MPFARET_ERROR; 
    }
    nIndex = 0;
    while(nIndex < nFlashSize)
    {
        if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_ALL, nIndex, (unsigned char*)&CgMpfaBiosInfo.infoIDLow, sizeof(CgMpfaBiosInfo.infoIDLow)))
        {
            return CG_MPFARET_ERROR;
        }
        else if(CgMpfaBiosInfo.infoIDLow == CG_SYS_BIOS_INFO_ID_L)
        {
            if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_ALL, nIndex + sizeof(CgMpfaBiosInfo.infoIDLow), (unsigned char*)&CgMpfaBiosInfo.infoIDHigh, sizeof(CgMpfaBiosInfo.infoIDHigh)))
            {
                return CG_MPFARET_ERROR;
            }
            else if(CgMpfaBiosInfo.infoIDHigh == CG_SYS_BIOS_INFO_ID_H)
            {
                // BIOS info structure found, now copy whole structure
                if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_ALL, nIndex, (unsigned char*)&CgMpfaBiosInfo, sizeof(CgMpfaBiosInfo)))
                {
                    return CG_MPFARET_ERROR;
                }
                infoFound = TRUE;
                break;
            }                       
        }
        else
        {
            // nIndex = nIndex + sizeof(CgMpfaBiosInfo.infoIDLow);
            nIndex = nIndex + 64;
        }
    }

    // Above we tried the fast 64 byte aligned scan which should work on all up to date BIOS versions.
    // However to support old versions we repeat with a dword scan to make sure we get everything.

    if(infoFound != TRUE)
    {
        nIndex = 0;
        while(nIndex < nFlashSize)
        {
            if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_ALL, nIndex, (unsigned char*)&CgMpfaBiosInfo.infoIDLow, sizeof(CgMpfaBiosInfo.infoIDLow)))
            {
                return CG_MPFARET_ERROR;
            }
            else if(CgMpfaBiosInfo.infoIDLow == CG_SYS_BIOS_INFO_ID_L)
            {
                if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_ALL, nIndex + sizeof(CgMpfaBiosInfo.infoIDLow), (unsigned char*)&CgMpfaBiosInfo.infoIDHigh, sizeof(CgMpfaBiosInfo.infoIDHigh)))
                {
                    return CG_MPFARET_ERROR;
                }
                else if(CgMpfaBiosInfo.infoIDHigh == CG_SYS_BIOS_INFO_ID_H)
                {
                    // BIOS info structure found, now copy whole structure
                    if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_ALL, nIndex, (unsigned char*)&CgMpfaBiosInfo, sizeof(CgMpfaBiosInfo)))
                    {
                        return CG_MPFARET_ERROR;
                    }
                    infoFound = TRUE;
                    break;
                }                       
            }
            else
            {
                // nIndex = nIndex + sizeof(CgMpfaBiosInfo.infoIDLow);
                nIndex = nIndex + 4;
            }
        }
    }
    if(infoFound == TRUE)
    {
        retVal = CG_MPFARET_OK; 
    }
    else
    {
        // No info means this cannot be a congatec BIOS file!
        retVal = CG_MPFARET_ERROR;
    }

    return retVal;
}

/*---------------------------------------------------------------------------
 * Name: CgCheckJpegForAMI
 * Desc: This function checks whether a certain module contains a valid JPEG
 *       boot logo for the AMIBIOS JPEG logo display handler. 
 * Inp:  pModuleBuffer          - Pointer to buffer holding the module to be
 *                                checked.
 * Outp: return code:
 *       CG_MPFARET_INV_DATA    - Invalid MPFA module data
 *       CG_MPFARET_OK          - Module and module data OK
 *        
 *---------------------------------------------------------------------------
 */
UINT16 CgCheckJpegForAMI(unsigned char *pModuleBuffer)
{
    unsigned char *pTempData;
    UINT32 nMaxSize, nIndex;
    unsigned char nByteVal;

    // Skip module header
    pTempData = pModuleBuffer + sizeof(localMpfaHdr);
    
    // Get max. size to check
    nMaxSize = (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modSize) - sizeof(localMpfaHdr) - sizeof(localMpfaEnd);

    // Check whether it is a JPEG file at all
    if( (*((UINT32 *)pTempData) != 0xE0FFD8FF) || (*((UINT32 *)(pTempData + 6)) != 0x4649464A))
    {
        return CG_MPFARET_INV_DATA;
    }

    // Skip JFIF header
    nIndex = 6;
    do
    {
        if( *(pTempData + nIndex) == 0xFF)
        {
            nByteVal = *(pTempData + nIndex + 1);
            if((nByteVal == 0xED) || (nByteVal == 0xED) || (nByteVal == 0xC1) || (nByteVal == 0xC2) || 
               (nByteVal == 0xC3) || (nByteVal == 0xC5) || (nByteVal == 0xC6) || (nByteVal == 0xC7) ||
               (nByteVal == 0xD0) || (nByteVal == 0xD1) || (nByteVal == 0xD2) || (nByteVal == 0xD3) ||
               (nByteVal == 0xD4) || (nByteVal == 0xD5) || (nByteVal == 0xD6) || (nByteVal == 0xD7) ||
               (nByteVal == 0xDD) || (nByteVal == 0xE1))
            {
                return CG_MPFARET_INV_DATA;
            }
        }        
        nIndex = nIndex + 1;
    }while(nIndex < nMaxSize);

    return CG_MPFARET_OK;
}

/*---------------------------------------------------------------------------
 * Name: CgCheckEpiModule
 * Desc: This function checks whether a certain module is a valid EPI BIOS
 *       module.
 * Inp:  pModuleBuffer          - Pointer to buffer holding the module to be
 *                                checked.
 * Outp: return code:
 *       CG_MPFARET_INV_DATA    - Module is no EPI module
 *       CG_MPFARET_OK          - Module is valid EPI module
 *        
 *---------------------------------------------------------------------------
 */
UINT16 CgCheckEpiModule(unsigned char *pModuleBuffer)
{
    unsigned char *pTempData;
    UINT32 nMaxSize, nIndex;

    // The check performed here is very limited, but better checks are not possible 
    // because of the 'bugs' in the EPI module.

    // Skip module header
    pTempData = pModuleBuffer + sizeof(localMpfaHdr);
    
    // Get max. size to check
    nMaxSize = (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modSize) - sizeof(localMpfaHdr) - sizeof(localMpfaEnd);

    // Set start index
    nIndex = 0;

    do
    {
        nIndex = nIndex + SIZE_EDID13_DATA;
        pTempData = pTempData + SIZE_EDID13_DATA;
        if( *((UINT32*)pTempData) == CG_EPDA_USER_AREA_START_L)
        {
            return CG_MPFARET_OK;
        }
    }while(nIndex < nMaxSize);
    return CG_MPFARET_INV_DATA;
}
/*---------------------------------------------------------------------------
 * Name: CgMpfaModuleTypeSpecificCheck
 * Desc: This function executes specific module or module data checks 
 *       depending on the respective module type.
 * Inp:  pModuleBuffer          - Pointer to buffer holding the module to be
 *                                checked.
 * Outp: return code:
 *       CG_MPFARET_INCOMP      - Module incompatible to operation target 
 *       CG_MPFARET_INV         - Module or module data is invalid
 *       CG_MPFARET_INV_DATA    - Invalid MPFA module data
 *       CG_MPFARET_INV_PARM    - Invalid MPFA module parameters
 *       CG_MPFARET_OK          - Module and module data OK
 *        
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaModuleTypeSpecificCheck
(
    unsigned char *pModuleBuffer
)
{
    UINT16 retVal;
    UINT32 *pTempUINT32, *pDataBiosVersion, ulVerbLength, nAlignment;			//MOD008
	
    switch (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modType)
    {
        case CG_MPFA_TYPE_CMOS_BACKUP:
			if(CgMpfaBiosInfo.biosType != CG_EFI_AMI)			
			{
				// Get the system CMOS map length and check whether it matches.
				if (((((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modSize) - sizeof(localMpfaHdr) - sizeof(localMpfaEnd)) != CgMpfaBiosInfo.cmosSize)
				{
					retVal = CG_MPFARET_INCOMP;
					break;
				}
			}
			else
			{
				//GWETODO: Find proper check for EFI 'CMOS Backup' = Active Setup Settings Map
			}													


            // Compare the BIOS version stored in the module header with
            // the version of the target.
            pTempUINT32 = (UINT32*)&(CgMpfaBiosInfo.biosVersion);
            if( (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modLoadAddr != *pTempUINT32) ||
                (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modEntryOff != *(pTempUINT32 +1)))
            {
                retVal = CG_MPFARET_INCOMP;
                break;
            }                
            retVal = CG_MPFARET_OK;
            break;
            
        case CG_MPFA_TYPE_CMOS_DEFAULT:
			if(CgMpfaBiosInfo.biosType != CG_EFI_AMI)			
			{
				// Get the system CMOS map length and check whether it matches.
				if (((((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modSize) - sizeof(localMpfaHdr) - sizeof(localMpfaEnd)) != CgMpfaBiosInfo.cmosSize)
				{
					retVal = CG_MPFARET_INCOMP;
					break;
				}
			}
			else
			{
				//GWETODO: Find proper check for EFI Setup Default Settings Map
			}													

            // Compare the BIOS version stored in the module header with
            // the version of the target.
            pTempUINT32 = (UINT32*)&(CgMpfaBiosInfo.biosVersion);
            if( (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modLoadAddr != *pTempUINT32) ||
                (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modEntryOff != *(pTempUINT32 +1)))
            {
                retVal = CG_MPFARET_INCOMP;
                break;
            }                
            retVal = CG_MPFARET_OK;
            break;

        case CG_MPFA_TYPE_CMOS_FIXED:
            // Get the system CMOS map length and check whether it matches.
            if (((((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modSize) - sizeof(localMpfaHdr) - sizeof(localMpfaEnd)) != CgMpfaBiosInfo.cmosSize)
            {
                retVal = CG_MPFARET_INCOMP;
                break;
            }

            // Compare the BIOS version stored in the module header with
            // the version of the target.
            pTempUINT32 = (UINT32*)&(CgMpfaBiosInfo.biosVersion);
            if( (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modLoadAddr != *pTempUINT32) ||
                (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modEntryOff != *(pTempUINT32 +1)))
            {
                retVal = CG_MPFARET_INCOMP;
                break;
            }                
            retVal = CG_MPFARET_OK;
            break;

        case CG_MPFA_TYPE_SETUP:
            // Look for the '$MPT' signature at start of data block
            if( *((UINT32 *)(pModuleBuffer + sizeof(localMpfaHdr))) != 'TPM$')
            {
                retVal = CG_MPFARET_INV_DATA;
                break;
            }

            // Compare the BIOS version stored in the module header with
            // the version of the target.
            pTempUINT32 = (UINT32*)&(CgMpfaBiosInfo.biosVersion);
            if( (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modLoadAddr != *pTempUINT32) ||
                (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modEntryOff != *(pTempUINT32 +1)))
            {
                retVal = CG_MPFARET_INCOMP;
                break;
            }                

            // Now compare the BIOS version stored at the end of the module data block
            // with the version of the target.
            /*GWETODO v
            pDataBiosVersion = (UINT32*)(pModuleBuffer + ( ((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modSize - sizeof(localMpfaEnd) - sizeof(CgMpfaBiosInfo.biosVersion) ));
            if( (*pDataBiosVersion != *pTempUINT32) ||
                (*(pDataBiosVersion +1) != *(pTempUINT32 +1)))
            {
                retVal = CG_MPFARET_INCOMP;
                break;
            }        
            GWETODO ^*/
            retVal = CG_MPFARET_OK;
            break;

        case CG_MPFA_TYPE_STRING:
            // Look for the 'LANG' signature at start of data block
            if( *((UINT32 *)(pModuleBuffer + sizeof(localMpfaHdr))) != 'LANG')
            {
                retVal = CG_MPFARET_INV_DATA;
                break;
            }

            // Compare the BIOS version stored in the module header with
            // the version of the target.
            pTempUINT32 = (UINT32*)&(CgMpfaBiosInfo.biosVersion);
            if( (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modLoadAddr != *pTempUINT32) ||
                (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modEntryOff != *(pTempUINT32 +1)))
            {
                retVal = CG_MPFARET_INCOMP;
                break;
            }                

            // Now compare the BIOS version stored at the end of the module data block
            // with the version of the target.
            pDataBiosVersion = (UINT32*)(pModuleBuffer + ( ((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modSize - sizeof(localMpfaEnd) - sizeof(CgMpfaBiosInfo.biosVersion) ));
            if( (*pDataBiosVersion != *pTempUINT32) ||
                (*(pDataBiosVersion +1) != *(pTempUINT32 +1)))
            {
                retVal = CG_MPFARET_INCOMP;
                break;
            }                
            retVal = CG_MPFARET_OK;
            break;
            
        case CG_MPFA_TYPE_PDA:
            // Check whether the module data consists of EPI blocks !
            retVal = CgCheckEpiModule(pModuleBuffer);
            break;

        case CG_MPFA_TYPE_LOGO:
            // Get BIOS type (AMIBIOS or XpressROM). For AMIBIOS do the JPEG test
            // as performed internally in the latest JPEG version AND check for EXIF
            // and IPTC blocks -> reject those JPEGs
            // if((CgMpfaBiosInfo.infoLen != sizeof(CgMpfaBiosInfo)) || (CgMpfaBiosInfo.biosType == CG_BIOS_AMI))
			if(CgMpfaBiosInfo.biosType == CG_BIOS_AMI)	// Do this because otherwise logos for LX800 boards cannot be generated/added anymore.
            {
                retVal = CgCheckJpegForAMI(pModuleBuffer);
                break;
            }
            retVal = CG_MPFARET_OK;
            break;

        case CG_MPFA_TYPE_UC:
            // For OEM code/data modules we simply ensure that the module is loaded before it 
            // should be executed and that the entry offset lies within the module area.
            if( ((((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modLoadTime > 
                ((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modExecTime) && (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modExecTime != CG_MPFA_POST_NO_EXECUTION)) ||
                (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modEntryOff >  
                ((((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modSize) - sizeof(localMpfaHdr) - sizeof(localMpfaEnd))))
            {
                retVal = CG_MPFARET_INV_PARM;
                break;
            }
            retVal = CG_MPFARET_OK;
            break;

        case CG_MPFA_TYPE_OEMVMODE:
            // Check whether the module data is a valid EDID 1.3 or EPI data set  !
            if(CheckEdid13Data(pModuleBuffer + sizeof(localMpfaHdr), (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modSize) - sizeof(localMpfaHdr) - sizeof(localMpfaEnd)))
            {
                retVal = CG_MPFARET_OK;
            }
            else
            {
                retVal = CG_MPFARET_INV_DATA;
            }
            break;

        case CG_MPFA_TYPE_OEMHDAVERB:
            // Look for the '$OEMHDA$' signature at start of data block
            pTempUINT32 = ((UINT32 *)(pModuleBuffer + sizeof(localMpfaHdr)));
            if( *pTempUINT32 != 'MEO$')
            {
                retVal = CG_MPFARET_INV_DATA;
                break;
            }
            if( *(pTempUINT32 +1) != '$ADH')
            {
                retVal = CG_MPFARET_INV_DATA;
                break;
            }

            // Check whether first verb table length matches with total module length
            if( ((*(pTempUINT32 + 3))<<2) > ( (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modSize) - sizeof(localMpfaHdr) - sizeof(localMpfaEnd) ) )
            {
                retVal = CG_MPFARET_INV_DATA;
                break;
            }

            // Sum of lenghts of single verb table entries must mathc module data size
            ulVerbLength = 0;
            do
            {
                ulVerbLength = ulVerbLength + *(pTempUINT32 + 3);
                pTempUINT32 = pTempUINT32 + *(pTempUINT32 + 3);
            }while(*pTempUINT32 == 'MEO$');

            if((ulVerbLength << 2) != ( (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modSize) - sizeof(localMpfaHdr) - sizeof(localMpfaEnd) ) )
            {
                retVal = CG_MPFARET_INV_DATA;
                break;
            }
  
            retVal = CG_MPFARET_OK;
            break;

		case CG_MPFA_TYPE_OEMSLIC:
		case CG_MPFA_TYPE_OEMSLP:												//MOD001 v
		case CG_MPFA_TYPE_VBIOS_STD:	
		case CG_MPFA_TYPE_VBIOS_OEM:											//MOD001 ^
			retVal = CG_MPFARET_OK;
            break;
		
		case CG_MPFA_TYPE_SETUP_MENU_SETTINGS:									//MOD008 v
			// Compare the BIOS version stored in the module header with
            // the version of the target.
            pTempUINT32 = (UINT32*)&(CgMpfaBiosInfo.biosVersion);
            if( (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modLoadAddr != *pTempUINT32) ||
                (((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modEntryOff != *(pTempUINT32 +1)))
            {
                retVal = CG_MPFARET_INCOMP;
                break;
            }                
            retVal = CG_MPFARET_OK;
            break;

		case CG_MPFA_TYPE_OEM_SMBIOS_DATA:
			retVal = CG_MPFARET_OK;
			break;
		
		case CG_MPFA_TYPE_FIRMWARE_VOLUME:
			// Look for ta valid FV header (see definition in cgbmod.h)
            pTempUINT32 = ((UINT32 *)(pModuleBuffer + sizeof(localMpfaHdr)));
			nAlignment = (UINT32)pow(2.0, (INT32)(*(pTempUINT32+0xb) & 0x001F0000) >> 16);
            if( ( *pTempUINT32 != 0x00000000) || ( *(pTempUINT32+1) != 0x00000000) || 
				( *(pTempUINT32+2) != 0x00000000) || ( *(pTempUINT32+3) != 0x00000000) || 
				(((((CG_MPFA_MODULE_HEADER *)pModuleBuffer)->modSize) - sizeof(localMpfaHdr) - sizeof(localMpfaEnd)) != *(pTempUINT32+8)) ||
				( nAlignment < FIRMWARE_VOLUME_MIN_ALIGN ) || ( nAlignment > FIRMWARE_VOLUME_MAX_ALIGN ))
            {
                retVal = CG_MPFARET_INV_DATA;
                break;
            }
			else
			{
				retVal = CG_MPFARET_OK;
				break;
			}

        default:
            retVal = CG_MPFARET_OK;
            break;																//MOD008 ^
    }
    return retVal;
}

/*---------------------------------------------------------------------------
 * Name: CgMpfaStart
 * Desc: Perform necessary operations to prepare MPFA module access.
 * Inp:  bIncMPFA_ALL   - If TRUE, the whole flash area should be read into 
 *                        the buffer as well.
 * Outp: return code:
 *       CG_MPFARET_INTRF_ERROR  - Interface access error 
 *       CG_MPFARET_ERROR        - Execution error
 *       CG_MPFARET_OK           - Success
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaStart(UINT16 bIncMPFA_ALL)
{        
    if(g_nOperationTarget == OT_ROMFILE)
    {
        g_fpBiosRomfile = fopen(g_lpszBiosFilename, "rb");
        if (!g_fpBiosRomfile)
        {
            return CG_MPFARET_INTRF_ERROR;
        }
        if(CgMpfaGetBiosInfoRomfile() != CG_MPFARET_OK)
        {
            return CG_MPFARET_ERROR;
        }
    }
    else if(g_nOperationTarget == OT_BOARD)
    {
        if(!CgosOpen())
        {
            return CG_MPFARET_INTRF_ERROR;
        }
        if(CgMpfaGetBiosInfoFlash() != CG_MPFARET_OK)
        {
            return CG_MPFARET_ERROR;
        }
    }
    else    //OT_NONE
    {
        return CG_MPFARET_OK;
    }

    if (CgMpfaCreateSectionInfo() != CG_MPFARET_OK)
    {
        return CG_MPFARET_ERROR;
    }
    if (CgMpfaBufferInit(bIncMPFA_ALL) != CG_MPFARET_OK)
    {
        return CG_MPFARET_ERROR;        
    }
    
    return CG_MPFARET_OK;          
}

/*---------------------------------------------------------------------------
 * Name: CgMpfaEnd
 * Desc: Perform necessary cleanup to end MPFA access.
 * Inp:  none
 * Outp: return code:
 *       CG_MPFARET_INTRF_ERROR  - Interface access error 
 *       CG_MPFARET_ERROR        - Execution error
 *       CG_MPFARET_OK           - Success
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaEnd(void)
{       
    
    if(g_nOperationTarget == OT_ROMFILE)
    {
        if (g_fpBiosRomfile)
        {
            fclose(g_fpBiosRomfile);
        }
    }
    else if(g_nOperationTarget == OT_BOARD)
    {
        CgosClose(); 
    }
    else    //OT_NONE
    {
        return CG_MPFARET_OK;
    }
    
    CgMpfaBufferCleanup();
    return CG_MPFARET_OK;          
}

/*---------------------------------------------------------------------------
 * Name: DeriveSectionInfoFromCgos
 * Desc: Acquire and store information about all MPFA sections.
 *       Information derived using CGOS interface.
 * Inp:  none
 * Outp: return code:
 *       CG_MPFARET_INTRF_ERROR  - Interface access error 
 *       CG_MPFARET_ERROR        - Execution error
 *       CG_MPFARET_OK           - Success
 *--------------------------------------------------------------------------
 */
static UINT16 DeriveSectionInfoFromCgos(void)
{
    UINT32 i;
    CG_MPFA_SECTION_INFO* pTempInfo;

    // Ensure that physical access types for CGOS are still valid. Might have been
    // overwritten by ROMFILE mode access before
    CgMpfaStaticInfo.physAccess = CG32_STORAGE_MPFA_STATIC;
    CgMpfaUserInfo.physAccess = CG32_STORAGE_FLASH;
    CgMpfaDynamicInfo.physAccess = CG32_STORAGE_MPFA_DYNAMIC;
    CgMpfaAllInfo.physAccess = CG32_STORAGE_MPFA_ALL;

    if(!hCgos)
    {
        return CG_MPFARET_INTRF_ERROR;
    }
    for(i= 0; i < g_nNoMpfaSections; i++)
    {		
		pTempInfo = g_MpfaSectionList[i];
		if(pTempInfo->sectionType != CG_MPFA_EXTD)								//MOD001
		{																		//MOD001 
			pTempInfo->sectionSize = CgosStorageAreaSize(hCgos, pTempInfo->physAccess); 
			//GWETODO: FIX after CGOS driver is fixed to return correct blocksize pTempInfo->sectionBlockSize = (CgosStorageAreaBlockSize(hCgos, pTempInfo->physAccess))*1024; 
			pTempInfo->sectionBlockSize = 64 * 1024;//GWETODO
		}																		//MOD001
    }
    return CG_MPFARET_OK;
}
/*---------------------------------------------------------------------------
 * Name: DeriveSectionInfoFromRomfile
 * Desc: Acquire and store information about all MPFA sections.
 *       Information derived from BIOS file.
 * Inp:  none
 * Outp: return code:
 *       CG_MPFARET_INTRF_ERROR  - Interface access error 
 *       CG_MPFARET_ERROR        - Execution error
 *       CG_MPFARET_OK           - Success
 *---------------------------------------------------------------------------
 */
static UINT16 DeriveSectionInfoFromRomfile(void)
{
    UINT16 infoFound = FALSE;
    CG_MPFA_INFO MpfaBiosInfo;
	INT32 lFileLength = 0;					
    

	// Get total file size and base all BIOS section info on this file size to be able to handle
	// ROM files in larger BIN (SPI flash) files. Condition: ROM is always at end of BIN file.
	if(!fseek(g_fpBiosRomfile,0, SEEK_END))
    {
		// Get position at end of file = file length
		if((lFileLength = ftell(g_fpBiosRomfile)) < 0)
		{
			return CG_MPFARET_ERROR;
		}
	}
	else
	{
		return CG_MPFARET_ERROR;
	}

    // Set file pointer to start of file
    if(fseek(g_fpBiosRomfile,0, SEEK_SET))
    {
        return CG_MPFARET_ERROR;
    }
    while( !feof( g_fpBiosRomfile ) )
    {
        fread( &MpfaBiosInfo.infoIDLow, sizeof( UINT32 ), 1, g_fpBiosRomfile );
        if( ferror( g_fpBiosRomfile ) )      
        {
            return CG_MPFARET_ERROR;
        }
        else if(MpfaBiosInfo.infoIDLow == CG_MPFA_INFO_ID_L)
        {
            fread( &MpfaBiosInfo.infoIDHigh, sizeof( UINT32 ), 1, g_fpBiosRomfile );
            if( ferror( g_fpBiosRomfile ) )      
            {
                return CG_MPFARET_ERROR;
            }
            else if(MpfaBiosInfo.infoIDHigh == CG_MPFA_INFO_ID_H)
            {
                fread( &MpfaBiosInfo.infoRev, (sizeof(MpfaBiosInfo) - (2* sizeof(UINT32))), 1, g_fpBiosRomfile );
                infoFound = TRUE;
                break;
            }                       
        }
    }
    if(infoFound == TRUE)
    {
/*
        //MOD001 CgMpfaAllInfo.sectionSize = MpfaBiosInfo.flashSize;
		CgMpfaAllInfo.sectionSize = lFileLength;
        CgMpfaAllInfo.sectionBlockSize = MpfaBiosInfo.mpfaBlkSize * 1024;
        //MOD001 CgMpfaAllInfo.physAccess = lFileLength - MpfaBiosInfo.flashSize;							
		CgMpfaAllInfo.physAccess = 0;	//MOD001
*/
//MOD001 v
        CgMpfaExtdInfo.sectionSize = lFileLength;
        CgMpfaExtdInfo.sectionBlockSize = MpfaBiosInfo.mpfaBlkSize * 1024;
        CgMpfaExtdInfo.physAccess = 0;

        CgMpfaAllInfo.sectionSize = MpfaBiosInfo.flashSize;
        CgMpfaAllInfo.sectionBlockSize = MpfaBiosInfo.mpfaBlkSize * 1024;
        CgMpfaAllInfo.physAccess = lFileLength - MpfaBiosInfo.flashSize;
//MOD001 ^

        CgMpfaUserInfo.sectionSize = MpfaBiosInfo.mpfaUEnd- MpfaBiosInfo.mpfaUStart;
        CgMpfaUserInfo.sectionBlockSize = MpfaBiosInfo.mpfaBlkSize * 1024;
        CgMpfaUserInfo.physAccess = lFileLength - ((0xFFFFFFFF - MpfaBiosInfo.mpfaUStart) + 1);		
    
        CgMpfaDynamicInfo.sectionSize = MpfaBiosInfo.mpfaBEnd- MpfaBiosInfo.mpfaBStart;
        CgMpfaDynamicInfo.sectionBlockSize = MpfaBiosInfo.mpfaBlkSize * 1024;
        CgMpfaDynamicInfo.physAccess = lFileLength - ((0xFFFFFFFF - MpfaBiosInfo.mpfaBStart) + 1);	

        CgMpfaStaticInfo.sectionSize = MpfaBiosInfo.mpfaMEnd- MpfaBiosInfo.mpfaMStart;
        CgMpfaStaticInfo.sectionBlockSize = MpfaBiosInfo.mpfaBlkSize *1024;
        CgMpfaStaticInfo.physAccess = lFileLength - ((0xFFFFFFFF - MpfaBiosInfo.mpfaMStart) + 1);	
    }
    else
    {
        return CG_MPFARET_ERROR;
    }

    return CG_MPFARET_OK;
}


/*---------------------------------------------------------------------------
 * Name: CgMpfaCreateSectionInfo
 * Desc: Acquire and store information about all MPFA sections.
 * Inp:  none
 * Outp: return code:
 *       CG_MPFARET_INTRF_ERROR  - Interface access error 
 *       CG_MPFARET_ERROR        - Execution error
 *       CG_MPFARET_OK           - Success
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaCreateSectionInfo(void)
{
    if(g_nOperationTarget == OT_ROMFILE)
    {
        return (DeriveSectionInfoFromRomfile());
    }
    else
    {
        return (DeriveSectionInfoFromCgos());
    }
}

/*---------------------------------------------------------------------------
 * Name: CgMpfaBufferInit
 * Desc: Allocate buffers to hold MPFA section data and load MPFA 
 *       section data to allocated buffers.
 * Inp:  bIncMPFA_ALL   - If TRUE, the whole flash area should be read into 
 *                        the buffer as well.
 * Outp: return code:
 *       CG_MPFARET_INTRF_ERROR  - Interface access error 
 *       CG_MPFARET_ERROR        - Execution error
 *       CG_MPFARET_OK           - Success
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaBufferInit(UINT16 bIncMPFA_ALL)
{
    UINT32 i,nTransfered;
    CG_MPFA_SECTION_INFO* pTempInfo;

    for(i= 0; i < g_nNoMpfaSections; i++)
    {
        pTempInfo = g_MpfaSectionList[i];
        if(pTempInfo->sectionSize != 0)
        {                        
            if(g_nOperationTarget == OT_ROMFILE)
            {
                pTempInfo->pSectionBuffer = (unsigned char*)malloc(pTempInfo->sectionSize);
                if(pTempInfo->pSectionBuffer == NULL)
                {
                    return CG_MPFARET_ERROR;
                }

                fseek(g_fpBiosRomfile,pTempInfo->physAccess, SEEK_SET);        
                fread(pTempInfo->pSectionBuffer, pTempInfo->sectionSize, 1, g_fpBiosRomfile );
                if( ferror( g_fpBiosRomfile ) )      
                {
                    return CG_MPFARET_ERROR;
                }
            }
            else if((g_nOperationTarget == OT_BOARD) && ((pTempInfo->sectionType != CG_MPFA_ALL) || (bIncMPFA_ALL == TRUE)) && //MOD001
				(pTempInfo->sectionType != CG_MPFA_EXTD))	//MOD001			
            {
                pTempInfo->pSectionBuffer = (unsigned char*)malloc(pTempInfo->sectionSize);
                if(pTempInfo->pSectionBuffer == NULL)
                {
                    return CG_MPFARET_ERROR;
                }
                nTransfered = 0;
		        do
                {
                    if(!CgosStorageAreaRead(hCgos, pTempInfo->physAccess, nTransfered, pTempInfo->pSectionBuffer + nTransfered, pTempInfo->sectionBlockSize))
                    {
                        //PRINTF("ERROR:Failed to read %X bytes from MPFA section %X\n",pTempInfo->sectionSize,pTempInfo->physAccess ); 
                        return CG_MPFARET_ERROR;        
                    }
                    nTransfered = nTransfered + pTempInfo->sectionBlockSize;
                }while(nTransfered < pTempInfo->sectionSize);
            }
            if((pTempInfo->sectionType == CG_MPFA_STATIC) || (pTempInfo->sectionType == CG_MPFA_DYNAMIC))
            {
                if(CgMpfaRebuildSection(pTempInfo) != CG_MPFARET_OK)
                {
                    // PRINTF("ERROR: Failed to rebuild MPFA section %X\n",pTempInfo->sectionType ); 
                    return CG_MPFARET_ERROR;        
                }
            }
        }
    }
    return CG_MPFARET_OK;
}
/*---------------------------------------------------------------------------
 * Name: CgMpfaBufferCleanup
 * Desc: Release all allocated buffers for MPFA handling.
 * Inp:  none
 * Outp: return code:
 *       CG_MPFARET_INTRF_ERROR  - Interface access error 
 *       CG_MPFARET_ERROR        - Execution error
 *       CG_MPFARET_OK           - Success
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaBufferCleanup(void)
{
    UINT32 i;
    CG_MPFA_SECTION_INFO* pTempInfo;
    
    for(i= 0; i < g_nNoMpfaSections; i++)
    {
        pTempInfo = g_MpfaSectionList[i];
        if(pTempInfo->pSectionBuffer != NULL)
        {
            free(pTempInfo->pSectionBuffer);  
            pTempInfo->pSectionBuffer = NULL;
        }
    }
    return CG_MPFARET_OK;
}

/*---------------------------------------------------------------------------
 * Name: ApplyChangesToRomfile
 * Desc: Apply changes to BIOS file.
 * Inp:  none
 * Outp: return code:
 *       CG_MPFARET_INTRF_ERROR  - Interface access error 
 *       CG_MPFARET_ERROR        - Execution error
 *       CG_MPFARET_OK           - Success
 *---------------------------------------------------------------------------
 */
static UINT16 ApplyChangesToRomfile(void)
{
    UINT32 i;
    CG_MPFA_SECTION_INFO* pTempInfo;
    
    if (g_fpBiosRomfile)
    {
        fclose(g_fpBiosRomfile);
    }
    else
    {
        return CG_MPFARET_ERROR;
    }

    if (!(g_fpBiosRomfile = fopen(g_lpszBiosFilename, "wb")))    
    {
        return CG_MPFARET_INTRF_ERROR;
    }

    for(i= 0; i < g_nNoMpfaSections; i++)
    {
        pTempInfo = g_MpfaSectionList[i];
        if(pTempInfo->pSectionBuffer != NULL)
        {
            if (!fseek(g_fpBiosRomfile,pTempInfo->physAccess, SEEK_SET))
            {
                if(fwrite(pTempInfo->pSectionBuffer, pTempInfo->sectionSize, 1, g_fpBiosRomfile ) != 1)
                {
                    return CG_MPFARET_INTRF_ERROR;
                }
            }
            else
            {
                return CG_MPFARET_INTRF_ERROR;
            } 
        }
    }
    return CG_MPFARET_OK;
}
/*---------------------------------------------------------------------------
 * Name: ApplyChangesToCgos
 * Desc: Apply changes to target board using CGOS interface.
 * Inp:  none
 * Outp: return code:
 *       CG_MPFARET_INTRF_ERROR  - Interface access error 
 *       CG_MPFARET_ERROR        - Execution error
 *       CG_MPFARET_OK           - Success
 *---------------------------------------------------------------------------
 */
//MOD002 v
//
// Added single erase and write retries, a module area update verification loop
// and in case of a failure here a complete module area update re-processing.
// 

static UINT16 ApplyChangesToCgos(void)
{
    UINT32 nSectionCount, j, k, nWriteStart, nWriteEnd, nRetryCount;
    UINT32 *pBufferData , flashData, *pFlashData;
    CG_MPFA_SECTION_INFO* pTempInfo;
    UINT16 bStartFound, bEraseError, bWriteError, bForceErase;
	

    pFlashData = &flashData;

    if (!hCgos)
    {
        return CG_MPFARET_ERROR;
    }
  
	//
	// Check each MPFA section whether an update is required, i.e. the current buffer contents of
	// that the utility holds for each section is different to the original flash contents.
	// CG_MPFA_ALL and CG_MPFA_EXTD sections are excluded, as they are only relevant for complete
	// BIOS flash updates.
	// The handler compares buffer and flash byte by byte (or DWORD by DWORD) and indicates that 
	// at least a section block update is required when one mismatch is found.
	// In order to improve performance, the handler also checks whether the flash really has
	// to be erased or if the current buffer contents can be written without preceeding erasure
	// of the flash block. This check is based on the assumption that on each flash a bit may be
	// written from '1' to '0' any time.
    for(nSectionCount=0; nSectionCount < g_nNoMpfaSections; nSectionCount++)
    {
		bForceErase = FALSE;
        pTempInfo = g_MpfaSectionList[nSectionCount];
        if((pTempInfo->pSectionBuffer != NULL) && (pTempInfo->sectionType != CG_MPFA_ALL) && (pTempInfo->sectionType != CG_MPFA_EXTD))	//MOD001
        {
SECTION_START:
            k = 0;
            bStartFound =FALSE;
            pBufferData = (UINT32 *)pTempInfo->pSectionBuffer;
            do
            {
                for(j = k/4; j < ((k + (pTempInfo->sectionBlockSize -4))/4); j++)
                {
                    if(!CgosStorageAreaRead(hCgos,pTempInfo->physAccess, j*4, (unsigned char*) pFlashData, 4))
                    {
                        return CG_MPFARET_ERROR;
                    }
                    if( *(pBufferData +j) != *pFlashData)
                    {                        
                        if(bStartFound != TRUE)
                        {
                            nWriteStart = j*4;
                            bStartFound = TRUE;
                        }
                        nWriteEnd = j*4;
                    }

					// Here we check whether the flash block really has to be erased to write the contents.
					// E.g. in case everything is still FF in the flash anything can be written without erasing the block.
                    *pFlashData = ~(*pFlashData);                                       
                    if( (*(pBufferData +j) & (*pFlashData)) || bForceErase)
                    {   
						// New data cannot be written without erasing the block. So start block based
						// erase and write sequence here.
						
						// First we can clear the bStartFound flag to indicate to the 'write only'
						// handler below that it cannot do anything.
						bStartFound = FALSE;	

						nRetryCount = 0;
						bEraseError = FALSE;
						bWriteError = FALSE;

						do
						{
							if(!CgosStorageAreaErase(hCgos, pTempInfo->physAccess, k, pTempInfo->sectionBlockSize))
							{
								bEraseError = TRUE;
							}
							else
							{
								bEraseError = FALSE;
							}
								                        
							if(bEraseError == FALSE)	//No need to try writing if erasing of the block already failed
							{
			                    Sleep(20L);	//Add a little bit of extra security in case the BIOS erase routines don't. MOD002

								if(!CgosStorageAreaWrite(hCgos, pTempInfo->physAccess, k, (unsigned char *)(pTempInfo->pSectionBuffer + k), pTempInfo->sectionBlockSize))
								{
									bWriteError = TRUE;
								}
								else
								{
									bWriteError = FALSE;
								}
							}
							nRetryCount = nRetryCount + 1;
						}while ( ((bEraseError==TRUE) || (bWriteError==TRUE)) && (nRetryCount < MAX_MODULE_FLASH_RETRIES) );

						// Check whether (at least finally after all retries) erasing and writing of the block 
						// was successful. If YES continue with  next section block, if NO exit with error.
						if((bEraseError==TRUE) || (bWriteError==TRUE))
						{
							return CG_MPFARET_ERROR;
						}

                        break;  //No need to check this block any longer, so leave FOR loop and continue with next block!
                    }
                }
                k = k + pTempInfo->sectionBlockSize;                    
            } while (k < pTempInfo->sectionSize);

            if(bStartFound == TRUE)
            { 
				// We only reach this point, if the code above has detected that a MPFA section  requires
				// some new data, but all the data can be written without preceeding erasure of a section block.
				// The code above also has gathered the start and the end address of the new data in the section.
                if(!CgosStorageAreaWrite(hCgos, pTempInfo->physAccess, nWriteStart, (unsigned char *)(pTempInfo->pSectionBuffer + nWriteStart),(nWriteEnd - nWriteStart) +4))
                {
					// Handling an error in a 'write only' processing is a little bit tricky. 
					// If something really went wrong during the write, it is very likely that at 
					// least now erasing of section blocks might be required to recover. A pure
					// retry of the write is likely to fail again. So we simply restart the loop
					// using the deprecated GOTO instruction, but this time we force an erase sequence.
					// The sequence that erases and write block by block of a section already includes
					// a retry mechanism, so this write error should finally be handled.
					bStartFound = FALSE;
					bForceErase = TRUE;
                    goto SECTION_START;
                }      
            }
        }
    }
    return CG_MPFARET_OK;
}
//MOD002 ^                        
//MOD002 v
/*---------------------------------------------------------------------------
 * Name: VerifyChangesToCgos
 * Desc: Checks whether the current utility buffer contents for the MPFA 
 *       sections matches the actual flash contents.
 * Inp:  none
 * Outp: return code:
 *       CG_MPFARET_INTRF_ERROR  - Interface access error 
 *       CG_MPFARET_ERROR        - Execution error
 *       CG_MPFARET_OK           - Success
 *---------------------------------------------------------------------------
 */
static UINT16 VerifyChangesToCgos(void)
{
    UINT32 nSectionCount, j, k;
    UINT32 *pBufferData , flashData, *pFlashData;
    CG_MPFA_SECTION_INFO* pTempInfo;


    pFlashData = &flashData;

    if (!hCgos)
    {
        return CG_MPFARET_ERROR;
    }
  
	//
	// Check each MPFA section whether the buffered section contents of the utility matches 
	// the flash contents of the sections. CG_MPFA_ALL and CG_MPFA_EXTD sections are excluded, as they are only 
	// relevant for complete BIOS flash updates.
	// The handler compares buffer and flash byte by byte (or DWORD by DWORD).
    for(nSectionCount=0; nSectionCount < g_nNoMpfaSections; nSectionCount++)
    {
        pTempInfo = g_MpfaSectionList[nSectionCount];
        if((pTempInfo->pSectionBuffer != NULL) && (pTempInfo->sectionType != CG_MPFA_ALL) && (pTempInfo->sectionType != CG_MPFA_EXTD))	//MOD001
        {
            k = 0;
            pBufferData = (UINT32 *)pTempInfo->pSectionBuffer;
            do
            {
                for(j = k/4; j < ((k + (pTempInfo->sectionBlockSize -4))/4); j++)
                {
                    if(!CgosStorageAreaRead(hCgos,pTempInfo->physAccess, j*4, (unsigned char*) pFlashData, 4))
                    {
                        return CG_MPFARET_ERROR;
                    }
                    if( *(pBufferData +j) != *pFlashData)
                    {                        
                        return CG_MPFARET_ERROR;
                    }
                }
                k = k + pTempInfo->sectionBlockSize;                    
            } while (k < pTempInfo->sectionSize);
        }
    }
    return CG_MPFARET_OK;
}
//MOD002 ^                        
/*---------------------------------------------------------------------------
 * Name: CgMpfaApplyChanges
 * Desc: Apply changes to operation target.
 * Inp:  bRestart       - TRUE: apply changes, end and restart
 *                      - FALSE: only apply changes
 * Outp: return code:
 *       CG_MPFARET_INTRF_ERROR  - Interface access error 
 *       CG_MPFARET_ERROR        - Execution error
 *       CG_MPFARET_OK           - Success
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaApplyChanges(UINT16 bRestart)
{
//MOD002 v
//
// Added verifaction routine to check whether the applied changes really have been
// applied. Added a retry handling in case of problems.
//
    UINT16 retVal;
	UINT32 nRetryCount;		

	nRetryCount = 0;				

	do
	{
		if(g_nOperationTarget == OT_ROMFILE)
		{
			retVal = ApplyChangesToRomfile();
		}
		else if(g_nOperationTarget == OT_BOARD)
		{
			retVal = ApplyChangesToCgos();
			if(retVal == CG_MPFARET_OK)
			{
				retVal = VerifyChangesToCgos();
			}
		}
		else
		{
			// OT_NONE
			return CG_MPFARET_OK;
		}
		nRetryCount = nRetryCount + 1;
	}while((nRetryCount < MAX_MODULE_FLASH_RETRIES) && (retVal != CG_MPFARET_OK));
//MOD002 ^
    if(retVal != CG_MPFARET_OK)
    {
        return retVal;
    }
    if(bRestart)
    {
        if ((retVal = CgMpfaEnd()) == CG_MPFARET_OK)
        {
            retVal = CgMpfaStart(FALSE);
        }
    }

    return retVal;
}


/*---------------------------------------------------------------------------
 * Name: CgMpfaRebuildSection
 * Desc: Rebuild specified section and update section addIndex (= index in 
 *       section where new modules can be added).
 * Inp:  pSectionInfo   - Pointer to information block of section to re-build
 * Outp: return code:
 *       CG_MPFARET_ERROR        - Execution error
 *       CG_MPFARET_OK           - Success
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaRebuildSection(CG_MPFA_SECTION_INFO *pSectionInfo)
{
    UINT32 *pCurrentSource, *pCurrentDest, *pTempSectionBuffer, *pTempUINT32, *pPadData;	//MOD008
    UINT32 nTempOffset, nCount, nCopyLength;    
    UINT32 nPadModuleSize, nPadModuleDataSize, nAlignment, nLocalAddIndex;					//MOD008
	
    // Allocate buffer to rebuild whole MPFA section
    if(!(pTempSectionBuffer = (UINT32 * )malloc(pSectionInfo->sectionSize)))
    {
        return CG_MPFARET_ERROR;
    }
    
    // Set start pointers
    pCurrentSource = (UINT32 *)pSectionInfo->pSectionBuffer;
    pCurrentDest = pTempSectionBuffer;
    
    // Fill temporary buffer with 0xFFs
    nCopyLength = (pSectionInfo->sectionSize) / 4;
    for(nCount = 0; nCount < nCopyLength; nCount++)
    {
        *(pCurrentDest + nCount) = 0xFFFFFFFF; 
    } 
    
    
    // Now scan original section buffer for valid and used modules and copy them
    // to the temporary section buffer  
    do
    {
        if (((CG_MPFA_MODULE_HEADER *)pCurrentSource)->hdrID == CG_MPFA_MOD_HDR_ID)
        {

            //Module header found, now check for module end structure.
            nTempOffset = (((CG_MPFA_MODULE_HEADER *)pCurrentSource)->modSize - sizeof(localMpfaEnd)) / 4;
            if (((CG_MPFA_MODULE_END *)(pCurrentSource + nTempOffset))->endID == CG_MPFA_MOD_END_ID)
            {
                // We have found a valid module entry, now check whether it is USED;
                // if not, we continue with the next module.            
                if (((CG_MPFA_MODULE_HEADER *)pCurrentSource)->modFlags & CG_MOD_ENTRY_USED)
                {
					////////////////////////////////////////////////////////////////////////	//MOD008 v 
					// Special handling for OEM UEFI FV modules. Make sure that the data part 
					// of the MPFA module, i.e. the FV is placed aligned according to the 
					// requirements specified in the FV header.
					// This is achieved by placing a dummy pad module in front of the MPFA FV 
					// module with a data block size that shifts the FV to the required alignment.
					if(((CG_MPFA_MODULE_HEADER *)pCurrentSource)->modType == CG_MPFA_TYPE_FIRMWARE_VOLUME)			
					{
						// Derive FV alignment requirement from FV header.
						pTempUINT32 = ((UINT32 *)(((unsigned char*)pCurrentSource) + sizeof(localMpfaHdr)));
						nAlignment = (UINT32)pow(2.0, (INT32)(*(pTempUINT32+0xb) & 0x001F0000) >> 16);

						//Ensure again, that requested FV alignment is within supported range (already done in module specific check before)
						if( (nAlignment < FIRMWARE_VOLUME_MIN_ALIGN) || (nAlignment > FIRMWARE_VOLUME_MAX_ALIGN))
						{
							free(pTempSectionBuffer);
							return CG_MPFARET_ERROR;
						}
						
						// For simplicity, we always add a PAD module in front of a FV module.
						// Thus we only have to calculate the data size of the PAD module, and always have  
						// the same handling.
						//
						nLocalAddIndex = (UINT32)((unsigned char*)pCurrentDest - (unsigned char*)pTempSectionBuffer);
						nPadModuleDataSize = (nAlignment - (  (nLocalAddIndex + sizeof(localMpfaHdr) + sizeof(localMpfaHdr) + sizeof(localMpfaEnd)) 
																& ((UINT32)(nAlignment-1))  ));

						// All modules have to be DWORD aligned. Thus if we get a non-DWORD			
						// aligned padding requirement, something must be wrong.											
						if(nPadModuleDataSize & 0x00000003)								
						{
							free(pTempSectionBuffer);
							return CG_MPFARET_ERROR;
						}																			
						nPadModuleSize = nPadModuleDataSize + sizeof(localMpfaHdr) + sizeof(localMpfaEnd);

						// Check whether our pad module + the original module can still be placed.
						if( ((unsigned char*)pTempSectionBuffer + pSectionInfo->sectionSize) >= ((unsigned char*)pCurrentDest + nPadModuleSize) )						
						{
							// Copy the pad module
							pPadData = (UINT32*)&localMpfaHdr;
							for(nCount = 0; nCount < sizeof(localMpfaHdr) / 4; nCount++)
							{
								*((UINT32 *)pCurrentDest + nCount) = 
									*((UINT32 *)pPadData + nCount);
							}
							// Adapt the pad module header data. 
							((CG_MPFA_MODULE_HEADER *)pCurrentDest)->modType = CG_MPFA_TYPE_PAD;
							((CG_MPFA_MODULE_HEADER *)pCurrentDest)->modSize = nPadModuleSize;
							((CG_MPFA_MODULE_HEADER *)pCurrentDest)->modFlags |= CG_MOD_ENTRY_USED;
							pCurrentDest = pCurrentDest + (sizeof(localMpfaHdr) / 4);

							// Fill PAD area
							if(nPadModuleDataSize >= 4)
							{
								for(nCount = 0; nCount < nPadModuleDataSize / 4; nCount++)
								{
									*((UINT32 *)pCurrentDest + nCount) = 0xFFFFFFFF;
								}
							}
							pCurrentDest = pCurrentDest + (nPadModuleDataSize / 4);
							
							pPadData = (UINT32*)&localMpfaEnd;
							for(nCount = 0; nCount < sizeof(localMpfaEnd) / 4; nCount++)
							{
								*((UINT32 *)pCurrentDest + nCount) = 
									*((UINT32 *)pPadData + nCount);
							}
							pCurrentDest = pCurrentDest + (sizeof(localMpfaEnd) /4);						
						}
						else
						{
							free(pTempSectionBuffer);
							return CG_MPFARET_ERROR;
						}			
					}
					else if(((CG_MPFA_MODULE_HEADER *)pCurrentSource)->modType == CG_MPFA_TYPE_PAD)
					{
						//An existing PAD module is skipped / no copied.
						pCurrentSource = pCurrentSource + ((((CG_MPFA_MODULE_HEADER *)pCurrentSource)->modSize) / 4);
					}
					
					////////////////////////////////////////////////////////////////////////
					// End of PAD module handling
					////////////////////////////////////////////////////////////////////////	
																				//MOD008 ^
                    // We have found a valid and used module, so store it in the temporary section buffer.
                    nCopyLength = (((CG_MPFA_MODULE_HEADER *)pCurrentSource)->modSize) / 4;
                    for (nCount = 0; nCount < nCopyLength; nCount++)
                    {
                        *pCurrentDest = *pCurrentSource;
                        pCurrentDest = pCurrentDest + 1;
                        pCurrentSource = pCurrentSource + 1;
                    }
                }
                else
                {
                    // The module is not used, so we don't copy it; however as we came here
                    // the module entry was valid, thus we can skip the module as a whole.
                    pCurrentSource = pCurrentSource + ((((CG_MPFA_MODULE_HEADER *)pCurrentSource)->modSize) / 4);
                }
            }
            else
            {
                // Go to next DWORD to check for MPFA header.
                pCurrentSource = pCurrentSource + 1;
            }
        }
        else
        {
            // Go to next DWORD to check for MPFA header.
            pCurrentSource = pCurrentSource + 1;
        }            
    }while(pCurrentSource < (UINT32 *)(pSectionInfo->pSectionBuffer + pSectionInfo->sectionSize));

   
    // Set new ADD index for this section 
    pSectionInfo->addIndex = (UINT32)((unsigned char*)pCurrentDest - (unsigned char*)pTempSectionBuffer);
        
    //
    // Write back temporary section buffer to original section buffer
    //
    
    // Set start pointers
    pCurrentSource = (UINT32 *)pSectionInfo->pSectionBuffer;
    pCurrentDest = pTempSectionBuffer;

    // Set copy length to copy the whole section
    nCopyLength = (pSectionInfo->sectionSize) / 4;

    for(nCount = 0; nCount < nCopyLength; nCount++)
    {
        *pCurrentSource = *pCurrentDest;
        pCurrentDest = pCurrentDest + 1;
        pCurrentSource = pCurrentSource + 1;        
    }
        
	free(pTempSectionBuffer);													//MOD008
    return CG_MPFARET_OK;        

}

/*---------------------------------------------------------------------------
 * Name: CgMpfaFindModule
 * Desc: Find specified module in  selected section according to search flags.
 * Inp:  pSectionInfo   - Pointer to info block of the section to be searched 
 *       pMpfaHeader    - Pointer to MPFA header describing the module to be
 *                        found
 *       nStartIndex    - Index in specified section where search should begin
 *       pFoundIndex    - Pointer to value to store the index in the section
 *                        where the specified module has been found
 *       nSearchFlags   - Flags controlling search procedure
 *
 * Outp: return code:
 *       CG_MPFARET_NOTFOUND    - Error, module not found
 *       CG_MPFARET_ERROR       - Execution error
 *       CG_MPFARET_OK          - Success, module found
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaFindModule
(
    CG_MPFA_SECTION_INFO *pSectionInfo, 
    CG_MPFA_MODULE_HEADER *pMpfaHeader,
    UINT32 nStartIndex,
    UINT32 *pFoundIndex,
    UINT32 nSearchFlags
)
{
    UINT16 retVal;
    unsigned char *pCurrent;
    UINT32 nTempOffset;
    UINT32 nIndex;
    
    //Check index parameter
    if(nStartIndex > (pSectionInfo->sectionSize - sizeof(UINT32)))
    {
        return CG_MPFARET_ERROR;
    }
    // Add index to section buffer pointer to create start address.
    pCurrent = pSectionInfo->pSectionBuffer + nStartIndex;
    
    // Indicate that we have not found the specified module.
    retVal = CG_MPFARET_NOTFOUND;
    do
    {
        if (((CG_MPFA_MODULE_HEADER *)pCurrent)->hdrID != CG_MPFA_MOD_HDR_ID)
        {
            // No more modules found.
            return CG_MPFARET_NOTFOUND;
        }
        //Module header found, now check for module end structure.
        nTempOffset = ((CG_MPFA_MODULE_HEADER *)pCurrent)->modSize - sizeof(localMpfaEnd);
        if (((CG_MPFA_MODULE_END *)(pCurrent + nTempOffset))->endID != CG_MPFA_MOD_END_ID)
        {
            // No more modules found.
            return CG_MPFARET_NOTFOUND;
        }
        // We have found a valid module entry, no check whether it is USED;
        // if not, we continue with the next module.            
        if (((CG_MPFA_MODULE_HEADER *)pCurrent)->modFlags & CG_MOD_ENTRY_USED)
        {
            // We have found a valid and used module, now check whether it's the right one.
            if(nSearchFlags == CG_MPFACMP_TYPE)
            {
                if(((CG_MPFA_MODULE_HEADER *)pCurrent)->modType == pMpfaHeader->modType)
                {
                    // Leave scan loop, we have found our module!
                    retVal = CG_MPFARET_OK;
                    break;   
                }
            }
            else if(nSearchFlags == (CG_MPFACMP_TYPE| CG_MPFACMP_ID))
            {
                if(((CG_MPFA_MODULE_HEADER *)pCurrent)->modType == pMpfaHeader->modType)
                {
                    if(((CG_MPFA_MODULE_HEADER *)pCurrent)->modID == pMpfaHeader->modID)
                    {
                        // Leave scan loop, we have found our module!
                        retVal = CG_MPFARET_OK;
                        break;   
                    }    
                }
            }
            else if(nSearchFlags == 0)
            {
                // If no search flags are specified we simply look for the next used module
                retVal = CG_MPFARET_OK;
                break;   
            }
            else if(nSearchFlags == 0xFFFFFFFF)
            {
                // All fields of the MPFA header must match
                retVal = CG_MPFARET_OK;
                for(nIndex=0; nIndex < sizeof(localMpfaHdr); nIndex++)
                {
                    if(*(pCurrent + nIndex) != *(((unsigned char *)pMpfaHeader) + nIndex))
                    {
                        retVal = CG_MPFARET_NOTFOUND;
                        break;  // Leave inner compare loop
                    }
                }
                if(retVal == CG_MPFARET_OK)
                {
                    // Leave outer scan loop, we have found our module!
                    break;   
                }
            }
            else
            {
                // GWETODO: Currently more detailed compare modes are not supported !
                return CG_MPFARET_NOTFOUND;
            }
        }
        // Set current pointer to next module in list.
        pCurrent = pCurrent + ((CG_MPFA_MODULE_HEADER *)pCurrent)->modSize;
            
    }while(pCurrent < pSectionInfo->pSectionBuffer + pSectionInfo->sectionSize);
    
    if(retVal == CG_MPFARET_OK)
    {
        // We have found the specified module, now set the return values.
        *pFoundIndex = (UINT32) (pCurrent - pSectionInfo->pSectionBuffer);
    }
    
    return retVal;;        
}

/*---------------------------------------------------------------------------
 * Name: CgMpfaCreateModule
 * Desc: Create a valid MPFA module using values specified in the MPFA header
 *       and data passed in input file. The result is written to the 
 *       output file.
 * Inp:  pMpfaHeader    - pointer to MPFA header specifying the 
 *                        module that should be added
 *       pInputFilename - pointer to input file name containing module data
 *       pOutputFilename - pointer to file name of the output file the
 *                         created MPFA module should be written to
 *       nAccessLevel   - Specify access level; may reject function execution 
 *       bSkipDataCheck - if set to TRUE the module contents is not checked
 *
 * Outp: return code:
 *      CG_MPFARET_OK               - Success
 *      CG_MPFARET_NOTALLOWED       - Operation not allowed with current
 *                                    access level
 *      CG_MPFARET_ERROR            - Execution error
 *      CG_MPFARET_ERROR_FILE       - Module input file processing error
 *      CG_MPFARET_INCOMP           - Module incompatible to operation target 
 *      CG_MPFARET_INV              - Invalid MPFA module
 *      CG_MPFARET_INV_DATA         - Invalid MPFA module data
 *      CG_MPFARET_INV_PARM         - Invalid MPFA module parameters
 *---------------------------------------------------------------------------
 */
extern UINT16 CgMpfaCreateModule
(
    CG_MPFA_MODULE_HEADER *pMpfaHeader, 
    _TCHAR *pInputFilename, 
    _TCHAR *pOutputFilename,
    UINT16 nAccessLevel,
    UINT16 bSkipDataCheck
)
{
    UINT32 nDataSize, nDataSizeFile, nChkIdx;
    UINT16 retVal;
    unsigned char *pTempModuleBuffer;
    FILE *fpInDatafile = NULL;
	FILE *fpOutDatafile = NULL;
    INT32 lTempFileSize;
    UINT16 nChkSum = 0;
    UINT32 * pTempUINT32;
    CG_MPFA_MODULE_HEADER TempModuleHeader;

    // Try to open input data file.   
    if(!(fpInDatafile = fopen(pInputFilename, "rb")))
    {
        return CG_MPFARET_ERROR_FILE;
    }
																				//MOD008 v
	// CG_MPFA_TYPE_OEM_SMBIOS_DATA module creation requires special pre-processing,
	// as a human-editable text file has to be converted into a binary file first before
	// eventually creating an MPFA module.
	if(((CG_MPFA_MODULE_HEADER *)pMpfaHeader)->modType == CG_MPFA_TYPE_OEM_SMBIOS_DATA)			
	{
		// This routine parses the current SMBIOS DMS text input file and 
		// creates a new binary representation of the contents stored in
		// a file called "tmp.bin".
		if((ini_parse_file(fpInDatafile,convert,NULL)) != 0)
		{
			fclose(fpInDatafile);
			return CG_MPFARET_INV_DATA;
		}
		// Close original DMS text input file and open the correponding binary representation.
		// Use this as new MPFA module data input for further processing.
		fclose(fpInDatafile);
		if(!(fpInDatafile = fopen("tmp.bin", "rb"))) 
		{
			return CG_MPFARET_ERROR_FILE;
		}
	}

	// CG_MPFA_TYPE_SETUP_MENU_SETTINGS module creation requires special pre-processing.
	// As input for this type of module an MLF file is used which contains the complete 
	// setup information (layout, strings, ...) and a change / modification addon package.
	// For the MPFA module only the changes are needed and have to be extracted. 
	if(((CG_MPFA_MODULE_HEADER *)pMpfaHeader)->modType == CG_MPFA_TYPE_SETUP_MENU_SETTINGS)			
	{
		// This routine parses the current MLF setup information input file stores the 
		// modification data only in a new file called "tmp.bin".
		if((CgSetupMenuDataExtract(pMpfaHeader, fpInDatafile)) != CG_MPFARET_OK)
		{
			fclose(fpInDatafile);
			return CG_MPFARET_INV_DATA;
		}
		// Close original setup information input file and open the stripped down diff file.
		// Use this as new MPFA module data input for further processing.
		fclose(fpInDatafile);
		if(!(fpInDatafile = fopen("tmp.bin", "rb"))) 
		{
			return CG_MPFARET_ERROR_FILE;
		}
	}																			//MOD008 ^

    // Get length of data block and check whether it is DWORD aligned!
    // Set file pointer to end of file
    if(!fseek(fpInDatafile,0, SEEK_END))
    {
        // Get position at end of file = file length
        if((lTempFileSize = ftell(fpInDatafile)) >= 0)
        {
            // Set file pointer back to start of file
            if(!fseek(fpInDatafile,0, SEEK_SET))
            {
                // Save raw data file length            
                nDataSizeFile = (UINT32) lTempFileSize;
                retVal = CG_MPFARET_OK;       
            }
            else
            {
                retVal = CG_MPFARET_ERROR_FILE;
            }
        }
        else
        {
            retVal = CG_MPFARET_ERROR_FILE;
        }
    }
    else
    {
        retVal = CG_MPFARET_ERROR_FILE;
    }
    
    if(retVal != CG_MPFARET_OK)
    {
        fclose(fpInDatafile);
        return retVal;
    }
   
    // Ensure data block length is DWORD aligned
    if(nDataSizeFile & 0x00000003)
    {
        //Force DWORD alignment
        nDataSize = (nDataSizeFile & 0xFFFFFFFC) + 4;
    }
    else
    {
        nDataSize = nDataSizeFile;
    }
    
    // Allocate temporary buffer to hold MPFA module
    pTempModuleBuffer = (unsigned char*)malloc(nDataSize + sizeof(localMpfaHdr) + sizeof(localMpfaEnd));
    if(pTempModuleBuffer == NULL)
    {
        fclose(fpInDatafile);
        return CG_MPFARET_ERROR;
    }

    // Copy module header to temporary module buffer
    *((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer) = *pMpfaHeader;


    // If we want to create a CMOS default/fixed map we need a CMOS backup map module as input.
    // Therefore a different handling is required.
    if((((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modType == CG_MPFA_TYPE_CMOS_DEFAULT)||
        (((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modType == CG_MPFA_TYPE_CMOS_FIXED))
    {
        // First assume that we can read a module header from the file.
        fread(&TempModuleHeader, sizeof(TempModuleHeader), 1, fpInDatafile );
        if( ferror( fpInDatafile ) )      
        {
            fclose(fpInDatafile);
            free(pTempModuleBuffer);
            return CG_MPFARET_ERROR_FILE;
        }
        
        // Here we perform a simple check whether the type is correct and the BIOS version matches
        if( (TempModuleHeader.hdrID != CG_MPFA_MOD_HDR_ID) ||  (TempModuleHeader.modType != CG_MPFA_TYPE_CMOS_BACKUP) )
        {
            fclose(fpInDatafile);
            free(pTempModuleBuffer);
            return CG_MPFARET_INV_DATA;
        }
        // Now we know, that the input file is probably a CMOS backup file. Now check the BIOS version.
        pTempUINT32 = (UINT32*)&(CgMpfaBiosInfo.biosVersion);
        if ((TempModuleHeader.modLoadAddr != *pTempUINT32) || (TempModuleHeader.modEntryOff != *(pTempUINT32 + 1)))
        {
            fclose(fpInDatafile);
            free(pTempModuleBuffer);
            return CG_MPFARET_INCOMP;
        }

        // Now that everything is fine, adjust the data size by subtracting header and end structure
        nDataSizeFile = nDataSizeFile - sizeof(TempModuleHeader) - sizeof(localMpfaEnd);
        nDataSize = nDataSize - sizeof(TempModuleHeader) - sizeof(localMpfaEnd);
    }

    // Insert correct module length
    ((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modSize = 
            nDataSize + sizeof(localMpfaHdr) + sizeof(localMpfaEnd);
    
    // Set USED flag
    ((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modFlags = 
        ((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modFlags | CG_MOD_ENTRY_USED;

    // Load module data to module buffer
    fread((pTempModuleBuffer + sizeof(localMpfaHdr)), nDataSizeFile, 1, fpInDatafile );
    if( ferror( fpInDatafile ) )      
    {
        fclose(fpInDatafile);
        free(pTempModuleBuffer);
        return CG_MPFARET_ERROR_FILE;
    }
    else
    {
        // Add module end structure
        *((CG_MPFA_MODULE_END *)(pTempModuleBuffer + sizeof(localMpfaHdr) + nDataSize)) = localMpfaEnd;
    
        // Calculate module data checksum (WORD storage, bytes added)
        for(nChkIdx = 0; nChkIdx < nDataSize; nChkIdx++)
        {
            nChkSum = nChkSum +  
            *(pTempModuleBuffer + nChkIdx + sizeof(localMpfaHdr));                
        }
        // Store checksum in MPFA header
        ((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modChkSum = nChkSum;
        retVal = CG_MPFARET_OK;
    }
    
    // Close input file
    fclose(fpInDatafile);
														    								
	// If an OEM smbios or setup menu MPFA module has been created,			//MOD008	v	
	// we have to remove the temporary file created in this processing. 
//	remove("tmp.bin");														//MOD008 ^
																						

    // We have created a module in our local buffer, now check whether it is valid !
    // In case it is a CMOS default map, fixed CMOS map, string table map or OEM setup map 
    // we first have to patch the BIOS version it is meant for !
    if((((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modType == CG_MPFA_TYPE_CMOS_DEFAULT) ||
        (((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modType == CG_MPFA_TYPE_CMOS_FIXED) ||
        (((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modType == CG_MPFA_TYPE_SETUP) ||
        (((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modType == CG_MPFA_TYPE_STRING))
    {
        pTempUINT32 = (UINT32*)&(CgMpfaBiosInfo.biosVersion);
        ((CG_MPFA_MODULE_HEADER*)pTempModuleBuffer)->modLoadAddr = *pTempUINT32;
        pTempUINT32 = pTempUINT32 + 1;
        ((CG_MPFA_MODULE_HEADER*)pTempModuleBuffer)->modEntryOff = *pTempUINT32; 
    }

    // Validate module !
    if(bSkipDataCheck == FALSE)
    {
        if((retVal = CgMpfaModuleTypeSpecificCheck(pTempModuleBuffer)) != CG_MPFARET_OK )
        {
            free(pTempModuleBuffer);
            return retVal;
        }
    }
    
    // We have created a module in our local buffer, now write it to the output file
    // Open the output file and save data.
    if (!(fpOutDatafile = fopen(pOutputFilename, "wb")))
    {
        retVal = CG_MPFARET_ERROR_FILE;
    }
    else 
    {
        if(fwrite(pTempModuleBuffer, sizeof(unsigned char),
            ((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modSize, fpOutDatafile ) 
            != ((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modSize)
        {
            retVal = CG_MPFARET_ERROR_FILE;
        }
        else
        {
            retVal = CG_MPFARET_OK;
        }
        fclose(fpOutDatafile);
    }

    // Release module buffer
    free(pTempModuleBuffer);

    return retVal;
}

/*---------------------------------------------------------------------------
 * Name: CgMpfaAddModule
 * Desc: Add specified module to MPFA section and update section addIndex
 *       (= index in section where new modules can be added)
 * Inp:  pInputFilename - pointer to MPFA module file
 *       nAccessLevel   - Specify access level; may reject function execution 
 *       bSkipDataCheck - if set to TRUE the module contents is not checked
 *
 * Outp: return code:
 *       CG_MPFARET_OK              - Success
 *       CG_MPFARET_NOTALLOWED      - Operation not allowed with current
 *                                    access level
 *      CG_MPFARET_ERROR            - Execution error
 *      CG_MPFARET_ERROR_FILE       - Module input file processing error
 *      CG_MPFARET_ERROR_SIZE       - Not enough room to add module
 *      CG_MPFARET_INTRF_ERROR      - Interface access error
 *      CG_MPFARET_INCOMP           - Module incompatible to operation target 
 *      CG_MPFARET_INV              - Invalid MPFA module
 *      CG_MPFARET_INV_DATA         - Invalid MPFA module data
 *      CG_MPFARET_INV_PARM         - Invalid MPFA module parameters
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaAddModule
(   
    _TCHAR *pInputFilename, 
    UINT16 nAccessLevel,
    UINT16 bSkipDataCheck
)
{
    
    UINT32 nTypeCount, nSectionCount, nSectionType, nFoundIndex, nDataSize, 
                    nTempOffset, nCount, nCompFlags,  
					nPadModuleSize, nPadModuleDataSize, nAlignment;				//MOD006 MOD008
    UINT16 retVal;
    CG_MPFA_SECTION_INFO *pTempInfo;
    unsigned char *pTempSectionBuffer;
    unsigned char *pTempModuleBuffer;
	unsigned char *pPadData;													//MOD006
    FILE *fpInDatafile = NULL;
    INT32 lTempFileSize;
	UINT32 *pTempUINT32;														//MOD008
   

    // Try to open input data file.   
    if(!(fpInDatafile = fopen(pInputFilename, "rb")))
    {
        return CG_MPFARET_ERROR_FILE;
    }

    // Get length of file and check whether it is DWORD aligned!
    // Set file pointer to end of file
    if(!fseek(fpInDatafile,0, SEEK_END))
    {
        // Get position at end of file = file length
        if((lTempFileSize = ftell(fpInDatafile)) >= 0)
        {
            // Set file pointer back to start of file
            if(!fseek(fpInDatafile,0, SEEK_SET))
            {
                // Save raw data file length            
                nDataSize = (UINT32) lTempFileSize;
                retVal = CG_MPFARET_OK;       
            }
            else
            {
                retVal = CG_MPFARET_ERROR_FILE;
            }
        }
        else
        {
            retVal = CG_MPFARET_ERROR_FILE;
        }
    }
    else
    {
        retVal = CG_MPFARET_ERROR_FILE;
    }

    // Check whether file length is DWORD aligned
    if(nDataSize & 0x00000003)
    {
        // Valid MPFA modules must at least be DWORD aligned !
        retVal = CG_MPFARET_INV;
    }
    
    if(retVal != CG_MPFARET_OK)
    {
        fclose(fpInDatafile);
        return retVal;
    }

    // Allocate temporary buffer to hold MPFA module
    pTempModuleBuffer = (unsigned char*)malloc(nDataSize);
    if(pTempModuleBuffer == NULL)
    {
        fclose(fpInDatafile);
        return CG_MPFARET_ERROR;
    }

    // Load module data to module buffer
    fread(pTempModuleBuffer, nDataSize, 1, fpInDatafile );
    if( ferror( fpInDatafile ) )      
    {
        fclose(fpInDatafile);
        free(pTempModuleBuffer);
        return CG_MPFARET_ERROR_FILE;
    }
    fclose(fpInDatafile);

    // Check whether module is valid
    retVal = CG_MPFARET_INV;
    if (((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->hdrID == CG_MPFA_MOD_HDR_ID)
    {
       //Module header found, now check for module end structure.
        nTempOffset = ((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modSize - sizeof(localMpfaEnd);
        if (((CG_MPFA_MODULE_END *)(pTempModuleBuffer + nTempOffset))->endID == CG_MPFA_MOD_END_ID)
        {
            // We have found a valid module entry, now check whether it is USED;
            // if not, we will not add it.            
            if (((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modFlags & CG_MOD_ENTRY_USED)
            {
                retVal = CG_MPFARET_OK;
            }        
        } 
    }
    if(retVal != CG_MPFARET_OK)
    {
        free(pTempModuleBuffer);
        return retVal;
    }
    // Common module check has been passed, now perform module type specific checks.
    if(bSkipDataCheck == FALSE)
    {
        if((retVal = CgMpfaModuleTypeSpecificCheck(pTempModuleBuffer)) != CG_MPFARET_OK)
        {
            free(pTempModuleBuffer);
            return retVal;
        }
    }
    
    // We have a valid module, now try to add it to the correct MPFA section
																				
	// Do not allow standard user to add a standard VBIOS module.				//MOD002 v
#ifndef INTERN																	//MOD004
	if((((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modType == CG_MPFA_TYPE_VBIOS_STD) && (nAccessLevel == CGUTL_ACC_LEV_USER))
	{
		free(pTempModuleBuffer);
		return CG_MPFARET_NOTALLOWED;
	}
#endif																			//MOD004
																				//MOD002 ^ 
	//Never allow to manually add a PAD module									//MOD00 v
	if( ((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modType == CG_MPFA_TYPE_PAD )
	{
		free(pTempModuleBuffer);
		return CG_MPFARET_NOTALLOWED;
	}																			//MOD008 ^

    // First find the respective entry in g_MpfaTypeList.
    for(nTypeCount = 0; nTypeCount < g_nNoMpfaTypes; nTypeCount++)
    {
        if(g_MpfaTypeList[nTypeCount].type == ((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modType)
        {
            // Here we have found the corresponding entry in g_MpfaTypeList.
            nSectionType = g_MpfaTypeList[nTypeCount].sectionType;
            break;  //Leave FOR loop.
        }
        if(nTypeCount == g_nNoMpfaTypes - 1)
        {
            // If we come here, we did not find the specified MPFA module type.
            free(pTempModuleBuffer);
            return CG_MPFARET_INV;
        }
    }
    
    // Now that we have the section type, find the respective entry in g_MpfaSectionList.
    for(nSectionCount = 0; nSectionCount < g_nNoMpfaSections; nSectionCount++)
    {
        pTempInfo = g_MpfaSectionList[nSectionCount];
        if(pTempInfo->sectionType == nSectionType)
        {
            // Here we have found the corresponding entry in g_MpfaSectionList.
            break;  //Leave FOR loop.
        }
        if(nSectionCount == g_nNoMpfaSections -1)
        {
            // If we come here, we did not find the specified MPFA section info.
			free(pTempModuleBuffer);											//MOD008
            return CG_MPFARET_ERROR;
        }
    }

    // Certain module types only allow one instance. Thus we have to find an 
    // existing module and delete it, before we add the new module.
    if((((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modType == CG_MPFA_TYPE_CMOS_BACKUP) || 
       (((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modType == CG_MPFA_TYPE_PDA))
    {
        nCompFlags = CG_MPFACMP_TYPE;
    }
    else
    {
        nCompFlags = (CG_MPFACMP_TYPE | CG_MPFACMP_ID);
    }
    // Due to a preceeding error there might be more than one module in a section, 
    // so make sure to loop and delete all of them.
    do
    {
        retVal = CgMpfaFindModule(pTempInfo,((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer), 0, &nFoundIndex, nCompFlags);
        if(retVal == CG_MPFARET_OK)
        {
            //PRINTF("Module %X found at index %X in section %X\n", ((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modType, nFoundIndex, pTempInfo->sectionType );
            // We have found the module, now set it to UNUSED.
            pTempSectionBuffer = (pTempInfo->pSectionBuffer + nFoundIndex);
            ((CG_MPFA_MODULE_HEADER *)pTempSectionBuffer)->modFlags = ((CG_MPFA_MODULE_HEADER *)pTempSectionBuffer)->modFlags & (~CG_MOD_ENTRY_USED);
    
            // Launch rebuild of the section to really remove the module 
            if(CgMpfaRebuildSection(pTempInfo) != CG_MPFARET_OK)
            {
				free(pTempModuleBuffer);											//MOD008
                return CG_MPFARET_ERROR;
            }
        }
    }while(retVal == CG_MPFARET_OK);
   
    if((pTempInfo->addIndex != 0xFFFFFFFF) && 
        (pTempInfo->sectionSize >= pTempInfo->addIndex +  ((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modSize ))
    {
        // Prepare to copy module from temporary buffer to section buffer
        pTempSectionBuffer = (pTempInfo->pSectionBuffer + pTempInfo->addIndex);

																				//MOD006 MOD008 v
		////////////////////////////////////////////////////////////////////////
		// Special handling for OEM UEFI FV modules. Make sure that the data part 
		// of the MPFA module, i.e. the FV is placed aligned according to the 
		// requirements specified in the FV header.
		// This is achieved by placing a dummy pad module in front of the MPFA FV 
		// module with a data block size that shifts the FV to the required alignment.
		if(((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modType == CG_MPFA_TYPE_FIRMWARE_VOLUME)			
		{
			// Derive FV alignment requirement from FV header.
			pTempUINT32 = ((UINT32 *)(pTempModuleBuffer + sizeof(localMpfaHdr)));
			nAlignment = (UINT32)pow(2.0, (INT32)(*(pTempUINT32+0xb) & 0x001F0000) >> 16);

			//Ensure again, that requested FV alignment is within supported range (already done in module specific check before)
			if( (nAlignment < FIRMWARE_VOLUME_MIN_ALIGN) || (nAlignment > FIRMWARE_VOLUME_MAX_ALIGN))
            {
                free(pTempModuleBuffer);
                return CG_MPFARET_INV_DATA;
            }
			
			// For simplicity, we always add a PAD module in front of a FV module.
			// Thus we only have to calculate the data size of the PAD module, and always have  
			// the same handling.
			// 
			nPadModuleDataSize = (nAlignment - (  (pTempInfo->addIndex + sizeof(localMpfaHdr) + sizeof(localMpfaHdr) + sizeof(localMpfaEnd)) 
													& ((UINT32)(nAlignment-1))  ));

			// All modules have to be DWORD aligned. Thus if we get a non-DWORD			
			// aligned padding requirement, something must be wrong.											
			if(nPadModuleDataSize & 0x00000003)								
			{
				free(pTempModuleBuffer);
				return CG_MPFARET_ERROR;
			}																			
			nPadModuleSize = nPadModuleDataSize + sizeof(localMpfaHdr) + sizeof(localMpfaEnd);

			// Check whether our pad module + the original module can still be placed.
			if( (pTempInfo->sectionSize >= pTempInfo->addIndex +  ((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modSize ) + nPadModuleSize)
			{
				// Copy the pad module
				pPadData = (unsigned char *)&localMpfaHdr;
				for(nCount = 0; nCount < sizeof(localMpfaHdr) / 4; nCount++)
				{
					*((UINT32 *)pTempSectionBuffer + nCount) = 
						*((UINT32 *)pPadData + nCount);
				}
				// Adapt the pad module header data. 
				((CG_MPFA_MODULE_HEADER *)pTempSectionBuffer)->modType = CG_MPFA_TYPE_PAD;
				((CG_MPFA_MODULE_HEADER *)pTempSectionBuffer)->modSize = nPadModuleSize;
				((CG_MPFA_MODULE_HEADER *)pTempSectionBuffer)->modFlags |= CG_MOD_ENTRY_USED;
				pTempSectionBuffer = pTempSectionBuffer + sizeof(localMpfaHdr);

				// Fill PAD area
				if(nPadModuleDataSize >= 4)
				{
					for(nCount = 0; nCount < nPadModuleDataSize / 4; nCount++)
					{
						*((UINT32 *)pTempSectionBuffer + nCount) = 0xFFFFFFFF;
					}
				}
				pTempSectionBuffer = pTempSectionBuffer + nPadModuleDataSize;
				
				pPadData = (unsigned char *)&localMpfaEnd;
				for(nCount = 0; nCount < sizeof(localMpfaEnd) / 4; nCount++)
				{
					*((UINT32 *)pTempSectionBuffer + nCount) = 
						*((UINT32 *)pPadData + nCount);
				}
				pTempSectionBuffer = pTempSectionBuffer + sizeof(localMpfaEnd);

				
				// Adjust add index for this section.
				pTempInfo->addIndex = pTempInfo->addIndex + nPadModuleSize;
			}
			else
			{
				free(pTempModuleBuffer);
				return CG_MPFARET_ERROR_SIZE;
			}			
		}																		//MOD006^ 
		////////////////////////////////////////////////////////////////////////
		// End of PAD module handling
		////////////////////////////////////////////////////////////////////////

		//
		// Actually copy the target module from the temporary buffer to 
		// the respective section buffer.										//MOD008 ^
        for(nCount = 0; nCount < nDataSize / 4; nCount++)
        {
            *((UINT32 *)pTempSectionBuffer + nCount) = 
                *((UINT32 *)pTempModuleBuffer + nCount);
        }

        // Adjust add index for this section.
        pTempInfo->addIndex = pTempInfo->addIndex + nDataSize;
        retVal = CG_MPFARET_OK;
    }
    else
    {
        retVal = CG_MPFARET_ERROR_SIZE;
    }
 
    free(pTempModuleBuffer);

    // Now go and mark the section as modified by marking the ROOT module
    // as modified. If there is no ROOT module, simply do nothing.
    localMpfaHdr.modType = CG_MPFA_TYPE_ROOT;
    if(CgMpfaFindModule(pTempInfo,&localMpfaHdr, 0, &nFoundIndex, CG_MPFACMP_TYPE) == CG_MPFARET_OK)
    {
        //
        pTempSectionBuffer = (pTempInfo->pSectionBuffer + nFoundIndex);
        ((CG_MPFA_MODULE_HEADER *)pTempSectionBuffer)->modFlags = 
            ((CG_MPFA_MODULE_HEADER *)pTempSectionBuffer)->modFlags & (~CG_MOD_ENTRY_MODIFIED);
    }
    
    return retVal;
}


/*---------------------------------------------------------------------------
 * Name: CgMpfaDelModule
 * Desc: Find and delete specified module       
 * Inp:  pMpfaHeader    - Pointer to MPFA header specifying the 
 *                        module that should be deleted
 *       nSearchFlags   - Flags controlling search procedure
 *       nAccessLevel   - Specify access level; may reject function execution 
 *
 * Outp: return code:
 *       CG_MPFARET_OK          - Success, module deleted
 *       CG_MPFARET_NOTALLOWED  - Operation not allowed with current
 *                                access level
 *       CG_MPFARET_ERROR       - Execution error
 *       CG_MPFARET_NOTFOUND    - Error, module not found 
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaDelModule
(
    CG_MPFA_MODULE_HEADER *pMpfaHeader,
    UINT32 nSearchFlags,
    UINT16 nAccessLevel
)
{
    UINT32 nTypeCount, nSectionCount, nSectionType, nFoundIndex;
    CG_MPFA_SECTION_INFO* pTempInfo;
    CG_MPFA_MODULE_HEADER *pTempMpfaHeader;
    UINT16 retVal;
    
    // First find the respective entry in g_MpfaTypeList.
    for(nTypeCount = 0; nTypeCount < g_nNoMpfaTypes; nTypeCount++)
    {
        if(g_MpfaTypeList[nTypeCount].type == pMpfaHeader->modType)
        {
            // Here we have found the corresponding entry in g_MpfaTypeList.
            nSectionType = g_MpfaTypeList[nTypeCount].sectionType;
            break;  //Leave FOR loop.
        }
        if(nTypeCount == g_nNoMpfaTypes - 1)
        {
            // If we come here, we did not find the specified MPFA module type.
            return CG_MPFARET_NOTFOUND;
        }
    }
    
    // Now that we have the section type, find the respective entry in g_MpfaSectionList.
    for(nSectionCount = 0; nSectionCount < g_nNoMpfaSections; nSectionCount++)
    {
        pTempInfo = g_MpfaSectionList[nSectionCount];
        if(pTempInfo->sectionType == nSectionType)
        {
            // Here we have found the corresponding entry in g_MpfaSectionList.
            break;  //Leave FOR loop.
        }
        if(nSectionCount == g_nNoMpfaSections -1)
        {
            // If we come here, we did not find the specified MPFA section info.
            return CG_MPFARET_ERROR;
        }
    }
 
    // Go and find module that should be deleted.   
    retVal = CgMpfaFindModule(pTempInfo,pMpfaHeader, 0, &nFoundIndex, nSearchFlags);
    if(retVal == CG_MPFARET_OK)
    {
        // Check whether operation is allowed with current access level

        // Do not allow standard user to delete the EPI module or a standard VBIOS module or a PAD module.
        if(((pMpfaHeader->modType == CG_MPFA_TYPE_PDA) || (pMpfaHeader->modType == CG_MPFA_TYPE_VBIOS_STD)	//MOD001
			|| (pMpfaHeader->modType == CG_MPFA_TYPE_PAD)) && (nAccessLevel == CGUTL_ACC_LEV_USER))
        {
            return CG_MPFARET_NOTALLOWED;
        }
        
        //PRINTF("Module %X found at index %X in section %X\n", pMpfaHeader->modType, nFoundIndex, pTempInfo->sectionType );
        // We have found the module, now set it to UNUSED.
        pTempMpfaHeader = (CG_MPFA_MODULE_HEADER *)(pTempInfo->pSectionBuffer + nFoundIndex);
        pTempMpfaHeader->modFlags = pTempMpfaHeader->modFlags & (~CG_MOD_ENTRY_USED);
        
        // Launch rebuild of the section to really remove the module 
        retVal = CgMpfaRebuildSection(pTempInfo);   

        // Now go and mark the section as modified by marking the ROOT module
        // as modified. If there is no ROOT module, simply do nothing.
        localMpfaHdr.modType = CG_MPFA_TYPE_ROOT;
        if(CgMpfaFindModule(pTempInfo,&localMpfaHdr, 0, &nFoundIndex, CG_MPFACMP_TYPE) == CG_MPFARET_OK)
        {
            //
            pTempMpfaHeader = (CG_MPFA_MODULE_HEADER *)(pTempInfo->pSectionBuffer + nFoundIndex);
            pTempMpfaHeader->modFlags = pTempMpfaHeader->modFlags & (~CG_MOD_ENTRY_MODIFIED);
        }
    }
    return retVal;
}

/*---------------------------------------------------------------------------
 * Name: CgMpfaSaveModule
 * Desc: Find specified module and save module or module data to output file.       
 * Inp:  pMpfaHeader     - pointer to MPFA header specifying the 
 *                         module that should be saved
 *       pOutputFilename - pointer to output file name for module data
 *       nSearchFlags   - Flags controlling search procedure
 *       nSaveFlags     - Specify save handling
 *                        CG_MPFASFL_MOD: Save whole module
 *                        CG_MPFASFL_DATA: Save module data only
 * Outp: return code:
 *       CG_MPFARET_OK          - Success, module saved
 *       CG_MPFARET_ERROR       - Execution error
 *       CG_MPFARET_INTRF_ERROR - Interface access error
 *       CG_MPFARET_NOTFOUND    - Error, module not found 
 *       CG_MPFARET_ERROR_FILE  - file processing error
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaSaveModule
(
    CG_MPFA_MODULE_HEADER *pMpfaHeader,
    _TCHAR *pOutputFilename,
    UINT32 nSearchFlags,
    UINT32 nSaveFlags
)
{
    UINT32 nTypeCount, nSectionCount, nSectionType, nFoundIndex, nDataSize,
                  nTempmodLoadAddr, nTempmodEntryOff;
    CG_MPFA_SECTION_INFO* pTempInfo;
    UINT16 retVal;
    unsigned char *pTempData;
    FILE *fpOutDatafile = NULL;
    UINT32 * pTempUINT32;
    
    // First find the respective entry in g_MpfaTypeList.
    for(nTypeCount = 0; nTypeCount < g_nNoMpfaTypes; nTypeCount++)
    {
        if(g_MpfaTypeList[nTypeCount].type == pMpfaHeader->modType)
        {
            // Here we have found the corresponding entry in g_MpfaTypeList.
            nSectionType = g_MpfaTypeList[nTypeCount].sectionType;
            break;  //Leave FOR loop.
        }
        if(nTypeCount == g_nNoMpfaTypes - 1)
        {
            // If we come here, we did not find the specified MPFA module type.
            return CG_MPFARET_NOTFOUND;
        }
    }
    
    // Now that we have the section type, find the respective entry in g_MpfaSectionList.
    for(nSectionCount = 0; nSectionCount < g_nNoMpfaSections; nSectionCount++)
    {
        pTempInfo = g_MpfaSectionList[nSectionCount];
        if(pTempInfo->sectionType == nSectionType)
        {
            // Here we have found the corresponding entry in g_MpfaSectionList.
            break;  //Leave FOR loop.
        }
        if(nSectionCount == g_nNoMpfaSections -1)
        {
            // If we come here, we did not find the specified MPFA section info.
            return CG_MPFARET_ERROR;
        }
    }
 
    // Go and find module that should be saved.   
    retVal = CgMpfaFindModule(pTempInfo,pMpfaHeader, 0, &nFoundIndex, nSearchFlags);
    if(retVal == CG_MPFARET_OK)
    {
        // We have found the module, now get the data block and save it.
        pTempData = (pTempInfo->pSectionBuffer + nFoundIndex);
        nDataSize =  ((CG_MPFA_MODULE_HEADER *) pTempData)->modSize;
        if(nSaveFlags & CG_MPFASFL_DATA)
        {
            // We only want to save the data part of the module so modify
            // index and length
            nDataSize = (nDataSize - sizeof(localMpfaEnd)) - sizeof(localMpfaHdr);
            pTempData = pTempData + sizeof(localMpfaHdr);
        }        

        // When saving a CMOS backup module we have to insert the BIOS version string !
        if((pMpfaHeader->modType == CG_MPFA_TYPE_CMOS_BACKUP) && (!(nSaveFlags & CG_MPFASFL_DATA)))
        {
            // We do not want to implicitly modify the CMOS backup map of the operating
            // target, so we save and restore the original values.
            nTempmodLoadAddr = ((CG_MPFA_MODULE_HEADER*)pTempData)->modLoadAddr;
            nTempmodEntryOff = ((CG_MPFA_MODULE_HEADER*)pTempData)->modEntryOff; 
            pTempUINT32 = (UINT32*)&(CgMpfaBiosInfo.biosVersion);
            ((CG_MPFA_MODULE_HEADER*)pTempData)->modLoadAddr = *pTempUINT32;
            pTempUINT32 = pTempUINT32 + 1;
            ((CG_MPFA_MODULE_HEADER*)pTempData)->modEntryOff = *pTempUINT32; 
        }

        // Open the output file and save data.
        if (!(fpOutDatafile = fopen(pOutputFilename, "wb")))
        {
                return CG_MPFARET_ERROR_FILE;
        }
        if(fwrite(pTempData, sizeof(unsigned char),nDataSize, fpOutDatafile ) != nDataSize)
        {
            retVal = CG_MPFARET_ERROR_FILE;
        }
        fclose(fpOutDatafile);
        
        // Restore the original load address and entry offset fields of a saved CMOS backup module
        if((pMpfaHeader->modType == CG_MPFA_TYPE_CMOS_BACKUP) && (!(nSaveFlags & CG_MPFASFL_DATA)))
        {
            ((CG_MPFA_MODULE_HEADER*)pTempData)->modLoadAddr = nTempmodLoadAddr;
            ((CG_MPFA_MODULE_HEADER*)pTempData)->modEntryOff = nTempmodEntryOff; 
        }
    }
    return retVal;
}


/*---------------------------------------------------------------------------
 * Name: CgMpfaCmpModule
 * Desc: Compare module or module data in BIOS with module or module data
 *       passed as inpout file.
 * Inp:  pInputFilename     - pointer to MPFA module file or module data file.
 *       nAccessLevel       - Specify access level; may reject function execution 
 *       pMpfaHeader        - pointer to MPFA header specifying the 
 *                            module that should be compared
 *       nSearchFlags       - Flags controlling search procedure
 *
 * Outp: return code:
 *      CG_MPFARET_OK               - Success; module found and data matches
 *      CG_MPFARET_NOTFOUND         - Error, module not found 
 *      CG_MPFARET_NOMATCH          - Error, module data does not match 
 *      CG_MPFARET_NOTALLOWED       - Operation not allowed with current
 *                                    access level
 *      CG_MPFARET_ERROR            - Execution error
 *      CG_MPFARET_ERROR_FILE       - Module input file processing error
 *      CG_MPFARET_INTRF_ERROR      - Interface access error
 *      CG_MPFARET_INCOMP           - Module incompatible to operation target 
 *      CG_MPFARET_INV              - Invalid MPFA module
 *      CG_MPFARET_INV_DATA         - Invalid MPFA module data
 *      CG_MPFARET_INV_PARM         - Invalid MPFA module parameters
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaCmpModule
(
    _TCHAR *pInputFilename,
    CG_MPFA_MODULE_HEADER *pMpfaHeader,
    UINT32 nSearchFlags,
    UINT16 nAccessLevel
)
{
    
    UINT32 nTypeCount, nSectionCount, nSectionType, nFoundIndex, nDataSize, 
                  nCount, nCorrect, nOffset;
    UINT16 retVal;
    CG_MPFA_SECTION_INFO *pTempInfo;
    unsigned char *pTempSectionBuffer;
    unsigned char *pTempModuleBuffer, *pOrgTempModuleBuffer;
    FILE *fpInDatafile = NULL;
    INT32 lTempFileSize;
	UINT32 nlocalSearchFlags;													//MOD003
    

    // Try to open input data file.   
    if(!(fpInDatafile = fopen(pInputFilename, "rb")))
    {
        return CG_MPFARET_ERROR_FILE;
    }

    // Get length of file.
    // Set file pointer to end of file
    if(!fseek(fpInDatafile,0, SEEK_END))
    {
        // Get position at end of file = file length
        if((lTempFileSize = ftell(fpInDatafile)) >= 0)
        {
            // Set file pointer back to start of file
            if(!fseek(fpInDatafile,0, SEEK_SET))
            {
                // Save raw data file length            
                nDataSize = (UINT32) lTempFileSize;
                retVal = CG_MPFARET_OK;       
            }
            else
            {
                retVal = CG_MPFARET_ERROR_FILE;
            }
        }
        else
        {
            retVal = CG_MPFARET_ERROR_FILE;
        }
    }
    else
    {
        retVal = CG_MPFARET_ERROR_FILE;
    }
    
    if(retVal != CG_MPFARET_OK)
    {
        fclose(fpInDatafile);
        return retVal;
    }

    // In case we only have a data file as input file, we 'manually' have to add the MPFA header,
    // so increase the data size to reserve room for the MPFA header.
    // In case we have a MPFA module as input file this is not required !
    // By definition the inout file type can be derived from the search flags. In case
    // every single field has to be compared, we know, that we have a MPFA module file as input.
    if(nSearchFlags != CG_MPFACMP_ALL)
    {
        nDataSize = nDataSize + sizeof(localMpfaHdr) + sizeof(localMpfaEnd);
    }

    // Now try to allocate a temporary buffer to hold the input data.
    pTempModuleBuffer = (unsigned char*)malloc(nDataSize);
    if(pTempModuleBuffer == NULL)
    {
        fclose(fpInDatafile);
        return CG_MPFARET_ERROR;
    }
    // We will modify this pointer, so keep a copy of the original 
    // value to free the allocated memory if required.
    pOrgTempModuleBuffer = pTempModuleBuffer; 

    // In case we only have a data file as input file, we have to add the MPFA header passed
    // to this function.
    if(nSearchFlags != CG_MPFACMP_ALL)
    {
        *((CG_MPFA_MODULE_HEADER *) pTempModuleBuffer) = *pMpfaHeader;
        // Add offset and end correction to correct buffer index and data length.
        nOffset = sizeof(localMpfaHdr);
        nCorrect = sizeof(localMpfaEnd);
    }
    else
    {
        nOffset = 0;
        nCorrect = 0;
    }


    // Load module data to module buffer
    fread(pTempModuleBuffer + nOffset , nDataSize - (nOffset + nCorrect), 1, fpInDatafile );
    if( ferror( fpInDatafile ) )      
    {
        fclose(fpInDatafile);
        free(pOrgTempModuleBuffer);
        return CG_MPFARET_ERROR_FILE;
    }
    fclose(fpInDatafile);

    
    // First find the respective entry in g_MpfaTypeList.
    for(nTypeCount = 0; nTypeCount < g_nNoMpfaTypes; nTypeCount++)
    {
        if(g_MpfaTypeList[nTypeCount].type == ((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modType)
        {
            // Here we have found the corresponding entry in g_MpfaTypeList.
            nSectionType = g_MpfaTypeList[nTypeCount].sectionType;
            break;  //Leave FOR loop.
        }
        if(nTypeCount == g_nNoMpfaTypes - 1)
        {
            // If we come here, we did not find the specified MPFA module type.
            free(pOrgTempModuleBuffer);
            return CG_MPFARET_INV;
        }
    }
    
    // Now that we have the section type, find the respective entry in g_MpfaSectionList.
    for(nSectionCount = 0; nSectionCount < g_nNoMpfaSections; nSectionCount++)
    {
        pTempInfo = g_MpfaSectionList[nSectionCount];
        if(pTempInfo->sectionType == nSectionType)
        {
            // Here we have found the corresponding entry in g_MpfaSectionList.
            break;  //Leave FOR loop.
        }
        if(nSectionCount == g_nNoMpfaSections -1)
        {
            // If we come here, we did not find the specified MPFA section info.
            return CG_MPFARET_ERROR;
        }
    }

    // Now go and try to find the module that we have to compare with !
																				//MOD003 v
	// New UEFI based BIOSes modify parts of the MPFA module header of 
	// settings/backup map modules which will lead to a module header mismatch.
	// This in turn causes problems with the congatec tester as the settings module
	// comparison always returns an error, the settins are applied again and so on forever.
	// In order to solve this in an easy way we simply modify the module search and
	// comparison parameters here.
	if( ((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer)->modType == CG_MPFA_TYPE_CMOS_BACKUP )
	{
		nlocalSearchFlags = CG_MPFACMP_TYPE | CG_MPFACMP_ID; 
	}
	else
	{
		nlocalSearchFlags = nSearchFlags;
	}
																				//MOD003 ^
	if(CgMpfaFindModule(pTempInfo,((CG_MPFA_MODULE_HEADER *)pTempModuleBuffer), 0, &nFoundIndex, nlocalSearchFlags) != CG_MPFARET_OK)	//MOD003
    {
        free(pOrgTempModuleBuffer);
        return CG_MPFARET_NOTFOUND;
    }
    
    // At this point we have found the proper module in the BIOS. Now compare the module data.

    // Get the pointer to the module within the BIOS.
    pTempSectionBuffer = (pTempInfo->pSectionBuffer + nFoundIndex);
    
    // Check whether the size matches
    if(((CG_MPFA_MODULE_HEADER *)pTempSectionBuffer)->modSize != nDataSize)
    {
        free(pOrgTempModuleBuffer);
        return CG_MPFARET_NOMATCH;
    }
            
    // Now compare the data parts 

    // Adjust pointers to point to data blocks.

    // For CMOS default/fixed and CMOS backup modules we have to skip a certain range
    // of data because it includes the RTC values which are not defined.
    if( (((CG_MPFA_MODULE_HEADER *)pTempSectionBuffer)->modType == CG_MPFA_TYPE_CMOS_BACKUP) ||
        (((CG_MPFA_MODULE_HEADER *)pTempSectionBuffer)->modType == CG_MPFA_TYPE_CMOS_DEFAULT) || 
        (((CG_MPFA_MODULE_HEADER *)pTempSectionBuffer)->modType == CG_MPFA_TYPE_CMOS_FIXED))
    {
        pTempSectionBuffer = pTempSectionBuffer + CG_MPFA_RTC_SKIP_SIZE;
        pTempModuleBuffer = pTempModuleBuffer + CG_MPFA_RTC_SKIP_SIZE;
        nDataSize = nDataSize - CG_MPFA_RTC_SKIP_SIZE;
    }

    // The MPFA headers now can be skipped as well !
    // (The order that we use to skip is not important so don't get confused with what we
    //  did above with the RTC size skip).
    pTempSectionBuffer = pTempSectionBuffer + sizeof(localMpfaHdr);
    pTempModuleBuffer = pTempModuleBuffer + sizeof(localMpfaHdr);

    // Adjust compare size.
    nDataSize = nDataSize - (sizeof(localMpfaHdr) + sizeof(localMpfaEnd));

    // Now compare byte by byte
    for(nCount = 0; nCount < nDataSize; nCount++)
    {
        if( *(pTempSectionBuffer + nCount) != *(pTempModuleBuffer + nCount))
        {
            free(pOrgTempModuleBuffer);
            return CG_MPFARET_NOMATCH;
        }
    }
   
    return CG_MPFARET_OK;
}

#ifdef WIN32
/*---------------------------------------------------------------------------
 * Name: CgMpfaCreateStringTableFile
 * Desc: This functions either saves an OEM string table module contained in 
 *       the current BIOS or the original BIOS string table to the specified
 *       file.
 *
 * Inp:  lpszStringTableFile - pointer to output file name for string data
 *       usStrModId          - Module ID of the OEM string module to be 
 *                             saved. If set to 0xFFFF we want the original 
 *                             BIOS string table.
 *
 * Outp: return code:
 *       CG_MPFARET_OK          - Success, string data saved
 *       CG_MPFARET_ERROR       - String data could not be generated
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaCreateStringTableFile
( 
    _TCHAR* lpszStringTableFile,
    UINT16 usStrModId
)
{

    CG_MPFA_MODULE_HEADER localMpfaHeader = {CG_MPFA_MOD_HDR_ID,     //hdrID
                                                0,                      //modSize
                                                CG_MPFA_TYPE_STRING,    //modType
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
                                                usStrModId,             //modID
                                                0                       //modChecksum
                                                };   

    if(usStrModId != 0xFFFF)
    {
        // Try to extract OEM string table.
        if(CgMpfaSaveModule(&localMpfaHeader, lpszStringTableFile,CG_MPFACMP_TYPE | CG_MPFACMP_ID,CG_MPFASFL_DATA) == CG_MPFARET_OK)
        {
            return CG_MPFARET_OK;
        }
    }
    else
    {
        // Get the original BIOS string table data
        if (CgExtractAMIBiosModule(CgMpfaAllInfo.pSectionBuffer, CgMpfaAllInfo.sectionSize, AMI_TYPE_LANGUAGE ,lpszStringTableFile) == CG_RET_OK)
        {
            return CG_MPFARET_OK;
        }
    }
    return CG_MPFARET_ERROR;
}

/*---------------------------------------------------------------------------
 * Name: CgMpfaCreateSetupTableFile
 * Desc: This functions either saves an OEM setup table module contained in 
 *       the current BIOS or the original BIOS setup table to the specified
 *       file.
 *
 * Inp:  lpszSetupTableFile - pointer to output file name for setup data
 *       usSetModId         - Module ID of the OEM setup module to be 
 *                            saved. If set to 0xFFFF we want the original 
 *                            BIOS setup table.
 * Outp: return code:
 *       CG_MPFARET_OK          - Success, setup data saved
 *       CG_MPFARET_ERROR       - Setup data could not be generated
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaCreateSetupTableFile
( 
    _TCHAR* lpszSetupTableFile,
    UINT16 usSetModId
)
{
    CG_MPFA_MODULE_HEADER localMpfaHeader = {CG_MPFA_MOD_HDR_ID,        //hdrID
                                                0,                      //modSize
                                                CG_MPFA_TYPE_SETUP,     //modType
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
                                                usSetModId,             //modID
                                                0                       //modChecksum
                                                };   

    if(usSetModId != 0xFFFF)
    {
        // Try to extract OEM setup table.
        if(CgMpfaSaveModule(&localMpfaHeader, lpszSetupTableFile,CG_MPFACMP_TYPE | CG_MPFACMP_ID,CG_MPFASFL_DATA) == CG_MPFARET_OK)
        {
            return CG_MPFARET_OK;
        }
    }
    else
    {
        if (CgExtractAMIBiosModule(CgMpfaAllInfo.pSectionBuffer, CgMpfaAllInfo.sectionSize, AMI_TYPE_MAINBIOS ,lpszSetupTableFile) == CG_RET_OK)
        {
            return CG_MPFARET_OK;
        }
    }
    return CG_MPFARET_ERROR;
}
#endif	//WIN32
/*---------------------------------------------------------------------------
 * Name: CgMpfaCheckBUPActive     
 * Desc: Checks whether the BIOS update protection is activated.
 * Inp:  None
 *       
 * Outp: return code:
 *          TRUE    - BIOS update protection active
 *          FALSE   - BIOS update protection inactive
 *
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaCheckBUPActive(void) 
{    
	if(g_nOperationTarget == OT_ROMFILE)
    {
        return FALSE;
    }
    else
    {		
        return (UINT16)(CgosStorageAreaIsLocked(hCgos, CG32_STORAGE_MPFA_ALL, 0)); 
    }
}

/*---------------------------------------------------------------------------
 * Name: CgMpfaSetBUPInactive     
 * Desc: Tries to deactivate the BIOS update protection using the given
 *       string as unlock password. 
 * Inp:  lpszPassword    - Pointer to zero terminated string that holds 
 *                         the unlock password for the BUP. 
 *       
 * Outp: return code:
 *          TRUE    - BIOS update protection deactivated
 *          FALSE   - BIOS update protection not deactivated
 *
 *---------------------------------------------------------------------------
 */
UINT16 CgMpfaSetBUPInactive( _TCHAR* lpszPassword) 
{
    if(g_nOperationTarget == OT_ROMFILE)
    {
        return TRUE;
    }
    else
    {
        return (UINT16)(CgosStorageAreaUnlock(hCgos, CG32_STORAGE_MPFA_ALL, 0, (unsigned char *) lpszPassword, (UINT32)strlen(lpszPassword)));
    }
}
