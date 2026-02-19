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
 * $Header:   S:/CG/archives/CGTOOLS/CGUTIL/CGUTLCMN/CGEPI.C-arc   1.7   Sep 06 2016 15:50:14   congatec  $
 *
 * Contents: Congatec panel configuration common implementation module.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/CGUTIL/CGUTLCMN/CGEPI.C-arc  $
 * 
 *    Rev 1.7   Sep 06 2016 15:50:14   congatec
 * Added BSD header.
 * 
 *    Rev 1.6   May 31 2012 15:45:36   gartner
 * Updated variable definition to ease 64bit porting.
 * 
 *    Rev 1.5   Jul 02 2007 12:59:54   gartner
 * Added support to delete an OEM EPI entry.
 * 
 *    Rev 1.4   Nov 09 2006 13:11:30   gartner
 * Changed data set description strings.
 * 
 *    Rev 1.3   Oct 30 2006 15:15:04   gartner
 * Check size of dataset to read from EEPROM before instead of reading 256 bytes always. After EPI EEPROM update, read back data and verify. Add support to selectively update OEM EPI datasets in EPI module. Add support to get datasets and related information from EPI flash module.
 * 
 *    Rev 1.2   Jan 27 2006 12:34:28   gartner
 * Changed return code handling. Adaption to new MPFA function parameters. 
 * 
 *    Rev 1.1   Dec 19 2005 13:28:00   gartner
 * MOD001: Added routines to clear EPI EEPROM. Added backlight control routines.
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
#include "cgpanel.h"
#include "cgbmod.h"

/*--------------
 * Externs used
 *--------------
 */

extern UINT16 g_nOperationTarget;
extern HCGOS hCgos;
extern FILE *g_fpBiosRomfile;
extern _TCHAR *g_lpszBiosFilename;

/*--------------------
 * Local definitions
 *--------------------
 */
#define FIX_TYPE    0   //CONNY

#define EPI_OEMBLOCK_SEP_SIZE   8   // Size of the tag within the EPI module that
                                    // separates standard data set from OEM data 
                                    // set block

/*------------------
 * Global variables
 *------------------
 */
UINT32 g_nDDCBusIndex;

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
 * Name: FindEpiOemArea
 * Desc: Find EPDA/EPI OEM area.
 * Inp:  pBuffer        - Pointer to buffer to search 
 *       nSize          - Buffer size
 *       pFoundIndex    - Pointer to value to store the index in the buffer
 *                        where the EPI OEM area has been found
 *
 * Outp: return code:
 *       FALSE      - Error, OEM area not found
 *       TRUE       - Success, OEM area found
 *---------------------------------------------------------------------------
 */
UINT16 FindEpiOemArea
(
    unsigned char *pBuffer,
    UINT32  nSize,
    UINT32 *pFoundIndex
)
{
    UINT32 *pTemp;
    UINT32 nIndex = 0;
    
    pTemp = (UINT32 *)pBuffer;

    do
    {
        if((*(pTemp + nIndex)) == CG_EPDA_USER_AREA_START_L)
        {
            if((*(pTemp + nIndex +1)) == CG_EPDA_USER_AREA_START_H)
            {
                *pFoundIndex = nIndex * 4;
                return TRUE;
            }
        }
        nIndex = nIndex + 1;
    }while((nIndex*4) < nSize);
    
    return FALSE;
}



/*---------------------------------------------------------------------------
 * Name: VerifyEpiModuleFile
 * Desc: Check whether the specified file is a valid EPI module.
 * Inp:  pInputFilename - pointer to input file name for module data
 *
 * Outp: return code:
 *          TRUE    - File is an EPI module
 *          FALSE   - File is no EPI module
 *---------------------------------------------------------------------------
 */
