/****************************************************************************
 * pas_procedure.c
 * Standard procedures (all called in pas_statement.c)
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "pas_debug.h"
#include "pas_defns.h"
#include "pas_tkndefs.h"
#include "pas_pcode.h"
#include "pas_errcodes.h"
#include "pas_sysio.h"
#include "pas_library.h"

#include "pas_main.h"
#include "pas_expression.h"
#include "pas_procedure.h"
#include "pas_initializer.h"
#include "pas_codegen.h"     /* for pas_Generate*() */
#include "pas_token.h"
#include "pas_symtable.h"    /* For parent symbol references */
#include "pas_error.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* There is an issue with using file types as the first argument of most
 * standard file I/O procedures and functions.  This is a one-pass compiler
 * and before we consume the input, we must be certain that that first
 * argument this is going to resolve into a file type.
 *
 * A RECORD could contain a file field.  All we know is that it is a RECORD
 * and we cannot know the type of the RECORD field that is selected until
 * we parse a little more.  So the logic will make bad decisions with
 * RECORDS (and worse, with ARRAYs of RECORDs).
 *
 * The option below disables all support for files in RECORDs and at least
 * makes the behavior consistent by disallowing RECORDs containing files.
 * Perhaps, in the future, we will add some kind of parse ahead logic to
 * handle this case.
 */

#undef CONFIG_PAS_FILERECORD

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static uint16_t pas_SimplifyFileNumber(symbol_t *varPtr, uint8_t fileFlags,
                                       uint16_t *pFileSize,
                                       symbol_t *defaultFilePtr);
static uint16_t pas_DefaultFileNumber(symbol_t *defaultFilePtr,
                                      uint16_t *pFileSize);

/* Helpers for standard procedures  */

static void     pas_ExitProc(void);                 /* EXIT procedure */
static void     pas_HaltProc(void);                 /* HALT procedure */

static void     pas_ReadProc(void);                 /* READ procedure */
static void     pas_ReadlnProc(void);               /* READLN procedure */
static void     pas_ReadProcCommon(bool text,       /* READ[LN] common logic */
                  uint16_t fileSize);
static void     pas_ReadText(void);                 /* READ text file */
static void     pas_ReadBinary(uint16_t fileSize);  /* READ binary file */
static void     pas_OpenFileProc(uint16_t opcode1,  /* Open file */
                  uint16_t opcode2);
static void     pas_FileProc(uint16_t opcode);      /* File procedure with 1 arg */
static void     pas_AssignFileProc(void);           /* ASSIGNFILE procedure */
static void     pas_WriteProc(void);                /* WRITE procedure */
static void     pas_WritelnProc(void);              /* WRITELN procedure */
static void     pas_WriteProcCommon(bool text,      /* WRITE[LN] common logic */
                  uint16_t fileSize);
static void     pas_WriteText(void);                /* WRITE text file */
static uint16_t pas_WriteFieldWidth(void);          /* Get text file write field-width. */
static void     pas_WriteBinary(uint16_t fileSize); /* WRITE binary file */
static void     pas_NewProc(void);                  /* Memory allocator */
static void     pas_DisposeProc(void);              /* Free memory */

static uint16_t pas_GenVarFileNumber(symbol_t *varPtr,
                  uint16_t *pFileSize,
                  symbol_t *defaultFilePtr);

/* Helpers for less-than-standard procedures */

static void     pas_ValProc(void);                  /* VAL procedure */

/* Misc. helper functions */

static void     pas_InitializeNewRecord(symbol_t *typePtr);
static void     pas_InitializeNewArray(symbol_t *typePtr);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* procedure val(const S : string; var V; var Code : word); */

static symbol_t valSymbol[4];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/***********************************************************************/

static uint16_t pas_SimplifyFileNumber(symbol_t *varPtr, uint8_t fileFlags,
                                       uint16_t *pFileSize,
                                       symbol_t *defaultFilePtr)
{
  uint16_t fileType;
  uint16_t fileSize;

  /* Check if this is a VAR parameter */

  switch (varPtr->sKind)
    {
      case sPOINTER :
      case sVAR_PARM :
        {
          return pas_GenVarFileNumber(varPtr, pFileSize, defaultFilePtr);
        }

#ifdef CONFIG_PAS_FILERECORD
      case sRECORD :
        {
          /* We expect the RECORD array to be followed by a field
           * selector.  On input the current token may be a RECORD
           * or, if we get here via an array, it may be the '.'
           * field separator.
           */

          if (g_token != '.')
            {
              getToken();
            }

          if (g_token != '.')
            {
              error(eRECORDOBJECT);
              return pas_DefaultFileNumber(defaultFilePtr, pFileSize);
            }
          else
            {
              getToken();
            }

          if (g_token != sRECORD_OBJECT)
            {
              error(eRECORDOBJECT);
              return pas_DefaultFileNumber(defaultFilePtr, pFileSize);
            }
          else
            {
              symbol_t *fieldPtr = g_tknPtr;
              symbol_t *baseTypePtr;

              /* Verify that the field selector resolves to a
               * file type.
               */

              baseTypePtr = pas_GetBaseTypePointer(fieldPtr->sParm.r.rParent);
              fileType    = baseTypePtr->sParm.t.tType;

              if (fileType != sFILE && fileType != sTEXTFILE)
                {
                  error(eINVFILETYPE);
                }

              /* If the field offset is non-zero, add that to the array
               * offset.
               */

              if (baseTypePtr->sParm.r.rOffset != 0)
                {
                  /* Add the field offset */

                  pas_GenerateDataOperation(opPUSH,
                                            fieldPtr->sParm.r.rOffset);
                  pas_GenerateSimple(opADD);
                }

              /* Generate the indexed load */

              pas_GenerateStackReference(opLDSX, varPtr);

              getToken();
              return fileType;
            }
        }
        break;
#endif

      /* Check if this is an array of files */

      case sARRAY :
        {
          symbol_t *typePtr;
          symbol_t *baseTypePtr;

          fileFlags |= FACTOR_INDEXED;

          /* REVISIT:  Before we consume the input, we must be certain that
           * this is going to resolve into a file type.  All we know now is
           * that this is an arry... but and array of what?
           *
           * The complex case is an array of records which may or may not
           * contain a file field.  We can't know which field is selected
           * until we parse the field selector.
           */

          typePtr = varPtr->sParm.v.vParent;
          if (typePtr == NULL)
            {
              error(eHUH);
              return pas_DefaultFileNumber(defaultFilePtr, pFileSize);
            }

          /* Get a pointer to the underlying base type symbol */

          baseTypePtr = pas_GetBaseTypePointer(typePtr);

#ifdef CONFIG_PAS_FILERECORD
          if (baseTypePtr->sParm.t.tType == sRECORD)
            {
              /* REVISIT:  This is the problem case */
            }
          else
#endif
          if (baseTypePtr->sParm.t.tType != sFILE &&
              baseTypePtr->sParm.t.tType != sTEXTFILE)
            {
              return pas_DefaultFileNumber(defaultFilePtr, pFileSize);
            }

          /* Skip over the array name */

          getToken();

          /* Then handle the bracketed array index.  Generate the array
           * offset calculation.
           */

          pas_ArrayIndex(typePtr);

          /* Return the parent type of the array */

          varPtr->sKind         = baseTypePtr->sParm.t.tType;
          varPtr->sParm.v.vSize = baseTypePtr->sParm.t.tAllocSize;

          return pas_SimplifyFileNumber(varPtr, fileFlags, pFileSize,
                                        defaultFilePtr);
        }

      /* Is this a variable representing a type binary or text FILE? */

      case sFILE :
      case sTEXTFILE :
        {
          fileType = varPtr->sKind;
          if (g_token == sTEXTFILE)
            {
              fileSize = sCHAR_SIZE;
            }
          else
            {
              fileSize = varPtr->sParm.v.vXfrUnit;
            }

          pas_GenerateStackReference(opLDS, varPtr);

          /* Skip over the variable identifer */

          getToken();
        }
        break;

      /* Not a file-type variable (or it is a more exotic form that is
       * not yet implemented).  Use the default file number (which we
       * assume to be of type sTEXTFILE)
       */

      default :
        return pas_DefaultFileNumber(defaultFilePtr, pFileSize);
    }

  if (pFileSize != NULL)
    {
      *pFileSize = fileSize;
    }

  return fileType;
}

