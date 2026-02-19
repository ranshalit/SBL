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
 * $Header:   S:/CG/archives/CGTOOLS/INC/atmelavr.h-arc   1.7   Sep 06 2016 15:17:48   congatec  $
 *
 * Contents: Atmel AVR microcontroller declarations
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/INC/atmelavr.h-arc  $
 * 
 *    Rev 1.7   Sep 06 2016 15:17:48   congatec
 * Updated BSD header.
 * 
 *    Rev 1.6   Jun 26 2013 10:56:56   ess
 * MOD008: added AVR_SPM_PROG_DISABLE command used for AVR SPM emulations
 * MOD007: added bit field masks and shift counts
 * 
 *    Rev 1.5   May 31 2012 15:45:16   gartner
 * Updated variable definition to ease 64bit porting.
 * 
 *    Rev 1.4   Nov 19 2009 10:56:28   ess
 * MOD006: added support for ATmega48PA/88PA/168PA/328P
 * 
 *    Rev 1.3   Jan 09 2009 11:27:48   ess
 * MOD005: corrected extended fuse bits declaration for ATmega325P
 * 
 *    Rev 1.2   Dec 18 2008 15:59:16   ess
 * MOD004: added support for ATmega325P
 * 
 *    Rev 1.1   Aug 26 2008 21:34:24   ess
 * MOD003: added definitions of lock bits and fuse bits of ATmegaX8
 * MOD002: definition of the SPM busy bit corrected
 * MOD001: added support for ATmega165P
 * 
 *    Rev 1.0   Sep 30 2005 16:29:08   gartner
 * Initial revision.
 *
 *---------------------------------------------------------------------------
 */



/*----------------------------------
 * Serial programming mode commands
 *----------------------------------
 */

#define AVR_SPM_PROG_ENABLE            0xAC530000
#define  AVR_SPM_PROG_ENABLE_OKAY_MSK  0x0000FF00
#define  AVR_SPM_PROG_ENABLE_OKAY      0x00005300

#define AVR_SPM_PROG_DISABLE           0x00000000                    /*MOD008*/
#define AVR_SPM_CHIP_ERASE             0xAC800000
#define AVR_SPM_RD_PROG_MEM_LO         0x20000000
#define AVR_SPM_RD_PROG_MEM_HI         0x28000000

#define AVR_SPM_LD_EXT_ADDR            0x4D000000                    //MOD001
#define  AVR_SPM_EXT_ADDR_M            0x00FFFF00                    /*MOD007*/
#define  AVR_SPM_EXT_ADDR_S                   8                      /*MOD007*/

#define AVR_SPM_LD_PROG_MEM_PAGE_LO    0x40000000
#define AVR_SPM_LD_PROG_MEM_PAGE_HI    0x48000000
#define  AVR_SPM_PROG_MEM_PAGE_IDX_M   0x00FFFF00                    /*MOD007*/
#define  AVR_SPM_PROG_MEM_PAGE_IDX_S          8                      /*MOD007*/

#define AVR_SPM_WR_PROG_MEM_PAGE       0x4C000000
#define  AVR_SPM_PROG_MEM_PAGE_ADDR_M  0x00FFFF00                    /*MOD007*/
#define  AVR_SPM_PROG_MEM_PAGE_ADDR_S         8                      /*MOD007*/

#define AVR_SPM_RD_EEPROM              0xA0000000
#define AVR_SPM_WR_EEPROM              0xC0000000
#define  AVR_SPM_EEPROM_IDX_M           0x00FFFF00                   /*MOD007*/
#define  AVR_SPM_EEPROM_IDX_S                  8                     /*MOD007*/
#define  AVR_SPM_EEPROM_DATA_M          0x000000FF                   /*MOD007*/
#define  AVR_SPM_EEPROM_DATA_S                   0                   /*MOD007*/

#define AVR_SPM_LD_EEPROM_PAGE         0xC1000000
#define AVR_SPM_WR_EEPROM_PAGE         0xC2000000

#define AVR_SPM_RD_LOCK_BITS           0x58000000
#define AVR_SPM_WR_LOCK_BITS           0xACE000C0
#define  AVR_SPM_LOCK_BITS_MSK         0x000000FF