UINT16 VerifyEpiModuleFile
(
    _TCHAR *pInputFilename
)
{
    FILE *fpInDatafile = NULL;
    static CG_MPFA_MODULE_HEADER MpfaHeader;

    // Try to open input data file.   
    if(!(fpInDatafile = fopen(pInputFilename, "rb")))
    {
        return FALSE;
    }
         
    // Try to read module header
    fread(&MpfaHeader, sizeof(MpfaHeader), 1, fpInDatafile );
    if( ferror( fpInDatafile ) )      
    {      
        fclose(fpInDatafile);
        return FALSE;
    }
    
    // We only verify that the module type matches.
    // Whether it is a valid module at all is already checked at another location. 
    if(MpfaHeader.modType == CG_MPFA_TYPE_PDA)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

/*---------------------------------------------------------------------------
 * Name: CheckEdid13Data
 * Desc: Check whether the data referenced is a valid EDID 1.3 data set.
 * Inp:  pDataBuffer - Pointer to EDID 1.3 data set
 *       nDataSize   - Size of the data set
 *
 * Outp: return code:
 *          TRUE    - File is an valid EDID 1.3 data set
 *          FALSE   - File is no valid EDID 1.3 data set
 *---------------------------------------------------------------------------
 */
UINT16 CheckEdid13Data
(
    unsigned char *pDataBuffer,
    UINT32 nDataSize
)
{

    UINT16 i;
    unsigned char nChecksum = 0;

    //First check header and size
    if((nDataSize != SIZE_EDID13_DATA) || (*(UINT32*)pDataBuffer != EDID13_HEADER_SIGNATURE1) || (*((UINT32*)pDataBuffer + 1) != EDID13_HEADER_SIGNATURE2))
    {
        return FALSE;
    }
    // Verify checksum
    for(i=0; i < (nDataSize - 1); i++)
    {
        nChecksum = nChecksum + *(pDataBuffer + i);
    }
    nChecksum = (unsigned char)(0x100 - nChecksum);
    if(nChecksum != *(pDataBuffer + (nDataSize - 1)))
    {
        return FALSE;
    }
    return TRUE;
}

/*---------------------------------------------------------------------------
 * Name: CheckEdid20Data
 * Desc: Check whether the data referenced is a valid EDID 2.0 data set.
 * Inp:  pDataBuffer - Pointer to EDID 2.0 data set
 *       nDataSize   - Size of the data set
 *
 * Outp: return code:
 *          TRUE    - File is an valid EDID 2.0 data set
 *          FALSE   - File is no valid EDID 2.0 data set
 *---------------------------------------------------------------------------
 */
UINT16 CheckEdid20Data
(
    unsigned char *pDataBuffer,
    UINT32 nDataSize
)
{
    UINT16 i;
    unsigned char nChecksum = 0;

    //First check header and size
    if((nDataSize != SIZE_EDID20_DATA) || (*pDataBuffer != EDID20_HEADER_SIGNATURE))
    {
        return FALSE;
    }
    // Verify checksum
    for(i=0; i < (nDataSize - 1); i++)
    {
        nChecksum = nChecksum + *(pDataBuffer + i);
    }
    nChecksum = (unsigned char)(0x100 - nChecksum);
    if(nChecksum != *(pDataBuffer + (nDataSize - 1)))
    {
        return FALSE;
    }
    return TRUE;
}

/*---------------------------------------------------------------------------
 * Name: CheckCongatecData
 * Desc: Check whether the data referenced is a congatec panel selection set.
 * Inp:  pDataBuffer - Pointer to congatec selection set
 *       nDataSize   - Size of the data set
 *
 * Outp: return code:
 *          TRUE    - File is a valid congatec selection set
 *          FALSE   - File is no valid congatec selection set
 *---------------------------------------------------------------------------
 */
UINT16 CheckCongatecData
(
    unsigned char *pDataBuffer,
    UINT32 nDataSize
)
{
    //Check header and size
    if( (*((UINT32*)pDataBuffer) != CG_EPDA_EEPROM_ID_FILE_HDR_ID_L) || 
        (*((UINT32*)pDataBuffer + 1) != CG_EPDA_EEPROM_ID_FILE_HDR_ID_H))
    {
        return FALSE;
    }
    
    return TRUE;
}

/*---------------------------------------------------------------------------
 * Name: CgEpiStart
 * Desc: Perform necessary operations to prepare EPI module access.
 * Inp:  none
 * Outp: return code:
 *       CG_EPIRET_INTRF_ERROR  - Interface access error 
 *       CG_EPIRET_ERROR        - Execution error
 *       CG_EPIRET_OK           - Success
 *---------------------------------------------------------------------------
 */
UINT16 CgEpiStart(void)
{        
    UINT16 retVal;
    UINT32 nI2CCount, nIndex;

    if ((retVal = CgMpfaStart(FALSE)) != CG_MPFARET_OK)
    {
        return retVal;
    }
    if(g_nOperationTarget == OT_BOARD)
    {
        if((nI2CCount = CgosI2CCount(hCgos)) != 0)
        {
            g_nDDCBusIndex = 0xFFFFFFFF;
            for(nIndex = 0; nIndex < nI2CCount; nIndex++)
            {
                if(CgosI2CType(hCgos, nIndex) == CGOS_I2C_TYPE_DDC)                
                {
                    g_nDDCBusIndex = nIndex;
                    break;
                }
            }
            if(g_nDDCBusIndex == 0xFFFFFFFF)
            {
                retVal = CG_EPIRET_INTRF_ERROR;
            }
            else
            {
                retVal = CG_EPIRET_OK;
            }
        }
        else
        {
            retVal = CG_EPIRET_INTRF_ERROR;
        }
    }
    return retVal;
}

/*---------------------------------------------------------------------------
 * Name: CgEpiEnd
 * Desc: Perform necessary cleanup to end EPI access.
 * Inp:  none
 * Outp: return code:
 *       CG_EPIRET_INTRF_ERROR  - Interface access error 
 *       CG_EPIRET_ERROR        - Execution error
 *       CG_EPIRET_OK           - Success
 *---------------------------------------------------------------------------
 */
UINT16 CgEpiEnd(void)
{       
    return CgMpfaEnd(); 
}


/*---------------------------------------------------------------------------
 * Name: CgEpiReadEpiFromMpfa
 * Desc: Find EPI module and save it to specified output file.       
 * Inp:  pOutputFilename - pointer to output file name for module data
 *
 * Outp: return code:
 *       CG_EPIRET_INTRF_ERROR - Interface access error
 *       CG_EPIRET_NOTFOUND    - Error, module not found 
 *       CG_EPIRET_ERROR       - Execution error
 *       CG_MPFARET_OK         - Success, module saved
 *       CG_EPIRET_ERROR_FILE  - File processing error
 *---------------------------------------------------------------------------
 */
UINT16 CgEpiReadEpiFromMpfa
(
    _TCHAR *pOutputFilename
)
{
    localMpfaHeader.modType = CG_MPFA_TYPE_PDA;
    return CgMpfaSaveModule(&localMpfaHeader, pOutputFilename,CG_MPFACMP_TYPE,CG_MPFASFL_MOD);
}

/*---------------------------------------------------------------------------
 * Name: CgEpiWriteEpiToMpfa
 * Desc: Load EPI module from file and write it to MPFA section.       
 * Inp:  pInputFilename - pointer to input file name for module data
 *       bRestart       - TRUE: perfrom changes and restart to do more
 *                        FALSE: only perform changes
 * Outp: return code:
 *      CG_EPIRET_OK            - Success
 *      CG_EPIRET_NOTALLOWED    - Operation not allowed with current
 *                                access level
 *      CG_EPIRET_ERROR         - Execution error
 *      CG_EPIRET_ERROR_FILE    - Module input file processing error
 *      CG_EPIRET_ERROR_SIZE    - Not enough room to add module
 *      CG_EPIRET_INTRF_ERROR   - Interface access error
 *      CG_EPIRET_INCOMP        - Module incompatible to operation target 
 *      CG_EPIRET_INV           - Invalid MPFA module
 *      CG_EPIRET_INV_DATA      - Invalid MPFA module data
 *      CG_EPIRET_INV_PARM      - Invalid MPFA module parameters
 *---------------------------------------------------------------------------
 */
UINT16 CgEpiWriteEpiToMpfa
(
    _TCHAR *pInputFilename,
    UINT16 bRestart
)
{
    UINT16 retVal; 

    if(VerifyEpiModuleFile(pInputFilename) != TRUE)
    {
        return CG_EPIRET_INV;
    }
    retVal = CgMpfaAddModule(pInputFilename,CGUTL_ACC_LEV_CONGA, FALSE);
    if(retVal != CG_MPFARET_OK)
    {
        return retVal;
    }
    else
    {
        return CgMpfaApplyChanges(bRestart);
    }
}

/*---------------------------------------------------------------------------
 * Name: CgEpiReadEdidFromEEP
 * Desc: Read EPI EEPROM contents and save it.
 * Inp:  pOutputFilename - pointer to output file name
 *
 * Outp: return code:
 *      CG_EPIRET_OK            - Success
 *      CG_EPIRET_ERROR         - Execution error
 *      CG_EPIRET_INTRF_ERROR   - Interface access error
 *      CG_EPIRET_INV           - Invalid EEPROM data
 *      CG_EPIRET_ERROR_FILE    - Output file processing error
 *---------------------------------------------------------------------------
 */
UINT16 CgEpiReadEdidFromEEP
(
    _TCHAR *pOutputFilename
)
{
    UINT32 nDataSize;
    unsigned char *pTempBuffer = NULL;
    FILE *fpOutDatafile = NULL;
    unsigned char nIndex;

    // Allocate buffer for max. supported data set = EDID 2.0  
    if( (pTempBuffer = (unsigned char *)malloc(SIZE_EDID20_DATA)) == NULL)
    {
        return CG_EPIRET_ERROR;
    }

    // First read 8 bytes from EEPROM to determine data set type
    nIndex = 0;
    // Set index
    if(!( CgosI2CWrite(hCgos, g_nDDCBusIndex /*CGOS_I2C_TYPE_DDC*/, 0xA0, &nIndex, 1) ))
    {
        free(pTempBuffer);
        return CG_EPIRET_INTRF_ERROR;
    }
    // Read 8 bytes
    if(!( CgosI2CRead(hCgos, g_nDDCBusIndex /*CGOS_I2C_TYPE_DDC*/, 0xA1, pTempBuffer, 8) ))
    {
        free(pTempBuffer);
        return CG_EPIRET_INTRF_ERROR;
    }
    // Now check data set type to determine total read length
    if( (*((UINT32*)pTempBuffer) == CG_EPDA_EEPROM_ID_FILE_HDR_ID_L) &&
        (*((UINT32*)pTempBuffer + 1) == CG_EPDA_EEPROM_ID_FILE_HDR_ID_H))
    {
        // We have a congatec ID data set
        nDataSize = SIZE_EPDA_EEPROM_ID_FILE;        
    }
    else if( (*(UINT32*)pTempBuffer == EDID13_HEADER_SIGNATURE1) && (*((UINT32*)pTempBuffer + 1) == EDID13_HEADER_SIGNATURE2) )
    {
        // We have an EDID 1.3/EPI data set
        nDataSize = SIZE_EDID13_DATA;
    }
    else
    {
        // Assume EDID 2.0 data set
        nDataSize = SIZE_EDID20_DATA;
    }


    // Read complete data from EEPROM
    nIndex = 0;
    // Set index
    if(!( CgosI2CWrite(hCgos, g_nDDCBusIndex /*CGOS_I2C_TYPE_DDC*/, 0xA0, &nIndex, 1) ))
    {
        free(pTempBuffer);
        return CG_EPIRET_INTRF_ERROR;
    }
    // Read data for detected data set type
    if(!( CgosI2CRead(hCgos, g_nDDCBusIndex /*CGOS_I2C_TYPE_DDC*/, 0xA1, pTempBuffer, nDataSize) ))
    {
        free(pTempBuffer);
        return CG_EPIRET_INTRF_ERROR;
    }

    // Now verify data set and set output data size again
    if(CheckCongatecData(pTempBuffer,SIZE_EPDA_EEPROM_ID_FILE))
    {
        nDataSize = SIZE_EPDA_EEPROM_ID_FILE;
    }
    else if(CheckEdid13Data(pTempBuffer,SIZE_EDID13_DATA))
    {
        nDataSize = SIZE_EDID13_DATA;
    }
    else if(CheckEdid20Data(pTempBuffer,SIZE_EDID20_DATA))
    {
        nDataSize = SIZE_EDID20_DATA;
    }
    else
    {
        free(pTempBuffer);
        return CG_EPIRET_INV;
    }

    // Open the output file and save data.
    if (!(fpOutDatafile = fopen(pOutputFilename, "wb")))
    {
        free(pTempBuffer);
        return CG_EPIRET_ERROR_FILE;
    }
  
    if(fwrite(pTempBuffer, sizeof(unsigned char),nDataSize, fpOutDatafile ) != nDataSize)
    {
        fclose(fpOutDatafile);
        free(pTempBuffer);
        return CG_EPIRET_ERROR_FILE;
    }

    fclose(fpOutDatafile);
    free(pTempBuffer);

    return CG_EPIRET_OK;
}

/*---------------------------------------------------------------------------
 * Name: CgEpiWriteEdidToEEP
 * Desc: Write panel data file to EPI EEPROM.
 * Inp:  pInputFilename - pointer to input file name
 *
 * Outp: return code:
 *      CG_EPIRET_OK            - Success
 *      CG_EPIRET_ERROR         - Execution error
 *      CG_EPIRET_INTRF_ERROR   - Interface access error
 *      CG_EPIRET_INV           - Invalid EEPROM data
 *      CG_EPIRET_ERROR_FILE    - Input file processing error
 *---------------------------------------------------------------------------
 */
UINT16 CgEpiWriteEdidToEEP
(
    _TCHAR *pInputFilename
)
{
    UINT32 nDataSize;
    UINT16 retVal;
    unsigned char *pTempBuffer = NULL;
    unsigned char *pCheckBuffer = NULL;
    FILE *fpInDatafile = NULL;
    INT32 lTempFileSize;
    unsigned char nIndex;

    // Try to open input data file.   
    if(!(fpInDatafile = fopen(pInputFilename, "rb")))
    {
        return CG_EPIRET_ERROR_FILE;
    }
    
    // Get length of data block.
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
                retVal = CG_EPIRET_ERROR_FILE;
            }
        }
        else
        {
            retVal = CG_EPIRET_ERROR_FILE;
        }
    }
    else
    {
        retVal = CG_EPIRET_ERROR_FILE;
    }
    
    if(retVal != CG_EPIRET_OK)
    {
        fclose(fpInDatafile);
        return retVal;
    }

    // If data file is larger than EDID 2.0 structure, it must be invalid.
    if(nDataSize > SIZE_EDID20_DATA)
    {
        fclose(fpInDatafile);
        return CG_EPIRET_INV;
    }
    // Allocate buffer 
    if( (pTempBuffer = (unsigned char *)malloc(nDataSize)) == NULL)
    {
        fclose(fpInDatafile);
        return CG_EPIRET_ERROR;
    }
    
    // Write data from file to temporary buffer
    fread(pTempBuffer, nDataSize, 1, fpInDatafile );
    if( ferror( fpInDatafile ) )      
    {      
        retVal = CG_EPIRET_ERROR_FILE;
    }
    else
    {
        if( (CheckEdid13Data(pTempBuffer,nDataSize)) || (CheckEdid20Data(pTempBuffer,nDataSize)) || 
            (CheckCongatecData(pTempBuffer,nDataSize)) )
        {
            retVal = CG_EPIRET_OK;
        }
        else
        {
            retVal = CG_EPIRET_INV;
        }
    }
    fclose(fpInDatafile);      
    if(retVal != CG_EPIRET_OK)
    {
        free(pTempBuffer);
        return retVal;
    }

    // Now write data block to EEPROM
    for(nIndex=0; nIndex < nDataSize; nIndex++)
    {
        if(!(CgosI2CWriteRegister(hCgos, g_nDDCBusIndex, 0xA0, nIndex, *(pTempBuffer + nIndex))))
        {
            free(pTempBuffer);
            return CG_EPIRET_INTRF_ERROR;
        }
        Sleep(10L);
    }


    // Read back EEPROM data and compare it with original data
    // Allocate check buffer
    if( (pCheckBuffer = (unsigned char *)malloc(nDataSize)) == NULL)
    {
        free(pTempBuffer);
        return CG_EPIRET_ERROR;
    }

    // Read data from EEPROM
    nIndex = 0;
    // Set index
    if(!( CgosI2CWrite(hCgos, g_nDDCBusIndex /*CGOS_I2C_TYPE_DDC*/, 0xA0, &nIndex, 1) ))
    {
        free(pTempBuffer);
        free(pCheckBuffer);
        return CG_EPIRET_INTRF_ERROR;
    }
    // Read data
    if(!( CgosI2CRead(hCgos, g_nDDCBusIndex /*CGOS_I2C_TYPE_DDC*/, 0xA1, pCheckBuffer, nDataSize) ))
    {
        free(pTempBuffer);
        free(pCheckBuffer);
        return CG_EPIRET_INTRF_ERROR;
    }

    // Compare data read back with original data
    for(nIndex = 0; nIndex < nDataSize; nIndex++)
    {
        if(*(pCheckBuffer+nIndex) != *(pTempBuffer+nIndex))
        {
            free(pTempBuffer);
            free(pCheckBuffer);
            return CG_EPIRET_INTRF_ERROR;
        }
    }

    free(pTempBuffer);
    free(pCheckBuffer);
    return CG_EPIRET_OK;
}