/***********************************************************************/

static uint16_t pas_DefaultFileNumber(symbol_t *defaultFilePtr,
                                      uint16_t *pFileSize)
{
  /* Push the default file number */

  pas_GenerateStackReference(opLDS, defaultFilePtr);

  /* Return the file type and size for a TEXTFILE */

  if (pFileSize != NULL)
    {
      *pFileSize = defaultFilePtr->sParm.v.vXfrUnit;
    }

  return defaultFilePtr->sKind;
}

/***********************************************************************/

static void pas_ExitProc(void)
{
  exprType_t exprType;

  /* FORM (Non-Standard): exit '(' exit-code '); */

  getToken();
  if (g_token != '(') error(eLPAREN);  /* Skip over '(' */
  else getToken();

  /* The argument should be an integer value */

  exprType = pas_Expression(exprInteger, NULL);
  if (exprType != exprInteger)
    {
      error(eINVARG);
    }

  if (g_token != ')') error(eRPAREN);  /* Skip over ')' */
  else getToken();

  pas_StandardFunctionCall(lbEXIT);
}

/***********************************************************************/

static void pas_HaltProc(void)
{
  /* FORM:
   *   halt
   */

  getToken();
  pas_GenerateDataOperation(opPUSH, 0);
  pas_StandardFunctionCall(lbEXIT);
}

/****************************************************************************/

static void pas_ReadProc(void)  /* READ procedure */
{
  uint16_t fileType;  /* sFILE or sTEXTFILE */
  uint16_t fileSize;  /* Size asociated with sFILE type */

  TRACE(g_lstFile, "[pas_ReadProc]");

  /* Handles read-parameter-list
   *
   * FORM: READ read-parameter-list
   * FORM: read-parameter-list =
   *       '(' [ file-variable ',' ] variable-access { ',' variable-access } ')'
   * FORM  variable-access = entire-variable | component-variable | identified-variable |
   *       selected-variable | buffer-variable
   */

  getToken();                          /* Skip over WRITE */

  /* The write-parameter-list is not optional with WRITE */

  if (g_token != '(') error(eLPAREN);  /* Skip over '(' */
  else getToken();

  /* There must be at one variable-access argument (for WRITE) */

  if (g_token == ')') error(eNOWRITEPARM);

  /* Get TOS = file number */

  fileType = pas_GenerateFileNumber(&fileSize, g_inputFile);

  /* Since there may or may not be a file number, there may or may
   * not be a comma present on the first time through this loop
   * either.
   */

  if (g_token == ',') getToken();

  /* Process the rest of the write-parameter-list */

  pas_ReadProcCommon((fileType == sTEXTFILE), fileSize);

  /* Discard the extra file number argument on the stack.  NOTE that the
   * file number is retained for READLN to handle the move to end-of-line
   * operation.
   */

  pas_GenerateDataOperation(opINDS, -sINT_SIZE);

  /* Verify that the write-parameter-list was terminated with a ')' */

  if (g_token != ')') error (eRPAREN);
  else getToken();
}

/****************************************************************************/

static void pas_ReadlnProc(void)  /* READLN procedure */
{
  uint16_t fileType;  /* sFILE or sTEXTFILE */
  uint16_t fileSize;  /* Size asociated with sFILE type */

  TRACE(g_lstFile, "[pas_ReadlnProc]");

  /* Handles read-parameter-list
   *
   * FORM: READLN read-parameter-list
   * FORM: read-parameter-list =
   *       '(' [ file-variable ',' ] variable-access { ',' variable-access } ')'
   * FORM  variable-access = entire-variable | component-variable | identified-variable |
   *       selected-variable | buffer-variable
   */

  getToken();                         /* Skip over READLN */

  /* The read-parameter-list is optional with READLN */

  if (g_token == '(')
    {
      getToken();

      /* Check for empty read-parameter-list */

      if (g_token == ')')
        {
          /* No read-parameter-list.  Use the INPUT file number */

          pas_GenerateStackReference(opLDS, g_inputFile);
        }
      else
        {
          /* Get TOS = file number */

          fileType = pas_GenerateFileNumber(&fileSize, g_inputFile);

          /* Since there may or may not be a file number, there may or may
           * not be a comma present on the first time through this loop
           * either.
           */

          if (g_token == ',') getToken();

          /* READLN for a binary file does not make sense */

          if (fileType != sTEXTFILE) error(eINVFILE);

          /* Process the rest of the write-parameter-list */

          pas_ReadProcCommon(true, fileSize);
        }

      /* If there was an opening ')' then there most alos be a matching
       * closing ')'.
       */

      if (g_token != ')') error(eRPAREN);
      else getToken();
    }
  else
    {
      /* No read-parameter-list.  Use the INPUT file number */

      pas_GenerateStackReference(opLDS, g_inputFile);
    }

  /* Set the end-of-line in the file. */

  pas_GenerateIoOperation(xREADLN);
}

/***********************************************************************/

/* Handles read-parameter-list
 *
 * FORM: READ|READLN read-parameter-list
 * FORM: read-parameter-list =
 *       '(' [ file-variable ',' ] variable-access { ',' variable-access } ')'
 * FORM  variable-access = entire-variable | component-variable | identified-variable |
 *       selected-variable | buffer-variable
 *
 * REVISIT:  Only entire-variable supported
 *
 * file-variable:  As an optional first parameter a TEXTFILE file  can be
 *                 specified where data are read from. Read is additionally
 *                 capable of reading from a typed file variable (FILE OF
 *                 recordType).  If no source is specified, input is assumed.
 *
 * variable-access Followed by any number of variables can be specified, but
 *                 at least one has to be present. They have to be either
 *                 INTEGER, CHAR, REAL, or STRING. If a typed file is the
 *                 source, then all variables have to be of the file’s
 *                 underlying base type.
 */

static void pas_ReadProcCommon(bool text, uint16_t fileSize)
{
  TRACE(g_lstFile, "[pas_ReadProcCommon]");

  /* On entry, g_token should refer to the the first variable-access.  The
   * caller has assure that g_token is not ',' or ')' before calling.
   *
   * Loop processing each expression
   */

  for (; ; )
    {
      /* Determine if this is a text or binary file */

      if (text)
        {
          pas_ReadText();
        }
      else
        {
          pas_ReadBinary(fileSize);
        }

      /* Should be followed by ',' meaning that there are more parameter, or
       * by ')' meaning that we have processed all of the parameters.
       */

      if (g_token == ':' || g_token == ',') getToken();
      else break;
    }
}

/***********************************************************************/

/* Handles READ and READLN text file access-list
 *
 * FORM: READ|READLN read-parameter-list
 * FORM: read-parameter-list =
 *       '(' [ file-variable ',' ] variable-access { ',' variable-access } ')'
 *
 * On entry token refers to a variable-access, the file number is at the
 * top of the stack.
 *
 * variable-access Has to be either INTEGER, CHAR, REAL, or STRING.
 */

