/* ----------------------------------------------------------------------------
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
 * ------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
 *
 * Contents: MEC170x microcontroller declarations for AVR serial programming
 *           mode emulation
 *
 * ----------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * MOD002: Added AVR microcontroller signatures definitions for all MEC170x
 *         variants in the 144 pin WFBGA package and corrected SRAM size of
 *         MEC170xQ types.
 *
 * MOD001: Added support for MEC1706.
 *
 * ------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------------
 * MEC170x microcontroller signatures
 *
 * NOTE!  The following definitions assign microcontroller signatures to the
 * MEC170x microcontrollers.  These signatures don't exist in hardware but are
 * reported by the BC firmware while emulating the AVR serial programming mode
 * (SPM).  They are mainly used by the congatec system utility CGUTIL to be
 * able to distinguish the different congatec Board Controller variants.
 *
 * NOTE!  The following signatures should not conflict with real existing AVR
 * microcontroller signatures.  It seems that Atmel uses the AVR SPM signature
 * byte 0 to report their manufacturer ID 0x1E.  In case of the MEC170x the
 * value 0x45 has been chosen for signature byte 0 which is the common value of
 * bits 7-0 of the MEC170x boundary scan JTAG IDs.
 * ------------------------------------------------------------------------- */

#define MC17_MFG_ID_MSK         0x0000FF               /* manufacturer ID    */
#define MC17_MFG_ID_MICROCHIP   0x000045

#define MC17_FAMILY_MSK         0x00FF00               /* MEC170x family ID  */
#define MC17_FAMILY_1701        0x001B00                   /* MEC1701x-C#-xx */
#define MC17_FAMILY_1703        0x001C00                   /* MEC1703x-C#-xx */
#define MC17_FAMILY_1705        0x002000                   /* MEC1705x-C#-xx */
#define MC17_FAMILY_1706        0x002100                   /* MEC1706x-C#-xx */ /* MOD001 */
#define MC17_FAMILY_1704        0x002200                   /* MEC1704x-C#-xx */ /* MOD002 */

#define MC17_SRAM_SIZE_MSK      0x0F0000               /* MEC170x SRAM size  */
#define MC17_SRAM_SIZE_256KB    0x010000                   /* MECxxxxH-C#-xx */
#define MC17_SRAM_SIZE_320KB    0x020000                   /* MECxxxxK-C#-xx */
#define MC17_SRAM_SIZE_480KB    0x030000                   /* MECxxxxQ-C#-xx */

#define MC17_PACKAGE_MSK        0xF00000               /* MEC170x package ID */
#define MC17_PACKAGE_144_WFBGA  0x100000                   /* MECxxxxx-C#-SZ */
#define MC17_PACKAGE_169_WFBGA  0x200000                   /* MECxxxxx-C#-TN */
#define MC17_PACKAGE_169_XFBGA  0x300000                   /* MECxxxxx-C#-9S */
                                                                   /* MOD002 */
                                                                   /*      v */
#define MEC1701HSZ_SIGNATURE (  MC17_MFG_ID_MICROCHIP \
                              | MC17_FAMILY_1701      \
                              | MC17_SRAM_SIZE_256KB  \
                              | MC17_PACKAGE_144_WFBGA)

#define MEC1701KSZ_SIGNATURE (  MC17_MFG_ID_MICROCHIP \
                              | MC17_FAMILY_1701      \
                              | MC17_SRAM_SIZE_320KB  \
                              | MC17_PACKAGE_144_WFBGA)

#define MEC1701QSZ_SIGNATURE (  MC17_MFG_ID_MICROCHIP \
                              | MC17_FAMILY_1701      \
                              | MC17_SRAM_SIZE_480KB  \
                              | MC17_PACKAGE_144_WFBGA)

#define MEC1703HSZ_SIGNATURE (  MC17_MFG_ID_MICROCHIP \
                              | MC17_FAMILY_1703      \
                              | MC17_SRAM_SIZE_256KB  \
                              | MC17_PACKAGE_144_WFBGA)

