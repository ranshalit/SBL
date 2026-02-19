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
 * $Header:   S:/CG/archives/CGTOOLS/CGUTIL/W32DOSX/CGUTLCMD/BiosUpdate.c-arc   1.11   Sep 06 2016 16:44:18   congatec  $
 *
 * Contents: Congatec BIOS update command line module.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/CGUTIL/W32DOSX/CGUTLCMD/BiosUpdate.c-arc  $
 * 
 *    Rev 1.11   Sep 06 2016 16:44:18   congatec
 * Added BSD header.
 * MOD006:
 * - Added options to BFLASH module allowing to control the automatic off-on handling for full SPI  update unlock for each operating system.  DOS and now also UEFI versions of the utility by default perform automatic off-on handling to  unlock the flash if required. For all other operating systems (Linux, Windows, ...) the automatic off-on unlock handling is deactivated by default (as before).
 * - Restructured, corrected and extended the usage description of the BFLASH module.
 * - BFLASH module commands EIL, EU, EUM, EL, ELM now do not longer require a BIOS file to be passed as parameter. As these commands only meant to change or get the SPI flash
 * lock state of a board, the BIOS file was never used and thus a useless 'dummy' anyway.
 * - Removed special handling and display of BIOS 'bootblock', which since long is not supported anymore in this form by any congatec BIOS.    
 * 
 *    Rev 1.10   Aug 08 2014 13:55:10   gartner
 * MOD003: Improved extended lock and update controls.
 * MOD004: Extended max. size of BIOs update password to 255.
 * MOD005: Improved BIOS update progress reporting strings.
 * 
 *    Rev 1.9   May 31 2012 15:33:10   gartner
 * Updated variable definitions to ease 64bit porting.
 * 
 *    Rev 1.8   Jun 22 2011 13:29:36   gartner
 * MOD002: Added new BIOS update options to suppress automatic reboot after extended flash update.
 * 
 *    Rev 1.7   Dec 06 2010 11:45:14   gartner
 * MOD001: Added support for extended/full flash update handling.
 * 
 *    Rev 1.6   Nov 09 2006 13:03:12   gartner
 * Removed description of 'FORCE' BIOS update feature. Added deferred BIOS handling also to save BIOS functionality to allow booting from one flash and reading the contents of another flash.
 * 
 *    Rev 1.5   Oct 30 2006 18:46:32   gartner
 * Change report string output for Linux/QNX to see BIOS update progress.
 * 
 *    Rev 1.4   Oct 30 2006 14:34:50   gartner
 * Added support for BIOS write protection feature.
 * 
 *    Rev 1.3   Mar 21 2006 15:09:24   gartner
 * CMOS is cleared by default on BIOS update. Added additional switch to avoid this while keeping the old /C switch also.
 * 
 *    Rev 1.2   Jan 27 2006 12:42:38   gartner
 * Changed some description strings.
 * 
 *    Rev 1.1   Dec 19 2005 13:21:30   gartner
 * MOD001: Support flash device switch. Set bbotblock update as default.
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
#include "biosflsh.h"
#include "cgbmod.h"

/*--------------
 * Externs used
 *--------------
 */
extern UINT32 nFlashSize, nFlashBlockSize, nExtdFlashSize;						//MOD003

