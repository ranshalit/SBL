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
 
/*
 * MOD905 added "3AWN" for customer Omron to perform MAC address recovery
 * 
 * MOD020: Added CG_CheckPatchInputExtd_ICL for Icelake modules for mac adress recovery. GMA
 * 
 * MOD019: Changed return values for DSAC GbE region recovery so that in case of
 *         no GbE region present or recovery error bios update continues without
 *         GbE region recovery
 * 
 * MOD018: Fixed EHL MAC address recovery when running non-extended update and also
 *         prevent exit with error when GbE region could not be found.
 * 
 * MOD017: Fixed DSAC identification string
 * 
 * MOD016: Added EHL MAC address recovery
 * 
 * MOD015: Added DSAC MAC address recovery and preservation of LAN areas
 * 
 * MOD014: Added support for 512k block size BIOS update
 * 
 * MOD013: Added BIOS update data preservation
 * 
 *    Rev 1.15   Sep 06 2016 15:42:24   congatec
 * Added BSD header.
 * MOD009: 
 * Again modified the MAC address recovery mechanism for full SPI flash update. 
 * Make sure htat the recovered area / data really is the active area, i.e.
 * really contains the currently used MAC address.
 * MOD010: Allow to control the automatic off-on handling for full SPI update unlock for each operating system via a specific parameter/flag.
 * MOD011: Removed special handling and display of BIOS 'bootblock', which since long is not supported anymore in this form by any congatec BIOS.
 * MOD012: Ensure that the flash is locked again correctly after full SPI update.
 * 
 *    Rev 1.14   Aug 10 2015 13:03:16   gartner
 * Replace undefined UINT8* with unsigned char*.
 * 
 *    Rev 1.13   Aug 06 2015 13:26:02   gartner
 * MOD008: Changed MAC address recovery mechanism.
 * 
 *    Rev 1.12   Aug 08 2014 13:44:20   gartner
 * MOD007: Improved BIOS update progress reporting.
 * 
 *    Rev 1.11   May 31 2012 15:44:20   gartner
 * Updated variable defintions to ease 64bit porting.
 * MOD006: Support MAC recovery for all projects by default.
 * 
 *    Rev 1.10   Jan 10 2012 15:15:24   gartner
 * MOD005: Added MAC recovery support for Bx67 and Bx77 BIOS versions.
 * 
 *    Rev 1.9   Jun 22 2011 13:45:50   gartner
 * MOD004: Added MAC address restore handling on ext. flash update for new BM67 and BM77 BIOS project based boards.
 * Fixed some report strings to be usable with the Windows GUI tool as well.
 * Added additional delay after flash erase for extra security.
 * Added support for extended BIOS update handling that does not require reboots to unlock/lock.
 * Added support to suppress automatic restart after ext. flash update.
 * 
 *    Rev 1.8   Dec 06 2010 11:57:34   gartner
 * MOD003: Added support for extended/full flash update handling support, including all required checks and patches as well as update support for BIOS ROM contents only from within a BIN input file.
 * 
 *    Rev 1.7   Apr 30 2010 13:03:26   gartner
 * Improved BIOS update error recovery handling. Indicate blocks that required a retry.
 * 
 *    Rev 1.6   Apr 16 2009 12:52:38   gartner
 * Added flash write/erase retries for BIOS update.
 * 
 *    Rev 1.5   Nov 09 2006 13:09:36   gartner
 * Changes in BIOS save sub-routine. This also is now used in combination with a prepare routine.
 * 
 *    Rev 1.4   Oct 30 2006 15:08:22   gartner
 * Adaptions for Linux and QNX support; split ip BIOS update in smaller blocks.
 * 
 *    Rev 1.3   Mar 21 2006 14:37:26   gartner
 * MOD002: Set the CMOS ROM checksum bad bit in the RTC diagnostics byte to indicate that CMOS is invalid. Added code to invalidate the CMOS checksum at location 2Eh to support XPressROM based BIOSes that do not check the 0Eh diagnostics byte.
 * 
 *    Rev 1.2   Jan 27 2006 12:38:48   gartner
 * Fixed function description in header.
 * 
 *    Rev 1.1   Dec 19 2005 13:25:56   gartner
 * MOD001: Added BIOS update prepare routine to be able to switch flash devices on update.
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
#include "biosflsh.h"

/*--------------
 * Externs used
 *--------------
 */

extern void BiosFlashReportState(UINT32 nControl, _TCHAR *szReportString);

/*--------------------
 * Local definitions
 *--------------------
 */
#define MAX_FLASH_RETRIES   10
//MOD008 #define BM57_GBE_REGION_OFFSET	0x1000									//MOD003
// Setup storage area 
#define CGOS_STORAGE_AREA_SETUP		0x82020000									//MOD013

/*------------------
 * Global variables
 *------------------
 */
UINT32 nFlashSize, nFlashBlockSize, nExtdFlashSize;								//MOD003
char szBoardBiosName[9] = {0x00};
CG_BIOS_INFO CgBiosInfoRomfile;													//MOD003
CG_BIOS_INFO CgBiosInfoFlash;													//MOD003

																				//MOD003 


