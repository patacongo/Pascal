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
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
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

uint16_t generateFileNumber(uint16_t defaultFileNumber,
                            uint16_t *pFileSize)
{
  symbol_t *varPtr   = g_tknPtr; /* Remember the variable symbol */
  symbol_t *typePtr  = g_tknPtr; /* The base type may be in a different symbol */
  uint16_t tknType   = g_token;  /* The base type may not be a symbol */
  uint16_t fileType;
  uint16_t fileSize;
  uint16_t fileFlags = 0;

  /* If the token is not a symbol table related token, then abort returning
   * the default.  For example, suppose the first argument is a constant
   * string.
   */

  if (typePtr == NULL)
    {
      pas_GenerateDataOperation(opPUSH, INPUT_FILE_NUMBER);
      if (pFileSize != NULL)
        {
          *pFileSize = sCHAR_SIZE;
        }

      return sTEXTFILE;
    }

  /* Check if this is a VAR parameter */

  if (typePtr->sKind == sVAR_PARM)
    {
      /* Use the parent type.  The parent of a a VAR parameter is a type */

      typePtr    = typePtr->sParm.v.parent;
      tknType    = typePtr->sKind;
      fileFlags |= VAR_PARM_FACTOR;
    }

#if 0
  if (typePtr->sKind == sARRAY)
    {
      fileFlags  |= INDEXED_FACTOR;
    }

  if (typePtr->sKind == sPOINTER)
    {
      fileFlags  |= ADDRESS_FACTOR;
    }
#endif

  /* Is this a symbole whose type is typedef'ed? */

  while (typePtr->sKind == sTYPE)
    {
      /* Get the type of the parent */

      tknType = typePtr->sParm.t.type;

      /* This is the final type if it has no parent. */

      symbol_t *tmpPtr = varPtr->sParm.t.parent;
      if (tmpPtr != NULL)
        {
          varPtr = tmpPtr;
        }
      else
        {
          break;
        }
    }

  /* Is this a dereferenced pointer to a file type? */
  /* REVISIT:  Not implemented. */

  /* Is this a symbol of type binary or text FILE? */

  if (tknType == sFILE || tknType == sTEXTFILE)
    {
      fileType = varPtr->sKind;
      fileSize = typePtr->sParm.v.size;

      /* If this is a VAR parameter, then we need to push the address of the
       * file variable and dereference that address.
       */

      if ((fileFlags & VAR_PARM_FACTOR) != 0)
        {
          pas_GenerateStackReference(opLDS, varPtr);
          pas_GenerateSimple(opLDI);
        }
      else
        {
          pas_GenerateStackReference(opLDS, varPtr);
        }

      /* Skip over the variable identifer */

      getToken();
    }

#if 0 /* Needs more thought */
  /* Is this a file type? */

  else if (typePtr->sKind == sTYPE)
    {
      fileType   = varPtr->sParm.t.type;
      fileSize   = varPtr->sParm.t.asize;
      pas_GenerateDataOperation(opPUSH, g_tokenPtr->sParm.f.fileNumber);

      if (dereference)
        {
        }
      else
        {
        }

      getToken();
    }
#endif

  /* Not a file-type variable.  Use the default file number (which we assume
   * to be of type sTEXTFILE)
   */

  else
    {
      fileType   = sTEXTFILE;
      fileSize   = sCHAR_SIZE;
      pas_GenerateDataOperation(opPUSH, INPUT_FILE_NUMBER);
    }

  if (pFileSize != NULL)
    {
      *pFileSize = fileSize;
    }

  return fileType;
}

/***********************************************************************/