/*--------------------
 * Local definitions
 *--------------------
 */

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
static void ShowUsage(void)
{
        PRINTF(_T("\nUsage:\n\n"));																			//MOD006 v
        PRINTF(_T("BFLASH [<BIOS file>] [options / commands]\n\n"));
        PRINTF(_T("Options:\n"));
/*        PRINTF(_T("/NOBB    - Do not update bootblock; by default bootblock is updated.\n"));*/			
/*        PRINTF(_T("/F       - Force BIOS update although project IDs do not match.\n"));*/
		PRINTF(_T("/E       - Perform extended/full flash update instead of BIOS content\n"));				
		PRINTF(_T("           only (standard) flash update.\n"));									
		PRINTF(_T("/EF      - Force extended/full flash update instead of BIOS content\n"));		
		PRINTF(_T("           only (standard) flash update.\n"));											
		PRINTF(_T("/EM      - Perform extended/full flash update instead of BIOS content\n"));				
		PRINTF(_T("           only (standard) flash update.\n"));									
		PRINTF(_T("           Manufacturing mode. No auto reboot after BIOS update.\n"));									
		PRINTF(_T("/EFM     - Force extended/full flash update instead of BIOS content\n"));		
		PRINTF(_T("           only (standard) flash update.\n"));									
		PRINTF(_T("           Manufacturing mode. No auto reboot after BIOS update.\n"));					
#ifdef INTERN																								
		PRINTF(_T("/S       - Read current BIOS from flash and save it to specified file.\n"));
#endif																										
        PRINTF(_T("/D       - Defer BIOS update. Allows to switch to external flash part.\n"));				

		PRINTF(_T("\nPress ENTER to continue...\n"));
		PRINTF(_T("\n"));																					
		getch();

		PRINTF(_T("/C       - Invalidate CMOS.\n"));														
        PRINTF(_T("/NOC     - Do not invalidate CMOS (DEFAULT).\n"));
		PRINTF(_T("/P       - Preserve BIOS password.\n"));											//MOD007	
		PRINTF(_T("/LAN     - Restore LAN area(s) when running an extended update.\n"));         	//MOD008	
		PRINTF(_T("/AOO     - Perform immediate/automatic off-on cycle to unlock extended\n"));				
		PRINTF(_T("           BIOS area if necessary. (Default for DOS and UEFI)\n"));
		PRINTF(_T("/NAOO    - Do NOT perform immediate/automatic off-on cycle to unlock extended\n"));				
		PRINTF(_T("           BIOS area. (Default for all other OSes)\n"));
		PRINTF(_T("           Requires manual off-on cycle to make unlock effective.\n"));					
		PRINTF(_T("\n"));																					
		PRINTF(_T("/BP:xxx  - Specify password to deactivate BIOS write protection (256 chars max).\n"));	
        PRINTF(_T("           Only required if BIOS write protection feature is available and\n"));			
        PRINTF(_T("           activated in setup.\n"));														
        PRINTF(_T("\n"));
        PRINTF(_T("The option '/BP:xxx' can also be used without the <BIOS file> parameter.\n"));
        PRINTF(_T("I.e.: BFLASH /BP:xxx.\n"));
        PRINTF(_T("This only deactivates the BIOS write protection in order to allow other\n"));
        PRINTF(_T("functions of this utility to perform necessary flash write accesses.\n"));
		
		PRINTF(_T("\nPress ENTER to continue...\n"));
		PRINTF(_T("\n"));																					
		getch();
		
		PRINTF(_T("Commands:\n"));																			
		PRINTF(_T("The commands EIL,EU,EUM,EL,ELM do not require a <BIOS file> parameter.\n"));				
		PRINTF(_T("\n"));																					
		PRINTF(_T("/EIL     - Check whether extended BIOS area is locked.\n"));
		PRINTF(_T("/EU      - Unlock extended BIOS area for update and execute automatic\n"));
		PRINTF(_T("           off-on cycle to make unlock effective.\n"));
		PRINTF(_T("/EUM     - Unlock extended BIOS area for update without automatic off-on.\n"));
		PRINTF(_T("           Requires manual off-on cycle to make unlock effective.\n"));
		PRINTF(_T("/EL      - Lock extended BIOS area and execute automatic off-on cycle\n"));
		PRINTF(_T("           to make lock effective.\n"));
		PRINTF(_T("/ELM     - Lock extended BIOS area without automatic off-on cycle.\n"));
		PRINTF(_T("           Requires manual off-on cycle to make lock effective.\n"));					
				
		PRINTF(_T("\nPress ENTER to continue...\n"));
		PRINTF(_T("\n"));																					
		getch();

		PRINTF(_T("NOTES:\n"));																				
		PRINTF(_T("\n"));																					
		PRINTF(_T("The extended update can be used to perform a full flash update for systems that\n"));
        PRINTF(_T("contain additional data in the flash part(s) beside the actual system BIOS.\n"));
		PRINTF(_T("With option /E the extended flash update is only performed if the extended\n"));
		PRINTF(_T("flash content has changed. If not, the utility automatically switches to the\n"));
		PRINTF(_T("standard, faster BIOS content only update. \n"));
		PRINTF(_T("With option /EF the extended update is performed, even if not mandatory.\n"));
		PRINTF(_T("For systems that do not support or require an extended flash update at all,\n"));
		PRINTF(_T("the  options /E and /EF are simply ignored.\n"));
		PRINTF(_T("A standard BIOS content only update is performed instead.\n"));
        PRINTF(_T("\n"));																					//MOD006 ^
        exit(1);
}


