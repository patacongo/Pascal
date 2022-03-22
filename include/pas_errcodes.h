/***********************************************************************
 * pas_errcodes.h
 * Definitions of error codes
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

#ifndef __PAS_ERRCODES_H
#define __PAS_ERRCODES_H

/***********************************************************************
 * Included Files
 ***********************************************************************/

#include <stdint.h>

/***********************************************************************
 * Pre-processor Definitions
 ***********************************************************************/

/***********************************************************************
 * COMPILATION TIME ERRORS
 * -----------------------
 * eASSIGN              Expected ':='
 * eBEGIN               Expected 'BEGIN'
 * eCOLON               Expected ':'
 * eCOMMA               Expected ','
 * eCOUNT               Error count exceeded (FATAL)
 * eDO                  Expected 'DO'
 * eDUPFILE             Attempt to re-define file
 * eDUPSYM              Attempt to declare duplicate symbol
 * eEND                 Expected 'END'
 * eEQ                  Expected '='
 * eEXPONENT            Error in exponent of real constant
 * eFILE                Expected file identifier declared in PROGRAM
 * eHUH                 Internal error (FATAL)
 * eIDENT               Expected identifier
 * eIMPLEMENTATION      Expected implementation section in unit file
 * eINCLUDE             Include file OPEN error (FATAL)
 * eINTCONST            Expected integer constant
 * eINTERFACE           Expected interface section in unit file
 * eINTOVF              Integer overflow (or underflow)
 * eINTVAR              Integer variable name expected
 * eINVALIDFUNC         Unrecognized built-in function
 * eINVALIDPROC         Unrecognized built-in procedure
 * eINVARG              Invalid procedure/function argument type
 * eINVCONST            Invalid constant in declaration
 * eINVSIGNEDCONST      Invalid constant after sign
 * eINVFACTOR           Invalid factor
 * eINVFILE             Invalid file identifier (as declared in PROGRAM)
 * eINVFILETYPE         Expected file type
 * eINVLABEL            Invalid label
 * eINVPTR              Invalid pointer type
 * eINVTYPE             Invalid type identifier
 * eINVPARMTYPE         Invalid/unsupported actual parameter type
 * eINVVARPARM          Invalid/unsupported VAR parameter type
 * eTOOMANYINIT         Too many initializers
 * eREALINIT            Bad initializer for type real
 * eSTRINGINIT          Bad initializer for string type
 * eSETINIT             Bad initializer for set type
 * eBADINITIALIZER      Bad/unrecognized initializer
 * eLBRACKET            Expected '['
 * eLPAREN              Expected '('
 * eMULTLABEL           Attempt to multiply define label
 * eNOSQUOTE            EOL encounter in a string, probable missing "'"
 * eNOTYET              Not implemented yet
 * eNPARMS              Number of parameters in function or procedure
 *                      call does not match number in declaration
 * eOF                  Expected 'OF'
 * eNOTINLOOP           Continue or break when not in a loop
 * eOVF                 Internal table overflow (FATAL)
 * ePERIOD              Expected '.'
 * ePOFFCONFUSION       Internal logic error in POFF file generation
 * ePOFFWRITEERROR      Error writing to POFF file
 * ePROGRAM             Expected 'PROGRAM'
 * ePTRADR              Expected pointer address form (probably got value)
 * ePTRVAL              Expected pointer value form, got value form
 * eRBRACKET            Expected ']'
 * eRPAREN              Expected ')'
 * eRPARENorCOMMA       Expected ')' or ','
 * eSEEKFAIL            Seek to file position failed
 * eSEMICOLON           Expected ';'
 * eSTRING              Expected a string
 * eTHEN                Expected 'THEN'
 * eTOorDOWNTO          Expected 'TO' or 'DOWNTO' in for statement
 * eTRUNC               String truncated
 * eUNDECLABEL          Attempt to define an undeclared label.
 * eUNDEFLABEL          A declared label was not defined.
 * eUNDEFSYM            Undefined symbol
 * eUNTIL               Expected 'UNTIL'
 * eEXPRTYPE            Illegal expression type for this operation
 * eSHIFTTYPE           Illegal expression type for shift
 * eTERMTYPE            Illegal term type for this operation
 * eFACTORTYPE          Illegal factor type for this operation
 * eCOMPARETYPE         Comparison type mismatch
 * eREADPARM            Illegal parameter in READ statement
 * eNOREADPARM          Missing parameter in READ statement
 * eREADPARMTYPE        Illegal read parameter type
 * eWRITEPARM           Illegal parameter in WRITE statement
 * eNOWRITEPARM         Missing parameter in WRITE statement
 * eBADFIELDWIDTH       Bad field width in WRITE statement
 * eBADPRECISION        Bad precision in WRITE statement
 * eINDEXTYPE           Illegal array index type
 * eARRAYTYPE           Illegal or incorrect ARRAY type
 * eTOOMANYINDICES      Two many indices for dimensionality of array
 * ePOINTERTYPE         Expected a pointer type
 * ePOINTERDEREF        Illegal pointer de-reference
 * eVARPARMTYPE         Illegal VAR parameter type
 * eSUBRANGE            Expected ".."
 * eSUBRANGETYPE        Illegal subrange type
 * eSET                 Expected valid/consistent type for SET OF
 * sSETRANGE            Value out of range for SET OF
 * sSETELEMENT          Invalid or missing set element
 * eSCALARTYPE          Illegal scalar type
 * eBADINTRANGE         Integer value is out of range
 * eSYMTABINTERNAL      Internal error in symbol table
 * eRECORDDECLARE       Error in RECORD declaration
 * eRECORDOBJECT        Expected a field of RECORD
 * eRECORDVAR           Expected a RECORD variable (e.g., in WITH)
 * eRECORDTYPE          Bad or incorrect record type
 * eUNIT                Expected UNIT at beginning of unit file
 * eUNITNAME            File does not contain the expected UNIT
 * eARGIGNORED          An argument was provided, but ignored
 *
 * ERRORS REPOORTED BY THE OPTIMIZER
 * ---------------------------------
 * eBUFTOOSMALL         Buffer is too small
 * eBADRELOCDATA        Relocation data is not in the expected order
 * eEXTRARELOCS         Relocation data remains afer all instructions processed
 *
 * LINK TIME ERRORS
 * ----------------
 * eUNDEFINEDSYMBOL     A necessary symbol was not defined
 * eMULTIDEFSYMBOL      A symbol was defined multiple times
 *
 * ERRORS WHICH MAY OCCUR AT EITHER COMPILATION, LINK OR RUNTIME
 * -------------------------------------------------------------
 * eNOMEMORY            Memory allocation failed
 * ePOFFREADERROR       Error reading a POFF file
 * ePOFFBADFORMAT       The file format does not like valid POFF
 * eRCVDSIGNAL          Received SIGSEGV (or other) signal.
 *
 * RUN TIME ERRORS
 * ---------------
 * eBADPC               Program Counter is out-of-range
 * eILLEGALOPCODE       Non-executable instruction found
 * eBADSP               Stack reference is out-of-range
 * eSTRSTKOVERFLOW      String stack overflow
 * eNEWFAILED           new() failed to allocate memory
 * eDOUBLEFREE          dispose() was called on memory previously freed
 * eEXIT                oEND P-Code encountered
 * eBADSYSIOFUNC        Illegal SYSIO sub-function
 * eBADSYSLIBCALL       Illegal runtime library call
 * eBADFPOPCODE         Illegal FLOAT POINT op-code
 * eBADSETOPCODE        Illegal SET operator op-code
 * eINTEGEROVERFLOW     Integer overflow
 * eVALUERANGE
 * eNESTINGLEVEL
 * eFAILEDLIBCALL       Runtime library call returned failure
 * eBADFILE
 * eFILENOTINUSE
 * eTOOMANYFILES
 * eFILENOTOPEN
 * eFILEALREADYOPEN
 * eBADOPENMODE
 * eOPENFAILED
 * eNOTOPENFORREAD
 * eREADFAILED
 * eNOTOPENFORWRITE
 * eWRITEFAILED
 * eFSEEKFAILED         fseek() returned an error
 * eFTELLFAILED         ftell() returned an error
 * eGETCWDFAILED        getcwd() returned an error
 * eBADPEXFILE          .pex file does not exist
 * eBADFILETYPE         .pex file is not a regular file
 * eSPAWANFAILED        posix_spawnp() failed
 * eWAITFAILED          waitpid() failed
 *
 **********************************************************************/