/*---------------------------------------------------------------------------
 * Name: CgEpiReadStdEdidFromMpfa
 * Desc: Find standard EDID set in EPI module and save it to specified output file.       
 * Inp:  pOutputFilename - pointer to output file name
 *       nStdEdidNo      - Standard EDID data set number
 *
 * Outp: return code:
 *      CG_EPIRET_OK            - Success
 *      CG_EPIRET_NOTALLOWED    - Operation not allowed with current
 *                                access level
 *      CG_EPIRET_ERROR         - Execution error
 *      CG_EPIRET_ERROR_FILE    - output file processing error
 *      CG_EPIRET_INTRF_ERROR   - Interface access error
 *      CG_EPIRET_NOTFOUND      - EPI module / data set not found
 *---------------------------------------------------------------------------
 */
UINT16 CgEpiReadStdEdidFromMpfa
(
    _TCHAR *pOutputFilename,
    UINT16 nStdEdidNo
)
{
    UINT16 retVal;
    UINT32 nFoundIndex, nEpiEndIndex;
    unsigned char * pTempData = NULL;
    UINT32 nEpiDataSize;
    FILE *fpOutDatafile = NULL;

    localMpfaHeader.modType = CG_MPFA_TYPE_PDA; // That's the module we are looking for
    if((retVal = CgMpfaFindModule(&CgMpfaStaticInfo,&localMpfaHeader, 0, &nFoundIndex, CG_MPFACMP_TYPE)) != CG_MPFARET_OK)
    {
        // EPI module not found
        return retVal;
    }

    pTempData = CgMpfaStaticInfo.pSectionBuffer + nFoundIndex + sizeof(localMpfaHeader);    // Points to EPI module data
    nEpiDataSize = ((CG_MPFA_MODULE_HEADER *)pTempData)->modSize - sizeof(localMpfaHeader); // Store total EPI module size

    // We have found the EPI module in the MPFA static section. Now find the OEM section start
    // to see where the EPI standard data entries end.
    if(!FindEpiOemArea(pTempData, nEpiDataSize, &nEpiEndIndex))
    {
        // There is no OEM data area, so just subtract the size of the module END structure
        nEpiEndIndex = nEpiDataSize - sizeof(localMpfaEnd);
    }

    // Now we have the absolute size of the EPI module data area
    nEpiDataSize = nEpiEndIndex;

    // Now check whether the selected data set is contained within our EPI module
    if(((UINT32)(nStdEdidNo * SIZE_EDID13_DATA)) >= nEpiDataSize)
    {
        return CG_EPIRET_NOTFOUND;
    }

    // Check whether the entry really contains a data set (it might as well be only an
    // empty placeholder
    if(!CheckEdid13Data((pTempData + (nStdEdidNo * SIZE_EDID13_DATA)),SIZE_EDID13_DATA))
    {
        return CG_EPIRET_NOTFOUND;
    }

    retVal = CG_EPIRET_OK;

    // Open the output file and save data.
    if (!(fpOutDatafile = fopen(pOutputFilename, "wb")))
    {
        return CG_EPIRET_ERROR_FILE;
    }
    if(fwrite((pTempData + (nStdEdidNo * SIZE_EDID13_DATA)), sizeof(unsigned char),SIZE_EDID13_DATA, fpOutDatafile ) != SIZE_EDID13_DATA)
    {
        retVal = CG_EPIRET_ERROR_FILE;
    }
    fclose(fpOutDatafile);

    return retVal;
}

