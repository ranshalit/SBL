/*---------------------------------------------------------------------------
 *
 * Copyright (c) 2016, congatec AG. All rights reserved.
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
 * $Header:   S:/CG/archives/CGTOOLS/INC/CGEPI.H-arc   1.4   Sep 06 2016 15:50:38   congatec  $
 *
 * Contents: EDID and other common panel data definitions.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/INC/CGEPI.H-arc  $
 * 
 *    Rev 1.4   Sep 06 2016 15:50:38   congatec
 * Added BSD header.
 * 
 *    Rev 1.3   May 31 2012 15:45:36   gartner
 * Updated variable definition to ease 64bit porting.
 * 
 *    Rev 1.2   Jul 02 2007 13:00:40   gartner
 * Fixed and updated EPI data set definitions.
 * 
 *    Rev 1.1   Oct 30 2006 15:16:16   gartner
 * Added EPI dataset extension definition. Extended EDID1.3 data set header definition.
 * 
 *    Rev 1.0   Sep 30 2005 16:29:08   gartner
 * Initial revision.
 *
 *---------------------------------------------------------------------------
 */

#ifndef _INC_EPI_EDID

#pragma pack (1)

#define CG_EPDA_MAX_BLOCK_ID    0x1F	


#define CG_EPDA_EEPROM_ID_FILE_HDR_ID_L 0x44504524      //$EPDAID$
#define CG_EPDA_EEPROM_ID_FILE_HDR_ID_H 0x24444941

#define CG_EPDA_USER_AREA_START_L       0x44504524      // $EPDAUA$ (EPDA User Area)
#define CG_EPDA_USER_AREA_START_H       0x24415541  
  
//----------------------------------------------------------------------------
//
// Definition of EEPROM EPDA block ID file, which only contains an ID for the
// proper EDID block entry inside the EPDA module
//
//----------------------------------------------------------------------------
                                              
typedef struct {
    UINT32		hdrIDLow;       // EPDA EEPROM ID file header low ID
    UINT32      hdrIDHigh;      // EPDA EEPROM ID file header high ID
    UINT16      blockID;		// EDID block ID inside EPDA module  (optional)
    UINT32      hdrFlags;       // non-checksum module flags (optional)
    UINT16      fileChkSum;     // module header checksum (optional)
} CG_EPDA_EEPROM_ID_FILE_HDR;

#define SIZE_EPDA_EEPROM_ID_FILE    16


//----------------------------------------------------------------------------
//
// EDID 1.3 Format Structure 
//
//----------------------------------------------------------------------------

typedef struct {
   unsigned char        EDIDHeader[8];          // EDID Header 				
   UINT16				VendorID;               // ID Vendor
   UINT16				ProductID;              // ID Product Code
   unsigned char        SerialNumID[4];         // ID Serial Number
   unsigned char        ManufactWeek;			// Week of manufactoring
   unsigned char        ManufactYear;           // Year of manufactoring
   unsigned char        EDIDVersion;            // EDID Version
   unsigned char        EDIDRevision;           // EDID Revision
   unsigned char        BasicParams[5];         // Basic Display Parameters/Features
   unsigned char        ColorChar[10];          // Color Characteristics
   unsigned char        EstablishedTiming[3];   // Established Timings Identification
   unsigned char        StandardTimings[16];    // Standard Timings Identification
   unsigned char        DetailedTimingDesc[72]; // Detailed Timing Descriptions
   unsigned char        ExtensionFlag;          // Extension Flag
   unsigned char        Checksum;               // Checksum of EDID block
} EDID13_DATA;

#define SIZE_EDID13_DATA    128

//----------------------------------------------------------------------------
//
// EDID 2.0 Format Structure 
//
//----------------------------------------------------------------------------

typedef struct {
   unsigned char        Revision;               // EDID Structure Revision
   UINT16				VendorID;               // EISA Vendor ID 
   UINT16				ProductID;              // Vendor assigned Product ID
   unsigned char        ManufactWeek;           // Week of manufactoring
   UINT16				ManufactYear;           // Year of manufactoring
   unsigned char        ID_String[32];          // Manufacturer/Product ID string
   unsigned char        SN_String[16];          // Serial Number string
   unsigned char        Unused_1[8]; 
   unsigned char        DFP_IntfParams[15];     // Display Interface Parameter
   unsigned char        DFP_DeviceDesc[5];      // Display Device Description
   UINT16				DFP_ResponseTime;       // Display Response Time
   unsigned char        ColorLumDesc[28];       // Color/Luminanc Description
   unsigned char        DFP_SpartialDesc[10];   // Display Spartial Description
   unsigned char        Unused_2;
   unsigned char        GFTSupportInfo;         // GFT Support Information
   UINT16				TimingMapInfo;          // Map of Timing Information
   unsigned char        TimingDescription[127]; // Luminance Table and Timing Description
   unsigned char        Checksum;               // checksum of all 256 bytes = 0
} EDID20_DATA;

#define SIZE_EDID20_DATA    256


//----------------------------------------------------------------------------
//
// EDID 2.0 Detailed Timing Range Format 27-byte 
//
//----------------------------------------------------------------------------

