/***************************************************************
 * pas_constexpr.c
 * Constant expression evaluation
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
#include <string.h>
#include <math.h>

#include "pas_debug.h"
#include "pas_defns.h"
#include "pas_tkndefs.h"
#include "pas_errcodes.h"

#include "pas_debug.h"
#include "pas_main.h"
#include "pas_statement.h"
#include "pas_expression.h"
#include "pas_function.h"
#include "pas_token.h"
#include "pas_error.h"

/***************************************************************
 * Pre-processor Definitions
 ***************************************************************/

#define isRelationalOperator(t) \
   (((t) == tEQ) || ((t) == tNE) || \
    ((t) == tLT) || ((t) == tLE) || \
    ((t) == tGT) || ((t) == tGE) || \
    ((t) == tIN))

#define isRelationalType(t) \
   (((t) == tINT_CONST) || ((t) == tCHAR_CONST) || \
    (((t) == tBOOLEAN_CONST) || ((t) == tREAL_CONST)))

#define isAdditiveType(t) \
   (((t) == tINT_CONST) || ((t) == tREAL_CONST))

#define isMultiplicativeType(t) \
   (((t) == tINT_CONST) || ((t) == tREAL_CONST))

#define isLogicalType(t) \
   (((t) == tINT_CONST) || ((t) == tBOOLEAN_CONST))

/***************************************************************
 * Private Type Declarations
 ***************************************************************/

/***************************************************************
 * Private Function Prototypes
 ***************************************************************/

static void pas_ConstantSimpleExpression(void);
static void pas_ConstantTerm(void);
static void pas_ConstantFactor(void);

/***************************************************************
 * Public Data
 ***************************************************************/

int     g_constantToken;
int32_t g_constantInt;
double  g_constantReal;
char   *g_constantStart;

/***************************************************************
 * Private Variables
 ***************************************************************/

/***************************************************************/
/* Evaluate a simple expression of constant values */

void pas_ConstantExpression(void)
{
  TRACE(g_lstFile,"[constantExpression]");

  /* Get the value of a simple constant expression */

  pas_ConstantSimpleExpression();

  /* Is it followed by a relational operator? */

  if (isRelationalOperator(g_token) && isRelationalType(g_constantToken))
    {
      int simple1        = g_constantToken;
      int32_t simple1Int = g_constantInt;
      double simple1Real = g_constantReal;
      int operator       = g_token;

      /* Get the second simple expression */

      pas_ConstantSimpleExpression();
      if (simple1 != g_constantToken)
        {
          /* Handle the case where the 1st argument is REAL and the
           * second is INTEGER. */

          if ((simple1 == tREAL_CONST) && (g_constantToken == tINT_CONST))
            {
              simple1Real = (double)simple1Int;
              simple1     = tREAL_CONST;
            }

          /* Handle the case where the 1st argument is Integer and the
           * second is REAL. */

          else if ((simple1 == tINT_CONST) && (g_constantToken == tREAL_CONST))
            {
              g_constantReal = (double)g_constantInt;
            }

          /* Allow the case of <scalar type> IN <set type>
           * Otherwise, the two terms must agree in type
           * -- NOT YET implemented.
           */

          else
            {
              error(eEXPRTYPE);
            }
        }

      /* Generate the comparison by type */

      switch (simple1)
        {
        case tINT_CONST :
        case tCHAR_CONST :
        case tBOOLEAN_CONST :
          switch (operator)
            {
            case tEQ :
              g_constantInt = (simple1Int == g_constantInt);
              break;
            case tNE :
              g_constantInt = (simple1Int != g_constantInt);
              break;
            case tLT :
              g_constantInt = (simple1Int < g_constantInt);
              break;
            case tLE :
              g_constantInt = (simple1Int <= g_constantInt);
              break;
            case tGT :
              g_constantInt = (simple1Int > g_constantInt);
              break;
            case tGE :
              g_constantInt = (simple1Int >= g_constantInt);
              break;
            case tIN :
              /* Not yet */
            default  :
              error(eEXPRTYPE);
              break;
            }
          break;

        case tREAL_CONST:
          switch (operator)
            {
            case tEQ :
              g_constantInt = (simple1Real == g_constantReal);
              break;
            case tNE :
              g_constantInt = (simple1Real != g_constantReal);
              break;
            case tLT :
              g_constantInt = (simple1Real < g_constantReal);
              break;
            case tLE :
              g_constantInt = (simple1Real <= g_constantReal);
              break;
            case tGT :
              g_constantInt = (simple1Real > g_constantReal);
              break;
            case tGE :
              g_constantInt = (simple1Real >= g_constantReal);
              break;
            case tIN :
              /* Not yet */
            default :
              error(eEXPRTYPE);
              break;
            }
          break;

        default :
          error(eEXPRTYPE);
          break;
        }

      /* The type resulting from these operations becomes BOOLEAN */

      g_constantToken = tBOOLEAN_CONST;
    }
}

