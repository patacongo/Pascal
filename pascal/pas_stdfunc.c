/***************************************************************
 * pas_stdfunc.c
 * Standard Functions
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
 ***************************************************************/

/***************************************************************
 * Included Files
 ***************************************************************/

#include <stdint.h>
#include <stdio.h>

#include "keywords.h"
#include "pas_defns.h"
#include "pas_tkndefs.h"
#include "podefs.h"
#include "pfdefs.h"
#include "pedefs.h"
#include "pxdefs.h"

#include "pas_main.h"
#include "pas_expression.h"
#include "pas_function.h"
#include "pas_codegen.h"  /* for pas_Generate*() */
#include "pas_token.h"
#include "pas_symtable.h"
#include "pinsn.h"
#include "pas_error.h"

/***************************************************************
 * Private Function Prototypes
 ***************************************************************/

/* Standard Pascal Functions */

static exprType_t absFunc(void);    /* Integer absolute value */
static exprType_t predFunc(void);
static void       ordFunc(void);    /* Convert scalar to integer */
static exprType_t sqrFunc(void);
static void       realFunc(uint8_t fpCode);
static exprType_t succFunc(void);
static void       oddFunc(void);
static void       chrFunc(void);
static void       fileFunc(uint16_t opcode);

/* Enhanced Pascal functions */

/* Non-standard C-library interface functions */

static exprType_t getenvFunc (void);    /* Get environment string value */

/***************************************************************
 * Public Functions
 ***************************************************************/

void primeBuiltInFunctions(void)
{
}

/***************************************************************/
/* Process a standard Pascal function call */

exprType_t builtInFunction(void)
{
  exprType_t funcType = exprUnknown;

  TRACE(g_lstFile,"[builtInFunction]");

  /* Is the token a function? */

  if (g_token == tFUNC)
    {
      /* Yes, process it procedure according to the extended token type */

      switch (g_tknSubType)
        {
          /* Functions which return the same type as their argument */
        case txABS :
          funcType = absFunc();
          break;

        case txSQR :
          funcType = sqrFunc();
          break;

        case txPRED :
          funcType = predFunc();
          break;

        case txSUCC :
          funcType = succFunc();
          break;

        case txGETENV : /* Non-standard C library interfaces */
          funcType = getenvFunc();
          break;

          /* Functions returning INTEGER with REAL arguments */

        case txROUND :
          getToken();                          /* Skip over 'round' */
          expression(exprReal, NULL);
          pas_GenerateFpOperation(fpROUND);
          funcType = exprInteger;
          break;

        case txTRUNC :
          getToken();                          /* Skip over 'trunc' */
          expression(exprReal, NULL);
          pas_GenerateFpOperation(fpTRUNC);
          funcType = exprInteger;
          break;

          /* Functions returning CHARACTER with INTEGER arguments. */

        case txCHR :
          chrFunc();
          funcType = exprChar;
          break;

          /* Function returning integer with scalar arguments */

        case txORD :
          ordFunc();
          funcType = exprInteger;
          break;

          /* Functions returning BOOLEAN */

        case txODD :
          oddFunc();
          funcType = exprBoolean;
          break;

        case txEOF :
          fileFunc(xEOF);
          funcType = exprBoolean;
          break;

        case txEOLN :
          fileFunc(xEOLN);
          funcType = exprBoolean;
          break;

          /* Functions returning REAL with REAL/INTEGER arguments */

        case txSQRT :
          realFunc(fpSQRT);
          funcType = exprReal;
          break;

        case txSIN :
          realFunc(fpSIN);
          funcType = exprReal;
          break;

        case txCOS :
          realFunc(fpCOS);
          funcType = exprReal;
          break;

        case txARCTAN :
          realFunc(fpATAN);
          funcType = exprReal;
          break;

        case txLN :
          realFunc(fpLN);
          funcType = exprReal;
          break;

        case txEXP :
          realFunc(fpEXP);
          funcType = exprReal;
          break;

        default :
          error(eINVALIDPROC);
          break;

        }
    }

  return funcType;
}

void checkLParen(void)
{
   getToken();                          /* Skip over function name */
   if (g_token != '(') error(eLPAREN);  /* Check for '(' */
   else getToken();
}

void checkRParen(void)
{
   if (g_token != ')') error(eRPAREN);  /* Check for ')') */
   else getToken();
}

/***************************************************************
 * Private Functions
 ***************************************************************/

static exprType_t absFunc(void)
{
   exprType_t absType;

   TRACE(g_lstFile,"[absFunc]");

   /* FORM:  ABS (<simple integer/real expression>) */

   checkLParen();

   absType = expression(exprUnknown, NULL);
   if (absType == exprInteger)
      pas_GenerateSimple(opABS);
   else if (absType == exprReal)
      pas_GenerateFpOperation(fpABS);
   else
      error(eINVARG);

   checkRParen();
   return absType;
}

/**********************************************************************/