#define AVR_SPM_RD_SIGNATURE_BYTE      0x30000000
#define  AVR_SPM_SIGNATURE_BYTE_M      0x00000300                    /*MOD007*/
#define  AVR_SPM_SIGNATURE_BYTE_S             8                      /*MOD007*/
#define  AVR_SPM_SIGNATURE_BYTE_0      0x00000000
#define  AVR_SPM_SIGNATURE_BYTE_1      0x00000100
#define  AVR_SPM_SIGNATURE_BYTE_2      0x00000200

#define AVR_SPM_WR_FUSE_BITS_LO        0xACA00000
#define AVR_SPM_RD_FUSE_BITS_LO        0x50000000
#define  AVR_SPM_FUSE_BITS_LO_MSK      0x000000FF

#define AVR_SPM_WR_FUSE_BITS_HI        0xACA80000
#define AVR_SPM_RD_FUSE_BITS_HI        0x58080000
#define  AVR_SPM_FUSE_BITS_HI_MSK      0x000000FF
#define  AVR_SPM_FUSE_BITS_HI_M        0x000000FF                    /*MOD007*/
#define  AVR_SPM_FUSE_BITS_HI_S                 0                    /*MOD007*/

#define AVR_SPM_WR_FUSE_BITS_EX        0xACA40000
#define AVR_SPM_RD_FUSE_BITS_EX        0x50080000
#define  AVR_SPM_FUSE_BITS_EX_MSK      0x000000FF

#define AVR_SPM_RD_CALIBRATION_BYTE    0x38000000

#define AVR_SPM_POLL_RDY_BUSY          0xF0000000
#define  AVR_SPM_BSY                   0x00000001                    //MOD002



/*--------------------------------------
 * Atmel AVR microcontroller signatures
 *--------------------------------------
 */

#define AVR_ATMEGA48_SIGNATURE    0x0005921E
#define AVR_ATMEGA48PA_SIGNATURE  0x000A921E                         //MOD006
#define AVR_ATMEGA88_SIGNATURE    0x000A931E
#define AVR_ATMEGA88PA_SIGNATURE  0x000F931E                         //MOD006
#define AVR_ATMEGA168_SIGNATURE   0x0006941E
#define AVR_ATMEGA168PA_SIGNATURE 0x000B941E                         //MOD006
#define AVR_ATMEGA328P_SIGNATURE  0x000F951E                         //MOD006
#define AVR_ATMEGA16_SIGNATURE    0x0003941E
                                                                     //MOD001v
#define AVR_ATMEGA165P_SIGNATURE 0x0007941E
#define AVR_ATMEGA325P_SIGNATURE 0x000D951E                          //MOD004

#ifdef __AVR_ATmega48__
#define AVR_SIGNATURE_BYTE_0 ((AVR_ATMEGA48_SIGNATURE >>  0) &0xFF)
#define AVR_SIGNATURE_BYTE_1 ((AVR_ATMEGA48_SIGNATURE >>  8) &0xFF)
#define AVR_SIGNATURE_BYTE_2 ((AVR_ATMEGA48_SIGNATURE >> 16) &0xFF)
#endif //__AVR_ATmega48__
                                                                     //MOD006v
#ifdef __AVR_ATmega48P__
#define AVR_SIGNATURE_BYTE_0 ((AVR_ATMEGA48PA_SIGNATURE >>  0) &0xFF)
#define AVR_SIGNATURE_BYTE_1 ((AVR_ATMEGA48PA_SIGNATURE >>  8) &0xFF)
#define AVR_SIGNATURE_BYTE_2 ((AVR_ATMEGA48PA_SIGNATURE >> 16) &0xFF)
#endif //__AVR_ATmega48P__
                                                                     //MOD006^
#ifdef __AVR_ATmega88__
#define AVR_SIGNATURE_BYTE_0 ((AVR_ATMEGA88_SIGNATURE >>  0) &0xFF)
#define AVR_SIGNATURE_BYTE_1 ((AVR_ATMEGA88_SIGNATURE >>  8) &0xFF)
#define AVR_SIGNATURE_BYTE_2 ((AVR_ATMEGA88_SIGNATURE >> 16) &0xFF)
#endif //__AVR_ATmega88__
                                                                     //MOD006v
