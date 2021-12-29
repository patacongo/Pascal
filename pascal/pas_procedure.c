/****************************************************************************
 * pas_procedure.c
 * Standard procedures (all called in pas_statement.c)
 *
 *   Copyright (C) 2008-2009, 2021 Gregory Nutt. All rights reserved.
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

static uint16_t simplifyFileNumber(symbol_t *varPtr, uint8_t fileFlags,
                                   uint16_t *pFileSize,
                                   symbol_t *defaultFilePtr);
static uint16_t defaultFileNumber(symbol_t *defaultFilePtr,
                                  uint16_t *pFileSize);

/* Helpers for standard procedures  */

static void haltProc(void);                     /* HALT procedure */

static void readProc(void);                     /* READ procedure */
static void readlnProc(void);                   /* READLN procedure */
static void readProcCommon(bool text,           /* READ[LN] common logic */
                           uint16_t fileSize);
static void readText(void);                     /* READ text file */
static void readBinary(uint16_t fileSize);      /* READ binary file */
static void openFileProc(uint16_t opcode1,      /* File procedure with 1 arg*/
                         uint16_t opcode2);
static void fileProc(uint16_t opcode);          /* File procedure with 1 arg*/
static void assignFileProc(void);               /* ASSIGNFILE procedure */
static void writeProc(void);                    /* WRITE procedure */
static void writelnProc (void);                 /* WRITELN procedure */
static void writeProcCommon(bool text,          /* WRITE[LN] common logic */
                            uint16_t fileSize);
static void writeText(void);                    /* WRITE text file */
static void writeBinary(uint16_t fileSize);     /* WRITE binary file */

static uint16_t genVarFileNumber(symbol_t *varPtr,
                                 uint16_t *pFileSize,
                                 symbol_t *defaultFilePtr);

/* Helpers for less-than-standard procedures */

static void valProc(void);                      /* VAL procedure */

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* procedure val(const S : string; var V; var Code : word); */

static symbol_t valSymbol[4];

/****************************************************************************/

void pas_PrimeStandardProcedures(void)
{
  /* procedure val(const S : string; var V; var Code : word);  */

  valSymbol[0].sParm.p.nParms = 3;
  valSymbol[1].sKind          = sSTRING;
  valSymbol[1].sParm.p.parent = g_parentString;
  valSymbol[2].sKind          = sVAR_PARM;
  valSymbol[2].sParm.p.parent = g_parentInteger;
  valSymbol[3].sKind          = sVAR_PARM;
  valSymbol[3].sParm.p.parent = g_parentInteger;
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

        case txHALT :
          getToken();
          haltProc();
          break;

        case txPAGE :
          fileProc(xWRITE_PAGE);
          break;

        /* Not implemented */

        case txGET :
        case txNEW :
        case txPACK :
        case txPUT :
        case txUNPACK :
          error(eNOTYET);
          getToken();
          break;

          /* less-than-standard procedures */

        case txVAL :
          valProc();
          break;

        /* File I/O */

        case txASSIGNFILE :
          assignFileProc();
          break;

        case txREAD :
          readProc();
          break;

        case txREADLN :
          readlnProc();
          break;

        case txRESET  :
          openFileProc(xRESET, xRESETR);
          break;

        case txREWRITE :
          openFileProc(xREWRITE, xREWRITER);
          break;

        case txAPPEND :
          fileProc(xAPPEND);
          break;

        case txCLOSEFILE :
          fileProc(xCLOSEFILE);
          break;

        case txWRITE :
          writeProc();
          break;

        case txWRITELN :
          writelnProc();
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

      return simplifyFileNumber(&varCopy, 0, pFileSize, defaultFilePtr);
    }
  else
    {
      return defaultFileNumber(defaultFilePtr, pFileSize);
    }
}

/***********************************************************************/