/*---------------------------------------------------------------------------
 * Name: CgEpiReadOEMEdidFromMpfa
 * Desc: Find OEM EDID set in EPI module and save it to specified output file.       
 * Inp:  pOutputFilename - pointer to output file name
 *       nOEMEdidNo      - OEM EDID data set number
 *
 * Outp: return code:
 *      CG_EPIRET_OK            - Success
 *      CG_EPIRET_NOTALLOWED    - Operation not allowed with current
 *                                access level
 *      CG_EPIRET_ERROR         - Execution error
 *      CG_EPIRET_ERROR_FILE    - output file processing error
 *      CG_EPIRET_INTRF_ERROR   - Interface access error
 *      CG_EPIRET_NOTFOUND      - EPI module / data set not found
 *---------------------------------------------------------------------------
 */
UINT16 CgEpiReadOEMEdidFromMpfa
(
    _TCHAR *pOutputFilename,
    UINT16 nOEMEdidNo
)
{
    UINT16 retVal;
    UINT32 nFoundIndex;
    unsigned char * pTempData = NULL;
    UINT32 nModSize;
    FILE *fpOutDatafile = NULL;

    localMpfaHeader.modType = CG_MPFA_TYPE_PDA; // That's the module we are looking for
    if((retVal = CgMpfaFindModule(&CgMpfaStaticInfo,&localMpfaHeader, 0, &nFoundIndex, CG_MPFACMP_TYPE)) != CG_MPFARET_OK)
    {
        // EPI module not found
        return retVal;
    }

    pTempData = CgMpfaStaticInfo.pSectionBuffer + nFoundIndex;  // Points to EPI module
    nModSize = ((CG_MPFA_MODULE_HEADER *)pTempData)->modSize;   // Store total EPI module size

    // We have found the EPI module in the MPFA static section. Now find the OEM section start
    // and check whether there is a OEM data set contained.
    if(!FindEpiOemArea(pTempData, nModSize, &nFoundIndex))
    {
        return CG_EPIRET_NOTFOUND;
    }
    if((nFoundIndex + sizeof(localMpfaEnd) + EPI_OEMBLOCK_SEP_SIZE + (nOEMEdidNo * SIZE_EDID13_DATA)) >= nModSize)
    {
        return CG_EPIRET_NOTFOUND;
    }

    // Check whether the OEM entry really contains a data set (it might as well be only an
    // empty placeholder 
    if(!CheckEdid13Data((pTempData + nFoundIndex + EPI_OEMBLOCK_SEP_SIZE + (nOEMEdidNo * SIZE_EDID13_DATA)),SIZE_EDID13_DATA))
    {
        return CG_EPIRET_NOTFOUND;
    }

    retVal = CG_EPIRET_OK;

    // Open the output file and save data.
    if (!(fpOutDatafile = fopen(pOutputFilename, "wb")))
    {
        return CG_EPIRET_ERROR_FILE;
    }
    if(fwrite((pTempData + nFoundIndex + EPI_OEMBLOCK_SEP_SIZE + (nOEMEdidNo * SIZE_EDID13_DATA)), sizeof(unsigned char),SIZE_EDID13_DATA, fpOutDatafile ) != SIZE_EDID13_DATA)
    {
        retVal = CG_EPIRET_ERROR_FILE;
    }
    fclose(fpOutDatafile);

    return retVal;
}