static void pas_ReadText(void)
{
  exprType_t exprType;

  TRACE(g_lstFile, "[pas_ReadText]");

  /* Get TOS = file number by duplicating the one that is already there (but
   * that we also need to preserve).
   */

  pas_GenerateSimple(opDUP);

  /* pas_VarParameter() will place a pointer to the variable to read at the
   * TOS and return the address of the pointer.
   */

  exprType = pas_VarParameter(exprUnknown, NULL);
  switch (exprType)
    {
      /* READ_INT: TOS(0) = Read address
       *           TOS(1) = File number
       */

      case exprIntegerPtr :
        pas_GenerateIoOperation(xREAD_INT);
        break;

      /* READ_CHAR: TOS(0) = Read address
       *            TOS(1) = File number
       */

      case exprCharPtr :
        pas_GenerateIoOperation(xREAD_CHAR);
        break;

      /* READ_REAL: TOS(0) = Read address
       *            TOS(1) = File number
       */

      case exprRealPtr :
        pas_GenerateIoOperation(xREAD_REAL);
        break;

      /* READ_STRING: TOS(0) = Address of string variable
       *              TOS(1) = File number
       *
       * REVISIT:  Won't that be the current string size?  Not the
       * maximum read size?
       */

      case exprStringPtr :
        pas_GenerateIoOperation(xREAD_STRING);
        break;

      case exprShortStringPtr :
        pas_GenerateIoOperation(xREAD_SHORTSTRING);
        break;

      default :
        error(eINVARG);
        break;
    }
}

/***********************************************************************/

/* Handles READ binary file access list
 *
 * FORM: READ read-parameter-list
 * FORM: read-parameter-list =
 *       '(' [ file-variable ',' ] variable-access { ',' variable-access } ')'
 * FORM  variable-access = entire-variable | component-variable | identified-variable |
 *       selected-variable | buffer-variable
 */

static void pas_ReadBinary(uint16_t fileSize)
{
  symbol_t *parent;
  uint16_t size;

  TRACE(g_lstFile, "[pas_ReadBinary]");

  /* It is binary.  Make sure that the token refers to a variable
   * with the same type as the FILE OF.
   *
   * REVISIT:  Here we just check only that the two types are the
   *           same size.
   *
   * Need some verification that the thing we encountered is really a
   * variable before access sParm.v.
   */

  switch (g_tknPtr->sKind)
    {
      /* Simple ordinal types */

      case sINT :
      case sWORD :
      case sSHORTINT :
      case sSHORTWORD :
      case sLONGINT :
      case sLONGWORD :
      case sBOOLEAN :
      case sCHAR :
      case sREAL :

      /* Complex types */

      case sSTRING :
      case sSHORTSTRING :
      case sARRAY :
      case sRECORD :
        size = g_tknPtr->sParm.v.vSize;
        break;

      /* VAR parameter */

      case sVAR_PARM :
        /* REVISIT:  Need to check the type of the parent.  Assuming
         * that the parent is an sTYPE
         */

        parent = g_tknPtr->sParm.v.vParent;
        size   = parent->sParm.t.tAllocSize;
        break;

#if 0 /* Not yet */
      /* Defined types */

      case sTYPE :
        size   = g_tknPtr->sParm.t.tAllocSize;
        break;
#endif

      default :
        error(eREADPARMTYPE);
        break;
    }

  if (size != fileSize) error(eREADPARMTYPE);
  else
    {
      /* READ_BINARY: TOS(0) = Read address
       *              TOS(1) = Read size
       *              TOS(2)  = File number
       */

      pas_GenerateSimple(opDUP);
      pas_GenerateDataOperation(opPUSH, size);
      pas_GenerateStackReference(opLAS, g_tknPtr);
      pas_GenerateIoOperation(xREAD_BINARY);
    }

  /* Skip over the variable-access */

  getToken();
}

/****************************************************************************/
/* This function handles file procedure calls that have a file number
 * argument followed by an integer argument:
 *
 *  FORM: open-procedure-name '(' file-variable {, record-size} ')'
 *
 * Includes RESET and REWRITE:
 *
 * - REWRITE sets the file pointer to the beginning of the file and prepares
 *   the file for write access.  It will also reset the record size if the
 *   record-size argument is present.
 * - RESET is similar to REWRITE except that it prepares the file for read
 *   access;
 *
 * APPEND also opens a file, but is handled by pas_FileProc().  This function
 * is almost identical to pas_FileProc(), differing only in that it accepts
 * two opcodes and handles the option record-size.
 */

static void pas_OpenFileProc(uint16_t opcode1, uint16_t opcode2)
{
  uint16_t opcode = opcode1;

  TRACE(g_lstFile, "[pas_OpenFileProc]");

  /* FORM: open-procedure-name '(' file-variable {, record-size} ')'
   * FORM: open-procedure-name = REWRITE | RESET
   *
   * On entry, g_token refers to the reserved procedure name.  It may
   * be followed with a argument list.
   */

  getToken();
  if (g_token == '(')
    {
      /* Push the file-number at the top of the stack */

      getToken();
      (void)pas_GenerateFileNumber(NULL, g_outputFile);

      /* Check for the option record-size */

      if (g_token == ',')
        {
          getToken();
          (void)pas_Expression(exprInteger, NULL);
          opcode = opcode2;
        }

      /* And generate the opcode */

      pas_GenerateIoOperation(opcode);

      /* Verify the closing right parenthesis */

      if (g_token != ')') error(eRPAREN);
      else getToken();
    }
  else
    {
      /* Assume the standard OUTPUT file? */

      pas_GenerateStackReference(opLDS, g_outputFile);

      /* And generate the opcode1 */

      pas_GenerateIoOperation(opcode);
    }
}

/****************************************************************************/
/* All file I/O procedures that have a single, file number argument.
 * Includes PAGE, APPEND, and CLOSEFILE procedure calls
 *
 * - PAGE simply writes a form-feed to the file (no check is made, but is
 *   meaningful only for a text file).
 * - APPEND prepars the file for appeand access.  It is similar to RESET and
 *   REWRITE but has no optional record-size argument.
 * - CLOSEFILE closes a previously opened file
 */

static void pas_FileProc(uint16_t opcode)
{
  TRACE(g_lstFile, "[pas_FileProc]");

  /* FORM: function-name(<file number>)
   * FORM: function-name = PAGE | APPEND | CLOSEFILE
   *
   * On entry, g_token refers to the reserved procedure name.  It may
   * be followed with a argument list.
   */

  getToken();
  if (g_token == '(')
    {
      /* Push the file-number at the top of the stack */

      getToken();
      (void)pas_GenerateFileNumber(NULL, g_outputFile);

      /* And generate the opcode */

      pas_GenerateIoOperation(opcode);

      /* Verify the closing right parenthesis */

      if (g_token != ')') error(eRPAREN);
      else getToken();
    }
  else
    {
      /* Assume the standard OUTPUT file? */

      pas_GenerateStackReference(opLDS, g_outputFile);

      /* And generate the opcode */

      pas_GenerateIoOperation(opcode);
    }
}

/****************************************************************************/

static void pas_AssignFileProc(void)  /* ASSIGNFILE procedure */
{
  exprType_t exprType;
  uint32_t   fileType;

  TRACE(g_lstFile, "[pas_AssignFileProc]");

  /* FORM: ASSIGNFILE|ASSIGN assignfile-parameter-list ';'
   * FORM: assignfile-parameter-list = '(' file-variable ',' file-name ')'
   * FORM: file-variable = file-variable | typed-file-variable | textfile-variable
   * FORM: file-name = string-variable
   *
   * On entry g_token refers to the ASSIGNFILE|ASSIGN reserved word
   */

   getToken();
   if (g_token != '(') error(eLPAREN);
   else
     {
       /* Skip to the file-variable token */

       getToken();

       /* Get TOS(0) = Pointer to string
        *     TOS(1) = 0:binary 1:text
        *     TOS(2) = File number
        */

       /* Push the file number onto the TOS */

       fileType = pas_GenerateFileNumber(NULL, g_outputFile);

       /* If the default as not used and a file variable provided as the
        * first argument, then it must be followed by a comma.
        */

       if (g_token == ',')
        {
          getToken();
        }

       /* Push the file type:  binary or text */

       pas_GenerateDataOperation(opPUSH, (uint16_t)(fileType == sTEXTFILE));

       /* Push the file name reference onto the stack. */

       exprType = pas_Expression(exprUnknown, NULL);
       if (exprType != exprString) error(eSTRING);

       /* And generate the SYSIO operation */

       pas_GenerateIoOperation(xASSIGNFILE);

       /* Make sure that the matching right parenthesis is present */

       if (g_token != ')') error(eRPAREN);
       else getToken();
     }
}

