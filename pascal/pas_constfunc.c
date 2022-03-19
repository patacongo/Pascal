/***************************************************************
 * pas_constfunc.c
 * Standard Function operating on constant values
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
 ***************************************************************/

/***************************************************************
 * Included Files
 ***************************************************************/

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>

#include "pas_debug.h"
#include "pas_defns.h"
#include "pas_tkndefs.h"
#include "pas_pcode.h"
#include "pas_fpops.h"
#include "pas_errcodes.h"
#include "pas_sysio.h"

#include "pas_main.h"
#include "pas_expression.h"
#include "pas_function.h"
#include "pas_token.h"
#include "pas_codegen.h"
#include "pas_error.h"

/***************************************************************
 * Private Function Prototypes
 ***************************************************************/

/* Standard Pascal Functions */

static void pas_ConstantAbsFunc(void);    /* Integer absolute value */
static void pas_ConstantPredFunc(void);
static void pas_ConstantOrdFunc(void);    /* Convert scalar to integer */
static void pas_ConstantSqrFunc(void);
static void pas_ConstantRealFunc(uint8_t fpCode);
static void pas_ConstantSuccFunc(void);
static void pas_ConstantOddFunc(void);
static void pas_ConstantChrFunc(void);
static void pas_ConstantReal2IntFunc(int kind);
static void pas_IsOrdinalConstant(void);

/***************************************************************
 * Private Functions
 ***************************************************************/

/**********************************************************************/

static void pas_ConstantAbsFunc(void)
{
  /* FORM:  ABS (<simple integer/real expression>) */

  pas_CheckLParen();
  pas_ConstantExpression(exprUnknown, NULL);

  if (g_constantToken == tINT_CONST)
    {
      if (g_constantInt < 0)
        {
          g_constantInt = -g_constantInt;
        }
    }
  else if (g_constantToken == tREAL_CONST)
    {
      if (g_constantReal < 0)
        {
          g_constantReal = -g_constantInt;
        }
    }
  else
    {
      error(eINVARG);
    }

  pas_CheckRParen();
}

/**********************************************************************/

static void pas_ConstantOrdFunc(void)
{
  /* FORM:  ORD (<scalar type>) */

  pas_CheckLParen();
  pas_ConstantExpression(exprScalar, NULL);
  pas_IsOrdinalConstant();
  g_constantToken = tINT_CONST;
  pas_CheckRParen();
}

/**********************************************************************/

static void pas_ConstantPredFunc(void)
{
  /* FORM:  PRED (<simple integer expression>) */

  pas_CheckLParen();
  pas_ConstantExpression(exprInteger, NULL);
  pas_IsOrdinalConstant();
  g_constantInt--;
  pas_CheckRParen();
}

/**********************************************************************/

static void pas_ConstantSqrFunc(void)
{
  /* FORM:  SQR (<simple integer OR real expression>) */

  pas_CheckLParen();
  pas_ConstantExpression(exprUnknown, NULL);
  if (g_constantToken == tINT_CONST)
    {
      g_constantInt *= g_constantInt;
    }
  else if (g_constantToken == tREAL_CONST)
    {
      g_constantReal *= g_constantReal;
    }
  else
    {
      error(eINVARG);
    }

  pas_CheckRParen();
}

/**********************************************************************/

static void pas_ConstantRealFunc(uint8_t fpOpCode)
{
  /* FORM:  <function identifier> (<real/integer expression>) */

  pas_CheckLParen();
  pas_ConstantExpression(exprUnknown, NULL);
  if (g_constantToken == tINT_CONST)
    {
      g_constantReal = (double)g_constantInt;
    }
  else
    {
      error(eINVARG);
    }

  pas_CheckRParen();
}

/**********************************************************************/

static void pas_ConstantSuccFunc(void)
{
  /* FORM:  SUCC (<simple integer expression>) */

  pas_CheckLParen();
  pas_ConstantExpression(exprAnyOrdinal, NULL);
  pas_IsOrdinalConstant();
  g_constantInt++;
  pas_CheckRParen();
}

/***********************************************************************/

static void pas_ConstantOddFunc(void)
{
  /* FORM:  ODD (<simple integer expression>) */

  pas_CheckLParen();
  pas_ConstantExpression(exprInteger, NULL);
  pas_IsOrdinalConstant();
  g_constantInt &= 1;
  pas_Expression(exprAnyOrdinal, NULL);
  pas_CheckRParen();
}

/***********************************************************************/
/* Process the standard chr function */

static void pas_ConstantChrFunc(void)
{
  /* Form:  chr(integer expression).
   *
   * char(val) is only defined if there exists a character ch such
   * that ord(ch) = val.  If this is not the case, we will simply
   * let the returned value exceed the range of type char.
   */

  pas_CheckLParen();
  pas_ConstantExpression(exprInteger, NULL);
  if (g_constantToken == tINT_CONST)
    {
      g_constantToken = tCHAR_CONST;
    }
  else
    {
      error(eINVARG);
    }

  pas_CheckRParen();
}

/***********************************************************************/

static void pas_ConstantReal2IntFunc(int kind)
{
  error(eNOTYET);
}

/***********************************************************************/

static void pas_IsOrdinalConstant(void)
{
  if (g_constantToken == tINT_CONST     || /* integer value */
      g_constantToken == tCHAR_CONST    || /* character value */
      g_constantToken == tBOOLEAN_CONST)
    {
      return;
    }
  else
    {
      error(eINVARG);
    }
}

/***************************************************************
 * Public Functions
 ***************************************************************/

/***************************************************************/
/* Process a standard Pascal function call */

void pas_StandardFunctionOfConstant(void)
{
  /* Is the token a function? */

  if (g_token == tSTDFUNC)
    {
      /* Yes, process it according to the extended token type */

      switch (g_tknSubType)
        {
          /* Functions which return the same type as their argument */

        case txABS :
          pas_ConstantAbsFunc();
          break;

        case txSQR :
          pas_ConstantSqrFunc();
          break;

        case txPRED :
          pas_ConstantPredFunc();
          break;

        case txSUCC :
          pas_ConstantSuccFunc();
          break;

        case txSIZEOF :
          pas_SizeOfFunc();
          g_constantToken = tINT_CONST;
          break;

          /* Functions returning INTEGER with REAL arguments */

        case txROUND :
          pas_ConstantReal2IntFunc(fpROUND);
          break;

        case txTRUNC :
          pas_ConstantReal2IntFunc(fpTRUNC);
          break;

          /* Functions returning CHARACTER with INTEGER arguments. */

        case txCHR :
          pas_ConstantChrFunc();
          break;

          /* Function returning integer with scalar arguments */

        case txORD :
          pas_ConstantOrdFunc();
          break;

          /* Functions returning BOOLEAN */

        case txODD :
          pas_ConstantOddFunc();
          break;

          /* Functions returning REAL with REAL/INTEGER arguments */

        case txSQRT :
          pas_ConstantRealFunc(fpSQRT);
          break;

        case txSIN :
          pas_ConstantRealFunc(fpSIN);
          break;

        case txCOS :
          pas_ConstantRealFunc(fpCOS);
          break;

        case txARCTAN :
          pas_ConstantRealFunc(fpATAN);
          break;

        case txLN :
          pas_ConstantRealFunc(fpLN);
          break;

        case txEXP :
          pas_ConstantRealFunc(fpEXP);
          break;

        case txEOLN :
        case txEOF :
        default :
          error(eINVALIDFUNC);
          break;
        }
    }
}
