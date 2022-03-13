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
#include "pas_machine.h"
#include "pas_sysio.h"
#include "pas_stringlib.h"
#include "pas_oslib.h"

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
static void     pas_SeekProc(void);                 /* Change position in file */
static void     pas_AssignFileProc(void);           /* ASSIGNFILE procedure */
static void     pas_WriteProc(void);                /* WRITE procedure */
static void     pas_WritelnProc(void);              /* WRITELN procedure */
static void     pas_WriteProcCommon(bool text,      /* WRITE[LN] common logic */
                  uint16_t fileSize);
static void     pas_WriteText(void);                /* WRITE text file */
static uint16_t pas_WriteFieldWidth(void);          /* Get text file write field-width. */
static void     pas_WriteBinary(uint16_t fileSize); /* WRITE binary file */
static void     pas_DirectoryProc(uint16_t opCode); /* Change|Create working directory */
static void     pas_NewProc(void);                  /* Memory allocator */
static void     pas_DisposeProc(void);              /* Free memory */

static uint16_t pas_GenVarFileNumber(symbol_t *varPtr,
                  uint16_t *pFileSize,
                  symbol_t *defaultFilePtr);

/* Borlan-style string operation functions */

static void     pas_StrProc(void);                  /* Convert number */
static void     pas_InsertProc(void);               /* Insert string */
static void     pas_DeleteProc(void);               /* Delete substring */
static void     pas_FillCharProc(void);             /* Pad to specific length */
static void     pas_ValProc(void);                  /* VAL procedure */

/* Misc. helper functions */

static void     pas_InitializeNewRecord(symbol_t *typePtr);
static void     pas_InitializeNewArray(symbol_t *typePtr);

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
  if (defaultFilePtr != NULL)
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
  else
    {
      error(eINVFILETYPE);
      return sTEXTFILE;
    }
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

  pas_OsInterfaceCall(osEXIT);
}

/***********************************************************************/

static void pas_HaltProc(void)
{
  /* FORM:
   *   halt
   */

  getToken();
  pas_GenerateDataOperation(opPUSH, 0);
  pas_OsInterfaceCall(osEXIT);
}

/****************************************************************************/

