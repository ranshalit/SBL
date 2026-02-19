/*---------------------------------------------------------------------------
 *
 * Copyright (c) 2022, congatec GmbH. All rights reserved.
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
 * Contents: Board controller firmware update common implementation module.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * MOD025: Added support for new command line switches /bldrenable and
 *         /bldrdisable.
 *
 * MOD024: Added support for all MEC170x variants in the 144 pin WFBGA package.
 *
 * MOD023: added support for the /!cidoverride switch.
 *
 * MOD022: added support for MEC1706
 * 
 * MOD021: changes for a better flash read/write performance during a firmware
 *         update on GEN5 cBC based designs
 *
 * MOD020: changes to skip verifying 0xFF data bytes by default in case of the
 *         GEN5 cBC family
 *
 * MOD019: added support for the GEN5 cBC family
 * 
 *    Rev 1.16   Sep 07 2016 12:24:56   congatec
 * Fixed type conversion warnings.
 * 
 *    Rev 1.15   Sep 06 2016 15:18:36   congatec
 * Updated BSD header.
 * 
 *    Rev 1.14   Jul 16 2013 10:16:00   ess
 * MOD018: changes to ignore extended address bits 31-24 for microcontrollers
 *         with have a flash memory address base other than 0 such as the
 *         STM32 controllers
 * 
 *    Rev 1.13   Jul 03 2013 10:31:28   ess
 * MOD017: changes for controllers not supporting the EESAVE fuse
 * MOD016: added support for Intel hex record types 2 (extended segment
 *         address), 3 (start segment) and 4 (extended linear address)
 * MOD015: added support for TM4E1231H6ZRB
 * 
 *    Rev 1.12   May 31 2012 15:45:18   gartner
 * Updated variable definition to ease 64bit porting.
 * 
 *    Rev 1.11   Jun 22 2011 15:03:12   ess
 * MOD014: added diagnostic output on port 80h
 * 
 *    Rev 1.10   May 24 2011 10:55:28   ess
 * MOD013: fixed a firmware verification error which showed up when flashing
 *         a firmware containing a bootblock only but no application section.
 *         In this case the detection of an existing EEEP section failed.
 * 
 *    Rev 1.9   Nov 11 2010 10:09:30   ess
 * MOD012: changes to support bootblocks at the bottom of the flash area and
 *         to support flash areas reserved for EEPROM emulation
 * MOD011: changed the fuse protection handling to support microcontrollers
 *         with specific fuses in different fuse bytes
 * MOD010: added support for STM32F100R8 and new Intel-Hex record types
 * 
 *    Rev 1.8   Nov 19 2009 11:00:32   ess
 * MOD009: added support for ATmega48PA/88PA/168PA/328P
 * 
 *    Rev 1.7   Feb 17 2009 15:07:22   ess
 * MOD008: bug fixed to get programming of ATmega325P working
 * 
 *    Rev 1.6   Dec 18 2008 15:42:36   ess
 * MOD007: added support for ATmega325P
 * 
 *    Rev 1.5   Nov 18 2008 14:16:52   ess
 * MOD006: added CGBC bootblock support
 * 
 *    Rev 1.4   Aug 26 2008 21:44:24   ess
 * MOD005: added usage of the SPM busy bit for bootblock support
 * MOD004: added support for ATmega165P
 * 
 *    Rev 1.3   Apr 15 2008 22:00:56   ess
 * MOD003: added support for the /!nofuseprotection command line switch.
 * 
 *    Rev 1.2   Nov 04 2005 11:32:46   ess
 * MOD002: bug fixed which caused a wrong error address being displayed when
 *         EEPROM data verification failed
 * 
 *    Rev 1.1   Oct 20 2005 18:41:58   ess
 * MOD001: added code to send a POST end signal to the board controller one
 *         second after releasing the board controller reset line / this is
 *         necessary to ensure that the board controller properly detects the
 *         next system restart
 * 
 *    Rev 1.0   Oct 04 2005 13:14:14   gartner
 * Initial revision.
 * 
 *    Rev 1.0   Sep 30 2005 16:31:18   gartner
 * Initial revision.
 *
 *---------------------------------------------------------------------------
 */



#define BCPRG_HW_EMULATION 0



/*---------------
 * Include files
 *---------------
 */

#include "cgutlcmn.h"
#include "atmelavr.h"
#include "mc17avr.h"                                                 //MOD019
#include "stm32avr.h"                                                //MOD010
#include "tivaavr.h"                                                 //MOD015
#include "cgbc.h"
#include "bcprg.h"



/*---------------------
 * External references
 *---------------------
 */

extern HCGOS hCgos;
extern void BcprgShowProgress( INT32 progressCode );


#define MAX_HEX_REC_SIZ    16

#define MAX_FLS_SIZ        MEC1705QSZ_FLASH_SIZE       //MOD010 MOD015 MOD019
#define MAX_EEP_SIZ        MEC1705QSZ_EEPROM_SIZE             //MOD008 MOD019

#define MAX_FLS_PROG_TIME  50
#define MAX_EEP_PROG_TIME  90
#define MAX_FUSE_PROG_TIME 50
#define MAX_ERASE_TIME     90

#define MFG_EEP_SIZE 64

enum  {
       NO_TAG,
       REVISION_TAG,
       FLASH_TAG,
       EEPROM_TAG,
       E_FUSE_AND_TAG,
       E_FUSE_OR_TAG,
       H_FUSE_AND_TAG,
       H_FUSE_OR_TAG,
       L_FUSE_AND_TAG,
       L_FUSE_OR_TAG,
       LOCKS_TAG
      };

typedef struct HEX_REC_STRUCT {
		                         unsigned char colon;
		                         unsigned char lenHi;
		                         unsigned char lenLo;
		                         unsigned char adrHi;
		                         unsigned char adrMh;
		                         unsigned char adrMl;
		                         unsigned char adrLo;
		                         unsigned char typHi;
		                         unsigned char typLo;
		                         unsigned char dat[MAX_HEX_REC_SIZ*2+2];
		                        } HEX_REC, *P_HEX_REC;

typedef struct EEP_ENTRY_STRUCT  {
                                  UINT32  adr;
                                  unsigned char val;
                                 } EEP_ENTRY, *P_EEP_ENTRY;



#ifdef _CONSOLE
static char eesaveWarnMes[]    = "\n"
                                 "WARNING! Currently the EESAVE fuse is not programmed, i.e. the content\n"
                                 "of the controller EEPROM will be destroyed by the chip erase command.\n\n";
static char askContinueMes[]   = "Do you want to continue? [y/n]: ";
static char askEepSaveMes[]    =  "Do you want to preserve the content of the EEPROM by setting the EESAVE\n"
                                  "fuse? [y/n] ";
static char verboseStartMes[]  = "\nVERBOSE\n{\n";
static char verboseEndMes[]    = "}\n\n";
static char _08lXspPat[]       = "%08lX ";
static char nl[]               = "\n";
static char nlNl[]             = "\n\n";
#endif //_CONSOLE

static char verifyInfoPat8[]   = _T("offset: %04Xh  expected: %02Xh  found: %02Xh");
static char verifyInfoPat8L[]  = _T("offset: %08Xh  expected: %02Xh  found: %02Xh"); //MOD021
static char flashReadErrMsg[]  = _T("flash page read error");                        //MOD021
static char verifyInfoPat16[]  = _T("offset: %04Xh  expected: %04Xh  found: %04Xh");
static char verifyInfoPatF[]   = _T("fuses: %c  expected: %02Xh  found: %02Xh");
static char verifyInfoPatL[]   = _T("expected: %02Xh  found: %02Xh");



static char revTag[] =         "CGBCP";
static char rev5Tag[] =        "GEN5P";                              //MOD019
static char flsTag[] =         "FLASH Code";
static char eepTag[] =         "EEPROM Data";
static char lFuseAndTag[] =    "Low Fuse Bits AND Mask";
static char lFuseOrTag[] =     "Low Fuse Bits OR Mask";
static char hFuseAndTag[] =    "High Fuse Bits AND Mask";
static char hFuseOrTag[] =     "High Fuse Bits OR Mask";
static char eFuseAndTag[] =    "Extended Fuse Bits AND Mask";
static char eFuseOrTag[] =     "Extended Fuse Bits OR Mask";
static char locksTag[] =       "Lock Bits";



/*-----------------------------
 * Global and static variables
 *-----------------------------
 */

static UINT32  flags           = 0;
static _TCHAR        *pDatFilNam      = NULL;
static FILE          *pDatFilStream   = NULL;
static unsigned char *pFlsBuf         = NULL;
static P_EEP_ENTRY    pEepBuf         = NULL;
static UINT32   eepEntryCount   = 0;
static UINT32   lowFuseAndMask  = 0xFF;
static UINT32   lowFuseOrMask   = 0;
static unsigned char  lowFuse         = 0;
static UINT32   highFuseAndMask = 0xFF;
static UINT32   highFuseOrMask  = 0;
static unsigned char  highFuse        = 0;
static UINT32   extFuseAndMask  = 0xFF;
static UINT32   extFuseOrMask   = 0;
static unsigned char  extFuse         = 0;
static unsigned char  lockBits        = 0xFF;
static UINT32   flsSiz          = 0;
static UINT32   flsPageSiz      = 0;
static UINT32   codeSiz         = 0;
static UINT32   eepSiz          = 0;
static unsigned char  curLocks        = 0xFF;
static unsigned char  curFusesX       = 0xFF;
static unsigned char  curFusesH       = 0xFF;
static unsigned char  curFusesL       = 0xFF;
                                                                     //MOD004v
static unsigned char *pEesave            = NULL;
static unsigned char  eesaveMsk          = 0x00;
static unsigned char *pSpien             = NULL;
static unsigned char  spienMsk           = 0x00;
static unsigned char *pRstdisbl          = NULL;
static unsigned char  rstdisblMsk        = 0x00;
static unsigned char *pCksel             = NULL;
static unsigned char  ckselMsk           = 0x00;
static unsigned char  intCalib           = 0x00;
static unsigned char  lbModeMsk          = 0x00;
static unsigned char  lbModeProgEnabled  = 0x00;
static unsigned char  lbModeProgDisabled = 0x00;
static UINT32   mfgStart           = 0;
                                                                     //MOD004^
static unsigned char  cgbcFeat        = ' ';
static unsigned char  cgbcRmaj        = ' ';
static unsigned char  cgbcRmin        = ' ';
static INT32            cgbcCid         = -1;
static UINT32  cgbcByteDelay   = 0;
static UINT32  cgbcEdgeDelay   = 15;

static UINT32  bcI2cBus;                                             //MOD006
static UINT32  extAddr = 0;                                          //MOD015



static INT32 SpiInit( void )
{
 if( CgosCgbcSetControl( hCgos, CGEB_BC_CONTROL_SS, 0 ) )
   {
    if( CgosCgbcSetControl( hCgos, CGEB_BC_CONTROL_RESET, 0 ) )
      {
       return( BCPRG_PASSED );
      }
   }
 return( BCPRG_PROG_ENABLE_ERROR );
}



static INT32 SpiReset( void )
{
 if( CgosCgbcSetControl( hCgos, CGEB_BC_CONTROL_RESET, 1 ) )
   {
    return( BCPRG_PASSED );
   }
 return( BCPRG_PROG_ENABLE_ERROR );
}


                                                                     //MOD001v
cgosret_bool SendPostEndSignal( void )
{
 unsigned char wbuf[3];
 unsigned char rbuf[1];
 UINT32 sts;

 wbuf[0] = CGBC_CMD_SYS_FLAGS_1;
 wbuf[1] = 0xFF;
 wbuf[2] = CGBC_SYS_POST_END;
 return( CgosCgbcHandleCommand( hCgos, &wbuf[0], 3, &rbuf[0], 1, &sts ) );
}
                                                                     //MOD001^


static UINT32 Xfer( UINT32 cmd );                                    //MOD019



static void Cleanup( void )
{
 if( flags & BCPRG_PROG_ENABLED )
   {
                                                                     //MOD019v
    if( (flags & BCPRG_AVR_DISABLE_CMD) != 0 )
      {
       Xfer( AVR_SPM_PROG_DISABLE );
      }
                                                                     //MOD019^
    SpiInit();
    Sleep( 1000 );                                                   //MOD001
    SendPostEndSignal();                                             //MOD001
    flags &= ~BCPRG_PROG_ENABLED;
   }

 if( pDatFilStream )
   {
    fclose( pDatFilStream );
    pDatFilStream = NULL;
   }

 if( pFlsBuf )
   {
    free( pFlsBuf );
    pFlsBuf = NULL;
   }

 if( pEepBuf )
   {
    free( pEepBuf );
    pEepBuf = NULL;
   }
}



static UINT32 CheckTag( char *pTagStr )
{
 if( !memcmp( pTagStr, revTag, strlen( revTag ) ) )
   {
    return( REVISION_TAG );
   }
                                                                     //MOD019v 
 if( !memcmp( pTagStr, rev5Tag, strlen( rev5Tag ) ) )
   {
    return( REVISION_TAG );
   }
                                                                     //MOD019^
 if( !memcmp( pTagStr, flsTag, strlen( flsTag ) ) )
   {
    return( FLASH_TAG );
   }

 if( !memcmp( pTagStr, eepTag, strlen( eepTag ) ) )
   {
    return( EEPROM_TAG );
   }

 if( !memcmp( pTagStr, lFuseAndTag, strlen( lFuseAndTag ) ) )
   {
    return( L_FUSE_AND_TAG );
   }

 if( !memcmp( pTagStr, lFuseOrTag, strlen( lFuseOrTag ) ) )
   {
    return( L_FUSE_OR_TAG );
   }

 if( !memcmp( pTagStr, hFuseAndTag, strlen( hFuseAndTag ) ) )
   {
    return( H_FUSE_AND_TAG );
   }

 if( !memcmp( pTagStr, hFuseOrTag, strlen( hFuseOrTag ) ) )
   {
    return( H_FUSE_OR_TAG );
   }

 if( !memcmp( pTagStr, eFuseAndTag, strlen( eFuseAndTag ) ) )
   {
    return( E_FUSE_AND_TAG );
   }

 if( !memcmp( pTagStr, eFuseOrTag, strlen( eFuseOrTag ) ) )
   {
    return( E_FUSE_OR_TAG );
   }

 if( !memcmp( pTagStr, locksTag, strlen( locksTag ) ) )
   {
    return( LOCKS_TAG );
   }

 return( NO_TAG );
}



static UINT32 toint( char c )
{
 if( isdigit( c ) )
   {
    return( c - '0' );
   }
 if( isxdigit( c ) )
   {
    return( (c | 0x20) - 'a' + 10 );
   }
 return( 0xFFFF );
}



static unsigned char HexRecordChecksum( P_HEX_REC p )
{
 UINT32 sum;
 UINT32 i;
 UINT32 len;

 sum =  toint( p->lenHi ) << 4;
 sum += toint( p->lenLo );
 len = sum;
 sum += toint( p->adrHi ) << 4;
 sum += toint( p->adrMh );
 sum += toint( p->adrMl ) << 4;
 sum += toint( p->adrLo );
 sum += toint( p->typHi ) << 4;
 sum += toint( p->typLo );
 for( i=0; i<=len; i++ )
    {
     sum += toint( p->dat[i*2] ) << 4;
     sum += toint( p->dat[i*2+1] );
    }
 return (unsigned char)( sum );
}



