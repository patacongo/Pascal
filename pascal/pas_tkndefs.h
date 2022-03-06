/***********************************************************************
 * pas_tkndefs.h
 * Token and Symbol Table Definitions
 *
 *   Copyright (C) 2008, 2021-2022 Gregory Nutt. All rights reserved.
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

#ifndef __PAS_TKNDEFS_H
#define __PAS_TKNDEFS_H

/***********************************************************************/

/* Token Values 0-0x20 reserved for get_token identification */

#define tIDENT           0x01
#define tINT_CONST       0x02
#define tCHAR_CONST      0x03
#define tBOOLEAN_CONST   0x04
#define tREAL_CONST      0x05
#define tSTRING_CONST    0x06
#define tSET_CONST       0x07

#define tLE              0x08  /* <= */
#define tGE              0x09  /* >= */
#define tASSIGN          0x0a  /* := */
#define tSUBRANGE        0x0b  /* .. */
#define tSYMDIFF         0x0c  /* Symmetric difference '><' */

/* Token Values 0x21-0x2F (except 0x24) are for ASCII character tokens:
 * '!', '"', '#', '$' '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/'
 */

#define tNE              ('#')  /* Or '<>' */
#define SQUOTE           0x27   /* Not a token returned by get_token */
#define tMUL             ('*')
#define tFDIV            ('/')

/* Token Values 0x30-0x39 are spare */

/* Token Values 0x3A-0x40 are for ASCII character tokens:
 * ':', ';', '<', '=', '>', '?', '@'
 */

#define tLT              ('<')
#define tEQ              ('=')
#define tGT              ('>')

/* Token Values 0x41-0x5a are SYMBOL TABLE definitions */

#define sPROC            0x41
#define sFUNC            0x42
#define sLABEL           0x43
#define sTYPE            0x44
#define sFILE            0x45
#define sTEXTFILE        0x46
#define sINT             0x47
#define sWORD            0x48
#define sSHORTINT        0x49
#define sSHORTWORD       0x4a
#define sLONGINT         0x4b
#define sLONGWORD        0x4c
#define sBOOLEAN         0x4d
#define sCHAR            0x4e
#define sREAL            0x4f
#define sSTRING          0x50
#define sSTRING_CONST    0x51
#define sPOINTER         0x52
#define sSCALAR          0x53
#define sSCALAR_OBJECT   0x54
#define sSUBRANGE        0x55
#define sSET             0x56
#define sARRAY           0x57
#define sRECORD          0x58
#define sRECORD_OBJECT   0x59
#define sVAR_PARM        0x5a

/* Token Values 0x5B-0x60 (except 0x5F) are for ASCII character tokens:
 * '[', '\', ']', '^', '_', '`'
 */

/* Token Values 0x61-0x7a are SYMBOL TABLE definitions */

#define sUNITNAME        0x61

/* Token Values 0x7b-0x7f are for ASCII character tokens:
 * '{', '|', '}''~', DEL
 */

/* Token Value  0x7f is spare */

/* Token Values 0x80-0xef are for RESERVED WORDS */

/* Standard constants (TRUE, FALSE, MAXINT) and standard files (INPUT, OUTPUT)
 * are hard initialized into the constant/symbol table and are transparent
 * to the compiler.
 */

/* Reserved Words 0x80-0xaf*/