#define MEC1703KSZ_SIGNATURE (  MC17_MFG_ID_MICROCHIP \
                              | MC17_FAMILY_1703      \
                              | MC17_SRAM_SIZE_320KB  \
                              | MC17_PACKAGE_144_WFBGA)

#define MEC1703QSZ_SIGNATURE (  MC17_MFG_ID_MICROCHIP \
                              | MC17_FAMILY_1703      \
                              | MC17_SRAM_SIZE_480KB  \
                              | MC17_PACKAGE_144_WFBGA)

#define MEC1704HSZ_SIGNATURE (  MC17_MFG_ID_MICROCHIP \
                              | MC17_FAMILY_1704      \
                              | MC17_SRAM_SIZE_256KB  \
                              | MC17_PACKAGE_144_WFBGA)

#define MEC1704KSZ_SIGNATURE (  MC17_MFG_ID_MICROCHIP \
                              | MC17_FAMILY_1704      \
                              | MC17_SRAM_SIZE_320KB  \
                              | MC17_PACKAGE_144_WFBGA)

#define MEC1704QSZ_SIGNATURE (  MC17_MFG_ID_MICROCHIP \
                              | MC17_FAMILY_1704      \
                              | MC17_SRAM_SIZE_480KB  \
                              | MC17_PACKAGE_144_WFBGA)

#define MEC1705HSZ_SIGNATURE (  MC17_MFG_ID_MICROCHIP \
                              | MC17_FAMILY_1705      \
                              | MC17_SRAM_SIZE_256KB  \
                              | MC17_PACKAGE_144_WFBGA)

#define MEC1705KSZ_SIGNATURE (  MC17_MFG_ID_MICROCHIP \
                              | MC17_FAMILY_1705      \
                              | MC17_SRAM_SIZE_320KB  \
                              | MC17_PACKAGE_144_WFBGA)
                                                                   /*      ^ */
                                                                   /* MOD002 */
#define MEC1705QSZ_SIGNATURE (  MC17_MFG_ID_MICROCHIP \
                              | MC17_FAMILY_1705      \
                              | MC17_SRAM_SIZE_480KB  \
                              | MC17_PACKAGE_144_WFBGA)
                                                                   /* MOD002 */
                                                                   /*      v */
#define MEC1706HSZ_SIGNATURE (  MC17_MFG_ID_MICROCHIP \
                              | MC17_FAMILY_1706      \
                              | MC17_SRAM_SIZE_256KB  \
                              | MC17_PACKAGE_144_WFBGA)

#define MEC1706KSZ_SIGNATURE (  MC17_MFG_ID_MICROCHIP \
                              | MC17_FAMILY_1706      \
                              | MC17_SRAM_SIZE_320KB  \
                              | MC17_PACKAGE_144_WFBGA)
                                                                   /*      ^ */
                                                                   /* MOD002 */
                                                                   /* MOD001 */
                                                                   /*      v */
#define MEC1706QSZ_SIGNATURE (  MC17_MFG_ID_MICROCHIP \
                              | MC17_FAMILY_1706      \
                              | MC17_SRAM_SIZE_480KB  \
                              | MC17_PACKAGE_144_WFBGA)
                                                                   /*      ^ */
                                                                   /* MOD001 */
/* --------------------------------------------
 * External SPI flash, EEPROM and SRAM sizes
 * ----------------------------------------- */
                                                                   /* MOD001 */
                                                                   /*      v */
#define MEC1706QSZ_FLASH_SIZE       (2*1024*1024)
#define MEC1706QSZ_FLASH_BLOCK_SIZE (4*1024)
#define MEC1706QSZ_FLASH_PAGE_SIZE  256
#define MEC1706QSZ_WRP_GRANULARITY  (4*1024)
#define MEC1706QSZ_EEPROM_SIZE      (2*1024)
#define MEC1706QSZ_SRAM_SIZE        (480*1024)                     /* MOD002 */
                                                                   /*      ^ */
                                                                   /* MOD001 */
                                                                   /* MOD002 */
                                                                   /*      v */