static INT32 HandleHexData( char *pDat, UINT32 adr,
                          UINT32 len, UINT32 tag )
{
 UINT32 i;

 switch( tag )
   {
    case FLASH_TAG:
                      if( adr+len > MAX_FLS_SIZ )
                        {
                         return( BCPRG_FLASH_SIZE_ERROR );
                        }
                      memcpy( &pFlsBuf[adr], pDat, len);
                      flags |= BCPRG_FLS_PROG_REQ;
                      break;

    case EEPROM_TAG:
                      if( adr+len > MAX_EEP_SIZ ) 
                        {
                         return( BCPRG_EEPROM_SIZE_ERROR );
                        }
                      for( i=0; i<len; i++ )
                        {
                         if( eepEntryCount >= MAX_EEP_SIZ )
                           {
                            return( BCPRG_EEPROM_SIZE_ERROR );
                           }
                         pEepBuf[eepEntryCount].adr = adr + i;
                         pEepBuf[eepEntryCount].val = pDat[i];
                         eepEntryCount++;
                        }
                      flags |= BCPRG_EEP_PROG_REQ;
                      break;

    case E_FUSE_AND_TAG:
                      if( adr  ||  len != 1 )
                        {
                         return( BCPRG_CHECK_DAT_FILE_ERROR );
                        }
                      extFuseAndMask = pDat[0];
                      flags |= BCPRG_FUSEX_PROG_REQ;
                      break;

    case E_FUSE_OR_TAG:
                      if( adr  ||  len != 1 )
                        {
                         return( BCPRG_CHECK_DAT_FILE_ERROR );
                        }
                      extFuseOrMask = pDat[0];
                      flags |= BCPRG_FUSEX_PROG_REQ;
                      break;

    case H_FUSE_AND_TAG:
                      if( adr  ||  len != 1 )
                        {
                         return( BCPRG_CHECK_DAT_FILE_ERROR );
                        }
                      highFuseAndMask = pDat[0];
                      flags |= BCPRG_FUSEH_PROG_REQ;
                      break;

    case H_FUSE_OR_TAG:
                      if( adr  ||  len != 1 )
                        {
                         return( BCPRG_CHECK_DAT_FILE_ERROR );
                        }
                      highFuseOrMask = pDat[0];
                      flags |= BCPRG_FUSEH_PROG_REQ;
                      break;

    case L_FUSE_AND_TAG:
                      if( adr  ||  len != 1 )
                        {
                         return( BCPRG_CHECK_DAT_FILE_ERROR );
                        }
                      lowFuseAndMask = pDat[0];
                      flags |= BCPRG_FUSEL_PROG_REQ;
                      break;

    case L_FUSE_OR_TAG:
                      if( adr  ||  len != 1 )
                        {
                         return( BCPRG_CHECK_DAT_FILE_ERROR );
                        }
                      lowFuseOrMask = pDat[0];
                      flags |= BCPRG_FUSEL_PROG_REQ;
                      break;

    case LOCKS_TAG:
                      if( adr  ||  len != 1 )
                        {
                         return( BCPRG_CHECK_DAT_FILE_ERROR );
                        }
                      lockBits = pDat[0];
                      flags |= BCPRG_LOCKS_PROG_REQ;
                      break;
   }
 return( BCPRG_PASSED );
}


                                                                      //MOD021v
static int iCheckFF16( UINT32 idx )
{
  UINT32 i;

  for( i = 0; i < 16; i = i + 1 )
    {
      if( pFlsBuf[idx+i] != 0xFF )
        {
          return( 0 );
        }
    }
  return( 1 );
}
                                                                      //MOD021^

                                                                      
static INT32 ReadDatFile( void )
{
 P_HEX_REC pRec;
 unsigned char pBuf[MAX_HEX_REC_SIZ];
 unsigned char pLinBuf[BCPRG_MAX_LIN_SIZ];
 UINT32 i;
 UINT32 address;
 UINT32 addrExt = 0;                                                 //MOD016
 UINT32 len;
 UINT32 curTag;

 BcprgShowProgress( BCPRG_OPEN_DAT_FILE_BEFORE );

 if( (pDatFilStream = FOPEN( pDatFilNam, _T("rt") )) == NULL )
   {
    return( BCPRG_OPEN_DAT_FILE_ERROR );
   }

 BcprgShowProgress( BCPRG_OPEN_DAT_FILE_AFTER );

 BcprgShowProgress( BCPRG_MALLOC_BEFORE );

 if( !(pFlsBuf = malloc( MAX_FLS_SIZ )) ||
     !(pEepBuf = malloc( MAX_EEP_SIZ * sizeof( EEP_ENTRY ) )) )
   {
    return( BCPRG_MALLOC_ERROR );
   }

 BcprgShowProgress( BCPRG_MALLOC_AFTER );

 memset( pFlsBuf, 0xFF, MAX_FLS_SIZ );

 curTag = NO_TAG;
 pRec = (P_HEX_REC)(&pLinBuf);

 for( ;; )
   {
    BcprgShowProgress( BCPRG_READ_DAT_FILE_BEFORE );

    if( fgets( (char *)pLinBuf, BCPRG_MAX_LIN_SIZ, pDatFilStream ) == NULL )
      {
       if( ferror( pDatFilStream ) )
         {
          return( BCPRG_READ_DAT_FILE_ERROR );
         }
       else
         {
          BcprgShowProgress( BCPRG_READ_DAT_FILE_COMPLETE );

          BCPRG_VERBOSE_OPEN
          BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );

          BCPRG_VERBOSE_PRINTF( "  Feature number: %c\n", cgbcFeat, NULL );
          BCPRG_VERBOSE_PRINTF( "Major rev number: %c\n", cgbcRmaj, NULL );
          BCPRG_VERBOSE_PRINTF( "Minor rev number: %c\n", cgbcRmin, NULL );
          BCPRG_VERBOSE_PRINTF( "Compatibility ID: %d\n", cgbcCid,  NULL );

          BCPRG_VERBOSE_PRINTF( flsTag, NULL, NULL );
          for( i=0; i<MAX_FLS_SIZ; i++ )
            {
                                                                      //MOD021v
             if( !(i % 16) )
               {
                while( (iCheckFF16( i ) != 0) && (i < MAX_FLS_SIZ) )
                  {
                   i = i + 16;
                  }
               }
             if( i < MAX_FLS_SIZ )
               {
                                                                      //MOD021^
                if( !(i % 16) )
                  {
                   BCPRG_VERBOSE_PRINTF( "\n%04X: ", i, NULL );
                  }
                BCPRG_VERBOSE_PRINTF( "%02X ", pFlsBuf[i], NULL );
               }                                                      //MOD021
            }
          BCPRG_VERBOSE_PRINTF( "\n", NULL, NULL );
          BCPRG_VERBOSE_PRINTF( eepTag, NULL, NULL );
          BCPRG_VERBOSE_PRINTF( "\n", NULL, NULL );
          for( i=0; i<eepEntryCount; i++ )
            {
             BCPRG_VERBOSE_PRINTF( "%04X: %02X\n", pEepBuf[i].adr, pEepBuf[i].val );
            }
          BCPRG_VERBOSE_PRINTF( lFuseAndTag, NULL, NULL );
          BCPRG_VERBOSE_PRINTF( ":      %02X\n", lowFuseAndMask & 0x00FF, NULL );
          BCPRG_VERBOSE_PRINTF( lFuseOrTag, NULL, NULL );
          BCPRG_VERBOSE_PRINTF( ":       %02X\n", lowFuseOrMask & 0x00FF, NULL );
          BCPRG_VERBOSE_PRINTF( hFuseAndTag, NULL, NULL );
          BCPRG_VERBOSE_PRINTF( ":     %02X\n", highFuseAndMask & 0x00FF, NULL );
          BCPRG_VERBOSE_PRINTF( hFuseOrTag, NULL, NULL );
          BCPRG_VERBOSE_PRINTF( ":      %02X\n", highFuseOrMask & 0x00FF, NULL);
          BCPRG_VERBOSE_PRINTF( eFuseAndTag, NULL, NULL );
          BCPRG_VERBOSE_PRINTF( ": %02X\n", extFuseAndMask & 0x00FF, NULL );
          BCPRG_VERBOSE_PRINTF( eFuseOrTag, NULL, NULL );
          BCPRG_VERBOSE_PRINTF( ":  %02X\n", extFuseOrMask & 0x00FF, NULL);
          BCPRG_VERBOSE_PRINTF( locksTag, NULL, NULL );
          BCPRG_VERBOSE_PRINTF( ":                   %02X\n", lockBits & 0x00FF, NULL );

          BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
          BCPRG_VERBOSE_CLOSE

          return( BCPRG_PASSED );
         }
      }

    BcprgShowProgress( BCPRG_READ_DAT_FILE_AFTER );

    BcprgShowProgress( BCPRG_CHECK_DAT_FILE_BEFORE );

    switch( pLinBuf[0] )
      {
       case ';':
                   curTag = CheckTag( (char *)&pLinBuf[1] );
                   if( curTag == REVISION_TAG )
                     {
                      cgbcFeat        = pLinBuf[6];
                      cgbcRmaj        = pLinBuf[7];
                      cgbcRmin        = pLinBuf[8];
                      if( !memcmp( &pLinBuf[9], ".DAT CID ", 9 ) )
                        {
                         cgbcCid = atoi( (char *) &pLinBuf[18] );
                        }
                     }
                   break;

       case ':':
                   if( !curTag  ||  HexRecordChecksum( pRec ) )
                     {
                      return( BCPRG_CHECK_DAT_FILE_ERROR );
                     }

                   len = toint( pRec->lenHi ) * 16;
                   len += toint( pRec->lenLo );
                   if( len > MAX_HEX_REC_SIZ )
                     {
                      return( BCPRG_CHECK_DAT_FILE_ERROR );
                     }

                   address = toint( pRec->adrHi ) << 12;
                   address |= toint( pRec->adrMh ) << 8;
                   address |= toint( pRec->adrMl ) << 4;
                   address |= toint( pRec->adrLo );
                                                                    //MOD016v
                   if( curTag == FLASH_TAG )
                     {
                      address += addrExt;
                     }
                                                                    //MOD016^
                   if( pRec->typHi != '0' )
                     {
                      return( BCPRG_CHECK_DAT_FILE_ERROR );
                     }

                   switch( pRec->typLo )
                     {
                      case '0':  /* Handle data record. */
                                  for( i=0; i<len; i++ )
                                    {
                                     pBuf[i] =  (unsigned char)toint( pRec->dat[i*2] ) << 4;
                                     pBuf[i] |= toint( pRec->dat[i*2+1] );
                                    }

                                  if( HandleHexData( (char *) pBuf, address, len, curTag ) )
                                    {
                                     return( BCPRG_CHECK_DAT_FILE_ERROR );
                                    }
                                  break;

                      case '1':  /* Ignore end of file record. */
                      case '3':  /* Ignore start segment record. */  //MOD016
                      case '5':  /* Ignore entry point record. */    //MOD010
                                  break;
                                                                     //MOD016v
                      case '2':  /* Handle extended segment address record. */
                                  addrExt  = toint( pRec->dat[0] ) << 12;
                                  addrExt |= toint( pRec->dat[1] ) <<  8;
                                  addrExt |= toint( pRec->dat[2] ) <<  4;
                                  addrExt |= toint( pRec->dat[3] ) <<  0;
                                  addrExt <<= 4;
                                  addrExt &= 0x00FFFFFF;             //MOD018
                                  break;
                                                                     //MOD016^
                                                                     //MOD010v
                      case '4':
                                                                     //MOD016v
                                 /* Handle extended linear address record. */ 
                                  addrExt  = toint( pRec->dat[0] ) << 12;
                                  addrExt |= toint( pRec->dat[1] ) <<  8;
                                  addrExt |= toint( pRec->dat[2] ) <<  4;
                                  addrExt |= toint( pRec->dat[3] ) <<  0;
                                  addrExt <<= 16;
                                  addrExt &= 0x00FFFFFF;             //MOD018
                                                                     //MOD016^
                                  break;
                                                                     //MOD010^
                      default:
                                  return( BCPRG_CHECK_DAT_FILE_ERROR );
                     }
                   break;

       default:
                   return( BCPRG_CHECK_DAT_FILE_ERROR );
      }

    BcprgShowProgress( BCPRG_CHECK_DAT_FILE_AFTER );
   }
}



static INT32 CgosInit( void )
{
 BcprgShowProgress( BCPRG_CGOS_INIT_BEFORE );

 if( !hCgos )
   {
    return( BCPRG_CGOS_INIT_ERROR );
   }

 BcprgShowProgress( BCPRG_CGOS_INIT_AFTER );
 return( BCPRG_PASSED );
}



static unsigned char SpiTransferByte( unsigned char val,
                                      UINT32 edgeTmg,
                                      UINT32 byteTmg )
{
 unsigned char retVal;

 if( CgosCgbcReadWrite( hCgos, val, &retVal, edgeTmg, byteTmg ) )
   {
    return( retVal );
   }
 return( 0xFF );
}



static UINT32 SpiTransfer32( UINT32 dat32 )
{
 INT32 i;
 UINT32 res32;

 for( res32=0, i=4; i; i-- )
   {
    res32 <<= 8;
    res32 |= SpiTransferByte( (unsigned char)(dat32 >> 24),
                              cgbcEdgeDelay, cgbcByteDelay );
    dat32 <<= 8;
   }
 return( res32 );
}



static UINT32 Xfer( UINT32 cmd )
{
 UINT32 res;

 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( _08lXspPat, cmd, NULL );
 BCPRG_VERBOSE_CLOSE

 res = SpiTransfer32( cmd );

 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( _08lXspPat, res, NULL );
 BCPRG_VERBOSE_CLOSE

 return( res );
}



static INT32 AvrEnterSerialProgrammingMode( void )
{
 UINT32 res;
 INT32 i;

 for( res=0, i=4; i && res!=AVR_SPM_PROG_ENABLE_OKAY; i-- )
   {
    if( SpiInit() )
      {
       return( BCPRG_PROG_ENABLE_ERROR );
      }
    Sleep( 1 );
    if( SpiReset() )
      {
       return( BCPRG_PROG_ENABLE_ERROR );
      }
    Sleep( 21 );
    res = Xfer( AVR_SPM_PROG_ENABLE ) & AVR_SPM_PROG_ENABLE_OKAY_MSK;
   }

 if( res != AVR_SPM_PROG_ENABLE_OKAY )
   {
#if !BCPRG_HW_EMULATION
    SpiInit();
    Sleep( 1000 );                                                   //MOD001
    SendPostEndSignal();                                             //MOD001
    return( BCPRG_PROG_ENABLE_ERROR );
#endif //!BCPRG_HW_EMULATION
   }

 flags |= BCPRG_PROG_ENABLED;
 return( BCPRG_PASSED );
}


                                                                     //MOD006v
/*----------------------------------------------------------------------------
 * Name: static INT32 FindI2cBus( void )
 * Desc: This function searches the board controller internal virtual
 *       I2C-bus.
 * Inp:  none
 * Outp: 1 - bus found
 *       0 - bus not found
 *----------------------------------------------------------------------------
 */