/****************************************************************************/

static void pas_WriteProc(void)  /* WRITE procedure */
{
  uint16_t fileType;  /* sFILE or sTEXTFILE */
  uint16_t fileSize;  /* Size asociated with sFILE type */

   TRACE(g_lstFile, "[pas_WriteProc]");

  /* FORM: WRITE write-parameter-list
   * FORM:       write-parameter-list = '(' [ file-variable ',' ]
   *             write-parameter { ',' write-parameter } ')'
   * FORM:       write-parameter = expression [ ':' expression [ ':' expression ] ]
   */

  getToken();                          /* Skip over WRITE */

  /* The write-parameter-list is not optional with WRITE */

  if (g_token != '(') error(eLPAREN);  /* Skip over '(' */
  else getToken();

  /* There must be at one variable-access argument (for WRITE) */

  if (g_token == ')') error(eNOWRITEPARM);

  /* Get TOS = file number */

  fileType = pas_GenerateFileNumber(&fileSize, g_outputFile);

  /* Since there may or may not be a file number, there may or may
   * not be a comma present on the first time through this loop
   * either.
   */

  if (g_token == ',') getToken();

  /* Process the rest of the write-parameter-list */

  pas_WriteProcCommon((fileType == sTEXTFILE), fileSize);

  /* Discard the extra file number argument on the stack.  NOTE that the
   * file number is retained for READLN to handle the move to end-of-line
   * operation.
   */

  pas_GenerateDataOperation(opINDS, -sINT_SIZE);

  /* Verify that the write-parameter-list was terminated with a ')' */

  if (g_token != ')') error (eRPAREN);
  else getToken();
}

/****************************************************************************/

static void pas_WritelnProc(void)  /* WRITELN procedure */
{
  uint16_t fileType;  /* sFILE or sTEXTFILE */
  uint16_t fileSize;  /* Size asociated with sFILE type */

  TRACE(g_lstFile, "[pas_WritelnProc]");

  /* FORM: WRITELN writeln-parameter-list
   * FORM:      writeln-parameter-list = [ write-parameter-list ]
   * FORM:      write-parameter = expression [ ':' expression [ ':' expression ] ]
   */

  getToken();                         /* Skip over WRITELN */

  /* The writeln-parameter-list is optional with WRITELN */

  if (g_token == '(')
    {
      getToken();

      /* Check for empty writeln-parameter-list */

      if (g_token == ')')
        {
          /* No writeln-parameter-list.  Use the OUTPUT file number */

          pas_GenerateStackReference(opLDS, g_outputFile);
        }
      else
        {
          /* Get TOS = file number */

          fileType = pas_GenerateFileNumber(&fileSize, g_outputFile);

          /* Since there may or may not be a file number, there may or may
           * not be a comma present on the first time through this loop
           * either.
           */

          if (g_token == ',') getToken();

          /* WRITELN for a binary file does not make sense */

          if (fileType != sTEXTFILE) error(eINVFILE);

          /* Process the rest of the write-parameter-list */

          pas_WriteProcCommon(true, fileSize);
        }

      /* If there was an opening ')' then there most alos be a matching
       * closing ')'.
       */

      if (g_token != ')') error(eRPAREN);
      else getToken();
    }
  else
    {
      /* No writeln-parameter-list.  Use the OUTPUT file number */

      pas_GenerateStackReference(opLDS, g_outputFile);
    }

  /* Set the end-of-line in the file.
   *
   * WRITELN: TOS = File number.  For WRITELN, pas_WriteProcCommon() will
   * leave the file number at the top of the stack
   */

  pas_GenerateIoOperation(xWRITELN);
}

/***********************************************************************/

static void pas_WriteProcCommon(bool text, uint16_t fileSize)
{
  TRACE(g_lstFile, "[pas_WriteProcCommon]");

  /* Handle the WRITE/WRITELN write-parameter
   *
   * FORM:  write-parameter-list = '(' [ file-variable ',' ]
   *        write-parameter { ',' write-parameter } ')'
   * FORM:  write-parameter = expression [ ':' expression [ ':' expression ] ]
   */

  /* On entry, g_token should refer to the the first expression.  The caller
   * has assure that g_token is not ',' or ')' before calling.
   *
   * Loop processing each expression
   */

  for (; ; )
    {
      /* Determine if this is a text or binary file */

      if (text)
        {
          pas_WriteText();
        }
      else
        {
          pas_WriteBinary(fileSize);
        }

      /* Should be followed by ',' meaning that there are more parameter, or
       * by ')' meaning that we have processed all of the parameters.
       */

      if (g_token == ',') getToken();
      else break;
    }
}

/***********************************************************************/

static void pas_WriteText(void)
{
  TRACE(g_lstFile, "[pas_WriteText]");

  /* The general form is <expression> */

  switch (g_token)
    {
      /* const strings -- either literal constants (tSTRING_CONST)
       * or defined string constant symbols (sSTRING_CONST)
       */

    case tSTRING_CONST :
      {
        /* Add the literal string constant to the RO data section
         * and receive the offset to the data.
         */

        uint32_t offset     = poffAddRoDataString(g_poffHandle, g_tokenString);
        int      size       = strlen(g_tokenString);
        uint16_t fieldWidth = pas_WriteFieldWidth();

        /* Set the file number, offset and size on the stack
         *
         * WRITE_STRING: TOS(0) = Field width
         *               TOS(1) = Write address
         *               TOS(2) = Write size
         *               TOS(3) = File number
         */

        pas_GenerateSimple(opDUP);
        pas_GenerateDataOperation(opPUSH, size);
        pas_GenerateDataOperation(opLAC, (uint16_t)offset);
        pas_GenerateDataOperation(opPUSH, fieldWidth);
        pas_GenerateIoOperation(xWRITE_STRING);

        g_stringSP = g_tokenString;
        getToken();
      }
      break;

    case sSTRING_CONST :
      {
        uint32_t offset     = (uint16_t)g_tknPtr->sParm.s.roOffset;
        int      size       = (uint16_t)g_tknPtr->sParm.s.roSize;
        uint16_t fieldWidth = pas_WriteFieldWidth();

        /* WRITE_STRING: TOS(0) = Field width
         *               TOS(1) = Write address
         *               TOS(2) = Write size
         *               TOS(3) = File number
         */

        pas_GenerateSimple(opDUP);
        pas_GenerateDataOperation(opPUSH, size);
        pas_GenerateDataOperation(opLAC, offset);
        pas_GenerateDataOperation(opPUSH, fieldWidth);
        pas_GenerateIoOperation(xWRITE_STRING);

        getToken();
      }
      break;

      /* Array of type CHAR without indexing */

    case sARRAY :
      {
        symbol_t *wPtr;
        uint16_t  fieldWidth;

        wPtr = g_tknPtr->sParm.v.vParent;
        if (wPtr != NULL && wPtr->sKind == sTYPE &&
            wPtr->sParm.t.tType == sCHAR &&
            pas_GetNextCharacter(true) != '[')
          {
            /* WRITE_STRING: TOS(0) = Field Width
             *               TOS(1) = Write address
             *               TOS(2) = Write size
             *               TOS(3) = File number
             */

            fieldWidth = pas_WriteFieldWidth();

            pas_GenerateSimple(opDUP);
            pas_GenerateDataOperation(opPUSH, wPtr->sParm.v.vSize);
            pas_GenerateStackReference(opLAS, wPtr);
            pas_GenerateDataOperation(opPUSH, fieldWidth);
            pas_GenerateIoOperation(xWRITE_STRING);
            getToken();
            break;
          }
      }

      /* Otherwise, we fall through to process the ARRAY like any
       * expression.
       */

    default :
      {
        exprType_t writeType;

        /* Put the file number, value, and field width on the stack */

        pas_GenerateSimple(opDUP);
        writeType = pas_Expression(exprUnknown, NULL);
        pas_GenerateDataOperation(opPUSH, pas_WriteFieldWidth());

        /* Then generate the operation */

        switch (writeType)
          {
          case exprInteger :
          case exprShortInteger :
            /* WRITE_INT: TOS(0) = Field width
             *            TOS(1) = Write value (signed)
             *            TOS(2) = File number
             */

            pas_GenerateIoOperation(xWRITE_INT);
            break;

          case exprWord :
          case exprShortWord :
            /* WRITE_INT: TOS(0) = Field width
             *            TOS(1) = Write value (unsiged)
             *            TOS(2) = File number
             */

            pas_GenerateIoOperation(xWRITE_WORD);
            break;

          case exprLongInteger :
            /* WRITE_LONGINT: TOS(0)   = Field width
             *                TOS(1-2) = Write value (signed)
             *                TOS(3)   = File number
             */

            pas_GenerateIoOperation(xWRITE_LONGINT);
            break;

          case exprLongWord :
            /* WRITE_LONGWORD: TOS(0)   = Field width
             *                 TOS(1-2) = Write value (unsigned)
             *                 TOS(3)   = File number
             */

            pas_GenerateIoOperation(xWRITE_LONGWORD);
            break;

          case exprChar :
            /* WRITE_CHAR: TOS(0) = Field width
             *             TOS(1) = Write value
             *             TOS(2) = File number
             */

            pas_GenerateIoOperation(xWRITE_CHAR);
            break;

          case exprReal :
            /* WRITE_CHAR: TOS(0)   = Field width/precision
             *             TOS(1-4) = Write value
             *             TOS(5)   = File number
             */

            pas_GenerateIoOperation(xWRITE_REAL);
            break;

          case exprString :
            /* WRITE_STRING: TOS(0) = Field width
             *               TOS(1) = Write address
             *               TOS(2) = Write size
             *               TOS(3) = File number
             */

            pas_GenerateIoOperation(xWRITE_STRING);
            break;

          case exprShortString :
            /* WRITE_SHORTSTRING: TOS(0) = Field width
             *                    TOS(1) = Write address
             *                    TOS(2) = Write size
             *                    TOS(3) = String allocation size (not used)
             *                    TOS(4) = File number
             */

            pas_GenerateIoOperation(xWRITE_SHORTSTRING);
            break;

          default :
            error(eWRITEPARM);
            break;
          }
      }
    }
}