typedef struct {
   UINT16				min_PixelCLK;           // min Pixel clock /10000
   unsigned char        min_H_Blank;            // min Horizontal Blanking
   unsigned char        min_V_Blank;            // min Vertical Blanking
   unsigned char        min_HV_Blank;           // upper nibble    
   unsigned char        min_H_SyncOffset;       // min Horizontal Sync Offset         (HSO)
   unsigned char        min_H_SyncPulseWith;    // min Horizontal Sync Puls With      (HSPW)
   unsigned char        min_VSO_VSPW;           // min Vertical Sync Offset/Puls With (VSO,VSPW)
   unsigned char        min_HSO_HSPW_VSO_VSPW;  // upper 2bits of min HSO/HSPW/VSO/VSPW
//
   UINT16				max_PixelCLK;           // max Pixel clock /10000
   unsigned char        max_H_Blank;            // max Horizontal Blanking
   unsigned char        max_V_Blank;            // max Vertical Blanking
   unsigned char        max_HV_Blank;           // upper nibble    
   unsigned char        max_H_SyncOffset;       // max Horizontal Sync Offset         (HSO)
   unsigned char        max_H_SyncPulseWith;    // max Horizontal Sync Puls With      (HSPW)
   unsigned char        max_VSO_VSPW;           // max Vertical Sync Offset/Puls With (VSO,VSPW)
   unsigned char        max_HSO_HSPW_VSO_VSPW;  // upper 2bits of min HSO/HSPW/VSO/VSPW
//
   unsigned char        avr_H_ImageSize;        // average Horizontal Image Size
   unsigned char        avr_V_ImageSize;        // average Vertical Image Size
   unsigned char        avr_HV_ImageSize;       // upper nibble  
   unsigned char        avr_H_Active;           // average Horizontal Image Size
   unsigned char        avr_V_Active;           // average Vertical Image Size
   unsigned char        avr_HV_Active;          // upper nibble 
   unsigned char        avr_H_Border;           // average Horizontal Border
   unsigned char        avr_V_Border;           // average Vertical Border
   unsigned char        avr_Flags;              // Flags
} EDID20_DTD_Range;

#define SIZE_EDID20_DTD_Range   ((sizeof EDID20_DTD_Range + 1) & 0xFE)

//----------------------------------------------------------------------------
//
// Common 18 byte DTD Data Structure 
//
//----------------------------------------------------------------------------

typedef struct {
   UINT16				PixelCLK;               // Pixel clock /10000
   unsigned char        H_Active;               // Horizontal Active
   unsigned char        H_Blank;                // Horizontal Blanking
   unsigned char        H_ActiveBlank;          // upper nibble    
   unsigned char        V_Active;               // Vertical Active
   unsigned char        V_Blank;                // Vertical Blanking
   unsigned char        V_ActiveBlank;          // upper nibble  
   unsigned char        H_SyncOffset;           // Horizontal Sync Offset         (HSO)
   unsigned char        H_SyncPulseWith;        // Horizontal Sync Puls With      (HSPW)
   unsigned char        VSO_VSPW;               // Vertical Sync Offset/Puls With (VSO,VSPW)
   unsigned char        HSO_HSPW_VSO_VSPW;      // upper 2bits of HSO/HSPW/VSO/VSPW
   unsigned char        H_ImageSize;            // Horizontal Image Size
   unsigned char        V_ImageSize;            // Vertical Image Size
   unsigned char        HV_ImageSize;           // upper nibble  
   unsigned char        H_Border;               // Horizontal Border
   unsigned char        V_Border;               // Vertical Border
   unsigned char        Flags;                  // Flags
} DTD_DATA;

#define SIZE_DTD_DATA   ((sizeof DTD_DATA + 1) & 0xFE)

//----------------------------------------------------------------------------
//
// Extended 13 byte EPID Embedded Panel Interface Data
//
//----------------------------------------------------------------------------

typedef struct {
   unsigned char        EpiDescriptorHeader[5]; // EPI Descriptor Header: 00h,00h,00h,0Eh,00h
   unsigned char        DataFormatByte;         // Interface Data Format
   unsigned char        SignalInterface;        // Signal Interface Configuration
   unsigned char        DisplayMode;            // Vertical/Horizontal Display Mode
   unsigned char        PowerOnSeqDelay;        // Total Power On/Off Sequence Delay
   unsigned char        ContrastOnSeqDelay;     // Contrast Power On/Off Sequence Delay
   unsigned char        BacklightControl;       // Backlight On/Off and Brightness Control 
   unsigned char        ContrastControl;        // Contrast On/Off and Value Control   
   unsigned char        Reserved[5];
   unsigned char        DataTypTag;             // Data Type Tag
} EXT_DDT_DATA;

#define SIZE_EXT_DDT_DATA       ((sizeof EXT_DDT_DATA + 1) & 0xFE)


#define EDID13_HEADER_SIGNATURE1 0xFFFFFF00
#define EDID13_HEADER_SIGNATURE2 0x00FFFFFF
#define EDID20_HEADER_SIGNATURE	0x20

#define EPI_DATA_OFFSET     0x6C                // According to EPI spec. this
                                                // is the absolute start address
                                                // of the EPI data extension within
                                                // the EDID data set.

#pragma pack()

#define _INC_EPI_EDID
#endif 