/***************************************************************/
/* Process Simple Expression */

static void pas_ConstantSimpleExpression(void)
{
  int16_t unary = ' ';
  int     term;
  int32_t termInt;
  double  termReal;

  TRACE(g_lstFile,"[pas_ConstantSimpleExpression]");

  /* FORM: [+|-] <term> [{+|-} <term> [{+|-} <term> [...]]] */
  /* get +/- unary operation */

  if ((g_token == '+') || (g_token == '-'))
    {
      unary = g_token;
      getToken();
    }

  /* Process first (non-optional) term and apply unary operation */

  pas_ConstantTerm();
  term = g_constantToken;
  if ((unary != ' ') && !isAdditiveType(term))
    {
      error(eINVSIGNEDCONST);
    }
  else if (unary == '-')
    {
      termInt  = -g_constantInt;
      termReal = -g_constantReal;
    }
  else
    {
      termInt  = g_constantInt;
      termReal = g_constantReal;
    }

  /* Process subsequent (optional) terms and binary operations */

  for (; ; )
    {
      int operator;

      /* Check for binary operator */

      if ((((g_token == '+') || (g_token == '-')) )&& isAdditiveType(term))
        operator = g_token;
      else if ((g_token == tOR) && isLogicalType(term))
        operator = g_token;
      else
        break;

      /* Get the 2nd term */

      getToken();
      pas_ConstantTerm();

      /* Before generating the operation, verify that the types match.
       * Perform automatic type conversion from INTEGER to REAL as
       * necessary.
       */

      if (term != g_constantToken)
        {
          /* Handle the case where the 1st argument is REAL and the
           * second is INTEGER. */

          if ((term == tREAL_CONST) && (g_constantToken == tINT_CONST))
            {
              g_constantReal  = (double)g_constantInt;
              g_constantToken = tREAL_CONST;
            }

          /* Handle the case where the 1st argument is Integer and the
           * second is REAL. */

          else if ((term == tINT_CONST) && (g_constantToken == tREAL_CONST))
            {
              termReal = (double)termInt;
              term = tREAL_CONST;
            }

          /* Otherwise, the two terms must agree in type */

          else
            {
              error(eTERMTYPE);
            }
        }


      /* Perform the selected binary operation */

      switch (term)
        {
        case tINT_CONST :
          if (operator == '+')
            {
              termInt += g_constantInt;
            }
          else
            {
              termInt -= g_constantInt;
            }
          break;

        case tREAL_CONST :
          if (operator == '+')
            {
              termReal += g_constantReal;
            }
          else
            {
              termReal -= g_constantReal;
            }
          break;

        case tBOOLEAN_CONST :
          termInt |= g_constantInt;
          break;

        default :
          error(eEXPRTYPE);
          break;
        }
    }

  g_constantToken = term;
  g_constantInt   = termInt;
  g_constantReal  = termReal;
}

/***************************************************************/
/* Evaluate a TERM */