/*---------------------------------------------------------------------------
 * Name: CgEpiWriteOEMEdidToMpfa
 * Desc: Load OEM EDID set from file and write it to EPI module in MPFA section.       
 * Inp:  pInputFilename - pointer to input file name for module data
 *       nOEMEdidNo     - OEM EDID data set number
 *       bRestart       - TRUE: perform changes and restart to do more
 *                        FALSE: only perform changes
 *
 * Outp: return code:
 *      CG_EPIRET_OK            - Success
 *      CG_EPIRET_NOTALLOWED    - Operation not allowed with current
 *                                access level
 *      CG_EPIRET_ERROR         - Execution error
 *      CG_EPIRET_ERROR_FILE    - Module input file processing error
 *      CG_EPIRET_ERROR_SIZE    - Not enough room to add module
 *      CG_EPIRET_INTRF_ERROR   - Interface access error
 *      CG_EPIRET_INCOMP        - Module incompatible to operation target 
 *      CG_EPIRET_INV           - Invalid MPFA module
 *      CG_EPIRET_INV_DATA      - Invalid MPFA module data
 *      CG_EPIRET_INV_PARM      - Invalid MPFA module parameters
 *      CG_EPIRET_NOTFOUND      - EPI module / data set entry not found
 *---------------------------------------------------------------------------
 */
UINT16 CgEpiWriteOEMEdidToMpfa
(
    _TCHAR *pInputFilename,
    UINT16 nOEMEdidNo,
    UINT16 bRestart
)
{
    UINT16 retVal;
    UINT32 nFoundIndex;
    unsigned char *pTempData = NULL;
    unsigned char *pTempBuffer = NULL;
    UINT32 nModDataSize, nDataSizeFile,nDataSize;
    FILE *fpInDatafile = NULL;
    FILE *fpTempDatafile = NULL;
    unsigned char szTempFilename[] = "TEMPEPDA.EPI";

    localMpfaHeader.modType = CG_MPFA_TYPE_PDA; // That's the module we are looking for
    if((retVal = CgMpfaFindModule(&CgMpfaStaticInfo,&localMpfaHeader, 0, &nFoundIndex, CG_MPFACMP_TYPE)) != CG_MPFARET_OK)
    {
        // EPI module not found
        return retVal;
    }

    pTempData = CgMpfaStaticInfo.pSectionBuffer + nFoundIndex;                              // Points to EPI module
    nModDataSize = ((CG_MPFA_MODULE_HEADER *)pTempData)->modSize - sizeof(localMpfaHeader) - 
        sizeof(localMpfaEnd);               // Store total EPI module DATA size
    pTempData = CgMpfaStaticInfo.pSectionBuffer + nFoundIndex + sizeof(localMpfaHeader);    // Points to EPI DATA

    // We have found the EPI module in the MPFA static section. Now find the OEM section start
    // and check whether there is a OEM data set contained.
    if(!FindEpiOemArea(pTempData, nModDataSize, &nFoundIndex))
    {
        return CG_EPIRET_NOTFOUND;
    }
    if((nFoundIndex + EPI_OEMBLOCK_SEP_SIZE) >= nModDataSize)
    {
        return CG_EPIRET_NOTFOUND;
    }

    // Check whehter the current EPI data module supports the OEM EDID data set index specified
    if(((nFoundIndex + EPI_OEMBLOCK_SEP_SIZE)+((nOEMEdidNo+1)*SIZE_EDID13_DATA)) > nModDataSize)
    {
        return CG_EPIRET_INV_PARM;
    }

    // Try to open input data file.   
    if(!(fpInDatafile = fopen(pInputFilename, "rb")))
    {
        return CG_EPIRET_INTRF_ERROR;
    }
    
    // Get length of data block and check whether it is DWORD aligned!
    // Set file pointer to end of file
    if(!fseek(fpInDatafile,0, SEEK_END))
    {
        // Get position at end of file = file length
        if(!((nDataSizeFile = ftell(fpInDatafile)) < 0))
        {
            // Set file pointer back to start of file
            if(!fseek(fpInDatafile,0, SEEK_SET))
            {
                retVal = CG_EPIRET_OK;       
            }
            else
            {
                retVal = CG_EPIRET_INTRF_ERROR;
            }
        }
        else
        {
            retVal = CG_EPIRET_INTRF_ERROR;
        }
    }
    else
    {
        retVal = CG_EPIRET_INTRF_ERROR;
    }
    
    if(retVal != CG_EPIRET_OK)
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
    
    // Allocate temporary buffer to hold file data
    pTempBuffer = (unsigned char*)malloc(nDataSize);
    if(pTempBuffer == NULL)
    {
        fclose(fpInDatafile);
        return CG_MPFARET_ERROR;
    }
            
    // Load data to buffer
    fread(pTempBuffer, nDataSizeFile, 1, fpInDatafile );
    if( ferror( fpInDatafile ) )      
    {      
        retVal = CG_EPIRET_ERROR;
    }
    else
    {
        if(CheckEdid13Data(pTempBuffer,nDataSizeFile))
        {
            retVal = CG_EPIRET_OK;
        }
        else
        {
            retVal = CG_EPIRET_INV;
        }
    }
    fclose(fpInDatafile);      
    if(retVal != CG_EPIRET_OK)
    {
        free(pTempBuffer);
        return retVal;
    }

    // Open the temporary output file and save data.
    if (!(fpTempDatafile = fopen( ( char * ) szTempFilename, "wb")))
    {
        return CG_EPIRET_INTRF_ERROR;
    }
    // First write current EPI data to file including existing OEM EDID data sets
    if(fwrite(pTempData, sizeof(unsigned char),nModDataSize/*nFoundIndex + 8*/, fpTempDatafile ) != nModDataSize/*(nFoundIndex + 8)*/)
    {
        fclose(fpTempDatafile);
        free(pTempBuffer);
        remove( ( char * ) szTempFilename);
        return CG_EPIRET_INTRF_ERROR;
    }

    // Then add OEM EDID data block at valid position in file
    fseek(fpTempDatafile,(nFoundIndex + EPI_OEMBLOCK_SEP_SIZE) + (nOEMEdidNo * SIZE_EDID13_DATA), SEEK_SET);
    if(fwrite(pTempBuffer, sizeof(unsigned char),nDataSize, fpTempDatafile ) != nDataSize)
    {
        fclose(fpTempDatafile);
        free(pTempBuffer);
        remove( ( char * ) szTempFilename);
        return CG_EPIRET_INTRF_ERROR;
    }
    free(pTempBuffer);
    fclose(fpTempDatafile);

    localMpfaHeader.modType = CG_MPFA_TYPE_PDA;

    // Now that we have created an EPI data block with OEM EDID data block we
    // re-create and add a valid EPI module file.
    retVal = CgMpfaCreateModule(&localMpfaHeader,  ( char * )szTempFilename,  ( char * ) szTempFilename, CGUTL_ACC_LEV_CONGA, FALSE);
    if(retVal != CG_MPFARET_OK)
    {
        remove( ( char * ) szTempFilename);
        return retVal;
    }

    retVal = CgMpfaAddModule( ( char * ) szTempFilename, CGUTL_ACC_LEV_CONGA, FALSE);
    if(retVal != CG_MPFARET_OK)
    {
        remove( ( char * ) szTempFilename);
        return retVal;
    }
    else
    {
        remove( ( char * ) szTempFilename);
        return CgMpfaApplyChanges(bRestart);
    }
}