static void ordFunc(void)
{
   TRACE(g_lstFile,"[ordFunc]");

   /* FORM:  ORD (<scalar type>) */

   checkLParen();
   expression(exprAnyOrdinal, NULL);     /* Get any ordinal type */
   checkRParen();
}

/**********************************************************************/

static exprType_t predFunc(void)
{
   exprType_t predType;

   TRACE(g_lstFile,"[predFunc]");

   /* FORM:  PRED (<simple integer expression>) */

   checkLParen();

   /* Process any ordinal expression */

   predType = expression(exprAnyOrdinal, NULL);
   checkRParen();
   pas_GenerateSimple(opDEC);
   return predType;
}

/**********************************************************************/

static exprType_t sqrFunc(void)
{
   exprType_t sqrType;

   TRACE(g_lstFile,"[sqrFunc]");

/* FORM:  SQR (<simple integer OR real expression>) */

   checkLParen();

   sqrType = expression(exprUnknown, NULL); /* Process any expression */
   if (sqrType == exprInteger) {

     pas_GenerateSimple(opDUP);
     pas_GenerateSimple(opMUL);

   }
   else if (sqrType == exprReal)
     pas_GenerateFpOperation(fpSQR);

   else
     error(eINVARG);

   checkRParen();
   return sqrType;
}

/**********************************************************************/

static void realFunc (uint8_t fpOpCode)
{
   exprType_t realType;

   TRACE(g_lstFile,"[realFunc]");

   /* FORM:  <function identifier> (<real/integer expression>) */

   checkLParen();

   realType = expression(exprUnknown, NULL); /* Process any expression */
   if (realType == exprInteger)
     pas_GenerateFpOperation((fpOpCode | fpARG1));
   else if (realType == exprReal)
     pas_GenerateFpOperation(fpOpCode);
   else
     error(eINVARG);

   checkRParen();
}

/**********************************************************************/

static exprType_t succFunc(void)
{
   exprType_t succType;

   TRACE(g_lstFile,"[succFunc]");

   /* FORM:  SUCC (<simple integer expression>) */

   checkLParen();

   /* Process any ordinal expression */

   succType = expression(exprAnyOrdinal, NULL);

   checkRParen();
   pas_GenerateSimple(opINC);
   return succType;
}

/***********************************************************************/

static void oddFunc(void)
{
   TRACE(g_lstFile,"[oddFunc]");

   /* FORM:  ODD (<simple integer expression>) */

   checkLParen();

   /* Process any ordinal expression */

   expression(exprAnyOrdinal, NULL);
   checkRParen();
   pas_GenerateDataOperation(opPUSH, 1);
   pas_GenerateSimple(opAND);
   pas_GenerateSimple(opNEQZ);
}

/***********************************************************************/
/* Process the standard chr function */

static void chrFunc(void)
{
   TRACE(g_lstFile,"[charFactor]");

   /* Form:  chr(integer expression).
    *
    * char(val) is only defined if there exists a character ch such
    * that ord(ch) = val.  If this is not the case, we will simply
    * let the returned value exceed the range of type char. */

   checkLParen();
   expression(exprInteger, NULL);
   checkRParen();
}

/****************************************************************************/
/* EOF/EOLN function */

static void fileFunc(uint16_t opcode)
{
  TRACE(g_lstFile,"[fileFunc]");

  /* FORM: EOF|EOLN (<file number>)
   *
   * The optional <file number> parameter is a reference to a file variable.
   * If the optional parameter is supplied then the eof function tests the
   * file associated with the parameter. If the optional parameter is not
   * supplied then the file associated with the built-in variable input is
   * used.
   */

  getToken();          /* Skip over function name */
  if (g_token == '(')  /* Check for '(' */
    {
      /* Get the file number argument */

      getToken();
      if (g_token != sFILE) error(eFILE);
      else
        {
          /* Generate the I/O operation */

          pas_GenerateDataOperation(opINDS, sBOOLEAN_SIZE);
          pas_GenerateIoOperation(opcode, g_tknPtr->sParm.fileNumber);
        }

      checkRParen();
    }
  else
    {
      pas_GenerateDataOperation(opINDS, sBOOLEAN_SIZE);
      pas_GenerateIoOperation(opcode, INPUT_FILE_NUMBER);
    }
}

/**********************************************************************/
/* C library getenv interface */

static exprType_t getenvFunc(void)
{
  exprType_t stringType;

  TRACE(g_lstFile, "[getenvFunc]");

  /* FORM:  <string_var> = getenv(<string>) */

  checkLParen();

  /* Get the string expression representing the environment variable
   * name.
   */

  stringType = expression(exprString, NULL);

  /* Two possible kinds of strings could be returned.
   * Anything else other then 'exprString' would be an error (but
   * should happen).
   */

  if ((stringType != exprString) && (stringType != exprStkString))
    {
      error(eINVARG);
    }

  pas_BuiltInFunctionCall(lbGETENV);
  checkRParen();
  return exprCString;
}

/***********************************************************************/