static INT32 FindI2cBus( void )
{
 UINT32 busCnt;
 UINT32 i;

 busCnt = CgosI2CCount( hCgos );

 for( i=0; i<busCnt; i++ )
   {
    if( CgosI2CType( hCgos, i ) == 0x00040000 )
      {
       bcI2cBus = i;
       return( 1 );
      }
   }

 return( 0 );
}
                                                                     //MOD006^

                                                                     //MOD025v
/*----------------------------------------------------------------------------
 * Name: static INT32 HandleBldrSwitches( void )
 * Desc: This function handles the command line switches /bldrenable and
 *       /bldrdisable.
 * Inp:  none
 * Outp: 1 - success
 *       0 - error
 *----------------------------------------------------------------------------
 */

static INT32 HandleBldrSwitches( void )
{
 unsigned char valOut;
 unsigned char valIn;

/* Nothing to do if none of the bootloader switches is specified. */
 if(    ((flags & BCPRG_BLDRENA_SWITCH) == 0)
     && ((flags & BCPRG_BLDRDIS_SWITCH) == 0) )
   {
    return( 1 );
   }

/* Reading cBC flags byte 1. */
 if( FindI2cBus() )
   {
    if( CgosI2CReadRegister( hCgos, bcI2cBus, 0xC0, 0x05, &valIn ) )
      {
      /* The CGBC_BLDR_ENABLE bit is set as desired. */
       if( (flags & BCPRG_BLDRENA_SWITCH) != 0 )
         {
          valOut = valIn | 0x08;   
         }
       else
         {
          valOut = valIn & ~0x08;   
         }
       if( CgosI2CWriteRegister( hCgos, bcI2cBus, 0xC0, 0x05, valOut ) )
         {
         /* The cBC flags byte 1 is read back to verifiy whether the desired
          * setting is supported. */
          if( CgosI2CReadRegister( hCgos, bcI2cBus, 0xC0, 0x05, &valIn ) )
            {
             if( (valIn & 0x08) == (valOut & 0x08) )
               {
               /* The handling of the bootloader switches was successful. */
                return( 1 );
               }
            }
         }
      }
   }
/* The handling of the bootloader switches failed. */
 return( 0 );
}
                                                                     //MOD025^


static INT32 EnableProgramming( void )
{
 unsigned char val;                                                  //MOD006

 BcprgShowProgress( BCPRG_PROG_ENABLE_BEFORE );
                                                                     //MOD025v
 if( HandleBldrSwitches() == 0 )
   {
    return( BCPRG_PROG_ENABLE_ERROR );
   }
                                                                     //MOD025^
                                                                     //MOD006v
 if( flags & BCPRG_BB_UPDATE )
   {
    if( FindI2cBus() )
      {
       if( CgosI2CReadRegister( hCgos, bcI2cBus, 0xC0, 0x05, &val ) )
         {
          CgosI2CWriteRegister( hCgos, bcI2cBus, 0xC0, 0x05, val | 0x80 );
         }
      }
   }
                                                                     //MOD006^
 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
 BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );
 BCPRG_VERBOSE_CLOSE

 if( AvrEnterSerialProgrammingMode() )
   {
    BCPRG_VERBOSE_OPEN
    BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
    BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
    BCPRG_VERBOSE_CLOSE

    return( BCPRG_PROG_ENABLE_ERROR );
   }

 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
 BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
 BCPRG_VERBOSE_CLOSE

 BcprgShowProgress( BCPRG_PROG_ENABLE_AFTER );
 return( BCPRG_PASSED );
}



static UINT32 AvrSpmReadSignature( void )
{
 UINT32 tmp, sig;

 tmp = Xfer( AVR_SPM_RD_SIGNATURE_BYTE | AVR_SPM_SIGNATURE_BYTE_2 );
 sig = (tmp & 0x000000FF) << 16;

 tmp = Xfer( AVR_SPM_RD_SIGNATURE_BYTE | AVR_SPM_SIGNATURE_BYTE_1 );
 sig |= (tmp & 0x000000FF) << 8;

 tmp = Xfer( AVR_SPM_RD_SIGNATURE_BYTE | AVR_SPM_SIGNATURE_BYTE_0 );
 sig |= tmp & 0x000000FF;

#if BCPRG_HW_EMULATION
 return( AVR_ATMEGA48_SIGNATURE );
#endif //BCPRG_HW_EMULATION
 return( sig );
}



