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
 * Contents: Board controller firmware update common module definitions.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * MOD013: Added support for new command line switches /bldrenable and
 *         /bldrdisable.
 *
 * MOD012: Added support for all MEC170x variants in the 144 pin WFBGA package.
 *
 * MOD011: added support for the /!cidoverride switch and the AVR SPM extension
 *         commands.
 *
 * MOD010: added support for MEC1706
 *
 * MOD009: added support for MEC1705
 * 
 *    Rev 1.9   Sep 06 2016 15:18:12   congatec
 * Updated BSD header.
 * 
 *    Rev 1.8   Jun 26 2013 10:47:30   ess
 * MOD008: added support for TM4E1231H6ZRB
 * 
 *    Rev 1.7   May 31 2012 15:45:18   gartner
 * Updated variable definition to ease 64bit porting.
 * 
 *    Rev 1.6   Nov 11 2010 09:53:04   ess
 * MOD007: added support for bootblocks at the bottom of the flash area and
 *         for flash areas reserved for EEPROM emulation
 * MOD006: added support for STM32F100R8
 * 
 *    Rev 1.5   Nov 19 2009 10:58:34   ess
 * MOD005: added support for ATmega48PA/88PA/168PA/328P
 * 
 *    Rev 1.4   Dec 18 2008 15:36:52   ess
 * MOD004: added support for ATmega325P
 * 
 *    Rev 1.3   Nov 18 2008 14:15:18   ess
 * MOD003: added CGBC bootblock support
 * 
 *    Rev 1.2   Aug 26 2008 21:37:18   ess
 * MOD002: added support for ATmega165P
 * 
 *    Rev 1.1   Apr 15 2008 22:01:28   ess
 * MOD001: added support for the /!nofuseprotection command line switch.
 * 
 *    Rev 1.0   Oct 04 2005 13:14:14   gartner
 * Initial revision.
 * 
 *    Rev 1.0   Sep 30 2005 16:31:16   gartner
 * Initial revision.
 *
 *---------------------------------------------------------------------------
 */