#define MEC1706KSZ_FLASH_SIZE       (2*1024*1024)
#define MEC1706KSZ_FLASH_BLOCK_SIZE (4*1024)
#define MEC1706KSZ_FLASH_PAGE_SIZE  256
#define MEC1706KSZ_WRP_GRANULARITY  (4*1024)
#define MEC1706KSZ_EEPROM_SIZE      (2*1024)
#define MEC1706KSZ_SRAM_SIZE        (320*1024)

#define MEC1706HSZ_FLASH_SIZE       (2*1024*1024)
#define MEC1706HSZ_FLASH_BLOCK_SIZE (4*1024)
#define MEC1706HSZ_FLASH_PAGE_SIZE  256
#define MEC1706HSZ_WRP_GRANULARITY  (4*1024)
#define MEC1706HSZ_EEPROM_SIZE      (2*1024)
#define MEC1706HSZ_SRAM_SIZE        (256*1024)
                                                                   /*      ^ */
                                                                   /* MOD002 */
#define MEC1705QSZ_FLASH_SIZE       (2*1024*1024)
#define MEC1705QSZ_FLASH_BLOCK_SIZE (4*1024)
#define MEC1705QSZ_FLASH_PAGE_SIZE  256
#define MEC1705QSZ_WRP_GRANULARITY  (4*1024)
#define MEC1705QSZ_EEPROM_SIZE      (2*1024)
#define MEC1705QSZ_SRAM_SIZE        (480*1024)                     /* MOD002 */
                                                                   /* MOD002 */
                                                                   /*      v */
#define MEC1705KSZ_FLASH_SIZE       (2*1024*1024)
#define MEC1705KSZ_FLASH_BLOCK_SIZE (4*1024)
#define MEC1705KSZ_FLASH_PAGE_SIZE  256
#define MEC1705KSZ_WRP_GRANULARITY  (4*1024)
#define MEC1705KSZ_EEPROM_SIZE      (2*1024)
#define MEC1705KSZ_SRAM_SIZE        (320*1024)

#define MEC1705HSZ_FLASH_SIZE       (2*1024*1024)
#define MEC1705HSZ_FLASH_BLOCK_SIZE (4*1024)
#define MEC1705HSZ_FLASH_PAGE_SIZE  256
#define MEC1705HSZ_WRP_GRANULARITY  (4*1024)
#define MEC1705HSZ_EEPROM_SIZE      (2*1024)
#define MEC1705HSZ_SRAM_SIZE        (256*1024)

#define MEC1704QSZ_FLASH_SIZE       (2*1024*1024)
#define MEC1704QSZ_FLASH_BLOCK_SIZE (4*1024)
#define MEC1704QSZ_FLASH_PAGE_SIZE  256
#define MEC1704QSZ_WRP_GRANULARITY  (4*1024)
#define MEC1704QSZ_EEPROM_SIZE      (2*1024) /* emulated in flash */
#define MEC1704QSZ_SRAM_SIZE        (480*1024)

#define MEC1704KSZ_FLASH_SIZE       (2*1024*1024)
#define MEC1704KSZ_FLASH_BLOCK_SIZE (4*1024)
#define MEC1704KSZ_FLASH_PAGE_SIZE  256
#define MEC1704KSZ_WRP_GRANULARITY  (4*1024)
#define MEC1704KSZ_EEPROM_SIZE      (2*1024) /* emulated in flash */
#define MEC1704KSZ_SRAM_SIZE        (320*1024)

#define MEC1704HSZ_FLASH_SIZE       (2*1024*1024)
#define MEC1704HSZ_FLASH_BLOCK_SIZE (4*1024)
#define MEC1704HSZ_FLASH_PAGE_SIZE  256
#define MEC1704HSZ_WRP_GRANULARITY  (4*1024)
#define MEC1704HSZ_EEPROM_SIZE      (2*1024) /* emulated in flash */
#define MEC1704HSZ_SRAM_SIZE        (256*1024)