#ifdef __AVR_ATmega88P__
#define AVR_SIGNATURE_BYTE_0 ((AVR_ATMEGA88PA_SIGNATURE >>  0) &0xFF)
#define AVR_SIGNATURE_BYTE_1 ((AVR_ATMEGA88PA_SIGNATURE >>  8) &0xFF)
#define AVR_SIGNATURE_BYTE_2 ((AVR_ATMEGA88PA_SIGNATURE >> 16) &0xFF)
#endif //__AVR_ATmega88P__
                                                                     //MOD006^
#ifdef __AVR_ATmega168__
#define AVR_SIGNATURE_BYTE_0 ((AVR_ATMEGA168_SIGNATURE >>  0) &0xFF)
#define AVR_SIGNATURE_BYTE_1 ((AVR_ATMEGA168_SIGNATURE >>  8) &0xFF)
#define AVR_SIGNATURE_BYTE_2 ((AVR_ATMEGA168_SIGNATURE >> 16) &0xFF)
#endif //__AVR_ATmega168__
                                                                     //MOD006v
#ifdef __AVR_ATmega168P__
#define AVR_SIGNATURE_BYTE_0 ((AVR_ATMEGA168PA_SIGNATURE >>  0) &0xFF)
#define AVR_SIGNATURE_BYTE_1 ((AVR_ATMEGA168PA_SIGNATURE >>  8) &0xFF)
#define AVR_SIGNATURE_BYTE_2 ((AVR_ATMEGA168PA_SIGNATURE >> 16) &0xFF)
#endif //__AVR_ATmega168P__

#ifdef __AVR_ATmega328P__
#define AVR_SIGNATURE_BYTE_0 ((AVR_ATMEGA328P_SIGNATURE >>  0) &0xFF)
#define AVR_SIGNATURE_BYTE_1 ((AVR_ATMEGA328P_SIGNATURE >>  8) &0xFF)
#define AVR_SIGNATURE_BYTE_2 ((AVR_ATMEGA328P_SIGNATURE >> 16) &0xFF)
#endif //__AVR_ATmega328P__
                                                                     //MOD006^
#ifdef __AVR_ATmega16__
#define AVR_SIGNATURE_BYTE_0 ((AVR_ATMEGA16_SIGNATURE >>  0) &0xFF)
#define AVR_SIGNATURE_BYTE_1 ((AVR_ATMEGA16_SIGNATURE >>  8) &0xFF)
#define AVR_SIGNATURE_BYTE_2 ((AVR_ATMEGA16_SIGNATURE >> 16) &0xFF)
#endif //__AVR_ATmega16__

#ifdef __AVR_ATmega165P__
#define AVR_SIGNATURE_BYTE_0 ((AVR_ATMEGA165P_SIGNATURE >>  0) &0xFF)
#define AVR_SIGNATURE_BYTE_1 ((AVR_ATMEGA165P_SIGNATURE >>  8) &0xFF)
#define AVR_SIGNATURE_BYTE_2 ((AVR_ATMEGA165P_SIGNATURE >> 16) &0xFF)
#endif //__AVR_ATmega165P__
                                                                     //MOD004v
#ifdef __AVR_ATmega325P__
#define AVR_SIGNATURE_BYTE_0 ((AVR_ATMEGA325P_SIGNATURE >>  0) &0xFF)
#define AVR_SIGNATURE_BYTE_1 ((AVR_ATMEGA325P_SIGNATURE >>  8) &0xFF)
#define AVR_SIGNATURE_BYTE_2 ((AVR_ATMEGA325P_SIGNATURE >> 16) &0xFF)
#endif //__AVR_ATmega325P__
                                                                     //MOD004^
                                                                     //MOD001^


/*------------------------------
 * Flash, EEPROM and SRAM sizes
 *------------------------------
 */

#define AVR_ATMEGA48_FLASH_SIZE        4*1024
#define AVR_ATMEGA48_FLASH_PAGE_SIZE   32*2
#define AVR_ATMEGA48_EEPROM_SIZE       256
#define AVR_ATMEGA48_SRAM_SIZE         512

