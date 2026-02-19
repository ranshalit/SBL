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
 * Contents: Board controller firmware update command line module.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * MOD015: Added support for new command line switches /bldrenable and
 *         /bldrdisable.
 *
 * MOD014: Added support for all MEC170x variants in the 144 pin WFBGA package.
 *
 * MOD013: added support for the /!cidoverride switch.
 *
 * MOD012: added support for MEC1706
 *
 * MOD011: Revised PrintMessage().
 *
 * MOD010: added support for MEC1705
 * 
 *    Rev 1.14   Sep 06 2016 16:23:42   congatec
 * Added BSD header.
 * 
 *    Rev 1.13   Jun 26 2013 10:44:34   ess
 * MOD009: added support for TM4E1231H6ZRB
 * 
 *    Rev 1.12   May 31 2012 15:31:26   gartner
 * Updated variable definitions to ease 64bit porting.
 * 
 *    Rev 1.11   Nov 11 2010 09:45:16   ess
 * MOD008: added support for STM32F100R8
 * 
 *    Rev 1.10   Nov 19 2009 10:53:50   ess
 * MOD007: added support for ATmega48PA/88PA/168PA/328P
 * 
 *    Rev 1.9   Feb 17 2009 15:08:44   ess
 * MOD006: bug fixed which caused an exception when an error occured.
 * 
 *    Rev 1.8   Dec 18 2008 16:02:16   ess
 * MOD005: added support for ATmega325P
 * 
 *    Rev 1.7   Nov 18 2008 14:13:24   ess
 * MOD004: added CGBC bootblock support
 * 
 *    Rev 1.6   Aug 26 2008 21:24:46   ess
 * MOD003: added support for ATmega165P
 * 
 *    Rev 1.5   Apr 15 2008 22:02:46   ess
 * MOD002: added support for the /!nofuseprotection command line switch.
 * 
 *    Rev 1.4   Oct 30 2006 14:30:18   gartner
 * Removed obsolete code.
 * 
 *    Rev 1.3   Mar 21 2006 15:06:04   gartner
 * Added support for xxx version.
 * 
 *    Rev 1.2   Jan 27 2006 12:45:00   gartner
 * Changed some description strings.
 * 
 *    Rev 1.1   Dec 21 2005 14:30:02   gartner
 * MOD001: Changed options description.
 * 
 *    Rev 1.0   Oct 04 2005 13:16:30   gartner
 * Initial revision.
 * 
 *    Rev 1.0   Sep 30 2005 16:31:18   gartner
 * Initial revision.
 *
 *---------------------------------------------------------------------------
 */



/*-------------------------
 * Revision identification
 *-------------------------
 */
#define CGBCPRG_MAJ_REV    1
#define CGBCPRG_MIN_REV    1

/*---------------
 * Include files
 *---------------
 */
#include "cgutlcmn.h"
#include "bcprg.h"

/*---------------------
 * External references
 *---------------------
 */
extern INT32 BcprgcmnMain( _TCHAR *pFilename, UINT32 flg );

/*----------
 * Messages
 *----------
 */
#ifdef CONGA
static _TCHAR helpMes[]      =  _T("This is the programming tool for the congatec board controller.\n")
#else
static _TCHAR helpMes[]      =  _T("This is the programming tool for the CPU-board controller.\n")
#endif
                                _T("\n")
                                _T("Syntax:   BCPROG <firmware file> [options]\n")
                                _T("\n")
                                _T("Options:  /B  batch mode / no user queries\n")
                                _T("          /F  fast mode / no verification at all\n")
                                _T("          /Q  quick mode / no verification of unused space\n")
                                _T("          /S  silent mode / no screen output except for error messages\n")
                                _T("          /V  verbose mode / enables additional diagnostic output\n")
                                                                     //MOD015v  
                                _T("\n")
                                _T("There may be two special options /bldrenable and /bldrdisable.  They are used\n")
                                _T("to force or inhibit the immediate loading and starting of the updated firmware\n")
                                _T("after it has been programmed.  These options should be employed with care as\n")
                                _T("they are intended for special use cases.  If not supported then they are either\n")
                                _T("ignored or the update process is aborted with a device programming error.\n")
                                _T("Note!  Generally in any case a cBC firmware update is not finished properly and\n")
                                _T("unexpected system behavior is possible until after a full G3 power cycle.\n");
                                                                     //MOD015^
