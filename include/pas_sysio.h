/***********************************************************************
 * pas_sysio.h
 * Definitions of the arguments of the oSYSIO opcode
 *
 *   Copyright (C) 2008-2009, 2021-2022 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ***********************************************************************/

#ifndef __PAS_SYSIO_H
#define __PAS_SYSIO_H

/***********************************************************************/
/* Codes for system IO calls associated with standard Pascal procedure
 * and function calls.  These must be confined to the range 0x0000
 * through 0xffff.
 */

/* These are for internal use by the compiler and run-time */

#define xALLOCFILE         (0x0001)  /* Allocate a file number */
#define xFREEFILE          (0x0002)  /* Release the allocated file */

/* These correspond to Pascal Standard I/O functions */

#define xEOF               (0x0003)  /* Test for end of file */
#define xEOLN              (0x0004)  /* Test for end of line */
#define xFILEPOS           (0x0005)  /* Get position in file */
#define xFILESIZE          (0x0006)  /* Get size of file */
#define xSEEK              (0x0007)  /* Change position in file */
#define xSEEKEOF           (0x0008)  /* Set file position to end of file */
#define xSEEKEOLN          (0x0009)  /* Set file position to end of line */

#define xASSIGNFILE        (0x000a)  /* Assign a name and type to the file */
#define xRESET             (0x000b)  /* Open the file for reading */
#define xRESETR            (0x000c)  /* Open the file for reading and reset
                                      * record size */
#define xREWRITE           (0x000d)  /* Open the file for writing */
#define xREWRITER          (0x000e)  /* Open the file for writing and reset
                                      * record size */
#define xAPPEND            (0x000f)  /* Open the file for appending */
#define xCLOSEFILE         (0x0010)  /* Close the file */

#define xREADLN            (0x0011)  /* Move to the next line */
#define xREAD_PAGE         (0x0012)  /* Move to the next page */
#define xREAD_BINARY       (0x0013)  /* Read from a binary file */
#define xREAD_INT          (0x0014)  /* Read an integer from a text file */
#define xREAD_CHAR         (0x0015)  /* Read an character from a text file */
#define xREAD_STRING       (0x0016)  /* Read an string from a text file */
#define xREAD_SHORTSTRING  (0x0017)  /* Read an string from a text file */
#define xREAD_REAL         (0x0018)  /* Read an real value from a text file */

#define xWRITELN           (0x0019)  /* Move to the next line */
#define xWRITE_PAGE        (0x001a)  /* Move to the next page */
#define xWRITE_BINARY      (0x001b)  /* Write to a binary file */
#define xWRITE_INT         (0x001c)  /* Write a signed integer to a text file */
#define xWRITE_WORD        (0x001d)  /* Write an unsigned integer to a text file */
#define xWRITE_LONGINT     (0x001e)  /* Write an long integer to a text file */
#define xWRITE_LONGWORD    (0x001f)  /* Write an unsigned integer to a text file */
#define xWRITE_CHAR        (0x0020)  /* Write an character to a text file */
#define xWRITE_STRING      (0x0021)  /* Write an string to a text file */
#define xWRITE_SHORTSTRING (0x0022)  /* Write an string to a text file */
#define xWRITE_REAL        (0x0023)  /* Write an real value to a text file */

#define xCHDIR             (0x0024)  /* Set current working directory */
#define xGETDIR            (0x0025)  /* Get the current working directory */
#define xMKDIR             (0x0026)  /* Create a new directory */
#define xRMDIR             (0x0027)  /* Remove an existing directory */

#define MAX_XOP            (0x0028)

#endif /* __PAS_SYSIO_H */