//MOD020 v
UINT16 CG_CheckPatchInputExtd_ICL
(
	unsigned char *pInputData,
	UINT32 nInputDataSize,
	UINT32 nProjID,	
    UINT32 nFlags
)
{
    unsigned char macAddress[8][6];
    unsigned char defaultMacAddress[8][6];
    unsigned char *lanBuffer, *orglanBuffer;
    unsigned int i, j;
    char lanAreaProgress[50];
    UINT32	data32;		
    UINT32	FRBAflash, FRBAfile;			//Flash region base address	
    UINT32	FLREG11flash, FLREG11file;
    UINT32	NACNISRegionflash, NACNISRegionfile;
    UINT32  NACNISRegionflashEnd; //NACNISRegionfileEnd;
    UINT32  NACNISSize;

    //PFA and Checksum variables
    //UINT16	oldPFASumFile = 0;
    UINT16	newPFASumFile = 0;
    //UINT16  PFASumFlash = 0;
    UINT16  newCheckSumFile = 0;
    UINT16  oldCheckSumFile = 0;
    //UINT16  checkSumFlash;

    //variables for the PFA iterations
    UINT32	PFAPointerFlash,            PFAPointerFile;
    UINT32  PFAOffsetFlash,             PFAOffsetFile;
    UINT32  PFALengthFlash,             PFALengthFile;
    UINT32  PFARegionStartOffsetFlash,  PFARegionStartOffsetFile;
    UINT32  typeIDFlash,                typeIDFile;
    UINT32  typeIdOffsetFlash,          typeIdOffsetFile;
    UINT32  PFARegionLengthFlash,       PFARegionLengthFile;
    UINT32  PFARegionNextFlash,         PFARegionNextFile;
    UINT32  usedPFALengthFlash = 0x0,   usedPFALengthFile = 0x0;
    UINT32  PFARegionDataOffsetFlash,   PFARegionDataOffsetFile;
    UINT32  PFAChecksumDataOffsetFile; //PFAChecksumDataOffsetFlash,

    //This is the offset for the second Bank. It is at word offset 0x8000 which is 0x8000 * 2 = 0x10000
    UINT32  bank2offset = 0x10000 ;
    UINT32  bank1Validity;
    UINT32  bank2Validity;

    UINT32  validBank;

    unsigned int  MACOFFSETS [MACADDRESSES];

    //Variables for the checksum calculation
    UINT16 oldMacPart1, oldMacPart2, oldMacPart3;
    UINT16 newMacPart1, newMacPart2, newMacPart3;
    UINT16 oldCheck,newCheck;
    UINT16 oldMac[MACADDRESSES], newMac[MACADDRESSES];

    //keep Lan areas
    #define __SIZE__ 0x10000
    UINT32 flashPortion = 0x0;
    UINT32 readFlashBytes = 0;
    UINT32 readAddress = 0;

    //Build an array for the relative mac address offsets
    MACOFFSETS[0]= 0;
    for(j=1;j<MACADDRESSES;j++)
    {
        MACOFFSETS[j] = MACOFFSETS[j-1]+8;
    }

 
    // -----------------------------

    //How to get the MAC addresses:
    
    //- Start at FDBAR (flash descriptor base address register) 
    //  which is at address 0x0 on the SPI flash device / the file.
	//(See Intel Xeon D-1700 and D-2700 SPI Programming Guide page 16, chapter 4.1)
    
    //- Go to the Flash Map 0 Register (FLMAP0) which is at FDBAR + 14h
    //  (See Intel Xeon D-1700 and D-2700 SPI Sprogramming guide page 17, chapter 4.1.1.2)
    
    //- Get the flash region base address (FRBA) -> Bits [23:16] of the FLMAP0 register 
    //  contain bits [11:4] of the FRBA. Bits [26:12] and bits [3:0] of the FRBA are 0.
    //  Hence, use (FLMAP0_content & 0x00FF0000) >> 12 to obtain the FRBA.
    //  FRBA is 0x40
    
    //- Using the FRBA, go to Flash Region 11 (FLREG11, which is at FRBA + 02Ch.
    //  FLEG11 = FRBA + 0x2c = 0x40 + 0x2c = 0x6c
    // (See Intel Xeon D-1700 and D-2700 SPI Sprogramming guide page 17, chapter 4.1.3.12)

    //  If it is used, obtain the Region Base address for the NAC NIS region address using bits [14:0] of the FLREG11 register.
    //  0x25e8 --> 0x25e8000 = NACNIS Region
    //  All operations from now on are relative to the NACNIS Region offset 0x25e8000

    //- Check valid Bank (only in flash)
    //  When operating in the flash, check if this bank of the NACNIS Region is valid. There are two bank available.
    //  The validity of the two banks switches after every MAC address update with eeupdate
    //  The bank is valid, if the bit [7:6] of the firts byte at offset 0x25e8000 is 0x0x1. If not check the second bank.
    //  The second bank is located at offset 0x08000, which would be 0x25e8000+(2*0x8000)=0x25F8000
    //  (See Intel Ethernet Connection E82X Programming MAC Addresses Rev.1.0, Chapter 2.1, doc nr. 736861)
    //  INFO: the second bank is empty in the BIOS binary file, and will be initialized after first boot.


    //- Using the valid bank, you have to look for the PFA start location. Go to the defined offset 0x40 (0x40*2).
    //  You should end in offset 0x25e8080. There you will see the PFA offset 0x0100. Double the PFA offset and add it to the NACNIS Region offset 0x25e8000.
    //  0x25e8000 + 0x200 = 0x25e8200. The first word represents the length of the whole PFA region, which should be 0x2f00.
    //  The actual start of the PFA region, which will also be the first PFA is one word further as the PFA length=0x2f00 is not part of the PFA region.
    //  PFAstartOffset = 0x25e8202
    //  All operation will be done inside the PFA region which is 0x2f00 long.
    //  (See Intel Ethernet Connection E82X Programming MAC Addresses Rev.1.0, Chapter 2.2.1, doc nr. 736861)

    //- The PFA Region consists of several PFA (preserved field areas) which are structured in TLVs, which are TypeID, Length and Values.
    //  The first word is the TypeID, the next word is length of the upcoming values and afterwards follow the actual values.
    //  (See Intel Ethernet Connection E82X Programming MAC Addresses Rev.1.0, Chapter 2.2.2 doc nr. 736861)

    //- To find the specific PFA for the mac address you have to look for the predefined MAC TypeID, which is 0x010F.
    //  You have to iterate through the PFAs by finding the first TypeID and Length.
    //  Check the typeID and jump to the next PFA's typeID by adding the Length value to the current PFA offset.
    //  Do this until you get the wanted type ID for MAC, which is 0x010F.
    //  (See Intel Ethernet Connection E82X Programming MAC Addresses Rev.1.0, Chapter 2.2.2 and 2.2.3, doc nr. 736861)

    //  The checksum is also a PFA and can be found with the typeID 0x003F.
    //  (See Intel Ethernet Connection E82X Programming MAC Addresses Rev.1.0, Chapter 2.2.2, doc nr. 736861)

    //  Checksum calculation
    //  The sum of whole PFA region should be 0xbaba
    //  Use the values of the default mac addresses, the restored mac addresses and the old checksum to calculate the new checksum.
    //  Use the formula that is documented in Intel Ethernet Connection E82X Programming MAC Addresses Rev.1.0, Chapter 3, doc nr. 736861
    //  New_Check = Old_Mac - New_Mac + Old_Check
    //
    // -Keep lan Areas
    //  The complete lan area has the size of 10MB, which is too big the CgosStorageAreaRead function, so it is necessary to split the operation in 64KB chunks.
    //  Keep the lan area takes several minutes, because of its size.
    //
    
    // -----------------------------
    
	// Verify correct flash descriptors

	// Try to read flash descriptor signature from flash part
	if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, 0x10, (unsigned char*)(&data32), 4))
    {
		// If we cannot read at all, we have a problem !
		// However, we might still only face a flash unlock issue.
		return CG_BFRET_OK; //MOD019
	}
    
    // Check flash descriptor signatures in flash part and in flash contents file 
	if( (*((UINT32*)(pInputData+0x10)) != FLASH_DESCRIPTOR_SIGNATURE) ||  (data32 != FLASH_DESCRIPTOR_SIGNATURE))
	{
		// Invalid descriptor signature in flash or file -> Break.
		return CG_BFRET_OK;  //MOD019
	}
    
    // Get flash region base address from input file
	data32 = *((UINT32*)(pInputData+FLMAP0_OFFSET));
	FRBAfile = (data32 & 0x00FF0000) >> 12;

    // Get flash region base address from flash part 
	if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, FLMAP0_OFFSET, (unsigned char*)(&data32), 4))
    {
		// If we cannot read at all, we have a problem !
		// However, we might still only face a flash unlock issue.
		return CG_BFRET_OK; //MOD019
	}
	FRBAflash = (data32 & 0x00FF0000) >> 12;

    // Check match
	if(FRBAfile != FRBAflash)
	{
		// No match. Break 
		return CG_BFRET_OK; //MOD019
	}
    // Now that we have the flash region base address, check whether
	// there is a used flash region 11, which is dedicated to LAN CTRL0

    FLREG11file	=   FRBAfile + FLREG11_OFFSET;
    FLREG11flash = FRBAflash + FLREG11_OFFSET;


    // Get NAC NIS region base address from input file
	data32 = *((UINT32*)(pInputData+FLREG11file));
    
	// Check whether region is used
	if((data32 & 0x00001FFF) == 0x00001FFF)
	{
		// Region unused. Break.
		return CG_BFRET_OK; //MOD019
	}
    // Get NAC NIS region base
    NACNISRegionfile = (data32 & 0x00007FFF) << 12;
    // Get NAC NIS region limit
    //NACNISRegionfileEnd = (data32 & 0x7FFF0000) >> 4;

    // Get NACNIS region base address from flash part
	if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, FLREG11flash, (unsigned char*)(&data32), 4))
    {
		// If we cannot read at all, we have a problem !
		// However, we might still only face a flash unlock issue.
		return CG_BFRET_OK; // MOD019
	} 
	// Check whether region is used
	if((data32 & 0x00001FFF) == 0x00001FFF)
	{
		// Region unused. Break.
		return CG_BFRET_OK; //MOD019
	}
    // Get NAC NIS region base
    NACNISRegionflash = (data32 & 0x00007FFF) << 12;
    // Get NAC NIS region limit
    NACNISRegionflashEnd = (data32 & 0x7FFF0000) >> 4;


    /*
   #######################################################

    //Parse through the PFA region in the INPUTFILE

   #######################################################
                                                        */
    PFAPointerFile = PFA_POINTER;
    //printf("PFA_Pointer_File 0x%02x\n",PFAPointerFile);
    //Double it to get the word offset of flash.
    PFAPointerFile = PFAPointerFile*2;


    //Get the PFA Offset and double the offset to get the word offset
    data32 = *((UINT32*)(pInputData+NACNISRegionfile+PFAPointerFile));
    PFAOffsetFile = (data32 & 0x00000FFF);
    PFAOffsetFile = PFAOffsetFile *2 ;

    //Get the PFALength
    data32 = *((UINT32*)(pInputData + NACNISRegionfile+PFAOffsetFile));
    PFALengthFile = (data32 & 0x00FFFF);

    //Get the offset for the start of PFA Region, which is 2 bytes further than the address of PFAOffset, or 2 byte next to PFALength.
    //Start of the PFA regions is the type ID of the first PFA region
    PFARegionStartOffsetFile = NACNISRegionfile + PFAOffsetFile + 0x2;


    //Get the first typeID
    typeIdOffsetFile = PFARegionStartOffsetFile;
    data32 = *((UINT32*)(pInputData+typeIdOffsetFile));
    typeIDFile = (data32 & 0x00FFFF);

    //Look through all PFA regions in the file and search for the designated type ID "0x010F" for MAC addresses PFA and "0x003F" for the checksum PFA
    //Stay in the limiterd area length 0x2f00, for safetey purposes.
    while (typeIDFile != 0x003f && usedPFALengthFile <= PFALengthFile)
    {
        //printf("typeIDFILE 0x%04x\n",typeIDFile);
        //Get the the length of the current pfa region, which is one word after the typeID so you need to add 0x2 to the typeID offset
        data32 = *((UINT32*)(pInputData+typeIdOffsetFile+0x2));
        PFARegionLengthFile = ((data32) & 0x00FFFF);

        //Use the pfa region length to get to the next region.
        //Go to the value offset of the PFA by adding 0x4 and add the length as word offset which is double of the length.
        PFARegionNextFile = typeIdOffsetFile+0x4+(PFARegionLengthFile*2);

        //Override the typeIdOffset with the offset of the next pfa region
        typeIdOffsetFile = PFARegionNextFile;

        //Get the typeid of the next pfa offset
        data32 = *((UINT32*)(pInputData+typeIdOffsetFile));
        typeIDFile = (data32 & 0x00FFFF);

        //Get the difference of the current pfa offset and the initial start offset to get the length of the regions that has been used.
        //Neccessary to compare it with the overall length of the PFA region which is "2F00" to not get out of the PFA region.
        usedPFALengthFile = PFARegionNextFile - PFARegionStartOffsetFile;

        //Grab the offset of the Mac adress PFA and safe it in a variable. The loop continues until "003F" checksum PFA is found.
        if(typeIDFile == 0x010F)
        {
            PFARegionDataOffsetFile = typeIdOffsetFile+0x4;
        }
    }
    //Safe the offset for the checksum PFA
    PFAChecksumDataOffsetFile = typeIdOffsetFile+0x4;

    /*
   #######################################################

    //Parse through the PFA regions in the FLASH

   #######################################################
                                                          */

    //Check bank1 validity in flash
    if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, NACNISRegionflash, (unsigned char*)(&data32), 4))
    {
        // If we cannot read at all, we have a problem !
        // However, we might still only face a flash unlock issue.
        return CG_BFRET_OK; // MOD019
    }
    //check Bit [7:6] for value 0x01 to see if bank valid. If not then it is invalid.
    //mask bit 7:6 with 11000000, which is C0, which should result in either 0x40 or 0x00
    bank1Validity = (data32 & 0x000000C0);

    //Check bank2 validity in flash
    //Add the word offset 0x8000 "bank2offset" to switch to the next bank
    if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, NACNISRegionflash + bank2offset, (unsigned char*)(&data32), 4))
    {
        // If we cannot read at all, we have a problem !
        // However, we might still only face a flash unlock issue.
        return CG_BFRET_OK; // MOD019
    }
    //check Bit [7:6] for value 0x01 to see if bank valid. If not then it is invalid.
    //mask bit 7:6 with 11000000, which is C0, which should result in either 0x40 or 0x00
    bank2Validity = (data32 & 0x000000C0);

    //Check which bank is valid and save the offset in validBank variable, which will be the relative offset for furher operations.
    //TODO_GMA Think about the validBank initialization in the else condition. When else condition is active, validBank is not initialized,
    if (bank1Validity == 0x40 && bank2Validity == 0x00)
    {
        validBank = NACNISRegionflash;
    }
    else if(bank1Validity == 0x00 && bank2Validity == 0x40)
    {
        validBank = NACNISRegionflash + bank2offset;
    }
    //Added this else condition, when none of the above conditions is valid. When there is any issue wth the bank validity, the same NACNIS area offset will be used as in the file.
    //This will maybe happen if the file is corrupt. Better way would be, to add a Warning or Error message of this case is happening.
    else
    {
        validBank = NACNISRegionfile;
    }

    PFAPointerFlash = PFA_POINTER;
    //double the PFA Pointer to get the word offset
    PFAPointerFlash = PFAPointerFlash*2;

    //Get the PFA Offset and double the offset to get the word offset
    if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, validBank+PFAPointerFlash, (unsigned char*)(&data32), 4))
    {
        // If we cannot read at all, we have a problem !
        // However, we might still only face a flash unlock issue.
        return CG_BFRET_OK; // MOD019
    }
    PFAOffsetFlash = (data32 & 0x00000FFF);
    PFAOffsetFlash = PFAOffsetFlash *2 ;

    //Get the PFALength
    if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, validBank+PFAOffsetFlash, (unsigned char*)(&data32), 4))
    {
        // If we cannot read at all, we have a problem !
        // However, we might still only face a flash unlock issue.
        return CG_BFRET_OK; // MOD019
    }
    PFALengthFlash = (data32 & 0x00FFFF);

    //Get the offset for the start of PFA Region, which is 2 bytes further than the address of PFAOffset, or 2 byte next to PFALength.
    //Start of the PFA regions is the type ID of the first PFA region
    PFARegionStartOffsetFlash = validBank + PFAOffsetFlash + 0x2;
    if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, PFARegionStartOffsetFlash, (unsigned char*)(&data32), 4))
    {
        // If we cannot read at all, we have a problem !
        // However, we might still only face a flash unlock issue.
        return CG_BFRET_OK; // MOD019
    }

    //Get the first typeID
    typeIdOffsetFlash = PFARegionStartOffsetFlash;
    if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, typeIdOffsetFlash, (unsigned char*)(&data32), 4))
    {
        // If we cannot read at all, we have a problem !
        // However, we might still only face a flash unlock issue.
        return CG_BFRET_OK; // MOD019
    }
    typeIDFlash = (data32 & 0x00FFFF);

    //Look through all PFA regions in the flash and search for the designated type ID "0x010F" for MAC addresses PFA and "0x003F" for Checksum PFA
    //Stay in the limiterd area length 0x2f00, for safetey purposes.
    while (typeIDFlash != 0x003f && usedPFALengthFlash <= PFALengthFlash)
    {
        //printf("typeID 0x%04x\n",typeIDFlash);
        //Get the the length of the current pfa region, which is one word after the typeID so you need to add 0x2 to the typeID offset
        if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, typeIdOffsetFlash+0x2, (unsigned char*)(&data32), 4))
        {
            // If we cannot read at all, we have a problem !
            // However, we might still only face a flash unlock issue.
            return CG_BFRET_OK; // MOD019
        }
        PFARegionLengthFlash = ((data32) & 0x00FFFF);

        //Use the pfa region length to get to the next region.
        //Go to the value offset of the PFA by adding 0x4 and add the length as word offset which is double of the length.
        PFARegionNextFlash = typeIdOffsetFlash+0x4+(PFARegionLengthFlash*2);

        //Override the typeIdOffset with the offset of the next pfa region
        typeIdOffsetFlash = PFARegionNextFlash;

        //Get the typeid of the next pfa offset
        if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, typeIdOffsetFlash, (unsigned char*)(&data32), 4))
        {
            // If we cannot read at all, we have a problem !
            // However, we might still only face a flash unlock issue.
            return CG_BFRET_OK; // MOD019
        }
        typeIDFlash = (data32 & 0x00FFFF);

        //Get the difference of the current pfa offset and the initial start offset to get the length of the regions that has been used.
        //This is neccessary to compare it with the overall length of the PFA region which is "0x2F00" to not get out of the PFA region.
        usedPFALengthFlash = PFARegionNextFlash - PFARegionStartOffsetFlash;

        //Grab the offset of the Mac adress PFA and safe it in a variable. The loop continues until "003F" checksum PFA is found.
        if(typeIDFlash== 0x010F)
        {
            PFARegionDataOffsetFlash = typeIdOffsetFlash+0x4;
        }
    }
    //Safe the offset of the checksum PFA
    //PFAChecksumDataOffsetFlash = typeIdOffsetFlash+0x4;


    /*
   #######################################################

    // Keep Lan Areas flag

   #######################################################
                                                          */
    //Use this for debug purposes to save processing time. Write the flash in a file once and read from it
    //FILE* biosFiletmp = NULL;
    //biosFiletmp = fopen("test.txt","rb");
    //biosFiletmp = fopen("test.txt","w");

    if (nFlags & CG_BFFLAG_KEEP_LANAREAS)
    {
        NACNISSize = NACNISRegionflashEnd - NACNISRegionflash;
        flashPortion = NACNISSize;
        readAddress = NACNISRegionflash;

        //Sleep was necessary for the next BIOS flash report state.
        //Sometimes the "Preserving LAN configuration...." was only wirtten after the whole keep lan area process ended with "DONE"
        //This way i looked like it hanged. With sleep it displays the string without "DONE" through the whole process .
        //sleep(1);
        BiosFlashReportState(0, "Preserving LAN configuration. . . . ");


        // Read NAC NIS Region area into buffer
        orglanBuffer = (unsigned char *)malloc(NACNISSize);
        lanBuffer = orglanBuffer;
        if(!lanBuffer)
        {
            BiosFlashReportState(1, "FAILED!");
            free(lanBuffer);
            return CG_BFRET_OK; //MOD019
        }

        do
        {
            if(flashPortion > __SIZE__)
            {
                readFlashBytes = __SIZE__;
            }
            else
            {
                readFlashBytes = flashPortion;
            }
            if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, readAddress, lanBuffer, readFlashBytes))
            //if(fread(lanBuffer,1,readFlashBytes,biosFiletmp) != readFlashBytes)         //DEBUG read from file
            {
                BiosFlashReportState(1, "FAILED!");
                free(orglanBuffer);
                return CG_BFRET_OK; //MOD019
            }
            //fwrite(lanBuffer,1,readFlashBytes,biosFiletmp); //DEBUG   //DEBUG write flash ino a file

            flashPortion -= readFlashBytes;
            readAddress +=readFlashBytes;
            lanBuffer += readFlashBytes;

            sprintf(lanAreaProgress,"Preserving LAN configuration. . . . %d %% ",(100*(NACNISSize-flashPortion))/NACNISSize);
            BiosFlashReportState(2,lanAreaProgress); //


        } while(flashPortion != 0);
        lanBuffer = orglanBuffer;

        //Write the whole lan area from buffer into the input file
        for (i= 0; i < NACNISSize; i++)
        {
             *(pInputData+NACNISRegionflash + i) = lanBuffer[i];
        }

        BiosFlashReportState(1, "DONE!");
        free(orglanBuffer);
        //fclose(biosFiletmp); //DEBUG read/write from/to file

    }

    //if  keep lan area flag is not set, only overwrite the mac addresses
    else{

        //Save the mac addresses from the flash into the mac addresses array macAddress[][]
        for(i = 0; i < MACADDRESSES;i++)
        {
            if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, PFARegionDataOffsetFlash + MACOFFSETS[i], &macAddress[i][0], 6))
            {
                return CG_BFRET_OK; //MOD019
            }
        }

        //Save the default mac addresses of the file in an array. This is necessary fo further checksum calculations
        for(i = 0; i < MACADDRESSES; i++)
        {
            for (j= 0; j < 6; ++j)
            {
                defaultMacAddress[i][j] = *(pInputData+PFARegionDataOffsetFile + MACOFFSETS[i] + j);
            }
        }

        //copy the mac addresses from the flash into the input file
        for(i = 0; i < MACADDRESSES; i++)
        {
            for (j= 0; j < 6; ++j)
            {
                *(pInputData+PFARegionDataOffsetFile + MACOFFSETS[i] + j) = macAddress[i][j];
            }
        }

        //calculate checksum for each macaddress
        //(See Intel Ethernet Connection E82X Programming MAC Addresses Rev.1.0, Chapter 3, doc nr. 736861)
        oldCheckSumFile = *((UINT16*)(pInputData+PFAChecksumDataOffsetFile));
        oldCheck = oldCheckSumFile;
        for (i= 0; i< MACADDRESSES; i++)
        {
            //split the default mac addresses in 3 words
             oldMacPart1 = ((UINT16)(defaultMacAddress[i][0]*0x100)+defaultMacAddress[i][1]);
             oldMacPart2 = ((UINT16)(defaultMacAddress[i][3]*0x100)+defaultMacAddress[i][2]);
             oldMacPart3 = ((UINT16)(defaultMacAddress[i][4]*0x100)+defaultMacAddress[i][5]);
            //convert them into little endian
             oldMacPart1 = (oldMacPart1 << 8) | (oldMacPart1 >> 8);
             oldMacPart2 = (oldMacPart2 << 8) | (oldMacPart2 >> 8);
             oldMacPart3 = (oldMacPart3 << 8) | (oldMacPart3 >> 8);
            //sum the 3 parts up to the oldMac value
             oldMac[i] = oldMacPart1 + oldMacPart2 + oldMacPart3;
             //printf("old_Mac: %04x \n",oldMac[i]);

             //split the flash mac addresses in 3 words
             newMacPart1 = ((UINT16)(macAddress[i][0]*0x100)+macAddress[i][1]);
             newMacPart2 = ((UINT16)(macAddress[i][2]*0x100)+macAddress[i][3]);
             newMacPart3 = ((UINT16)(macAddress[i][4]*0x100)+macAddress[i][5]);
             //convert them into little endian
             newMacPart1 = (newMacPart1 << 8) | (newMacPart1 >> 8);
             newMacPart2 = (newMacPart2 << 8) | (newMacPart2 >> 8);
             newMacPart3 = (newMacPart3 << 8) | (newMacPart3 >> 8);
             //summarize them to newMAC value
             newMac[i] = newMacPart1 + newMacPart2 + newMacPart3;
             //printf("newMac: %04x \n",newMac[i]);

             //calculate the new checksum
             newCheck = oldMac[i]-newMac[i]+oldCheck;
             //printf("newCheck: %04x \n",newCheck);
             //use new check as oldcheck value for the next mac address checksum calculation.
             oldCheck = newCheck;

        }
        //when all 8 mac addresses have been checked, save the new checksum into variable
        newCheckSumFile = newCheck;

        //Override the checksum of file with the new calculated checksum
        *((UINT16*)(pInputData+PFAChecksumDataOffsetFile)) = newCheckSumFile;

    }

    //check if the PFA sum of the updated file is 0xBABA
    newPFASumFile = 0x0;
    for (i=0; i < PFALengthFile; i++)
    {
        newPFASumFile += (*(((UINT16 *)(pInputData+PFARegionStartOffsetFile-0x2))+i));
    }

    if(newPFASumFile != 0xbaba)
    {
       return CG_BFRET_OK;
    }

    return CG_BFRET_OK;
}