static _TCHAR nl[]           =  _T("\n");
static _TCHAR nlNl[]         =  _T("\n\n");
static _TCHAR passedMes[]    =  _T("PASSED\n");
static _TCHAR errMes[]       =  _T("Error (%d): \a");
static _TCHAR filErrMes[]    =  _T("file I/O error on %s");
static _TCHAR prgErrMes[]    =  _T("device programming error");
static _TCHAR verErrMes[]    =  _T("verification failed (%s)");
static _TCHAR flsSizErrMes[] = _T("program code exceeds FLASH space");
static _TCHAR eepSizErrMes[] = _T("program data exceeds EEPROM space");

static _TCHAR *messages[] = {
 NULL,                                               // BCPRG_PASSED
 _T("program aborted"),                              // BCPRG_ABORT
 NULL,                                               // BCPRG_HELP_AFTER
 _T("syntax error / use -h for syntax description"), // BCPRG_SYNTAX_CHECK_ERROR,
 _T("Reading data file . . . . . . . "),             // BCPRG_OPEN_DAT_FILE_BEFORE
 filErrMes,                                          // BCPRG_OPEN_DAT_FILE_ERROR
 NULL,                                               // BCPRG_OPEN_DAT_FILE_AFTER
 NULL,                                               // BCPRG_MALLOC_BEFORE
 _T("memory allocation error"),                      // BCPRG_MALLOC_ERROR
 NULL,                                               // BCPRG_MALLOC_AFTER
 NULL,                                               // BCPRG_READ_DAT_FILE_BEFORE
 filErrMes,                                          // BCPRG_READ_DAT_FILE_ERROR
 NULL,                                               // BCPRG_READ_DAT_FILE_AFTER
 NULL,                                               // BCPRG_CHECK_DAT_FILE_BEFORE
 _T("invalid data in %s"),                           // BCPRG_CHECK_DAT_FILE_ERROR
 flsSizErrMes,                                       // BCPRG_FLASH_SIZE_ERROR
 eepSizErrMes,                                       // BCPRG_EEPROM_SIZE_ERROR
 NULL,                                               // BCPRG_CHECK_DAT_FILE_AFTER
 passedMes,                                          // BCPRG_READ_DAT_FILE_COMPLETE
#ifdef BANDR
 _T("Connecting system interface . . . "),           // BCPRG_CGOS_INIT_BEFORE
 _T("System interface error"),                         // BCPRG_CGOS_INIT_ERROR
#else
 _T("Connecting CGOS interface . . . "),             // BCPRG_CGOS_INIT_BEFORE
 _T("CGOS interface error"),                         // BCPRG_CGOS_INIT_ERROR
#endif
 passedMes,                                          // BCPRG_CGOS_INIT_AFTER
 _T("Checking compatibility. . . . . "),             // BCPRG_CHECK_COMPATIBILITY_BEFORE
 _T("Incompatible data file"),                       // BCPRG_CHECK_COMPATIBILITY_ERROR
 passedMes,                                          // BCPRG_CHECK_COMPATIBILITY_AFTER
 _T("Entering programming mode . . . "),             // BCPRG_PROG_ENABLE_BEFORE
 prgErrMes,                                          // BCPRG_PROG_ENABLE_ERROR
 passedMes,                                          // BCPRG_PROG_ENABLE_AFTER
 _T("Checking controller type. . . . "),             // BCPRG_CHECK_DEVICE_BEFORE
 _T("unknown type"),                                 // BCPRG_UNKNOWN_DEVICE_ERROR
 _T("ATmega48\n"),                                   // BCPRG_FOUND_ATMEGA48
 _T("ATmega48PA\n"),                                 // BCPRG_FOUND_ATMEGA48PA //MOD007
 _T("ATmega88\n"),                                   // BCPRG_FOUND_ATMEGA88
 _T("ATmega88PA\n"),                                 // BCPRG_FOUND_ATMEGA88PA //MOD007
 _T("ATmega168\n"),                                  // BCPRG_FOUND_ATMEGA168
 _T("ATmega168PA\n"),                                // BCPRG_FOUND_ATMEGA168PA //MOD007
 _T("ATmega328P\n"),                                 // BCPRG_FOUND_ATMEGA328P //MOD007
 _T("ATmega165P\n"),                                 // BCPRG_FOUND_ATMEGA165P //MOD003
 _T("ATmega325P\n"),                                 // BCPRG_FOUND_ATMEGA325P //MOD005
 _T("STM32F100R8\n"),                                // BCPRG_FOUND_STM32F100R8 //MOD008
 _T("TM4E1231H6ZRB\n"),                              // BCPRG_FOUND_TM4E1231H6ZRB //MOD009
 _T("MEC1701HSZ\n"),                                 // BCPRG_FOUND_MEC1701HSZ //MOD014
 _T("MEC1701KSZ\n"),                                 // BCPRG_FOUND_MEC1701KSZ //MOD014
 _T("MEC1701QSZ\n"),                                 // BCPRG_FOUND_MEC1701QSZ //MOD014
 _T("MEC1703HSZ\n"),                                 // BCPRG_FOUND_MEC1703HSZ //MOD014
 _T("MEC1703KSZ\n"),                                 // BCPRG_FOUND_MEC1703KSZ //MOD014
 _T("MEC1703QSZ\n"),                                 // BCPRG_FOUND_MEC1703QSZ //MOD014
 _T("MEC1704HSZ\n"),                                 // BCPRG_FOUND_MEC1704HSZ //MOD014
 _T("MEC1704KSZ\n"),                                 // BCPRG_FOUND_MEC1704KSZ //MOD014
 _T("MEC1704QSZ\n"),                                 // BCPRG_FOUND_MEC1704QSZ //MOD014
 _T("MEC1705HSZ\n"),                                 // BCPRG_FOUND_MEC1705HSZ //MOD014
 _T("MEC1705KSZ\n"),                                 // BCPRG_FOUND_MEC1705KSZ //MOD014
 _T("MEC1705QSZ\n"),                                 // BCPRG_FOUND_MEC1705QSZ //MOD010
 _T("MEC1706HSZ\n"),                                 // BCPRG_FOUND_MEC1706HSZ //MOD014
 _T("MEC1706KSZ\n"),                                 // BCPRG_FOUND_MEC1706KSZ //MOD014
 _T("MEC1706QSZ\n"),                                 // BCPRG_FOUND_MEC1706QSZ //MOD012
 _T("Checking locks and fuses. . . . "),             // BCPRG_CHECK_FUSES_BEFORE
 _T("EEPROM is unprotected"),                        // BCPRG_BAD_FUSES_ERROR
 passedMes,                                          // BCPRG_CHECK_FUSES_AFTER
 _T("Programming EESAVE fuse . . . . "),             // BCPRG_SET_EESAVE_BEFORE
 prgErrMes,                                          // BCPRG_SET_EESAVE_ERROR
 passedMes,                                          // BCPRG_SET_EESAVE_AFTER
 _T("Erasing FLASH . . . . . . . . . "),             // BCPRG_FLS_ERASE_BEFORE
 _T("Erasing FLASH and EEPROM. . . . "),             // BCPRG_FLS_EEP_ERASE_BEFORE
 prgErrMes,                                          // BCPRG_ERASE_ERROR
 passedMes,                                          // BCPRG_ERASE_AFTER
 _T("Resetting EESAVE fuse . . . . . "),             // BCPRG_CLEAR_EESAVE_BEFORE
 prgErrMes,                                          // BCPRG_CLEAR_EESAVE_ERROR
 passedMes,                                          // BCPRG_CLEAR_EESAVE_AFTER
 _T("Verifying clean EEPROM. . . . . "),             // BCPRG_VERIFY_EEP_CLEAN_BEFORE
 verErrMes,                                          // BCPRG_VERIFY_EEP_CLEAN_ERROR
 passedMes,                                          // BCPRG_VERIFY_EEP_CLEAN_AFTER
 _T("Checking FLASH space. . . . . . "),             // BCPRG_CHECK_FLS_SPACE_BEFORE
 flsSizErrMes,                                       // BCPRG_CHECK_FLS_SPACE_ERROR
 passedMes,                                          // BCPRG_CHECK_FLS_SPACE_AFTER
 _T("Programming FLASH . . . . . . . "),             // BCPRG_PROGRAM_FLS_BEFORE
 prgErrMes,                                          // BCPRG_PROGRAM_FLS_ERROR
 passedMes,                                          // BCPRG_PROGRAM_FLS_AFTER
 _T("Checking EEPROM space . . . . . "),             // BCPRG_CHECK_EEP_SPACE_BEFORE
 eepSizErrMes,                                       // BCPRG_CHECK_EEP_SPACE_ERROR
 passedMes,                                          // BCPRG_CHECK_EEP_SPACE_AFTER
 _T("Programming EEPROM. . . . . . . "),             // BCPRG_PROGRAM_EEP_BEFORE
 prgErrMes,                                          // BCPRG_PROGRAM_EEP_ERROR
 passedMes,                                          // BCPRG_PROGRAM_EEP_AFTER
 _T("Programming fuse bits . . . . . "),             // BCPRG_PROGRAM_FUSES_BEFORE
 prgErrMes,                                          // BCPRG_WRITE_FUSEX_ERROR
 prgErrMes,                                          // BCPRG_WRITE_FUSEH_ERROR
 prgErrMes,                                          // BCPRG_WRITE_FUSEL_ERROR
 passedMes,                                          // BCPRG_PROGRAM_FUSES_AFTER
 _T("\nWARNING! Some fuse settings were ignored.\n\n"), // BCPRG_FUSES_WARNING
 _T("Verifying FLASH . . . . . . . . "),             // BCPRG_VERIFY_FLS_BEFORE
 verErrMes,                                          // BCPRG_VERIFY_FLS_ERROR
 passedMes,                                          // BCPRG_VERIFY_FLS_AFTER
 _T("Verifying EEPROM. . . . . . . . "),             // BCPRG_VERIFY_EEP_BEFORE
 verErrMes,                                          // BCPRG_VERIFY_EEP_ERROR
 passedMes,                                          // BCPRG_VERIFY_EEP_AFTER
 _T("Verifying fuse bits . . . . . . "),             // BCPRG_VERIFY_FUSES_BEFORE
 verErrMes,                                          // BCPRG_VERIFY_FUSES_ERROR
 passedMes,                                          // BCPRG_VERIFY_FUSES_AFTER
 _T("Programming lock bits . . . . . "),             // BCPRG_PROGRAM_LOCKS_BEFORE
 prgErrMes,                                          // BCPRG_WRITE_LOCKS_ERROR
 passedMes,                                          // BCPRG_PROGRAM_LOCKS_AFTER
 _T("Verifying lock bits . . . . . . "),             // BCPRG_VERIFY_LOCKS_BEFORE
 verErrMes,                                          // BCPRG_VERIFY_LOCKS_ERROR
 passedMes,                                          // BCPRG_VERIFY_LOCKS_AFTER
 _T("\nWARNING! The controller firmware is unprotected.\n"), // BCPRG_LOCKS_WARNING
 _T("unknown error")                                 // BCPRG_UNKNOWN_ERROR
};