int pas_ActualParameterSize(symbol_t *procPtr, int parmNo)
{
  /* These sizes must agree with the sizes used in
   * pas_ActualParameterListg() below.
   */

  symbol_t *typePtr = procPtr[parmNo].sParm.v.parent;
  switch (typePtr->sKind)
    {
    case sINT :
    case sSUBRANGE :
    case sSCALAR :
    case sSET_OF :
    default:
      return sINT_SIZE;
      break;

    case sCHAR :
      return sCHAR_SIZE;
      break;

    case sREAL :
      return sREAL_SIZE;
      break;

    case sSTRING :
      return sSTRING_SIZE;
      break;

    case sARRAY :
    case sRECORD :
      return typePtr->sParm.t.asize;
      break;

    case sVAR_PARM :
      return sPTR_SIZE;
      break;
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

  if (procPtr->sParm.p.nParms)
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
           parmIndex <= procPtr->sParm.p.nParms;
           parmIndex++)
        {
          typePtr = procPtr[parmIndex].sParm.v.parent;
          switch (procPtr[parmIndex].sKind)
            {
            case sINT :
              pas_Expression(exprInteger, typePtr);
              size += sINT_SIZE;
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

            case sSUBRANGE :
              pas_Expression(exprInteger, typePtr);
              size += sINT_SIZE;
              break;

            case sSCALAR :
              pas_Expression(exprScalar, typePtr);
              size += sINT_SIZE;
              break;

            case sSET_OF :
              pas_Expression(exprSet, typePtr);
              size += sINT_SIZE;
              break;

            case sARRAY :
              {
                symbol_t *arrayType;
                symbol_t *nextType;
                uint16_t arrayKind;

                /* Get the base type of the array */

                arrayKind = typePtr->sKind;
                arrayType = typePtr;
                nextType  = typePtr->sParm.v.parent;

                while (nextType != NULL && nextType->sKind == sTYPE)
                  {
                    arrayType = nextType;
                    arrayKind = arrayType->sParm.t.type;
                    nextType  = arrayType->sParm.t.parent;
                  }

                /* REVISIT:  For subranges, we use the base type of
                 * the subrange.
                 */

                if (arrayKind == sSUBRANGE)
                  {
                    arrayKind = arrayType->sParm.t.subType;
                  }

                /* Then get the expression type associated with the
                 * base type
                 */

                exprType = pas_MapVariable2ExprType(arrayKind, false);
                pas_Expression(exprType, typePtr);
                size += typePtr->sParm.t.asize;
              }
              break;

            case sRECORD :
              pas_Expression(exprRecord, typePtr);
              size += typePtr->sParm.t.asize;
              break;

            case sVAR_PARM :
              if (typePtr)
                {
                  switch (typePtr->sParm.t.type)
                    {
                    case sINT :
                      pas_VarParameter(exprIntegerPtr, typePtr);
                      size += sPTR_SIZE;
                      break;

                    case sBOOLEAN :
                      pas_VarParameter(exprBooleanPtr, typePtr);
                      size += sPTR_SIZE;
                      break;

                    case sCHAR :
                      pas_VarParameter(exprCharPtr, typePtr);
                      size += sPTR_SIZE;
                      break;

                    case sREAL :
                      pas_VarParameter(exprRealPtr, typePtr);
                      size += sPTR_SIZE;
                      break;

                    case sARRAY :
                      {
                        symbol_t *arrayType;
                        symbol_t *nextType;
                        uint16_t arrayKind;

                        /* Get the base type of the array */

                        arrayKind = typePtr->sKind;
                        arrayType = typePtr;
                        nextType  = typePtr->sParm.v.parent;

                        while (nextType != NULL && nextType->sKind == sTYPE)
                          {
                            arrayType = nextType;
                            arrayKind = arrayType->sParm.t.type;
                            nextType  = arrayType->sParm.t.parent;
                          }

                        /* REVISIT:  For subranges, we use the base type of
                         * the subrange.
                         */

                        if (arrayKind == sSUBRANGE)
                          {
                            arrayKind = arrayType->sParm.t.subType;
                          }

                        /* Then get the expression type associated with the
                         * base type
                         */

                        exprType = pas_MapVariable2ExprType(arrayKind, false);
                        pas_VarParameter(exprType, typePtr);
                        size += sPTR_SIZE;
                      }
                      break;

                    case sRECORD :
                      pas_VarParameter(exprRecordPtr, typePtr);
                      size += sPTR_SIZE;
                      break;

                    case sFILE :
                    case sTEXTFILE :
                      pas_VarParameter(exprFilePtr, typePtr);
                      size += sPTR_SIZE;
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

          if (parmIndex < procPtr->sParm.p.nParms)
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

/***********************************************************************/

static uint16_t simplifyFileNumber(symbol_t *varPtr, uint8_t fileFlags,
                                   uint16_t *pFileSize,
                                   symbol_t *defaultFilePtr)
{
  uint16_t fileType;
  uint16_t fileSize;

  /* Check if this is a VAR parameter */

  switch (varPtr->sKind)
    {
      case sVAR_PARM :
        {
          return genVarFileNumber(varPtr, pFileSize, defaultFilePtr);
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
              return defaultFileNumber(defaultFilePtr, pFileSize);
            }
          else
            {
              getToken();
            }

          if (g_token != sRECORD_OBJECT)
            {
              error(eRECORDOBJECT);
              return defaultFileNumber(defaultFilePtr, pFileSize);
            }
          else
            {
              symbol_t *fieldPtr = g_tknPtr;
              symbol_t *baseTypePtr;
              symbol_t *nextPtr;

              /* Verify that the field selector resolves to a
               * file type.
               */

              nextPtr         = fieldPtr->sParm.r.parent;
              baseTypePtr     = nextPtr;
              while (nextPtr != NULL && nextPtr->sKind == sTYPE)
                {
                  baseTypePtr = nextPtr;
                  nextPtr     = baseTypePtr->sParm.t.parent;
                }

              fileType        = baseTypePtr->sParm.t.type;

              if (fileType != sFILE && fileType != sTEXTFILE)
                {
                  error(eINVFILETYPE);
                }

              /* If the field offset is non-zero, add that to the array
               * offset.
               */

              if (baseTypePtr->sParm.r.offset != 0)
                {
                  /* Add the field offset */

                  pas_GenerateDataOperation(opPUSH,
                                            fieldPtr->sParm.r.offset);
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
          symbol_t *nextPtr;
          symbol_t *baseTypePtr;
          symbol_t *indexTypePtr;

          fileFlags |= INDEXED_FACTOR;

          /* REVISIT:  Before we consume the input, we must be certain that
           * this is going to resolve into a file type.  All we know now is
           * that this is an arry... but and array of what?
           *
           * The complex case is an array of records which may or may not
           * contain a file field.  We can't know which field is selected
           * until we parse the field selector.
           */

          typePtr = varPtr->sParm.v.parent;
          if (typePtr == NULL)
            {
              error(eHUH);
              return defaultFileNumber(defaultFilePtr, pFileSize);
            }

          /* Get a pointer to the underlying base type symbol */

          nextPtr         = typePtr;
          baseTypePtr     = typePtr;
          while (nextPtr != NULL && nextPtr->sKind == sTYPE)
            {
              baseTypePtr = nextPtr;
              nextPtr     = baseTypePtr->sParm.t.parent;
            }

#ifdef CONFIG_PAS_FILERECORD
          if (baseTypePtr->sParm.t.type == sRECORD)
            {
              /* REVISIT:  This is the problem case */
            }
          else
#endif
          if (baseTypePtr->sParm.t.type != sFILE &&
              baseTypePtr->sParm.t.type != sTEXTFILE)
            {
              return defaultFileNumber(defaultFilePtr, pFileSize);
            }

          /* Skip over the array name */

          getToken();

          /* Then handle the bracketed array index */

          indexTypePtr = typePtr->sParm.t.index;
          if (indexTypePtr == NULL) error(eHUH);
          else
            {
              /* Generate the array offset calculation */

              pas_ArrayIndex(indexTypePtr, baseTypePtr->sParm.t.asize);

              /* Return the parent type of the array */

              varPtr->sKind        = baseTypePtr->sParm.t.type;
              varPtr->sParm.v.size = baseTypePtr->sParm.t.asize;

              return simplifyFileNumber(varPtr, fileFlags, pFileSize,
                                        defaultFilePtr);
            }
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
              fileSize = varPtr->sParm.v.xfrUnit;
            }

          pas_GenerateStackReference(opLDS, varPtr);

          /* Skip over the variable identifer */

          getToken();
        }
        break;

      /* Check for other exotic forms that have not yet been implemented */

      case sPOINTER :
      case sTYPE :
        error(eNOTYET);  /* Fall-through to the default case */

      /* Not a file-type variable.  Use the default file number (which we
       * assume to be of type sTEXTFILE)
       */

      default :
        return defaultFileNumber(defaultFilePtr, pFileSize);
    }

  if (pFileSize != NULL)
    {
      *pFileSize = fileSize;
    }

  return fileType;
}

/***********************************************************************/

static uint16_t defaultFileNumber(symbol_t *defaultFilePtr,
                                  uint16_t *pFileSize)
{
  /* Push the default file number */

  pas_GenerateStackReference(opLDS, defaultFilePtr);

  /* Return the file type and size for a TEXTFILE */

  if (pFileSize != NULL)
    {
      *pFileSize = defaultFilePtr->sParm.v.xfrUnit;
    }

  return defaultFilePtr->sKind;
}

/***********************************************************************/

static void haltProc (void)
{
  /* FORM:
   *   halt
   */

  pas_StandardFunctionCall(lbHALT);
}

/****************************************************************************/

static void readProc(void)          /* READLN procedure */
{
  uint16_t fileType;  /* sFILE or sTEXTFILE */
  uint16_t fileSize;  /* Size asociated with sFILE type */

  TRACE(g_lstFile, "[readProc]");

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

  readProcCommon((fileType == sTEXTFILE), fileSize);

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

static void readlnProc(void)          /* READLN procedure */
{
  uint16_t fileType;  /* sFILE or sTEXTFILE */
  uint16_t fileSize;  /* Size asociated with sFILE type */

  TRACE(g_lstFile, "[readlnProc]");

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

          readProcCommon(true, fileSize);
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

static void readProcCommon(bool text, uint16_t fileSize)
{
  TRACE(g_lstFile, "[readProcCommon]");

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
          readText();
        }
      else
        {
          readBinary(fileSize);
        }

      /* Should be followed by ':' meaning that there are more parameter, or
       * by ')' meaning that we have processed all of the parameters.
       *
       * NOTE:  Some test cases use ',' as the separator.  Let's supported
       * either.
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

static void readText(void)
{
  exprType_t exprType;

  TRACE(g_lstFile, "[readText]");

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
      /* READ_INT: TOS   = Read address
       *           TOS+1 = File number
       */

      case exprIntegerPtr :
        pas_GenerateIoOperation(xREAD_INT);
        break;

      /* READ_CHAR: TOS   = Read address
       *            TOS+1 = File number
       */

      case exprCharPtr :
        pas_GenerateIoOperation(xREAD_CHAR);
        break;

      /* READ_REAL: TOS   = Read address
       *            TOS+1 = File number
       */

      case exprRealPtr :
        pas_GenerateIoOperation(xREAD_REAL);
        break;

      /* READ_STRING: TOS   = Address of string variable
       *              TOS+1 = File number
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

static void readBinary(uint16_t fileSize)
{
  symbol_t *parent;
  uint16_t size;

  TRACE(g_lstFile, "[readBinary]");

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
      case sBOOLEAN :
      case sCHAR :
      case sREAL :

      /* Complex types */

      case sSTRING :
      case sARRAY :
      case sRECORD :
        size   = g_tknPtr->sParm.v.size;
        break;

      /* VAR parameter */

      case sVAR_PARM :
        /* REVISIT:  Need to check the type of the parent.  Assuming
         * that the parent is an sTYPE
         */

        parent = g_tknPtr->sParm.v.parent;
        size   = parent->sParm.t.asize;
        break;

#if 0 /* Not yet */
      /* Defined types */

      case sTYPE :
        size   = g_tknPtr->sParm.t.asize;
        break;
#endif

      default :
        error(eREADPARMTYPE);
        break;
    }

  if (size != fileSize) error(eREADPARMTYPE);
  else
    {
      /* READ_BINARY: TOS   = Read address
       *              TOS+1 = Read size
       *              TOS+2 = File number
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
 * APPEND also opens a file, but is handled by fileProc().  This function is
 * almost identical to fileProc(), differing only in that it accepts two
 * opcodes and handles the option record-size.
 */

static void openFileProc(uint16_t opcode1, uint16_t opcode2)
{
  uint16_t opcode = opcode1;

  TRACE(g_lstFile, "[fileProc]");

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

static void fileProc(uint16_t opcode)
{
  TRACE(g_lstFile, "[fileProc]");

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

static void assignFileProc(void)       /* ASSIGNFILE procedure */
{
  exprType_t exprType;
  uint32_t   fileType;

  TRACE(g_lstFile, "[assignFileProc]");

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

static void writeProc(void)            /* WRITE procedure */
{
  uint16_t fileType;  /* sFILE or sTEXTFILE */
  uint16_t fileSize;  /* Size asociated with sFILE type */

   TRACE(g_lstFile, "[writeProc]");

  /* FORM: WRITE  write-parameter-list
   * FORM:      write-parameter-list = '(' [ file-variable ',' ]
   *            write-parameter { ',' write-parameter } ')'
   * FORM:      write-parameter = expression [ ':' expression [ ':' expression ] ]
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

  writeProcCommon((fileType == sTEXTFILE), fileSize);

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

static void writelnProc(void)         /* WRITELN procedure */
{
  uint16_t fileType;  /* sFILE or sTEXTFILE */
  uint16_t fileSize;  /* Size asociated with sFILE type */

  TRACE(g_lstFile, "[writelnProc]");

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

          writeProcCommon(true, fileSize);
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
   * WRITELN: TOS = File number.  For WRITELN, writeProcCommon() will leave
   * the file number at the top of the stack
   */

  pas_GenerateIoOperation(xWRITELN);
}

/***********************************************************************/

static void writeProcCommon(bool text, uint16_t fileSize)
{
  TRACE(g_lstFile, "[writeProcCommon]");

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
          writeText();
        }
      else
        {
          writeBinary(fileSize);
        }

      /* Should be followed by ':' meaning that there are more parameter, or
       * by ')' meaning that we have processed all of the parameters.
       *
       * NOTE:  Some test cases use ',' as the separator.  Let's support
       * either.
       */

      if (g_token == ':' || g_token == ',') getToken();
      else break;
    }
}

/***********************************************************************/

static void writeText(void)
{
  exprType_t writeType;
  symbol_t  *wPtr;

  TRACE(g_lstFile, "[writeText]");

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

        uint32_t offset = poffAddRoDataString(poffHandle, g_tokenString);

        /* Set the file number, offset and size on the stack
         *
         * WRITE_STRING: TOS   = Write size
         *               TOS+1 = Write address
         *               TOS+2 = File number
         */

        pas_GenerateSimple(opDUP);
        pas_GenerateDataOperation(opPUSH, strlen(g_tokenString));
        pas_GenerateDataOperation(opLAC, (uint16_t)offset);
        pas_GenerateIoOperation(xWRITE_STRING);

        g_stringSP = g_tokenString;
        getToken();
      }
      break;

    case sSTRING_CONST :
      /* WRITE_STRING: TOS   = Write size
       *               TOS+1 = Write address
       *               TOS+2 = File number
       */

      pas_GenerateSimple(opDUP);
      pas_GenerateDataOperation(opPUSH, (uint16_t)g_tknPtr->sParm.s.size);
      pas_GenerateDataOperation(opLAC, (uint16_t)g_tknPtr->sParm.s.offset);
      pas_GenerateIoOperation(xWRITE_STRING);

      getToken();
      break;

      /* Array of type CHAR without indexing */

    case sARRAY :
      wPtr = g_tknPtr->sParm.v.parent;
      if (wPtr != NULL && wPtr->sKind == sTYPE &&
          wPtr->sParm.t.type == sCHAR &&
          getNextCharacter(true) != '[')
        {
          /* WRITE_STRING: TOS   = Write size
           *               TOS+1 = Write address
           *               TOS+2 = File number
           */

          pas_GenerateSimple(opDUP);
          pas_GenerateDataOperation(opPUSH, wPtr->sParm.v.size);
          pas_GenerateStackReference(opLAS, wPtr);
          pas_GenerateIoOperation(xWRITE_STRING);
          getToken();
          break;
        }

      /* Otherwise, we fall through to process the ARRAY like any
       * expression.
       */

    default :
      /* Put the file number and value on the stack */

      pas_GenerateSimple(opDUP);
      writeType = pas_Expression(exprUnknown, NULL);

      /* Then generate the operation */

      switch (writeType)
        {
        case exprInteger :
          /* WRITE_INT: TOS   = Write value
           *            TOS+1 = File number
           */

          pas_GenerateIoOperation(xWRITE_INT);
          break;

        case exprChar :
          /* WRITE_CHAR: TOS   = Write value
           *             TOS+1 = File number
           */

          pas_GenerateIoOperation(xWRITE_CHAR);
          break;

        case exprReal :
          /* WRITE_CHAR: TOS-TOS+3 = Write value
           *             TOS+4     = File number
           */

          pas_GenerateIoOperation(xWRITE_REAL);
          break;

        case exprString :
          /* WRITE_STRING: TOS   = Write address
           *               TOS+1 = Write size
           *               TOS+2 = File number
           */

          pas_GenerateIoOperation(xWRITE_STRING);
          break;

        default :
          error(eWRITEPARM);
          break;
        }
    }
}

/***********************************************************************/

static void writeBinary(uint16_t fileSize)
{
  symbol_t *parent;
  uint16_t size;

  TRACE(g_lstFile, "[writeBinary]");

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
      case sBOOLEAN :
      case sCHAR :
      case sREAL :

      /* Complex types */

      case sSTRING :
      case sARRAY :
      case sRECORD :
        size   = g_tknPtr->sParm.v.size;
        break;

      /* VAR parameter */

      case sVAR_PARM :
        /* REVISIT:  Need to check the type of the parent.  Assuming
         * that the parent is an sTYPE
         */

        parent = g_tknPtr->sParm.v.parent;
        size   = parent->sParm.t.asize;
        break;

#if 0 /* Not yet */
      /* Defined types */

      case sTYPE :
        size   = g_tknPtr->sParm.t.asize;
        break;
#endif

      default :
        error(eWRITEPARMTYPE);
        break;
    }

  if (size != fileSize) error(eWRITEPARMTYPE);
  else
    {
      /* WRITE_BINARY: TOS   = Write address
       *               TOS+1 = Write size
       *               TOS+2 = File number
       */

      pas_GenerateSimple(opDUP);
      pas_GenerateDataOperation(opPUSH, size);
      pas_GenerateStackReference(opLAS, g_tknPtr);
      pas_GenerateIoOperation(xWRITE_BINARY);
    }

  getToken();
}

/****************************************************************************/
/* The VAR file parameter is more complex than the "normal" file variable
 * because the transfer unit size is more difficult to find.
 */

static uint16_t genVarFileNumber(symbol_t *varPtr, uint16_t *pFileSize,
                                 symbol_t *defaultFilePtr)
{
  symbol_t *typePtr;  /* The base type may be in a different symbol */
  uint16_t  symType;  /* The base type may not be a symbol */
  uint16_t  fileType;
  uint16_t  fileSize;

  /* Use the parent type.  The parent of a a VAR parameter is a type */

  typePtr = varPtr->sParm.v.parent;
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

      symType = typePtr->sParm.t.type;

      /* Check if this is a file in the sequence of types. */

      if (symType == sFILE)
        {
          /* For binary files, we find the transfer unit size in the asize
           * field.
           */

          fileSize = typePtr->sParm.t.asize;
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
          symbol_t *tmpPtr  = typePtr->sParm.t.parent;

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
      return defaultFileNumber(defaultFilePtr, pFileSize);
    }

  if (pFileSize != NULL)
    {
      *pFileSize = fileSize;
    }

  return fileType;
}

/****************************************************************************/

static void valProc(void)         /* VAL procedure */
{
  TRACE(g_lstFile, "[valProc]");

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

/***********************************************************************/
