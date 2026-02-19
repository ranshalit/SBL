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
 * $Header:   S:/CG/archives/CGTOOLS/CGUTIL/W32DOSX/CGUTLCMD/cgutil.h-arc   1.3   Sep 06 2016 16:34:44   congatec  $
 *
 * Contents: Common congatec configuration tool definitions.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/CGUTIL/W32DOSX/CGUTLCMD/cgutil.h-arc  $
 * 
 *    Rev 1.3   Sep 06 2016 16:34:44   congatec
 * Added BSD header.
 * 
 *    Rev 1.2   May 31 2012 15:33:32   gartner
 * Updated variable definitions to ease 64bit porting.
 * 
 *    Rev 1.1   Oct 30 2006 14:37:24   gartner
 * Removed obsolete definitions.
 * 
 *    Rev 1.0   Oct 04 2005 13:16:30   gartner
 * Initial revision.
 * 
 *    Rev 1.0   Sep 30 2005 16:25:06   gartner
 * Initial revision.
 *
 *---------------------------------------------------------------------------
 */

#ifndef _INC_CGCONFIG

#ifdef __cplusplus
extern "C" {
#endif 

#if (_MSC_VER <= 600)
#define __cdecl     _cdecl
#define __far       _far
#endif 

//
// Executable dependent report definitions.
//
#ifdef _CONSOLE

#ifndef TEXT
#define TEXT(s) s
#endif


void report(char *s)
{
        printf("%s\n",s);
}
#endif


#undef _UNICODE


#ifdef __cplusplus
}
#endif 

#define _INC_CGCONFIG
#endif 
