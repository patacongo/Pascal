/****************************************************************************
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

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
#include "pas_machine.h"
#include "pas_statement.h"
#include "pas_expression.h"
#include "pas_function.h"
#include "pas_token.h"
#include "pas_error.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define isRelationalOperator(t) \
   ((t) == tEQ || (t) == tNE || \
    (t) == tLT || (t) == tLE || \
    (t) == tGT || (t) == tGE)

#define isRelationalSetOperator(t) \
   ((t) == tEQ || (t) == tNE || \
    (t) == tLE || (t) == tIN)

#define isRelationalType(t) \
   ((t) == tINT_CONST || (t) == tCHAR_CONST || \
    (t) == tBOOLEAN_CONST || (t) == tREAL_CONST)

#define isAdditiveType(t) \
   ((t) == tINT_CONST || (t) == tREAL_CONST)

#define isMultiplicativeType(t) \
   ((t) == tINT_CONST || (t) == tREAL_CONST)

#define isLogicalType(t) \
   ((t) == tINT_CONST || (t) == tBOOLEAN_CONST)

#define isOrdinalType(t) \
   ((t) == tINT_CONST || (t) == tCHAR_CONST || \
    (t) == tBOOLEAN_CONST || (t) == sSCALAR_OBJECT)

#define pascalBoolean(c) \
   ((c) ? PASCAL_TRUE : PASCAL_FALSE)

#define copySet(src, dest) \
   do \
     { \
       (dest)[0] = (src)[0]; \
       (dest)[1] = (src)[1]; \
       (dest)[2] = (src)[2]; \
       (dest)[3] = (src)[3]; \
     } \
   while (0)

#define emptySet(dest) \
   do \
     { \
       (dest)[0] = 0; \
       (dest)[1] = 0; \
       (dest)[2] = 0; \
       (dest)[3] = 0; \
     } \
   while (0)

#define equalSets(s1, s2) \
   pascalBoolean((s1)[0] == (s2)[0] && \
                 (s1)[1] == (s2)[1] && \
                 (s1)[2] == (s2)[2] && \
                 (s1)[3] == (s2)[3])

#define unEqualSets(s1, s2) \
   pascalBoolean((s1)[0] != (s2)[0] || \
                 (s1)[1] != (s2)[1] || \
                 (s1)[2] != (s2)[2] || \
                 (s1)[3] != (s2)[3])

#define containsSet(s1, s2) \
   pascalBoolean(((s1)[0] & (s2)[0]) == (s2)[0] && \
                 ((s1)[1] & (s2)[1]) == (s2)[1] && \
                 ((s1)[2] & (s2)[2]) == (s2)[2] && \
                 ((s1)[3] & (s2)[3]) == (s2)[3])

#define setUnion(src, dest) \
   do \
     { \
       (dest)[0] |= (src)[0]; \
       (dest)[1] |= (src)[1]; \
       (dest)[2] |= (src)[2]; \
       (dest)[3] |= (src)[3]; \
     } \
   while (0)

#define setDifference(src, dest) \
   do \
     { \
       (dest)[0] &= ~(src)[0]; \
       (dest)[1] &= ~(src)[1]; \
       (dest)[2] &= ~(src)[2]; \
       (dest)[3] &= ~(src)[3]; \
     } \
   while (0)

#define setSymmetricDifference(src, dest) \
   do \
     { \
       (dest)[0] ^= (src)[0]; \
       (dest)[1] ^= (src)[1]; \
       (dest)[2] ^= (src)[2]; \
       (dest)[3] ^= (src)[3]; \
     } \
   while (0)

#define setIntersection(src, dest) \
   do \
     { \
       (dest)[0] &= (src)[0]; \
       (dest)[1] &= (src)[1]; \
       (dest)[2] &= (src)[2]; \
       (dest)[3] &= (src)[3]; \
     } \
   while (0)

/****************************************************************************
 * Private Function Prototypes
 ***************************************************************************/

static void pas_ConstantSimpleExpression(exprType_t findExprType,
                                         symbol_t *typePtr);
static void pas_ConstantTerm(exprType_t findExprType, symbol_t *typePtr);
static void pas_ConstantFactor(exprType_t findExprType, symbol_t *typePtr);

static void pas_GetConstantSubSet(uint16_t minElement, uint16_t maxElement);
static void pas_AddBitSetElements(uint16_t firstElement,
                                  uint16_t lastElement, uint16_t minElement,
                                  uint16_t maxElement);