/* This is the error code that indicates that no error has occurred */

#define eNOERROR       ((uint16_t) 0x00)

/* This is the maximum number of errors that will be reported before
 * compilation is terminated.
 */

#define MAX_ERRORS 100

/* COMPILATION TIME ERRORS */

#define eASSIGN          ((uint16_t) 0x01)
#define eBEGIN           ((uint16_t) 0x02)
#define eCOLON           ((uint16_t) 0x03)
#define eCOMMA           ((uint16_t) 0x04)
#define eCOUNT           ((uint16_t) 0x05)
#define eDO              ((uint16_t) 0x06)
#define eDUPFILE         ((uint16_t) 0x07)
#define eDUPSYM          ((uint16_t) 0x08)
#define eEND             ((uint16_t) 0x09)
#define eEQ              ((uint16_t) 0x0a)
#define eEXPONENT        ((uint16_t) 0x0b)
#define eFILE            ((uint16_t) 0x0c)
#define eHUH             ((uint16_t) 0x0d)
#define eIDENT           ((uint16_t) 0x0e)
#define eIMPLEMENTATION  ((uint16_t) 0x0f)

#define eINCLUDE         ((uint16_t) 0x10)
#define eINTCONST        ((uint16_t) 0x11)
#define eINTERFACE       ((uint16_t) 0x12)
#define eINTOVF          ((uint16_t) 0x13)
#define eINTVAR          ((uint16_t) 0x14)
#define eINVALIDFUNC     ((uint16_t) 0x15)
#define eINVALIDPROC     ((uint16_t) 0x16)
#define eINVARG          ((uint16_t) 0x17)
#define eINVCONST        ((uint16_t) 0x18)
#define eINVSIGNEDCONST  ((uint16_t) 0x19)
#define eINVFACTOR       ((uint16_t) 0x1a)
#define eINVFILE         ((uint16_t) 0x1b)
#define eINVFILETYPE     ((uint16_t) 0x1c)
#define eINVLABEL        ((uint16_t) 0x1d)
#define eINVPTR          ((uint16_t) 0x1e)
#define eINVTYPE         ((uint16_t) 0x1f)