//MOD020 ^




//MOD015 v 

/*---------------------------------------------------------------------------
 * Name: CG_UpdateCheckSum_DSAC 
 * Desc: Recalculate and update the checksum for X550 10GbE LAN Region
         (DSAC (Denverton)).
 * Inp:  gbeRegion:             Pointer to the start of the LAN Region
 * Outp: return code:
 *       CG_BFRET_OK            - Success
 *       CG_BFRET_ERROR         - General processing error.
 *
 *---------------------------------------------------------------------------
 */

UINT16 CG_UpdateCheckSum_DSAC (
    UINT16  *gbeRegion
)
{
    UINT16  checkSum = 0;
    UINT16  Pointer, Size, Start, Length;
    UINT16  *NvmModulePtr;
    UINT32  i, j;

    //calculate check sum

    for( i = 0; i <= X550_SIZE; i++ )
    {
        if( i != X550_CHECKSUM_OFFSET )
        {
            checkSum += gbeRegion[i];
        }
    }
    for (i = X550_PCIE_ANALOG_POINTER; i < X550_FW_POINTER; i++) 
    {

        if (i == X550_PHY_POINTER || i == X550_OPTION_ROM_POINTER)
            continue;
        
        Pointer = gbeRegion[i];
        
        if (Pointer == 0xFFFF || Pointer == 0 )
             continue;

        switch (i) 
        {
                case X550_PCIE_GENERAL_POINTER:
                    Size = X550_PCIE_GENERAL_SIZE;
                    break;
                    
                case X550_PCIE_CONFIG0_POINTER:
                case X550_PCIE_CONFIG1_POINTER:
                    Size = X550_PCIE_CONFIG1_POINTER;
                    break;
                    
                default:
                    Size = 0;
                    break;
        }
        
        NvmModulePtr = &gbeRegion[Pointer];

        if (Size) 
        {
            Start = 0;
            Length = Size;
        } 
        else 
        {
            Start = 1;
            Length = NvmModulePtr[0];

            if (Length == 0xFFFF || Length == 0 )
            {
                continue;
            }
        }
        
        for (j = Start; Length; j++, Length--) 
        {
            checkSum += NvmModulePtr[j];    
        }
    }

    checkSum = 0xBABA - checkSum;
    // Update checksum in 10GbE region
    gbeRegion[X550_CHECKSUM_OFFSET] = checkSum;
    return CG_BFRET_OK;
}

// MOD015 ^

// MOD015 v 

/*---------------------------------------------------------------------------
 * Name: CG_CheckPatchInputExtd_DSAC
 * Desc: Check whether input BIOS data modification is required and if yes
 *       perform it. Handling for DSAC (Denverton).
 * Inp:  pInputData		- Pointer to input BIOS data
 *		 nInputDataSize	- Size of input BIOS data in bytes
 *       nProjID		- BIOS project ID
 * Outp: return code:
 *       CG_BFRET_OK            - Success
 *       CG_BFRET_ERROR         - General processing error.
 *
 *---------------------------------------------------------------------------
 */