static INT32 CheckDeviceType( void )
{
 UINT32 devId;

 BcprgShowProgress( BCPRG_CHECK_DEVICE_BEFORE );

 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
 BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );
 BCPRG_VERBOSE_CLOSE

 devId = AvrSpmReadSignature();

 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( _08lXspPat, devId, NULL );
 BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
 BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
 BCPRG_VERBOSE_CLOSE

 switch( devId )
   {
    case AVR_ATMEGA48_SIGNATURE:
                                  flsSiz = AVR_ATMEGA48_FLASH_SIZE;
                                  flsPageSiz = AVR_ATMEGA48_FLASH_PAGE_SIZE;
                                  eepSiz = AVR_ATMEGA48_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_ATMEGA48 );
                                                                     //MOD004v
                                  pEesave = &curFusesH;
                                  eesaveMsk = AVR_ATMEGAX8_FUSE_EESAVE;
                                  pSpien = &curFusesH;
                                  spienMsk = AVR_ATMEGAX8_FUSE_SPIEN;
                                  pRstdisbl = &curFusesH;
                                  rstdisblMsk = AVR_ATMEGAX8_FUSE_RSTDISBL;
                                  pCksel = &curFusesL;
                                  ckselMsk = AVR_ATMEGAX8_FUSE_CKSEL_MSK;
                                  intCalib = AVR_ATMEGAX8_FUSE_CKSEL_INT_CALIB;
                                  lbModeMsk = AVR_ATMEGAX8_EXT_PROG_LOCK_MSK;
                                  lbModeProgEnabled = AVR_ATMEGAX8_EXT_PROG_ENABLED;
                                  lbModeProgDisabled = AVR_ATMEGAX8_EXT_PROG_VERY_DISABLED;
                                  mfgStart = 0;
                                                                     //MOD004^
                                  break;
                                                                     //MOD009v
    case AVR_ATMEGA48PA_SIGNATURE:
                                  flsSiz = AVR_ATMEGA48_FLASH_SIZE;
                                  flsPageSiz = AVR_ATMEGA48_FLASH_PAGE_SIZE;
                                  eepSiz = AVR_ATMEGA48_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_ATMEGA48PA );
                                  pEesave = &curFusesH;
                                  eesaveMsk = AVR_ATMEGAX8_FUSE_EESAVE;
                                  pSpien = &curFusesH;
                                  spienMsk = AVR_ATMEGAX8_FUSE_SPIEN;
                                  pRstdisbl = &curFusesH;
                                  rstdisblMsk = AVR_ATMEGAX8_FUSE_RSTDISBL;
                                  pCksel = &curFusesL;
                                  ckselMsk = AVR_ATMEGAX8_FUSE_CKSEL_MSK;
                                  intCalib = AVR_ATMEGAX8_FUSE_CKSEL_INT_CALIB;
                                  lbModeMsk = AVR_ATMEGAX8_EXT_PROG_LOCK_MSK;
                                  lbModeProgEnabled = AVR_ATMEGAX8_EXT_PROG_ENABLED;
                                  lbModeProgDisabled = AVR_ATMEGAX8_EXT_PROG_VERY_DISABLED;
                                  mfgStart = 0;
                                  break;
                                                                     //MOD009^
    case AVR_ATMEGA88_SIGNATURE:
                                  flsSiz = AVR_ATMEGA88_FLASH_SIZE;
                                  flsPageSiz = AVR_ATMEGA88_FLASH_PAGE_SIZE;
                                  eepSiz = AVR_ATMEGA88_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_ATMEGA88 );
                                                                     //MOD004v
                                  pEesave = &curFusesH;
                                  eesaveMsk = AVR_ATMEGAX8_FUSE_EESAVE;
                                  pSpien = &curFusesH;
                                  spienMsk = AVR_ATMEGAX8_FUSE_SPIEN;
                                  pRstdisbl = &curFusesH;
                                  rstdisblMsk = AVR_ATMEGAX8_FUSE_RSTDISBL;
                                  pCksel = &curFusesL;
                                  ckselMsk = AVR_ATMEGAX8_FUSE_CKSEL_MSK;
                                  intCalib = AVR_ATMEGAX8_FUSE_CKSEL_INT_CALIB;
                                  lbModeMsk = AVR_ATMEGAX8_EXT_PROG_LOCK_MSK;
                                  lbModeProgEnabled = AVR_ATMEGAX8_EXT_PROG_ENABLED;
                                  lbModeProgDisabled = AVR_ATMEGAX8_EXT_PROG_VERY_DISABLED;
                                  mfgStart = 0;
                                                                     //MOD004^
                                  break;
                                                                     //MOD009v
    case AVR_ATMEGA88PA_SIGNATURE:
                                  flsSiz = AVR_ATMEGA88_FLASH_SIZE;
                                  flsPageSiz = AVR_ATMEGA88_FLASH_PAGE_SIZE;
                                  eepSiz = AVR_ATMEGA88_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_ATMEGA88PA );
                                  pEesave = &curFusesH;
                                  eesaveMsk = AVR_ATMEGAX8_FUSE_EESAVE;
                                  pSpien = &curFusesH;
                                  spienMsk = AVR_ATMEGAX8_FUSE_SPIEN;
                                  pRstdisbl = &curFusesH;
                                  rstdisblMsk = AVR_ATMEGAX8_FUSE_RSTDISBL;
                                  pCksel = &curFusesL;
                                  ckselMsk = AVR_ATMEGAX8_FUSE_CKSEL_MSK;
                                  intCalib = AVR_ATMEGAX8_FUSE_CKSEL_INT_CALIB;
                                  lbModeMsk = AVR_ATMEGAX8_EXT_PROG_LOCK_MSK;
                                  lbModeProgEnabled = AVR_ATMEGAX8_EXT_PROG_ENABLED;
                                  lbModeProgDisabled = AVR_ATMEGAX8_EXT_PROG_VERY_DISABLED;
                                  mfgStart = 0;
                                  break;
                                                                     //MOD009^
    case AVR_ATMEGA168_SIGNATURE:
                                  flsSiz = AVR_ATMEGA168_FLASH_SIZE;
                                  flsPageSiz = AVR_ATMEGA168_FLASH_PAGE_SIZE;
                                  eepSiz = AVR_ATMEGA168_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_ATMEGA168 );
                                                                     //MOD004v
                                  pEesave = &curFusesH;
                                  eesaveMsk = AVR_ATMEGAX8_FUSE_EESAVE;
                                  pSpien = &curFusesH;
                                  spienMsk = AVR_ATMEGAX8_FUSE_SPIEN;
                                  pRstdisbl = &curFusesH;
                                  rstdisblMsk = AVR_ATMEGAX8_FUSE_RSTDISBL;
                                  pCksel = &curFusesL;
                                  ckselMsk = AVR_ATMEGAX8_FUSE_CKSEL_MSK;
                                  intCalib = AVR_ATMEGAX8_FUSE_CKSEL_INT_CALIB;
                                  lbModeMsk = AVR_ATMEGAX8_EXT_PROG_LOCK_MSK;
                                  lbModeProgEnabled = AVR_ATMEGAX8_EXT_PROG_ENABLED;
                                  lbModeProgDisabled = AVR_ATMEGAX8_EXT_PROG_VERY_DISABLED;
                                  mfgStart = 0;
                                                                     //MOD004^
                                  break;
                                                                     //MOD009v
    case AVR_ATMEGA168PA_SIGNATURE:
                                  flsSiz = AVR_ATMEGA168_FLASH_SIZE;
                                  flsPageSiz = AVR_ATMEGA168_FLASH_PAGE_SIZE;
                                  eepSiz = AVR_ATMEGA168_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_ATMEGA168PA );
                                  pEesave = &curFusesH;
                                  eesaveMsk = AVR_ATMEGAX8_FUSE_EESAVE;
                                  pSpien = &curFusesH;
                                  spienMsk = AVR_ATMEGAX8_FUSE_SPIEN;
                                  pRstdisbl = &curFusesH;
                                  rstdisblMsk = AVR_ATMEGAX8_FUSE_RSTDISBL;
                                  pCksel = &curFusesL;
                                  ckselMsk = AVR_ATMEGAX8_FUSE_CKSEL_MSK;
                                  intCalib = AVR_ATMEGAX8_FUSE_CKSEL_INT_CALIB;
                                  lbModeMsk = AVR_ATMEGAX8_EXT_PROG_LOCK_MSK;
                                  lbModeProgEnabled = AVR_ATMEGAX8_EXT_PROG_ENABLED;
                                  lbModeProgDisabled = AVR_ATMEGAX8_EXT_PROG_VERY_DISABLED;
                                  mfgStart = 0;
                                  break;

    case AVR_ATMEGA328P_SIGNATURE:
                                  flsSiz = AVR_ATMEGA328P_FLASH_SIZE;
                                  flsPageSiz = AVR_ATMEGA328P_FLASH_PAGE_SIZE;
                                  eepSiz = AVR_ATMEGA328P_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_ATMEGA328P );
                                  pEesave = &curFusesH;
                                  eesaveMsk = AVR_ATMEGA328P_FUSE_EESAVE;
                                  pSpien = &curFusesH;
                                  spienMsk = AVR_ATMEGA328P_FUSE_SPIEN;
                                  pRstdisbl = &curFusesH;
                                  rstdisblMsk = AVR_ATMEGA328P_FUSE_RSTDISBL;
                                  pCksel = &curFusesL;
                                  ckselMsk = AVR_ATMEGAX8_FUSE_CKSEL_MSK;
                                  intCalib = AVR_ATMEGAX8_FUSE_CKSEL_INT_CALIB;
                                  lbModeMsk = AVR_ATMEGAX8_EXT_PROG_LOCK_MSK;
                                  lbModeProgEnabled = AVR_ATMEGAX8_EXT_PROG_ENABLED;
                                  lbModeProgDisabled = AVR_ATMEGAX8_EXT_PROG_VERY_DISABLED;
                                  mfgStart = 0;
                                  break;
                                                                     //MOD009^
                                                                     //MOD004v
    case AVR_ATMEGA165P_SIGNATURE:
                                  flsSiz = AVR_ATMEGA165P_FLASH_SIZE;
                                  flsPageSiz = AVR_ATMEGA165P_FLASH_PAGE_SIZE;
                                  eepSiz = AVR_ATMEGA165P_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_ATMEGA165P );
                                  pEesave = &curFusesH;
                                  eesaveMsk = AVR_ATMEGA165P_FUSE_EESAVE;
                                  pSpien = &curFusesH;
                                  spienMsk = AVR_ATMEGA165P_FUSE_SPIEN;
                                  pRstdisbl = &curFusesX;
                                  rstdisblMsk = AVR_ATMEGA165P_FUSE_RSTDISBL;
                                  pCksel = &curFusesL;
                                  ckselMsk = AVR_ATMEGA165P_FUSE_CKSEL_MSK;
                                  intCalib = AVR_ATMEGA165P_FUSE_CKSEL_INT_CALIB;
                                  lbModeMsk = AVR_ATMEGA165P_EXT_PROG_LOCK_MSK;
                                  lbModeProgEnabled = AVR_ATMEGA165P_EXT_PROG_ENABLED;
                                  lbModeProgDisabled = AVR_ATMEGA165P_EXT_PROG_VERY_DISABLED;
                                  mfgStart = 0x100;
                                  flags |= BCPRG_BB_SUPPORT;
                                  break;
                                                                     //MOD004^
                                                                     //MOD007v
    case AVR_ATMEGA325P_SIGNATURE:
                                  flsSiz = AVR_ATMEGA325P_FLASH_SIZE;
                                  flsPageSiz = AVR_ATMEGA325P_FLASH_PAGE_SIZE;
                                  eepSiz = AVR_ATMEGA325P_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_ATMEGA325P );
                                  pEesave = &curFusesH;
                                  eesaveMsk = AVR_ATMEGA325P_FUSE_EESAVE;
                                  pSpien = &curFusesH;
                                  spienMsk = AVR_ATMEGA325P_FUSE_SPIEN;
                                  pRstdisbl = &curFusesX;
                                  rstdisblMsk = AVR_ATMEGA325P_FUSE_RSTDISBL;
                                  pCksel = &curFusesL;
                                  ckselMsk = AVR_ATMEGA325P_FUSE_CKSEL_MSK;
                                  intCalib = AVR_ATMEGA325P_FUSE_CKSEL_INT_CALIB;
                                  lbModeMsk = AVR_ATMEGA325P_EXT_PROG_LOCK_MSK;
                                  lbModeProgEnabled = AVR_ATMEGA325P_EXT_PROG_ENABLED;
                                  lbModeProgDisabled = AVR_ATMEGA325P_EXT_PROG_VERY_DISABLED;
                                  mfgStart = 0x100;
                                  flags |= BCPRG_BB_SUPPORT;
                                  break;
                                                                     //MOD007^
                                                                     //MOD010v
    case STM32F100R8_SIGNATURE:
                                  flsSiz = STM32F100R8_FLASH_SIZE;
                                  flsPageSiz = STM32F100R8_FLASH_PAGE_SIZE;
                                  eepSiz = STM32F100R8_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_STM32F100R8 );
                                  pEesave = &curFusesH;
                                  eesaveMsk = STM32_FUSE_EESAVE;
                                  pSpien = NULL;
                                  pRstdisbl = NULL;
                                  pCksel = NULL;
                                  lbModeMsk = STM32_EXT_PROG_MSK;
                                  lbModeProgEnabled = STM32_EXT_PROG_ENABLED;
                                  lbModeProgDisabled = STM32_EXT_PROG_DISABLED;
                                  mfgStart = 0;
                                  flags |= BCPRG_BB_SUPPORT|BCPRG_BB_BOTTOM|
                                           BCPRG_EEEP_SUPPORT;
                                  break;
                                                                     //MOD010^
                                                                     //MOD015v
    case TM4E1231H6ZRB_SIGNATURE:
                                  flsSiz = TM4E1231H6ZRB_FLASH_SIZE;
                                  flsPageSiz = TM4E1231H6ZRB_FLASH_PAGE_SIZE;
                                  eepSiz = TM4E1231H6ZRB_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_TM4E1231H6ZRB );
                                  pEesave = &curFusesH;
                                  eesaveMsk = TIVA_FUSE_EESAVE;
                                  pSpien = NULL;
                                  pRstdisbl = NULL;
                                  pCksel = NULL;
                                  lbModeMsk = 0;
                                  lbModeProgEnabled = 0;
                                  lbModeProgDisabled = 0;
                                  mfgStart = 0;
                                  flags |= BCPRG_BB_SUPPORT|BCPRG_BB_BOTTOM|
                                           BCPRG_EEEP_SUPPORT;
                                  break;
                                                                     //MOD015^
                                                                     //MOD024v
    case MEC1701HSZ_SIGNATURE:
                                  flsSiz = MEC1701HSZ_FLASH_SIZE;
                                  flsPageSiz = MEC1701HSZ_FLASH_PAGE_SIZE;
                                  eepSiz = MEC1701HSZ_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_MEC1701HSZ );
                                  pEesave = &curFusesH;
                                  eesaveMsk = MC17_FUSE_EESAVE;
                                  pSpien = NULL;
                                  pRstdisbl = NULL;
                                  pCksel = NULL;
                                  lbModeMsk = 0;
                                  lbModeProgEnabled = 0;
                                  lbModeProgDisabled = 0;
                                  mfgStart = 0;
                                  flags |= BCPRG_AVR_DISABLE_CMD;
                                  flags |= BCPRG_SKIP_VERIFY_FF;
                                  flags |= BCPRG_SPM_EXT_SUPPORT;
                                  break;

    case MEC1701KSZ_SIGNATURE:
                                  flsSiz = MEC1701KSZ_FLASH_SIZE;
                                  flsPageSiz = MEC1701KSZ_FLASH_PAGE_SIZE;
                                  eepSiz = MEC1701KSZ_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_MEC1701KSZ );
                                  pEesave = &curFusesH;
                                  eesaveMsk = MC17_FUSE_EESAVE;
                                  pSpien = NULL;
                                  pRstdisbl = NULL;
                                  pCksel = NULL;
                                  lbModeMsk = 0;
                                  lbModeProgEnabled = 0;
                                  lbModeProgDisabled = 0;
                                  mfgStart = 0;
                                  flags |= BCPRG_AVR_DISABLE_CMD;
                                  flags |= BCPRG_SKIP_VERIFY_FF;
                                  flags |= BCPRG_SPM_EXT_SUPPORT;
                                  break;

    case MEC1701QSZ_SIGNATURE:
                                  flsSiz = MEC1701QSZ_FLASH_SIZE;
                                  flsPageSiz = MEC1701QSZ_FLASH_PAGE_SIZE;
                                  eepSiz = MEC1701QSZ_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_MEC1701QSZ );
                                  pEesave = &curFusesH;
                                  eesaveMsk = MC17_FUSE_EESAVE;
                                  pSpien = NULL;
                                  pRstdisbl = NULL;
                                  pCksel = NULL;
                                  lbModeMsk = 0;
                                  lbModeProgEnabled = 0;
                                  lbModeProgDisabled = 0;
                                  mfgStart = 0;
                                  flags |= BCPRG_AVR_DISABLE_CMD;
                                  flags |= BCPRG_SKIP_VERIFY_FF;
                                  flags |= BCPRG_SPM_EXT_SUPPORT;
                                  break;

    case MEC1703HSZ_SIGNATURE:
                                  flsSiz = MEC1703HSZ_FLASH_SIZE;
                                  flsPageSiz = MEC1703HSZ_FLASH_PAGE_SIZE;
                                  eepSiz = MEC1703HSZ_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_MEC1703HSZ );
                                  pEesave = &curFusesH;
                                  eesaveMsk = MC17_FUSE_EESAVE;
                                  pSpien = NULL;
                                  pRstdisbl = NULL;
                                  pCksel = NULL;
                                  lbModeMsk = 0;
                                  lbModeProgEnabled = 0;
                                  lbModeProgDisabled = 0;
                                  mfgStart = 0;
                                  flags |= BCPRG_AVR_DISABLE_CMD;
                                  flags |= BCPRG_SKIP_VERIFY_FF;
                                  flags |= BCPRG_SPM_EXT_SUPPORT;
                                  break;

    case MEC1703KSZ_SIGNATURE:
                                  flsSiz = MEC1703KSZ_FLASH_SIZE;
                                  flsPageSiz = MEC1703KSZ_FLASH_PAGE_SIZE;
                                  eepSiz = MEC1703KSZ_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_MEC1703KSZ );
                                  pEesave = &curFusesH;
                                  eesaveMsk = MC17_FUSE_EESAVE;
                                  pSpien = NULL;
                                  pRstdisbl = NULL;
                                  pCksel = NULL;
                                  lbModeMsk = 0;
                                  lbModeProgEnabled = 0;
                                  lbModeProgDisabled = 0;
                                  mfgStart = 0;
                                  flags |= BCPRG_AVR_DISABLE_CMD;
                                  flags |= BCPRG_SKIP_VERIFY_FF;
                                  flags |= BCPRG_SPM_EXT_SUPPORT;
                                  break;

    case MEC1703QSZ_SIGNATURE:
                                  flsSiz = MEC1703QSZ_FLASH_SIZE;
                                  flsPageSiz = MEC1703QSZ_FLASH_PAGE_SIZE;
                                  eepSiz = MEC1703QSZ_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_MEC1703QSZ );
                                  pEesave = &curFusesH;
                                  eesaveMsk = MC17_FUSE_EESAVE;
                                  pSpien = NULL;
                                  pRstdisbl = NULL;
                                  pCksel = NULL;
                                  lbModeMsk = 0;
                                  lbModeProgEnabled = 0;
                                  lbModeProgDisabled = 0;
                                  mfgStart = 0;
                                  flags |= BCPRG_AVR_DISABLE_CMD;
                                  flags |= BCPRG_SKIP_VERIFY_FF;
                                  flags |= BCPRG_SPM_EXT_SUPPORT;
                                  break;

    case MEC1704HSZ_SIGNATURE:
                                  flsSiz = MEC1704HSZ_FLASH_SIZE;
                                  flsPageSiz = MEC1704HSZ_FLASH_PAGE_SIZE;
                                  eepSiz = MEC1704HSZ_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_MEC1704HSZ );
                                  pEesave = &curFusesH;
                                  eesaveMsk = MC17_FUSE_EESAVE;
                                  pSpien = NULL;
                                  pRstdisbl = NULL;
                                  pCksel = NULL;
                                  lbModeMsk = 0;
                                  lbModeProgEnabled = 0;
                                  lbModeProgDisabled = 0;
                                  mfgStart = 0;
                                  flags |= BCPRG_AVR_DISABLE_CMD;
                                  flags |= BCPRG_SKIP_VERIFY_FF;
                                  flags |= BCPRG_SPM_EXT_SUPPORT;
                                  break;

    case MEC1704KSZ_SIGNATURE:
                                  flsSiz = MEC1704KSZ_FLASH_SIZE;
                                  flsPageSiz = MEC1704KSZ_FLASH_PAGE_SIZE;
                                  eepSiz = MEC1704KSZ_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_MEC1704KSZ );
                                  pEesave = &curFusesH;
                                  eesaveMsk = MC17_FUSE_EESAVE;
                                  pSpien = NULL;
                                  pRstdisbl = NULL;
                                  pCksel = NULL;
                                  lbModeMsk = 0;
                                  lbModeProgEnabled = 0;
                                  lbModeProgDisabled = 0;
                                  mfgStart = 0;
                                  flags |= BCPRG_AVR_DISABLE_CMD;
                                  flags |= BCPRG_SKIP_VERIFY_FF;
                                  flags |= BCPRG_SPM_EXT_SUPPORT;
                                  break;

    case MEC1704QSZ_SIGNATURE:
                                  flsSiz = MEC1704QSZ_FLASH_SIZE;
                                  flsPageSiz = MEC1704QSZ_FLASH_PAGE_SIZE;
                                  eepSiz = MEC1704QSZ_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_MEC1704QSZ );
                                  pEesave = &curFusesH;
                                  eesaveMsk = MC17_FUSE_EESAVE;
                                  pSpien = NULL;
                                  pRstdisbl = NULL;
                                  pCksel = NULL;
                                  lbModeMsk = 0;
                                  lbModeProgEnabled = 0;
                                  lbModeProgDisabled = 0;
                                  mfgStart = 0;
                                  flags |= BCPRG_AVR_DISABLE_CMD;
                                  flags |= BCPRG_SKIP_VERIFY_FF;
                                  flags |= BCPRG_SPM_EXT_SUPPORT;
                                  break;

    case MEC1705HSZ_SIGNATURE:
                                  flsSiz = MEC1705HSZ_FLASH_SIZE;
                                  flsPageSiz = MEC1705HSZ_FLASH_PAGE_SIZE;
                                  eepSiz = MEC1705HSZ_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_MEC1705HSZ );
                                  pEesave = &curFusesH;
                                  eesaveMsk = MC17_FUSE_EESAVE;
                                  pSpien = NULL;
                                  pRstdisbl = NULL;
                                  pCksel = NULL;
                                  lbModeMsk = 0;
                                  lbModeProgEnabled = 0;
                                  lbModeProgDisabled = 0;
                                  mfgStart = 0;
                                  flags |= BCPRG_AVR_DISABLE_CMD;
                                  flags |= BCPRG_SKIP_VERIFY_FF;
                                  flags |= BCPRG_SPM_EXT_SUPPORT;
                                  break;

    case MEC1705KSZ_SIGNATURE:
                                  flsSiz = MEC1705KSZ_FLASH_SIZE;
                                  flsPageSiz = MEC1705KSZ_FLASH_PAGE_SIZE;
                                  eepSiz = MEC1705KSZ_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_MEC1705KSZ );
                                  pEesave = &curFusesH;
                                  eesaveMsk = MC17_FUSE_EESAVE;
                                  pSpien = NULL;
                                  pRstdisbl = NULL;
                                  pCksel = NULL;
                                  lbModeMsk = 0;
                                  lbModeProgEnabled = 0;
                                  lbModeProgDisabled = 0;
                                  mfgStart = 0;
                                  flags |= BCPRG_AVR_DISABLE_CMD;
                                  flags |= BCPRG_SKIP_VERIFY_FF;
                                  flags |= BCPRG_SPM_EXT_SUPPORT;
                                  break;
                                                                     //MOD024^
                                                                     //MOD019v
    case MEC1705QSZ_SIGNATURE:
                                  flsSiz = MEC1705QSZ_FLASH_SIZE;
                                  flsPageSiz = MEC1705QSZ_FLASH_PAGE_SIZE;
                                  eepSiz = MEC1705QSZ_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_MEC1705QSZ );
                                  pEesave = &curFusesH;
                                  eesaveMsk = MC17_FUSE_EESAVE;
                                  pSpien = NULL;
                                  pRstdisbl = NULL;
                                  pCksel = NULL;
                                  lbModeMsk = 0;
                                  lbModeProgEnabled = 0;
                                  lbModeProgDisabled = 0;
                                  mfgStart = 0;
                                  flags |= BCPRG_AVR_DISABLE_CMD;
                                  flags |= BCPRG_SKIP_VERIFY_FF;     //MOD020
                                  flags |= BCPRG_SPM_EXT_SUPPORT;    //MOD021
                                  break;
                                                                     //MOD019^
                                                                     //MOD024v
    case MEC1706HSZ_SIGNATURE:
                                  flsSiz = MEC1706HSZ_FLASH_SIZE;
                                  flsPageSiz = MEC1706HSZ_FLASH_PAGE_SIZE;
                                  eepSiz = MEC1706HSZ_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_MEC1706HSZ );
                                  pEesave = &curFusesH;
                                  eesaveMsk = MC17_FUSE_EESAVE;
                                  pSpien = NULL;
                                  pRstdisbl = NULL;
                                  pCksel = NULL;
                                  lbModeMsk = 0;
                                  lbModeProgEnabled = 0;
                                  lbModeProgDisabled = 0;
                                  mfgStart = 0;
                                  flags |= BCPRG_AVR_DISABLE_CMD;
                                  flags |= BCPRG_SKIP_VERIFY_FF;
                                  flags |= BCPRG_SPM_EXT_SUPPORT;
                                  break;

    case MEC1706KSZ_SIGNATURE:
                                  flsSiz = MEC1706KSZ_FLASH_SIZE;
                                  flsPageSiz = MEC1706KSZ_FLASH_PAGE_SIZE;
                                  eepSiz = MEC1706KSZ_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_MEC1706KSZ );
                                  pEesave = &curFusesH;
                                  eesaveMsk = MC17_FUSE_EESAVE;
                                  pSpien = NULL;
                                  pRstdisbl = NULL;
                                  pCksel = NULL;
                                  lbModeMsk = 0;
                                  lbModeProgEnabled = 0;
                                  lbModeProgDisabled = 0;
                                  mfgStart = 0;
                                  flags |= BCPRG_AVR_DISABLE_CMD;
                                  flags |= BCPRG_SKIP_VERIFY_FF;
                                  flags |= BCPRG_SPM_EXT_SUPPORT;
                                  break;
                                                                     //MOD024^
                                                                     //MOD022v
    case MEC1706QSZ_SIGNATURE:
                                  flsSiz = MEC1706QSZ_FLASH_SIZE;
                                  flsPageSiz = MEC1706QSZ_FLASH_PAGE_SIZE;
                                  eepSiz = MEC1706QSZ_EEPROM_SIZE;
                                  BcprgShowProgress( BCPRG_FOUND_MEC1706QSZ );
                                  pEesave = &curFusesH;
                                  eesaveMsk = MC17_FUSE_EESAVE;
                                  pSpien = NULL;
                                  pRstdisbl = NULL;
                                  pCksel = NULL;
                                  lbModeMsk = 0;
                                  lbModeProgEnabled = 0;
                                  lbModeProgDisabled = 0;
                                  mfgStart = 0;
                                  flags |= BCPRG_AVR_DISABLE_CMD;
                                  flags |= BCPRG_SKIP_VERIFY_FF;
                                  flags |= BCPRG_SPM_EXT_SUPPORT;
                                  break;
                                                                     //MOD022^
    default:                                       
                                  return( BCPRG_UNKNOWN_DEVICE_ERROR );
   }

 return( BCPRG_PASSED );
}