/*-----------------------------
 * Global and static variables
 *-----------------------------
 */

       _TCHAR         infoMesBuf[BCPRG_MAX_LIN_SIZ];
static UINT32  flags      = 0;
static _TCHAR        *pDatFilNam = NULL;



/*---------------------------------------------------------------------------
 * Name: static void ParseArguments( INT32 argc, _TCHAR **argv )
 * Desc: This function parses the command line parameters. It determines the
 *       name of the firmware data file and checks for optional command line
 *       switches. In case of a syntax error it sets the BCPRG_SYNTAX_ERROR
 *       flag.
 * Inp:  INT32 argc      - command line parameter count
 *       _TCHAR **argv - array of pointers to command line parameters
 * Outp: pDatFilNam - pointer to name of firmware data file
 *       flags      - flags set according to optional command line switches
 *---------------------------------------------------------------------------
 */

static void ParseArguments( INT32 argc, _TCHAR **argv )
{
 INT32 i;

 if(argc == 1)
 {
    flags |= BCPRG_HELP_SWITCH;
    return;
 }
                     
 for( i=1; i<argc; i++ )       // Ignore argument 0 which is the program name.
   {
    if( argv[i][0] == _T('/')  ||  argv[i][0] == _T('-') )
      {
       switch( TOLOWER( argv[i][1] ) )        // Handle command line switches.
         {
          case _T('s'):
                      flags |= BCPRG_SILENT_SWITCH;
                      break;

          case _T('n'):
                      flags |= BCPRG_NOLOGO_SWITCH;
                      break;

          case _T('v'):
                      flags |= BCPRG_VERBOSE_SWITCH;
                      break;

          case _T('b'):
                                                                     //MOD015v  
                      if( !STRCMP( &argv[i][2], _T("ldrenable") ) )
                        {
                         flags |= BCPRG_BLDRENA_SWITCH;
                         break;
                        }
                      if( !STRCMP( &argv[i][2], _T("ldrdisable") ) )
                        {
                         flags |= BCPRG_BLDRDIS_SWITCH;
                         break;
                        }
                                                                     //MOD015^
                                                                     //MOD004v
                      if( !STRCMP( &argv[i][2], _T("blk") ) )
                        {
                         flags |= BCPRG_BB_UPDATE;
                         break;                                      //MOD015
                        }
                      if( argv[i][2] == '\0' )                       //MOD015
                        {
                                                                     //MOD004^
                         flags |= BCPRG_BATCH_SWITCH;
                         break;                                      //MOD015
                        }                                            //MOD004
                      flags |= BCPRG_SYNTAX_ERROR;                   //MOD015
                      break;

          case _T('h'):
                      flags |= BCPRG_HELP_SWITCH;
                      break;

          case _T('q'):
                      flags |= BCPRG_SKIP_VERIFY_FF;
                      break;

          case _T('f'):
                      flags |= BCPRG_SKIP_VERIFY;
                      break;

          case _T('!'):
                      if( !STRCMP( &argv[i][2], _T("{[(e)]}") ) )
                        {
                         flags |= BCPRG_EEP_ERASE_SWITCH;
                        }
                      if( !STRCMP( &argv[i][2], _T("{[(m)]}") ) )
                        {
                         flags |= BCPRG_MFG_WRITE_SWITCH;
                        }
                                                                      //MOD002v  
                      if( !STRCMP( &argv[i][2], _T("nofuseprotection") ) )
                        {
                         flags |= BCPRG_NFP_SWITCH;
                        }
                                                                      //MOD002^
                                                                      //MOD013v  
                      if( !STRCMP( &argv[i][2], _T("cidoverride") ) )
                        {
                         flags |= BCPRG_CID_OVR_SWITCH;
                        }
                                                                      //MOD013^
                      break;

          default:
                      flags |= BCPRG_SYNTAX_ERROR;          // Unknown switch!
                      break;
         }
      }
    else
      {
       if( pDatFilNam )                  // Handle name of firmware data file.
         {
          flags |= BCPRG_SYNTAX_ERROR;        // Set error if already defined.
         }
       else
         {
          pDatFilNam = argv[i];            // Save name of firmware data file.
         }
      }
   }

 if( !pDatFilNam )
   {
    flags |= BCPRG_SYNTAX_ERROR;            // Set error if no filename found.
   }
                                                                     //MOD015v  
 if(    ((flags & BCPRG_BLDRENA_SWITCH) != 0)
     && ((flags & BCPRG_BLDRDIS_SWITCH) != 0) )
   {
    flags |= BCPRG_SYNTAX_ERROR;  // Set error upon mutual exclusive switches.
   }
                                                                     //MOD015^
}