UINT16 CG_CheckPatchInputExtd_DSAC
(
	unsigned char *pInputData,
	UINT32 nInputDataSize,
	UINT32 nProjID,	
    UINT32 nFlags
)
{
    unsigned char macAddress[4][6];  
    unsigned char *lanBuffer;	
    unsigned int i;
    UINT32	data32;		
    UINT32	FRBAflash, FRBAfile;			//Flash region base address	
    UINT32	FLREG11flash, FLREG11file;		//LAN CTRL0 region base address		
    UINT32	FLREG12file, FLREG12flash;		//LAN CTRL1 region base address		
	UINT32	LANCTRL0Regionflash, LANCTRL0Regionfile;
    UINT32  LANCTRL0RegionflashEnd, LANCTRL0RegionfileEnd;
    UINT32	LANCTRL1Regionfile, LANCTRL1Regionflash;		
    UINT32  LANCTRL1RegionfileEnd, LANCTRL1RegionflashEnd;	
    UINT32  LANCTRL0Size, LANCTRL1Size;
    
 
    // -----------------------------

    //How to get the MAC addresses:
    
    //- Start at FDBAR (flash descriptor base address register) 
    //  which is at address 0x0 on the SPI flash device / the file.
    
    //- Go to the Flash Map 0 Register (FLMAP0) which is at FDBAR + 14h
    //  (see Denverton SPI flash programming guide revision 1.0, page 24)
    
    //- Get the flash region base address (FRBA) -> Bits [23:16] of the FLMAP0 register 
    //  contain bits [11:4] of the FRBA. Bits [26:12] and bits [3:0] of the FRBA are 0.
    //  Hence, use (FLMAP0_content & 0x00FF0000) >> 12 to obtain the FRBA.
    
    //- Using the FRBA, go to Flash Region 11 (FLREG11, which is at FRBA + 02Ch, 
    //  see page 32). If it is used, obtain the Region Base address for
    //  the LAN CTRL 0 region address using bits [14:0] of the FLREG11 register.
    
    //- MAC address 0 can be found at LAN CTRL 0 region + Offset 202
    //- MAC address 1 can be found at LAN CTRL 1 region + Offset 212
    
    //- For getting MAC address 2 and 3, follow the same procedure as for FLREG11 for FLREG12.
    
    //- Recalculate the Checksum by calling UpdateCheckSum_DSAC
    
    // -----------------------------
    
	// Verify correct flash descriptors

	// Try to read flash descriptor signature from flash part
	if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, 0x10, (unsigned char*)(&data32), 4))
    {
		// If we cannot read at all, we have a problem !
		// However, we might still only face a flash unlock issue.
		return CG_BFRET_OK; //MOD019
	}
    
    // Check flash descriptor signatures in flash part and in flash contents file 
	if( (*((UINT32*)(pInputData+0x10)) != FLASH_DESCRIPTOR_SIGNATURE) ||  (data32 != FLASH_DESCRIPTOR_SIGNATURE))
	{
		// Invalid descriptor signature in flash or file -> Break.
		return CG_BFRET_OK;  //MOD019
	}
    
    // Get flash region base address from input file
	data32 = *((UINT32*)(pInputData+FLMAP0_OFFSET));
	FRBAfile = (data32 & 0x00FF0000) >> 12;

    // Get flash region base address from flash part 
	if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, FLMAP0_OFFSET, (unsigned char*)(&data32), 4))
    {
		// If we cannot read at all, we have a problem !
		// However, we might still only face a flash unlock issue.
		return CG_BFRET_OK; //MOD019
	}
	FRBAflash = (data32 & 0x00FF0000) >> 12;

    // Check match
	if(FRBAfile != FRBAflash)
	{
		// No match. Break 
		return CG_BFRET_OK; //MOD019
	}
    // Now that we have the flash region base address, check whether
	// there is a used flash region 11, which is dedicated to LAN CTRL0
	FLREG11file	=   FRBAfile + FLREG11_OFFSET;
	FLREG11flash = FRBAflash + FLREG11_OFFSET;

	// Get LAN CTRL0 region base address from input file
	data32 = *((UINT32*)(pInputData+FLREG11file));
    
	// Check whether region is used
	if((data32 & 0x00001FFF) == 0x00001FFF)
	{
		// Region unused. Break.
		return CG_BFRET_OK; //MOD019
	}
    LANCTRL0Regionfile = (data32 & 0x00007FFF) << 12;
    // Get LAN CTRL 0 region limit
    LANCTRL0RegionfileEnd = (data32 & 0x7FFF0000) >> 4;

    // Get LAN CTRL0 region base address from flash part
	if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, FLREG11flash, (unsigned char*)(&data32), 4))
    {
		// If we cannot read at all, we have a problem !
		// However, we might still only face a flash unlock issue.
		return CG_BFRET_OK; // MOD019
	} 
	// Check whether region is used
	if((data32 & 0x00001FFF) == 0x00001FFF)
	{
		// Region unused. Break.
		return CG_BFRET_OK; //MOD019
	}
    LANCTRL0Regionflash = (data32 & 0x00007FFF) << 12;
    // Get LAN CTRL 0 region limit
    LANCTRL0RegionflashEnd = (data32 & 0x7FFF0000) >> 4;
    
    if((LANCTRL0Regionfile != LANCTRL0Regionflash) || (LANCTRL0RegionfileEnd != LANCTRL0RegionflashEnd))
	{
		// No match. Break
		return CG_BFRET_OK; //MOD019
	}
    // Check whether
	// there is a used flash region 11, which is dedicated to LAN CTRL1
	FLREG12file	= FRBAfile + FLREG12_OFFSET;
	FLREG12flash = FRBAflash + FLREG12_OFFSET;

    // Get LAN CTRL1 region base address from input file
	data32 = *((UINT32*)(pInputData+FLREG12file));
    // Check whether region is used
	if((data32 & 0x00001FFF) == 0x00001FFF)
	{
		// Region unused. Break.
		return CG_BFRET_OK; //MOD019
	}

    LANCTRL1Regionfile = (data32 & 0x00007FFF) << 12;
    LANCTRL1RegionfileEnd = (data32 & 0x7FFF0000) >> 4;
    
 	// Get LAN CTRL1 region base address from flash part
	if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, FLREG12flash, (unsigned char*)(&data32), 4))
    {
		// If we cannot read at all, we have a problem !
		// However, we might still only face a flash unlock issue.
		return CG_BFRET_OK; //MOD019
	}
	// Check whether region is used
	if((data32 & 0x00001FFF) == 0x00001FFF)
	{
		// Region unused. Break.
		return CG_BFRET_OK; //MOD019
	}
    LANCTRL1Regionflash = (data32 & 0x00007FFF) << 12;
    // Get LAN CTRL 1 region limit
    LANCTRL1RegionflashEnd = (data32 & 0x7FFF0000) >> 4;
    

    if((LANCTRL1Regionfile != LANCTRL1Regionflash) || (LANCTRL1RegionfileEnd != LANCTRL1RegionflashEnd)) 
	{
		// No match. Break
		return CG_BFRET_OK; //MOD019
	} 
     
    //There is a valid LANCTRL 1 section in the flash and in the file. 
    
    // If the /LAN parameter has been entered, preserve LAN CTRL 0 and 1 areas completely
    if (nFlags & CG_BFFLAG_KEEP_LANAREAS)
    {
        LANCTRL0Size = LANCTRL0RegionflashEnd - LANCTRL0Regionflash;
        LANCTRL1Size = LANCTRL1RegionflashEnd - LANCTRL1Regionflash;
        
        BiosFlashReportState(0, "Preserving LAN configuration. . . . ");
        
        // Read LAN CTRL 0 area into buffer
        
        lanBuffer = (unsigned char *)malloc(LANCTRL0Size);
        if(!lanBuffer)
        {
            BiosFlashReportState(1, "FAILED!");
            free(lanBuffer);
            return CG_BFRET_OK; //MOD019
        }

        if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, LANCTRL0Regionflash, lanBuffer, LANCTRL0Size))
        {
            BiosFlashReportState(1, "FAILED!");
            free(lanBuffer);
            return CG_BFRET_OK; //MOD019
        }

        for (i= 0; i < LANCTRL0Size; ++i)
        {
            *(pInputData+LANCTRL0Regionflash + i) = lanBuffer[i];
            
        }

        free(lanBuffer);
        
        // Read LAN CTRL 1 area into buffer
        
        lanBuffer = (unsigned char *)malloc(LANCTRL1Size);
        if(!lanBuffer)
        {
            BiosFlashReportState(1, "FAILED!");
            free(lanBuffer);
            return CG_BFRET_OK; //MOD019
        }
        
        if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, LANCTRL1Regionflash, lanBuffer, LANCTRL1Size))
        {
            BiosFlashReportState(1, "FAILED!");
            free(lanBuffer);
            return CG_BFRET_OK; //MOD019
        }
        for (i= 0; i < LANCTRL1Size; ++i)
        {
            *(pInputData+LANCTRL1Regionflash + i) = lanBuffer[i];
        } 
        BiosFlashReportState(1, "DONE!");
        free(lanBuffer);  
    }
    else // preserve only the MAC addresses and overwrite the other parts of the LAN CTRL 0 and 1 areas
    {

        //Get first mac address with fixed offset
        if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, LANCTRL0Regionflash + MAC_0_2_OFFSET, &macAddress[0][0], 6))
        {
            return CG_BFRET_OK; //MOD019
        } 
        
        //MAC address 1 is stored at offset 0x212
        if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, LANCTRL0Regionflash + MAC_1_3_OFFSET, &macAddress[1][0], 6))
        {
            return CG_BFRET_OK; //MOD019
        }
        
        //Try to recover MAC address 2 using offset 0x202
        
        if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, LANCTRL1Regionflash + MAC_0_2_OFFSET, &macAddress[2][0], 6))
        {
            return CG_BFRET_OK; //MOD019
        }
        
        //MAC address 3 is stored at offset 0x212
        if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, LANCTRL1Regionflash + MAC_1_3_OFFSET, &macAddress[3][0], 6))
        {
            return CG_BFRET_OK; //MOD019
        }
        
        // copy old mac address 0 to input bios buffer
        
        for (i= 0; i < 6; ++i)
        {
            *(pInputData+LANCTRL0Regionflash + MAC_0_2_OFFSET + i) = macAddress[0][i];
        }
        
        // copy old mac address 1 to input bios buffer
        
        for (i= 0; i < 6; ++i)
        {
            *(pInputData+LANCTRL0Regionflash + MAC_1_3_OFFSET + i) = macAddress[1][i];
        }
        
        // copy old mac address 2 to input bios buffer
        
        for (i= 0; i < 6; ++i)
        {
            *(pInputData+LANCTRL1Regionflash + MAC_0_2_OFFSET + i) = macAddress[2][i];
        }
        
        // copy old mac address 3 to input bios buffer
        
        for (i= 0; i < 6; ++i)
        {
            *(pInputData+LANCTRL1Regionflash + MAC_1_3_OFFSET + i) = macAddress[3][i];
        }

        //Update checksum for Region 0
        CG_UpdateCheckSum_DSAC((UINT16 *)(pInputData+LANCTRL0Regionflash));

        //Update checksum for Region 1
        CG_UpdateCheckSum_DSAC((UINT16 *)(pInputData+LANCTRL1Regionflash));
        
    }

    return CG_BFRET_OK; 
}

//MOD015 ^


//MOD016 v 

/* ###################################
 *  ELKHART LAKE MAC ADDRESS RECOVERY
 * ###################################
 * EHL GbE Region Layout:
 * 
 * GUID: 16 Bytes
 * FFS Header: 12 Bytes
 * Version: 4 Bytes
 * Number of Ports: 4 Bytes
 * 
 * For each MAC address (see number of ports):
 * BDF: 4 Bytes
 * MAC Address Low: 4 Bytes
 * MAC Address High: 4 Bytes
 * 
 */

UINT16 CG_CheckPatchInputExtd_EHL
(
	unsigned char *pInputData,
	UINT32 nInputDataSize,
	UINT32 nProjID,	
    UINT32 nFlags
)
{
    UINT32 gbeguid[4] = {0x12E29FB4, 0x4172AA56, 0x5FDD4EB3, 0xA90A444B};
    UINT32 guidData[4] = {0}; // buffer for storing the guid read from the bios file in order to compare
    UINT32 gbeFlashOffset, gbeFileOffset, nIndex, i, j;
    UINT8 guidFlashFound = FALSE;
    UINT8 guidFileFound = FALSE;
    UINT32 storageAreaSize;
    UINT32 versionFlash, numberOfPortsFlash; //version number and number of ports stored from bios flash
    unsigned char macEntryFlash[EHL_GBE_REGION_MAXNUM_PORTS][EHL_GBE_REGION_MAC_SIZE]; //array holding the mac entries stored from bios flash
    
    //##############################################
    // BIOS Flash : Get MAC addresses
    //##############################################
    
    BiosFlashReportState(0, "GbE Region recovery . . . . ");
    
    //get size of bios area (CG32_STORAGE_MPFA_ALL -> BIOS ROM part (comprises _STATIC, _USER, _DYNAMIC))
    if ((storageAreaSize = CgosStorageAreaSize(hCgos, CG32_STORAGE_MPFA_ALL)) <1)
    {
        BiosFlashReportState(1, "\nERROR: Could not get flash size. Area locked??? Please try again with /ef flag");
    }

    nIndex = 0;
        
    //search bios flash for gbe guid
    while(nIndex < (storageAreaSize)-sizeof(guidData))
    {
        if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_ALL, nIndex, (unsigned char*)&guidData, sizeof(guidData)))
        {
            BiosFlashReportState(1, "\nERROR: Could not get flash size. Area locked??? Please try again with /ef flag");
            return CG_MPFARET_OK; // MOD018
        }
        //compare if 4 dwords read from the offset match the gbe guid.
        else if((guidData[0] == gbeguid[0]) && (guidData[1] == gbeguid[1]) && (guidData[2] == gbeguid[2]) && (guidData[3] == gbeguid[3]))
        {
            //guid found!
            guidFlashFound = TRUE;
            gbeFlashOffset = nIndex; //this is the offset to GbE region (the first byte of its guid)
            break;
        }
        else
        {   //guid not found -> increase the index by 32 bits and keep searching
            nIndex = nIndex + 4;
        }
    }
    
    if (guidFlashFound == FALSE)
    {
        // no GUID found in BIOS Flash -> exit with error
        BiosFlashReportState(1, "\nERROR: Could not get flash size. Area locked??? Please try again with /ef flag\n");
        printf("ERROR: Could not find GbE GUID in BIOS Flash!\n");
        return CG_BFRET_OK; // MOD018 return OK, and flash BIOS anyway (GbE region will be written with default value)
    }

    // guidFlashFound == TRUE: A GbE GUID has been found in the BIOS file -> get the data
    
    // save the version dword
    if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_ALL, (gbeFlashOffset+EHL_GBE_REGION_NUMPORTS_OFFSET), (unsigned char*)&versionFlash, sizeof(versionFlash)))
    {
        printf("ERROR: Could not get version from BIOS flash\n");
        return CG_MPFARET_OK;  // MOD018 return OK, and flash BIOS anyway (GbE region will be written with default value)
    }

    // get the number of ports and the mac addresses
    
    // the number of Ports dword is at gbeFlashOffset + EHL_GBE_REGION_NUMPORTS_OFFSET
    if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_ALL, (gbeFlashOffset+EHL_GBE_REGION_NUMPORTS_OFFSET), (unsigned char*)&numberOfPortsFlash, sizeof(numberOfPortsFlash)))
    {
        printf("ERROR: Could not get number of ports from BIOS flash\n");
        return CG_MPFARET_OK;  // MOD018 return OK, and flash BIOS anyway (GbE region will be written with default value)
    }
            
    //sanity check: the number of ports is higher than the array for the mac addresses can hold
    if (numberOfPortsFlash > ((sizeof(macEntryFlash)/EHL_GBE_REGION_MAC_SIZE)))
    {

        printf("\nERROR: Number of MAC addresses found in BIOS flash exceeds buffer!\n");
        return CG_BFRET_OK;   // MOD018 return OK, and flash BIOS anyway (GbE region will be written with default value)
    }
    
    // print the mac addresses in the flash
    
    for (i = 0; i < numberOfPortsFlash; ++i)
    {
        if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_ALL, (gbeFlashOffset+EHL_GBE_REGION_MAC_OFFSET+(EHL_GBE_REGION_MAC_SIZE*i)), (unsigned char*)&macEntryFlash[i][0], EHL_GBE_REGION_MAC_SIZE))
        {
            printf("ERROR: Could not get MAC address %i from BIOS flash\n",i);
            return CG_MPFARET_OK;  // MOD018 return OK, and flash BIOS anyway (GbE region will be written with default value)
        }
        
    }
    
    //##############################################
    // BIOS File: Get Gbe Region
    //##############################################
    
    //search for GbE GUID in BIOS flash file

    nIndex = 0;
    
    while(nIndex < nInputDataSize)
    {

        if ((*(UINT32*)(pInputData+nIndex) == gbeguid[0]) && (*(UINT32*)(pInputData+nIndex+4) == gbeguid[1]) && (*(UINT32*)(pInputData+nIndex+8) == gbeguid[2]) && (*(UINT32*)(pInputData+nIndex+12) == gbeguid[3]))
        {
            // GUID found! 
            guidFileFound = TRUE;
            gbeFileOffset = nIndex; //this is the offset to GbE region (the first byte of its guid)
            break;
        }
        else
        {
            //GUID not found (yet) -> keep on searching
            nIndex = nIndex + 4;
        }
    }
    
    //exit if GUID has not been found
    if (guidFileFound == FALSE) 
    {
        // no GUID found in BIOS file -> exit with error
        printf("\nERROR: Could not find GbE Region in BIOS File!");
        return CG_BFRET_OK;   // MOD018 return OK, and flash BIOS anyway (GbE region will be written with default value)
    }

    // guidFileFound == TRUE here -> GUID has been found, now restore MAC addresses

    //##############################################
    // Save GbE region data from flash to file
    //##############################################
    
    // The file has an empty Gbe Region, the only data present is the GUID and the FFS Header. Thus, skip the FFS header when restoring.
    // The version, the number of ports and all the MAC entries have to be copied.
    
    
    // copy version
    
    *(UINT32*)(pInputData + gbeFileOffset + EHL_GBE_REGION_VERSION_OFFSET) = versionFlash;

    // copy number of ports
    
    *(UINT32*)(pInputData + gbeFileOffset + EHL_GBE_REGION_NUMPORTS_OFFSET) = numberOfPortsFlash;

    for (i = 0; i < numberOfPortsFlash; ++i)
    {
        //restore MAC address for each port
        for (j = 0; j < EHL_GBE_REGION_MAC_SIZE; ++j)
        {
             *(pInputData+gbeFileOffset+EHL_GBE_REGION_MAC_OFFSET + (EHL_GBE_REGION_MAC_SIZE*i) + j) = macEntryFlash[i][j];
        }
    }

    printf("DONE!\n");

    return CG_BFRET_OK; 
    
}