static UINT32 WaitRdy( UINT32 msecs )
{
 UINT32 tmp;
 UINT32  i;                                                    //MOD005v

 if ( flags & BCPRG_BB_SUPPORT )
   {
    for( tmp=AVR_SPM_BSY, i=1999; tmp==AVR_SPM_BSY && i; i-- )
      {
       Sleep( 1 );
       BCPRG_VERBOSE_OPEN
       if( !(i%4) )
         {
          BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
         }
       BCPRG_VERBOSE_CLOSE
       tmp = Xfer( AVR_SPM_POLL_RDY_BUSY ) & AVR_SPM_BSY;
      }
   }
 else
   {
    Sleep( msecs );
    tmp = 0;
   }
                                                                     //MOD005^
 return( tmp );
}



static unsigned char AvrSpmReadLocks( void )
{
 return( (unsigned char)Xfer( AVR_SPM_RD_LOCK_BITS ) &
                              AVR_SPM_LOCK_BITS_MSK );
}



static INT32 AvrSpmWriteLocks( unsigned char locks, UINT32 msecs )
{
 UINT32 tmp;

 Xfer( AVR_SPM_WR_LOCK_BITS | locks );

 tmp = WaitRdy( msecs );

 if( tmp == AVR_SPM_BSY )                                            //MOD005
   {
    return( BCPRG_WRITE_LOCKS_ERROR );
   }
 return( BCPRG_PASSED );
}



static unsigned char AvrSpmReadFusesX( void )
{
 return( (unsigned char)Xfer( AVR_SPM_RD_FUSE_BITS_EX ) &
                              AVR_SPM_FUSE_BITS_EX_MSK );
}



static INT32 AvrSpmWriteFusesX( unsigned char fuses, UINT32 msecs )
{
 UINT32 tmp;

 Xfer( AVR_SPM_WR_FUSE_BITS_EX | fuses );

 tmp = WaitRdy( msecs );

 if( tmp == AVR_SPM_BSY )                                            //MOD005
   {
    return( BCPRG_WRITE_FUSEX_ERROR );
   }
 return( BCPRG_PASSED );
}



static unsigned char AvrSpmReadFusesH( void )
{
 return( (unsigned char)Xfer( AVR_SPM_RD_FUSE_BITS_HI ) &
                              AVR_SPM_FUSE_BITS_HI_MSK );
}



static INT32 AvrSpmWriteFusesH( unsigned char fuses, UINT32 msecs )
{
 UINT32 tmp;

 Xfer( AVR_SPM_WR_FUSE_BITS_HI | fuses );

 tmp = WaitRdy( msecs );

 if( tmp == AVR_SPM_BSY )                                            //MOD005
   {
    return( BCPRG_WRITE_FUSEH_ERROR );
   }
 return( BCPRG_PASSED );
}



static unsigned char AvrSpmReadFusesL( void )
{
 return( (unsigned char)Xfer( AVR_SPM_RD_FUSE_BITS_LO ) &
                              AVR_SPM_FUSE_BITS_LO_MSK );
}



static INT32 AvrSpmWriteFusesL( unsigned char fuses, UINT32 msecs )
{
 UINT32 tmp;

 Xfer( AVR_SPM_WR_FUSE_BITS_LO | fuses );

 tmp = WaitRdy( msecs );

 if( tmp == AVR_SPM_BSY )                                            //MOD005
   {
    return( BCPRG_WRITE_FUSEL_ERROR );
   }
 return( BCPRG_PASSED );
}



static INT32 SetEesaveFuse( void )
{
 INT32 tmp;
                                                                     //MOD017v
 if( pEesave == NULL )                /* Skip if EESAVE fuse not supported. */
   {
    return( BCPRG_PASSED );
   }
                                                                     //MOD017^
 BcprgShowProgress( BCPRG_SET_EESAVE_BEFORE );

 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
 BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );
 BCPRG_VERBOSE_CLOSE
                                                                     //MOD004v
 if( pEesave == &curFusesX )
   {
    tmp = AvrSpmWriteFusesX( (unsigned char)(curFusesX & ~eesaveMsk),
                             MAX_FUSE_PROG_TIME );
   }
 if( pEesave == &curFusesH )
   {
    tmp = AvrSpmWriteFusesH( (unsigned char)(curFusesH & ~eesaveMsk),
                             MAX_FUSE_PROG_TIME );
   }
 if( pEesave == &curFusesL )
   {
    tmp = AvrSpmWriteFusesL( (unsigned char)(curFusesL & ~eesaveMsk),
                             MAX_FUSE_PROG_TIME );
   }
                                                                     //MOD004^
 /*
  * NOTE! Changing the EESAVE fuse will take effect once it is programmed.
  * Therefore it is not necessary to leave and re-enter programming mode again
  * as for other fuses.
  */

 if( !tmp )
   {
                                                                     //MOD004v
    if( pEesave == &curFusesX )   curFusesX = AvrSpmReadFusesX();
    if( pEesave == &curFusesH )   curFusesH = AvrSpmReadFusesH();
    if( pEesave == &curFusesL )   curFusesL = AvrSpmReadFusesL();
                                                                     //MOD004^
#if BCPRG_HW_EMULATION
                                                                     //MOD004v
    if( pEesave == &curFusesX )   curFusesX &= ~eesaveMsk;
    if( pEesave == &curFusesH )   curFusesH &= ~eesaveMsk;
    if( pEesave == &curFusesL )   curFusesL &= ~eesaveMsk;
                                                                     //MOD004^
#endif //BCPRG_HW_EMULATION
   }

 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
 BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
 BCPRG_VERBOSE_CLOSE

 if( tmp  ||  (*pEesave & eesaveMsk) )                               //MOD004
   {
    return( BCPRG_SET_EESAVE_ERROR );
   }

 BcprgShowProgress( BCPRG_SET_EESAVE_AFTER );
 return( BCPRG_PASSED );
}



static INT32 CheckLocksAndFuses( void )
{
#ifdef _CONSOLE
 char input;
#endif

 BcprgShowProgress( BCPRG_CHECK_FUSES_BEFORE );

 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
 BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );
 BCPRG_VERBOSE_CLOSE

 curLocks = AvrSpmReadLocks();
 curFusesX = AvrSpmReadFusesX();
 curFusesH = AvrSpmReadFusesH();
 curFusesL = AvrSpmReadFusesL();

 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
 BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
 BCPRG_VERBOSE_CLOSE

 BcprgShowProgress( BCPRG_CHECK_FUSES_AFTER );

 if( !(flags & BCPRG_FLS_PROG_REQ)   &&
     ((curLocks & lockBits) == curLocks) )
   {
    return( BCPRG_PASSED );
   }

 flags |= BCPRG_CHIP_ERASE_REQ;
                                                                     //MOD017v
 if( pEesave == NULL )                /* Skip if EESAVE fuse not supported. */
   {
    return( BCPRG_PASSED );
   }
                                                                     //MOD017^
 if( (*pEesave & eesaveMsk)  &&  !(flags & BCPRG_EEP_ERASE_SWITCH) ) //MOD004
   {
#ifdef _CONSOLE
    BCPRG_CONSOLE_OUTPUT( eesaveWarnMes );
#endif // _CONSOLE

    if( (curLocks & lbModeMsk) != lbModeProgEnabled )                //MOD004
      {
#ifdef _CONSOLE
       if( !(flags & BCPRG_BATCH_SWITCH) )
         {
          PRINTF( askContinueMes );
                                                                   /*MOD014v*/
#ifdef DOSX
          outp( 0x80, (unsigned char)0xDD );
#endif /*DOSX*/
                                                                   /*MOD014^*/
          do
            {
             input = getch();
             input = tolower( input );
             if( input == 'n' )
               {
                putchar( input );
                return( BCPRG_ABORT );
               }
            } while( input != 'y' );
          putchar( input );
          PRINTF( nlNl );
         }
#else // _CONSOLE
       return( BCPRG_BAD_FUSES_ERROR );
#endif // _CONSOLE
      }
    else
      {
       if( !(flags & BCPRG_BATCH_SWITCH) )
         {
#ifdef _CONSOLE
          PRINTF( askEepSaveMes );
                                                                   /*MOD014v*/
#ifdef DOSX
          outp( 0x80, (unsigned char)0x55 );
#endif /*DOSX*/
                                                                   /*MOD014^*/
          do
            {
             input = getch();
             input = tolower( input );
             if( input == 'n'  ||  input == 'y' )
               {
                putchar( input );
                PRINTF( nlNl );
                if( input == 'y' )
                  {
                   if( SetEesaveFuse() != BCPRG_PASSED )
                     {
                      return( BCPRG_SET_EESAVE_ERROR );
                     }
                  }
               }
            } while( input != 'y'  &&  input != 'n' );
#else // _CONSOLE
          if( SetEesaveFuse() != BCPRG_PASSED )
            {
             return( BCPRG_SET_EESAVE_ERROR );
            }
#endif // _CONSOLE
         }
      }
   }
 return( BCPRG_PASSED );
}



static INT32 AvrSpmEraseChip( UINT32 msecs )
{
 UINT32 tmp;

 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
 BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );
 BCPRG_VERBOSE_CLOSE
                                                                   /*MOD014v*/
#ifdef DOSX
 outp( 0x80, (unsigned char)0xEE );
#endif /*DOSX*/
                                                                   /*MOD014^*/
 Xfer( AVR_SPM_CHIP_ERASE );

 tmp = WaitRdy( msecs );

 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
 BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
 BCPRG_VERBOSE_CLOSE

 if( tmp == AVR_SPM_BSY )                                            //MOD005
   {
    return( BCPRG_ERASE_ERROR );
   }
 return( BCPRG_PASSED );
}



static INT32 EraseChip( void )
{
 if( flags & BCPRG_CHIP_ERASE_REQ )
   {
    if( (pEesave != NULL) && (*pEesave & eesaveMsk) )         //MOD004 MOD017
      {
       BcprgShowProgress( BCPRG_FLS_EEP_ERASE_BEFORE );
       flags |= BCPRG_EEP_ERASED;
      }
    else
      {
       BcprgShowProgress( BCPRG_FLS_ERASE_BEFORE );
      }
    if( AvrSpmEraseChip( MAX_ERASE_TIME ) )
      {
       return( BCPRG_ERASE_ERROR );
      }
    BcprgShowProgress( BCPRG_ERASE_AFTER );
   }
 return( BCPRG_PASSED );
}



static INT32 ClearEesaveFuse( void )
{
 INT32 tmp;
                                                                     //MOD017v
 if( pEesave == NULL )                /* Skip if EESAVE fuse not supported. */
   {
    return( BCPRG_PASSED );
   }
                                                                     //MOD017^
 BcprgShowProgress( BCPRG_CLEAR_EESAVE_BEFORE );

 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
 BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );
 BCPRG_VERBOSE_CLOSE
                                                                     //MOD004v
 if( pEesave == &curFusesX )
   {
    tmp = AvrSpmWriteFusesX( (unsigned char)(curFusesX | eesaveMsk),
                             MAX_FUSE_PROG_TIME );
   }
 if( pEesave == &curFusesH )
   {
    tmp = AvrSpmWriteFusesH( (unsigned char)(curFusesH | eesaveMsk),
                             MAX_FUSE_PROG_TIME );
   }
 if( pEesave == &curFusesL )
   {
    tmp = AvrSpmWriteFusesL( (unsigned char)(curFusesL | eesaveMsk),
                             MAX_FUSE_PROG_TIME );
   }
                                                                     //MOD004^
 /*
  * NOTE! Changing the EESAVE fuse will take effect once it is programmed.
  * Therefore it is not necessary to leave and re-enter programming mode again
  * as for other fuses.
  */

 if( !tmp )
   {
                                                                     //MOD004v
    if( pEesave == &curFusesX )   curFusesX = AvrSpmReadFusesX();
    if( pEesave == &curFusesH )   curFusesH = AvrSpmReadFusesH();
    if( pEesave == &curFusesL )   curFusesL = AvrSpmReadFusesL();
                                                                     //MOD004^
#if BCPRG_HW_EMULATION
                                                                     //MOD004v
    if( pEesave == &curFusesX )   curFusesX |= eesaveMsk;
    if( pEesave == &curFusesH )   curFusesH |= eesaveMsk;
    if( pEesave == &curFusesL )   curFusesL |= eesaveMsk;
                                                                     //MOD004^
#endif //BCPRG_HW_EMULATION
   }

 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
 BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
 BCPRG_VERBOSE_CLOSE

 if( tmp  ||  !(*pEesave & eesaveMsk) )                              //MOD004
   {
    return( BCPRG_CLEAR_EESAVE_ERROR );
   }
 BcprgShowProgress( BCPRG_CLEAR_EESAVE_AFTER );
 return( BCPRG_PASSED );
}



static INT32 EraseEep( void )
{
                                                                     //MOD017v
 if( pEesave == NULL )                /* Skip if EESAVE fuse not supported. */
   {
    return( BCPRG_PASSED );
   }
                                                                     //MOD017^
 if( !(*pEesave & eesaveMsk)  &&  (flags & BCPRG_EEP_ERASE_SWITCH) ) //MOD004
   {
    if( ClearEesaveFuse() == BCPRG_PASSED )
      {
       return( EraseChip() );
      }
    return( BCPRG_ERASE_ERROR );
   }
 return( BCPRG_PASSED );
}



