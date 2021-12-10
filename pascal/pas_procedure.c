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

#include "keywords.h"
#include "pas_defns.h"
#include "pas_tkndefs.h"
#include "pas_pcode.h"
#include "pas_errcodes.h"
#include "pxdefs.h"

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

static void    haltProc    (void);                /* HALT procedure */
static int16_t readProc    (void);                /* READ procedure */
static void    readText    (uint16_t fileNumber); /* READ text file */
static void    readlnProc  (void);                /* READLN procedure */
static void    fileProc    (uint16_t opcode);     /* RESET/REWRITE/PAGE procedure */
static int16_t writeProc   (void);                /* WRITE procedure */
static void    writeText   (uint16_t fileNumber); /* WRITE text file */
static void    writelnProc (void);                /* WRITELN procedure */

/* Helpers for less-than-standard procedures */

static void   valProc      (void);                /* VAL procedure */

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* procedure val(const S : string; var V; var Code : word); */

static symbol_t valSymbol[4];

/****************************************************************************/

void primeBuiltInProcedures(void)
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

void builtInProcedure(void)
{
  TRACE(g_lstFile, "[builtInProcedure]");

  /* Is the token a procedure? */

  if (g_token == tPROC)
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

        case txREAD :
          getToken();
          (void)readProc();
          break;

        case txREADLN :
          readlnProc();
          break;

        case txRESET  :
          fileProc(xRESET);
          break;

        case txREWRITE :
          fileProc(xREWRITE);
          break;

        case txWRITE :
          getToken();
          (void)writeProc();
          break;

        case txWRITELN :
          writelnProc();
          break;

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

          /* Its not a recognized procedure */

        default :
          error(eINVALIDPROC);
          break;
        }
    }
}

/***********************************************************************/