#define MEC1703QSZ_FLASH_SIZE       (2*1024*1024)
#define MEC1703QSZ_FLASH_BLOCK_SIZE (4*1024)
#define MEC1703QSZ_FLASH_PAGE_SIZE  256
#define MEC1703QSZ_WRP_GRANULARITY  (4*1024)
#define MEC1703QSZ_EEPROM_SIZE      (2*1024)
#define MEC1703QSZ_SRAM_SIZE        (480*1024)

#define MEC1703KSZ_FLASH_SIZE       (2*1024*1024)
#define MEC1703KSZ_FLASH_BLOCK_SIZE (4*1024)
#define MEC1703KSZ_FLASH_PAGE_SIZE  256
#define MEC1703KSZ_WRP_GRANULARITY  (4*1024)
#define MEC1703KSZ_EEPROM_SIZE      (2*1024)
#define MEC1703KSZ_SRAM_SIZE        (320*1024)

#define MEC1703HSZ_FLASH_SIZE       (2*1024*1024)
#define MEC1703HSZ_FLASH_BLOCK_SIZE (4*1024)
#define MEC1703HSZ_FLASH_PAGE_SIZE  256
#define MEC1703HSZ_WRP_GRANULARITY  (4*1024)
#define MEC1703HSZ_EEPROM_SIZE      (2*1024)
#define MEC1703HSZ_SRAM_SIZE        (256*1024)

#define MEC1701QSZ_FLASH_SIZE       (2*1024*1024)
#define MEC1701QSZ_FLASH_BLOCK_SIZE (4*1024)
#define MEC1701QSZ_FLASH_PAGE_SIZE  256
#define MEC1701QSZ_WRP_GRANULARITY  (4*1024)
#define MEC1701QSZ_EEPROM_SIZE      (2*1024) /* emulated in flash */
#define MEC1701QSZ_SRAM_SIZE        (480*1024)

#define MEC1701KSZ_FLASH_SIZE       (2*1024*1024)
#define MEC1701KSZ_FLASH_BLOCK_SIZE (4*1024)
#define MEC1701KSZ_FLASH_PAGE_SIZE  256
#define MEC1701KSZ_WRP_GRANULARITY  (4*1024)
#define MEC1701KSZ_EEPROM_SIZE      (2*1024) /* emulated in flash */
#define MEC1701KSZ_SRAM_SIZE        (320*1024)

#define MEC1701HSZ_FLASH_SIZE       (2*1024*1024)
#define MEC1701HSZ_FLASH_BLOCK_SIZE (4*1024)
#define MEC1701HSZ_FLASH_PAGE_SIZE  256
#define MEC1701HSZ_WRP_GRANULARITY  (4*1024)
#define MEC1701HSZ_EEPROM_SIZE      (2*1024) /* emulated in flash */
#define MEC1701HSZ_SRAM_SIZE        (256*1024)
                                                                   /*      ^ */
                                                                   /* MOD002 */
/* ------------------
 * Locks and fuses
 * --------------- */

#define MC17_LOCK_BITS_MSK      0x00    /* AVR lock byte not supported.      */
#define MC17_FUSE_BITS_LO_MSK   0x00    /* AVR low fuse byte not supported.  */
#define MC17_FUSE_BITS_HI_MSK   0x01    /* AVR high fuse byte faked in XRAM. */
#define  MC17_FUSE_EESAVE        0x01
#define MC17_FUSE_BITS_EX_MSK   0x00    /* AVR ext. fuse byte not supported. */

/* -----------------
 * Default values
 * -------------- */

#define AVR_SIGNATURE_BYTE_0     ((MEC1705QSZ_SIGNATURE >>  0) & 0xFF)
#define AVR_SIGNATURE_BYTE_1     ((MEC1705QSZ_SIGNATURE >>  8) & 0xFF)
#define AVR_SIGNATURE_BYTE_2     ((MEC1705QSZ_SIGNATURE >> 16) & 0xFF)
#define MHAL_MCU_FLASH_PAGE_SIZE MEC1705QSZ_FLASH_PAGE_SIZE