static unsigned char AvrSpmReadEepByte( UINT32 adr )
{
                                                                   /*MOD014v*/
#ifdef DOSX
 outp( 0x80, (unsigned char)(adr & 0x00FF) );
#endif /*DOSX*/
                                                                   /*MOD014^*/
 return( (unsigned char)Xfer( AVR_SPM_RD_EEPROM |
                              ((UINT32)adr << 8) ) );
}



static INT32 VerifyEepClean( void )
{
 unsigned char in;
 UINT32  i;
 UINT32  cnt;

 if( (flags & BCPRG_EEP_ERASED)  &&  !(flags & BCPRG_SKIP_VERIFY)
                                 &&  !(flags & BCPRG_SKIP_VERIFY_FF) )
   {
    BcprgShowProgress( BCPRG_VERIFY_EEP_CLEAN_BEFORE );

    BCPRG_VERBOSE_OPEN
    BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
    BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );
    BCPRG_VERBOSE_CLOSE

    for( cnt=0, i=0; i<eepSiz; i++ )
      {
       if( cnt == 4 )
         {
          BCPRG_VERBOSE_OPEN
          BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
          BCPRG_VERBOSE_CLOSE
          cnt = 0;
         }
       in = AvrSpmReadEepByte( i );
#if BCPRG_HW_EMULATION
       in = 0xFF;
#endif //BCPRG_HW_EMULATION
       if( in != 0xFF )
         {
          BCPRG_VERBOSE_OPEN
          BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
          BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
          BCPRG_VERBOSE_CLOSE

          SPRINTF( infoMesBuf, verifyInfoPat8, i, 0xFF, in );
          return( BCPRG_VERIFY_EEP_CLEAN_ERROR );
         }
       cnt++;
      }

    BCPRG_VERBOSE_OPEN
    if( cnt )
      {
       BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
      }
    BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
    BCPRG_VERBOSE_CLOSE

    BcprgShowProgress( BCPRG_VERIFY_EEP_CLEAN_AFTER );
   }
 return( BCPRG_PASSED );
}



static INT32 AvrSpmProgramFlsPage( UINT32 adr, unsigned char *pBuf,
                                 UINT32 siz, UINT32 msecs )
{
 UINT32  i;
 UINT32 tmp;
                                                                   /*MOD014v*/
#ifdef DOSX
 outp( 0x80, (unsigned char)(adr >> 8) );
#endif /*DOSX*/
                                                                   /*MOD014^*/
 for( i=0; i<siz; i+=2 )
   {
    if( i && !(i%4) )
      {
       BCPRG_VERBOSE_OPEN
       BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
       BCPRG_VERBOSE_CLOSE
      }
    Xfer( AVR_SPM_LD_PROG_MEM_PAGE_LO | ((UINT32)(i/2)<<8)
                                      | pBuf[i] );
    Xfer( AVR_SPM_LD_PROG_MEM_PAGE_HI | ((UINT32)(i/2)<<8)
                                      | pBuf[i+1] );
   }
 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
 BCPRG_VERBOSE_CLOSE
                                                                   /*MOD015v*/
 if( ((UINT32)adr>>17) != extAddr )
    {
     extAddr = (UINT32)adr>>17;
     Xfer( AVR_SPM_LD_EXT_ADDR | (extAddr << 8) );
    }
 Xfer( AVR_SPM_WR_PROG_MEM_PAGE | ((((UINT32)adr>>1)&0xFFFF)<<8) );
                                                                   /*MOD015^*/
 tmp = WaitRdy( msecs );

 if( tmp == AVR_SPM_BSY )                                            //MOD005
   {
    return( BCPRG_PROGRAM_FLS_ERROR );
   }
 return( BCPRG_PASSED );
}


                                                                     //MOD021v
static uint32_t ui32Gen5PgSize;

static int iSkipFlsPage( uint32_t ui32PageIdx )
{
  uint32_t *pui32Blk;
  uint32_t   ui32Idx;

  if( (flags & BCPRG_SKIP_VERIFY_FF) == 0 )
    {
      return( 0 );
    }

  pui32Blk = (uint32_t *)(&pFlsBuf[ui32PageIdx]);
  for( ui32Idx = 0; ui32Idx < (ui32Gen5PgSize/4); ui32Idx = ui32Idx + 1 )
    {
      if( pui32Blk[ui32Idx] != 0xFFFFFFFF )
        {
          return( 0 );
        }
    }
  return( 1 );
}

cgosret_bool Gen5SetFlashPageAddr( uint32_t ui32FlashPageAddress )
{
  unsigned char wbuf[6];
  cgosret_bool  cgosRet;
  UINT32        sts;

  wbuf[0] = CGBC_CMD_AVR_SPM_EXT;
  wbuf[1] = CGBC_CMD_AVR_SPM_FLS_ADDR;
  wbuf[2] = (uint8_t)(ui32FlashPageAddress >>  0);
  wbuf[3] = (uint8_t)(ui32FlashPageAddress >>  8);
  wbuf[4] = (uint8_t)(ui32FlashPageAddress >> 16);
  wbuf[5] = (uint8_t)(ui32FlashPageAddress >> 24);
  cgosRet = CgosCgbcHandleCommand( hCgos, &wbuf[0], 6, NULL, 0, &sts );
  if( cgosRet != 0 )
    {
      if( (sts & CGBC_STAT_MSK) != CGBC_RDY_STAT )
        {
          cgosRet = 0;
        }
    }
  return( cgosRet );
}



uint8_t Gen5GetFlashCtlrSts( uint32_t *pui32FlsPageSize )
{
  unsigned char wbuf[2];
  unsigned char rbuf[5];
  UINT32        sts;

  wbuf[0] = CGBC_CMD_AVR_SPM_EXT;
  wbuf[1] = CGBC_CMD_AVR_SPM_FLS_STAT;
  if( CgosCgbcHandleCommand( hCgos, &wbuf[0], 2, &rbuf[0], 5, &sts ) != 0 )
    {
      if( ((sts & CGBC_STAT_MSK   ) == CGBC_RDY_STAT )  &&
          ((sts & CGBC_DAT_PENDING) != 0             )  &&
          ((sts & CGBC_DAT_CNT_MSK) == (5-1)         )     )
        {
          if( pui32FlsPageSize != NULL )
            {
              *pui32FlsPageSize = *((uint32_t *)(&(rbuf[1])));
            }
          return( rbuf[0] );
        }
    }
  return( CGBC_AVR_SPM_FLS_ERR );
}



cgosret_bool Gen5WriteFlashPage( uint8_t *pui8WrDat )
{
  unsigned char wbuf[34];
  cgosret_bool  cgosRet;
  UINT32        sts;
  uint32_t  ui32Idx;

  for( cgosRet = 1,
       ui32Idx = 0; (   (cgosRet != 0             )
                     && (ui32Idx <  ui32Gen5PgSize)); ui32Idx = ui32Idx + 32 )
    {
      wbuf[0] = CGBC_CMD_AVR_SPM_EXT;
      wbuf[1] = CGBC_CMD_AVR_SPM_FLS_WR32;
      memcpy( &wbuf[2], &pui8WrDat[ui32Idx], 32 );
      cgosRet = CgosCgbcHandleCommand( hCgos, &wbuf[0], 34, NULL, 0, &sts );
      if( cgosRet != 0 )
        {
          if( (sts & CGBC_STAT_MSK) != CGBC_RDY_STAT )
            {
              cgosRet = 0;
            }
        }
    }
  return( cgosRet );    
}



cgosret_bool Gen5ReadFlashPage( uint8_t *pui8RdDat )
{
  unsigned char wbuf[2];
  cgosret_bool  cgosRet;
  UINT32        sts;
  uint32_t  ui32Idx;

  for( cgosRet = 1,
       ui32Idx = 0; (   (cgosRet != 0             )
                     && (ui32Idx <  ui32Gen5PgSize)); ui32Idx = ui32Idx + 32 )
    {
      wbuf[0] = CGBC_CMD_AVR_SPM_EXT;
      wbuf[1] = CGBC_CMD_AVR_SPM_FLS_RD32;
      cgosRet = CgosCgbcHandleCommand( hCgos, &wbuf[0],       2,
                                              &pui8RdDat[ui32Idx], 32, &sts );
      if( cgosRet != 0 )
        {
          if( ((sts & CGBC_STAT_MSK   ) != CGBC_RDY_STAT )  ||
              ((sts & CGBC_DAT_PENDING) == 0             )  ||
              ((sts & CGBC_DAT_CNT_MSK) != (32-1)        )     )
            {
              cgosRet = 0;
            }
        }
    }
  return( cgosRet );    
}



static INT32 Gen5ProgramFls( void )
{
  UINT32 i, j;
  INT32  ret;
  int    iSkipped;

  BcprgShowProgress( BCPRG_PROGRAM_FLS_BEFORE );

  BCPRG_VERBOSE_OPEN
  BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
  BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );
  BCPRG_VERBOSE_CLOSE

  ret = BCPRG_PASSED;
  iSkipped = 1;
  for( i = 0; (   (i   <  codeSiz     )
               && (ret == BCPRG_PASSED)); i = i + ui32Gen5PgSize )
    {
      while( (iSkipFlsPage( i ) != 0) && (i < codeSiz) )
        {
          iSkipped = 1;
          i = i + ui32Gen5PgSize;
        }
      if( i < codeSiz )
        {
          if( iSkipped != 0 )
            {
              if( Gen5SetFlashPageAddr( i ) == 0 )
                {
                  ret = BCPRG_PROGRAM_FLS_ERROR;
                }
              iSkipped = 0;
            }
          if( ret == BCPRG_PASSED )
            {
              BCPRG_VERBOSE_OPEN
              for( j = 0; j < ui32Gen5PgSize; j++ )
                {
                  if( (j & 0x0F) == 0 )
                    {
                     BCPRG_VERBOSE_PRINTF( "%08X:", i+j, NULL );
                    }
                  if( (j & 0x0F) == 8 )
                    {
                     BCPRG_VERBOSE_PRINTF( "-%02X", pFlsBuf[i+j], NULL );
                    }
                  else
                    {
                     BCPRG_VERBOSE_PRINTF( " %02X", pFlsBuf[i+j], NULL );
                    }
                  if( (j & 0x0F) == 15 )
                    {
                     BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
                    }
                }
              BCPRG_VERBOSE_CLOSE
    
              if( Gen5WriteFlashPage( &pFlsBuf[i] ) == 0 )
                {
                 ret = BCPRG_PROGRAM_FLS_ERROR;
                }
            }
        }
    }

  while( Gen5GetFlashCtlrSts( NULL ) == CGBC_AVR_SPM_FLS_BSY );

  BCPRG_VERBOSE_OPEN
  BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
  BCPRG_VERBOSE_CLOSE

  if( ret == BCPRG_PASSED )
    {
      BcprgShowProgress( BCPRG_PROGRAM_FLS_AFTER );
    }
 return( ret );
}
                                                                     //MOD021^


static INT32 ProgramFls( void )
{
 UINT32 i, j;

 if( flags & BCPRG_FLS_PROG_REQ )
   {
    BcprgShowProgress( BCPRG_CHECK_FLS_SPACE_BEFORE );
    for( codeSiz=0, i=0; i<MAX_FLS_SIZ; i++ )
      {
       if( pFlsBuf[i] != 0xFF )
         {
          codeSiz = i;
         }
      }
    codeSiz++;
    if( codeSiz > flsSiz )
      {
       return( BCPRG_CHECK_FLS_SPACE_ERROR );
      }
    BcprgShowProgress( BCPRG_CHECK_FLS_SPACE_AFTER );
                                                                     //MOD021v
    /* When the design supports the extended AVR SPM commands then a faster
     * programming procedure can be used.  Note!  The Tiva cBC bootblock
     * firmware hangs as soon as it receives any other cBC command than the
     * standard AVR SPM commands.  Therefore the check whether the extended AVR
     * SPM commands are supported cannot be done by just executing one of them.
     * Instead the BCPRG_SPM_EXT_SUPPORT flag is used first to see whether the
     * running cBC can support them at all. */
    if( (flags & BCPRG_SPM_EXT_SUPPORT) != 0 )
      {
        if( Gen5GetFlashCtlrSts( &ui32Gen5PgSize ) != CGBC_AVR_SPM_FLS_ERR )
          {
            return( Gen5ProgramFls() );
          }
      }
                                                                     //MOD021^
    BcprgShowProgress( BCPRG_PROGRAM_FLS_BEFORE );

    BCPRG_VERBOSE_OPEN
    BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
    BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );
    BCPRG_VERBOSE_CLOSE

    for( i=0; i<codeSiz; i+=flsPageSiz )
      {
       for( j=0; j<flsPageSiz && pFlsBuf[i+j]==0xFF; j++ );
       if( j != flsPageSiz )
         {
          if( i )
            {
             BCPRG_VERBOSE_OPEN
             BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
             BCPRG_VERBOSE_CLOSE
            }

          if( AvrSpmProgramFlsPage( i, &pFlsBuf[i], flsPageSiz,
                                    MAX_FLS_PROG_TIME ) )
            {
             BCPRG_VERBOSE_OPEN
             BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
             BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
             BCPRG_VERBOSE_CLOSE

             return( BCPRG_PROGRAM_FLS_ERROR );
            }
         }
      }

    BCPRG_VERBOSE_OPEN
    BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
    BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
    BCPRG_VERBOSE_CLOSE

    BcprgShowProgress( BCPRG_PROGRAM_FLS_AFTER );
   }
 return( BCPRG_PASSED );
}



static INT32 AvrSpmWriteEepByte( UINT32 adr, unsigned char val,
                                                 UINT32 msecs )
{
 UINT32 tmp;
                                                                   /*MOD014v*/
#ifdef DOSX
 outp( 0x80, (unsigned char)(adr & 0x00FF) );
#endif /*DOSX*/
                                                                   /*MOD014^*/
 Xfer( AVR_SPM_WR_EEPROM | ((UINT32)adr<<8) | val );

 tmp = WaitRdy( msecs );

 if( tmp == AVR_SPM_BSY )                                            //MOD005
   {
    return( BCPRG_PROGRAM_EEP_ERROR );
   }
 return( BCPRG_PASSED );
}