symbol_t *getFileBaseType(void)
{
  symbol_t *typePtr = g_tknPtr;
  symbol_t fake;

  /* Check if this is a VAR parameter */

  if (typePtr->sKind == sVAR_PARM)
    {
      /* Use the parent type.  The parent of a a VAR parameter is a type */

      typePtr = typePtr->sParm.v.parent;
    }

  /* Is this a typedef'ed type? */

  while (typePtr->sKind == sTYPE)
    {
      /* This is the final type if it has no parent.
       */

      symbol_t *tmpPtr = typePtr->sParm.t.parent;
      if (tmpPtr != NULL)
        {
          typePtr = tmpPtr;
        }
      else
        {
          break;
        }
    }

  /* Is this a dereferenced pointer to a file type? */
  /* REVISIT:  No implemented. */

  /* We should have the base type..  Is it a file? */

  if (typePtr->sKind == sFILE || typePtr->sKind == sTEXT)
    {
      return typePtr;
    }

  /* Is this a file type? */

  if (typePtr->sKind == sTYPE)
    {
      /* Typedef'ed symbols look different from sKind or sFILE.
       * I suppose we need to create a fake file symbol?
       */
    }

  return NULL;
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
  register int nParms = 0;
  bool lparen = false;
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

      for (nParms = 1; nParms <= procPtr->sParm.p.nParms; nParms++)
        {
          typePtr = procPtr[nParms].sParm.v.parent;
          switch (procPtr[nParms].sKind)
            {
            case sINT :
              expression(exprInteger, typePtr);
              size += sINT_SIZE;
              break;

            case sCHAR :
              expression(exprChar, typePtr);
              size += sCHAR_SIZE;
              break;

            case sREAL :
              expression(exprReal, typePtr);
              size += sREAL_SIZE;
              break;

            case sSTRING :
            case sRSTRING :
              expression(exprString, typePtr);
              size += sRSTRING_SIZE;
              break;

            case sSUBRANGE :
              expression(exprInteger, typePtr);
              size += sINT_SIZE;
              break;

            case sSCALAR :
              expression(exprScalar, typePtr);
              size += sINT_SIZE;
              break;

            case sSET_OF :
              expression(exprSet, typePtr);
              size += sINT_SIZE;
              break;

            case sARRAY :
              expression(exprArray, typePtr);
              size += typePtr->sParm.t.asize;
              break;

            case sRECORD :
              expression(exprRecord, typePtr);
              size += typePtr->sParm.t.asize;
              break;

            case sVAR_PARM :
              if (typePtr)
                {
                  switch (typePtr->sParm.t.type)
                    {
                    case sINT :
                      varParm(exprIntegerPtr, typePtr);
                      size += sPTR_SIZE;
                      break;

                    case sBOOLEAN :
                      varParm(exprBooleanPtr, typePtr);
                      size += sPTR_SIZE;
                      break;

                    case sCHAR :
                      varParm(exprCharPtr, typePtr);
                      size += sPTR_SIZE;
                      break;

                    case sREAL :
                      varParm(exprRealPtr, typePtr);
                      size += sPTR_SIZE;
                      break;

                    case sARRAY :
                      varParm(exprArrayPtr, typePtr);
                      size += sPTR_SIZE;
                      break;

                    case sRECORD :
                      varParm(exprRecordPtr, typePtr);
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

          if (nParms < procPtr->sParm.p.nParms)
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

  pas_BuiltInFunctionCall(lbHALT);
}

/***********************************************************************/

static int16_t readProc(void)
{
  uint16_t fileNumber;
  symbol_t *filePtr;

  TRACE(g_lstFile, "[readProc]");

  /* FORM:
   *  (1) Binary READ: read '(' file-variable ')'
   *  (2) Test   READ: read read-parameter-list
   * FORM: read-parameter-list =
   *   '(' [ file-variable ',' ] variable-access { ',' variable-access } ')'
   */

  if (g_token != '(') error (eLPAREN);   /* Skip over '(' */
  else getToken();

  /* Get file number */

  filePtr = getFileBaseType();
  if (filePtr == NULL)
    {
      fileNumber = INPUT_FILE_NUMBER;
    }
  else
    {
      fileNumber = g_tknPtr->sParm.f.fileNumber;

      getToken();
      if (g_token == ',') getToken();
    }

  /* Determine if this is a text or binary file */

  if (!(g_files[fileNumber].defined)) error (eUNDEFILE);
  else if (g_files[fileNumber].ftype == sCHAR)
    {
      readText(fileNumber);
    }
  else
    {
      pas_GenerateLevelReference(opLAS, g_files[fileNumber].flevel, g_files [fileNumber].faddr);
      pas_GenerateDataOperation(opPUSH, g_files[fileNumber].fsize);
      pas_GenerateIoOperation(xREAD_BINARY, fileNumber);
    }

  if (g_token != ')') error (eRPAREN);
  else getToken();

  return fileNumber;
}

/***********************************************************************/

static void readText(uint16_t fileNumber)
{
  symbol_t *rPtr;

  TRACE(g_lstFile, "[readText]");

  /* The general form is <VAR parm>, <VAR parm>,... */

  for (; ; )
    {
      switch (g_token)
        {
          /* SPECIAL CASE: Array of type CHAR without indexing */

        case sARRAY :
          rPtr = g_tknPtr->sParm.v.parent;
          if (((rPtr) && (rPtr->sKind == sTYPE)) &&
              (rPtr->sParm.t.type == sCHAR) &&
              (getNextCharacter(true) != '['))
            {
              pas_GenerateStackReference(opLAS, rPtr);
              pas_GenerateDataOperation(opPUSH, rPtr->sParm.v.size);
              pas_GenerateIoOperation(xREAD_STRING, fileNumber);
              pas_GenerateDataOperation(opINDS, -(sPTR_SIZE + sINT_SIZE));
            }

          /* Otherwise, we fall through to process the ARRAY like any
           * expression.
           */

        default :

          switch (varParm(exprUnknown, NULL))
            {
            case exprIntegerPtr :
              pas_GenerateIoOperation(xREAD_INT, fileNumber);
              pas_GenerateDataOperation(opINDS, -sPTR_SIZE);
              break;

            case exprCharPtr :
              pas_GenerateIoOperation(xREAD_CHAR, fileNumber);
              pas_GenerateDataOperation(opINDS, -sPTR_SIZE);
              break;

            case exprRealPtr :
              pas_GenerateIoOperation(xREAD_REAL, fileNumber);
              pas_GenerateDataOperation(opINDS, -sPTR_SIZE);
              break;

            default :
              error(eINVARG);
              break;
            }
          break;
        }

      if (g_token == ',') getToken();
      else return;
    }
}

/****************************************************************************/

static void readlnProc(void)          /* READLN procedure */
{
  int32_t fileNumber;

  TRACE(g_lstFile, "[readlnProc]");

  /* FORM:  Just like READ */

  getToken();
  if (g_token == '(')
    {
      fileNumber = readProc();
    }

   /* skip to end-of-line mark in the file (NOTE:  No check is made,
    * but this is meaningful only for a test file).
    */

   pas_GenerateIoOperation(xREADLN, fileNumber);
}

/****************************************************************************/
/* REWRITE/RESET/PAGE procedure call -- REWRITE sets the file pointer to the
 * beginning of the file and prepares the file for write access; RESET is
 * similar except that it prepares the file for read access; PAGE simply
 * writes a form-feed to the file (no check is made, but is meaningful only
 * for a text file). */

static void fileProc (uint16_t opcode)
{
   TRACE(g_lstFile, "[fileProc]");

   /* FORM: RESET|REWRITE(<file number>) */

   getToken();
   if (g_token != '(') error(eLPAREN);
   else getToken();
   if (g_token !=  sFILE) error(eFILE);
   else
     {
       pas_GenerateIoOperation(opcode, g_tknPtr->sParm.f.fileNumber);
       getToken();
       if (g_token != ')') error(eRPAREN);
       else getToken();
     }
}

/***********************************************************************/

static int16_t writeProc(void)
{
   uint16_t fileNumber = 0;

   TRACE(g_lstFile, "[writeProc]");

   /* FORM: (1) Binary WRITE:  WRITE(<fileNumber>);
    *       (2) Test   WRITE:  WRITE([<fileNumber>], arg1 [,arg2 [...]]) */

   if (g_token != '(') error(eLPAREN);   /* Skip over '(' */
   else getToken();

   /* Get file number */

   if (g_token == sFILE)
     {
       fileNumber = g_tknPtr->sParm.f.fileNumber;
       getToken();
     }

   if (g_token == ',') getToken();

   /* Determine if this is a text or binary file */

   if (!(g_files [fileNumber].defined)) error(eUNDEFILE);
   else if (g_files [fileNumber].ftype == sCHAR)
     writeText(fileNumber);
   else
     {
       pas_GenerateLevelReference(opLAS, g_files[fileNumber].flevel, g_files [fileNumber].faddr);
       pas_GenerateDataOperation(opPUSH, g_files[fileNumber].fsize);
       pas_GenerateIoOperation(xWRITE_BINARY, fileNumber);
     }

   if (g_token != ')') error(eRPAREN);
   else getToken();
   return(fileNumber);
}

/***********************************************************************/

static void writeText (uint16_t fileNumber)
{
  exprType_t writeType;
  symbol_t  *wPtr;

  TRACE(g_lstFile, "[writeText]");

  for (;;)
    {
      /* The general form is <expression>, <expression>, ...  However,
       * there are a few unique things that must be handled as special
       * cases
       */

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

            /* Set the offset and size on the stack (order is important) */

            pas_GenerateDataOperation(opLAC, (uint16_t)offset);
            pas_GenerateDataOperation(opPUSH, strlen(g_tokenString));

            pas_GenerateIoOperation(xWRITE_STRING, fileNumber);
            pas_GenerateDataOperation(opINDS, -(sPTR_SIZE + sINT_SIZE));
            g_stringSP = g_tokenString;
            getToken();
          }
          break;

        case sSTRING_CONST :
          pas_GenerateDataOperation(opLAC, (uint16_t)g_tknPtr->sParm.s.offset);
          pas_GenerateDataOperation(opPUSH, (uint16_t)g_tknPtr->sParm.s.size);
          pas_GenerateIoOperation(xWRITE_STRING, fileNumber);
          pas_GenerateDataOperation(opINDS, -(sPTR_SIZE + sINT_SIZE));
          getToken();
          break;

          /* Array of type CHAR without indexing */

        case sARRAY :
          wPtr = g_tknPtr->sParm.v.parent;
          if (((wPtr) && (wPtr->sKind == sTYPE)) &&
              (wPtr->sParm.t.type == sCHAR) &&
              (getNextCharacter(true) != '['))
            {
              pas_GenerateStackReference(opLAS, wPtr);
              pas_GenerateDataOperation(opPUSH, wPtr->sParm.v.size);
              pas_GenerateIoOperation(xWRITE_STRING, fileNumber);
              pas_GenerateDataOperation(opINDS, -(sPTR_SIZE + sINT_SIZE));
              break;
            }

          /* Otherwise, we fall through to process the ARRAY like any */
          /* expression */

        default :
          writeType = expression(exprUnknown, NULL);
          switch (writeType)
            {
            case exprInteger :
              pas_GenerateIoOperation(xWRITE_INT, fileNumber);
              pas_GenerateDataOperation(opINDS, -sINT_SIZE);
              break;

            case exprBoolean :
              error(eNOTYET);
              break;

            case exprChar :
              pas_GenerateIoOperation(xWRITE_CHAR, fileNumber);
              pas_GenerateDataOperation(opINDS, -sINT_SIZE);
              break;

            case exprReal :
              pas_GenerateIoOperation(xWRITE_REAL, fileNumber);
              pas_GenerateDataOperation(opINDS, -sREAL_SIZE);
              break;

            case exprString :
            case exprStkString :
              pas_GenerateIoOperation(xWRITE_STRING, fileNumber);
              pas_GenerateDataOperation(opINDS, -sRSTRING_SIZE);
              break;

            default :
              error(eWRITEPARM);
              break;

            }
          break;

        }

      if (g_token == ',') getToken();
      else return;
    }
}

/****************************************************************************/

static void writelnProc(void)         /* WRITELN procedure */
{
   int32_t fileNumber = 0;

   TRACE(g_lstFile, "[writelnProc]");

   /* FORM:  Just like WRITE */

   getToken();
   if (g_token == '(')
     {
       fileNumber = writeProc();
     }

   /* Skip to past end-of-line mark in the file (NOTE:  No check is made, but
    * this is meaningful only for a test file).
    */

   pas_GenerateIoOperation(xWRITELN, fileNumber);
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

  pas_BuiltInFunctionCall(lbVAL);
}

/***********************************************************************/