static void pas_ReadProc(void)  /* READ procedure */
{
  uint16_t fileType;  /* sFILE or sTEXTFILE */
  uint16_t fileSize;  /* Size asociated with sFILE type */

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
 * - FLUSH writes all buffered outgoing data to the open file
 * - CLOSEFILE closes a previously opened file
 */

static void pas_FileProc(uint16_t opcode)
{
  /* FORM: procedure-name(<file number>)
   * FORM: procedure-name = PAGE | APPEND | FLUSH | CLOSEFILE
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
/* Sets position in a file */

static void pas_SeekProc(void)
{
  /* FORM: procedure Seek(var f : file; Pos : Int64);
   *
   * On entry, g_token refers to the reserved procedure name.  It may
   * be followed with a argument list.
   */

  getToken();
   if (g_token != '(') error(eLPAREN);
    {
      exprType_t exprType;

      /* Get the first expression following the left parenthesis.  This must
       * always be present, even if the default file is seek-able.
       */

      /* Push the file-number at the top of the stack */

      getToken();
      (void)pas_GenerateFileNumber(NULL, NULL);

      /* Verify that the fileNumber is followed with a comma */

      if (g_token != ',') error(eCOMMA);
      else getToken();

      /* The position argument should be an Int64 value. */

      exprType = pas_Expression(exprInt64, NULL);
      if (exprType != exprInt64)
        {
          error(eINVARG);
        }

      /* Generate the I/O operation */

      pas_GenerateIoOperation(xSEEK);

      /* Verify the closing right parenthesis */

      if (g_token != ')') error(eRPAREN);
      else getToken();
    }
}

/****************************************************************************/

static void pas_AssignFileProc(void)  /* ASSIGNFILE procedure */
{
  exprType_t exprType;
  uint32_t   fileType;

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
         *               TOS(1) = Fake write string buffer allocation size
         *               TOS(2) = Write string buffer address
         *               TOS(3) = Write string size
         *               TOS(4) = File number
         */

        pas_GenerateSimple(opDUP);
        pas_GenerateDataOperation(opPUSH, size);
        pas_GenerateDataOperation(opLAC, (uint16_t)offset);
        pas_GenerateDataOperation(opPUSH, size);
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
         *               TOS(1) = Fake buffer allocation size
         *               TOS(2) = Write address
         *               TOS(3) = Write size
         *               TOS(4) = File number
         */

        pas_GenerateSimple(opDUP);
        pas_GenerateDataOperation(opPUSH, size);
        pas_GenerateDataOperation(opLAC, offset);
        pas_GenerateDataOperation(opPUSH, size);
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
/* Change current working directory */

static void pas_DirectoryProc(uint16_t opCode)
{
  /* FORM: 'chdir' '(' string-expression ')' */

  getToken();
  if (g_token != '(') error(eLPAREN);  /* Skip over '(' */
  else getToken();

  /* Get the string expression */

  pas_Expression(exprString, NULL);

  /* Now we can generate the directory operation */

  pas_GenerateIoOperation(opCode);

  /* xCHDIR returns a boolean success/fail value; CHDIR returns nothing */

  pas_GenerateDataOperation(opINDS, -sINT_SIZE);

  /* Assure that the parameter list terminates with a right parenthesis. */

  if (g_token != ')') error(eRPAREN);  /* Skip over ')' */
  else getToken();
}

/****************************************************************************/
/* Memory allocator */

static void pas_NewProc(void)
{
  /* FORM:  'new' '(' pointer-variable ')' */

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
          pas_OsInterfaceCall(osNEW);

          /* Save this into the pointer variable */

          pas_GenerateStackReference(opSTS, varPtr);

          /* If we just allocated a string or file type, then we  have to
           * initialize the allocated instance.
           */

          baseTypePtr = pas_GetBaseTypePointer(parentTypePtr);
          varType     = baseTypePtr->sParm.t.tType;

          /* If we just created a string variable, then set up and initializer
           * for the string; memory for the string buffer must be set up at run
           * time.
           */

          /* Dereference the new pointer variable and place the address of
           * the new variable at the top of the stack.  If don't actually use
           * the value, then the optimizer should remove it.
           */

          pas_GenerateStackReference(opLDS, varPtr);
          if (varType == sSTRING)
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

           /* Discard the variable value at the top of the stack.  It was
            * duplicated before being used, so this clean-up is necessary.
            */

           pas_GenerateDataOperation(opINDS, -sPTR_SIZE);
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

      pas_OsInterfaceCall(osDISPOSE);
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
/* Converts a numeric value into a string.
 *
 *   Str(numvar : integer, VAR strvar : string)
 *
 * 'numvar' may include a fieldwidth and, for the case of real values, a
 * precision.
 */

static void pas_StrProc(void)
{
  exprType_t exprType;
  uint16_t fieldWidth;
  uint16_t stdOpCode;
  uint16_t opCode;

  /* FORM: 'str' '(' integer-expression ',' string-variable  ')'
   *       'str' '(' real-expression ',' string-variable  ')'
   */

  /* Verify that the argument list is enclosed in parentheses */

  getToken();                          /* Skip over 'str' */
  if (g_token != '(') error(eLPAREN);  /* Skip over '(' */
  else getToken();

  /* The first argument can be any signed or unsigned integer or a real type */

  exprType = pas_Expression(exprUnknown, NULL);
  if (exprType == exprInteger || exprType == exprShortInteger)
    {
      stdOpCode   = lbINTSTR;
    }
  else if (exprType == exprWord || exprType == exprShortWord)
    {
      stdOpCode   = lbWORDSTR;
    }
  else if (exprType == exprLongInteger)
    {
      stdOpCode   = lbLONGSTR;
    }
  else if (exprType == exprLongWord)
    {
      stdOpCode   = lbULONGSTR;
    }
  else if (exprType == exprReal)
    {
      stdOpCode   = lbREALSTR;
    }
  else
    {
      error(eINVARG);
    }

  /* The expression may be followed by a field width and precision */

  fieldWidth = pas_WriteFieldWidth();
  pas_GenerateDataOperation(opPUSH, fieldWidth);

  if (g_token != ',') error(eCOMMA);
  else getToken();

  /* The second argument is a string variable LValue that receives the
   * converted number.
   */

  if (g_token == sSTRING)
    {
      opCode = stdOpCode;
    }
  else
    {
      error(eINVARG);
    }

  /* Push the address of the string variable onto the stack */

  pas_GenerateStackReference(opLAS, g_tknPtr);

  /* Now we can generate the string operation */

  pas_StringLibraryCall(opCode);

  /* Assure that the parameter list terminates with a right parenthesis. */

  getToken();                          /* Skip over second string argument */
  if (g_token != ')') error(eRPAREN);  /* Skip over ')' */
  else getToken();
}

/****************************************************************************/
/* Insert a string inside another string from at the indexth character.
 *
 *   insert(source : string, VAR target : string; index : integer)
 */

static void pas_InsertProc(void)
{
  exprType_t exprType;

  /* FORM: 'insert' '(' string-expression ',' string-variable ','
   *        integer-expression ')'
   */

  /* Verify that the argument list is enclosed in parentheses */

  getToken();                          /* Skip over 'insert' */
  if (g_token != '(') error(eLPAREN);  /* Skip over '(' */
  else getToken();

  /* The first argument must be an string value */

  exprType = pas_Expression(exprString, NULL);
  if (exprType != exprString)
    {
      error(eINVARG);
    }

  if (g_token != ',') error(eCOMMA);
  else getToken();

  /* The second parameter must be a standard string LValue */

  if (g_token == sSTRING)
    {
      pas_GenerateStackReference(opLAS, g_tknPtr);
    }
  else
    {
      error(eINVARG);
    }

  getToken();                          /* Skip over the string */

  /* A comma should separate the arguments */

  if (g_token != ',') error(eCOMMA);
  else getToken();

  /* Get the final integer expression */

  pas_Expression(exprInteger, NULL);

  /* Now we can generate the string operation */

  pas_StringLibraryCall(lbINSERTSTR);

  /* Assure that the parameter list terminates with a right parenthesis. */

  if (g_token != ')') error(eRPAREN);  /* Skip over '(' */
  else getToken();
}

/****************************************************************************/
/* Deletes n characters from string s starting from index i.
 *
 *   delete(VAR s : string; i, n: integer)
 */

static void pas_DeleteProc(void)
{
  /* FORM: 'delete' '(' string-variable ',' integer-expression ','
   *        integer-expression ')'
   */

  /* Verify that the argument list is enclosed in parentheses */

  getToken();                          /* Skip over 'delete' */
  if (g_token != '(') error(eLPAREN);  /* Skip over '(' */
  else getToken();

  /* The first parameter must be a standard string LValue */

  if (g_token == sSTRING)
    {
      pas_GenerateStackReference(opLAS, g_tknPtr);
    }
  else
    {
      error(eINVARG);
    }

  getToken();                          /* Skip over the string */

  /* A comma should separate the arguments */

  if (g_token != ',') error(eCOMMA);
  else getToken();

  /* Get the first integer expression */

  pas_Expression(exprInteger, NULL);

  /* A comma should separate the arguments */

  if (g_token != ',') error(eCOMMA);
  else getToken();

  /* Get the second integer expression */

  pas_Expression(exprInteger, NULL);

  /* Now we can generate the string operation */

  pas_StringLibraryCall(lbDELSUBSTR);

  /* Assure that the parameter list terminates with a right parenthesis. */

  if (g_token != ')') error(eRPAREN);  /* Skip over '(' */
  else getToken();
}

/****************************************************************************/
/* Fill string s with character value until s is count-1 char long
 *
 *   fillchar(s : string; count : integer; value : shortword)
 */

static void pas_FillCharProc(void)
{
  uint16_t opCode;

  /* FORM: 'fillchar' '(' string-variable ',' integer-expression ','
   *        integer-expression ')'
   */

  /* Verify that the argument list is enclosed in parentheses */

  getToken();                          /* Skip over 'fillchar' */
  if (g_token != '(') error(eLPAREN);  /* Skip over '(' */
  else getToken();

  /* The first parameter must be a standard string LValue */

  if (g_token == sSTRING)
    {
      opCode = lbFILLCHAR;
      pas_GenerateStackReference(opLAS, g_tknPtr);
    }
  else
    {
      error(eINVARG);
    }

  getToken();                          /* Skip over the string */

  /* A comma should separate the arguments */

  if (g_token != ',') error(eCOMMA);
  else getToken();

  /* Get the first integer expression */

  pas_Expression(exprInteger, NULL);

  /* A comma should separate the arguments */

  if (g_token != ',') error(eCOMMA);
  else getToken();

  /* Get the second integer expression */

  pas_Expression(exprInteger, NULL);

  /* Now we can generate the string operation */

  pas_StringLibraryCall(opCode);

  /* Assure that the parameter list terminates with a right parenthesis. */

  if (g_token != ')') error(eRPAREN);  /* Skip over '(' */
  else getToken();
}

/****************************************************************************/

static void pas_ValProc(void)  /* VAL procedure */
{
  exprType_t exprType;

  /* Declaration:
   *   procedure val(const S : string; var V; var Code : word);
   *
   * Description:
   * val() converts the value represented in the string S to a numerical
   * value, and stores this value in the variable V, which can be of type
   * Integer, Longinteger, ShortInteger, or Real. If the conversion isn't
   * succesful, then the parameter Code contains the index of the character
   * in S which prevented the conversion. The string S is allowed to contain
   * spaces in the beginning.
   *
   * The string S can contain a number in decimal, hexadecimal, binary or
   * octal format, as described in the language reference.
   *
   * Errors:
   * If the conversion doesn't succeed, the value of Code indicates the
   * position where the conversion went wrong.
   */

  /* Skip over the 'val' identifier and verify that the parameter list
   * begins with a left parenthesis.
   */

  getToken();                          /* Skip over 'val' */
  if (g_token != '(') error(eLPAREN);  /* Skip over '(' */
  else getToken();

  /* The first argument must be an string value */

  exprType = pas_Expression(exprString, NULL);
  if (exprType != exprString)
    {
      error(eINVARG);
    }

  if (g_token != ',') error(eCOMMA);
  else getToken();

  /* The second argument must be an Integer, Long/Short Integer, or Real
   * variable.
   */

  if (g_token == sLONGINT || g_token == sSHORTINT || g_token == sREAL)
    {
      /* Long/ShortInteger and Real not yet supported. */

      error(eNOTYET);
    }
  else if (g_token == sINT)
    {
      pas_GenerateStackReference(opLAS, g_tknPtr);
    }
  else
    {
      error(eINVARG);
    }

  getToken();
  if (g_token != ',') error(eCOMMA);
  else getToken();

  /* The third argument must be an Integer variable. */

  if (g_token == sINT)
    {
      pas_GenerateStackReference(opLAS, g_tknPtr);
    }
  else
    {
      error(eINVARG);
    }

  /* And this should conclue with a right parenthesis */

  getToken();
  if (g_token != ')') error(eRPAREN);  /* Skip over ')' */
  else getToken();

  /* Generate the built-in procedure call.  NOTE the procedure call
   * logic will release the parameters from the stack saving us from
   * having to generate the INDS here.
   */

  pas_StringLibraryCall(lbVAL);
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
          else if (parentTypePtr->sParm.t.tType == sSTRING)
            {
              /* Get the address of the string field to be initialized at the
               * top of the stack.  pas_InitializeNewString() will call
               * lbSTRINIT which will consume that address.
               */

              pas_GenerateSimple(opDUP);
              pas_GenerateDataOperation(opPUSH,
                                        recordObjectPtr->sParm.r.rOffset);
              pas_GenerateSimple(opADD);
              pas_InitializeNewString(parentTypePtr);
            }
          else if (parentTypePtr->sParm.t.tType == sFILE ||
                   parentTypePtr->sParm.t.tType == sTEXTFILE)
            {
              /* Get the address of the file field to be initialized at the
               * top of the stack.  pas_InitializeNewString() will call
               * xALLOCFILE
               */

              pas_GenerateSimple(opDUP);
              pas_GenerateDataOperation(opPUSH,
                                        recordObjectPtr->sParm.r.rOffset);
              pas_GenerateSimple(opADD);
              pas_InitializeNewFile(parentTypePtr);
            }
          else if (parentTypePtr->sParm.t.tType == sRECORD)
            {
              /* Duplicate record address and offset it to the record field
               * of interest.
               */

              pas_GenerateSimple(opDUP);
              pas_GenerateDataOperation(opPUSH,
                                        recordObjectPtr->sParm.r.rOffset);
              pas_GenerateSimple(opADD);

              /* Process any initializers for the record and free the
               * duplicated address.
               */

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

              /* Process any initializers for the record and free the
               * duplicated address.
               */

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

/***********************************************************************/

void pas_StandardProcedure(void)
{
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

        case txSEEK :
          pas_SeekProc();
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

          /* Borland-style string operator procedures */

        case txSTR :
          pas_StrProc();
          break;

        case txINSERT :
          pas_InsertProc();
          break;

        case txDELETE :
          pas_DeleteProc();
          break;

        case txFILLCHAR :
          pas_FillCharProc();
          break;

        case txVAL :
          pas_ValProc();
          break;

          /* Borland-style string directory operations */

        case txCHDIR :
          pas_DirectoryProc(xCHDIR);
          break;

        case txMKDIR :
          pas_DirectoryProc(xMKDIR);
          break;

        case txRMDIR :
          pas_DirectoryProc(xRMDIR);
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

        case txFLUSH :
          pas_FileProc(xFLUSH);
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
   *
   * REVISIT:  This alignment is only necessary on odd byte-size data
   * types because they may be followed by a parameter that does require
   * alignment.  We could probably pack byte-sized parameters if we were
   * more clever.
   */

  baseTypePtr = pas_GetBaseTypePointer(procPtr[parmNo].sParm.v.vParent);
  switch (baseTypePtr->sParm.t.tType)
    {
    case sINT :
    case sWORD :
    case sSUBRANGE :
    case sSCALAR :
      return INT_ALIGNUP(sINT_SIZE);

    case sSHORTINT :
    case sSHORTWORD :
      return INT_ALIGNUP(sSHORTINT_SIZE);

    case sLONGINT :
    case sLONGWORD :
      return INT_ALIGNUP(sLONGINT_SIZE);

    case sCHAR :
      return INT_ALIGNUP(sCHAR_SIZE);

    case sBOOLEAN :
      return INT_ALIGNUP(sBOOLEAN_SIZE);

    case sREAL :
      return INT_ALIGNUP(sREAL_SIZE);

    case sSET :
      return INT_ALIGNUP(sSET_SIZE);

    case sSTRING :
      return INT_ALIGNUP(sSTRING_SIZE);

    case sARRAY :
    case sRECORD :
      return INT_ALIGNUP(baseTypePtr->sParm.t.tAllocSize);

    case sFILE :
    case sTEXTFILE :
      return INT_ALIGNUP(sINT_SIZE);

    case sVAR_PARM :
      return INT_ALIGNUP(sPTR_SIZE);

    default:
      error(eINVPARMTYPE);
      return INT_ALIGNUP(sINT_SIZE);
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
       *
       * REVISIT:  This alignment is only necessary on odd byte-size data
       * types because they may be followed by a parameter that does require
       * alignment.  We could probably pack byte-sized parameters if we were
       * more clever.
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
            case sBOOLEAN :
              exprType = pas_MapVariable2ExprType(procPtr[parmIndex].sKind, true);
              pas_Expression(exprType, typePtr);
              size += INT_ALIGNUP(sINT_SIZE);
              break;

            case sSHORTINT :
            case sSHORTWORD :
              exprType = pas_MapVariable2ExprType(procPtr[parmIndex].sKind, true);
              pas_Expression(exprType, typePtr);
              size += INT_ALIGNUP(sSHORTINT_SIZE);
              break;

            case sLONGINT :
            case sLONGWORD :
              exprType = pas_MapVariable2ExprType(procPtr[parmIndex].sKind, true);
              pas_Expression(exprType, typePtr);
              size += INT_ALIGNUP(sLONGINT_SIZE);
              break;

            case sCHAR :
              pas_Expression(exprChar, typePtr);
              size += INT_ALIGNUP(sCHAR_SIZE);
              break;

            case sREAL :
              pas_Expression(exprReal, typePtr);
              size += INT_ALIGNUP(sREAL_SIZE);
              break;

            case sSTRING :
              pas_Expression(exprString, typePtr);
              size += INT_ALIGNUP(sSTRING_SIZE);
              break;

            case sSUBRANGE :
              pas_Expression(exprInteger, typePtr);
              size += INT_ALIGNUP(sINT_SIZE);
              break;

            case sSCALAR :
              pas_Expression(exprScalar, typePtr);
              size += INT_ALIGNUP(sINT_SIZE);
              break;

            case sSET :
              pas_Expression(exprSet, typePtr);
              size += INT_ALIGNUP(sSET_SIZE);
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
                size += INT_ALIGNUP(typePtr->sParm.t.tAllocSize);
              }
              break;

            case sRECORD :
              pas_Expression(exprRecord, typePtr);
              size += INT_ALIGNUP(typePtr->sParm.t.tAllocSize);
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
                      size += INT_ALIGNUP(sPTR_SIZE);
                      break;

                    /* Simple non-ordinal types */

                    case sSET :
                    case sREAL :
                    case sSTRING :
                    case sRECORD :
                    case sRECORD_OBJECT :
                    case sFILE :
                    case sTEXTFILE :
                      varExprType = pas_MapVariable2ExprPtrType(varType, false);
                      pas_VarParameter(varExprType, typePtr);
                      size += INT_ALIGNUP(sPTR_SIZE);
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
                        size += INT_ALIGNUP(sPTR_SIZE);
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
              error (eINVPARMTYPE);
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