#define AVR_ATMEGA88_FLASH_SIZE        8*1024
#define AVR_ATMEGA88_FLASH_PAGE_SIZE   32*2
#define AVR_ATMEGA88_EEPROM_SIZE       512
#define AVR_ATMEGA88_SRAM_SIZE         1024

#define AVR_ATMEGA168_FLASH_SIZE       16*1024
#define AVR_ATMEGA168_FLASH_PAGE_SIZE  64*2
#define AVR_ATMEGA168_EEPROM_SIZE      512
#define AVR_ATMEGA168_SRAM_SIZE        1024
                                                                     //MOD006v
#define AVR_ATMEGA328P_FLASH_SIZE       32*1024
#define AVR_ATMEGA328P_FLASH_PAGE_SIZE  64*2
#define AVR_ATMEGA328P_EEPROM_SIZE      1024
#define AVR_ATMEGA328P_SRAM_SIZE        2048
                                                                     //MOD006^
#define AVR_ATMEGA16_FLASH_SIZE        16*1024
#define AVR_ATMEGA16_FLASH_PAGE_SIZE   64*2
#define AVR_ATMEGA16_EEPROM_SIZE       512
#define AVR_ATMEGA16_SRAM_SIZE         1024
                                                                     //MOD001v
#define AVR_ATMEGA165P_FLASH_SIZE      16*1024
#define AVR_ATMEGA165P_FLASH_PAGE_SIZE 64*2
#define AVR_ATMEGA165P_EEPROM_SIZE     512
#define AVR_ATMEGA165P_SRAM_SIZE       1024
                                                                     //MOD001^
                                                                     //MOD004v
#define AVR_ATMEGA325P_FLASH_SIZE      32*1024
#define AVR_ATMEGA325P_FLASH_PAGE_SIZE 64*2
#define AVR_ATMEGA325P_EEPROM_SIZE     1024
#define AVR_ATMEGA325P_SRAM_SIZE       2048
                                                                     //MOD004^


/*-----------------
 * Locks and fuses
 *-----------------
 */
                                                                     //MOD003v
#define AVR_ATMEGAX8_LOCK_BITS_MSK           0x3F
#define  AVR_ATMEGAX8_EXT_PROG_LOCK_MSK       0x03
#define  AVR_ATMEGAX8_EXT_PROG_ENABLED        0x03
#define  AVR_ATMEGAX8_EXT_PROG_DISABLED       0x02
#define  AVR_ATMEGAX8_EXT_PROG_VERY_DISABLED  0x00
#define  AVR_ATMEGAX8_INT_PROG_LOCK_MSK       0x3C

#define AVR_ATMEGAX8_FUSE_BITS_LO_MSK        0xFF
#define  AVR_ATMEGAX8_FUSE_CKDIV8             0x80
#define  AVR_ATMEGAX8_FUSE_CKOUT              0x40
#define  AVR_ATMEGAX8_FUSE_SUT_MSK            0x30
#define  AVR_ATMEGAX8_FUSE_SUT1               0x20
#define  AVR_ATMEGAX8_FUSE_SUT0               0x10
#define  AVR_ATMEGAX8_FUSE_CKSEL_MSK          0x0F
#define  AVR_ATMEGAX8_FUSE_CKSEL3             0x08
#define  AVR_ATMEGAX8_FUSE_CKSEL2             0x04
#define  AVR_ATMEGAX8_FUSE_CKSEL1             0x02
#define  AVR_ATMEGAX8_FUSE_CKSEL0             0x01
#define  AVR_ATMEGAX8_FUSE_CKSEL_EXT_LPC      0x08
#define  AVR_ATMEGAX8_FUSE_CKSEL_EXT_FSC      0x06
#define  AVR_ATMEGAX8_FUSE_CKSEL_EXT_LFC      0x04
#define  AVR_ATMEGAX8_FUSE_CKSEL_INT_128KHZ   0x03
#define  AVR_ATMEGAX8_FUSE_CKSEL_INT_CALIB    0x02
#define  AVR_ATMEGAX8_FUSE_CKSEL_EXT_CLK      0x00