/*---------------------------------------------------------------------------
 * Name: static INT32 HandleHelpAndSyntaxError( void )
 * Desc: This function handles the 'help'-switch and syntax errors.
 * Inp:  none
 * Outp: BCPRG_SYNTAX_CHECK_ERROR if syntax error
 *       BCPRG_HELP_AFTER if the help switch was processed
 *       BCPRG_PASSED if no help switch and no syntax error
 *---------------------------------------------------------------------------
 */

static INT32 HandleHelpAndSyntaxError( void )
{
 if( flags & BCPRG_HELP_SWITCH )                        // Handle help switch.
   {
    PRINTF( helpMes, "%s" );
    return( BCPRG_HELP_AFTER );
   }

 if( flags & BCPRG_SYNTAX_ERROR )                      // Handle syntax error.
   {
    return( BCPRG_SYNTAX_CHECK_ERROR );
   }
 return( BCPRG_PASSED );
}



/*---------------------------------------------------------------------------
 * Name: static void PrintMessage( INT32 mesNum, _TCHAR *pErrMesParm )
 * Desc: This function displays a message from the message table on the
 *       console.
 * Inp:  INT32 mesNum          - index into message table
 *       _TCHAR *pErrMesParm - pointer to an optional string parameter
 * Outp: none
 *---------------------------------------------------------------------------
 */