#ifdef __cplusplus
extern "C" {
#endif


/*--------------------
 * Flags and switches
 *--------------------
 */

#define  BCPRG_SYNTAX_ERROR      0x00001
#define  BCPRG_SILENT_SWITCH     0x00002
#define  BCPRG_NOLOGO_SWITCH     0x00004
#define  BCPRG_VERBOSE_SWITCH    0x00008
#define  BCPRG_BATCH_SWITCH      0x00010
#define  BCPRG_EEP_ERASE_SWITCH  0x00020
#define  BCPRG_PROG_ENABLED      0x00040
#define  BCPRG_FLS_PROG_REQ      0x00080
#define  BCPRG_EEP_PROG_REQ      0x00100
#define  BCPRG_FUSEX_PROG_REQ    0x00200
#define  BCPRG_FUSEH_PROG_REQ    0x00400
#define  BCPRG_FUSEL_PROG_REQ    0x00800
#define  BCPRG_LOCKS_PROG_REQ    0x01000
#define  BCPRG_CHIP_ERASE_REQ    0x02000
#define  BCPRG_HELP_SWITCH       0x04000
#define  BCPRG_SKIP_VERIFY_FF    0x08000
#define  BCPRG_SKIP_VERIFY       0x10000
#define  BCPRG_EEP_ERASED        0x20000
#define  BCPRG_MFG_WRITE_SWITCH  0x40000
#define  BCPRG_NFP_SWITCH        0x80000                             //MOD001
#define  BCPRG_BB_SUPPORT       0x100000                             //MOD002
#define  BCPRG_BB_UPDATE        0x200000                             //MOD003
#define  BCPRG_BB_BOTTOM        0x400000                             //MOD007
#define  BCPRG_EEEP_SUPPORT     0x800000                             //MOD007
#define  BCPRG_AVR_DISABLE_CMD 0x1000000                             //MOD009
#define  BCPRG_CID_OVR_SWITCH  0x2000000                             //MOD011
#define  BCPRG_SPM_EXT_SUPPORT 0x4000000                             //MOD011
#define  BCPRG_BLDRENA_SWITCH  0x8000000                             //MOD013
#define  BCPRG_BLDRDIS_SWITCH 0x10000000                             //MOD013


/*--------------------------
 * Progress and error codes
 *--------------------------
 */

enum  {
       BCPRG_PASSED,
       BCPRG_ABORT,
       BCPRG_HELP_AFTER,
       BCPRG_SYNTAX_CHECK_ERROR,
       BCPRG_OPEN_DAT_FILE_BEFORE,
       BCPRG_OPEN_DAT_FILE_ERROR,
       BCPRG_OPEN_DAT_FILE_AFTER,
       BCPRG_MALLOC_BEFORE,
       BCPRG_MALLOC_ERROR,
       BCPRG_MALLOC_AFTER,
       BCPRG_READ_DAT_FILE_BEFORE,
       BCPRG_READ_DAT_FILE_ERROR,
       BCPRG_READ_DAT_FILE_AFTER,
       BCPRG_CHECK_DAT_FILE_BEFORE,
       BCPRG_CHECK_DAT_FILE_ERROR,
       BCPRG_FLASH_SIZE_ERROR,
       BCPRG_EEPROM_SIZE_ERROR,
       BCPRG_CHECK_DAT_FILE_AFTER,
       BCPRG_READ_DAT_FILE_COMPLETE,
       BCPRG_CGOS_INIT_BEFORE,
       BCPRG_CGOS_INIT_ERROR,
       BCPRG_CGOS_INIT_AFTER,
       BCPRG_CHECK_COMPATIBILITY_BEFORE,
       BCPRG_CHECK_COMPATIBILITY_ERROR,
       BCPRG_CHECK_COMPATIBILITY_AFTER,
       BCPRG_PROG_ENABLE_BEFORE,
       BCPRG_PROG_ENABLE_ERROR,
       BCPRG_PROG_ENABLE_AFTER,
       BCPRG_CHECK_DEVICE_BEFORE,
       BCPRG_UNKNOWN_DEVICE_ERROR,
       BCPRG_FOUND_ATMEGA48,
       BCPRG_FOUND_ATMEGA48PA,                                       //MOD005
       BCPRG_FOUND_ATMEGA88,
       BCPRG_FOUND_ATMEGA88PA,                                       //MOD005
       BCPRG_FOUND_ATMEGA168,
       BCPRG_FOUND_ATMEGA168PA,                                      //MOD005
       BCPRG_FOUND_ATMEGA328P,                                       //MOD005
       BCPRG_FOUND_ATMEGA165P,                                       //MOD002
       BCPRG_FOUND_ATMEGA325P,                                       //MOD004
       BCPRG_FOUND_STM32F100R8,                                      //MOD006
       BCPRG_FOUND_TM4E1231H6ZRB,                                    //MOD008
       BCPRG_FOUND_MEC1701HSZ,                                       //MOD012
       BCPRG_FOUND_MEC1701KSZ,                                       //MOD012
       BCPRG_FOUND_MEC1701QSZ,                                       //MOD012
       BCPRG_FOUND_MEC1703HSZ,                                       //MOD012
       BCPRG_FOUND_MEC1703KSZ,                                       //MOD012
       BCPRG_FOUND_MEC1703QSZ,                                       //MOD012
       BCPRG_FOUND_MEC1704HSZ,                                       //MOD012
       BCPRG_FOUND_MEC1704KSZ,                                       //MOD012
       BCPRG_FOUND_MEC1704QSZ,                                       //MOD012
       BCPRG_FOUND_MEC1705HSZ,                                       //MOD012
       BCPRG_FOUND_MEC1705KSZ,                                       //MOD012
       BCPRG_FOUND_MEC1705QSZ,                                       //MOD009
       BCPRG_FOUND_MEC1706HSZ,                                       //MOD012
       BCPRG_FOUND_MEC1706KSZ,                                       //MOD012
       BCPRG_FOUND_MEC1706QSZ,                                       //MOD010
       BCPRG_CHECK_FUSES_BEFORE,
       BCPRG_BAD_FUSES_ERROR,
       BCPRG_CHECK_FUSES_AFTER,
       BCPRG_SET_EESAVE_BEFORE,
       BCPRG_SET_EESAVE_ERROR,
       BCPRG_SET_EESAVE_AFTER,
       BCPRG_FLS_ERASE_BEFORE,
       BCPRG_FLS_EEP_ERASE_BEFORE,
       BCPRG_ERASE_ERROR,
       BCPRG_ERASE_AFTER,
       BCPRG_CLEAR_EESAVE_BEFORE,
       BCPRG_CLEAR_EESAVE_ERROR,
       BCPRG_CLEAR_EESAVE_AFTER,
       BCPRG_VERIFY_EEP_CLEAN_BEFORE,
       BCPRG_VERIFY_EEP_CLEAN_ERROR,
       BCPRG_VERIFY_EEP_CLEAN_AFTER,
       BCPRG_CHECK_FLS_SPACE_BEFORE,
       BCPRG_CHECK_FLS_SPACE_ERROR,
       BCPRG_CHECK_FLS_SPACE_AFTER,
       BCPRG_PROGRAM_FLS_BEFORE,
       BCPRG_PROGRAM_FLS_ERROR,
       BCPRG_PROGRAM_FLS_AFTER,
       BCPRG_CHECK_EEP_SPACE_BEFORE,
       BCPRG_CHECK_EEP_SPACE_ERROR,
       BCPRG_CHECK_EEP_SPACE_AFTER,
       BCPRG_PROGRAM_EEP_BEFORE,
       BCPRG_PROGRAM_EEP_ERROR,
       BCPRG_PROGRAM_EEP_AFTER,
       BCPRG_PROGRAM_FUSES_BEFORE,
       BCPRG_WRITE_FUSEX_ERROR,
       BCPRG_WRITE_FUSEH_ERROR,
       BCPRG_WRITE_FUSEL_ERROR,
       BCPRG_PROGRAM_FUSES_AFTER,
       BCPRG_FUSES_WARNING,
       BCPRG_VERIFY_FLS_BEFORE,
       BCPRG_VERIFY_FLS_ERROR,
       BCPRG_VERIFY_FLS_AFTER,
       BCPRG_VERIFY_EEP_BEFORE,
       BCPRG_VERIFY_EEP_ERROR,
       BCPRG_VERIFY_EEP_AFTER,
       BCPRG_VERIFY_FUSES_BEFORE,
       BCPRG_VERIFY_FUSES_ERROR,
       BCPRG_VERIFY_FUSES_AFTER,
       BCPRG_PROGRAM_LOCKS_BEFORE,
       BCPRG_WRITE_LOCKS_ERROR,
       BCPRG_PROGRAM_LOCKS_AFTER,
       BCPRG_VERIFY_LOCKS_BEFORE,
       BCPRG_VERIFY_LOCKS_ERROR,
       BCPRG_VERIFY_LOCKS_AFTER,
       BCPRG_LOCKS_WARNING,
       BCPRG_UNKNOWN_ERROR
      };



/*---------------
 * Miscellaneous
 *---------------
 */

#define BCPRG_MAX_LIN_SIZ  80



/*--------
 * Macros
 *--------
 */

#define BCPRG_CONSOLE_OUTPUT( x ) if( !(flags & BCPRG_SILENT_SWITCH) ) PRINTF( x );
#define BCPRG_VERBOSE_OPEN        if( flags & BCPRG_VERBOSE_SWITCH ) {
#define BCPRG_VERBOSE_CLOSE       }
#ifdef _CONSOLE
#define BCPRG_VERBOSE_PRINTF( fmt, prm1, prm2 ) printf( fmt, prm1, prm2 )
#else //_CONSOLE
#define BCPRG_VERBOSE_PRINTF( fmt, prm1, prm2 )
#endif //_CONSOLE


extern _TCHAR infoMesBuf[BCPRG_MAX_LIN_SIZ];


#ifdef __cplusplus
}
#endif

