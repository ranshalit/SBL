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

/*-----------------------------------------------------------------------------
 *
 * $Header:   S:/CG/archives/CGTOOLS/INC/tivaavr.h-arc   1.2   Sep 06 2016 15:53:38   congatec  $
 *
 * Contents: Tiva microcontroller declarations for AVR serial programming mode
 *           emulation
 *
 *-----------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/INC/tivaavr.h-arc  $
 * 
 *    Rev 1.2   Sep 06 2016 15:53:38   congatec
 * Added BSD header.
 * 
 *    Rev 1.1   Jan 15 2014 16:55:20   ess
 * MOD001: change to fit to the microcontroller selection format of TI libraries
 * 
 *    Rev 1.0   Jul 03 2013 09:43:48   ess
 * Initial revision.
 *
 *---------------------------------------------------------------------------
 */
/*-----------------------------------------------------------------------------
 * Tiva microcontroller signatures
 *
 * NOTE! The following definitions assign microcontroller signatures to Tiva
 * microcontrollers. These signatures don't exist in hardware. They are used
 * by firmware and software to be able to distinguish the different Tiva
 * controllers. The BC firmware code which emulates the AVR serial programming
 * mode (SPM) reports these signatures.
 *
 * NOTE! The following signatures should not conflict with real existing AVR
 * microcontroller signatures. It seems that Atmel uses signature byte 0 to
 * report their manufacturer ID 0x1E. Therefore the following definitions set
 * the signature byte 0 to 0x3B. These are the lower eight bits of TI's
 * manufacturer ID 0x23B which is returned by the JTAG IDCODE data register.
 *-----------------------------------------------------------------------------
 */
#define TIVA_MFG_ID_MSK         0x0000FF                  /* manufacturer ID */
#define TIVA_MFG_ID_TI          0x00003B

#define TIVA_FAMILY_MSK         0x000F00                  /* Tiva family ID  */
#define TIVA_FAMILY_1           0x000100                    /* TM4x1xxxxxxxx */

#define TIVA_SERIES_MSK         0x00F000                  /* Tiva series ID  */
#define TIVA_CONNECTED_MCUS     0x00C000                    /* TM4Cxxxxxxxxx */
#define TIVA_EMBEDDED_CTRL      0x00E000                    /* TM4Exxxxxxxxx */

#define TIVA_PACKAGE_MSK        0x0F0000                  /* Tiva package ID */
#define TIVA_PACKAGE_64_LQFP    0x030000                    /* TM4xxxxxxxPM  */
#define TIVA_PACKAGE_100_LQFP   0x040000                    /* TM4xxxxxxxPZ  */
#define TIVA_PACKAGE_144_LQFP   0x050000                    /* TM4xxxxxxxPGE */
#define TIVA_PACKAGE_157_BGA    0x060000                    /* TM4xxxxxxxZRB */

#define TIVA_FLASH_SIZE_MSK     0xF00000                  /* Tiva flash size */
#define TIVA_FLASH_SIZE_32KB    0x100000                    /* TM4xxxxxCxxxx */
#define TIVA_FLASH_SIZE_64KB    0x200000                    /* TM4xxxxxDxxxx */
#define TIVA_FLASH_SIZE_128KB   0x300000                    /* TM4xxxxxExxxx */
#define TIVA_FLASH_SIZE_256KB   0x400000                    /* TM4xxxxxHxxxx */

#define TM4E1231H6ZRB_SIGNATURE (TIVA_MFG_ID_TI       |                       \
                                 TIVA_FAMILY_1        | TIVA_EMBEDDED_CTRL |  \
                                 TIVA_PACKAGE_157_BGA | TIVA_FLASH_SIZE_256KB)

#ifdef PART_TM4E1231H6ZRB                                          /* MOD001 */
#define AVR_SIGNATURE_BYTE_0 ((TM4E1231H6ZRB_SIGNATURE >>  0) &0xFF)
#define AVR_SIGNATURE_BYTE_1 ((TM4E1231H6ZRB_SIGNATURE >>  8) &0xFF)
#define AVR_SIGNATURE_BYTE_2 ((TM4E1231H6ZRB_SIGNATURE >> 16) &0xFF)
#endif /* PART_TM4E1231H6ZRB */                                    /* MOD001 */

/*------------------------------
 * Flash, EEPROM and SRAM sizes
 *------------------------------
 */
#define TM4E1231H6ZRB_FLASH_SIZE       (256*1024)
#define TM4E1231H6ZRB_FLASH_BLOCK_SIZE 1024
#define TM4E1231H6ZRB_FLASH_PAGE_SIZE  (32*4)
#define TM4E1231H6ZRB_WRP_GRANULARITY  (2*1024)
#define TM4E1231H6ZRB_EEPROM_SIZE      256                /* emulated EEPROM */
#define TM4E1231H6ZRB_SRAM_SIZE        (32*1024)

/*-----------------
 * Locks and fuses
 *-----------------
 */
#define TIVA_LOCK_BITS_MSK      0x00    /* AVR lock byte not supported.      */
#define TIVA_FUSE_BITS_LO_MSK   0x00    /* AVR low fuse byte not supported.  */
#define TIVA_FUSE_BITS_HI_MSK   0x01    /* AVR high fuse byte faked in GRAM. */
#define  TIVA_FUSE_EESAVE        0x01
#define TIVA_FUSE_BITS_EX_MSK   0x00    /* AVR ext. fuse byte not supported. */