/***********************************************************************/

/* Get text file write field-width. */

static uint16_t pas_WriteFieldWidth(void)
{
  uint8_t fieldWidth = 0;
  uint8_t precision  = 0;

  TRACE(g_lstFile, "[pas_WriteFieldWidth]");

  /* If a field width/precision is present, then the current token will
   * be ':'
   *
   * FORM:  write-value [ : field-width [ : precision ]]
   */

  if (g_token == ':')
    {
      /* A constant, integer field-width with must follow the colon. */

      getToken();
      pas_ConstantExpression(exprInteger, NULL);
      if (g_constantToken != tINT_CONST || g_constantInt < 0 ||
          g_constantInt > UINT8_MAX)
        {
          error(eBADFIELDWIDTH);
        }

      fieldWidth = (uint8_t)g_constantInt;

      /* Check if a precision is present after the field-width (only applies
       * to REAL values.
       */

      if (g_token == ':')
        {
          /* A constant, integer precision with must follow the colon.
           * REVISIT:  Could this be an integer expression?
           */

          getToken();
          pas_ConstantExpression(exprInteger, NULL);
          if (g_constantToken != tINT_CONST || g_constantInt < 0 ||
              g_constantInt > fieldWidth)
            {
              error(eBADPRECISION);
            }

          precision = (uint8_t)g_constantInt;
        }
    }

  return ((uint16_t)fieldWidth << 8) | ((uint16_t)precision & 0xff);
}

/***********************************************************************/

static void pas_WriteBinary(uint16_t fileSize)
{
  symbol_t *parent;
  uint16_t size;

  TRACE(g_lstFile, "[pas_WriteBinary]");

  /* It is binary.  Make sure that the token refers to a variable
   * with the same type as the FILE OF.
   *
   * REVISIT:  Here we just check only that the two types are the
   *           same size.
   *
   * Need some verification that the thing we encountered is really a
   * variable before access sParm.v.
   */

  switch (g_tknPtr->sKind)
    {
      /* Simple ordinal types */

      case sINT :
      case sWORD :
      case sSHORTINT :
      case sSHORTWORD :
      case sLONGINT :
      case sLONGWORD :
      case sBOOLEAN :
      case sCHAR :
      case sREAL :

      /* Complex types */

      case sSTRING :
      case sSHORTSTRING :
      case sARRAY :
      case sRECORD :
        size = g_tknPtr->sParm.v.vSize;
        break;

      /* VAR parameter */

      case sVAR_PARM :
        /* REVISIT:  Need to check the type of the parent.  Assuming
         * that the parent is an sTYPE
         */

        parent = g_tknPtr->sParm.v.vParent;
        size   = parent->sParm.t.tAllocSize;
        break;

#if 0 /* Not yet */
      /* Defined types */

      case sTYPE :
        size   = g_tknPtr->sParm.t.tAllocSize;
        break;
#endif

      default :
        error(eWRITEPARMTYPE);
        break;
    }

  if (size != fileSize) error(eWRITEPARMTYPE);
  else
    {
      /* WRITE_BINARY: TOS(0) = Write address
       *               TOS(1) = Write size
       *               TOS(2)  = File number
       */

      pas_GenerateSimple(opDUP);
      pas_GenerateDataOperation(opPUSH, size);
      pas_GenerateStackReference(opLAS, g_tknPtr);
      pas_GenerateIoOperation(xWRITE_BINARY);
    }

  getToken();
}

/****************************************************************************/
/* Memory allocator */

static void pas_NewProc(void)
{
  /* FORM:  'new' '(' pointer-variable ')' */

  TRACE(g_lstFile,"[pas_NewProc]");

  getToken();
  if (g_token != '(') error(eLPAREN);  /* Skip over '(' */
  else getToken();

  /* Check for pointer (or VAR parm) variable */

  if (g_token == sPOINTER || g_token == sVAR_PARM)
    {
      symbol_t *baseTypePtr;
      symbol_t *varPtr;
      symbol_t *typePtr;
      uint16_t  varType;

      varPtr = g_tknPtr;
      getToken();

      /* The variable must be a pointer type */

      typePtr = varPtr->sParm.v.vParent;
      if (typePtr->sParm.t.tType != sPOINTER)
        {
          error(ePOINTERTYPE);
        }
      else
        {
          /* Get the size of the allocation.  We want the size of the parent of
           * this pointer type.  This would not be meaningful for the case of
           * a pointer-to-a-pointer, but let's do as we are told.
           */

          symbol_t *parentTypePtr = typePtr->sParm.t.tParent;

          /* Allocate memory for an object the size of an allocated instance of
           * this type.  A pointer to the allocated memory will lie at the top of
           * the stack at run-time.
           */

          pas_GenerateDataOperation(opPUSH, parentTypePtr->sParm.t.tAllocSize);
          pas_StandardFunctionCall(lbNEW);

          /* Save this into the pointer variable */

          pas_GenerateStackReference(opSTS, varPtr);

          /* If we just allocated a string, shortstring, or file type, then we
           * have to initialize the allocated instance.
           */

          baseTypePtr = pas_GetBaseTypePointer(parentTypePtr);
          varType     = baseTypePtr->sParm.t.tType;

          /* If we just created a string variable, then set up and initializer
           * for the string; memory for the string buffer must be set up at run
           * time.
           */

          if (varType == sSTRING || varType == sSHORTSTRING)
            {
              pas_InitializeNewString(baseTypePtr);
            }

          /* Handle files similarly */

          else if (varType == sFILE || varType == sTEXTFILE)
            {
              pas_InitializeNewFile(baseTypePtr);
            }

          /* A more complex case:  We just created a RECORD variable that may
           * contain string or file fields that need to be initialized.
           */

          else if (varType == sRECORD)
            {
              pas_InitializeNewRecord(baseTypePtr);
            }

          /* Or an array that may contain variables that need initialization.
           * (OR an array or records with fields that are arrays that ... and
           * all need to be initialized).
           */

          else if (parentTypePtr->sParm.t.tType == sARRAY)
            {
              pas_InitializeNewArray(parentTypePtr);
            }
        }
    }

  if (g_token != ')') error(eRPAREN);  /* Skip over ')' */
  else getToken();
}