static INT32 ProgramEep( void )
{
 UINT32 i;
 UINT32 reqSiz;

 if( flags & BCPRG_EEP_PROG_REQ )
   {
    BcprgShowProgress( BCPRG_CHECK_EEP_SPACE_BEFORE );
    for( reqSiz=0, i=0; i<eepEntryCount; i++ )
      {
       if( pEepBuf[i].adr > reqSiz )
         {
          reqSiz = pEepBuf[i].adr;
         }
      }
    reqSiz++;
    if( reqSiz > eepSiz )
      {
       return( BCPRG_CHECK_EEP_SPACE_ERROR );
      }
    BcprgShowProgress( BCPRG_CHECK_EEP_SPACE_AFTER );

    BcprgShowProgress( BCPRG_PROGRAM_EEP_BEFORE );

    BCPRG_VERBOSE_OPEN
    BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
    BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );
    BCPRG_VERBOSE_CLOSE

    for( i=0; i<eepEntryCount; i++ )
      {
       if( i )
         {
          BCPRG_VERBOSE_OPEN
          BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
          BCPRG_VERBOSE_CLOSE
         }

       if( (flags & BCPRG_MFG_WRITE_SWITCH           ) ||
                                                                     //MOD004v
           (pEepBuf[i].adr <   mfgStart              ) ||
           (pEepBuf[i].adr >= (mfgStart+MFG_EEP_SIZE))    )
                                                                     //MOD004^
         {
          if( AvrSpmWriteEepByte( pEepBuf[i].adr, pEepBuf[i].val,
                                  MAX_EEP_PROG_TIME ) )
            {
             BCPRG_VERBOSE_OPEN
             BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
             BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
             BCPRG_VERBOSE_CLOSE

             return( BCPRG_PROGRAM_EEP_ERROR );
            }
         }
      }

    BCPRG_VERBOSE_OPEN
    BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
    BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
    BCPRG_VERBOSE_CLOSE

    BcprgShowProgress( BCPRG_PROGRAM_EEP_AFTER );
   }
 return( BCPRG_PASSED );
}


                                                      // MOD003 MOD004 MOD011v
static INT32 FuseProtection( unsigned char *pFuseByte)
{
 INT32 ignored = 0;

/*
 * The whole function is skipped if fuse protection is disabled.
 */
 if( !(flags & BCPRG_NFP_SWITCH) )
   {

   /*
    * When the system has no BC bootblock support emulating the ISP mode,
    * then firmware updates are possible in real hardware ISP mode only. This
    * requires that the RESET# signal and the SPI ISP support are enabled.
    * The following code ensures that the related fuses are set as needed.
    */
    if( !(flags & BCPRG_BB_SUPPORT) )
      {
       if( (pFuseByte == pRstdisbl)  &&  !(*pRstdisbl & rstdisblMsk) )
         {
          *pRstdisbl |= rstdisblMsk;
          ignored = 1;
         }

       if( (pFuseByte == pSpien   )  &&   (*pSpien    & spienMsk   ) )
         {
          *pSpien &= ~spienMsk;
          ignored = 1;
         }
      }

   /*
    * The congatec Board Controller normally takes its clock from an internal
    * RC oscillator because there is no external crystal/oscillator. The
    * following code ensures that the internal RC oscillator is selected.
    */
    if( (pFuseByte == pCksel)  &&  ((*pCksel & ckselMsk) != intCalib) )
      {
       *pCksel &= ~ckselMsk;
       *pCksel |= intCalib;
       ignored = 1;
      }
   }

 return( ignored );
}
                                                      // MOD003 MOD004 MOD011^


static INT32 ProgramFuses( void )
{
 INT32 ignored;
 INT32 tmp;

 if( flags & (BCPRG_FUSEX_PROG_REQ |
              BCPRG_FUSEH_PROG_REQ |
              BCPRG_FUSEL_PROG_REQ) )
   {
    BcprgShowProgress( BCPRG_PROGRAM_FUSES_BEFORE );

    ignored = 0;
    tmp = BCPRG_PASSED;

    BCPRG_VERBOSE_OPEN
    BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
    BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );
    BCPRG_VERBOSE_CLOSE

    if( flags & BCPRG_FUSEX_PROG_REQ )
      {
       curFusesX = AvrSpmReadFusesX();
       extFuse = curFusesX;
       extFuse &= extFuseAndMask;
       extFuse |= extFuseOrMask;
       ignored = FuseProtection( &curFusesX );        // MOD003 MOD004 MOD011
       tmp = AvrSpmWriteFusesX( extFuse, MAX_FUSE_PROG_TIME );
       BCPRG_VERBOSE_OPEN
       BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
       BCPRG_VERBOSE_CLOSE
      }

    if( !tmp  &&  (flags & BCPRG_FUSEH_PROG_REQ) )
      {
       curFusesH = AvrSpmReadFusesH();
       highFuse = curFusesH;
       highFuse &= highFuseAndMask;
       highFuse |= highFuseOrMask;
       ignored = FuseProtection( &curFusesH );        // MOD003 MOD004 MOD011
       tmp = AvrSpmWriteFusesH( highFuse, MAX_FUSE_PROG_TIME );
       BCPRG_VERBOSE_OPEN
       BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
       BCPRG_VERBOSE_CLOSE
      }

    if( !tmp  &&  (flags & BCPRG_FUSEL_PROG_REQ) )
      {
       curFusesL = AvrSpmReadFusesL();
       lowFuse = curFusesL;
       lowFuse &= lowFuseAndMask;
       lowFuse |= lowFuseOrMask;
       ignored = FuseProtection( &curFusesL );        // MOD003 MOD004 MOD011
       tmp = AvrSpmWriteFusesL( lowFuse, MAX_FUSE_PROG_TIME );
       BCPRG_VERBOSE_OPEN
       BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
       BCPRG_VERBOSE_CLOSE
      }

    BCPRG_VERBOSE_OPEN
    BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
    BCPRG_VERBOSE_CLOSE

    if( tmp )
      {
       return( tmp );
      }

    BcprgShowProgress( BCPRG_PROGRAM_FUSES_AFTER );

    if( ignored )
      {
       BcprgShowProgress( BCPRG_FUSES_WARNING );
      }
   }
 return( BCPRG_PASSED );
}



static UINT32 AvrSpmReadFlsWord( UINT32 wordAdr )
{
 UINT32 tmp;
 UINT32  val;
                                                                   /*MOD014v*/
#ifdef DOSX
 outp( 0x80, (unsigned char)(wordAdr >> 7) );
#endif /*DOSX*/
                                                                   /*MOD014^*/
                                                                   /*MOD015v*/
 if( (wordAdr>>16) != extAddr )
    {
     extAddr = wordAdr>>16;
     Xfer( AVR_SPM_LD_EXT_ADDR | (extAddr << 8) );

     BCPRG_VERBOSE_OPEN
     BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
     BCPRG_VERBOSE_CLOSE
    }
 tmp = Xfer( AVR_SPM_RD_PROG_MEM_LO | ((wordAdr & 0xFFFF) << 8) );
                                                                   /*MOD015^*/
 val = (UINT32)tmp & 0x00FF;
 tmp = Xfer( AVR_SPM_RD_PROG_MEM_HI | ((wordAdr & 0xFFFF) << 8) );  /*MOD015*/
 val |= ((UINT32)tmp & 0x00FF) << 8;
#if BCPRG_HW_EMULATION
 return( (UINT32)(pFlsBuf[wordAdr*2]) |
        ((UINT32)(pFlsBuf[wordAdr*2+1]) << 8) );
#endif //BCPRG_HW_EMULATION
 return( val );
}


                                                              //MOD006 MOD012v
static void DetermineVerificationArea( UINT32 *pOffsetStart,
                                       UINT32 *pOffsetEnd )
{
 UINT32 bbSize;
 UINT32 appEnd;
                                                                     //MOD013v
 UINT32 in;

 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
 BCPRG_VERBOSE_CLOSE
                                                                     //MOD013^
/* Preset values to verify the complete flash area. */
 *pOffsetStart = 0;
 *pOffsetEnd   = flsSiz;

/*
 * The following code sets the start of the flash verification area above the
 * bootblock area if a bootblock at the bottom of the flash area is supported
 * and present and the bootblock area has not been updated during the flash
 * programming phase.
 */
 if( (flags & BCPRG_BB_SUPPORT)  &&
     (flags & BCPRG_BB_BOTTOM )  &&
    !(flags & BCPRG_BB_UPDATE )     )
   {
    bbSize = 128;
    do
      {
       if( (pFlsBuf[bbSize-6] == 'B'                                  ) &&
           (pFlsBuf[bbSize-5] == 'S'                                  ) &&
           (isdigit( pFlsBuf[bbSize-4] ) || (pFlsBuf[bbSize-4] == 'B')) &&
           (isdigit( pFlsBuf[bbSize-3] )                              ) &&
           (isdigit( pFlsBuf[bbSize-2] )                              )    )
         {
          *pOffsetStart = bbSize;
         }
       if( bbSize >= 1*1024 )    bbSize += 1*1024;
       else                      bbSize <<= 1;
      } while( (*pOffsetStart == 0)  &&  (bbSize < flsSiz) );
   }

/*
 * The following code sets the end of the flash verification area at the end
 * of the application area if emulated EEPROM is supported and there is no
 * bootblock above the application area.
 */
 if( flags & BCPRG_EEEP_SUPPORT )
   {
    if( !(flags & BCPRG_BB_SUPPORT)                            ||
        ((flags & BCPRG_BB_SUPPORT) && (flags & BCPRG_BB_BOTTOM)) )
      {
       appEnd = flsSiz;
       do
         {
         if( (pFlsBuf[appEnd-6] == 'A')    &&
             (pFlsBuf[appEnd-5] == 'S')    &&
             isdigit( pFlsBuf[appEnd-4] )  &&
             isdigit( pFlsBuf[appEnd-3] )  &&
             isdigit( pFlsBuf[appEnd-2] )     )
            {
             *pOffsetEnd = appEnd;
            }
          appEnd -= 1*1024;
         } while( (*pOffsetEnd == flsSiz)  &&  (appEnd > 0) );
                                                                     //MOD013v
       if( *pOffsetEnd == flsSiz )
         {
         /*
          * There was no application section ID found which marks the end of
          * the verification area. This may be the case when a DAT file was
          * specified which contains a bootblock only but no application
          * section. Nevertheless, the board controller may have some flash
          * space reserved for EEPROM emulation. Therefore, the following
          * code scans the board controller for an EEPROM emulation area.
          */

          BCPRG_VERBOSE_OPEN
          BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );
          BCPRG_VERBOSE_CLOSE

         /* Check possible upper EEPS ID at top of flash first. */
          in = AvrSpmReadFlsWord( (flsSiz-6)/2 );
          BCPRG_VERBOSE_OPEN
          BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
          BCPRG_VERBOSE_CLOSE

          if( in == 0x53FF )                    // EEPS ID at top of flash ?
            {
             in = ((AvrSpmReadFlsWord( (flsSiz-2)/2 ) << 16) |
                   (AvrSpmReadFlsWord( (flsSiz-4)/2 )      )  );
             BCPRG_VERBOSE_OPEN
             BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
             BCPRG_VERBOSE_CLOSE

             if( (in & 0xFFF80000) == 0x08000000 )
               {
                in &= 0x0007FFFF;
                if( (in < flsSiz) && (in > *pOffsetStart) )
                  {
                   *pOffsetEnd = in;            // Start of EEPS found.
                  }
               }
            }
          if( *pOffsetEnd == flsSiz )
            {
             /* Scan for lower EEPS ID. */
             appEnd = flsSiz - 1024;
             do
               {
                in = AvrSpmReadFlsWord( appEnd/2 );
                BCPRG_VERBOSE_OPEN
                BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
                BCPRG_VERBOSE_CLOSE

                if( in == 0x53FF )              // Lower EEPS ID ?
                  {
                   in = ((AvrSpmReadFlsWord( (appEnd+4)/2 ) << 16) |
                         (AvrSpmReadFlsWord( (flsSiz+2)/2 )      )  );
                   BCPRG_VERBOSE_OPEN
                   BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
                   BCPRG_VERBOSE_CLOSE

                   if( (in & 0xFFF80000) == 0x08000000 )
                     {
                      in &= 0x0007FFFF;
                      if( in == flsSiz-6 )
                        {
                         *pOffsetEnd = appEnd;  // Start of EEPS found.
                        }
                     }
                  }
                appEnd -= 1*1024;
               } while( (*pOffsetEnd == flsSiz      )  &&
                        (appEnd      > *pOffsetStart)     );
            }

          BCPRG_VERBOSE_OPEN
          BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
          BCPRG_VERBOSE_CLOSE
         }
                                                                     //MOD013^
      }
   }

/*
 * The following code sets the end of the flash verification area above the
 * application area if a bootblock at the top of the flash area is supported
 * and present and the bootblock area has not been updated during the flash
 * programming phase.
 */
 if( (flags & BCPRG_BB_SUPPORT)  &&
    !(flags & BCPRG_BB_BOTTOM )  &&
    !(flags & BCPRG_BB_UPDATE )     )
   {
    appEnd = flsSiz;
    bbSize = 0;
    do
      {
       if( (pFlsBuf[appEnd-6] == 'A')    &&
           (pFlsBuf[appEnd-5] == 'S')    &&
           isdigit( pFlsBuf[appEnd-4] )  &&
           isdigit( pFlsBuf[appEnd-3] )  &&
           isdigit( pFlsBuf[appEnd-2] )     )
         {
          *pOffsetEnd = appEnd;
         }
       if( !bbSize ) bbSize = 128;
       else          bbSize <<= 1;
       appEnd = flsSiz - bbSize;
      } while( (*pOffsetEnd == flsSiz)  &&  ( bbSize <= 1024 ) );
   }
 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( "start of verification at offset %X\n",
                       *pOffsetStart, NULL );
 BCPRG_VERBOSE_PRINTF( "  end of verification at offset %X\n",
                       *pOffsetEnd, NULL );
 BCPRG_VERBOSE_CLOSE
}
                                                              //MOD006 MOD012^

                                                                     //MOD021v
static INT32 Gen5VerifyFls( void )
{
  UINT32       i, j;
  INT32        ret;
  int         iSkipped;
  UINT32       verEnd;
  UINT32       verStart;
  uint8_t *pui8PgBuf;

  BcprgShowProgress( BCPRG_VERIFY_FLS_BEFORE );

  pui8PgBuf = malloc( ui32Gen5PgSize );
  if( pui8PgBuf == NULL )
    {
     return( BCPRG_MALLOC_ERROR );
    }

  DetermineVerificationArea( &verStart, &verEnd );

  ret = BCPRG_PASSED;
  iSkipped = 1;
  for( i = verStart; (   (i   <  verEnd      )
                      && (ret == BCPRG_PASSED)); i = i + ui32Gen5PgSize )
    {
      while( (iSkipFlsPage( i ) != 0) && (i < verEnd) )
        {
          iSkipped = 1;
          i = i + ui32Gen5PgSize;
        }
      if( i < verEnd )
        {
          if( iSkipped != 0 )
            {
              if( Gen5SetFlashPageAddr( i ) == 0 )
                {
                  ret = BCPRG_PROGRAM_FLS_ERROR;
                }
              iSkipped = 0;
            }
          if( ret == BCPRG_PASSED )
            {
              if( Gen5ReadFlashPage( pui8PgBuf ) == 0 )
                {
                  strcpy( infoMesBuf, flashReadErrMsg );
                  ret = BCPRG_VERIFY_FLS_ERROR;
                }
            }
          if( ret == BCPRG_PASSED )
            {
              if( memcmp( pui8PgBuf, &pFlsBuf[i], ui32Gen5PgSize ) != 0 )
                {
                  for( j = 0; j < ui32Gen5PgSize; j = j + 1 )
                    {
                      if( pui8PgBuf[j] != pFlsBuf[j+i] )
                        {
                          SPRINTF( infoMesBuf,
                                   verifyInfoPat8L,
                                   j+i, pFlsBuf[j+i], pui8PgBuf[j] );
                          ret = BCPRG_VERIFY_FLS_ERROR;
                        }
                      j++;
                    }
                }
            }
        }          
    }

  if( ret == BCPRG_PASSED )
    {
      BcprgShowProgress( BCPRG_VERIFY_FLS_AFTER );
    }
 return( ret );
}
                                                                     //MOD021^


