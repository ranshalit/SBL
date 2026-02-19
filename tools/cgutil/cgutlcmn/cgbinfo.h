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
 * $Header:   S:/CG/archives/CGTOOLS/INC/CGBINFO.H-arc   1.5   Sep 06 2016 15:46:38   congatec  $
 *
 * Contents: CGEB system BIOS constants and structures.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/INC/CGBINFO.H-arc  $
 * 
 *    Rev 1.5   Sep 06 2016 15:46:38   congatec
 * Added BSD header.
 * 
 *    Rev 1.4   May 31 2012 15:45:34   gartner
 * Updated variable definition to ease 64bit porting.
 * 
 *    Rev 1.3   Dec 06 2010 12:00:54   gartner
 * MOD003: Added extended compatibility ID to BIOS info structure.
 * 
 *    Rev 1.2   Feb 15 2010 14:32:44   gartner
 * MOD002: Added BIOS type AMI EFI.
 * 
 *    Rev 1.1   Jan 27 2006 12:36:16   gartner
 * MOD001: Added BIOS (vendor) type to the BIOS info structure.
 * 
 *    Rev 1.0   Sep 30 2005 16:29:08   gartner
 * Initial revision.
 *
 *---------------------------------------------------------------------------
 */

#ifndef _INC_CGBINFO

#pragma pack (1)

//+---------------------------------------------------------------------------
//
//       BIOS system information structure
//
#define CG_SYS_BIOS_INFO_ID_L   0x42474324      // $CGBIOS$
#define CG_SYS_BIOS_INFO_ID_H   0x24534F49
#define CG_BIOS_AMI             0x01            // BIOS vendor ID AMIBIOS        
#define CG_BIOS_XPRESS          0x02            // BIOS vendor ID XPressROM
#define CG_EFI_AMI				0x03            // BIOS vendor ID AMI EFI

typedef struct {
    UINT32			infoIDLow;      // BIOS info low ID
    UINT32			infoIDHigh;     // BIOS info high ID
    unsigned char	infoLen;        // BIOS info struct. total length in bytes
    unsigned char   biosVersion[8]; // 8 bytes for BIOS version.  
    UINT16			cmosSize;       // System CMOS size.
    unsigned char   biosType;       // BIOS vendor ID
	UINT16			biosExtCompID;	// Extended BIOS compatibility ID		// MOD003       
} CG_BIOS_INFO;


//+---------------------------------------------------------------------------
//
//       Watchdog POST configuration information structure
//
#define CG_WD_CONF_ID_L         0x57474324      // $CGWDCI$
#define CG_WD_CONF_ID_H         0x24494344          
   
//
// Watchdog modes
//
#define CG_WDOG_MODE_REBOOT_PC        EQU     0       //Reset
#define CG_WDOG_MODE_RESTART_OS       EQU     1       //NMI
#define CG_WDOG_MODE_STAGED           EQU     080h    //Check stages

//
// Watchdog operating modes
//
#define CG_WDOG_OPMODE_DISABLED         0       // Watchdog disabled
#define CG_WDOG_OPMODE_ONETIME_TRIG     1       // One time trigger (WD disabled after first trigger)
#define CG_WDOG_OPMODE_SINGLE_EVENT     2       // Single event; stages only processed once
#define CG_WDOG_OPMODE_EVENT_REPEAT     3       // Repeated event; stage processing re-started after last stage

//
// Watchdog events
//
#define CG_WDOG_EVENT_INT       0       // NMI/IRQ
#define CG_WDOG_EVENT_SCI       1       // SMI/SCI
#define CG_WDOG_EVENT_RST       2       // system reset
#define CG_WDOG_EVENT_BTN       3       // power button

typedef struct {
    UINT32       infoIDLow;      // WD config. info low ID
    UINT32       infoIDHigh;     // WD config. info high ID
    UINT32       configSize;     // Size of this structure in bytes
    UINT32       timeout;        // Timeout (not used in staged mode)
    UINT32       delay;          // Delay
    UINT32       mode;           // Watchdog mode
    UINT32       opMode;         // Watchdog operating mode
    UINT32       stageCount;     // Number of stages to use
    UINT32       timeout1;       // Timeout for this stage
    UINT32       event1;         // Event for this stage
    UINT32       timeout2;       // Timeout for this stage
    UINT32       event2;         // Event for this stage
    UINT32       timeout3;       // Timeout for this stage
    UINT32       event3;         // Event for this stage
} CG_WD_CONF_INFO;

//+---------------------------------------------------------------------------
//
//       VGA POST configuration information structure
//
#define CG_VGA_CONF_ID_L        0x56474324      // $CGVGAI$
#define CG_VGA_CONF_ID_H        0x24494147          
  
//
// Display mode definitions
//
#define CG_DISP_DISABLED        0
#define CG_DISP_CRTONLY         1
#define CG_DISP_TVONLY          2
#define CG_DISP_CRT_TV          3
#define CG_DISP_EFP             4
#define CG_DISP_CRT_EFP         5
#define CG_DISP_EFP_TV          6
#define CG_DISP_RES1            7       // Reserved for future use.		
#define CG_DISP_LCDONLY         8
#define CG_DISP_CRT_LCD         9
#define CG_DISP_TV_LCD          10


typedef struct {
        UINT32			infoIDLow;      // VGA info low ID
        UINT32			infoIDHigh;     // VGA info high ID
        unsigned char   configSize;     // Size of this structure in bytes
        unsigned char   vgaType;        // VGA type(s)
        unsigned char   dispMode;       // display mode
        unsigned char   requestedBpp;   // req. color depth (bit used for color)                 
        UINT16			requestedWidth; // req. resolution width       
        UINT16			requestedHeight;// req. resolution height
        UINT16			nativeWidth;    // native panel width
        UINT16			nativeHeight;   // native panel height
} CG_VGA_INFO;


#pragma pack()

#define _INC_CGBINFO
#endif 