void pas_ConstantTerm(void)
{
  int     operator;
  int     factor;
  int32_t factorInt;
  double  factorReal;

  TRACE(g_lstFile,"[pas_ConstantTerm]");

  /* FORM:  <factor> [<operator> <factor>[<operator><factor>[...]]] */

  pas_ConstantFactor();
  factor     = g_constantToken;
  factorInt  = g_constantInt;
  factorReal = g_constantReal;

  for (; ; )
    {
        /* Check for binary operator */

        if (((g_token == tMUL) || (g_token == tMOD)) &&
            (isMultiplicativeType(factor)))
          operator = g_token;
        else if (((g_token == tDIV) || (g_token == tSHL) || (g_token == tSHR)) &&
                 (factor == tINT_CONST))
          operator = g_token;
        else if ((g_token == tFDIV) && (factor == tREAL_CONST))
          operator = g_token;
#if 0
        else if ((g_token == tFDIV) && (factor == tINT_CONST))
          {
            factorReal = (double)factorInt;
            factor     = tREAL_CONST;
            operator  = g_token;
          }
#endif
        else if ((g_token == tAND) && isLogicalType(factor))
          operator = g_token;
        else
          {
            g_constantToken = factor;
            g_constantInt   = factorInt;
            g_constantReal  = factorReal;
            break;
          }

        /* Get the next factor */

        getToken();
        pas_ConstantFactor();

        /* Before generating the operation, verify that the types match.
         * Perform automatic type conversion from INTEGER to REAL as
         * necessary.
         */

        if (factor != g_constantToken)
          {
            /* Handle the case where the 1st argument is REAL and the
             * second is INTEGER. */

            if ((factor == tREAL_CONST) && (g_constantToken == tINT_CONST))
              {
                g_constantReal = (double)g_constantInt;
              }

            /* Handle the case where the 1st argument is Integer and the
             * second is REAL. */

            else if ((factor == tINT_CONST) && (g_constantToken == tREAL_CONST))
              {
                factorReal = (double)factorInt;
                factor = tREAL_CONST;
              }

            /* Otherwise, the two factors must agree in type */

            else
              {
                error(eFACTORTYPE);
              }
          }

        /* Generate code to perform the selected binary operation */

        switch (operator)
          {
          case tMUL :
            if (factor == tINT_CONST)
              factorInt *= g_constantInt;
            else if (factor == tREAL_CONST)
              factorReal *= g_constantReal;
            else
              error(eFACTORTYPE);
            break;

          case tDIV :
            if (factor == tINT_CONST)
              factorInt /= g_constantInt;
            else
              error(eFACTORTYPE);
            break;

          case tFDIV :
            if (factor == tREAL_CONST)
              factorReal /= g_constantReal;
            else
              error(eFACTORTYPE);
            break;

          case tMOD :
            if (factor == tINT_CONST)
              factorInt %= g_constantInt;
            else if (factor == tREAL_CONST)
              factorReal = fmod(factorReal, g_constantReal);
            else
              error(eFACTORTYPE);
            break;

          case tAND :
            if ((factor == tINT_CONST) || (factor == tBOOLEAN_CONST))
              factorInt &= g_constantInt;
            else
              error(eFACTORTYPE);
            break;

          case tSHL :
            if (factor == tINT_CONST)
              factorInt <<= g_constantInt;
            else
              error(eFACTORTYPE);
            break;

          case tSHR :
            if (factor == tINT_CONST)
              factorInt >>= g_constantInt;
            else
              error(eFACTORTYPE);
            break;
        }
    }
}

/***************************************************************/
/* Process a constant FACTOR */

static void pas_ConstantFactor(void)
{
  TRACE(g_lstFile,"[pas_ConstantFactor]");

  /* Process by token type */

  switch (g_token)
    {
    case tINT_CONST :
    case tBOOLEAN_CONST :
    case tCHAR_CONST :
      g_constantToken = g_token;
      g_constantInt   = g_tknInt;
      getToken();
      break;

    case tREAL_CONST     :
      g_constantToken = g_token;
      g_constantReal  = g_tknReal;
      getToken();
      break;

    case tSTRING_CONST :
      g_constantToken = g_token;
      g_constantStart = g_tokenString;
      getToken();
      break;

      /* Highest Priority Operators */

    case tNOT:
      getToken();
      pas_ConstantFactor();
      if (g_constantToken != tINT_CONST &&
          g_constantToken != tBOOLEAN_CONST)
        {
          error(eFACTORTYPE);
        }

      g_constantInt = ~g_constantInt;
      break;

      /* Standard or Built-in function? */

    case tSTDFUNC:
      pas_StandardFunctionOfConstant();
      break;

    case tBUILTIN:
      (void)pas_BuiltInFunction();
      break;

      /* Hmmm... Try the standard functions */

    default :
      error(eINVFACTOR);
      break;
    }
}