/*---------------------------------------------------------------------------
 * Name: CgEpiDelOEMEdidFromMpfa
 * Desc: Remove an OEM data set from the EPI module in the MPFA section.       
 * Inp:  nOEMEdidNo     - OEM EDID data set number
 *       bRestart       - TRUE: perform changes and restart to do more
 *                        FALSE: only perform changes
 *
 * Outp: return code:
 *      CG_EPIRET_OK            - Success
 *      CG_EPIRET_NOTALLOWED    - Operation not allowed with current
 *                                access level
 *      CG_EPIRET_ERROR         - Execution error
 *      CG_EPIRET_INTRF_ERROR   - Interface access error
 *      CG_EPIRET_NOTFOUND      - EPI module / data set entry not found
 *---------------------------------------------------------------------------
 */
UINT16 CgEpiDelOEMEdidFromMpfa
(
    UINT16 nOEMEdidNo,
    UINT16 bRestart
)
{
    UINT16 retVal;
    UINT32 nFoundIndex;
    unsigned char *pTempData = NULL;
    unsigned char *pTempBuffer = NULL;
    UINT32 nModDataSize,nDataSize;
    FILE *fpTempDatafile = NULL;
    unsigned char szTempFilename[] = "TEMPEPDA.EPI";

    localMpfaHeader.modType = CG_MPFA_TYPE_PDA; // That's the module we are looking for
    if((retVal = CgMpfaFindModule(&CgMpfaStaticInfo,&localMpfaHeader, 0, &nFoundIndex, CG_MPFACMP_TYPE)) != CG_MPFARET_OK)
    {
        // EPI module not found
        return retVal;
    }

    pTempData = CgMpfaStaticInfo.pSectionBuffer + nFoundIndex;                              // Points to EPI module
    nModDataSize = ((CG_MPFA_MODULE_HEADER *)pTempData)->modSize - sizeof(localMpfaHeader) - 
        sizeof(localMpfaEnd);               // Store total EPI module DATA size
    pTempData = CgMpfaStaticInfo.pSectionBuffer + nFoundIndex + sizeof(localMpfaHeader);    // Points to EPI DATA

    // We have found the EPI module in the MPFA static section. Now find the OEM section start
    // and check whether there is a OEM data set contained.
    if(!FindEpiOemArea(pTempData, nModDataSize, &nFoundIndex))
    {
        return CG_EPIRET_NOTFOUND;
    }
    if((nFoundIndex + EPI_OEMBLOCK_SEP_SIZE) >= nModDataSize)
    {
        return CG_EPIRET_NOTFOUND;
    }

    // Check whether the current EPI data module supports the OEM EDID data set index specified
    if(((nFoundIndex + EPI_OEMBLOCK_SEP_SIZE)+((nOEMEdidNo+1)*SIZE_EDID13_DATA)) > nModDataSize)
    {
        return CG_EPIRET_INV_PARM;
    }

    
    // Allocate temporary buffer for dummy data
    nDataSize = SIZE_EDID13_DATA;
    pTempBuffer = (unsigned char*)malloc(nDataSize);
    if(pTempBuffer == NULL)
    {
        return CG_MPFARET_ERROR;
    }
    memset(pTempBuffer,0xFF,nDataSize);
            
    // Open the temporary output file and save data.
    if (!(fpTempDatafile = fopen( ( char * ) szTempFilename, "wb")))
    {
        return CG_EPIRET_INTRF_ERROR;
    }
    // First write current EPI data to file including existing OEM EDID data sets
    if(fwrite(pTempData, sizeof(unsigned char),nModDataSize/*nFoundIndex + 8*/, fpTempDatafile ) != nModDataSize/*(nFoundIndex + 8)*/)
    {
        fclose(fpTempDatafile);
        free(pTempBuffer);
        remove( ( char * ) szTempFilename);
        return CG_EPIRET_INTRF_ERROR;
    }

    // Then add OEM EDID data block at valid position in file
    fseek(fpTempDatafile,(nFoundIndex + EPI_OEMBLOCK_SEP_SIZE) + (nOEMEdidNo * SIZE_EDID13_DATA), SEEK_SET);
    if(fwrite(pTempBuffer, sizeof(unsigned char),nDataSize, fpTempDatafile ) != nDataSize)
    {
        fclose(fpTempDatafile);
        free(pTempBuffer);
        remove( ( char * ) szTempFilename);
        return CG_EPIRET_INTRF_ERROR;
    }
    free(pTempBuffer);
    fclose(fpTempDatafile);

    localMpfaHeader.modType = CG_MPFA_TYPE_PDA;

    // Now that we have created an EPI data block with OEM EDID data block we
    // re-create and add a valid EPI module file.
    retVal = CgMpfaCreateModule(&localMpfaHeader,  ( char * )szTempFilename,  ( char * )szTempFilename, CGUTL_ACC_LEV_CONGA, FALSE);
    if(retVal != CG_MPFARET_OK)
    {
        remove( ( char * ) szTempFilename);
        return retVal;
    }

    retVal = CgMpfaAddModule( ( char * )szTempFilename, CGUTL_ACC_LEV_CONGA, FALSE);
    if(retVal != CG_MPFARET_OK)
    {
        remove( ( char * ) szTempFilename);
        return retVal;
    }
    else
    {
        remove( ( char * ) szTempFilename);
        return CgMpfaApplyChanges(bRestart);
    }
}