#define AVR_ATMEGAX8_FUSE_BITS_HI_MSK        0xFF
#define  AVR_ATMEGAX8_FUSE_RSTDISBL           0x80
#define  AVR_ATMEGAX8_FUSE_DWEN               0x40
#define  AVR_ATMEGAX8_FUSE_SPIEN              0x20
#define  AVR_ATMEGAX8_FUSE_WDTON              0x10
#define  AVR_ATMEGAX8_FUSE_EESAVE             0x08
#define  AVR_ATMEGAX8_FUSE_BODLEVEL_MSK       0x07
#define  AVR_ATMEGAX8_FUSE_BODLEVEL2          0x04
#define  AVR_ATMEGAX8_FUSE_BODLEVEL1          0x02
#define  AVR_ATMEGAX8_FUSE_BODLEVEL0          0x01

#define AVR_ATMEGAX8_FUSE_BITS_EX_MSK        0xFF
#define  AVR_ATMEGA48_FUSE_SELFPRGEN          0x01    /* mega48     */
#define  AVR_ATMEGAX8_FUSE_BOOTSZ_MSK         0x06    /* mega88/168 */
#define  AVR_ATMEGAX8_FUSE_BOOTSZ1            0x04    /* mega88/168 */
#define  AVR_ATMEGAX8_FUSE_BOOTSZ0            0x02    /* mega88/168 */
#define  AVR_ATMEGAX8_FUSE_BOOTRST            0x01    /* mega88/168 */
                                                                     //MOD003^
                                                                     //MOD006v
#define AVR_ATMEGA328P_FUSE_BITS_HI_MSK      0xFF
#define  AVR_ATMEGA328P_FUSE_RSTDISBL         0x80
#define  AVR_ATMEGA328P_FUSE_DWEN             0x40
#define  AVR_ATMEGA328P_FUSE_SPIEN            0x20
#define  AVR_ATMEGA328P_FUSE_WDTON            0x10
#define  AVR_ATMEGA328P_FUSE_EESAVE           0x08
#define  AVR_ATMEGA328P_FUSE_BOOTSZ_MSK       0x06
#define  AVR_ATMEGA328P_FUSE_BOOTSZ1          0x04
#define  AVR_ATMEGA328P_FUSE_BOOTSZ0          0x02
#define  AVR_ATMEGA328P_FUSE_BOOTRST          0x01

#define AVR_ATMEGA328P_FUSE_BITS_EX_MSK      0xFF
#define  AVR_ATMEGA328P_FUSE_BODLEVEL_MSK     0x07
#define  AVR_ATMEGA328P_FUSE_BODLEVEL2        0x04
#define  AVR_ATMEGA328P_FUSE_BODLEVEL1        0x02
#define  AVR_ATMEGA328P_FUSE_BODLEVEL0        0x01
                                                                     //MOD006^
                                                                     //MOD001v
#define AVR_ATMEGA165P_LOCK_BITS_MSK           0x3F
#define  AVR_ATMEGA165P_EXT_PROG_LOCK_MSK       0x03
#define  AVR_ATMEGA165P_EXT_PROG_ENABLED        0x03
#define  AVR_ATMEGA165P_EXT_PROG_DISABLED       0x02
#define  AVR_ATMEGA165P_EXT_PROG_VERY_DISABLED  0x00
#define  AVR_ATMEGA165P_INT_PROG_LOCK_MSK       0x3C

#define AVR_ATMEGA165P_FUSE_BITS_LO_MSK        0xFF
#define  AVR_ATMEGA165P_FUSE_CKDIV8             0x80
#define  AVR_ATMEGA165P_FUSE_CKOUT              0x40
#define  AVR_ATMEGA165P_FUSE_SUT_MSK            0x30
#define  AVR_ATMEGA165P_FUSE_SUT1               0x20
#define  AVR_ATMEGA165P_FUSE_SUT0               0x10
#define  AVR_ATMEGA165P_FUSE_CKSEL_MSK          0x0F
#define  AVR_ATMEGA165P_FUSE_CKSEL3             0x08
#define  AVR_ATMEGA165P_FUSE_CKSEL2             0x04
#define  AVR_ATMEGA165P_FUSE_CKSEL1             0x02
#define  AVR_ATMEGA165P_FUSE_CKSEL0             0x01
#define  AVR_ATMEGA165P_FUSE_CKSEL_INT_CALIB    0x02