//MOD016 ^


/*---------------------------------------------------------------------------
 * Name: CG_CheckPatchInputExtd
 * Desc: Check whether input BIOS data modification is required and if yes
 *       perform it.
 * Inp:  pInputData		- Pointer to input BIOS data
 *		 nInputDataSize	- Size of input BIOS data in bytes
 *       nProjID		- BIOS project ID
 * Outp: return code:
 *       CG_BFRET_OK            - Success
 *       CG_BFRET_ERROR         - General processing error.
 *
 *---------------------------------------------------------------------------
 */
UINT16 CG_CheckPatchInputExtd
(
	unsigned char *pInputData,
	UINT32 nInputDataSize,
	UINT32 nProjID	
)
{
	unsigned char macAddress[6];															
	UINT32	data32;																//MOD008 v												
	UINT32	FRBAfile, FRBAflash;			//Flash region base address			
	UINT32	FLREG3file, FLREG3flash;		//GbE region base address			
	UINT32	GbERegionfile, GbERegionflash;										//MOD008 ^
	UINT16	i;
	UINT16	checkSum = 0;
	UINT16	SharedICW0x13 = 0;													//MOD009
    
//MOD008 v
	// Check whether ethernet configuration data and MAC address are stored in BIOS flash.
	// If this is the case, we try to read an already set MAC address from the flash 
	// and re-apply it after flash update with new contents.

	// If the flash does not contain (valid) ethernet configuration or MAC address, or the 
	// flash descriptors / layouts of the new flash file and the current physical flash
	// part do not match in general, we have to skip this restore process, but do NOT cancel 
	// the whole flash update process. We might e.g. be updating a completely empty flash 
	// (external BIOS update), which of course does not contain valid data but nevertheless
	// has to be updated. 

	//
	// Verify correct flash descriptors

	// Try to read flash descriptor signature from flash part
	if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, 0x10, (unsigned char*)(&data32), 4))
    {
		// If we cannot read at all, we have a problem !
		// However, we might still only face a flash unlock issue.
		return CG_BFRET_OK; // CG_BFRET_ERROR;
	}

	// Check flash descriptor signatures in flash part and in flash contents file 
	if( (*((UINT32*)(pInputData+0x10)) != FLASH_DESCRIPTOR_SIGNATURE) ||  (data32 != FLASH_DESCRIPTOR_SIGNATURE))
	{
		// Invalid descriptor signature in flash or file -> Break.
		return CG_BFRET_OK; 
	}
	
	//
	//
	//
	// Get flash region base address from input file
	data32 = *((UINT32*)(pInputData+FLMAP0_OFFSET));
	FRBAfile = (data32 & 0x00FF0000) >> 12;

	// Get flash region base address from flash part
	if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, FLMAP0_OFFSET, (unsigned char*)(&data32), 4))
    {
		// If we cannot read at all, we have a problem !
		// However, we might still only face a flash unlock issue.
		return CG_BFRET_OK; // CG_BFRET_ERROR;
	}
	FRBAflash = (data32 & 0x00FF0000) >> 12;
	
	// Check match
	if(FRBAfile != FRBAflash)
	{
		// No match. Break 
		return CG_BFRET_OK;
	}

	// Now that we have the flash region base address, check whether
	// there is a used flash region 3, which is dedicated to GbE
	FLREG3file	= FRBAfile + FLREG3_OFFSET;
	FLREG3flash	= FRBAflash + FLREG3_OFFSET;

	//
	//
	//
	// Get GbE region base address from input file
	data32 = *((UINT32*)(pInputData+FLREG3file));
	// Check whether region is used
	if((data32 & 0x00001FFF) == 0x00001FFF)
	{
		// Region unused. Break.
		return CG_BFRET_OK;
	}
	GbERegionfile = (data32 & 0x00007FFF) << 12;

	// Get GbE region base address from flash part
	if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, FLREG3flash, (unsigned char*)(&data32), 4))
    {
		// If we cannot read at all, we have a problem !
		// However, we might still only face a flash unlock issue.
		return CG_BFRET_OK; // CG_BFRET_ERROR;
	}
	// Check whether region is used
	if((data32 & 0x00001FFF) == 0x00001FFF)
	{
		// Region unused. Break.
		return CG_BFRET_OK;
	}
	GbERegionflash = (data32 & 0x00007FFF) << 12;
	
	// Check match
	if(GbERegionfile != GbERegionflash)
	{
		// No match. Break
		return CG_BFRET_OK;
	}

	// We have valid GbE sections in the flash and in the BIOS file. Thus try 
	// to save and recover an already programmed MAC address.
	// Try to read old MAC address from second storage address in flash first.
	// If there is a valid address at this second location and the second GbE NVM area is 
	// generally marked as valid/used via the 'Shared Init Control Word' (GbE NVM word 0x13) 
	// than the address of the second region will be maintained. 
	// If not the first location address is assumed to be valid and will be re-used.
	if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, GbERegionflash + 0x1000, &macAddress[0], 6))
    {
        return CG_BFRET_ERROR;
    }
	// Read shared init control word 0x13 from GbE NVM.							//MOD009 v
	if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, GbERegionflash + 0x1026, (UINT8*)(&SharedICW0x13), 2))
    {
        return CG_BFRET_ERROR;
    }
	if( (*((UINT32 *)(&macAddress[0])) == 0xFFFFFFFF) || (*((UINT32 *)(&macAddress[0])) == 0x88888788) ||
		((SharedICW0x13 & 0xC000) != 0x8000))									//MOD009 ^
//MOD009	if( *((UINT32 *)(&macAddress[0])) == 0xFFFFFFFF )
	{
		// Check other location 
		if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, GbERegionflash, &macAddress[0], 6))
		{
			return CG_BFRET_ERROR;
		}
	}

	// Copy old MAC address to input BIOS buffer
	for(i=0; i<6; i++)
	{
		*(pInputData+GbERegionflash+i) = macAddress[i];
	}

	// Recalculate checksum of GBE region
	checkSum = 0xBABA;
	for (i=0; i < 0x3F; i++)
	{
		checkSum = checkSum - (*(((UINT16 *)(pInputData+GbERegionflash))+i));
	}

	// Update checksum 
	*(((UINT16 *)(pInputData+GbERegionflash))+0x3F) = checkSum;

//MOD008 ^
	
	return CG_BFRET_OK;
}

																				//MOD003 ^

/*---------------------------------------------------------------------------
 * Name: CG_BiosSave
 * Desc: Save system BIOS to file.
 * Inp:  lpszBiosFile   - Pointer to name of BIOS file.
 * Outp: return code:
 *       CG_BFRET_OK            - Success
 *       CG_BFRET_INTRF_ERROR   - Failed to access CGOS interface
 *       CG_BFRET_ERROR_FILE    - File processing error
 *       CG_BFRET_ERROR         - General processing error.
 *
 *---------------------------------------------------------------------------
 */
UINT16 CG_BiosSave( _TCHAR* lpszBiosFile)
{   
    UINT32 nTransfered;
    unsigned char *pBuffer;
    FILE *fpBiosRomfile = NULL;
	UINT32 ulStorageSelector;											//MOD003        
	UINT32 nLocalFlashSize;												//MOD003        

	if(!hCgos)
    {
        return CG_BFRET_INTRF_ERROR;
    }

    // Check system flash
    // Performed in CG_BiosFlashPrepare
    //if((retVal = CgBfGetFlashSize(&nFlashSize, &nFlashBlockSize)) != CG_BFRET_OK)
    //{
    //    return retVal;
    //}

    if((nFlashSize == 0) || (nFlashBlockSize == 0))
    {
        // Something must be really wrong if we get these values !
        return CG_BFRET_INTRF_ERROR;
    }
																				//MOD003 v
	// If we have a extended flash area, we automatically save everything
/*GWETODO v
	//GWETODO : As long as readback of the extended flash area is so extremely slow, we only support
	// saving the BIOS content !
	if(nExtdFlashSize != 0)
	{
		nLocalFlashSize = nExtdFlashSize;
		ulStorageSelector = CG32_STORAGE_MPFA_EXTD;
	}
	else
	{
GWETODO ^*/
		nLocalFlashSize = nFlashSize;
		ulStorageSelector = CG32_STORAGE_MPFA_ALL;
//GWETODO	}
																				//MOD003 ^

    // Check ROM file
    fpBiosRomfile = fopen(lpszBiosFile, "wb");
    if (!fpBiosRomfile)
    {
        return CG_BFRET_ERROR_FILE;
    }

    // Allocate buffer for system BIOS    
    pBuffer = (unsigned char*)malloc(nLocalFlashSize);
    if(!pBuffer)
    {
        fclose(fpBiosRomfile);
        return CG_BFRET_ERROR;
    }

    
    BiosFlashReportState(0, " ");
    BiosFlashReportState(0, "Saving system BIOS. . . . . . ");
   
    // We have to split up MPFA transfers in reasonable block sizes due to limitations
    // of some CGOS driver implementations.
    nTransfered = 0;
    // Read flash contens to buffer
    do
    {
        if(!CgosStorageAreaRead(hCgos, ulStorageSelector, nTransfered, pBuffer + nTransfered, nFlashBlockSize))
        {
            BiosFlashReportState(1, "FAILED!");
            free(pBuffer); 
            fclose(fpBiosRomfile);
            return CG_BFRET_INTRF_ERROR;
        }
        nTransfered = nTransfered + nFlashBlockSize;
    }while(nTransfered < nLocalFlashSize);
    
    // Save buffer to file
    if(fwrite(pBuffer, nLocalFlashSize, 1, fpBiosRomfile ) != 1)
    {
        BiosFlashReportState(1, "FAILED!");
        free(pBuffer); 
        fclose(fpBiosRomfile);
        return CG_BFRET_ERROR_FILE;
    }
    free(pBuffer); 
    fclose(fpBiosRomfile);
    BiosFlashReportState(1, "DONE!");
 
    return CG_BFRET_OK;
}

/*---------------------------------------------------------------------------
 * Name: CG_BiosFlashPrepare
 * Desc: Collect information required for flash programming.
 * Inp:  None
 * Outp: return code:
 *       CG_BFRET_OK            - Success
 *       CG_BFRET_INTRF_ERROR   - Failed to access CGOS interface 
 *
 *---------------------------------------------------------------------------
 */