#define tAND             0x80
#define tARRAY           0x81
#define tBEGIN           0x82
#define tCASE            0x83
#define tCONST           0x84
#define tDIV             0x85
#define tDO              0x86
#define tDOWNTO          0x87
#define tELSE            0x88
#define tEND             0x89
#define tFILE            0x8a
#define tFINALIZATION    0x8b /* Extended pascal */
#define tFOR             0x8c
#define tFUNCTION        0x8d
#define tGOTO            0x8e
#define tIF              0x8f
#define tIMPLEMENTATION  0x90 /* Extended pascal */
#define tIN              0x91
#define tINITIALIZATION  0x92 /* Extended pascal */
#define tINTERFACE       0x93 /* Extended pascal */
#define tLABEL           0x94
#define tMOD             0x95
#define tNIL             0x96
#define tNOT             0x97
#define tOF              0x98
#define tOR              0x99
#define tXOR             0x9a
#define tPACKED          0x9b
#define tPROCEDURE       0x9c
#define tPROGRAM         0x9d
#define tRECORD          0x9e
#define tREPEAT          0x9f
#define tSET             0xa0
#define tSHL             0xa1
#define tSHR             0xa2
#define tTHEN            0xa3
#define tTO              0xa4
#define tTYPE            0xa5
#define tUNIT            0xa6 /* Extended pascal */
#define tUNTIL           0xa7
#define tUSES            0xa8 /* Extended pascal */
#define tVAR             0xa9
#define tWHILE           0xb0
#define tWITH            0xb1

/* The following codes indicate that the token is a built-in procedure
 * or function recognized by the compiler.  An additional code will be
 * place in g_tknSubType by the tokenizer to indicate which built-in
 * procedure or function applies.
 */

#define tSTDFUNC         0xc0
#define tSTDPROC         0xc1
#define tBUILTIN         0xc2

/***********************************************************************/

/* Codes to indentify built-in functions and procedures */

#define txNONE           0x00

/* Standard Functions 0x01-0x1f*/

#define txABS            0x01
#define txADDR           0x02
#define txARCTAN         0x03
#define txCHR            0x04
#define txCOS            0x05
#define txEOLN           0x06
#define txEXP            0x07
#define txLN             0x08
#define txODD            0x09
#define txORD            0x0a
#define txPRED           0x0b
#define txROUND          0x0c
#define txSIN            0x0d
#define txSQR            0x0e
#define txSQRT           0x0f
#define txSUCC           0x10
#define txTRUNC          0x11

/* "Less than standard" Functions 0x20-0x7f */

#define txGETENV         0x20

/* Standard Procedures 0x81-0xbf */

#define txDISPOSE        0x80
#define txEXIT           0x81
#define txGET            0x82
#define txHALT           0x83
#define txNEW            0x84
#define txPACK           0x85
#define txPAGE           0x86
#define txPUT            0x87
#define txUNPACK         0x88

/* File I/O */

#define txAPPEND         0x89
#define txASSIGNFILE     0x8a
#define txCLOSEFILE      0x8b
#define txFILEPOS        0x8c
#define txFILESIZE       0x8d
#define txEOF            0x8e
#define txREAD           0x8f
#define txREADLN         0x90
#define txRESET          0x91
#define txREWRITE        0x92
#define txSEEK           0x93
#define txSEEKEOF        0x94
#define txSEEKEOLN       0x95
#define txWRITE          0x96
#define txWRITELN        0x97

/* SET operators */

#define txINCLUDE        0x98
#define txEXCLUDE        0x99
#define txCARD           0x9a

/* Built-in, compile-time operations */

#define txSIZEOF         0x9b

/* Borland-style string procedures and functions */

#define txLENGTH         0x9c
#define txCHARAT         0x9d
#define txCHDIR          0x9e
#define txCLOSEDIR       0x9f
#define txCOPY           0xa0
#define txCREATEDIR      0xa1
#define txFILLCHAR       0xa2
#define txGETDIR         0xa3
#define txMKDIR          0xa4
#define txOPENDIR        0xa5
#define txPOS            0xa6
#define txREADDIR        0xa7
#define txREMOVEDIR      0xa8
#define txREWINDDIR      0xa9
#define txRMDIR          0xaa
#define txVAL            0xab
#define txSETCURRDIR     0xac
#define txSTR            0xad
#define txCONCAT         0xae
#define txINSERT         0xaf
#define txDELETE         0xb0

#endif /* __PAS_TKNDEFS_H */