/****************************************************************************/
/* Free memory */

static void pas_DisposeProc(void)
{
  exprType_t exprType;
  symbol_t *varPtr;

  /* FORM:  'dispose' '(' pointer-value ')' */

  getToken();
  if (g_token != '(') error(eLPAREN);  /* Skip over '(' */
  else getToken();

  /* The argument should be a pointer or perhaps a VAR parameter */

  varPtr   = g_tknPtr;
  exprType = pas_Expression(exprAnyPointer, NULL);
  if (!IS_POINTER_EXPRTYPE(exprType))
    {
      error(ePOINTERTYPE);
    }
  else
    {
      symbol_t *baseTypePtr = pas_GetBaseTypePointer(varPtr->sParm.v.vParent);
      uint16_t baseType     = baseTypePtr->sParm.t.tType;

      /* Free allocated file numbers. */

      if (baseType == sFILE || baseType == sTEXTFILE)
        {
          /* Generate logic to free the file number */

          pas_FinalizeNewFile(varPtr);
        }

      /* Free the allocated memory. pas_Expression() left the pointer value
       * at the top of the stack.  We need only issue the library call to
       * free it.
       */

      pas_StandardFunctionCall(lbDISPOSE);
    }

  if (g_token != ')') error(eRPAREN);  /* Skip over ')' */
  else getToken();
}

/****************************************************************************/
/* The VAR file parameter is more complex than the "normal" file variable
 * because the transfer unit size is more difficult to find.
 */

static uint16_t pas_GenVarFileNumber(symbol_t *varPtr, uint16_t *pFileSize,
                                     symbol_t *defaultFilePtr)
{
  symbol_t *typePtr;  /* The base type may be in a different symbol */
  uint16_t  symType;  /* The base type may not be a symbol */
  uint16_t  fileType;
  uint16_t  fileSize;

  /* Use the parent type.  The parent of a a VAR parameter is a type */

  typePtr  = varPtr->sParm.v.vParent;
  symType  = 0;  /* Invalid type */
  fileType = 0;  /* Invalid type */
  fileSize = 0;  /* Invalid transfer unit size */

  /* Is this a symbole whose type is typedef'ed?  This will be a sequence of
   * types referenced from the variable.  One of the intermediate types will
   * be sFILE or sTEXTFILE.  Another sTYPE symbol will appear after the sFILE
   * entry to refer to the FILE OF type.
   */

  while (typePtr != NULL && typePtr->sKind == sTYPE)
    {
      /* Get the type of the parent */

      symType = typePtr->sParm.t.tType;

      /* Check if this is a file in the sequence of types. */

      if (symType == sFILE)
        {
          /* For binary files, we find the transfer unit size in the asize
           * field.
           */

          fileSize = typePtr->sParm.t.tAllocSize;
          fileType = sFILE;
          break;
        }
      else if (symType == sTEXTFILE)
        {
          /* For text files, the size is always the size of one character */

          fileSize = sCHAR_SIZE;
          fileType = sTEXTFILE;
          break;
        }
      else
        {
          symbol_t *tmpPtr  = typePtr->sParm.t.tParent;

          /* This is the final type if it has no parent. */

          if (tmpPtr != NULL)
            {
              typePtr = tmpPtr;
            }
          else
            {
              break;
            }
        }
    }

  /* Is this a variable representing a type binary or text FILE? */

  if (symType == sFILE || symType == sTEXTFILE)
    {
      /* Since this is a VAR parameter, we need to push the address of the
       * file variable and dereference that address.
       */

      pas_GenerateStackReference(opLDS, varPtr);
      pas_GenerateSimple(opLDI);

      /* Skip over the variable identifer */

      getToken();
    }

  /* Not a file-type variable.  Use the default file number (which we assume
   * to be of type sTEXTFILE)
   */

  else
    {
      return pas_DefaultFileNumber(defaultFilePtr, pFileSize);
    }

  if (pFileSize != NULL)
    {
      *pFileSize = fileSize;
    }

  return fileType;
}

/****************************************************************************/

static void pas_ValProc(void)  /* VAL procedure */
{
  TRACE(g_lstFile, "[pas_ValProc]");

  /* Declaration:
   *   procedure val(const S : string; var V; var Code : word);
   *
   * Description:
   * val() converts the value represented in the string S to a numerical
   * value, and stores this value in the variable V, which can be of type
   * Longint, Real and Byte. If the conversion isn��t succesfull, then the
   * parameter Code contains the index of the character in S which
   * prevented the conversion. The string S is allowed to contain spaces
   * in the beginning.
   *
   * The string S can contain a number in decimal, hexadecimal, binary or
   * octal format, as described in the language reference.
   *
   * Errors:
   * If the conversion doesn��t succeed, the value of Code indicates the
   * position where the conversion went wrong.
   */

  /* Skip over the 'val' identifer */

  getToken();

  /* Setup the actual-parameter-list */

  (void)pas_ActualParameterList(valSymbol);

  /* Generate the built-in procedure call.  NOTE the procedure call
   * logic will release the parameters from the stack saving us from
   * having to generate the INDS here.
   */

  pas_StandardFunctionCall(lbVAL);
}

/****************************************************************************/

static void pas_InitializeNewRecord(symbol_t *typePtr)
{
  /* Verify that this is a RECORD type */

  if (typePtr == NULL ||
      typePtr->sKind != sTYPE ||
      typePtr->sParm.t.tType != sRECORD)
    {
      error(eRECORDTYPE);
    }

  /* Looks like a good RECORD type.  On entry, a pointer to the RECORD to
   * be initialized will be at the top of the stack.
   */

  else
    {
      symbol_t *recordObjectPtr;
      int nObjects = typePtr->sParm.t.tMaxValue;
      int objectIndex;

      /* The parent is the RECORD type.  That is followed by the
       * RECORD OBJECT symbols.  The number of following RECORD
       * OBJECT symbols is given by the maxValue field of the
       * RECORD type entry.
       *
       * RECORD OBJECTS may not be contiguous but may be interspersed
       * with spurious (un-named) type symbols.  The first RECORD
       * OBJECT symbol is, however, guaranteed to immediately follow
       * the RECORD type.
       */

      for (objectIndex = 1, recordObjectPtr = &typePtr[1];
           objectIndex <= nObjects && recordObjectPtr != NULL;
           objectIndex++, recordObjectPtr = recordObjectPtr->sParm.r.rNext)
        {
          symbol_t *parentTypePtr;

          if (recordObjectPtr->sKind != sRECORD_OBJECT)
            {
              /* The symbol table must be corrupted */

              error(eHUH);
            }

          /* If this field is a string, then set up to initialize it.
           * At run-time, a pointer to the allocated RECORD will be
           * at the top of the stack.
           */

          parentTypePtr = recordObjectPtr->sParm.r.rParent;

          if (parentTypePtr == NULL || parentTypePtr->sKind != sTYPE)
            {
              error(eHUH);
            }
          else if (parentTypePtr->sParm.t.tType == sSTRING ||
                   parentTypePtr->sParm.t.tType == sSHORTSTRING)
            {
              /* Get the address of the string field to be initialized at the
               * top of the stack.
               */

              pas_GenerateSimple(opDUP);
              pas_GenerateDataOperation(opPUSH,
                                        recordObjectPtr->sParm.r.rOffset);
              pas_GenerateSimple(opADD);
              pas_InitializeNewString(parentTypePtr);
              pas_GenerateDataOperation(opINDS, -sINT_SIZE);
            }
          else if (parentTypePtr->sParm.t.tType == sFILE ||
                   parentTypePtr->sParm.t.tType == sTEXTFILE)
            {
              /* Get the address of the file field to be initialized at the
               * top of the stack.
               */

              pas_GenerateSimple(opDUP);
              pas_GenerateDataOperation(opPUSH,
                                        recordObjectPtr->sParm.r.rOffset);
              pas_GenerateSimple(opADD);
              pas_InitializeNewFile(parentTypePtr);
              pas_GenerateDataOperation(opINDS, -sINT_SIZE);
            }
          else if (parentTypePtr->sParm.t.tType == sRECORD)
            {
              pas_GenerateSimple(opDUP);
              pas_GenerateDataOperation(opPUSH,
                                        recordObjectPtr->sParm.r.rOffset);
              pas_GenerateSimple(opADD);
              pas_InitializeNewRecord(parentTypePtr);
              pas_GenerateDataOperation(opINDS, -sINT_SIZE);
            }
          else if (parentTypePtr->sParm.t.tType == sARRAY)
            {
              /* Get the address of the array field to be initialized at the
               * top of the stack.
               */

              pas_GenerateSimple(opDUP);
              pas_GenerateDataOperation(opPUSH,
                                        recordObjectPtr->sParm.r.rOffset);
              pas_GenerateSimple(opADD);
              pas_InitializeNewArray(parentTypePtr);
              pas_GenerateDataOperation(opINDS, -sINT_SIZE);
            }
        }
    }
}