UINT16 CG_BiosFlashPrepare(void)
{  
    UINT16 retVal;

    if(!hCgos)
    {
        return CG_BFRET_INTRF_ERROR;
    }

    // Check system flash
    if((retVal = CgBfGetFlashSize(&nFlashSize, &nExtdFlashSize, &nFlashBlockSize)) != CG_BFRET_OK)	//MOD003
    {
        return retVal;
    }

    if((nFlashSize == 0) || (nFlashBlockSize == 0))
    {
        // Something must be really wrong if we get these values !
        return CG_BFRET_INTRF_ERROR;
    }

    retVal = CgBfGetBiosInfoBoard(&szBoardBiosName[0]);
    return retVal;
}


/*---------------------------------------------------------------------------
 * Name: CG_BiosFlash
 * Desc: Flash new BIOS.
 * Inp:  lpszBiosFile   - Pointer to name of BIOS file.
 *       nFlags         - Flags to control flash procedure.
 * Outp: return code:
 *       CG_BFRET_OK            - Success
 *       CG_BFRET_INTRF_ERROR   - Failed to access CGOS interface 
 *       CG_BFRET_INVALID       - Invalid BIOS file
 *       CG_BFRET_INCOMP        - Rom file and system are incompatible
 *       CG_BFRET_ERROR_SIZE    - Sizes of ROM file and target flash don't match
 *       CG_BFRET_ERROR_FILE    - File processing error
 *       CG_BFRET_ERROR         - General processing error.
 *
 *---------------------------------------------------------------------------
 */