#define AVR_ATMEGA165P_FUSE_BITS_HI_MSK        0xFF
#define  AVR_ATMEGA165P_FUSE_OCDEN              0x80
#define  AVR_ATMEGA165P_FUSE_JTAGEN             0x40
#define  AVR_ATMEGA165P_FUSE_SPIEN              0x20
#define  AVR_ATMEGA165P_FUSE_WDTON              0x10
#define  AVR_ATMEGA165P_FUSE_EESAVE             0x08
#define  AVR_ATMEGA165P_FUSE_BOOTSZ_MSK         0x06
#define  AVR_ATMEGA165P_FUSE_BOOTSZ1            0x04
#define  AVR_ATMEGA165P_FUSE_BOOTSZ0            0x02
#define  AVR_ATMEGA165P_FUSE_BOOTRST            0x01

#define AVR_ATMEGA165P_FUSE_BITS_EX_MSK        0x0F
#define  AVR_ATMEGA165P_FUSE_BODLEVEL_MSK       0x0E
#define  AVR_ATMEGA165P_FUSE_BODLEVEL2          0x08
#define  AVR_ATMEGA165P_FUSE_BODLEVEL1          0x04
#define  AVR_ATMEGA165P_FUSE_BODLEVEL0          0x02
#define  AVR_ATMEGA165P_FUSE_RSTDISBL           0x01
                                                                     //MOD001^

                                                                     //MOD004v
#define AVR_ATMEGA325P_LOCK_BITS_MSK           0x3F
#define  AVR_ATMEGA325P_EXT_PROG_LOCK_MSK       0x03
#define  AVR_ATMEGA325P_EXT_PROG_ENABLED        0x03
#define  AVR_ATMEGA325P_EXT_PROG_DISABLED       0x02
#define  AVR_ATMEGA325P_EXT_PROG_VERY_DISABLED  0x00
#define  AVR_ATMEGA325P_INT_PROG_LOCK_MSK       0x3C

#define AVR_ATMEGA325P_FUSE_BITS_LO_MSK        0xFF
#define  AVR_ATMEGA325P_FUSE_CKDIV8             0x80
#define  AVR_ATMEGA325P_FUSE_CKOUT              0x40
#define  AVR_ATMEGA325P_FUSE_SUT_MSK            0x30
#define  AVR_ATMEGA325P_FUSE_SUT1               0x20
#define  AVR_ATMEGA325P_FUSE_SUT0               0x10
#define  AVR_ATMEGA325P_FUSE_CKSEL_MSK          0x0F
#define  AVR_ATMEGA325P_FUSE_CKSEL3             0x08
#define  AVR_ATMEGA325P_FUSE_CKSEL2             0x04
#define  AVR_ATMEGA325P_FUSE_CKSEL1             0x02
#define  AVR_ATMEGA325P_FUSE_CKSEL0             0x01
#define  AVR_ATMEGA325P_FUSE_CKSEL_INT_CALIB    0x02

#define AVR_ATMEGA325P_FUSE_BITS_HI_MSK        0xFF
#define  AVR_ATMEGA325P_FUSE_OCDEN              0x80
#define  AVR_ATMEGA325P_FUSE_JTAGEN             0x40
#define  AVR_ATMEGA325P_FUSE_SPIEN              0x20
#define  AVR_ATMEGA325P_FUSE_WDTON              0x10
#define  AVR_ATMEGA325P_FUSE_EESAVE             0x08
#define  AVR_ATMEGA325P_FUSE_BOOTSZ_MSK         0x06
#define  AVR_ATMEGA325P_FUSE_BOOTSZ1            0x04
#define  AVR_ATMEGA325P_FUSE_BOOTSZ0            0x02
#define  AVR_ATMEGA325P_FUSE_BOOTRST            0x01

#define AVR_ATMEGA325P_FUSE_BITS_EX_MSK        0x07                  //MOD005
#define  AVR_ATMEGA325P_FUSE_BODLEVEL_MSK       0x06                 //MOD005
#define  AVR_ATMEGA325P_FUSE_BODLEVEL1          0x04
#define  AVR_ATMEGA325P_FUSE_BODLEVEL0          0x02
#define  AVR_ATMEGA325P_FUSE_RSTDISBL           0x01
                                                                     //MOD004^