/****************************************************************************/

static void pas_InitializeNewArray(symbol_t *typePtr)
{
  /* On entry, a pointer to the ARRAY to be initialized will be at the top
   * of the stack.
   */

  symbol_t *baseTypePtr;

  /* Some sanity checks */

  if (typePtr->sKind           != sTYPE  ||
      typePtr->sParm.t.tType   != sARRAY ||
      typePtr->sParm.t.tParent == NULL   ||
      typePtr->sParm.t.tIndex  == NULL)
    {
      error(eHUH);  /* Should never happen */
    }

  /* We are only interested if the parent type is a FILE, STRING, or abort
   * RECORD that may contain file or string fields.
   */

  /* Get a pointer to the underlying base type symbol */

  baseTypePtr = pas_GetBaseTypePointer(typePtr);

  if (baseTypePtr->sParm.t.tType == sFILE        ||
      baseTypePtr->sParm.t.tType == sTEXTFILE    ||
      baseTypePtr->sParm.t.tType == sSTRING      ||
      baseTypePtr->sParm.t.tType == sSHORTSTRING ||
      baseTypePtr->sParm.t.tType == sRECORD      ||
      baseTypePtr->sParm.t.tType == sARRAY)
    {
      symbol_t *indexPtr;
      int       nElements;
      int       index;

      /* The index should be a SUBRANGE or SCALAR type */

      indexPtr = typePtr->sParm.t.tIndex;
      if (indexPtr->sKind != sTYPE ||
          (indexPtr->sParm.t.tType != sSUBRANGE &&
           indexPtr->sParm.t.tType != sSCALAR))
        {
          error(eHUH);  /* Should not happen */
        }

      /* Now loop for each element of the array */

      nElements = (int)indexPtr->sParm.t.tMaxValue -
                  (int)indexPtr->sParm.t.tMinValue + 1;

      for (index = 0; index < nElements; index++)
        {
          /* The address of the beginning of the array is at the TOP of the
           * stack.  Duplicate it and offset it for the index and element
           * size.
           */

          pas_GenerateSimple(opDUP);
          if (index > 0)
            {
              pas_GenerateDataOperation(opPUSH,
                                        baseTypePtr->sParm.t.tAllocSize);
              if (index > 1)
                {
                  pas_GenerateDataOperation(opPUSH, index);
                  pas_GenerateSimple(opMUL);
                }
              else
                {
                  pas_GenerateSimple(opADD);
                }
            }

          /* Generate the initializer */

          switch (baseTypePtr->sParm.t.tType)
            {
              case sFILE :
              case sTEXTFILE :
                pas_InitializeNewFile(baseTypePtr);
                break;

              case sSTRING :
              case sSHORTSTRING :
                pas_InitializeNewString(baseTypePtr);
                break;

              case sRECORD :
                pas_InitializeNewRecord(baseTypePtr);
                break;

              case sARRAY :
                pas_InitializeNewArray(baseTypePtr);
                break;

              default:
                error(eHUH);
                break;
            }
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************/

void pas_PrimeStandardProcedures(void)
{
  /* procedure val(const S : string; var V; var Code : word);  */

  valSymbol[0].sParm.p.pNParms = 3;
  valSymbol[1].sKind           = sSTRING;
  valSymbol[1].sParm.p.pParent = g_parentString;
  valSymbol[2].sKind           = sVAR_PARM;
  valSymbol[2].sParm.p.pParent = g_parentInteger;
  valSymbol[3].sKind           = sVAR_PARM;
  valSymbol[3].sParm.p.pParent = g_parentInteger;
}

/***********************************************************************/

void pas_StandardProcedure(void)
{
  TRACE(g_lstFile, "[pas_StandardProcedure]");

  /* Is the token a procedure? */

  if (g_token == tSTDPROC)
    {
      /* Yes, process it procedure according to the extended token type */

      switch (g_tknSubType)
        {
          /* Standard Procedures & Functions */

        case txEXIT :
          pas_ExitProc();
          break;

        case txHALT :
          pas_HaltProc();
          break;

        case txPAGE :
          pas_FileProc(xWRITE_PAGE);
          break;

           /* Memory alloctor */

        case txNEW :
          pas_NewProc();
          break;

        case txDISPOSE :
          pas_DisposeProc();
          break;

        /* Not implemented */

        case txGET :
        case txPACK :
        case txPUT :
        case txUNPACK :
          error(eNOTYET);
          getToken();
          break;

          /* less-than-standard procedures */

        case txVAL :
          pas_ValProc();
          break;

        /* File I/O */

        case txASSIGNFILE :
          pas_AssignFileProc();
          break;

        case txREAD :
          pas_ReadProc();
          break;

        case txREADLN :
          pas_ReadlnProc();
          break;

        case txRESET  :
          pas_OpenFileProc(xRESET, xRESETR);
          break;

        case txREWRITE :
          pas_OpenFileProc(xREWRITE, xREWRITER);
          break;

        case txAPPEND :
          pas_FileProc(xAPPEND);
          break;

        case txCLOSEFILE :
          pas_FileProc(xCLOSEFILE);
          break;

        case txWRITE :
          pas_WriteProc();
          break;

        case txWRITELN :
          pas_WritelnProc();
          break;

          /* Its not a recognized procedure */

        default :
          error(eINVALIDPROC);
          break;
        }
    }
}

/***********************************************************************/

uint16_t pas_GenerateFileNumber(uint16_t *pFileSize,
                                symbol_t *defaultFilePtr)
{
  /* If the token is not a symbol table related token, then abort returning
   * the default.  For example, suppose the first argument is a constant
   * string.
   */

  if (g_tknPtr != NULL)
    {
      /* Make a write-able copy of the variable symbol table entry */

      symbol_t varCopy = *g_tknPtr;

      /* Then work with the write-able copy */

      return pas_SimplifyFileNumber(&varCopy, 0, pFileSize, defaultFilePtr);
    }
  else
    {
      return pas_DefaultFileNumber(defaultFilePtr, pFileSize);
    }
}

/***********************************************************************/

int pas_ActualParameterSize(symbol_t *procPtr, int parmNo)
{
  symbol_t *baseTypePtr;

  /* These sizes must agree with the sizes used in
   * pas_ActualParameterList() below.
   */

  baseTypePtr = pas_GetBaseTypePointer(procPtr[parmNo].sParm.v.vParent);
  switch (baseTypePtr->sParm.t.tType)
    {
    case sINT :
    case sWORD :
    case sSUBRANGE :
    case sSCALAR :
      return sINT_SIZE;

    case sSHORTINT :
    case sSHORTWORD :
      return sSHORTINT_SIZE;

    case sLONGINT :
    case sLONGWORD :
      return sLONGINT_SIZE;

    case sCHAR :
      return sCHAR_SIZE;

    case sBOOLEAN :
      return sBOOLEAN_SIZE;

    case sREAL :
      return sREAL_SIZE;

    case sSET :
      return sSET_SIZE;

    case sSTRING :
      return sSTRING_SIZE;

    case sSHORTSTRING :
      return sSHORTSTRING_SIZE;

    case sARRAY :
    case sRECORD :
      return baseTypePtr->sParm.t.tAllocSize;

    case sFILE :
    case sTEXTFILE :
      return sINT_SIZE;

    case sVAR_PARM :
      return sPTR_SIZE;

    default:
      error(eINVPARMTYPE);
      return sINT_SIZE;
    }
}

/***********************************************************************/

int pas_ActualParameterList(symbol_t *procPtr)
{
  symbol_t *typePtr;
  exprType_t exprType;
  bool lparen = false;
  int parmIndex = 0;
  int size = 0;

  TRACE(g_lstFile,"[pas_ActualParameterList]");

  /* Processes the (optional) actual-parameter-list associated with
   * a function or procedure call:
   *
   * FORM: procedure-method-statement =
   *       procedure-method-specifier [ actual-parameter-list ]
   * FORM: function-designator = function-identifier [ actual-parameter-list ]
   *
   *
   * On entry, 'g_token' refers to the token just AFTER the procedure
   * function identifier.
   *
   * FORM: actual-parameter-list =
   *       '(' actual-parameter { ',' actual-parameter } ')'
   * FORM: actual-parameter =
   *       expression | variable-access |
   *       procedure-identifier | function-identifier
   */

  if (g_token == '(')
    {
      lparen = true;
      getToken();
    }

  /* If this procedure requires parameters, get them and make sure that
   * they match in type and number
   */

  if (procPtr->sParm.p.pNParms)
    {
      /* If it requires parameters, then the actual-parameter-list must
       * be present and must begin with '('
       */

      if (!lparen) error (eLPAREN);

      /* Loop to process the expected number of parameters.  The formal
       * argument descriptions follow the procedure/function description
       * as an array of variable declarations. (These sizes below must
       * agree with pas_ActualParameterSize() above);
       */

      for (parmIndex = 1;
           parmIndex <= procPtr->sParm.p.pNParms;
           parmIndex++)
        {
          typePtr = procPtr[parmIndex].sParm.v.vParent;
          switch (procPtr[parmIndex].sKind)
            {
            case sINT :
            case sWORD :
              exprType = pas_MapVariable2ExprType(procPtr[parmIndex].sKind, true);
              pas_Expression(exprType, typePtr);
              size += sINT_SIZE;
              break;

            case sSHORTINT :
            case sSHORTWORD :
              exprType = pas_MapVariable2ExprType(procPtr[parmIndex].sKind, true);
              pas_Expression(exprType, typePtr);
              size += sSHORTINT_SIZE;
              break;

            case sLONGINT :
            case sLONGWORD :
              exprType = pas_MapVariable2ExprType(procPtr[parmIndex].sKind, true);
              pas_Expression(exprType, typePtr);
              size += sLONGINT_SIZE;
              break;

            case sCHAR :
              pas_Expression(exprChar, typePtr);
              size += sCHAR_SIZE;
              break;

            case sREAL :
              pas_Expression(exprReal, typePtr);
              size += sREAL_SIZE;
              break;

            case sSTRING :
              pas_Expression(exprString, typePtr);
              size += sSTRING_SIZE;
              break;

            case sSHORTSTRING :
              pas_Expression(exprShortString, typePtr);
              size += sSHORTSTRING_SIZE;
              break;

            case sSUBRANGE :
              pas_Expression(exprInteger, typePtr);
              size += sINT_SIZE;
              break;

            case sSCALAR :
              pas_Expression(exprScalar, typePtr);
              size += sINT_SIZE;
              break;

            case sSET :
              pas_Expression(exprSet, typePtr);
              size += sSET_SIZE;
              break;

            case sARRAY :
              {
                symbol_t *arrayType;
                uint16_t arrayKind;

                /* Get the base type of the array */

                arrayType = pas_GetBaseTypePointer(typePtr);
                arrayKind = arrayType->sKind;

                /* REVISIT:  For subranges, we use the base type of
                 * the subrange.
                 */

                if (arrayKind == sSUBRANGE)
                  {
                    arrayKind = arrayType->sParm.t.tSubType;
                  }

                /* Then get the expression type associated with the
                 * base type
                 */

                exprType = pas_MapVariable2ExprType(arrayKind, false);
                pas_Expression(exprType, typePtr);
                size += typePtr->sParm.t.tAllocSize;
              }
              break;

            case sRECORD :
              pas_Expression(exprRecord, typePtr);
              size += typePtr->sParm.t.tAllocSize;
              break;

            case sVAR_PARM :
              if (typePtr)
                {
                  exprType_t varExprType;
                  uint16_t   varType = typePtr->sParm.t.tType;

                  switch (varType)
                    {
                    /* Simple ordinal types */

                    case sINT :
                    case sWORD :
                    case sSHORTINT :
                    case sSHORTWORD :
                    case sLONGINT :
                    case sLONGWORD :
                    case sSUBRANGE :
                    case sCHAR :
                    case sBOOLEAN :
                    case sSCALAR :
                    case sSCALAR_OBJECT :
                      varExprType = pas_MapVariable2ExprPtrType(varType, true);
                      pas_VarParameter(varExprType, typePtr);
                      size += sPTR_SIZE;
                      break;

                    /* Simple non-ordinal types */

                    case sSET :
                    case sREAL :
                    case sSTRING :
                    case sSHORTSTRING :
                    case sRECORD :
                    case sRECORD_OBJECT :
                    case sFILE :
                    case sTEXTFILE :
                      varExprType = pas_MapVariable2ExprPtrType(varType, false);
                      pas_VarParameter(varExprType, typePtr);
                      size += sPTR_SIZE;
                      break;

                    /* Not so simple types that require a little more effort */

                    case sARRAY :
                      {
                        symbol_t *arrayType;
                        uint16_t arrayKind;

                        /* Get the base type of the array */

                        arrayType = pas_GetBaseTypePointer(typePtr);
                        arrayKind = arrayType->sKind;

                        /* REVISIT:  For subranges, we use the base type of
                         * the subrange.
                         */

                        if (arrayKind == sSUBRANGE)
                          {
                            arrayKind = arrayType->sParm.t.tSubType;
                          }

                        /* Then get the expression type associated with the
                         * base type
                         */

                        exprType = pas_MapVariable2ExprPtrType(arrayKind, false);
                        pas_VarParameter(exprType, typePtr);
                        size += sPTR_SIZE;
                      }
                      break;

                    case sPOINTER :
                      error(eNOTYET);
                      break;

                    default :
                      error(eVARPARMTYPE);
                      break;
                    }
                }
              else
                {
                  error(eVARPARMTYPE);
                }
              break;

            default :
              error (eVARPARMTYPE);
            }

          if (parmIndex < procPtr->sParm.p.pNParms)
            {
              if (g_token != ',') error (eCOMMA);
              else getToken();
            }
        }
    }

  if (lparen == true)
    {
      if (g_token != ')') error (eRPAREN);
      else getToken();
    }

  return size;
}