UINT16 CG_BiosFlash( _TCHAR* lpszBiosFile, UINT32 nFlags)
{   
    UINT16 retVal;
    UINT32 nRomfileSize, nBlockCount, nCount, nCheckLength, nTransfered;
    char szFileBiosName[9] = {0x00};
    unsigned char *pBuffer, *pCheckBuffer;
    unsigned char nCmosVal = 0x00;
    FILE *fpBiosRomfile = NULL;
    char strBlockInfo[80] = {0};
    unsigned char nRetryCount = 0;
	UINT32 bufferOffset = 0;												//MOD003 v	
	UINT32 ulStorageSelector;		
	UINT32 ulAreaBlocksize;													//MOD014
	UINT32 nLocalFlashSize;			
	unsigned char bExtdUpdate;				
	UINT32 nLocalFlashBlockSize;											//MOD014
																			//MOD003 ^
	pCheckBuffer = NULL;													//MOD014


    if(!hCgos)
    {
        return CG_BFRET_INTRF_ERROR;
    }

    if((nFlashSize == 0) || (nFlashBlockSize == 0))
    {
        // Something must be really wrong if we get these values !
        return CG_BFRET_INTRF_ERROR;
    }
																				//MOD003 v
	if((nFlags & CG_BFFLAG_EXTD) && (nExtdFlashSize != 0))
	{
		// Extended/full flash update requested and extended storage available
		nLocalFlashSize = nExtdFlashSize;
		ulStorageSelector = CG32_STORAGE_MPFA_EXTD;
		bExtdUpdate = 1;
	}
	else if((nFlags & CG_BFFLAG_EXTD) && (nExtdFlashSize == 0))
	{
		// Extended/full flash update requested but no extended storage available.
		// We gracefully ignore this here for legacy BIOS support (BM45/BS45/CS45 projects)
		// and switch to standard BIOS content update only.
        nLocalFlashSize = nFlashSize;
		ulStorageSelector = CG32_STORAGE_MPFA_ALL;
		bExtdUpdate = 0;
	}
	else
	{
		// Standard/BIOS content only flash update requested
		nLocalFlashSize = nFlashSize;
		ulStorageSelector = CG32_STORAGE_MPFA_ALL;
		bExtdUpdate = 0;
	}
																				//MOD003 ^    
    // Check ROM file
    fpBiosRomfile = fopen(lpszBiosFile, "rb");
    if (!fpBiosRomfile)
    {
        return CG_BFRET_ERROR_FILE;
    }
    
    if((retVal = CgBfGetFileSize(fpBiosRomfile, &nRomfileSize)) != CG_BFRET_OK)
    {
        fclose(fpBiosRomfile);
        return retVal;
    }

    if(nRomfileSize != nLocalFlashSize)
    {
        // System flash and BIOS file sizes don't match
																				//MOD003 v
		if(bExtdUpdate)
		{
			// For extended/full flash updates a size match is mandatory
#ifndef INTERN						
			fclose(fpBiosRomfile);
			return CG_BFRET_ERROR_SIZE;
#else								
			nFlashSize = nRomfileSize;	
#endif						
		}

		else
		{
			// For non-extended / standard BIOS content only flash updates the ROM file must only be bigger than the flash size
			if(nRomfileSize < nLocalFlashSize)
			{
				fclose(fpBiosRomfile);
				return CG_BFRET_ERROR_SIZE;
			}
			else
			{
				// We assume that the BIOS is part of the given file and stored at the end of this file.
				// To accomodate this we only update the buffer index to read from.
				// This is required for updating BIOS content only from within a larger full SPI file.
				bufferOffset = nRomfileSize - nLocalFlashSize;
			}
		}
																				//MOD003 ^
    }
#ifndef INTERN						
    // Get BIOS info from ROM file
    if((retVal = CgBfGetBiosInfoRomfile(fpBiosRomfile,&szFileBiosName[0])) != CG_BFRET_OK)
    {
        // Don't flash if we cannot get info from ROM file
        fclose(fpBiosRomfile);
        return retVal;
    }

    // Compare project IDs
    if( *((UINT32 *)(&szFileBiosName[0])) != *((UINT32 *)(&szBoardBiosName[0])) )
    {
        // Project IDs don't match

        if(!(nFlags & CG_BFFLAG_FORCE))
        {
            fclose(fpBiosRomfile);
            return CG_BFRET_INCOMP;
        }
    }
																				//MOD003 v
	// If a non-extended update is selected, but an extended flash area is available,
	// we perform additional checks to see wether this non-extended update is sufficient !
	// If no extended storage area is available we simply allow non-extended updates.
	// This is ok for older boards.
	// In case an extended/full flash update is requested, but the compatibility ID
	// shows that it is not required, we automatically switch to standard BIOS content only update.
	if(nExtdFlashSize != 0)
	{
		if(CgBfCheckExtendedCompatibility(fpBiosRomfile,nRomfileSize)!= CG_BFRET_OK)
		{
			if(!bExtdUpdate)
			{
				// Indicate that there is a mismatch in the extended information.
				// Usually this should result in a message that a extended update is required !
				fclose(fpBiosRomfile);
				return CG_BFRET_INCOMP_EXTD;
			}
		}
		else
		{
			// Compatibility ID matches. This means no extended/full update necessary. Check forced extended 
			// update flag. If not set, switch to standard BIOS content only update.
			if(bExtdUpdate && (!(nFlags & CG_BFFLAG_FEXTD)))
			{
				// Switch to standard BIOS content update only.
				nLocalFlashSize = nFlashSize;
				ulStorageSelector = CG32_STORAGE_MPFA_ALL;
				bExtdUpdate = 0;
				// We assume that the BIOS is part of the given file and stored at the end of this file.
				// To accomodate this we only update the buffer index to read from.
				// This is required for updating BIOS content only from within a larger full SPI file.
				bufferOffset = nRomfileSize - nLocalFlashSize;
				BiosFlashReportState(0, "");												//MOD004
				BiosFlashReportState(0, "Extended flash update requested but not required!");
				BiosFlashReportState(0, "Switching to standard flash update to reduce");	//MOD004
				BiosFlashReportState(0, "update time.");									//MOD004
			}
		}
	}

	// For extended update we have to check the extended flash lock state and if
	// enabled, deactivate it.
	if(bExtdUpdate)
	{
		if(CgosStorageAreaIsLocked(hCgos, CG32_STORAGE_MPFA_EXTD, 0))
		{
//MOD004 v
			// Try to unlock without reset first and check unlock state again.
			// There may be some boards that can perform the required unlock 'on the fly'
			// without the need (or even the possibility) for a restart.
			// Try to deactivate the extended flash lock, but do not perform automatic off-on cycle
			if(!CgosStorageAreaUnlock(hCgos, CG32_STORAGE_MPFA_EXTD, 0x00000000, NULL, 0))
			{
				// Close BIOS file as we failed and will exit.
				fclose(fpBiosRomfile);
				return CG_BFRET_ERROR_UNLOCK_EXTD;
			}

			// Check lock state again.
			if(CgosStorageAreaIsLocked(hCgos, CG32_STORAGE_MPFA_EXTD, 0))
			{
				// 'On the fly' unlock not successful. Try again with restart mechanism.
//MOD004 ^
				// Close BIOS file as we will not come back.												//MOD004
				fclose(fpBiosRomfile);			

				if(nFlags & CG_BFFLAG_AUTO_OFFON)															//MOD010						
				{
					// Try to deactivate the extended flash lock and set flag to perform automatic off-on cycle
					BiosFlashReportState(0, "The system will perform a power off-on cycle now to restart in unlocked mode!");
					BiosFlashReportState(0, "Please run tool again after restart to perform actual flash update.");			
					Sleep(3000);
					
					if(!CgosStorageAreaUnlock(hCgos, CG32_STORAGE_MPFA_EXTD, 0x00000001, NULL, 0))
					{
						return CG_BFRET_ERROR_UNLOCK_EXTD;
					}
				}
				else																						//MOD010
				{
					// Try to deactivate the extended flash lock, but do not perform automatic off-on cycle
					if(!CgosStorageAreaUnlock(hCgos, CG32_STORAGE_MPFA_EXTD, 0x00000000, NULL, 0))
					{
						return CG_BFRET_ERROR_UNLOCK_EXTD;
					}
					BiosFlashReportState(0, "");															//MOD004 v
					BiosFlashReportState(0, "Flash part unlock for extended update");
					BiosFlashReportState(0, "has been prepared!");
					BiosFlashReportState(0, "Please perform a Soft-Off (S5) now.");			
					BiosFlashReportState(0, "Afterwards restart the system in the unlocked");
					BiosFlashReportState(0, "state using the power button and run");
					BiosFlashReportState(0, "tool again to perform actual flash update.");		
																											//MOD004 ^
					return CG_BFRET_NOTCOMP_EXTD;
				}																							//MOD010
			}																								//MOD004
		}
	}

																											//MOD003 ^
#endif								
    // Allocate buffer for BIOS file    
    pBuffer = (unsigned char*)malloc(nRomfileSize);
    if(!pBuffer)
    {
        fclose(fpBiosRomfile);
        return CG_BFRET_ERROR;
    }

    // Read contents of BIOS file to buffer
    BiosFlashReportState(0, " ");   //Placeholder for next string
    BiosFlashReportState(0, "Reading BIOS file. . . . ");
    fseek(fpBiosRomfile,0, SEEK_SET);
    fread(pBuffer, nRomfileSize, 1, fpBiosRomfile );
    if( ferror( fpBiosRomfile ) )      
    {
        BiosFlashReportState(1, "FAILED!");
        fclose(fpBiosRomfile);
        return CG_BFRET_ERROR_FILE;
    }

    fclose(fpBiosRomfile);
    BiosFlashReportState(1, "DONE!");
    
    //MOD018 v 
    // Check if platform is Elkhart Lake (QA70, SA70, TA70, PA70, MA70). If yes, perform EHL MAC address recovery.
    //MOD905 added "3AWN" for customer Omron to perform MAC address recovery
    if ((memcmp(&szBoardBiosName,"3AWN",4) == 0) || (memcmp(&szBoardBiosName,"QA70",4) == 0) || (memcmp(&szBoardBiosName,"SA70",4) == 0) || (memcmp(&szBoardBiosName,"TA70",4) == 0) || (memcmp(&szBoardBiosName,"PA70",4) == 0) || (memcmp(&szBoardBiosName,"MA70",4) == 0))
        {
                // EHL MAC address recovery
                if(CG_CheckPatchInputExtd_EHL(pBuffer,nRomfileSize,*((UINT32 *)(&szBoardBiosName[0])),nFlags) != CG_BFRET_OK)
                {
                    // Failed to perform required input data patch or check. Quit.
                    free(pBuffer);
                    return CG_BFRET_ERROR;
                }
                if (nFlags & CG_BFFLAG_KEEP_LANAREAS)
                {   // Restoring the LAN areas LAN CTRL 0 and LAN CTRL 1 is only available for DSAC
                    BiosFlashReportState(0, "\nRestoring LAN areas is not available for this project!\n");  
                }
    } 
    //MOD018 ^ 
																				//MOD003 v
	// For a real extended flash update special handling of the additional flash content might be required,
	// like restoring the MAC address stored in the flash or other things. This can be handled here.
	if(bExtdUpdate)
	{
        //MOD015 v
        //Check if we want to flash a DSAC. If yes, use the special handling for DSAC.
        if (memcmp(&szBoardBiosName,"DSA",3) == 0) //MOD017
        {
                // keep MAC addresses only
                if(CG_CheckPatchInputExtd_DSAC(pBuffer,nRomfileSize,*((UINT32 *)(&szBoardBiosName[0])),nFlags) != CG_BFRET_OK)
                {
                    // Failed to perform required input data patch or check. Quit.
                    free(pBuffer);
                    return CG_BFRET_ERROR;
                }
        }
        
        //MOD020 v
                //Check if we want to flash a Icelake. If yes, use the special handling for Icelake.
        if((memcmp(&szBoardBiosName,"HEI",3) == 0) || (memcmp(&szBoardBiosName,"HSI",3) == 0) ||(memcmp(&szBoardBiosName,"DICL",4) == 0))
        {
                // keep MAC addresses only
                
                if(CG_CheckPatchInputExtd_ICL(pBuffer,nRomfileSize,*((UINT32 *)(&szBoardBiosName[0])),nFlags) != CG_BFRET_OK)
                {
                    // Failed to perform required input data patch or check. Quit.
                    free(pBuffer);
                    return CG_BFRET_ERROR;
                }
        } 
        //MOD020 ^
        
        /*MOD018 v  
        // Check if platform is Elkhart Lake (QA70, SA70, TA70, PA70, MA70). If yes, perform EHL MAC address recovery.
        else if ((memcmp(&szBoardBiosName,"QA70",4) == 0) || (memcmp(&szBoardBiosName,"SA70",4) == 0) || (memcmp(&szBoardBiosName,"TA70",4) == 0) || (memcmp(&szBoardBiosName,"PA70",4) == 0) || (memcmp(&szBoardBiosName,"MA70",4) == 0))
        {
                // EHL MAC address recovery
                if(CG_CheckPatchInputExtd_EHL(pBuffer,nRomfileSize,*((UINT32 *)(&szBoardBiosName[0])),nFlags) != CG_BFRET_OK)
                {
                    // Failed to perform required input data patch or check. Quit.
                    free(pBuffer);
                    return CG_BFRET_ERROR;
                }
                if (nFlags & CG_BFFLAG_KEEP_LANAREAS)
                {   // Restoring the LAN areas LAN CTRL 0 and LAN CTRL 1 is only available for DSAC
                    BiosFlashReportState(0, "\nRestoring LAN areas is not available for this project!\n");  
                }
        } MOD018 ^*/
       
        else // Use the standard handling for all other projects.
        {
            if(CG_CheckPatchInputExtd(pBuffer,nRomfileSize,*((UINT32 *)(&szBoardBiosName[0]))) != CG_BFRET_OK)
            {
                // Failed to perform required input data patch or check. Quit.
                free(pBuffer);
                return CG_BFRET_ERROR;
            }
            if (nFlags & CG_BFFLAG_KEEP_LANAREAS)
            {          // Restoring the LAN areas LAN CTRL 0 and LAN CTRL 1 is only available for DSAC
                       BiosFlashReportState(0, "\nRestoring LAN areas is not available for this project!\n");  
            }
        }

        //MOD015 ^
	}
    //MOD015 v 
    else
    {
        if (nFlags & CG_BFFLAG_KEEP_LANAREAS)
        {   // Preserving the LAN CTRL 0 an 1 areas for DSAC is only available when running an extended update.
            BiosFlashReportState(0, "\nAn extended update is needed for restoring LAN areas!\n");  
        }
    }
    //MOD015 ^

    																			//MOD003 ^
																	
	if(nFlags & CG_BFFLAG_PRESERVE)												//MOD013 v
    {
		// Part 1 of BIOS update data preservation.
		BiosFlashReportState(0, "Save data to be preserved . . . . . . . ");
        if(CgosStorageAreaRead(hCgos, CGOS_STORAGE_AREA_SETUP, 0, NULL, 0))
        {
			// Data to be preserved successfully restored.
			BiosFlashReportState(1, "DONE!");
        }        
        else
        {
            //Failed to save data to be preserved.
            BiosFlashReportState(1, "FAILED!");
        }
    }
    //MOD013 ^
                                                                        //MOD014 v
	// Distinguish between new BIOSes supporting 512k update blocks and older ones supporting only 64k	
	ulAreaBlocksize = CgosStorageAreaBlockSize(hCgos, CG32_STORAGE_MPFA_EXTD);
	if(( ulAreaBlocksize == 0x80000) && ((nLocalFlashSize & 0x0007FFFF ) == 0))	// Check BIOS supports 512KB update.
	{ 		
    BiosFlashReportState(0, " ");   //Placeholder for next string
       BiosFlashReportState(0, "BIOS supports 512KB update data block size !\n");   	//Placeholder for next string
	   nLocalFlashBlockSize = 0x80000;		// Updating 512kb block everytime
		for(nBlockCount = 0; nBlockCount < (nLocalFlashSize /nLocalFlashBlockSize ) ; nBlockCount++) 
		{	
        	for(nRetryCount=0; nRetryCount <= MAX_FLASH_RETRIES; nRetryCount++)
			{      		
				SPRINTF(&strBlockInfo[0],"Update flash block %d of %d", nBlockCount + 1, (nLocalFlashSize /nLocalFlashBlockSize ) );
				BiosFlashReportState(2, &strBlockInfo[0]);
				// Erase is performed only if current data is not matched with data in flash block
	    		// Erase was done by BIOS routine before performing flash write
    			// Check CGMPProgramFlash_Ex() in CgMpfaSmmLib.c
            	if(!CgosStorageAreaWrite(hCgos, ulStorageSelector, (nBlockCount * nLocalFlashBlockSize), (pBuffer + bufferOffset + (nBlockCount * nLocalFlashBlockSize)), nLocalFlashBlockSize)) 
				{ 
					if(nRetryCount >= MAX_FLASH_RETRIES) 
					{
                       BiosFlashReportState(1, "FAILED!");
                       free(pBuffer);
                       return CG_BFRET_INTRF_ERROR;
                	}
					BiosFlashReportState(1, "RETRY!");
					BiosFlashReportState(0, " ");   //Placeholder for next string
            	} 
				else 
				{
                 	break;
            	}				
        	}
		}
		BiosFlashReportState(0, "Verify BIOS update . . . . . ");
    	// Verification was done by BIOS routine after performing flash write
    	// Check CGMPProgramFlash_Ex() in CgMpfaSmmLib.c
    	// Avoid verifying multiple times
    	BiosFlashReportState(1, "DONE!");
	}
	else 
	{																			//MOD014 ^
		// Proceed for regular BIOS Udpate
		BiosFlashReportState(0, " ");   //Placeholder for next string
	    for(nBlockCount = 0; nBlockCount < (nLocalFlashSize /nFlashBlockSize ) ; nBlockCount++)
	    {
        // MOD011: Removed outdated special handling for 'bootblock'. Does not exist anymore in this form.
        
		// Standard block processing         
        for(nRetryCount=0; nRetryCount <= MAX_FLASH_RETRIES; nRetryCount++)     
        {            
			SPRINTF(&strBlockInfo[0],"Update flash block %d of %d", nBlockCount + 1, (nLocalFlashSize /nFlashBlockSize ) );
			BiosFlashReportState(2, &strBlockInfo[0]);
            if(!CgosStorageAreaErase(hCgos, ulStorageSelector, (nBlockCount * nFlashBlockSize), nFlashBlockSize))
            {
                if(nRetryCount >= MAX_FLASH_RETRIES)
                {
                    BiosFlashReportState(1, "FAILED!");
                    free(pBuffer);
                    return CG_BFRET_INTRF_ERROR;
                }
				BiosFlashReportState(1, "RETRY!");
				BiosFlashReportState(0, " ");   //Placeholder for next string
				break;
            }

			Sleep(20L);	//Add a little bit of extra security in case the BIOS erase routines don't. MOD004

            if(!CgosStorageAreaWrite(hCgos, ulStorageSelector, (nBlockCount * nFlashBlockSize), (pBuffer + bufferOffset + (nBlockCount * nFlashBlockSize)), nFlashBlockSize))	//MOD003
            {
                if(nRetryCount >= MAX_FLASH_RETRIES)
                {
                    BiosFlashReportState(1, "FAILED!");
                    free(pBuffer);
                    return CG_BFRET_INTRF_ERROR;
                }
				BiosFlashReportState(1, "RETRY!");
				BiosFlashReportState(0, " ");   //Placeholder for next string
            }
            else
            {
                break;
            }
        }
    }

	// GWETODO v
	// As long as readback of the extended flash area is so extremely slow we only check 
	// the BIOS content !
	nLocalFlashSize = nFlashSize;
	ulStorageSelector = CG32_STORAGE_MPFA_ALL;
	bufferOffset = nRomfileSize - nLocalFlashSize;
	// GWETODO  ^

    BiosFlashReportState(0, "Verify BIOS update . . . . . ");
    // Now verify that the update was o.k.
    retVal = CG_BFRET_OK;
    pCheckBuffer = (unsigned char*)malloc(nRomfileSize);
    if(pCheckBuffer != NULL)
    {
        nTransfered = 0;
        do
        {
            if(!CgosStorageAreaRead(hCgos, ulStorageSelector, nTransfered, pCheckBuffer + nTransfered, nFlashBlockSize))
            {
                retVal = CG_BFRET_INTRF_ERROR;
				break;
            }
            nTransfered = nTransfered + nFlashBlockSize;
        }while(nTransfered < nLocalFlashSize);

        if(retVal == CG_BFRET_OK)
        {
			nCheckLength = nLocalFlashSize / 4;	//MOD011: Always check whole file.
												//       Removed special 'bootblock' handling.
			for(nCount = 0; nCount < nCheckLength; nCount++)
            {
                if( *(((UINT32*)pBuffer)+ (bufferOffset/4) + nCount) != *(((UINT32*)pCheckBuffer)+ nCount))
                {
                    retVal = CG_BFRET_ERROR;
                    break;
                }
            }
        }
    }
    else
    {
        retVal = CG_BFRET_ERROR; 
    }

    free(pCheckBuffer);
    free(pBuffer); 

    if(retVal != CG_BFRET_OK)
    {
        BiosFlashReportState(1, "FAILED!");
        return retVal;
    }
    BiosFlashReportState(1, "DONE!");

	}// end of Check BIOS supports 512KB update									//MOD014
	
	if(nFlags & CG_BFFLAG_PRESERVE)												//MOD013 v
    {
		// Part 2 of BIOS update data preservation.
		BiosFlashReportState(0, "Restore data to be preserved . . . . . . . ");
		if(CgosStorageAreaWrite(hCgos, CGOS_STORAGE_AREA_SETUP, 0, NULL, 0))
        {
			// Data to be preserved successfully restored.
			BiosFlashReportState(1, "DONE!");
        }        
        else
        {
            //Failed to restore data to be preserved.
            BiosFlashReportState(1, "FAILED!");
        }
    }																			//MOD013 ^	


    if(nFlags & CG_BFFLAG_DELCMOS)
    {
        // Invalidate CMOS 
        BiosFlashReportState(0, "Invalidate CMOS. . . . . . . ");
        // First we try to set the CMOS RAM checksum bad bit in the diagnostics byte.
        nCmosVal = 0x40;
        if(CgosStorageAreaWrite(hCgos, CGOS_STORAGE_AREA_CMOS, 0x0E, &nCmosVal, 1))
        {
            // For BIOS versions that do not care, we also invalidate the checksum.
            if(CgosStorageAreaRead(hCgos, CGOS_STORAGE_AREA_CMOS, 0x2E, &nCmosVal, 1))
            {
                // Incrementing the original checksum has to cause a failure.
                nCmosVal = nCmosVal + 1;
                if(CgosStorageAreaWrite(hCgos, CGOS_STORAGE_AREA_CMOS, 0x2E, &nCmosVal, 1))
                {
                    BiosFlashReportState(1, "DONE!");
                }
            }
        }        
        else
        {
            //Failed to invalidate CMOS
            BiosFlashReportState(1, "FAILED!");
        }
    }
    BiosFlashReportState(0, " ");
    BiosFlashReportState(0, "BIOS successfully updated!");
																				//MOD003 v
	//
	// For extended flash update we have to re-enable the extended flash lock state.
	// Usually this invokes a board restart.
	//
	if(bExtdUpdate)
	{
		if(nFlags & CG_BFFLAG_AUTO_OFFON)																	//MOD010						
		{
																											//MOD004 v
			if(!(nFlags & CG_BFFLAG_MANUF))
			{
				// Manufacturing mode not set. Perform automatic restart.
				BiosFlashReportState(0, "Board will restart now!");
				Sleep(2000);
				// Perform flash lock/board restart after complete flash update
				if(!CgosStorageAreaLock(hCgos, CG32_STORAGE_MPFA_EXTD, 0x00000001, NULL, 0))
				{
					return CG_BFRET_ERROR_LOCK_EXTD;
				}
			}
			else
			{
				// If manufacturing mode is selected we do not perform an automatic restart.
				// It might e.g. be necessary to add a settings map to the BIOS first.
				BiosFlashReportState(0, "An extended flash update has been performed!");					//MOD004 v
				BiosFlashReportState(0, "Please perform a complete power off-on cycle");
				BiosFlashReportState(0, "to guarantee that all changes become effective.");					//MOD004 ^
				
				// Perform flash lock after complete flash update											//MOD010 v 
				if(!CgosStorageAreaLock(hCgos, CG32_STORAGE_MPFA_EXTD, 0x00000000, NULL, 0))
				{
					return CG_BFRET_ERROR_LOCK_EXTD;
				}																							//MOD010 ^
			}																								//MOD004 ^		
		}
		else																								//MOD010																				//MOD010
		{
			BiosFlashReportState(0, "An extended flash update has been performed!");						//MOD004 v
			BiosFlashReportState(0, "Please perform a complete power off-on cycle");
			BiosFlashReportState(0, "to guarantee that all changes become effective.");						//MOD004 ^
			
			// Perform flash lock after complete flash update												//MOD012 v 
			if(!CgosStorageAreaLock(hCgos, CG32_STORAGE_MPFA_EXTD, 0x00000000, NULL, 0))
			{
				return CG_BFRET_ERROR_LOCK_EXTD;
			}																								//MOD012 ^
		}																									//MOD010
	}
	else
	{
		BiosFlashReportState(0, "Please reboot the system!");
		BiosFlashReportState(0, " ");	//MOD007
	}
                                                                //MOD003 ^    
	return CG_BFRET_OK;
}