/*---------------------------------------------------------------------------
 * Name:
 * Desc:
 * Inp:         
 * Outp:        
 *---------------------------------------------------------------------------
 */
void HandleBiosUpdate(INT32 argc, _TCHAR* argv[])
{
    // UINT32 nFlags = (CG_BFFLAG_UPDBB | CG_BFFLAG_DELCMOS); /* Update bootblock and invalidate CMOS by default! */ //MOD006
	UINT32 nFlags = CG_BFFLAG_AUTO_OFFON;										//MOD006
    UINT32 nBfRet = 0;
    UINT16 i, nBupDeactivate = 0x00;
    char    cTemp;
    _TCHAR szNewBiosFile[256];
    _TCHAR szBupPassword[256] = {0};											//MOD004

    g_nOperationTarget = OT_BOARD;                   


	// By default the flag CG_BFFLAG_AUTO_OFFON should be set 
	// for DOS(X) and UEFI versions only.
#ifndef  DOSX
#ifdef	WIN32
	nFlags = nFlags & (~CG_BFFLAG_AUTO_OFFON);
#endif
#endif

#ifdef LINUX
	nFlags = nFlags & (~CG_BFFLAG_AUTO_OFFON);
#endif


    PRINTF(_T("BIOS Update Module\n"));
    if(argc < 2)
    {
            ShowUsage();
    }
    
    if (STRNCMP(argv[1], _T("/BP:"),4) == 0)
	{
        if ((SSCANF(argv[1], _T("/BP:%256s%c"), &szBupPassword[0], &cTemp) != 1) &&	//MOD004
            (SSCANF(argv[1], _T("/bp:%256s%c"), &szBupPassword[0], &cTemp) != 1))	//MOD004
	    {
            PRINTF(_T("ERROR: Invalid password specified (min. 1, max. 256 characters)!\n"));          
            exit(1);
        }
        nBupDeactivate = 0x02;  /*Only deactivate BUP, no BIOS update */
    }
	else if (STRNCMP(argv[1], "/EIL",4) == 0)									//MOD006 v
	{            
		// Check ext. flash area lock.					
        nFlags = nFlags | CG_BFFLAG_ISLOCKED;
	}
	else if (STRNCMP(argv[1], "/EUM",4) == 0)
    {            
		// Unlock ext. flash area.					
        nFlags = nFlags | CG_BFFLAG_UNLOCK;
		nFlags = nFlags | CG_BFFLAG_MANUF;
	}
	else if (STRNCMP(argv[1], "/ELM",4) == 0)
    {            
		// Lock ext. flash area.					
        nFlags = nFlags | CG_BFFLAG_LOCK;
		nFlags = nFlags | CG_BFFLAG_MANUF;
	}																			
	else if (STRNCMP(argv[1], "/EU",3) == 0)
    {            
		// Unlock ext. flash area.					
        nFlags = nFlags | CG_BFFLAG_UNLOCK;
	}
	else if (STRNCMP(argv[1], "/EL",3) == 0)
    {            
		// Lock ext. flash area.					
        nFlags = nFlags | CG_BFFLAG_LOCK;
	}																			//MOD006 ^
    else
    {
        if (SSCANF(argv[1], "%s%c", &szNewBiosFile[0], &cTemp) != 1)
	    {
            PRINTF(_T("ERROR: You have to specify a BIOS file name!\n"));
            exit(1);				
	    }

	    for (i=2; i<argc; i++)
	    {
		    if (STRNCMP(argv[i], "/C",2) == 0)
		    {
                nFlags = nFlags | CG_BFFLAG_DELCMOS;
		    }
            else if (STRNCMP(argv[i], "/NOC",4) == 0)
		    {
			    nFlags = nFlags & (~(CG_BFFLAG_DELCMOS));
		    }
		    else if (STRNCMP(argv[i], "/P",2) == 0)								//MOD007 v
            {            
                nFlags = nFlags | CG_BFFLAG_PRESERVE;
		    }																	//MOD007 ^
            else if (STRNCMP(argv[i], "/LAN",2) == 0)							//MOD009 v
            {            
                nFlags = nFlags | CG_BFFLAG_KEEP_LANAREAS;
		    }		                                                            //MOD009 ^
//MOD006 v   else if (STRNCMP(argv[i], "/NOBB",5) == 0)
//		    {
//				nFlags = nFlags & (~(CG_BFFLAG_UPDBB));
//MOD006 ^	}
		    else if (STRNCMP(argv[i], "/F",2) == 0)
            {            
                nFlags = nFlags | CG_BFFLAG_FORCE;
		    }		
#ifdef INTERN																	//MOD006
            else if (STRNCMP(argv[i], "/S",2) == 0)
            {            
                nFlags = nFlags | CG_BFFLAG_SAVE;
		    }
#endif																			//MOD006
            else if (STRNCMP(argv[i], _T("/BP:"),4) == 0)
	        {
                if ((SSCANF(argv[i], _T("/BP:%256s%c"), &szBupPassword[0], &cTemp) != 1) &&	//MOD004
                    (SSCANF(argv[i], _T("/bp:%256s%c"), &szBupPassword[0], &cTemp) != 1))	//MOD004
	            {
                    PRINTF(_T("ERROR: Invalid password specified (min. 1, max. 256 characters)!\n"));          
                    exit(1);
                }
                nBupDeactivate = 0x01;  /* Deactivate BUP and update or save BIOS */
            }
            else if (STRNCMP(argv[i], "/D",2) == 0)
            {            
                nFlags = nFlags | CG_BFFLAG_ASK;
		    }
			else if (STRNCMP(argv[i], "/EFM",4) == 0)												//MOD002 v										
            {            
				// Request forced extended/full flash update manufacturing mode.
				// Force extended flash update although not required due to matching compatibility IDs
                nFlags = nFlags | CG_BFFLAG_EXTD;
				nFlags = nFlags | CG_BFFLAG_FEXTD;
				nFlags = nFlags | CG_BFFLAG_MANUF;
		    }
			else if (STRNCMP(argv[i], "/EIL",4) == 0)												//MOD003 v
            {            
				// Check ext. flash area lock.					
                nFlags = nFlags | CG_BFFLAG_ISLOCKED;
		    }
			else if (STRNCMP(argv[i], "/EUM",4) == 0)
            {            
				// Unlock ext. flash area.					
                nFlags = nFlags | CG_BFFLAG_UNLOCK;
				nFlags = nFlags | CG_BFFLAG_MANUF;
		    }
			else if (STRNCMP(argv[i], "/ELM",4) == 0)
            {            
				// Lock ext. flash area.					
                nFlags = nFlags | CG_BFFLAG_LOCK;
				nFlags = nFlags | CG_BFFLAG_MANUF;
		    }																			
			else if (STRNCMP(argv[i], "/EU",3) == 0)
            {            
				// Unlock ext. flash area.					
                nFlags = nFlags | CG_BFFLAG_UNLOCK;
		    }
			else if (STRNCMP(argv[i], "/EL",3) == 0)
            {            
				// Lock ext. flash area.					
                nFlags = nFlags | CG_BFFLAG_LOCK;
		    }																						//MOD003 ^									
			else if (STRNCMP(argv[i], "/EM",3) == 0)												
            {            
				// Request extended/full flash update manufacturing mode.
                nFlags = nFlags | CG_BFFLAG_EXTD;
				nFlags = nFlags | CG_BFFLAG_MANUF;
		    }																						//MOD002 ^																																				
			else if (STRNCMP(argv[i], "/EF",3) == 0)												//MOD001 v
            {            
				// Request forced extended/full flash update
				// Force extended flash update although not required due to matching compatibility IDs
                nFlags = nFlags | CG_BFFLAG_EXTD;
				nFlags = nFlags | CG_BFFLAG_FEXTD;
		    }													
			else if (STRNCMP(argv[i], "/E",2) == 0)				
            {            
				// Request extended/full flash update
				// Perform extended flash update if required due to non-matching compatibility IDs.
				// If compatibility IDs match only a standard (BIOS content only) flash update is performed instead.
                nFlags = nFlags | CG_BFFLAG_EXTD;
		    }																						//MOD001 ^
			else if (STRNCMP(argv[i], "/NAOO",5) == 0)
            {            
				// Unlock ext. flash area.					
                nFlags = nFlags & (~CG_BFFLAG_AUTO_OFFON);
		    }
			else if (STRNCMP(argv[i], "/AOO",4) == 0)
            {            
				// Lock ext. flash area.					
                nFlags = nFlags | CG_BFFLAG_AUTO_OFFON;
		    }																								
			else
		    {
                PRINTF(_T("ERROR: Unknown command!\n"));
			    exit(1);
		    }
	    }
    }
    
    if (!CgosOpen())
    {
        PRINTF(_T("ERROR: Failed to access system interface!\n"));
        exit(1);   
    }

    if(nBupDeactivate > 0x00)
    {
        if(!CgMpfaCheckBUPActive())
        {
            /* In case we only wanted to deactivate BUP we treat it as an error if 
               deactivation is tried although BUP is not active. If it is combined
               with a BIOS update we ignore it */ 
            if(nBupDeactivate == 0x02)
            {
                PRINTF(_T("ERROR: BIOS write protection is not active!\n"));          
                CgosClose();
                exit(1);
            }
        }
        else
        {
            if(!(CgMpfaSetBUPInactive(&szBupPassword[0])))
            {
                PRINTF(_T("ERROR: Failed to deactivate BIOS write protection!\n"));          
                CgosClose();
                exit(1);
            }
            else
            {
                PRINTF(_T("BIOS write protection temporarily deactivated!\n")); 
                PRINTF(_T("Protection will automatically be re-activated on next boot.\n\n"));             
                if(nBupDeactivate == 0x02)
                {
                    /* We only wanted to deactivate BUP, so exit here */
                    CgosClose();
                    exit(0);
                }
            }            
        }
    }

    if(nFlags & CG_BFFLAG_SAVE)
    {
        nBfRet = CG_BiosFlashPrepare();

        if(nBfRet != CG_BFRET_OK)
        {
            PRINTF(_T("\nERROR: Failed get BIOS information!\n"));
            if(nBfRet == CG_BFRET_INTRF_ERROR)
            {
                PRINTF(_T("Failed to access system interface!\n"));
            }
            CgosClose();
            exit( 1);
        }
        if(nFlags & CG_BFFLAG_ASK)
        {
            PRINTF(_T("\nYou may now switch to another flash part!\n"));
            PRINTF(_T("Afterwards press any key to read BIOS from selected flash...\n"));
            getch();
        }

        nBfRet = CG_BiosSave((_TCHAR *) &szNewBiosFile);
        if(nBfRet != CG_BFRET_OK)
        {
            PRINTF(_T("\nERROR: Failed to save system BIOS!\n"));
            if(nBfRet == CG_BFRET_INTRF_ERROR)
            {
                PRINTF(_T("Failed to access system interface!\n"));
            }
            else if(nBfRet == CG_BFRET_ERROR_FILE)
            {
                PRINTF(_T("File processing error!\n"));
            }
            else
            {
                PRINTF(_T("Internal processing error!\n"));
            }
            CgosClose();
            exit( 1);
        }
    }
    else
    {
        // Check state of BIOS update protection
        if(CgMpfaCheckBUPActive())
        {
            PRINTF(_T("\nERROR: BIOS write protection is active!\n"));
            PRINTF(_T("Please use /BP:xxx option to deactivate it.\n"));
            CgosClose();
            exit(1);
        }            
        nBfRet = CG_BiosFlashPrepare();
        if(nBfRet != CG_BFRET_OK)
        {
            PRINTF(_T("\nERROR: Failed get BIOS update information!\n"));
            if(nBfRet == CG_BFRET_INTRF_ERROR)
            {
                PRINTF(_T("Failed to access system interface!\n"));
            }
            CgosClose();
            exit(1);
        }
//MOD003 v
		// Check whether flash part is locked or unlocked for update.
		if(nFlags & CG_BFFLAG_ISLOCKED)
        {
			if(nExtdFlashSize != 0)
			{
				// Check extended flash lock.
				if(CgosStorageAreaIsLocked(hCgos, CG32_STORAGE_MPFA_EXTD, 0))
				{
					PRINTF(_T("\nExtended flash part is locked!\n"));
					CgosClose();
					exit(0);	
				}
				else
				{
					PRINTF(_T("\nExtended flash part is unlocked!\n"));
					CgosClose();
					exit(1);	
				}
			}
			else
			{
				// Check regular flash lock.
				if(CgosStorageAreaIsLocked(hCgos, CG32_STORAGE_MPFA_ALL, 0))
				{
					PRINTF(_T("\nStandard flash part is locked!\n"));
					CgosClose();
					exit(0);	
				}
				else
				{
					PRINTF(_T("\nStandard flash part is unlocked!\n"));
					CgosClose();
					exit(1);	
				}
			}            
        }
		// Unlock extended flash part for BIOS update.
		if(nFlags & CG_BFFLAG_UNLOCK)
        {
			if(nExtdFlashSize != 0)
			{
				if(nFlags & CG_BFFLAG_MANUF)
				{
					// In manufacturing mode we do not execute an automatic restart
					if(!CgosStorageAreaUnlock(hCgos, CG32_STORAGE_MPFA_EXTD, 0x00000000, NULL, 0))
					{
						PRINTF(_T("\nERROR: Flash part unlock for extended update failed!\n"));
						CgosClose();
						exit(1);	
					}
					else
					{
						PRINTF(_T("\nFlash part unlock for extended update\n"));
						PRINTF(_T("has been prepared!\n"));
						PRINTF(_T("Please perform a Soft-Off (S5) now.\n"));
						PRINTF(_T("Afterwards restart the system in the unlocked\n"));
						PRINTF(_T("state using the power button.\n"));
						CgosClose();
						exit(0);	
					}	
				}
				else
				{
					PRINTF(_T("\nThe system will perform a power off-on cycle now to restart in unlocked mode!\n"));
					Sleep(3000);
					// Try to deactivate the extended flash lock and set flag to perform automatic off-on cycle
					if(!CgosStorageAreaUnlock(hCgos, CG32_STORAGE_MPFA_EXTD, 0x00000001, NULL, 0))
					{
						PRINTF(_T("\nERROR: Flash part unlock for extended update failed!\n"));
						CgosClose();
						exit(1);	
					}
					else
					{
						PRINTF(_T("\nERROR: Failed to perform power off-on cycle to unlock!\n"));
						Sleep(10000);
						CgosClose();
						exit(1);	
					}	
				}
			}
			else
			{
				// Standard flash does not require any unlock right now.
				PRINTF(_T("\nExtended flash part unlock not required!\n"));
				CgosClose();
				exit(0);
			}            
        }

		// Re-lock extended flash part.
		if(nFlags & CG_BFFLAG_LOCK)
        {
			if(nExtdFlashSize != 0)
			{
				if(nFlags & CG_BFFLAG_MANUF)
				{
					// In manufacturing mode we do not execute an automatic restart
					if(!CgosStorageAreaLock(hCgos, CG32_STORAGE_MPFA_EXTD, 0x00000000, NULL, 0))		//MOD006
					{
						PRINTF(_T("\nERROR: Extended flash part lock failed!\n"));
						CgosClose();
						exit(1);	
					}
					else
					{
						PRINTF(_T("\nFlash part lock has been prepared!\n"));
						PRINTF(_T("Please perform a Soft-Off (S5) now.\n"));
						PRINTF(_T("Afterwards restart the system in the locked\n"));
						PRINTF(_T("state using the power button.\n"));
						CgosClose();
						exit(0);	
					}	
				}
				else
				{
					// Manufacturing mode not set. Perform automatic restart.
					PRINTF(_T("\nThe system will perform a power off-on cycle now to restart in locked mode!\n"));
					Sleep(3000);
					// Perform flash unlock/board restart
					if(!CgosStorageAreaLock(hCgos, CG32_STORAGE_MPFA_EXTD, 0x00000001, NULL, 0))
					{
						PRINTF(_T("\nERROR: Extended flash part lock failed!\n"));
						CgosClose();
						exit(1);	
					}
					else
					{
						PRINTF(_T("\nERROR: Failed to perform power off-on cycle to lock!\n"));
						Sleep(10000);
						CgosClose();
						exit(1);	
					}	
				}
			}
			else
			{
				// Standard flash does not require any lock right now.
				PRINTF(_T("\nExtended flash part lock not required!\n"));
				CgosClose();
				exit(0);
			}            
        }
//MOD003 ^
        if(nFlags & CG_BFFLAG_ASK)
        {
            PRINTF(_T("\nYou may now switch to another flash part!\n"));
            PRINTF(_T("Afterwards press any key to start BIOS update...\n"));
            getch();
        }
        nBfRet = CG_BiosFlash((_TCHAR *) &szNewBiosFile, nFlags);
        if(nBfRet != CG_BFRET_OK)
        {
            PRINTF(_T("\nERROR: Failed to update BIOS!\n"));
            if(nBfRet == CG_BFRET_INCOMP)
            {
                PRINTF(_T("Project ID of selected BIOS file and current BIOS do not match!\n"));
                PRINTF(_T("The BIOS you tried to flash is not meant to be used on this board!\n"));
                /*PRINTF(_T("Select 'Force Update' option (/F) to flash anyways.\n"));*/
            }
            else if(nBfRet == CG_BFRET_INVALID)
            {
                PRINTF(_T("The specified file is not a valid BIOS file!\n"));						//MOD001
            }
            else if(nBfRet == CG_BFRET_ERROR_SIZE)
            {
                PRINTF(_T("File size and flash size do not match!\n"));
            }
            else if(nBfRet == CG_BFRET_INTRF_ERROR)
            {
                PRINTF(_T("Failed to access system interface!\n"));
            }
            else if(nBfRet == CG_BFRET_ERROR_FILE)
            {
                PRINTF(_T("File processing error!\n"));
            }
            else if(nBfRet == CG_BFRET_ERROR_NOEXTD)												//MOD001 v
			{
				PRINTF(_T("Extended/full flash update not possible!\n"));
			}
			else if(nBfRet == CG_BFRET_ERROR_UNLOCK_EXTD)								
			{
				PRINTF(_T("Failed to unlock flash for extended/full flash update!\n"));
			}
			else if(nBfRet == CG_BFRET_ERROR_LOCK_EXTD)								
			{
				PRINTF(_T("Failed to lock flash after extended/full flash update!\n"));
			}
			else if(nBfRet == CG_BFRET_NOTCOMP_EXTD)								
			{
				PRINTF(_T("Extended/full flash update not (yet) completed!\n"));
			}
			else if(nBfRet == CG_BFRET_INCOMP_EXTD)								
			{
				PRINTF(_T("This platform/BIOS requires an extended/full flash update (/E)!\n"));
			}																						//MOD001 ^
			else
            {
                PRINTF(_T("Internal processing error!\n"));
            }
            CgosClose();
            exit (nBfRet);	//MOD006: Return dedicated exit codes to indicate what went wrong.  exit( 1);
        }
    }
    CgosClose();
    exit( 0);
}

/*---------------------------------------------------------------------------
 * Name:
 * Desc:
 * Inp:         
 * Outp:        
 *---------------------------------------------------------------------------
 */
void BiosFlashReportState(UINT32 nControl, _TCHAR *szReportString)
{
#ifdef WIN32
    if(nControl == 1)
    {
        PRINTF("%s",szReportString);
    }
    else if(nControl == 2)
    {
        PRINTF("\r%s",szReportString);
    }
    else
    {
        PRINTF("\n%s",szReportString);
    }
#else
//MOD005 v
    if(nControl == 1)
    {
        PRINTF("%s",szReportString);
    }
    else if(nControl == 2)
    {
        PRINTF("\r%s",szReportString);
    }
    else
    {
        PRINTF("\n%s",szReportString);
    }
    fflush(stdout);
	//MOD008 sleep(1);
	usleep(50000);	//MOD009
//MOD005 ^ 
//PRINTF("\n%s",szReportString);
#endif
}