static void PrintMessage( INT32 mesNum, _TCHAR *pErrMesParm )
{
    //MOD011 v 
    if( !(flags & BCPRG_SILENT_SWITCH) )
    {
        if( mesNum >= sizeof( messages ) )
        {
            PRINTF( messages[BCPRG_UNKNOWN_ERROR], "%c" );
        }
        else
        {
            if( messages[mesNum] )
            {
                PRINTF( messages[mesNum], pErrMesParm );
            }
        }
    }
    //MOD011 ^
}



/*---------------------------------------------------------------------------
 * Name: static void StdErr( INT32 errCode )
 * Desc: This function is the standard error handler. It simply displays the
 *       error code and the corresponding error message on the console.
 * Inp:  INT32 errCode - error code
 * Outp: none
 *---------------------------------------------------------------------------
 */

static void StdErr( INT32 errCode )
{
 PRINTF( nlNl, "%s" );
 PRINTF( errMes, errCode );
 PrintMessage( errCode, NULL );
 PRINTF( nl, "%s" );
}



/*---------------------------------------------------------------------------
 * Name: static void FilErr( INT32 errCode )
 * Desc: This function is the handler for file errors. It displays the error
 *       code and the corresponding error message including the filename on
 *       the console.
 * Inp:  INT32     errCode    - error code
 *       _TCHAR *pDatFilNam - pointer to filename
 * Outp: none
 *---------------------------------------------------------------------------
 */