#define eINVPARMTYPE     ((uint16_t) 0x20)
#define eINVVARPARM      ((uint16_t) 0x21)
#define eTOOMANYINIT     ((uint16_t) 0x22)
#define eREALINIT        ((uint16_t) 0x23)
#define eSTRINGINIT      ((uint16_t) 0x24)
#define eSETINIT         ((uint16_t) 0x25)
#define eBADINITIALIZER  ((uint16_t) 0x26)
#define eLBRACKET        ((uint16_t) 0x27)
#define eLPAREN          ((uint16_t) 0x28)
#define eMULTLABEL       ((uint16_t) 0x29)
#define eNOSQUOTE        ((uint16_t) 0x2a)
#define eNOTYET          ((uint16_t) 0x2b)
#define eNPARMS          ((uint16_t) 0x2c)
#define eOF              ((uint16_t) 0x2d)
#define eNOTINLOOP       ((uint16_t) 0x2e)
#define eOVF             ((uint16_t) 0x2f)

#define ePERIOD          ((uint16_t) 0x30)
#define ePOFFCONFUSION   ((uint16_t) 0x31)
#define ePOFFWRITEERROR  ((uint16_t) 0x32)
#define ePROGRAM         ((uint16_t) 0x33)
#define ePTRADR          ((uint16_t) 0x34)
#define ePTRVAL          ((uint16_t) 0x35)
#define eRBRACKET        ((uint16_t) 0x36)
#define eRPAREN          ((uint16_t) 0x37)
#define eRPARENorCOMMA   ((uint16_t) 0x38)
#define eSEEKFAIL        ((uint16_t) 0x39)
#define eSEMICOLON       ((uint16_t) 0x3a)
#define eSTRING          ((uint16_t) 0x3b)
#define eTHEN            ((uint16_t) 0x3c)
#define eTOorDOWNTO      ((uint16_t) 0x3d)
#define eTRUNC           ((uint16_t) 0x3e)
#define eUNDECLABEL      ((uint16_t) 0x3f)

#define eUNDEFLABEL      ((uint16_t) 0x40)
#define eUNDEFSYM        ((uint16_t) 0x41)
#define eUNTIL           ((uint16_t) 0x42)
#define eEXPRTYPE        ((uint16_t) 0x43)
#define eSHIFTTYPE       ((uint16_t) 0x44)
#define eTERMTYPE        ((uint16_t) 0x45)
#define eFACTORTYPE      ((uint16_t) 0x46)
#define eCOMPARETYPE     ((uint16_t) 0x47)
#define eREADPARM        ((uint16_t) 0x48)
#define eNOREADPARM      ((uint16_t) 0x49)
#define eREADPARMTYPE    ((uint16_t) 0x4a)
#define eWRITEPARM       ((uint16_t) 0x4b)
#define eNOWRITEPARM     ((uint16_t) 0x4c)
#define eWRITEPARMTYPE   ((uint16_t) 0x4d)
#define eBADFIELDWIDTH   ((uint16_t) 0x4e)
#define eBADPRECISION    ((uint16_t) 0x4f)

