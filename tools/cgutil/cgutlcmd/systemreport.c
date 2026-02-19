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
 * Copyright (c) 2005 congatec AG
 *
 * This file contains proprietary and confidential information.
 * All rights reserved.
 *
 * $Header:   S:/CG/archives/CGTOOLS/CGUTIL/W32DOSX/CGUTLCMD/SystemReport.c-arc   1.2   Sep 06 2016 16:20:02   congatec  $
 *
 * Contents: Congatec system report module interface routines.
 *
 *---------------------------------------------------------------------------
 *
 *                      R E V I S I O N   H I S T O R Y
 *
 * $Log:   S:/CG/archives/CGTOOLS/CGUTIL/W32DOSX/CGUTLCMD/SystemReport.c-arc  $
 * 
 *    Rev 1.2   Sep 06 2016 16:20:02   congatec
 * Added BSD header.
 * 
 *    Rev 1.1   May 31 2012 15:36:54   gartner
 * Updated variable definitions to ease 64bit porting.
 * 
 *    Rev 1.0   Oct 04 2005 13:16:32   gartner
 * Initial revision.
 * 
 *    Rev 1.0   Sep 30 2005 16:25:06   gartner
 * Initial revision.
 *
 *---------------------------------------------------------------------------
 */

/*---------------
 * Include files
 *---------------
 */
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <conio.h>
#include <tchar.h>
#include "CGUTLCMN.H"
#include "CGOS.H"

/*--------------
 * Externs used
 *--------------
 */


/*--------------------
 * Local definitions
 *--------------------
 */

/*------------------
 * Global variables
 *------------------
 */

/*---------------------------------------------------------------------------
 * Name:
 * Desc:
 * Inp:         
 * Outp:        
 *---------------------------------------------------------------------------
 */
void HandleReportGeneration(int argc, _TCHAR* argv[])
{
        PRINTF(_T("\nHandleReportGeneration\n"));
        exit (0);
}