static void FilErr( INT32 errCode )
{
 PRINTF( nlNl, "%s" );
 PRINTF( errMes, errCode );
 PrintMessage( errCode, pDatFilNam );
 PRINTF( nl, "%s" );
}



/*---------------------------------------------------------------------------
 * Name: static void InfoErr( INT32 errCode )
 * Desc: This function is the handler for errors with additional information.
 *       It displays the error code and the corresponding error message
 *       including the additional information on the console.
 * Inp:  INT32     errCode    - error code
 *       _TCHAR *infoMesBuf - pointer to additional information string
 * Outp: none
 *---------------------------------------------------------------------------
 */

static void InfoErr( INT32 errCode )
{
 PRINTF( nlNl, "%s" );
 PRINTF( errMes, errCode );
 PrintMessage( errCode, infoMesBuf );
 PRINTF( nl, "%s" );
}



/*---------------------
 * Error handler table
 *---------------------
 */

static void (*errHandlerTbl[])() = {
                                    NULL,      // BCPRG_PASSED
                                    StdErr,    // BCPRG_ABORT
                                    NULL,      // BCPRG_HELP_AFTER
                                    StdErr,    // BCPRG_SYNTAX_CHECK_ERROR
                                    NULL,      // BCPRG_OPEN_DAT_FILE_BEFORE
                                    FilErr,    // BCPRG_OPEN_DAT_FILE_ERROR
                                    NULL,      // BCPRG_OPEN_DAT_FILE_AFTER
                                    NULL,      // BCPRG_MALLOC_BEFORE
                                    StdErr,    // BCPRG_MALLOC_ERROR
                                    NULL,      // BCPRG_MALLOC_AFTER
                                    NULL,      // BCPRG_READ_DAT_FILE_BEFORE
                                    FilErr,    // BCPRG_READ_DAT_FILE_ERROR
                                    NULL,      // BCPRG_READ_DAT_FILE_AFTER
                                    NULL,      // BCPRG_CHECK_DAT_FILE_BEFORE
                                    FilErr,    // BCPRG_CHECK_DAT_FILE_ERROR
                                    StdErr,    // BCPRG_FLASH_SIZE_ERROR
                                    StdErr,    // BCPRG_EEPROM_SIZE_ERROR
                                    NULL,      // BCPRG_CHECK_DAT_FILE_AFTER
                                    NULL,      // BCPRG_READ_DAT_FILE_COMPLETE
                                    NULL,      // BCPRG_CGOS_INIT_BEFORE
                                    StdErr,    // BCPRG_CGOS_INIT_ERROR
                                    NULL,      // BCPRG_CGOS_INIT_AFTER
                                    NULL,      // BCPRG_CHECK_COMPATIBILITY_BEFORE
                                    StdErr,    // BCPRG_CHECK_COMPATIBILITY_ERROR
                                    NULL,      // BCPRG_CHECK_COMPATIBILITY_AFTER
                                    NULL,      // BCPRG_PROG_ENABLE_BEFORE
                                    StdErr,    // BCPRG_PROG_ENABLE_ERROR
                                    NULL,      // BCPRG_PROG_ENABLE_AFTER
                                    NULL,      // BCPRG_CHECK_DEVICE_BEFORE
                                    StdErr,    // BCPRG_UNKNOWN_DEVICE_ERROR
                                    NULL,      // BCPRG_FOUND_ATMEGA48
                                    NULL,      // BCPRG_FOUND_ATMEGA48PA //MOD007
                                    NULL,      // BCPRG_FOUND_ATMEGA88
                                    NULL,      // BCPRG_FOUND_ATMEGA88PA //MOD007
                                    NULL,      // BCPRG_FOUND_ATMEGA168
                                    NULL,      // BCPRG_FOUND_ATMEGA168PA //MOD007
                                    NULL,      // BCPRG_FOUND_ATMEGA328P //MOD007
                                    NULL,      // BCPRG_FOUND_ATMEGA165P //MOD003
                                    NULL,      // BCPRG_FOUND_ATMEGA325P //MOD006
                                    NULL,      // BCPRG_FOUND_STM32F100R8 //MOD008
                                    NULL,      // BCPRG_FOUND_TM4E1231H6ZRB //MOD009
                                    NULL,      // BCPRG_FOUND_MEC1701HSZ //MOD014
                                    NULL,      // BCPRG_FOUND_MEC1701KSZ //MOD014
                                    NULL,      // BCPRG_FOUND_MEC1701QSZ //MOD014
                                    NULL,      // BCPRG_FOUND_MEC1703HSZ //MOD014
                                    NULL,      // BCPRG_FOUND_MEC1703KSZ //MOD014
                                    NULL,      // BCPRG_FOUND_MEC1703QSZ //MOD014
                                    NULL,      // BCPRG_FOUND_MEC1704HSZ //MOD014
                                    NULL,      // BCPRG_FOUND_MEC1704KSZ //MOD014
                                    NULL,      // BCPRG_FOUND_MEC1704QSZ //MOD014
                                    NULL,      // BCPRG_FOUND_MEC1705HSZ //MOD014
                                    NULL,      // BCPRG_FOUND_MEC1705KSZ //MOD014
                                    NULL,      // BCPRG_FOUND_MEC1705QSZ //MOD010
                                    NULL,      // BCPRG_FOUND_MEC1706HSZ //MOD014
                                    NULL,      // BCPRG_FOUND_MEC1706KSZ //MOD014
                                    NULL,      // BCPRG_FOUND_MEC1706QSZ //MOD012
                                    NULL,      // BCPRG_CHECK_FUSES_BEFORE
                                    StdErr,    // BCPRG_BAD_FUSES_ERROR
                                    NULL,      // BCPRG_CHECK_FUSES_AFTER
                                    NULL,      // BCPRG_SET_EESAVE_BEFORE
                                    StdErr,    // BCPRG_SET_EESAVE_ERROR
                                    NULL,      // BCPRG_SET_EESAVE_AFTER
                                    NULL,      // BCPRG_FLS_ERASE_BEFORE
                                    NULL,      // BCPRG_FLS_EEP_ERASE_BEFORE
                                    StdErr,    // BCPRG_ERASE_ERROR
                                    NULL,      // BCPRG_ERASE_AFTER
                                    NULL,      // BCPRG_CLEAR_EESAVE_BEFORE
                                    StdErr,    // BCPRG_CLEAR_EESAVE_ERROR
                                    NULL,      // BCPRG_CLEAR_EESAVE_AFTER
                                    NULL,      // BCPRG_VERIFY_EEP_CLEAN_BEFORE
                                    InfoErr,   // BCPRG_VERIFY_EEP_CLEAN_ERROR
                                    NULL,      // BCPRG_VERIFY_EEP_CLEAN_AFTER
                                    NULL,      // BCPRG_CHECK_FLS_SPACE_BEFORE
                                    StdErr,    // BCPRG_CHECK_FLS_SPACE_ERROR
                                    NULL,      // BCPRG_CHECK_FLS_SPACE_AFTER
                                    NULL,      // BCPRG_PROGRAM_FLS_BEFORE
                                    StdErr,    // BCPRG_PROGRAM_FLS_ERROR
                                    NULL,      // BCPRG_PROGRAM_FLS_AFTER
                                    NULL,      // BCPRG_CHECK_EEP_SPACE_BEFORE
                                    StdErr,    // BCPRG_CHECK_EEP_SPACE_ERROR
                                    NULL,      // BCPRG_CHECK_EEP_SPACE_AFTER
                                    NULL,      // BCPRG_PROGRAM_EEP_BEFORE
                                    StdErr,    // BCPRG_PROGRAM_EEP_ERROR
                                    NULL,      // BCPRG_PROGRAM_EEP_AFTER
                                    NULL,      // BCPRG_PROGRAM_FUSES_BEFORE
                                    StdErr,    // BCPRG_WRITE_FUSEX_ERROR
                                    StdErr,    // BCPRG_WRITE_FUSEH_ERROR
                                    StdErr,    // BCPRG_WRITE_FUSEL_ERROR
                                    NULL,      // BCPRG_PROGRAM_FUSES_AFTER
                                    NULL,      // BCPRG_FUSES_WARNING
                                    NULL,      // BCPRG_VERIFY_FLS_BEFORE
                                    InfoErr,   // BCPRG_VERIFY_FLS_ERROR
                                    NULL,      // BCPRG_VERIFY_FLS_AFTER
                                    NULL,      // BCPRG_VERIFY_EEP_BEFORE
                                    InfoErr,   // BCPRG_VERIFY_EEP_ERROR
                                    NULL,      // BCPRG_VERIFY_EEP_AFTER
                                    NULL,      // BCPRG_VERIFY_FUSES_BEFORE
                                    InfoErr,   // BCPRG_VERIFY_FUSES_ERROR
                                    NULL,      // BCPRG_VERIFY_FUSES_AFTER
                                    NULL,      // BCPRG_PROGRAM_LOCKS_BEFORE
                                    StdErr,    // BCPRG_WRITE_LOCKS_ERROR
                                    NULL,      // BCPRG_PROGRAM_LOCKS_AFTER
                                    NULL,      // BCPRG_VERIFY_LOCKS_BEFORE
                                    InfoErr,   // BCPRG_VERIFY_LOCKS_ERROR
                                    NULL,      // BCPRG_VERIFY_LOCKS_AFTER
                                    NULL,      // BCPRG_LOCKS_WARNING
                                    StdErr     // BCPRG_UNKNOWN_ERROR
                                   };