/*---------------------------------------------------------------------------
 * Name: CgBfGetBiosInfoBoard     
 * Desc: Retrieves project ID and BIOS revision from the current BIOS using
 *       the CGOS interface.
 * Inp:  lpszBiosVersion    - Pointer to storage for BIOS version string
 * Outp: return code:
 *       CG_BFRET_OK            - Success
 *       CG_BFRET_INTRF_ERROR   - Failed to access CGOS interface 
 *
 *---------------------------------------------------------------------------
 */
UINT16 CgBfGetBiosInfoBoard
(
    _TCHAR *lpszBiosVersion
)
{        
    UINT16 nCount;
    CGOSBOARDINFO CgBoardInfo;

    if(!hCgos)
    {
        return CG_BFRET_INTRF_ERROR;
    }
    
    // Initialize size of structure to receive and get CGOS board info
    CgBoardInfo.dwSize = sizeof(CgBoardInfo);
    if (!CgosBoardGetInfo(hCgos, &CgBoardInfo))
    {
        return CG_BFRET_INTRF_ERROR;
    }
    
    for(nCount=0; nCount < 4; nCount++)
    {
        *(lpszBiosVersion + nCount) = CgBoardInfo.szBoard[nCount];
    }
    *(lpszBiosVersion + 4) = 'R';

    SPRINTF((lpszBiosVersion + 5),"%03x", CgBoardInfo.wSystemBiosRevision );
 
    return  CG_BFRET_OK;
}

																				//MOD003 v
/*---------------------------------------------------------------------------
 * Name: CgBfCheckExtendedCompatibility     
 * Desc: Check whether 
 * Inp:  fpBiosRomfile      - BIOS input file handle.
 *       nRomfileSize		- Size of input file
 * Outp: return code:
 *       CG_BFRET_OK            - Success. Extended compatibility is prooved
 *       CG_BFRET_INVALID       - Given BIOS file does not fullfill the extended
 *                                compatibility requirements.
 *
 *---------------------------------------------------------------------------
 */
UINT16 CgBfCheckExtendedCompatibility
( 
    FILE *fpBiosRomfile,
	UINT32 nRomfileSize
)
{    
	if(CgBfGetBiosInfoFlash() != CG_BFRET_OK)
	{
		return CG_BFRET_INVALID;
	}

	if(CgBiosInfoRomfile.biosExtCompID !=  CgBiosInfoFlash.biosExtCompID)
	{
		return CG_BFRET_INVALID;
	}

	return CG_BFRET_OK;
}
/*---------------------------------------------------------------------------
 * Name: CgBfGetBiosInfoFlash
 * Desc: Retrieves the complete BIOS information structure from the operating
 *       target flash part and stores it in the global BIOS info structure.
 * Inp:  none
 *       
 * Outp: return code:
 *       CG_BFRET_OK			- Success, BIOS info found
 *       CG_BFRET_INTRF_ERROR	- Error, BIOS info not found
 *
 *---------------------------------------------------------------------------
 */
UINT16 CgBfGetBiosInfoFlash(void) 
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
        if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_ALL, nIndex, (unsigned char*)&CgBiosInfoFlash.infoIDLow, sizeof(CgBiosInfoFlash.infoIDLow)))
        {
            return CG_MPFARET_ERROR;
        }
        else if(CgBiosInfoFlash.infoIDLow == CG_SYS_BIOS_INFO_ID_L)
        {
            if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_ALL, nIndex + sizeof(CgBiosInfoFlash.infoIDLow), (unsigned char*)&CgBiosInfoFlash.infoIDHigh, sizeof(CgBiosInfoFlash.infoIDHigh)))
            {
                return CG_MPFARET_ERROR;
            }
            else if(CgBiosInfoFlash.infoIDHigh == CG_SYS_BIOS_INFO_ID_H)
            {
                // BIOS info structure found, now copy whole structure
                if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_ALL, nIndex, (unsigned char*)&CgBiosInfoFlash, sizeof(CgBiosInfoFlash)))
                {
                    return CG_MPFARET_ERROR;
                }
                infoFound = TRUE;
                break;
            }                       
        }
        else
        {
            // nIndex = nIndex + sizeof(CgBiosInfoFlash.infoIDLow);
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
            if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_ALL, nIndex, (unsigned char*)&CgBiosInfoFlash.infoIDLow, sizeof(CgBiosInfoFlash.infoIDLow)))
            {
                return CG_MPFARET_ERROR;
            }
            else if(CgBiosInfoFlash.infoIDLow == CG_SYS_BIOS_INFO_ID_L)
            {
                if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_ALL, nIndex + sizeof(CgBiosInfoFlash.infoIDLow), (unsigned char*)&CgBiosInfoFlash.infoIDHigh, sizeof(CgBiosInfoFlash.infoIDHigh)))
                {
                    return CG_MPFARET_ERROR;
                }
                else if(CgBiosInfoFlash.infoIDHigh == CG_SYS_BIOS_INFO_ID_H)
                {
                    // BIOS info structure found, now copy whole structure
                    if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_ALL, nIndex, (unsigned char*)&CgBiosInfoFlash, sizeof(CgBiosInfoFlash)))
                    {
                        return CG_MPFARET_ERROR;
                    }
                    infoFound = TRUE;
                    break;
                }                       
            }
            else
            {
                // nIndex = nIndex + sizeof(CgBiosInfoFlash.infoIDLow);
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
																				//MOD003 ^

/*---------------------------------------------------------------------------
 * Name: CgBfGetBiosInfoRomfile     
 * Desc: Retrieves project ID and BIOS revision from a ROM file and checks
 *       whether the ROM file is valid.     
 * Inp:  fpBiosRomfile      - BIOS file handle.
 *       lpszBiosVersion    - Pointer to storage for BIOS version string
 * Outp: return code:
 *       CG_BFRET_OK            - Success
 *       CG_BFRET_INTRF_ERROR   - Failed to access CGOS interface 
 *       CG_BFRET_ERROR_FILE    - File processing error
 *       CG_BFRET_INVALID       - Invalid BIOS file
 *
 *---------------------------------------------------------------------------
 */
UINT16 CgBfGetBiosInfoRomfile
( 
    FILE *fpBiosRomfile,
    _TCHAR *lpszBiosVersion
)
{         
 
    UINT16 infoFound = FALSE;
    UINT16 retVal, nCount;
   
    if(!fpBiosRomfile)
    {
        return CG_BFRET_ERROR_FILE;
    }
    while( !feof( fpBiosRomfile ) )
    {
        fread( &CgBiosInfoRomfile.infoIDLow, sizeof(CgBiosInfoRomfile.infoIDLow), 1, fpBiosRomfile );
        if( ferror( fpBiosRomfile ) )      
        {
            return CG_BFRET_ERROR_FILE;
        }
        else if(CgBiosInfoRomfile.infoIDLow == CG_SYS_BIOS_INFO_ID_L)
        {
            fread( &CgBiosInfoRomfile.infoIDHigh, sizeof(CgBiosInfoRomfile.infoIDHigh), 1, fpBiosRomfile );
            if( ferror( fpBiosRomfile ) )      
            {
                return CG_BFRET_ERROR_FILE;
            }
            else if(CgBiosInfoRomfile.infoIDHigh == CG_SYS_BIOS_INFO_ID_H)
            {
                // BIOS info structure found, now copy whole structure
                fread( &CgBiosInfoRomfile.infoLen, sizeof(CgBiosInfoRomfile) -  8, 1, fpBiosRomfile );
                infoFound = TRUE;
                break;
            }                       
        }
    }
   
    if(infoFound == TRUE)
    {
        // We have copied the BIOS info structure now extract the desired values

        for (nCount = 0; nCount < 8; nCount++)
        {
            *(lpszBiosVersion + nCount) = CgBiosInfoRomfile.biosVersion[nCount];
        }
        retVal = CG_BFRET_OK;
    }
    else
    {
        // No info means this cannot be a congatec BIOS file!
        retVal = CG_BFRET_INVALID;
    }

    return retVal;
}

/*---------------------------------------------------------------------------
 * Name: CgBfGetFlashSize     
 * Desc: Retrieves the size of the system flash.     
 * Inp:  pFlashSize     - Pointer to storage for the total BIOS flash size.	//MOD003
 *       pExtdFlashSize - Pointer to storage for the extended flash size.	//MOD003
 *       pFlashBlockSize- Pointer to storage for flash block size
 * Outp: return code:
 *       CG_BFRET_OK            - Success
 *       CG_BFRET_INTRF_ERROR   - Failed to access CGOS interface
 *
 *---------------------------------------------------------------------------
 */
UINT16 CgBfGetFlashSize
(
    UINT32 *pFlashSize, 
	UINT32 *pExtdFlashSize, 
    UINT32 *pFlashBlockSize
)
{   
	unsigned char	dummy;

    if(!hCgos)
    {
        return CG_BFRET_INTRF_ERROR;
    }

    *pFlashSize = CgosStorageAreaSize(hCgos, CG32_STORAGE_MPFA_ALL); 
    //GWETODO: FIX after CGOS driver is fixed to return correct blocksize pFlashBlockSize = (CgosStorageAreaBlockSize(hCgos, pCG32_STORAGE_MPFA_ALL))*1024; 
    *pFlashBlockSize = 64 * 1024;//GWETODO
																				//MOD003 v
	// Try to get extended flash size (e.g. full SPI flash size) as well
	if(!CgosStorageAreaRead(hCgos, CG32_STORAGE_MPFA_EXTD, 0, &dummy, 1))
	{
		// If we cannot read from extended flash area, we assume that there is no such area !
		*pExtdFlashSize = 0;
	}
	else
	{
		*pExtdFlashSize = CgosStorageAreaSize(hCgos, CG32_STORAGE_MPFA_EXTD);
		// If the extended flash area has the same size as the standard BIOS (MPFA_ALL) area,
		// there is in fact no special extended area !
		if(*pExtdFlashSize == *pFlashSize)
		{
			*pExtdFlashSize = 0;
		}
	}
																				//MOD003 ^

    return CG_BFRET_OK;
}


/*---------------------------------------------------------------------------
 * Name: CgBfGetFileSize     
 * Desc: Retrieves the size of a specified file.     
 * Inp:  fpFile     - File handle.
 *       pFileSize  - Pointer to storage for the file size.
 * Outp: return code:
 *       CG_BFRET_OK            - Success
 *       CG_BFRET_ERROR_FILE    - File processing error
 *
 *---------------------------------------------------------------------------
 */
UINT16 CgBfGetFileSize
(
    FILE *fpFile,
    UINT32 *pFileSize
)
{
    INT32 lTempFileSize = 0;

    // Set file pointer to end of file
    if(!fseek(fpFile,0, SEEK_END))
    {
        // Get position at end of file = file length
        if ((lTempFileSize = ftell(fpFile)) < 0)
        {
            return CG_BFRET_ERROR_FILE;
        }
        *pFileSize = (UINT32) lTempFileSize;

        // Set file pointer back to start of file
        if(!fseek(fpFile,0, SEEK_SET))
        {
            return CG_BFRET_OK;
        }
    }
    return CG_BFRET_ERROR_FILE;
}