static INT32 VerifyFls( void )
{
 UINT32 i;
 UINT32 in;
 UINT32 out;
 UINT32 cnt;
 UINT32 verEnd;                                               //MOD006 MOD012
 UINT32 verStart;                                                    //MOD012

 if( (flags & BCPRG_FLS_PROG_REQ)  &&  !(flags & BCPRG_SKIP_VERIFY) )
   {
                                                                     //MOD021v
    /* When the design supports the extended AVR SPM commands then a faster
     * verification procedure can be used.  Note!  The Tiva cBC bootblock
     * firmware hangs as soon as it receives any other cBC command than the
     * standard AVR SPM commands.  Therefore the check whether the extended AVR
     * SPM commands are supported cannot be done by just executing one of them.
     * Instead the BCPRG_SPM_EXT_SUPPORT flag is used first to see whether the
     * running cBC can support them at all. */
    if( (flags & BCPRG_SPM_EXT_SUPPORT) != 0 )
      {
       if( Gen5GetFlashCtlrSts( &ui32Gen5PgSize ) != CGBC_AVR_SPM_FLS_ERR )
         {
          return( Gen5VerifyFls() );
         }
      }
                                                                     //MOD021^
    BcprgShowProgress( BCPRG_VERIFY_FLS_BEFORE );
                                                                     //MOD006v
    DetermineVerificationArea( &verStart, &verEnd );                 //MOD012
                                                                     //MOD006^
    BCPRG_VERBOSE_OPEN
    BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
    BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );
    BCPRG_VERBOSE_CLOSE

    for( cnt=0, i=verStart; i<verEnd; i+=2 )                         //MOD012
      {
       if( cnt == 2 )
         {
          BCPRG_VERBOSE_OPEN
          BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
          BCPRG_VERBOSE_CLOSE
          cnt = 0;
         }
       out = (UINT32)(pFlsBuf[i]) | ((UINT32)(pFlsBuf[i+1]) << 8);
       if( !(flags & BCPRG_SKIP_VERIFY_FF)  ||  out!=0xFFFF )
         {
          in = AvrSpmReadFlsWord( i/2 );
          if( in != out )
            {
             BCPRG_VERBOSE_OPEN
             BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
             BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
             BCPRG_VERBOSE_CLOSE

             SPRINTF( infoMesBuf, verifyInfoPat16, i, out, in );
             return( BCPRG_VERIFY_FLS_ERROR );
            }
          cnt++;
         }
      }

    BCPRG_VERBOSE_OPEN
    if( cnt )
      {
       BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
      }
    BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
    BCPRG_VERBOSE_CLOSE

    BcprgShowProgress( BCPRG_VERIFY_FLS_AFTER );
   }
 return( BCPRG_PASSED );
}



static INT32 VerifyEep( void )
{
 unsigned char in;
 unsigned char out;
 UINT32  i;
 UINT32  cnt;

 if( (flags & BCPRG_EEP_PROG_REQ)  &&  !(flags & BCPRG_SKIP_VERIFY) )
   {
    BcprgShowProgress( BCPRG_VERIFY_EEP_BEFORE );

    BCPRG_VERBOSE_OPEN
    BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
    BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );
    BCPRG_VERBOSE_CLOSE

    for( cnt=0, i=0; i<eepEntryCount; i++ )
      {
       if( cnt == 4 )
         {
          BCPRG_VERBOSE_OPEN
          BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
          BCPRG_VERBOSE_CLOSE
          cnt = 0;
         }
       out = pEepBuf[i].val;
       if( !(flags & BCPRG_SKIP_VERIFY_FF)  ||
           !(flags & BCPRG_EEP_ERASED)      ||  out!=0xFF )
         {
          in = AvrSpmReadEepByte( pEepBuf[i].adr );
#if BCPRG_HW_EMULATION
          in = out;
#endif //BCPRG_HW_EMULATION
          if( in != out )
            {
             BCPRG_VERBOSE_OPEN
             BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
             BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
             BCPRG_VERBOSE_CLOSE

             SPRINTF( infoMesBuf, verifyInfoPat8,                    //MOD002
                                  pEepBuf[i].adr, out, in );         //MOD002
             return( BCPRG_VERIFY_EEP_ERROR );
            }
          cnt++;
         }
      }

    BCPRG_VERBOSE_OPEN
    if( cnt )
      {
       BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
      }
    BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
    BCPRG_VERBOSE_CLOSE

    BcprgShowProgress( BCPRG_VERIFY_EEP_AFTER );
   }
 return( BCPRG_PASSED );
}



static INT32 VerifyFuses( void )
{
 if( !(flags & BCPRG_SKIP_VERIFY)  &&
     (flags & (BCPRG_FUSEX_PROG_REQ |
               BCPRG_FUSEH_PROG_REQ |
               BCPRG_FUSEL_PROG_REQ  )) )
   {
    BcprgShowProgress( BCPRG_VERIFY_FUSES_BEFORE );

    BCPRG_VERBOSE_OPEN
    BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
    BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );
    BCPRG_VERBOSE_CLOSE

    if( flags & BCPRG_FUSEX_PROG_REQ )
      {
       curFusesX = AvrSpmReadFusesX();
#if BCPRG_HW_EMULATION
       curFusesX = extFuse;
#endif //BCPRG_HW_EMULATION

       BCPRG_VERBOSE_OPEN
       BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
       BCPRG_VERBOSE_CLOSE

       if( curFusesX != extFuse )
         {
          BCPRG_VERBOSE_OPEN
          BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
          BCPRG_VERBOSE_CLOSE
          SPRINTF( infoMesBuf, verifyInfoPatF, _T('X'), extFuse, curFusesX );
          return( BCPRG_VERIFY_FUSES_ERROR );
         }
      }

    if( flags & BCPRG_FUSEH_PROG_REQ )
      {
       curFusesH = AvrSpmReadFusesH();
#if BCPRG_HW_EMULATION
       curFusesH = highFuse;
#endif //BCPRG_HW_EMULATION

       BCPRG_VERBOSE_OPEN
       BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
       BCPRG_VERBOSE_CLOSE

       if( curFusesH != highFuse )
         {
          BCPRG_VERBOSE_OPEN
          BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
          BCPRG_VERBOSE_CLOSE
          SPRINTF( infoMesBuf, verifyInfoPatF, _T('H'), highFuse, curFusesH );
          return( BCPRG_VERIFY_FUSES_ERROR );
         }
      }

    if( flags & BCPRG_FUSEL_PROG_REQ )
      {
       curFusesL = AvrSpmReadFusesL();
#if BCPRG_HW_EMULATION
       curFusesL = lowFuse;
#endif //BCPRG_HW_EMULATION

       BCPRG_VERBOSE_OPEN
       BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
       BCPRG_VERBOSE_CLOSE

       if( curFusesL != lowFuse )
         {
          BCPRG_VERBOSE_OPEN
          BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
          BCPRG_VERBOSE_CLOSE
          SPRINTF( infoMesBuf, verifyInfoPatF, _T('L'), lowFuse, curFusesL );
          return( BCPRG_VERIFY_FUSES_ERROR );
         }
      }

    BCPRG_VERBOSE_OPEN
    BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
    BCPRG_VERBOSE_CLOSE

    BcprgShowProgress( BCPRG_VERIFY_FUSES_AFTER );
   }
 return( BCPRG_PASSED );
}



static INT32 ProgramLocks( void )
{
 INT32 tmp;

 if( flags & BCPRG_LOCKS_PROG_REQ )
   {
    BcprgShowProgress( BCPRG_PROGRAM_LOCKS_BEFORE );

    BCPRG_VERBOSE_OPEN
    BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
    BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );
    BCPRG_VERBOSE_CLOSE

    tmp = AvrSpmWriteLocks( lockBits, MAX_FUSE_PROG_TIME );

    BCPRG_VERBOSE_OPEN
    BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
    BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
    BCPRG_VERBOSE_CLOSE

    if( tmp )
      {
       return( BCPRG_WRITE_LOCKS_ERROR );
      }

    BcprgShowProgress( BCPRG_PROGRAM_LOCKS_AFTER );
   }
 return( BCPRG_PASSED );
}



static INT32 VerifyLocks( void )
{
 if( flags & BCPRG_LOCKS_PROG_REQ )
   {
    BcprgShowProgress( BCPRG_VERIFY_LOCKS_BEFORE );

    BCPRG_VERBOSE_OPEN
    BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
    BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );
    BCPRG_VERBOSE_CLOSE

    curLocks = AvrSpmReadLocks();
#if BCPRG_HW_EMULATION
    curLocks = lockBits;
#endif //BCPRG_HW_EMULATION

    BCPRG_VERBOSE_OPEN
    BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
    BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
    BCPRG_VERBOSE_CLOSE

    if( curLocks != lockBits )
      {
       SPRINTF( infoMesBuf, verifyInfoPatL, lockBits, curLocks );
       return( BCPRG_VERIFY_LOCKS_ERROR );
      }
    BcprgShowProgress( BCPRG_VERIFY_LOCKS_AFTER );
   }

 if( (curLocks & lbModeMsk) != lbModeProgDisabled )                  //MOD004
   {
    BcprgShowProgress( BCPRG_LOCKS_WARNING );
   }
 return( BCPRG_PASSED );
}



static INT32 CheckBcTiming( void )
{
 unsigned char wBuf[1];
 unsigned char rBuf[2];
 UINT32 sts;

 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );
 BCPRG_VERBOSE_CLOSE

 wBuf[0] = CGBC_CMD_SPI_TIMING;

 if( CgosCgbcHandleCommand( hCgos, &wBuf[0], 1, &rBuf[0], 2, &sts ) )
   {
    if( ((sts & CGBC_STAT_MSK   ) == CGBC_RDY_STAT )  &&
        ((sts & CGBC_DAT_PENDING)                  )  &&
        ((sts & CGBC_DAT_CNT_MSK) >= 1             )     )
      {
       cgbcEdgeDelay = (UINT32)(rBuf[0] >> 4);
      }
   }
 
 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( "Timing: %ld microseconds\n", cgbcEdgeDelay, NULL );
 BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
 BCPRG_VERBOSE_CLOSE

 return( BCPRG_PASSED );
}



static INT32 CheckCompatibility( void )
{
 unsigned char wBuf[1];
 unsigned char rBuf[4];
 UINT32 sts;
 INT32           curCid;

 BcprgShowProgress( BCPRG_CHECK_COMPATIBILITY_BEFORE );

 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( nl, NULL, NULL );
 BCPRG_VERBOSE_PRINTF( verboseStartMes, NULL, NULL );
 BCPRG_VERBOSE_CLOSE

 curCid = -1;

 wBuf[0] = CGBC_CMD_GET_FW_REV;
 if( CgosCgbcHandleCommand( hCgos, &wBuf[0], 1, &rBuf[0], 4, &sts ) )
   {
    if( ((sts & CGBC_STAT_MSK   ) == CGBC_RDY_STAT )  &&
        ((sts & CGBC_DAT_PENDING)                  )  &&
        ((sts & CGBC_DAT_CNT_MSK) >= 3             )     )
      {
       curCid = rBuf[3];
      }
   }

 BCPRG_VERBOSE_OPEN
 BCPRG_VERBOSE_PRINTF( "Old CID: %d   New CID: %d\n", curCid, cgbcCid );
 BCPRG_VERBOSE_PRINTF( verboseEndMes, NULL, NULL );
 BCPRG_VERBOSE_CLOSE

 if( (flags & BCPRG_CID_OVR_SWITCH) == 0 )                           //MOD023
   {                                                                 //MOD023
    if( (curCid != -1) && (curCid != cgbcCid) )
      {
       return( BCPRG_CHECK_COMPATIBILITY_ERROR );
      }
   }                                                                 //MOD023

 BcprgShowProgress( BCPRG_CHECK_COMPATIBILITY_AFTER );
 return( BCPRG_PASSED );
}



/*---------------------------------------------------------------------------
 * Name: INT32 BcprgcmnMain( _TCHAR* pFilename, UINT32 flg )
 * Desc: This is the main function of the command line version of the congatec
 *       board controller firmware update utility.
 * Inp:  _TCHAR* pFilename - pointer to the name of the firmware data file
 *       UINT32 flg - program flags and switches
 * Outp: BCPRG_PASSED if the firmware update was successful
 *       error code if the firmware update failed
 *---------------------------------------------------------------------------
 */

INT32 BcprgcmnMain( _TCHAR* pFilename, UINT32 flg )
{
 INT32 retCode;

 pDatFilStream   = NULL;
 pFlsBuf         = NULL;
 pEepBuf         = NULL;
 eepEntryCount   = 0;
 lowFuseAndMask  = 0xFF;
 lowFuseOrMask   = 0;
 lowFuse         = 0;
 highFuseAndMask = 0xFF;
 highFuseOrMask  = 0;
 highFuse        = 0;
 extFuseAndMask  = 0xFF;
 extFuseOrMask   = 0;
 extFuse         = 0;
 lockBits        = 0xFF;
 flsSiz          = 0;
 flsPageSiz      = 0;
 codeSiz         = 0;
 eepSiz          = 0;
 curLocks        = 0xFF;
 curFusesX       = 0xFF;
 curFusesH       = 0xFF;
 curFusesL       = 0xFF;
 cgbcFeat        = ' ';
 cgbcRmaj        = ' ';
 cgbcRmin        = ' ';
 cgbcCid         = -1;
 cgbcByteDelay   = 0;
 cgbcEdgeDelay   = 15;

 flags = flg;
 pDatFilNam = pFilename;

 
 if( (retCode = ReadDatFile())        == BCPRG_PASSED )
 if( (retCode = CgosInit())           == BCPRG_PASSED )
 if( (retCode = CheckBcTiming())      == BCPRG_PASSED )
 if( (retCode = CheckCompatibility()) == BCPRG_PASSED )
 if( (retCode = EnableProgramming())  == BCPRG_PASSED )
 if( (retCode = CheckDeviceType())    == BCPRG_PASSED )
 if( (retCode = CheckLocksAndFuses()) == BCPRG_PASSED )
 if( (retCode = EraseChip())          == BCPRG_PASSED )
 if( (retCode = EraseEep())           == BCPRG_PASSED )
 if( (retCode = VerifyEepClean())     == BCPRG_PASSED )
 if( (retCode = ProgramFls())         == BCPRG_PASSED )
 if( (retCode = ProgramEep())         == BCPRG_PASSED )
 if( (retCode = ProgramFuses())       == BCPRG_PASSED )
 if( (retCode = VerifyFls())          == BCPRG_PASSED )
 if( (retCode = VerifyEep())          == BCPRG_PASSED )
 if( (retCode = VerifyFuses())        == BCPRG_PASSED )
 if( (retCode = ProgramLocks())       == BCPRG_PASSED )
      retCode = VerifyLocks();

 Cleanup();
                                                                   /*MOD014v*/
#ifdef DOSX
 outp( 0x80, (unsigned char)retCode );
#endif /*DOSX*/
                                                                   /*MOD014^*/
 return( retCode );
}
