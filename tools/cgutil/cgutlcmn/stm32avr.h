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
 * $Header:   S:/CG/archives/CGTOOLS/INC/stm32avr.h-arc   1.2   Sep 06 2016 15:53:14   congatec  $
 *
 * Contents: STM32 declarations for AVR ISP emulation
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/INC/stm32avr.h-arc  $
 * 
 *    Rev 1.2   Sep 06 2016 15:53:14   congatec
 * Added BSD header.
 * 
 *    Rev 1.1   May 31 2012 15:45:54   gartner
 * Updated variable definition to ease 64bit porting.
 * 
 *    Rev 1.0   Nov 11 2010 10:17:08   ess
 * Initial revision.
 *
 *---------------------------------------------------------------------------
 */



/*---------------------------------------------------------------------------
 * STM32 microcontroller signatures
 *
 * NOTE! The following definitions assign microcontroller signatures to STM32
 * microcontrollers. These signatures don't exist in hardware. They are used
 * by firmware and software to be able to distinguish the different STM32
 * controllers. The STM32 bootloader code which emulates the AVR in-system
 * programming (ISP) reports these signatures.
 *
 * NOTE! The following signatures should not conflict with real existing AVR
 * microcontroller signatures. It seems that Atmel uses signature byte 0 to
 * report their manufacturer ID 0x1E. Therefore the following definitions set
 * the signature byte 0 to ST's manufacturer ID 0x20.
 *---------------------------------------------------------------------------
 */

#define STM32_MFG_ID_MSK         0x0000FF       /* manufacturer ID          */
#define STM32_MFG_ID_ST          0x000020

#define STM32_FAMILY_MSK         0x000F00       /* STM32 family ID          */
#define STM32_FAMILY_F           0x000000                    /* STM32Fxxxxx */
#define STM32_FAMILY_L           0x000100                    /* STM32Lxxxxx */
#define STM32_FAMILY_W           0x000200                    /* STM32Wxxxxx */

#define STM32_LINE_MSK           0x00F000       /* STM32Fxxxxx product line */
#define STM32_VALUE_LINE         0x000000                    /* STM32F100xx */
#define STM32_ACCESS_LINE        0x001000                    /* STM32F101xx */
#define STM32_USB_ACCESS_LINE    0x002000                    /* STM32F102xx */
#define STM32_PERFORMANCE_LINE   0x003000                    /* STM32F103xx */
#define STM32_CONNECTIVITY5_LINE 0x005000                    /* STM32F105xx */
#define STM32_CONNECTIVITY7_LINE 0x007000                    /* STM32F107xx */

#define STM32_PINS_MSK           0x0F0000       /* STM32 pin count          */
#define STM32_PINS_36            0x010000                    /* STM32xxxxTx */
#define STM32_PINS_48            0x020000                    /* STM32xxxxCx */
#define STM32_PINS_64            0x030000                    /* STM32xxxxRx */
#define STM32_PINS_100           0x040000                    /* STM32xxxxVx */
#define STM32_PINS_144           0x050000                    /* STM32xxxxZx */

#define STM32_FLASH_SIZE_MSK     0xF00000       /* STM32 flash size         */
#define STM32_FLASH_SIZE_16KB    0x000000                    /* STM32xxxxx4 */
#define STM32_FLASH_SIZE_32KB    0x100000                    /* STM32xxxxx6 */
#define STM32_FLASH_SIZE_64KB    0x200000                    /* STM32xxxxx8 */
#define STM32_FLASH_SIZE_128KB   0x300000                    /* STM32xxxxxB */
#define STM32_FLASH_SIZE_256KB   0x400000                    /* STM32xxxxxC */
#define STM32_FLASH_SIZE_384KB   0x500000                    /* STM32xxxxxD */
#define STM32_FLASH_SIZE_512KB   0x600000                    /* STM32xxxxxE */
#define STM32_FLASH_SIZE_768KB   0x700000                    /* STM32xxxxxF */
#define STM32_FLASH_SIZE_1MB     0x800000                    /* STM32xxxxxG */

#define STM32F100R8_SIGNATURE    (STM32_MFG_ID_ST | \
                                  STM32_FAMILY_F  | STM32_VALUE_LINE | \
                                  STM32_PINS_64   | STM32_FLASH_SIZE_64KB)



/*------------------------------
 * Flash, EEPROM and SRAM sizes
 *------------------------------
 */

#define STM32F100R8_FLASH_SIZE      (64*1024)
#define STM32F100R8_FLASH_PAGE_SIZE 1024
#define STM32F100R8_WRP_GRANULARITY (4*1024)
#define STM32F100R8_EEPROM_SIZE     256         /* EEPROM emulated in flash */
#define STM32F100R8_SRAM_SIZE       (8*1024)



/*-----------------
 * Locks and fuses
 *-----------------
 */

/*
 * On STM32 microcontrollers the locking of flash memory is done by the read
 * protection option byte (RDP) and the four flash memory write protection
 * option bytes (WRP0-WRP3). But there is a limitation compared to AVR
 * microcontrollers. On STM32 microcontrollers the write protection inhibits
 * both the in-system programming and the in-application programming. On AVR
 * microcontrollers this can be controlled independent of each other. Another
 * difference is the read protection. On AVR microcontrollers an active read
 * protection automatically implies the ISP write-protection while on STM32
 * the read protection and the write protection can be controlled independent
 * of each other. On STM32 microcontrollers an active read protection implies
 * a write protection for the first flash block only.
 */
#define STM32_LOCK_BITS_MSK           0x03
#define  STM32_EXT_PROG_LOCK_MSK       0x03
#define  STM32_EXT_PROG_MSK            0x01
#define  STM32_EXT_PROG_ENABLED        0x01
#define  STM32_EXT_PROG_DISABLED       0x00
#define  STM32_EXT_READ_MSK            0x02
#define  STM32_EXT_READ_ENABLED        0x02
#define  STM32_EXT_READ_DISABLED       0x00

/*
 * The AVR low fuse byte is mapped to the STM32 USER option byte.
 */
#define STM32_FUSE_BITS_LO_MSK        0x07      /* USER option byte */
#define  STM32_FUSE_RST_STDBY          0x04
#define  STM32_FUSE_RST_STOP           0x02
#define  STM32_FUSE_WDG_HW             0x01

/*
 * The AVR high fuse byte is mapped to the STM32 USER data byte 0. When the
 * EESAVE fuse is programmed to 0, then the EEPROM emulation section is
 * preserved during an AVR chip erase ISP command.
 */
#define STM32_FUSE_BITS_HI_MSK        0x01
#define  STM32_FUSE_EESAVE             0x01

/*
 * The AVR high fuse byte is mapped to the STM32 USER data byte 1. All bits
 * are reserved.
 */
#define STM32_FUSE_BITS_EX_MSK        0x00