/*---------------------------------------------------------------------------
 * Name: void BcprgShowProgress( INT32 progressCode )
 * Desc: This function displays the message corresponding to a specific
 *       progress code.
 * Inp:  INT32 progressCode - progress code (see bcprg.h)
 * Outp: none
 *---------------------------------------------------------------------------
 */

void BcprgShowProgress( INT32 progressCode )
{
 PrintMessage( progressCode, NULL );
}



/*---------------------------------------------------------------------------
 * Name: INT32 CgbcprgcMain( INT32 argc, _TCHAR* argv[] )
 * Desc: This is the main function of the command line version of the congatec
 *       board controller firmware update utility.
 * Inp:  INT32 argc      - command line parameter count
 *       _TCHAR **argv - array of pointers to command line parameters
 * Outp: BCPRG_PASSED if the firmware update was successful
 *       error code if the firmware update failed
 *---------------------------------------------------------------------------
 */

INT32 BcprgcmdMain( INT32 argc, _TCHAR* argv[] )
{
 INT32 retCode;

 ParseArguments( argc, argv );               // Check command line parameters.
 retCode = HandleHelpAndSyntaxError();       // Process help and syntax error.

 if( retCode == BCPRG_PASSED )
   {
    retCode = BcprgcmnMain( pDatFilNam, flags );   // Execute firmware update.
   }

 if( retCode != BCPRG_PASSED  &&             // Handle errors.
     retCode != BCPRG_HELP_AFTER )
   {
    if( retCode >= sizeof( errHandlerTbl ) )
      {
       retCode = BCPRG_UNKNOWN_ERROR;
      }
    (*errHandlerTbl[retCode])( retCode );
   }

 return( retCode );                          // Return result.
}