int actualParameterSize(symbol_t *procPtr, int parmNo)
{
  /* These sizes must agree with the sizes used in actualParameterListg()
   * below.
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
    case sRSTRING :
      return sRSTRING_SIZE;
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

int actualParameterList(symbol_t *procPtr)
{
  symbol_t *typePtr;
  exprType_t exprType;
  bool lparen = false;
  int parmIndex = 0;
  int size = 0;

  TRACE(g_lstFile,"[actualParameterList]");

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
       * agree with actualParameterSize() above);
       */

      for (parmIndex = 1;
           parmIndex <= procPtr->sParm.p.nParms;
           parmIndex++)
        {
          typePtr = procPtr[parmIndex].sParm.v.parent;
          switch (procPtr[parmIndex].sKind)
            {
            case sINT :
              pas_Exression(exprInteger, typePtr);
              size += sINT_SIZE;
              break;

            case sCHAR :
              pas_Exression(exprChar, typePtr);
              size += sCHAR_SIZE;
              break;

            case sREAL :
              pas_Exression(exprReal, typePtr);
              size += sREAL_SIZE;
              break;

            case sSTRING :
            case sRSTRING :
              pas_Exression(exprString, typePtr);
              size += sRSTRING_SIZE;
              break;

            case sSUBRANGE :
              pas_Exression(exprInteger, typePtr);
              size += sINT_SIZE;
              break;

            case sSCALAR :
              pas_Exression(exprScalar, typePtr);
              size += sINT_SIZE;
              break;

            case sSET_OF :
              pas_Exression(exprSet, typePtr);
              size += sINT_SIZE;
              break;

            case sARRAY :
              {
                symbol_t *arrayType;
                symbol_t *nextType;
                uint16_t  arrayKind;

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
                pas_Exression(exprType, typePtr);
                size += typePtr->sParm.t.asize;
              }
              break;

            case sRECORD :
              pas_Exression(exprRecord, typePtr);
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

  fileType = generateFileNumber(INPUT_FILE_NUMBER, &fileSize);

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

          pas_GenerateDataOperation(opPUSH, INPUT_FILE_NUMBER);
        }
      else
        {
          /* Get TOS = file number */

          fileType = generateFileNumber(INPUT_FILE_NUMBER, &fileSize);

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

      pas_GenerateDataOperation(opPUSH, INPUT_FILE_NUMBER);
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
  symbol_t *rPtr;

  TRACE(g_lstFile, "[readText]");

  /* The general form is <VAR parm> */

  switch (g_token)
    {
      /* SPECIAL CASE: Array of type CHAR without indexing */

      case sARRAY :
        rPtr = g_tknPtr->sParm.v.parent;
        if (rPtr != NULL && rPtr->sKind == sTYPE &&
            rPtr->sParm.t.type == sCHAR &&
            getNextCharacter(true) != '[')
          {
            /* READ_STRING: TOS   = Read size
             *              TOS+1 = Read address
             *              TOS+2 = File number
             */

            pas_GenerateSimple(opDUP);
            pas_GenerateStackReference(opLAS, rPtr);
            pas_GenerateDataOperation(opPUSH, rPtr->sParm.v.size);
            pas_GenerateIoOperation(xREAD_STRING);
          }

        /* Otherwise, we fall through to process the ARRAY like any
         * expression.
         */

      default :
        /* Get TOS = file number by duplicating the one that is already
         * there (but that also need to preserve).
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

            /* READ_STRING: TOS   = Read size
             *              TOS+1 = Read address
             *              TOS+2 = File number
             */

            case exprString :
              pas_GenerateIoOperation(xREAD_STRING);
              break;

            default :
              error(eINVARG);
              break;
          }
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
      /* READ_BINARY: TOS   = Read size
       *              TOS+1 = Read address
       *              TOS+2 = File number
       */

      pas_GenerateSimple(opDUP);
      pas_GenerateLevelReference(opLAS, g_tknPtr->sLevel,
                                 g_tknPtr->sParm.v.offset);
      pas_GenerateDataOperation(opPUSH, size);
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
      (void)generateFileNumber(OUTPUT_FILE_NUMBER, NULL);

      /* Check for the option record-size */

      if (g_token == ',')
        {
          getToken();
          (void)pas_Exression(exprInteger, NULL);
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

      pas_GenerateDataOperation(opPUSH, OUTPUT_FILE_NUMBER);

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
      (void)generateFileNumber(OUTPUT_FILE_NUMBER, NULL);

      /* And generate the opcode */

      pas_GenerateIoOperation(opcode);

      /* Verify the closing right parenthesis */

      if (g_token != ')') error(eRPAREN);
      else getToken();
    }
  else
    {
      /* Assume the standard OUTPUT file? */

      pas_GenerateDataOperation(opPUSH, OUTPUT_FILE_NUMBER);

      /* And generate the opcode */

      pas_GenerateIoOperation(opcode);
    }
}

/****************************************************************************/

static void assignFileProc(void)       /* ASSIGNFILE procedure */
{
  uint32_t fileType;
  uint32_t exprType;

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

       /* Get TOS     = Pointer to string
        *     TOS + 1 = 0:binary 1:text
        *     TOS + 2 = File number
        */

       /* Push the file number onto the TOS */

       fileType = generateFileNumber(OUTPUT_FILE_NUMBER, NULL);

       /* The file variable must be followed by a comma. */

       if (g_token != ',') error(eCOMMA);
       else getToken();

       /* Push the file type:  binary or text */

       pas_GenerateDataOperation(opPUSH, (uint16_t)(fileType == sTEXTFILE));

       /* Push the file name reference onto the stack. */

       exprType = pas_Exression(exprUnknown, NULL);
       if (exprType != exprString) error(eSTRING);

       /* And generate the SYSIO operation */

       pas_GenerateIoOperation(txASSIGNFILE);

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

  /* FORM: WRITE   write-parameter-list
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

  fileType = generateFileNumber(OUTPUT_FILE_NUMBER, &fileSize);

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

          pas_GenerateDataOperation(opPUSH, OUTPUT_FILE_NUMBER);
        }
      else
        {
          /* Get TOS = file number */

          fileType = generateFileNumber(OUTPUT_FILE_NUMBER, &fileSize);

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

      pas_GenerateDataOperation(opPUSH, OUTPUT_FILE_NUMBER);
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
       * NOTE:  Some test cases use ',' as the separator.  Let's supported
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

        /* WRITE_STRING: TOS   = Write size
         *               TOS+1 = Write address
         *               TOS+2 = File number
         */

        /* Set the file number, offset and size on the stack */

        pas_GenerateSimple(opDUP);
        pas_GenerateDataOperation(opLAC, (uint16_t)offset);
        pas_GenerateDataOperation(opPUSH, strlen(g_tokenString));
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
      pas_GenerateDataOperation(opLAC, (uint16_t)g_tknPtr->sParm.s.offset);
      pas_GenerateDataOperation(opPUSH, (uint16_t)g_tknPtr->sParm.s.size);
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
          pas_GenerateStackReference(opLAS, wPtr);
          pas_GenerateDataOperation(opPUSH, wPtr->sParm.v.size);
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
      writeType = pas_Exression(exprUnknown, NULL);

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
        case exprStkString :
          /* WRITE_STRING: TOS   = Write size
           *               TOS+1 = Write address
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
      /* WRITE_BINARY: TOS   = Write size
       *               TOS+1 = Write address
       *               TOS+2 = File number
       */

      pas_GenerateSimple(opDUP);
      pas_GenerateLevelReference(opLAS, g_tknPtr->sLevel,
                                 g_tknPtr->sParm.v.offset);
      pas_GenerateDataOperation(opPUSH, size);
      pas_GenerateIoOperation(xWRITE_BINARY);
    }

  getToken();
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

  (void)actualParameterList(valSymbol);

  /* Generate the built-in procedure call.  NOTE the procedure call
   * logic will release the parameters from the stack saving us from
   * having to generate the INDS here.
   */

  pas_StandardFunctionCall(lbVAL);
}

/***********************************************************************/