/*---------------------------------------------------------------------------
 * Name: CgEpiClearEEP
 * Desc: Clear EPI EEPROM.
 * Inp:  None
 *
 * Outp: return code:
 *      CG_EPIRET_OK            - Success
 *      CG_EPIRET_INTRF_ERROR   - Interface access error
 *---------------------------------------------------------------------------
 */
UINT16 CgEpiClearEEP
(
    void
)
{
    UINT32 nDataSize;
    unsigned char nIndex;


    // Assume, that an EPI EEPROM will at least be big enough to hold an EDID 1.3
    // data set
    nDataSize = SIZE_EDID13_DATA;    

    // Clear EEPROM
    for(nIndex=0; nIndex < nDataSize; nIndex++)
    {
        if(!(CgosI2CWriteRegister(hCgos, g_nDDCBusIndex, 0xA0, nIndex, 0xFF)))
        {
            return CG_EPIRET_INTRF_ERROR;
        }
        Sleep(10L);
    }
    return CG_EPIRET_OK;
}

/*---------------------------------------------------------------------------
 * Name: CgEpiSetBLValue
 * Desc: Set backlight control value.
 * Inp:  valueBL - Backlight control value.
 *
 * Outp: return code:
 *      CG_EPIRET_OK            - Success
 *      CG_EPIRET_INV           - Invalid parameter
 *      CG_EPIRET_INTRF_ERROR   - Interface access error
 *---------------------------------------------------------------------------
 */
UINT16 CgEpiSetBLValue
(
    UINT32 valueBL
)
{
    if(valueBL > CGOS_VGA_BACKLIGHT_MAX)
    {
        return CG_EPIRET_INV;
    }

    if(!CgosVgaSetBacklight(hCgos, FIX_TYPE, valueBL))
    {
        return CG_EPIRET_INTRF_ERROR;
    }

    return  CG_EPIRET_OK;
}

/*---------------------------------------------------------------------------
 * Name: CgEpiGetBLValue
 * Desc: Get current backlight control value.
 * Inp:  pValueBL - Pointer to variable for backlight control value.
 *
 * Outp: return code:
 *      CG_EPIRET_OK            - Success
 *      CG_EPIRET_INTRF_ERROR   - Interface access error
 *---------------------------------------------------------------------------
 */
UINT16 CgEpiGetBLValue
(
    UINT32 *pValueBL
)
{
    if(!CgosVgaGetBacklight(hCgos, FIX_TYPE, pValueBL))
    {
        return CG_EPIRET_INTRF_ERROR;
    }
    return  CG_EPIRET_OK;
}

/*---------------------------------------------------------------------------
 * Name: CgEpiSetBLEnable
 * Desc: Set backlight enable state.
 * Inp:  stateBL - Backlight enable state.
 *                    1 :  BL Enabled
 *                    0 :  BL Disabled
 *
 * Outp: return code:
 *      CG_EPIRET_OK            - Success
 *      CG_EPIRET_INV           - Invalid parameter
 *      CG_EPIRET_INTRF_ERROR   - Interface access error
 *---------------------------------------------------------------------------
 */
UINT16 CgEpiSetBLEnable
(
    UINT32 stateBL
)
{
    if((stateBL != 0) && (stateBL != 1))
    {
        return CG_EPIRET_INV;
    }
    if(!CgosVgaSetBacklightEnable(hCgos, FIX_TYPE, stateBL))
    {
        return CG_EPIRET_INTRF_ERROR;
    }
    return  CG_EPIRET_OK;
}

/*---------------------------------------------------------------------------
 * Name: CgEpiGetBLEnable
 * Desc: Get current backlight enable state.
 * Inp:  pStateBL    - Pointer to variable for backlight enable state.
 *                    1 : BL Enabled
 *                    0 : BL Disabled
 *
 * Outp: return code:
 *      CG_EPIRET_OK            - Success
 *      CG_EPIRET_INTRF_ERROR   - Interface access error
 *---------------------------------------------------------------------------
 */
UINT16 CgEpiGetBLEnable
(
    UINT32 *pStateBL
)
{
    if(!CgosVgaGetBacklightEnable(hCgos, FIX_TYPE, pStateBL))
    {
        return CG_EPIRET_INTRF_ERROR;
    }
    return  CG_EPIRET_OK;
}

/*---------------------------------------------------------------------------
 * Name: CgEpiGetEpiDataSetDesc
 * Desc: Find specified EPI data set in the EPI module, analyse the data,
 *       build and return a description string.
 * Inp:  lpszEpiDesc    - Pointer to zero terminated string that will hold 
 *                        the EPI data set description.
 *       nDataSetNo     - EPI data set number
 *       bOemSelect     - If TRUE nDataSet specifies an OEM entry number, else
 *                        a standard data set number
 *
 * Outp: return code:
 *      CG_EPIRET_OK            - Success
 *      CG_EPIRET_NOTALLOWED    - Operation not allowed with current
 *                                access level
 *      CG_EPIRET_ERROR         - Execution error
 *      CG_EPIRET_INTRF_ERROR   - Interface access error
 *      CG_EPIRET_NOTFOUND      - EPI module / data set not found
 *---------------------------------------------------------------------------
 */