#define eINDEXTYPE       ((uint16_t) 0x50)
#define eARRAYTYPE       ((uint16_t) 0x51)
#define ePOINTERTYPE     ((uint16_t) 0x52)
#define eTOOMANYINDICES  ((uint16_t) 0x53)
#define ePOINTERDEREF    ((uint16_t) 0x54)
#define eVARPARMTYPE     ((uint16_t) 0x55)
#define eSUBRANGE        ((uint16_t) 0x56)
#define eSUBRANGETYPE    ((uint16_t) 0x57)
#define eSET             ((uint16_t) 0x58)
#define eSETRANGE        ((uint16_t) 0x59)
#define eSETELEMENT      ((uint16_t) 0x5a)
#define eSCALARTYPE      ((uint16_t) 0x5b)
#define eBADINTRANGE     ((uint16_t) 0x5c)
#define eSYMTABINTERNAL  ((uint16_t) 0x5d)
#define eRECORDDECLARE   ((uint16_t) 0x5e)
#define eRECORDOBJECT    ((uint16_t) 0x5f)

#define eRECORDVAR       ((uint16_t) 0x60)
#define eRECORDTYPE      ((uint16_t) 0x61)
#define eUNIT            ((uint16_t) 0x62)
#define eUNITNAME        ((uint16_t) 0x63)
#define eARGIGNORED      ((uint16_t) 0x64)

/* ERRORS REPOORTED BY THE OPTIMIZER */

#define eBUFTOOSMALL     ((uint16_t) 0x70)
#define eBADRELOCDATA    ((uint16_t) 0x71)
#define eEXTRARELOCS     ((uint16_t) 0x72)

/* LINK TIME ERRORS */

#define eUNDEFINEDSYMBOL ((uint16_t) 0x80)
#define eMULTIDEFSYMBOL  ((uint16_t) 0x81)

/* ERRORS WHICH MAY OCCUR AT EITHER COMPILATION OR RUNTIME */

#define eNOMEMORY        ((uint16_t) 0x90)
#define ePOFFREADERROR   ((uint16_t) 0x91)
#define ePOFFBADFORMAT   ((uint16_t) 0x92)
#define eRCVDSIGNAL      ((uint16_t) 0x93)

/* RUN TIME ERRORS */

#define eBADPC           ((uint16_t) 0xa0)
#define eILLEGALOPCODE   ((uint16_t) 0xa1)
#define eBADSP           ((uint16_t) 0xa2)
#define eSTRSTKOVERFLOW  ((uint16_t) 0xa3)
#define eNEWFAILED       ((uint16_t) 0xa4)
#define eDOUBLEFREE      ((uint16_t) 0xa5)
#define eEXIT            ((uint16_t) 0xa6)
#define eBADSYSIOFUNC    ((uint16_t) 0xa7)
#define eBADSYSLIBCALL   ((uint16_t) 0xa8)
#define eBADFPOPCODE     ((uint16_t) 0xa9)
#define eBADSETOPCODE    ((uint16_t) 0xaa)
#define eINTEGEROVERFLOW ((uint16_t) 0xab)
#define eVALUERANGE      ((uint16_t) 0xac)
#define eNESTINGLEVEL    ((uint16_t) 0xad)
#define eFAILEDLIBCALL   ((uint16_t) 0xae)

#define eBADFILE         ((uint16_t) 0xb0)
#define eBADFILENAME     ((uint16_t) 0xb1)
#define eFILENOTINUSE    ((uint16_t) 0xb2)
#define eTOOMANYFILES    ((uint16_t) 0xb3)
#define eFILENOTOPEN     ((uint16_t) 0xb4)
#define eFILEALREADYOPEN ((uint16_t) 0xb5)
#define eBADOPENMODE     ((uint16_t) 0xb6)
#define eOPENFAILED      ((uint16_t) 0xb7)
#define eNOTOPENFORREAD  ((uint16_t) 0xb8)
#define eREADFAILED      ((uint16_t) 0xb9)
#define eNOTOPENFORWRITE ((uint16_t) 0xba)
#define eWRITEFAILED     ((uint16_t) 0xbb)
#define eFSEEKFAILED     ((uint16_t) 0xbc)
#define eFTELLFAILED     ((uint16_t) 0xbd)
#define eGETCWDFAILED    ((uint16_t) 0xbe)
#define eBADPEXFILE      ((uint16_t) 0xbf)
#define eBADFILETYPE     ((uint16_t) 0xc0)
#define eSPAWANFAILED    ((uint16_t) 0xc1)
#define eWAITFAILED      ((uint16_t) 0xc2)

#endif /* __PAS_ERRCODES_H */