/****************************************************************************
 * Public Data
 ****************************************************************************/

int      g_constantToken;
int32_t  g_constantInt;
double   g_constantReal;
char    *g_constantStart;
uint32_t g_constantStrOffset;
int      g_constantStrLen;
uint16_t g_constantSet[sSET_WORDS];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************/
/* Process Simple Expression */

static void pas_ConstantSimpleExpression(exprType_t findExprType,
                                         symbol_t *typePtr)
{
  int16_t  unary = ' ';
  int      term;
  int32_t  termInt;
  double   termReal;
  uint16_t termSet[sSET_WORDS];

  /* FORM: [+|-] <term> [{+|-} <term> [{+|-} <term> [...]]]
   * get +/- unary operation
   */

  if (g_token == '+' || g_token == '-')
    {
      unary = g_token;
      getToken();
    }

  /* Process first (non-optional) term and apply unary operation */

  pas_ConstantTerm(findExprType, typePtr);
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

  if (term == tSET_CONST)
    {
      copySet(g_constantSet, termSet);
    }

  /* If we got an integer but we were asked to find a REAL, then make that
   * conversion now.
   */

  if (term == tINT_CONST && findExprType == exprReal)
    {
      termReal = (double)termInt;
      term     = tREAL_CONST;
    }

  /* Process subsequent (optional) terms and binary operations */

  for (; ; )
    {
      int operator;

      /* Check for binary operator */

      if ((g_token == '+' || g_token == '-') && isAdditiveType(term))
        {
          operator = g_token;
        }
      else if (g_token == tOR && isLogicalType(term))
        {
          operator = g_token;
        }
      else if ((g_token == '+' || g_token == '-' || g_token == tSYMDIFF) &&
               term == tSET_CONST)
        {
          operator = g_token;
        }
      else
        {
          break;
        }

      /* Get the 2nd term */

      getToken();
      pas_ConstantTerm(findExprType, typePtr);

      /* Before generating the operation, verify that the types match.
       * Perform automatic type conversion from INTEGER to REAL as
       * necessary.
       */

      if (term != g_constantToken)
        {
          /* Handle the case where the 1st argument is REAL and the
           * second is INTEGER. */

          if (term == tREAL_CONST && g_constantToken == tINT_CONST)
            {
              g_constantReal  = (double)g_constantInt;
              g_constantToken = tREAL_CONST;
            }

          /* Handle the case where the 1st argument is Integer and the
           * second is REAL. */

          else if (term == tINT_CONST && g_constantToken == tREAL_CONST)
            {
              /* This could only happen if findExprType is not exprReal.
               * Expect an error down the road.
               */

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

        case tSET_CONST :
          if (operator == '+')
            {
              setUnion(termSet, g_constantSet);
            }
          else if (operator == '-')
            {
              setDifference(termSet, g_constantSet);
            }
          else /* if (operator == tSYMDIFF) */
            {
              setSymmetricDifference(termSet, g_constantSet);
            }
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

/****************************************************************************/
/* Evaluate a TERM */

void pas_ConstantTerm(exprType_t findExprType, symbol_t *typePtr)
{
  int      operator;
  int      factor;
  int32_t  factorInt;
  double   factorReal;
  uint16_t factorSet[sSET_WORDS];

  /* FORM:  <factor> [<operator> <factor>[<operator><factor>[...]]] */

  pas_ConstantFactor(findExprType, typePtr);
  factor     = g_constantToken;
  factorInt  = g_constantInt;
  factorReal = g_constantReal;

  for (; ; )
    {
      /* Check for binary operator */

      if ((g_token == tMUL || g_token == tMOD) &&
          isMultiplicativeType(factor))
        {
          operator = g_token;
        }
      else if ((g_token == tDIV || g_token == tSHL || g_token == tSHR) &&
               factor == tINT_CONST)
        {
          operator = g_token;
        }
      else if (g_token == tMUL && factor == tSET_CONST)
        {
          operator = g_token;
          copySet(g_constantSet, factorSet);
        }
      else if (g_token == tFDIV && factor == tREAL_CONST)
        {
          operator = g_token;
        }
#if 0
      else if (g_token == tFDIV && factor == tINT_CONST)
        {
          factorReal = (double)factorInt;
          factor     = tREAL_CONST;
          operator   = g_token;
        }
#endif
      else if (g_token == tAND && isLogicalType(factor))
        {
          operator = g_token;
        }
      else
        {
          g_constantToken = factor;
          g_constantInt   = factorInt;
          g_constantReal  = factorReal;
          break;
        }

      /* Get the next factor */

      getToken();
      pas_ConstantFactor(findExprType, typePtr);

      /* Before generating the operation, verify that the types match.
       * Perform automatic type conversion from INTEGER to REAL as
       * necessary.
       */

      if (factor != g_constantToken)
        {
          /* Handle the case where the 1st argument is REAL and the
           * second is INTEGER.
           */

          if (factor == tREAL_CONST && g_constantToken == tINT_CONST)
            {
              g_constantReal = (double)g_constantInt;
            }

          /* Handle the case where the 1st argument is Integer and the
           * second is REAL.
           */

          else if (factor == tINT_CONST && g_constantToken == tREAL_CONST)
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
            {
              factorInt *= g_constantInt;
            }
          else if (factor == tREAL_CONST)
            {
              factorReal *= g_constantReal;
            }
          else if (factor == tREAL_CONST)
            {
              setIntersection(factorSet, g_constantSet);
            }
          else
            {
              error(eFACTORTYPE);
            }
          break;

        case tDIV :
          if (factor == tINT_CONST)
            {
              factorInt /= g_constantInt;
            }
          else
            {
              error(eFACTORTYPE);
            }
          break;

        case tFDIV :
          if (factor == tREAL_CONST)
            {
              factorReal /= g_constantReal;
            }
          else
            {
             error(eFACTORTYPE);
            }
          break;

        case tMOD :
          if (factor == tINT_CONST)
            {
              factorInt %= g_constantInt;
            }
          else if (factor == tREAL_CONST)
            {
              factorReal = fmod(factorReal, g_constantReal);
            }
          else
            {
              error(eFACTORTYPE);
            }
          break;

        case tAND :
          if (factor == tINT_CONST || factor == tBOOLEAN_CONST)
            {
              factorInt &= g_constantInt;
            }
          else
            {
              error(eFACTORTYPE);
            }
          break;

        case tSHL :
          if (factor == tINT_CONST)
            {
              factorInt <<= g_constantInt;
            }
          else
            {
              error(eFACTORTYPE);
            }
          break;

        case tSHR :
          if (factor == tINT_CONST)
            {
              factorInt >>= g_constantInt;
            }
          else
            {
              error(eFACTORTYPE);
            }
          break;
        }
    }
}

/****************************************************************************/
/* Process a constant FACTOR */

static void pas_ConstantFactor(exprType_t findExprType, symbol_t *typePtr)
{
  /* Process by token type */

  switch (g_token)
    {
    case tCHAR_CONST :
      if (findExprType == exprString)
        {
          char temp[2];

          /* Create a 1 character constant string */

          temp[0] = (char)g_tknUInt;
          temp[1] = '\0';

          g_constantToken     = tSTRING_CONST;
          g_constantStrOffset = poffAddRoDataString(g_poffHandle, temp);
          g_constantStrLen    = 1;

          getToken();
          break;
        }

      /* Fall through to treat like integer (or REAL) */

    case tINT_CONST :
    case tBOOLEAN_CONST :
      if (findExprType == exprReal)
        {
          g_constantToken = tREAL_CONST;
          g_constantReal  = (double)g_tknUInt;
        }
      else
        {
          g_constantToken = g_token;
          g_constantInt   = g_tknUInt;
        }

      getToken();
      break;

    case tREAL_CONST     :
      g_constantToken = g_token;
      g_constantReal  = g_tknReal;
      getToken();
      break;

    case tSTRING_CONST :
      /* REVISIT:  No support for constant string expressions. */

      /* Add the string to the RO data section of the output and get the
       * offset to the string location.
       */

      g_constantToken     = tSTRING_CONST;
      g_constantStrOffset = poffAddRoDataString(g_poffHandle, g_tokenString);
      g_constantStrLen    = strlen(g_tokenString);
      getToken();
      break;

    case sSTRING_CONST :
      /* Named string constant.  String is already in RO data. */

      g_constantToken     = tSTRING_CONST;
      g_constantStrOffset = g_tknPtr->sParm.s.roOffset;
      g_constantStrLen    = g_tknPtr->sParm.s.roSize;
      getToken();
      break;

    case '[' :
      /* Set constant:
       *
       * FORM: '['[ set-subset [',' set-subset [',' ...]]]']'
       *       set-subset = set-element | set-subrange
       *       set-element = set-constant | set-ordinal-variable
       *       set-subrange = set-element '..' set-element
       */

      emptySet(g_constantSet);

      /* Check for the empty set */

      getToken();
      if (g_token == ']') getToken();
      else
        {
          uint16_t minElement = 0;
          uint16_t maxElement = sSET_MAXELEM - 1;

          /* We need to the the type in order to correctly set the min/max
           * element values.
           */

          if (typePtr != NULL)
            {
              minElement = typePtr->sParm.t.tMinValue;
              maxElement = typePtr->sParm.t.tMaxValue;
            }

          /* Get the first element of the set */

          pas_GetConstantSubSet(minElement, maxElement);

          /* Incorporate each additional element into the set */

          while (g_token == ',')
            {
              /* Get the next element of the set */

              getToken();
              pas_GetConstantSubSet(minElement, maxElement);
            }

          /* Verify the closing brace */

          if (g_token != ']') error(eRBRACKET);
          else getToken();
        } 

      g_constantToken = tSET_CONST;
      break;

      /* Highest Priority Operators */

    case tNOT:
      getToken();
      pas_ConstantFactor(findExprType, typePtr);
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

/****************************************************************************/

static void pas_GetConstantSubSet(uint16_t minElement, uint16_t maxElement)
{
  /* Process a subset:
   *
   * FORM: set-subset = set-element | set-subrange
   *       set-element = set-constant | set-ordinal-variable
   *       set-subrange = set-element '..' set-element
   *
   * On entry, g_token should refer to a set-element.
   */

  /* REVISIT:  This is missing all type and range checks.  It could easily
   * generate an invalid set for the type needed.
   *
   * REVISIT:  This won't work:  We need to have the lower limit of the
   * subrange or scalar in order to establish bit 0 of the bitset.
   */

  if (!isOrdinalType(g_token))
    {
      error(eSETELEMENT);
    }
  else
    {
      uint16_t setType;
      uint16_t firstElement;

      setType      = g_token;
      firstElement = g_tknUInt;

      /* Check if this is a single element, or a sub-range of set elements */

      getToken();
      if (g_token == tSUBRANGE)
        {
          /* The upper value of the sub-range should be the same kind
           * of ordinal type as was the lower value (very weak check).
           */

          getToken();
          if (g_token != setType)
            {
              error(eSUBRANGETYPE);
            }
          else
            {
              /* Add a range of elements to the set */

              pas_AddBitSetElements(firstElement, g_tknUInt, minElement,
                                    maxElement);
              getToken();
            }
        }
      else
        {
          /* Add a single element to the set */

          uint16_t bitNumber = firstElement - minElement;
          int wordIndex      = bitNumber >> 4;
          int bitIndex       = bitNumber & 0x0f;

          g_constantSet[wordIndex] |= (1 << bitIndex);
        }
    }
}

/****************************************************************************/
 
static void pas_AddBitSetElements(uint16_t firstElement,
                                  uint16_t lastElement, uint16_t minElement,
                                  uint16_t maxElement)
{
  uint16_t firstBitNo;
  uint16_t lastBitNo;
  uint16_t leadMask;
  uint16_t tailMask;
  int      wordIndex;

  /* Set all bits from firstElement through lastElement. */

  firstBitNo = firstElement - minElement;
  lastBitNo  = lastElement  - minElement;
  leadMask   = (0xffff << (firstBitNo & 0x0f));
  tailMask   = (0xffff >> ((BITS_IN_INTEGER - 1) - (lastBitNo & 0x0f)));

  /* Special case:  The entire sub-range fits in one word */

  wordIndex = firstBitNo >> 4;
  if (wordIndex == (lastBitNo >> 4))
    {
      g_constantSet[wordIndex] = (leadMask & tailMask);
    }

  /* No, the last bit lies in a different word than the first */

  else
    {
      uint16_t bitMask;
      int      nBits;
      int      leadBits;
      int      tailBits;
      int      bitsInWord;

      nBits    = lastElement  - firstElement + 1;
      leadBits = BITS_IN_INTEGER - firstBitNo;
      tailBits = (lastBitNo & 0x0f) + 1;

      /* Loop for each word */

      for (bitMask = leadMask, bitsInWord = leadBits;
           nBits > 0;
           wordIndex++)
        {
          g_constantSet[wordIndex] = bitMask;
          nBits                   -= bitsInWord;

          if (nBits >= BITS_IN_INTEGER)
            {
              bitsInWord = BITS_IN_INTEGER;
              bitMask    = 0xffff;
            }
          else if (nBits > 0)
            {
              nBits   = tailBits;
              bitMask = tailMask;
            }
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************/
/* Evaluate a simple expression of constant values */

void pas_ConstantExpression(exprType_t findExprType, symbol_t *typePtr)
{
  /* Get the value of a simple constant expression */

  pas_ConstantSimpleExpression(findExprType, typePtr);

  /* Is it followed by a relational operator? */

  if ((isRelationalOperator(g_token) && isRelationalType(g_constantToken)) ||
      (isRelationalSetOperator(g_token) && g_constantToken == tSET_CONST))
    {
      uint16_t simple1Set[sSET_WORDS];

      int simple1        = g_constantToken;
      int32_t simple1Int = g_constantInt;
      double simple1Real = g_constantReal;
      int operator       = g_token;

      copySet(g_constantSet, simple1Set);

      /* If this is an integer constant and we were asked to get a real
       * constant, then convert the integer to real to force subsequent
       * conversions.
       */

      if (simple1 == tINT_CONST && findExprType == exprReal)
        {
          simple1Real    = (double)simple1Real;
          simple1        = tREAL_CONST;
        }

      /* Get the second simple expression */

      pas_ConstantSimpleExpression(findExprType, typePtr);
      if (simple1 != g_constantToken)
        {
          /* Handle the case where the 1st argument is REAL and the
           * second is INTEGER.
           */

          if (simple1 == tREAL_CONST && g_constantToken == tINT_CONST)
            {
              g_constantReal  = (double)g_constantInt;
              g_constantToken = tREAL_CONST;
            }

          /* Handle the case where the 1st argument is Integer and the
           * second is REAL. */

          else if (simple1 == tINT_CONST && g_constantToken == tREAL_CONST)
            {
              /* This could only hapen if findExprType is not exprReal.
               * Expect an error down the road.
               */

              simple1Real = (double)simple1Int;
              simple1     = tREAL_CONST;
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
              g_constantInt = pascalBoolean(simple1Int == g_constantInt);
              break;

            case tNE :
              g_constantInt = pascalBoolean(simple1Int != g_constantInt);
              break;

            case tLT :
              g_constantInt = pascalBoolean(simple1Int < g_constantInt);
              break;

            case tLE :
              g_constantInt = pascalBoolean(simple1Int <= g_constantInt);
              break;

            case tGT :
              g_constantInt = pascalBoolean(simple1Int > g_constantInt);
              break;

            case tGE :
              g_constantInt = pascalBoolean(simple1Int >= g_constantInt);
              break;

            case tIN :  /* Not yet */
            default  :
              error(eEXPRTYPE);
              break;
            }
          break;

        case tREAL_CONST:
          switch (operator)
            {
            case tEQ :
              g_constantInt = pascalBoolean(simple1Real == g_constantReal);
              break;

            case tNE :
              g_constantInt = pascalBoolean(simple1Real != g_constantReal);
              break;

            case tLT :
              g_constantInt = pascalBoolean(simple1Real < g_constantReal);
              break;

            case tLE :
              g_constantInt = pascalBoolean(simple1Real <= g_constantReal);
              break;

            case tGT :
              g_constantInt = pascalBoolean(simple1Real > g_constantReal);
              break;

            case tGE :
              g_constantInt = pascalBoolean(simple1Real >= g_constantReal);
              break;

            case tIN :  /* Not yet */
            default :
              error(eEXPRTYPE);
              break;
            }
          break;

        case tSET_CONST:
          switch (operator)
            {
            case tEQ : /* Check equality of two sets */
              g_constantInt = equalSets(simple1Set, g_constantSet);
              break;

            case tNE : /* Check inequality of two sets */
              g_constantInt = unEqualSets(simple1Set, g_constantSet);
              break;

            case tLE : /* Check if simple1Set contains simple2Set */
              g_constantInt = containsSet(simple1Set, g_constantSet);
              break;

            case tIN : /* Check for member of a simple1Set */
              error(eNOTYET);
              break;

            default:
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