UINT16 CgEpiGetEpiDataSetDesc
(
    _TCHAR *lpszEpiDesc,
    UINT16 nDataSetNo,
    UINT16 bOemSelect
)
{
    UINT16 retVal, horizRes, vertRes;
    UINT32 nFoundIndex, nEpiStdDataEndIndex;
    unsigned char * pTempData = NULL;
    UINT32 nEpiModSize,nEpiStdBlockSize,nEpiOemBlockSize;
    DTD_DATA *pDTDData;
    EXT_DDT_DATA *pEpiData;
    unsigned char nEpiDataFormatByte;
    _TCHAR szTempString[32];
    
    localMpfaHeader.modType = CG_MPFA_TYPE_PDA; // That's the module we are looking for
    if((retVal = CgMpfaFindModule(&CgMpfaStaticInfo,&localMpfaHeader, 0, &nFoundIndex, CG_MPFACMP_TYPE)) != CG_MPFARET_OK)
    {
        // EPI module not found
        return retVal;
    }

    pTempData = CgMpfaStaticInfo.pSectionBuffer + nFoundIndex ;     // Points to EPI module header
    nEpiModSize = ((CG_MPFA_MODULE_HEADER *)pTempData)->modSize;   // Store total EPI module size

    // Temporary sizes of standard and OEM data block sizes (of course they are wrong at the moment)
    nEpiStdBlockSize =  nEpiModSize;
    nEpiOemBlockSize = 0;

    // We have found the EPI module in the MPFA static section. Now find the OEM section start
    // to see where the EPI standard data entries end.
    if(!FindEpiOemArea(pTempData, nEpiModSize, &nEpiStdDataEndIndex))
    {
        // There is no OEM data area, so just subtract the size of the module END structure
        nEpiStdDataEndIndex = nEpiStdBlockSize - sizeof(localMpfaEnd);
    }

    // Now we can set the actual sizes of the EPI standard data set block and the OEM data 
    // set block and set out pointer to the respective data block start
    nEpiStdBlockSize = nEpiStdDataEndIndex  - sizeof(localMpfaHeader);
    nEpiOemBlockSize = nEpiModSize - sizeof(localMpfaHeader) - sizeof(localMpfaEnd) - 
                       nEpiStdBlockSize - EPI_OEMBLOCK_SEP_SIZE;


    if(bOemSelect)
    {
        // Check whether the selected data set is contained within the OEM data block
        if(((UINT32)(nDataSetNo * SIZE_EDID13_DATA)) >= nEpiOemBlockSize)
        {
            return CG_EPIRET_NOTFOUND;
        }
        pTempData = pTempData + sizeof(localMpfaHeader) + nEpiStdBlockSize + EPI_OEMBLOCK_SEP_SIZE;
    }
    else
    {
        // Check whether the selected data set is contained within the standard data block
        if(((UINT32)(nDataSetNo * SIZE_EDID13_DATA)) >= nEpiStdBlockSize )
        {
            return CG_EPIRET_NOTFOUND;
        }
        pTempData = pTempData + sizeof(localMpfaHeader);
    }
    

    // Check whether the entry really contains a data set (it might as well be only an
    // empty placeholder
    if(!CheckEdid13Data((pTempData + (nDataSetNo * SIZE_EDID13_DATA)),SIZE_EDID13_DATA))
    {
        if(bOemSelect)
        {
            sprintf(szTempString, "OEM%d:", nDataSetNo + 1);
            strcat(lpszEpiDesc, szTempString);
            sprintf(szTempString, " EMPTY");
        }
        else
        {
            sprintf(szTempString, "%2Xh:", nDataSetNo + 1);
            strcat(lpszEpiDesc, szTempString);
            sprintf(szTempString, " RESERVED ENTRY");
        }        
        strcat(lpszEpiDesc, szTempString);
        return CG_EPIRET_OK;
    }

    retVal = CG_EPIRET_OK;

    // Now start analysing the data and building the output string.

    // First write back the data set number
    if(bOemSelect)
    {
        sprintf(szTempString, "OEM%d:", nDataSetNo + 1);
        strcat(lpszEpiDesc, szTempString);
    }
    else
    {
        sprintf(szTempString, "%2Xh:", nDataSetNo + 1);
        strcat(lpszEpiDesc, szTempString);
    }


    // Set up the substruction pointers 
    pEpiData = (EXT_DDT_DATA *) (pTempData + (nDataSetNo * SIZE_EDID13_DATA) + EPI_DATA_OFFSET);
    pDTDData = (DTD_DATA *) (&((EDID13_DATA *)(pTempData + (nDataSetNo * SIZE_EDID13_DATA)))->DetailedTimingDesc);

    // Get the resolution information
    horizRes = (((UINT16) ((pDTDData->H_ActiveBlank) >> 4)) << 8) + 
                ((UINT16) (pDTDData->H_Active)) ;
    vertRes = (((UINT16) ((pDTDData->V_ActiveBlank) >> 4)) << 8) + 
                ((UINT16) (pDTDData->V_Active)) ;

    sprintf(szTempString, " %dx%d,", horizRes,vertRes);
    strcat(lpszEpiDesc, szTempString);

    // Get add. information from EPI specific data    
    nEpiDataFormatByte = pEpiData->DataFormatByte;

    // Get pixels per clock info
    if(((nEpiDataFormatByte >> 3) & 0x03) == 0x00 )
    {
        sprintf(szTempString, " 1x");        
    }
    else if (((nEpiDataFormatByte >> 3) & 0x03) == 0x01 )
    {
        sprintf(szTempString, " 2x");
    }
    else if (((nEpiDataFormatByte >> 3) & 0x03) == 0x02 )
    {
        sprintf(szTempString, " 4x");
    }
    else
    {
        sprintf(szTempString, " ?x");
    }
    strcat(lpszEpiDesc, szTempString);

    // Get bits per pixel info
    if( (nEpiDataFormatByte & 0x07) == 0x00 )
    {
        sprintf(szTempString, "18bit");        
    }
    else if ( (nEpiDataFormatByte & 0x07) == 0x01 )
    {
        sprintf(szTempString, "24bit");
    }
    else if ( (nEpiDataFormatByte & 0x07) == 0x02 )
    {
        sprintf(szTempString, "30bit");
    }
    else
    {
        sprintf(szTempString, "??bit");
    }
    strcat(lpszEpiDesc, szTempString);

    // Add color mapping
    if( (nEpiDataFormatByte & 0x07) != 0x00 )
    {
        sprintf(szTempString, ",");
        strcat(lpszEpiDesc, szTempString);
        if(((nEpiDataFormatByte >> 5) & 0x03) == 0x00 )
        {
            sprintf(szTempString, " VESA");        
        }
        else if (((nEpiDataFormatByte >> 5) & 0x03) == 0x01 )
        {
            sprintf(szTempString, " openLDI");
        }
        else
        {
            sprintf(szTempString, " ???");
        }
        strcat(lpszEpiDesc, szTempString);
    }
    return retVal;
}

