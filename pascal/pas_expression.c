/****************************************************************************
 * pas_expression.c
 * Integer Expression
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
#include "pas_pcode.h"      /* general operation codes */
#include "pas_fpops.h"      /* floating point operations */
#include "pas_library.h"    /* library operations */
#include "pas_errcodes.h"

#include "pas_main.h"
#include "pas_statement.h"
#include "pas_expression.h"
#include "pas_procedure.h"  /* for pas_ActualParameterList() */
#include "pas_function.h"
#include "pas_codegen.h"    /* for pas_Generate*() */
#include "pas_token.h"
#include "pas_insn.h"
#include "pas_error.h"

/****************************************************************************
 * Private Type Declarations
 ****************************************************************************/

/* This structure holds a writable copy of a symbol table variable plus
 * additional information to help with evaluation of expressions.
 */

struct varInfo_s
{
  symbol_t  variable;   /* Writable copy of symbol table variable entry */
  int16_t   fOffset;    /* Record field offset into variable */
};

typedef struct varInfo_s varInfo_t;

/* This structure is used for managing SET variables */

struct setType_s
{
  uint8_t    setType;
  bool       typeFound;
  int16_t    minValue;
  int16_t    maxValue;
  symbol_t  *typePtr;
};

typedef struct setType_s setType_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static exprType_t pas_SimplifyExpression(exprType_t findExprType);
static exprType_t pas_Term(exprType_t findExprType);
static exprType_t pas_Factor(exprType_t findExprType);
static exprType_t pas_ComplexFactor(void);
static exprType_t pas_SimplifyFactor(varInfo_t *varInfo,
                    uint8_t factorFlags);
static exprType_t pas_BaseFactor(symbol_t *varPtr, uint8_t factorFlags);
static exprType_t pas_PointerFactor(void);
static exprType_t pas_ComplexPointerFactor(void);
static exprType_t pas_SimplifyPointerFactor(varInfo_t *varInfo,
                    uint8_t factorFlags);
static exprType_t pas_BasePointerFactor(symbol_t *varPtr,
                    uint8_t factorFlags);
static exprType_t pas_FunctionDesignator(void);
static void       pas_SetAbstractType(symbol_t *sType);
static void       pas_GetSetFactor(void);
static void       pas_GetSetElement(setType_t *s);
static bool       pas_IsOrdinalType(exprType_t testExprType);
static bool       pas_IsAnyStringType(exprType_t testExprType);
static bool       pas_IsStringReference(exprType_t testExprType);

/****************************************************************************
 * Private Variables
 ****************************************************************************/

/* The abstract types - SETs, RECORDS, etc - require an exact
 * match in type.  This variable points to the symbol table
 * sTYPE entry associated with the expression.
 */

static symbol_t *g_abstractType;

/****************************************************************************/
/* Evaluate (boolean) Expression */

exprType_t pas_Expression(exprType_t findExprType, symbol_t *typePtr)
{
  uint8_t    operation;
  uint16_t   intOpCode;
  uint16_t   fpOpCode;
  uint16_t   strOpCode;
  exprType_t simple1Type;
  exprType_t simple2Type;

  TRACE(g_lstFile,"[expression]");

  /* The abstract types - SETs, RECORDS, etc - require an exact
   * match in type.  Save the symbol table sTYPE entry associated
   * with the expression.
   */

  if (typePtr != NULL && typePtr->sKind != sTYPE) error(eINVTYPE);
  g_abstractType = typePtr;

  /* FORM <simple expression> [<relational operator> <simple expression>]
   *
   * Get the first <simple expression>
   */

  simple1Type = pas_SimplifyExpression(findExprType);

  /* Get the optional <relational operator> which may follow */

  operation = g_token;
  switch (operation)
    {
    case tEQ :
      intOpCode = opEQU;
      fpOpCode  = fpEQU;
      strOpCode = opEQUZ;
      break;

    case tNE :
      intOpCode = opNEQ;
      fpOpCode  = fpNEQ;
      strOpCode = opNEQZ;
      break;

    case tLT :
      intOpCode = opLT;
      fpOpCode  = fpLT;
      strOpCode = opLTZ;
      break;

    case tLE :
      intOpCode = opLTE;
      fpOpCode  = fpLTE;
      strOpCode = opLTEZ;
      break;

    case tGT :
      intOpCode = opGT;
      fpOpCode  = fpGT;
      strOpCode = opGTZ;
      break;

    case tGE :
      intOpCode = opGTE;
      fpOpCode  = fpGTE;
      strOpCode = opGTEZ;
      break;

    case tIN :
      if (!g_abstractType ||
          (g_abstractType->sParm.t.tType != sSCALAR &&
           g_abstractType->sParm.t.tType != sSUBRANGE))
        {
          error(eEXPRTYPE);
        }
      else if (g_abstractType->sParm.t.tMinValue)
        {
          pas_GenerateDataOperation(opPUSH,
                                    g_abstractType->sParm.t.tMinValue);
          pas_GenerateSimple(opSUB);
        }

      intOpCode = opBIT;
      fpOpCode  = fpINVLD;
      strOpCode = opNOP;
      break;

    default  :
      intOpCode = opNOP;
      fpOpCode  = fpINVLD;
      strOpCode = opNOP;
      break;
    }

  /* Check if there is a 2nd simple expression needed */

  if (intOpCode != opNOP)
    {
      /* Get the second simple expression */

      getToken();
      simple2Type = pas_SimplifyExpression(findExprType);

      /* Perform automatic type conversion from INTEGER to REAL
       * for integer vs. real comparisons.
       */

      if (simple1Type != simple2Type)
        {
          /* Handle the case where the 1st argument is REAL and the
           * second is INTEGER.
           */

          if (simple1Type == exprReal &&
              simple2Type == exprInteger &&
              fpOpCode != fpINVLD)
            {
              fpOpCode   |= fpARG2;
              simple2Type = exprReal;
            }

          /* Handle the case where the 1st argument is Integer and the
           * second is REAL.
           */

          else if (simple1Type == exprInteger &&
                   simple2Type == exprReal &&
                   fpOpCode != fpINVLD)
            {
              fpOpCode   |= fpARG1;
              simple1Type = exprReal;
            }

          /* Allow the case of <scalar type> IN <set type> */
          /* Otherwise, the two terms must agree in type */

          else if (operation != tIN || simple2Type != exprSet)
            {
              error(eEXPRTYPE);
            }
        }

      /* Generate the comparison */

      if (simple1Type == exprReal)
        {
          if (fpOpCode == fpINVLD)
            {
              error(eEXPRTYPE);
            }
          else
            {
              pas_GenerateFpOperation(fpOpCode);
            }
        }
      else if (simple1Type == exprString || simple1Type == exprString)
        {
          if (strOpCode != opNOP)
            {
              pas_StandardFunctionCall(lbSTRCMP);
              pas_GenerateSimple(strOpCode);
            }
          else
            {
              error(eEXPRTYPE);
            }
        }
      else
        {
          pas_GenerateSimple(intOpCode);
        }

      /* The type resulting from these operations becomes BOOLEAN */

      simple1Type = exprBoolean;
    }

  /* Verify that the expression is of the requested type.
   * The following are okay:
   *
   * 1. We were told to find any kind of expression
   *
   * 2. We were told to find a specific kind of expression and
   *    we found just that type.
   *
   * 3. We were told to find any kind of ordinal expression and
   *    we found a ordinal expression.  This is what is needed, for
   *    example, as an argument to ord(), pred(), succ(), or odd().
   *    This is the kind of expression we need in a CASE statement
   *    as well.
   *
   * 4. We were told to find any kind of string expression and
   *    we found a string expression. This is a hack to handle
   *    calls to system functions that return exprCString pointers
   *    that must be converted to exprString records upon assignment.
   *
   * Special case:
   *
   *    We will perform automatic conversions to real from integer
   *    if the requested type is a real expression.
   */

  if (findExprType != exprUnknown &&         /* 1)NOT Any expression */
      findExprType != simple1Type &&         /* 2)NOT Matched expression */
      (findExprType != exprAnyOrdinal ||     /* 3)NOT any ordinal type */
       !pas_IsOrdinalType(simple1Type)) &&   /*   OR type is not ordinal */
      (findExprType != exprAnyString ||      /* 4)NOT any string type */
       !pas_IsAnyStringType(simple1Type)) && /*   OR type is not string */
      (findExprType != exprString ||         /* 5)Not looking for string ref */
       !pas_IsStringReference(simple1Type))) /*   OR type is not string ref */
    {
      /* Automatic conversions from INTEGER to REAL will be performed */

      if ((findExprType == exprReal) && (simple1Type == exprInteger))
        {
          pas_GenerateFpOperation(fpFLOAT);
          simple1Type = exprReal;
        }

      /* Any other type mismatch is an error */

      else
        {
          error(eEXPRTYPE);
        }
    }

  return simple1Type;
}

/****************************************************************************/
/* Provide VAR parameter assignments */

exprType_t pas_VarParameter(exprType_t varExprType, symbol_t *typePtr)
{
  exprType_t factorType;

  /* The abstract types - SETs, RECORDS, etc - require an exact
   * match in type.  Save the symbol table sTYPE entry associated
   * with the expression.
   */

  if (typePtr != NULL && typePtr->sKind != sTYPE) error(eINVTYPE);
  g_abstractType = typePtr;

  /* This function is really just an interface to the
   * static function pas_PointerFactor with some extra error
   * checking.
   */

  factorType = pas_PointerFactor();
  if (varExprType != exprUnknown && factorType != varExprType)
    {
      error(eINVVARPARM);
    }

  return factorType;
}

/****************************************************************************/
/* Process Array Index */

void pas_ArrayIndex(symbol_t *indexTypePtr, uint16_t elemSize)
{
  uint16_t offset;

  TRACE(g_lstFile,"[pas_ArrayIndex]");

  /* FORM:  [<integer expression>].
   * On entry 'g_token' should refer to the ']' token.
   */

  if (g_token != '[') error(eLBRACKET);
  else
    {
      uint16_t indexType;
      exprType_t exprType;

      /* Get the type of the index */

      if (indexTypePtr->sKind != sTYPE)
        {
          error(eINDEXTYPE);
          exprType = exprUnknown;
        }
      else
        {
          indexType = indexTypePtr->sParm.t.tType;

          /* REVISIT:  For subranges, we use the base type of the subrange. */

          if (indexType == sSUBRANGE)
            {
              indexType = indexTypePtr->sParm.t.tSubType;
            }

          /* Get the expression type from the index type */

          exprType = pas_MapVariable2ExprType(indexType, true);
        }

      /* Evaluate index expression */

      getToken();
      pas_Expression(exprType, NULL);

      /* We now have the array element at the top of the step.  If the index
       * is not zero-based, the we need to offset the index value so that it
       * is.
       */

      offset = indexTypePtr->sParm.t.tMinValue;
      if (offset != 0)
        {
          pas_GenerateDataOperation(opPUSH, offset);
          pas_GenerateSimple(opSUB);
        }

      /* The index is in units of the base type of the elements of array.  If
       * that element size if not one, then we need to multiply the zero-
       * based index by the element size.
       */

      if (elemSize != 1)
        {
          pas_GenerateDataOperation(opPUSH, elemSize);
          pas_GenerateSimple(opMUL);
        }

      /* Verify right bracket */

      if (g_token !=  ']') error(eRBRACKET);
      else getToken();
    }
}

/*************************************************************************/
/* Determine the expression type associated with a pointer to a type
 * symbol
 */

exprType_t pas_GetExpressionType(symbol_t *sType)
{
  exprType_t factorType = sINT;

  TRACE(g_lstFile,"[getExprType]");

  if (sType != NULL && sType->sKind == sTYPE)
    {
      switch (sType->sParm.t.tType)
        {
        case sINT :
          factorType = exprInteger;
          break;

        case sBOOLEAN :
          factorType = exprBoolean;
          break;

        case sCHAR :
          factorType = exprChar;
          break;

        case sREAL :
          factorType = exprReal;
          break;

        case sSCALAR :
          factorType = exprScalar;
          break;

        case sSTRING :
          factorType = exprString;
          break;

        case sSUBRANGE :
          switch (sType->sParm.t.tSubType)
            {
            case sINT :
              factorType = exprInteger;
              break;

            case sCHAR :
              factorType = exprChar;
              break;

            case sSCALAR :
              factorType = exprScalar;
              break;

            default :
              error(eSUBRANGETYPE);
              break;
            }
          break;

        case sPOINTER :
          sType = sType->sParm.t.tParent;
          if (sType)
            {
              switch (sType->sKind)
                {
                case sINT :
                  factorType = exprIntegerPtr;
                  break;

                case sBOOLEAN :
                  factorType = exprBooleanPtr;
                  break;

                case sCHAR :
                  factorType = exprCharPtr;
                  break;

                case sREAL :
                  factorType = exprRealPtr;
                  break;

                case sSCALAR :
                  factorType = exprScalarPtr;
                  break;

                default :
                  error(eINVTYPE);
                  break;
                }
            }
          break;

        default :
          error(eINVTYPE);
          break;
        }
    }

  return factorType;
}

/*************************************************************************/

exprType_t pas_MapVariable2ExprType(uint16_t varType, bool ordinal)
{
  switch (varType)
    {
      /* Ordinal type mappings */

      case sINT :
        return exprInteger;           /* integer value */

      case sCHAR :
        return exprChar;              /* character value */

      case sBOOLEAN :
        return exprBoolean;           /* boolean(integer) value */

      case sSCALAR :
      case sSCALAR_OBJECT :
        return exprScalar;            /* scalar(integer) value */

      case sSET_OF :
        return exprSet;               /* set(integer) value */

      case sTYPE :                    /* Variable is defined type */
        return exprUnknown;           /* REVISIT */

      default:
        if (!ordinal)
          {
            switch (varType)
              {
                case sREAL :
                  return exprReal;    /* real value */

                case sSTRING :
                case sSTRING_CONST :
                  return exprString;  /* variable length string reference */

                case sFILE :
                case sTEXTFILE :
                  return exprFile;    /* File number */

                case sRECORD :
                case sRECORD_OBJECT :
                  return exprRecord;  /* record */

                case sARRAY :         /* REVISIT: array of something */
                case sPOINTER :       /* REVISIT: pointer to something */
                default:
                  error(eEXPRTYPE);
                  return exprUnknown;
              }
          }
        else
          {
            error(eEXPRTYPE);
            return exprUnknown;
          }
    }
}

/****************************************************************************/

exprType_t pas_MapVariable2ExprPtrType(uint16_t varType, bool ordinal)
{
  exprType_t exprType;

  exprType = pas_MapVariable2ExprType(varType, ordinal);
  if (exprType != exprUnknown)
    {
      exprType = (exprType_t)((unsigned int)exprType | EXPRTYPE_POINTER);
    }

  return exprType;
}

/****************************************************************************/
/* Process Simple Expression */

static exprType_t pas_SimplifyExpression(exprType_t findExprType)
{
  int16_t    operation = '+';
  uint16_t   arg8FpBits;
  exprType_t term1Type;
  exprType_t term2Type;

  TRACE(g_lstFile,"[pas_SimplifyExpression]");

  /* FORM: [+|-] <term> [{+|-} <term> [{+|-} <term> [...]]]
   *
   * get +/- unary operation
   */

  if (g_token == '+' || g_token == '-')
    {
      operation = g_token;
      getToken();
    }

  /* Process first (non-optional) term and apply unary operation */

  term1Type = pas_Term(findExprType);
  if (operation == '-')
    {
      if (term1Type == exprInteger)
        {
          pas_GenerateSimple(opNEG);
        }
      else if (term1Type == exprReal)
        {
          pas_GenerateFpOperation(fpNEG);
        }
      else
        {
          error(eTERMTYPE);
        }
    }

  /* Process subsequent (optional) terms and binary operations */

  for (; ; )
    {
      /* Check for binary operator */

      if (g_token == '+' || g_token == '-' || g_token == tOR)
        {
          operation = g_token;
        }
      else
        {
          break;
        }

      /* Special case for string types.  So far, we have parsed
       * '<string> +'  At this point, it is safe to assume we
       * going to modify a string.  So, if the string has not
       * been copied to the string stack, we will have to do that
       * now.
       */

      if (term1Type == exprString && operation == '+')
        {
          /* Duplicate the string on the string stack. */

          pas_StandardFunctionCall(lbSTRDUP);
        }

      /* If we are going to add something to a char, then the result must be
       * a string.  We will similarly have to convert the character to a
       * string.
       */

      else if (term1Type == exprChar && operation == '+')
        {
          /* Expand the character to a string on the string stack.  And
           * change the expression type to reflect this.
           */

          pas_StandardFunctionCall(lbMKSTKC);
          term1Type = exprString;
        }

      /* Get the 2nd term */

      getToken();
      term2Type = pas_Term(findExprType);

      /* Before generating the operation, verify that the types match.
       * Perform automatic type conversion from INTEGER to REAL as
       * necessary.
       */

      arg8FpBits = 0;

      /* Skip over string types.  These will be handled below */

      if (!pas_IsStringReference(term1Type))
        {
          /* Handle the case where the type of the terms differ. */

          if (term1Type != term2Type)
            {
              /* Handle the case where the 1st argument is REAL and the
               * second is INTEGER. */

              if (term1Type == exprReal && term2Type == exprInteger)
                {
                  arg8FpBits = fpARG2;
                  term2Type  = exprReal;
                }

              /* Handle the case where the 1st argument is Integer and the
               * second is REAL. */

              else if (term1Type == exprInteger && term2Type == exprReal)
                {
                  arg8FpBits = fpARG1;
                  term1Type = exprReal;
                }

               /* Otherwise, the two terms must agree in type */

              else
                {
                  error(eTERMTYPE);
                }
            }

          /* We do not perform conversions for the cases where the two
           * terms agree in type. There is only one interesting case:
           * When the expected expression is real and both arguments are
           * integer.  Since addition an subtraction are exact, it would,
           * in general, be more efficient to perform the conversion
           * AFTER the operation (at the the risk of possible overflow
           * conditions due to the limited range of integers).
           */
        }

      /* Generate code to perform the selected binary operation */

      switch (operation)
        {
        case '+' :
          switch (term1Type)
            {
              /* Integer addition */

             case exprInteger :
              pas_GenerateSimple(opADD);
              break;

              /* Floating point addition */

            case exprReal :
              pas_GenerateFpOperation(fpADD | arg8FpBits);
              break;

              /* Set 'addition' */

            case exprSet :
              pas_GenerateSimple(opOR);
              break;

              /* Handle the special cases where '+' indicates that we are
               * concatenating a string or a character to the end of a
               * string.  Note that these operations can only be performed
               * on stack copies of the strings.  Logic above should have
               * made the conversion for the case of exprString.
               */

            case exprString :
              if (term2Type == exprString)
                {
                  /* We are concatenating one string with another.*/

                  pas_StandardFunctionCall(lbSTRCAT);
                }
              else if (term2Type == exprChar)
                {
                  /* We are concatenating a character to the end of a string */

                  pas_StandardFunctionCall(lbSTRCATC);
                }
              else
                {
                  error(eTERMTYPE);
                }
              break;

              /* Otherwise, the '+' operation is not permitted */

            default :
              error(eTERMTYPE);
              break;
            }
          break;

        case '-' :
          /* Integer subtraction */

          if (term1Type == exprInteger)
            {
              pas_GenerateSimple(opSUB);
            }

           /* Floating point subtraction */

          else if (term1Type == exprReal)
            {
              pas_GenerateFpOperation(fpSUB | arg8FpBits);
            }

          /* Set 'subtraction' */

          else if (term1Type == exprSet)
            {
              pas_GenerateSimple(opNOT);
              pas_GenerateSimple(opAND);
            }

          /* Otherwise, the '-' operation is not permitted */

          else
            error(eTERMTYPE);
          break;

        case tOR :
          /* Integer/boolean 'OR' */

          if (term1Type == exprInteger || term1Type == exprBoolean)
            pas_GenerateSimple(opOR);

          /* Otherwise, the 'OR' operation is not permitted */

          else
            error(eTERMTYPE);
          break;
        }
    }

  return term1Type;
}

/****************************************************************************/
/* Evaluate a TERM */

static exprType_t pas_Term(exprType_t findExprType)
{
  uint8_t    operation;
  uint16_t   arg8FpBits;
  exprType_t factor1Type;
  exprType_t factor2Type;

  TRACE(g_lstFile,"[pas_Term]");

  /* FORM:  <factor> [<operator> <factor>[<operator><factor>[...]]] */

  factor1Type = pas_Factor(findExprType);
  for (; ; )
    {
      /* Check for binary operator */

      if ((g_token == tMUL)  || (g_token == tDIV)  ||
          (g_token == tFDIV) || (g_token == tMOD)  ||
          (g_token == tAND)  || (g_token == tSHL)  ||
          (g_token == tSHR))
        {
          operation = g_token;
        }
      else
        {
          break;
        }

      /* Get the next factor */

      getToken();
      factor2Type = pas_Factor(findExprType);

      /* Before generating the operation, verify that the types match.
       * Perform automatic type conversion from INTEGER to REAL as
       * necessary.
       */

      arg8FpBits = 0;

      /* Handle the case where the type of the terms differ. */

      if (factor1Type != factor2Type)
        {
          /* Handle the case where the 1st argument is REAL and the
           * second is INTEGER.
           */

          if (factor1Type == exprReal && factor2Type == exprInteger)
            {
              arg8FpBits = fpARG2;
            }

          /* Handle the case where the 1st argument is Integer and the
           * second is REAL.
           */

          else if (factor1Type == exprInteger && factor2Type == exprReal)
            {
              arg8FpBits = fpARG1;
              factor1Type = exprReal;
            }

          /* Otherwise, the two factors must agree in type */

          else
            {
              error(eFACTORTYPE);
            }
        }

    /* Handle the cases for conversions when the two string
     * type are the same type.
     */

      else
        {
          /* There is only one interesting case:  When the
           * expected expression is real and both arguments are
           * integer.  In this case, for example, 1/2 must yield
           * 0.5, not 0.
           */

          if ((factor1Type == exprInteger) && (findExprType == exprReal))
            {
              /* However, we will perform this conversin only for the
               * arithmetic operations: tMUL, tDIV/tFDIV, and tMOD.
               * The logical operations must be performed on integer
               * types with the result converted to a real type afterward.
               */

              if ((operation == tMUL)  || (operation == tDIV)  ||
                  (operation == tFDIV) || (operation == tMOD))
                {
                  /* Perform the conversion of both terms */

                  arg8FpBits = fpARG1 | fpARG2;
                  factor1Type = exprReal;

                  /* We will also have to switch the operation in
                   * the case of tDIV:  We'll have to used tFDIV.
                   */

                  if (operation == tDIV)
                    {
                      operation = tFDIV;
                    }
                }
            }
        }

      /* Generate code to perform the selected binary operation */

      switch (operation)
        {
        case tMUL :
          if (factor1Type == exprInteger)
            {
              pas_GenerateSimple(opMUL);
            }
          else if (factor1Type == exprReal)
            {
              pas_GenerateFpOperation(fpMUL | arg8FpBits);
            }
          else if (factor1Type == exprSet)
            {
              pas_GenerateSimple(opAND);
            }
          else
            {
              error(eFACTORTYPE);
            }
          break;

        case tDIV :
          if (factor1Type == exprInteger)
            {
              pas_GenerateSimple(opDIV);
            }
          else
            {
              error(eFACTORTYPE);
            }
          break;

        case tFDIV :
          if (factor1Type == exprReal)
            {
              pas_GenerateFpOperation(fpDIV | arg8FpBits);
            }
          else
            {
              error(eFACTORTYPE);
            }
          break;

        case tMOD :
          if (factor1Type == exprInteger)
            {
              pas_GenerateSimple(opMOD);
            }
          else if (factor1Type == exprReal)
            {
              pas_GenerateFpOperation(fpMOD | arg8FpBits);
            }
          else
            {
              error(eFACTORTYPE);
            }
          break;

        case tAND :
          if (factor1Type == exprInteger || factor1Type == exprBoolean)
            {
              pas_GenerateSimple(opAND);
            }
          else
            {
              error(eFACTORTYPE);
            }
          break;

        case tSHL :
          if (factor1Type == exprInteger)
            {
              pas_GenerateSimple(opSLL);
            }
          else
            {
              error(eFACTORTYPE);
            }
          break;

        case tSHR :
          if (factor1Type == exprInteger)
            {
              pas_GenerateSimple(opSRA);
            }
          else
            {
              error(eFACTORTYPE);
            }
          break;
        }
    }

  return factor1Type;
}

/****************************************************************************/
/* Process a FACTOR */

static exprType_t pas_Factor(exprType_t findExprType)
{
  exprType_t factorType = exprUnknown;

  TRACE(g_lstFile,"[pas_Factor]");

  /* Process by token type */

  switch (g_token)
    {
      /* User defined tokens */

    case tIDENT :
      error(eUNDEFSYM);
      g_stringSP = g_tokenString;
      factorType = exprUnknown;
      break;

      /* Constant factors */

    case tINT_CONST :
      pas_GenerateDataOperation(opPUSH, g_tknInt);
      getToken();
      factorType = exprInteger;
      break;

    case tBOOLEAN_CONST :
      pas_GenerateDataOperation(opPUSH, g_tknInt);
      getToken();
      factorType = exprBoolean;
      break;

    case tCHAR_CONST :
      pas_GenerateDataOperation(opPUSH, g_tknInt);
      getToken();
      factorType = exprChar;
      break;

    case tREAL_CONST     :
      pas_GenerateDataOperation(opPUSH, (int32_t)*(((uint16_t*)&g_tknReal) + 0));
      pas_GenerateDataOperation(opPUSH, (int32_t)*(((uint16_t*)&g_tknReal) + 1));
      pas_GenerateDataOperation(opPUSH, (int32_t)*(((uint16_t*)&g_tknReal) + 2));
      pas_GenerateDataOperation(opPUSH, (int32_t)*(((uint16_t*)&g_tknReal) + 3));
      getToken();
      factorType = exprReal;
      break;

    case sSCALAR_OBJECT :
      if (g_abstractType)
        {
          if (g_tknPtr->sParm.c.parent != g_abstractType) error(eSCALARTYPE);
        }
      else
        {
         g_abstractType = g_tknPtr->sParm.c.parent;
        }

      pas_GenerateDataOperation(opPUSH, g_tknPtr->sParm.c.val.i);
      getToken();
      factorType = exprScalar;
      break;

      /* Simple Factors */

    case sINT :
      pas_GenerateStackReference(opLDS, g_tknPtr);
      getToken();
      factorType = exprInteger;
      break;

    case sBOOLEAN :
      pas_GenerateStackReference(opLDS, g_tknPtr);
      getToken();
      factorType = exprBoolean;
      break;

    case sCHAR :
      pas_GenerateStackReference(opLDSB, g_tknPtr);
      getToken();
      factorType = exprChar;
      break;

    case sREAL :
      pas_GenerateDataSize(sREAL_SIZE);
      pas_GenerateStackReference(opLDSM, g_tknPtr);
      getToken();
      factorType = exprReal;
      break;

      /* Strings -- constant and variable */

    case tSTRING_CONST :
      {
        /* Final stack representation is:
         *
         *   TOS(0) : pointer to string
         *   TOS(1) : size in bytes
         *
         * Add the string to the RO data section of the output
         * and get the offset to the string location.
         */

        uint32_t offset = poffAddRoDataString(poffHandle, g_tokenString);

        /* Get the offset then size of the string on the stack */

        pas_GenerateDataOperation(opPUSH, strlen(g_tokenString));
        pas_GenerateDataOperation(opLAC, offset);

        /* Release the tokenized string */

        g_stringSP = g_tokenString;
        getToken();
        factorType = exprString;
      }
      break;

    case sSTRING_CONST :
      /* Final stack representation is:
       *
       *   TOS(0) : pointer to string
       *   TOS(1) : size in bytes
       */

      pas_GenerateDataOperation(opPUSH, g_tknPtr->sParm.s.roSize);
      pas_GenerateDataOperation(opLAC, g_tknPtr->sParm.s.roOffset);
      getToken();
      factorType = exprString;
      break;

    case sSTRING :
      /* Stack representation is:
       *
       *   TOS(0) = pointer to string data
       *   TOS(1) = size in bytes
       */

      pas_GenerateDataSize(sSTRING_SIZE);
      pas_GenerateStackReference(opLDSM, g_tknPtr);

      getToken();
      factorType = exprString;
      break;

    case sSCALAR :
      if (g_abstractType)
        {
          if (g_tknPtr->sParm.v.vParent != g_abstractType) error(eSCALARTYPE);
        }
      else
        {
          g_abstractType = g_tknPtr->sParm.v.vParent;
        }

      pas_GenerateStackReference(opLDS, g_tknPtr);
      getToken();
      factorType = exprScalar;
      break;

    case sSET_OF :
      /* If an g_abstractType is specified then it should either be the */
      /* same SET OF <object> -OR- the same <object> */

      if (g_abstractType)
        {
          if ((g_tknPtr->sParm.v.vParent != g_abstractType) &&
              (g_tknPtr->sParm.v.vParent->sParm.t.tParent != g_abstractType))
            error(eSET);
        }
      else
        {
          g_abstractType = g_tknPtr->sParm.v.vParent;
        }

      pas_GenerateStackReference(opLDS, g_tknPtr);
      getToken();
      factorType = exprSet;
      break;

      /* SET factors */

    case '[' : /* Set constant */
      getToken();
      pas_GetSetFactor();
      if (g_token != ']') error(eRBRACKET);
      else getToken();
      factorType = exprSet;
      break;

      /* Complex factors */

    case sSUBRANGE :
    case sRECORD :
    case sRECORD_OBJECT :
    case sVAR_PARM :
    case sPOINTER :
    case sARRAY :
      factorType = pas_ComplexFactor();
      break;

      /* Functions */

    case sFUNC :
      factorType = pas_FunctionDesignator();
      break;

      /* Nested Expression */

    case '(' :
      getToken();
      factorType = pas_Expression(exprUnknown, g_abstractType);
      if (g_token == ')') getToken();
      else error (eRPAREN);
      break;

      /* Address references */

    case '^' :
      getToken();
      factorType = pas_PointerFactor();
      break;

      /* Highest Priority Operators */

    case '@':
      /* The address operator @ returns the address of a variable, procedure
       * or function.
       *
       * Verify that the expression expects a pointer type.
       */

      if (!IS_POINTER_EXPRTYPE(findExprType)) error(ePOINTERTYPE);

      /* Then handle the pointer factor */

      getToken();
      factorType = pas_PointerFactor();
      break;

    case tNOT:
      getToken();
      factorType = pas_Factor(findExprType);
      if (factorType != exprInteger && factorType != exprBoolean)
        {
          error(eFACTORTYPE);
        }

      pas_GenerateSimple(opNOT);
      break;

      /* Standard or Built-in function? */

    case tSTDFUNC:
      factorType = pas_StandardFunction();
      break;

    case tBUILTIN:
      factorType = pas_BuiltInFunction();
      break;

      /* Hmmm... Try the standard functions */

    default :
      error(eINVFACTOR);
      break;
    }

  return factorType;
}

/****************************************************************************/
/* Process a complex factor */

static exprType_t pas_ComplexFactor(void)
{
  varInfo_t varInfo;

  TRACE(g_lstFile,"[pas_ComplexFactor]");

  /* First, make a copy of the symbol table entry because the call to
   * pas_SimplifyFactor() will modify it.
   */

  varInfo.variable = *g_tknPtr;
  varInfo.fOffset  = 0;
  getToken();

  /* Then process the complex factor until it is reduced to a simple factor
   * (like int, char, etc.)
   */

  return pas_SimplifyFactor(&varInfo, 0);
}

/****************************************************************************/
/* Process a complex factor (recursively) until it becomes a simple factor */

static exprType_t pas_SimplifyFactor(varInfo_t *varInfo, uint8_t factorFlags)
{
  symbol_t  *varPtr = &varInfo->variable;
  symbol_t  *typePtr;
  symbol_t  *baseTypePtr;
  symbol_t  *nextPtr;
  exprType_t factorType;
  uint16_t   arrayKind;

  TRACE(g_lstFile,"[pas_SimplifyFactor]");

  /* Check if it has been reduced to a simple factor. */

  factorType = pas_BaseFactor(varPtr, factorFlags);
  if (factorType != exprUnknown)
    {
      return factorType;
    }

  /* NOPE... recurse until it becomes a simple factor
   *
   * Process the complex factor according to the current variable sKind.
   */

  typePtr = varPtr->sParm.v.vParent;
  switch (varPtr->sKind)
    {
    case sSUBRANGE :
      if (!g_abstractType) g_abstractType = typePtr;
      varPtr->sKind = typePtr->sParm.t.tSubType;
      factorType    = pas_SimplifyFactor(varInfo, factorFlags);
      break;

    case sRECORD :
      /* Check if this is a pointer to a record */

      if ((factorFlags & ADDRESS_FACTOR) != 0)
        {
          if (g_token == '.') error(ePOINTERTYPE);

          if ((factorFlags & INDEXED_FACTOR) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opLDS, varPtr);
            }

          factorType = exprRecordPtr;
        }

      /* Verify that a period separates the RECORD identifier from the */
      /* record field identifier */

      else if (g_token == '.')
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0 &&
              (factorFlags & VAR_PARM_FACTOR) == 0)
            {
              error(ePOINTERTYPE);
            }

          /* Skip over the period. */

          getToken();

          /* Verify that a field identifier associated with this record.
           * follows the period.
           */

          nextPtr         = typePtr->sParm.t.tParent;
          baseTypePtr     = typePtr;
          while (nextPtr != NULL && nextPtr->sKind == sTYPE)
            {
              baseTypePtr = nextPtr;
              nextPtr     = baseTypePtr->sParm.t.tParent;
            }

          if ((g_token != sRECORD_OBJECT) ||
              (g_tknPtr->sParm.r.rRecord != baseTypePtr))
            {
              error(eRECORDOBJECT);
              factorType = exprInteger;
            }
          else
            {
              /* Modify the variable so that it has the characteristics of
               * the field but with level and offset associated with the
               * record.
               */

              typePtr                 = g_tknPtr->sParm.r.rParent;
              varPtr->sKind           = typePtr->sParm.t.tType;
              varPtr->sParm.v.vParent = typePtr;

              /* Adjust the variable size and offset.  Add the RECORD offset
               * to the RECORD data stack offset to get the data stack
               * offset to the record object; Change the size to match the
               * size of RECORD object.
               */

              varPtr->sParm.v.vSize   = g_tknPtr->sParm.r.rSize;

              if (factorFlags == (INDEXED_FACTOR | ADDRESS_DEREFERENCE |
                                  VAR_PARM_FACTOR))
                {
                  /* Add the offset to the record field to the RECORD address
                   * that should already be on the stack.
                   */

                  pas_GenerateDataOperation(opPUSH, g_tknPtr->sParm.r.rOffset);
                  pas_GenerateSimple(opADD);
                }
              else if ((factorFlags & (ADDRESS_DEREFERENCE | VAR_PARM_FACTOR)) != 0)
                {
                  /* Remember the offset to RECORD object to RECORD data stack
                   * offset so that we can apply it later.
                   */

                  varInfo->fOffset = g_tknPtr->sParm.r.rOffset;
                }
              else
                {
                  /* Add the offset to RECORD object to RECORD data stack
                   * offset.
                   */

                  varPtr->sParm.v.vOffset += g_tknPtr->sParm.r.rOffset;
                }

              getToken();
              factorType = pas_SimplifyFactor(varInfo, factorFlags);
            }
        }

      /* A RECORD name may be a valid factor -- as the input */
      /* parameter of a function or in an assignment */

      else if (g_abstractType == typePtr)
        {
          /* Special case:  The record is a VAR parameter. */

          if (factorFlags == (INDEXED_FACTOR | ADDRESS_DEREFERENCE |
                              VAR_PARM_FACTOR))
            {
              pas_GenerateStackReference(opLDS, varPtr);
              pas_GenerateSimple(opADD);
              pas_GenerateDataSize(varPtr->sParm.v.vSize);
              pas_GenerateSimple(opLDIM);
            }
          else
            {
              pas_GenerateDataSize(varPtr->sParm.v.vSize);
              pas_GenerateStackReference(opLDSM, varPtr);
            }

          factorType = exprRecord;
        }
      else error(ePERIOD);
      break;

    case sRECORD_OBJECT :
      /* NOTE:  This must have been preceeded with a WITH statement
       * defining the RECORD type
       */

      if (!g_withRecord.parent)
        {
          error(eINVTYPE);
        }
      else if ((factorFlags && (ADDRESS_DEREFERENCE | ADDRESS_FACTOR)) != 0)
        {
          error(ePOINTERTYPE);
        }
      else if ((factorFlags && INDEXED_FACTOR) != 0)
        {
          error(eARRAYTYPE);
        }

      /* Verify that a field identifier is associated with the RECORD
       * specified by the WITH statement.
       */

      else if (varPtr->sParm.r.rRecord != g_withRecord.parent)
        {
          error(eRECORDOBJECT);
        }
      else
        {
          int16_t tempOffset;

          /* Now there are two cases to consider:  (1) the g_withRecord is a */
          /* pointer to a RECORD, or (2) the g_withRecord is the RECOR itself */

          if (g_withRecord.pointer)
            {
              /* If the pointer is really a VAR parameter, then other syntax */
              /* rules will apply */

              if (g_withRecord.varParm)
                {
                  factorFlags |= (INDEXED_FACTOR | ADDRESS_DEREFERENCE |
                                  VAR_PARM_FACTOR);
                }
              else
                {
                  factorFlags |= (INDEXED_FACTOR | ADDRESS_DEREFERENCE);
                }

              pas_GenerateDataOperation(opPUSH,
                                        varPtr->sParm.r.rOffset + g_withRecord.index);
              tempOffset   = g_withRecord.wOffset;
            }
          else
            {
              tempOffset   = varPtr->sParm.r.rOffset + g_withRecord.wOffset;
            }

          /* Modify the variable so that it has the characteristics of the */
          /* the field but with level and offset associated with the record */
          /* NOTE:  We have to be careful here because the structure */
          /* associated with sRECORD_OBJECT is not the same as for */
          /* variables! */

          typePtr                 = varPtr->sParm.r.rParent;
          tempOffset              = varPtr->sParm.r.rOffset;

          varPtr->sKind           = typePtr->sParm.t.tType;
          varPtr->sLevel          = g_withRecord.level;
          varPtr->sParm.v.vSize   = typePtr->sParm.t.tAllocSize;
          varPtr->sParm.v.vOffset = tempOffset + g_withRecord.wOffset;
          varPtr->sParm.v.vParent = typePtr;

          factorType = pas_SimplifyFactor(varInfo, factorFlags);
        }
      break;

    case sPOINTER :
      /* Are we dereferencing a pointer? */

      if (g_token == '^')
        {
          getToken();
          factorFlags |= ADDRESS_DEREFERENCE;
        }
      else
        {
          factorFlags |= ADDRESS_FACTOR;
        }

      /* If the parent type is itself a typed pointer, then get the
       * pointed-at type.
       */

      if (/* typePtr->sKind == sTYPE && */ typePtr->sParm.t.tType == sPOINTER)
        {
          baseTypePtr   = typePtr->sParm.t.tParent;
          varPtr->sKind = baseTypePtr->sParm.t.tType;

          /* REVISIT:  What if the type is a pointer to a pointer? */

           if (varPtr->sKind == sPOINTER) fatal(eNOTYET);
        }
      else
        {
          /* Get the kind of parent type */

          varPtr->sKind = typePtr->sParm.t.tType;
        }

      factorType = pas_SimplifyFactor(varInfo, factorFlags);
      break;

    case sVAR_PARM :
      if ((factorFlags & (ADDRESS_DEREFERENCE | VAR_PARM_FACTOR)) != 0)
        {
          error(eVARPARMTYPE);
        }

      factorFlags  |= (ADDRESS_DEREFERENCE | VAR_PARM_FACTOR);
      varPtr->sKind = typePtr->sParm.t.tType;
      factorType    = pas_SimplifyFactor(varInfo, factorFlags);
      break;

    case sARRAY :
      if ((factorFlags & INDEXED_FACTOR) != 0)
        {
          error(eARRAYTYPE);
        }

      /* Get a pointer to the underlying base type of the array */

      nextPtr         = typePtr;
      baseTypePtr     = typePtr;
      while (nextPtr != NULL && nextPtr->sKind == sTYPE)
        {
          baseTypePtr = nextPtr;
          nextPtr     = baseTypePtr->sParm.t.tParent;
        }

      /* Extract the base type */

      arrayKind = baseTypePtr->sParm.t.tType;

      /* REVISIT:  For subranges, we use the base type of the subrange. */

      if (arrayKind == sSUBRANGE)
        {
          arrayKind = baseTypePtr->sParm.t.tSubType;
        }

      /* The "normal" case, an array will be followed by an index within
       * braces.
       */

      if (g_token == '[')
        {
          /* Get the type of the index.  We will need minimum value of
           * if the index type in order to offset the array index
           * calculation
           */

          symbol_t *indexTypePtr = typePtr->sParm.t.tIndex;
          if (indexTypePtr == NULL) error(eHUH);
          else
            {
              factorFlags     |= INDEXED_FACTOR;

              /* Generate the array offset calculation and indexed load */

              pas_ArrayIndex(indexTypePtr, baseTypePtr->sParm.t.tAllocSize);

              /* We have reduced this to a base type.  So we can generate
               * the indexed load from that base type.
               */

              varPtr->sKind  = arrayKind;
              factorType     = pas_SimplifyFactor(varInfo, factorFlags);

              if (factorType == exprUnknown)
                {
                  error(eHUH);  /* Should never happen */
                }

              /* Return the parent type of the array */

              varPtr->sKind         = typePtr->sParm.t.tType;
              varPtr->sParm.v.vSize = typePtr->sParm.t.tAllocSize;
            }
        }

      /* A very special case is 'PACKED ARRAY[] OF CHAR' which legacy pascal
       * treats as a STRING.
       */

      else if (arrayKind == sCHAR)
        {
          /* Convert the char array to a string using the BSTR2STR run-time
           * library function.  We need:
           *
           *    TOS   = Address of array
           *    TOS+1 = Size of array (bytes)
           */

          pas_GenerateDataOperation(opPUSH, varPtr->sParm.v.vSize);

          /* This could be either a simple packed array of char, or it
           * could a packed array of char field of a RECORD.
           */

          if ((factorFlags & (ADDRESS_DEREFERENCE | VAR_PARM_FACTOR)) != 0)
            {
              uint16_t fieldOffset;

              pas_GenerateStackReference(opLDS, varPtr);

              fieldOffset = varInfo->fOffset;
              if (fieldOffset != 0)
                {
                  pas_GenerateDataOperation(opPUSH, fieldOffset);
                  pas_GenerateSimple(opADD);
                }
            }
          else
            {
              varPtr->sParm.v.vOffset += varInfo->fOffset;
              pas_GenerateStackReference(opLAS, varPtr);
            }

          pas_StandardFunctionCall(lbBSTR2STR);
          factorType = exprString;
        }

      /* An ARRAY name may be a valid factor as the input parameter of
       * a function.
       */

      else if (g_abstractType == varPtr)
        {
          pas_GenerateDataSize(varPtr->sParm.v.vSize);
          pas_GenerateStackReference(opLDSM, varPtr);
          factorType = pas_MapVariable2ExprType(varPtr->sParm.t.tType, false);
        }
      else
        {
          error(eLBRACKET);
        }
      break;

    default :
      error(eINVTYPE);
      factorType = exprInteger;
      break;
    }

  return factorType;
}

/**********************************************************************/

static exprType_t pas_BaseFactor(symbol_t *varPtr, uint8_t factorFlags)
{
  symbol_t  *typePtr;
  exprType_t factorType;

  TRACE(g_lstFile,"[pas_BaseFactor]");

  /* Process according to the current variable sKind */

  typePtr = varPtr->sParm.v.vParent;
  switch (varPtr->sKind)
    {
      /* Check if we have reduced the complex factor to a simple factor */

    case sINT :
      if ((factorFlags & INDEXED_FACTOR) != 0)
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              pas_GenerateSimple(opLDI);
              factorType = exprInteger;
            }
          else if ((factorFlags & ADDRESS_FACTOR) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              factorType = exprIntegerPtr;
            }
          else
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              factorType = exprInteger;
            }
        }
      else
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              pas_GenerateSimple(opLDI);
              factorType = exprInteger;
            }
          else if ((factorFlags & ADDRESS_FACTOR) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              factorType = exprIntegerPtr;
            }
          else
            {
              pas_GenerateStackReference(opLDS, varPtr);
              factorType = exprInteger;
            }
        }
      break;

    case sCHAR :
      if ((factorFlags & INDEXED_FACTOR) != 0)
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              pas_GenerateSimple(opLDIB);
              factorType = exprChar;
            }
          else if ((factorFlags & ADDRESS_FACTOR) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              factorType = exprCharPtr;
            }
          else
            {
              pas_GenerateStackReference(opLDSXB, varPtr);
              factorType = exprChar;
            }
        }
      else
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              pas_GenerateSimple(opLDIB);
              factorType = exprChar;
            }
          else if ((factorFlags & ADDRESS_FACTOR) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              factorType = exprCharPtr;
            }
          else
            {
              pas_GenerateStackReference(opLDSB, varPtr);
              factorType = exprChar;
            }
        }
      break;

    case sBOOLEAN :
      if ((factorFlags & INDEXED_FACTOR) != 0)
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              pas_GenerateSimple(opLDI);
              factorType = exprBoolean;
            }
          else if ((factorFlags & ADDRESS_FACTOR) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              factorType = exprBooleanPtr;
            }
          else
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              factorType = exprBoolean;
            }
        }
      else
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              pas_GenerateSimple(opLDI);
              factorType = exprBoolean;
            }
          else if ((factorFlags & ADDRESS_FACTOR) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              factorType = exprBooleanPtr;
            }
          else
            {
              pas_GenerateStackReference(opLDS, varPtr);
              factorType = exprBoolean;
            }
        }
      break;

    /* The only thing that REAL and STRING have in common is that they are
     * both represented by a multi-word object.
     */

    case sREAL         :
    case sSTRING       :
      if ((factorFlags & INDEXED_FACTOR) != 0)
        {
          symbol_t *baseTypePtr;
          symbol_t *nextType;

          /* In the case of an array, the size of the variable refers to the size of
           * array.  We need to traverse back to the base type of the array to get the
           * size of and element.
           */

          baseTypePtr = typePtr;
          nextType    = typePtr->sParm.t.tParent;

          while (nextType != NULL && baseTypePtr->sKind == sTYPE)
            {
              baseTypePtr = nextType;
              nextType    = baseTypePtr->sParm.t.tParent;
            }

          /* Now we can handle all of the ornaments */

          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              pas_GenerateDataSize(baseTypePtr->sParm.t.tAllocSize);
              pas_GenerateSimple(opLDIM);
              factorType = (varPtr->sKind) == sREAL ? exprReal : exprString;
            }
          else if ((factorFlags & ADDRESS_FACTOR) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              factorType = (varPtr->sKind) == sREAL ? exprRealPtr : exprStringPtr;
            }
          else
            {
              pas_GenerateDataSize(baseTypePtr->sParm.t.tAllocSize);
              pas_GenerateStackReference(opLDSXM, varPtr);
              factorType = (varPtr->sKind) == sREAL ? exprReal : exprString;
            }
        }
      else
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              pas_GenerateDataSize(varPtr->sParm.v.vSize);
              pas_GenerateSimple(opLDIM);
              factorType = (varPtr->sKind) == sREAL ? exprReal : exprString;
            }
          else if ((factorFlags & ADDRESS_FACTOR) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              factorType = (varPtr->sKind) == sREAL ? exprRealPtr : exprStringPtr;
            }
          else
            {
              pas_GenerateDataSize(varPtr->sParm.v.vSize);
              pas_GenerateStackReference(opLDSM, varPtr);
              factorType = (varPtr->sKind) == sREAL ? exprReal : exprString;
            }
        }
      break;

    case sSCALAR :
      if (!g_abstractType)
        {
          g_abstractType = typePtr;
        }
      else if (typePtr != g_abstractType)
        {
          error(eSCALARTYPE);
        }

      if ((factorFlags & INDEXED_FACTOR) != 0)
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              pas_GenerateSimple(opLDI);
              factorType = exprScalar;
            }
          else if ((factorFlags & ADDRESS_FACTOR) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              factorType = exprScalarPtr;
            }
          else
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              factorType = exprScalar;
            }
        }
      else
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              pas_GenerateSimple(opLDI);
              factorType = exprScalar;
            }
          else if ((factorFlags & ADDRESS_FACTOR) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              factorType = exprScalarPtr;
            }
          else
            {
              pas_GenerateStackReference(opLDS, varPtr);
              factorType = exprScalar;
            }
        }
      break;

    case sSET_OF :
      if (!g_abstractType)
        {
          g_abstractType = typePtr;
        }
      else if ((typePtr != g_abstractType) &&
               (typePtr->sParm.v.vParent != g_abstractType))
        {
          error(eSCALARTYPE);
        }

      if ((factorFlags & INDEXED_FACTOR) != 0)
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              pas_GenerateSimple(opLDI);
              factorType = exprSet;
            }
          else if ((factorFlags & ADDRESS_FACTOR) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              factorType = exprSetPtr;
            }
          else
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              factorType = exprSet;
            }
        }
      else
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              pas_GenerateSimple(opLDI);
              factorType = exprSet;
            }
          else if ((factorFlags & ADDRESS_FACTOR) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              factorType = exprSetPtr;
            }
          else
            {
              pas_GenerateStackReference(opLDS, varPtr);
              factorType = exprSet;
            }
        }
      break;

    case sFILE :
    case sTEXTFILE :
      if ((factorFlags & INDEXED_FACTOR) != 0)
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              pas_GenerateSimple(opLDI);
              factorType = exprFile;
            }
          else if ((factorFlags & ADDRESS_FACTOR) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              factorType = exprFilePtr;
            }
          else
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              factorType = exprFile;
            }
        }
      else
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              pas_GenerateSimple(opLDI);
              factorType = exprFile;
            }
          else if ((factorFlags & ADDRESS_FACTOR) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              factorType = exprFilePtr;
            }
           else
             {
              pas_GenerateStackReference(opLDS, varPtr);
               factorType = exprFile;
             }
        }
      break;

    /* REVISIT: RECORD name may be a base factor -- as the input parameter of
     * a function or in an assignment
     */

    default :
      factorType = exprUnknown;
      break;
    }

  return factorType;
}

/**********************************************************************/
/* Process a factor of the for ^variable OR a VAR parameter (where the
 * ^ is implicit.
 */

static exprType_t pas_PointerFactor(void)
{
  exprType_t factorType;

  TRACE(g_lstFile,"[pas_PointerFactor]");

  /* Process by token type */

  switch (g_token)
    {
      /* Pointers to simple types */

      case sINT              :
        pas_GenerateStackReference(opLAS, g_tknPtr);
        getToken();
        factorType = exprIntegerPtr;
        break;

      case sBOOLEAN :
        pas_GenerateStackReference(opLAS, g_tknPtr);
        getToken();
        factorType = exprBooleanPtr;
        break;

      case sCHAR           :
        pas_GenerateStackReference(opLAS, g_tknPtr);
        getToken();
        factorType = exprCharPtr;
        break;

      case sREAL              :
        pas_GenerateStackReference(opLAS, g_tknPtr);
        getToken();
        factorType = exprRealPtr;
        break;

      case sSCALAR :
        if (g_abstractType)
          {
            if (g_tknPtr->sParm.v.vParent != g_abstractType) error(eSCALARTYPE);
          }
        else
          {
           g_abstractType = g_tknPtr->sParm.v.vParent;
          }

        pas_GenerateStackReference(opLAS, g_tknPtr);
        getToken();
        factorType = exprScalarPtr;
        break;

      case sSET_OF :
        /* If an g_abstractType is specified then it should either be the
         * same SET OF <object> -OR- the same <object>
         */

        if (g_abstractType)
          {
            if ((g_tknPtr->sParm.v.vParent != g_abstractType)
            &&   (g_tknPtr->sParm.v.vParent->sParm.t.tParent != g_abstractType))
              {
                 error(eSET);
              }
          }
        else
          {
             g_abstractType = g_tknPtr->sParm.v.vParent;
          }

        pas_GenerateStackReference(opLAS, g_tknPtr);
        getToken();
        factorType = exprSetPtr;
        break;

      case sSTRING           :
        pas_GenerateStackReference(opLAS, g_tknPtr);
        getToken();
        factorType = exprStringPtr;
        break;

      case sFILE             :
      case sTEXTFILE         :
        pas_GenerateStackReference(opLAS, g_tknPtr);
        getToken();
        factorType = exprFilePtr;
        break;

      /* Complex factors */

      case sSUBRANGE :
      case sRECORD :
      case sRECORD_OBJECT :
      case sVAR_PARM :
      case sPOINTER :
      case sARRAY :
        factorType = pas_ComplexPointerFactor();
        break;

      /* References to address of a pointer */

      case '^' :
        error(eNOTYET);
        getToken();
        factorType = pas_PointerFactor();
        break;

      case '('             :
        getToken();
        factorType = pas_PointerFactor();
        if (g_token != ')') error (eRPAREN);
        else getToken();
        break;

      default :
        error(ePTRADR);
        factorType = exprUnknown;
        break;
    }

  return factorType;
}

/****************************************************************************/
/* Process a complex pointer factor */

static exprType_t pas_ComplexPointerFactor(void)
{
  varInfo_t varInfo;

  TRACE(g_lstFile,"[pas_ComplexPointerFactor]");

  /* First, make a copy of the symbol table entry because the call to
   * pas_SimplifyPointerFactor() will modify it.
   */

  varInfo.variable = *g_tknPtr;
  varInfo.fOffset  = 0;
  getToken();

  /* Then process the complex factor until it is reduced to a simple
   * factor (like int, char, etc.)
   */

  return pas_SimplifyPointerFactor(&varInfo, 0);
}

/****************************************************************************/
/* Process a complex factor (recursively) until it becomes a simple
 * factor.
 */

static exprType_t pas_SimplifyPointerFactor(varInfo_t *varInfo,
                                            uint8_t factorFlags)
{
  symbol_t  *varPtr = &varInfo->variable;
  symbol_t  *typePtr;
  exprType_t factorType;

  TRACE(g_lstFile,"[pas_SimplifyPointerFactor]");

  /* Check if it has been reduced to a simple factor. */

  factorType = pas_BasePointerFactor(varPtr, factorFlags);
  if (factorType != exprUnknown)
    {
      return factorType;
    }

  /* NOPE... recurse until it becomes a simple pointer factor
   *
   * Process the complex factor according to the current variable sKind.
   */

  typePtr = varPtr->sParm.v.vParent;
  switch (varPtr->sKind)
    {
      /* Check if we have reduced the complex factor to a simple factor */

    case sSUBRANGE :
      if (!g_abstractType) g_abstractType = typePtr;
      varPtr->sKind = typePtr->sParm.t.tSubType;
      factorType = pas_SimplifyPointerFactor(varInfo, factorFlags);
      break;

    case sRECORD :
      /* Check if this is a pointer to a record */

      if (g_token != '.')
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              error(ePOINTERTYPE);
            }

          if ((factorFlags & INDEXED_FACTOR) != 0)
            {
              pas_GenerateStackReference(opLASX, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opLAS, varPtr);
            }

          factorType = exprRecordPtr;
        }
      else
        {
          /* Verify that a period separates the RECORD identifier from the
           * record field identifier
           */

          if (g_token != '.') error(ePERIOD);
          else getToken();

          /* Verify that a field identifier associated with this record
           * follows the period.
           */

          if ((g_token != sRECORD_OBJECT) ||
              (g_tknPtr->sParm.r.rRecord != typePtr))
            {
              error(eRECORDOBJECT);
              factorType = exprInteger;
            }
          else
            {
              /* Modify the variable so that it has the characteristics
               * of the field but with level and offset associated with
               * the record
               */

              typePtr                 = g_tknPtr->sParm.r.rParent;
              varPtr->sKind           = typePtr->sParm.t.tType;
              varPtr->sParm.v.vParent = typePtr;

              varInfo->fOffset        = g_tknPtr->sParm.r.rOffset;

              getToken();
              factorType = pas_SimplifyPointerFactor(varInfo, factorFlags);
            }
        }
      break;

    case sRECORD_OBJECT :
      /* NOTE:  This must have been preceeded with a WITH statement
       * defining the RECORD type
       */

      if (!g_withRecord.parent)
        {
          error(eINVTYPE);
        }
      else if ((factorFlags && ADDRESS_DEREFERENCE) != 0)
        {
          error(ePOINTERTYPE);
        }
      else if ((factorFlags && INDEXED_FACTOR) != 0)
        {
          error(eARRAYTYPE);
        }

      /* Verify that a field identifier is associated with the RECORD
       * specified by the WITH statement.
       */

      else if (varPtr->sParm.r.rRecord != g_withRecord.parent)
        {
          error(eRECORDOBJECT);
        }
      else
        {
          int16_t tempOffset;

          /* Now there are two cases to consider:  (1) the g_withRecord is a
           * pointer to a RECORD, or (2) the g_withRecord is the RECOR itself
           */

          if (g_withRecord.pointer)
            {
              pas_GenerateDataOperation(opPUSH,
                                        varPtr->sParm.r.rOffset +
                                        g_withRecord.index);
              factorFlags |= (INDEXED_FACTOR | ADDRESS_DEREFERENCE);
              tempOffset   = g_withRecord.wOffset;
            }
          else
            {
              tempOffset   = varPtr->sParm.r.rOffset + g_withRecord.wOffset;
            }

          /* Modify the variable so that it has the characteristics of the
           * the field but with level and offset associated with the record
           * NOTE:  We have to be careful here because the structure
           * associated with sRECORD_OBJECT is not the same as for
           * variables!
           *
           * REVISIT:  What is going on with tempOffset here?
           */

          typePtr                 = varPtr->sParm.r.rParent;
          tempOffset              = varPtr->sParm.r.rOffset;

          varPtr->sKind           = typePtr->sParm.t.tType;
          varPtr->sLevel          = g_withRecord.level;
          varPtr->sParm.v.vSize   = typePtr->sParm.t.tAllocSize;
          varPtr->sParm.v.vOffset = tempOffset + g_withRecord.wOffset;
          varPtr->sParm.v.vParent = typePtr;

          factorType = pas_SimplifyPointerFactor(varInfo, factorFlags);
        }
      break;

    case sPOINTER :
      if (g_token == '^') error(ePTRADR);
      else getToken();

      factorFlags   |= ADDRESS_DEREFERENCE;
      varPtr->sKind  = typePtr->sParm.t.tType;
      factorType     = pas_SimplifyPointerFactor(varInfo, factorFlags);
      break;

    case sVAR_PARM :
      if (factorFlags != 0) error(eVARPARMTYPE);

      factorFlags  |= ADDRESS_DEREFERENCE;
      varPtr->sKind = typePtr->sParm.t.tType;
      factorType    = pas_SimplifyPointerFactor(varInfo, factorFlags);
      break;

    case sARRAY :
      if ((factorFlags & ~ADDRESS_DEREFERENCE) != 0)
        {
          error(eARRAYTYPE);
        }

      if (g_token == '[')
        {
          /* Get the type of the index.  We will need minimum value of
           * if the index type in order to offset the array index
           * calculation
           */

          symbol_t *indexTypePtr = typePtr->sParm.t.tIndex;
          if (indexTypePtr == NULL) error(eHUH);
          else
            {
              symbol_t *nextPtr;
              symbol_t *baseTypePtr;
              uint16_t arrayKind;

              factorFlags     |= INDEXED_FACTOR;

              /* Get a pointer to the underlying base type symbol */

              nextPtr          = typePtr;
              baseTypePtr      = typePtr;
              while (nextPtr != NULL && nextPtr->sKind == sTYPE)
                {
                   baseTypePtr = nextPtr;
                   nextPtr     = baseTypePtr->sParm.t.tParent;
                }

              /* Generate the array offset calculation */

              pas_ArrayIndex(indexTypePtr, baseTypePtr->sParm.t.tAllocSize);

              /* We have reduced this to a base type.  So we can generate
               * the indexed load from that base type.
               */

              arrayKind = baseTypePtr->sParm.t.tType;

             /* REVISIT:  For subranges, we use the base type of the
              * subrange.
              */

              if (arrayKind == sSUBRANGE)
                {
                  arrayKind = baseTypePtr->sParm.t.tSubType;
                }

              /* If this is an array of records, then are not finished. */

              varPtr->sKind = arrayKind;
              if (arrayKind == sRECORD)
                {
                  factorType =
                    pas_SimplifyPointerFactor(varInfo, factorFlags);
                }

              /* Load the indexed base type */

              else
                {
                  factorType = pas_BasePointerFactor(varPtr, factorFlags);
                }

              if (factorType == exprUnknown)
                {
                  error(eHUH);  /* Should never happen */
                }

              /* Return the parent type of the array */

              varPtr->sKind         = baseTypePtr->sParm.t.tType;
              varPtr->sParm.v.vSize = baseTypePtr->sParm.t.tAllocSize;
            }
        }
      break;

    default :
      error(eINVTYPE);
      factorType = exprInteger;
      break;
    }

  return factorType;
}

/****************************************************************************/

static exprType_t pas_BasePointerFactor(symbol_t *varPtr, uint8_t factorFlags)
{
  symbol_t  *typePtr;
  exprType_t factorType;

  TRACE(g_lstFile,"[pas_BasePointerFactor]");

  /* NOPE... recurse until it becomes a simple pointer factor
   *
   * Process the complex factor according to the current variable sKind.
   */

  typePtr = varPtr->sParm.v.vParent;
  switch (varPtr->sKind)
    {
      /* Check if we have reduced the complex factor to a simple factor */

    case sINT :
      if ((factorFlags & INDEXED_FACTOR) != 0)
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opLASX, varPtr);
            }

          factorType = exprIntegerPtr;
        }
      else
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opLAS, varPtr);
            }

          factorType = exprIntegerPtr;
        }
      break;

    case sCHAR :
      if ((factorFlags & INDEXED_FACTOR) != 0)
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opLASX, varPtr);
            }

          factorType = exprCharPtr;
        }
      else
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opLAS, varPtr);
            }

          factorType = exprCharPtr;
        }
      break;

    case sBOOLEAN :
      if ((factorFlags & INDEXED_FACTOR) != 0)
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opLASX, varPtr);
            }

          factorType = exprBooleanPtr;
        }
      else
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opLAS, varPtr);
            }

          factorType = exprBooleanPtr;
        }
      break;

    /* The only thing that REAL and STRING have in common is that they are
     * both represented by a multi-word object.
     */

    case sREAL         :
    case sSTRING       :
      if ((factorFlags & INDEXED_FACTOR) != 0)
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opLASX, varPtr);
            }

          factorType = (varPtr->sKind) == sREAL ? exprRealPtr : exprStringPtr;
        }
      else
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opLAS, varPtr);
            }

          factorType = (varPtr->sKind) == sREAL ? exprRealPtr : exprStringPtr;
        }
      break;

    case sSCALAR :
      if (!g_abstractType)
        {
          g_abstractType = typePtr;
        }
      else if (typePtr != g_abstractType)
        {
          error(eSCALARTYPE);
        }

      if ((factorFlags & INDEXED_FACTOR) != 0)
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opLASX, varPtr);
            }

          factorType = exprScalarPtr;
        }
      else
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opLAS, varPtr);
            }

          factorType = exprScalarPtr;
        }
      break;

    case sSET_OF :
      if (!g_abstractType)
        {
          g_abstractType = typePtr;
        }
      else if ((typePtr != g_abstractType) &&
               (typePtr->sParm.v.vParent != g_abstractType))
        {
          error(eSCALARTYPE);
        }

      if ((factorFlags & INDEXED_FACTOR) != 0)
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opLASX, varPtr);
            }

          factorType = exprSetPtr;
        }
      else
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opLAS, varPtr);
            }

          factorType = exprSetPtr;
        }
      break;

    case sFILE :
    case sTEXTFILE :
      if ((factorFlags & INDEXED_FACTOR) != 0)
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              factorType = exprFile;
            }
          else
            {
              pas_GenerateStackReference(opLASX, varPtr);
              factorType = exprFilePtr;
            }
        }
      else
        {
          if ((factorFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              factorType = exprFile;
            }
          else
            {
              pas_GenerateStackReference(opLAS, varPtr);
              factorType = exprFilePtr;
            }
        }
      break;

    default :
      factorType = exprUnknown;
      break;
    }

  return factorType;
}

/****************************************************************************/

static exprType_t pas_FunctionDesignator(void)
{
  symbol_t  *funcPtr      = g_tknPtr;
  symbol_t  *typePtr      = funcPtr->sParm.p.parent;
  exprType_t factorType;
  int        size         = 0;

  TRACE(g_lstFile,"[pas_FunctionDesignator]");

  /* FORM: function-designator =
   *       function-identifier [ actual-parameter-list ]
   */

  /* Allocate stack space for a reference instance of the type returned by
   * the function.  This is an "container" that will catch the valued
   * returned by the function.
   *
   * STRING return value containers need some special initialization.
   */

  if (typePtr->sKind == sTYPE && typePtr->sParm.t.tType == sSTRING)
    {
      /* REVISIT:  This string container really needs to be enclosed in PUSHS
       * and POPS.  Need to assure that in order to release string stack as
       * soon as possible after the temporary container is released.
       */

      pas_StandardFunctionCall(lbSTRTMP);
    }
  else
    {
      pas_GenerateDataOperation(opINDS, typePtr->sParm.t.tAllocSize);
    }

  /* Get the type of the function */

  factorType = pas_GetExpressionType(typePtr);
  pas_SetAbstractType(typePtr);

   /* Skip over the function-identifier */

  getToken();

  /* Get the actual parameters (if any) associated with the procedure
   * call.  This will lie in the stack "above" the function return
   * value container.
   */

  size = pas_ActualParameterList(funcPtr);

  /* Generate function call and stack adjustment (if required) */

  pas_GenerateProcedureCall(funcPtr);

  /* Release the actual parameter list (if any). */

  if (size)
    {
      pas_GenerateDataOperation(opINDS, -size);
    }

  return factorType;
}

/*************************************************************************/
/* Determine the expression type associated with a pointer to a type
 * symbol
 */

static void pas_SetAbstractType(symbol_t *sType)
{
  TRACE(g_lstFile,"[pas_SetAbstractType]");

  if (sType != NULL &&
      sType->sKind == sTYPE &&
      sType->sParm.t.tType == sPOINTER)
    {
      sType = sType->sParm.t.tParent;
    }

  if (sType != NULL && sType->sKind == sTYPE)
  {
    switch (sType->sParm.t.tType)
      {
        case sSCALAR :
          if (g_abstractType)
            {
              if (sType != g_abstractType) error(eSCALARTYPE);
            }
          else
            {
              g_abstractType = sType;
            }
          break;

        case sSUBRANGE :
          if (!g_abstractType)
            {
              g_abstractType = sType;
            }
          else if ((g_abstractType->sParm.t.tType != sSUBRANGE)
          ||        (g_abstractType->sParm.t.tSubType != sType->sParm.t.tSubType))
            {
              error(eSUBRANGETYPE);
            }

          switch (sType->sParm.t.tSubType)
            {
              case sINT :
              case sCHAR :
                break;

              case sSCALAR :
                if (g_abstractType != sType) error(eSUBRANGETYPE);
                break;

              default :
                error(eSUBRANGETYPE);
                break;
            }
          break;
        }
    }
  else
    {
      error(eINVTYPE);
    }
}

/****************************************************************************/
static void pas_GetSetFactor(void)
{
  setType_t s;

  TRACE(g_lstFile,"[pas_GetSetFactor]");

  /* FORM: [[<constant>[,<constant>[, ...]]]] */
  /* ASSUMPTION:  The first '[' has already been processed */

  /* First, verify that a scalar expression type has been specified */
  /* If the g_abstractType is a SET, then we will need to get the TYPE */
  /* that it is a SET OF */

  if (g_abstractType)
    {
      if (g_abstractType->sParm.t.tType == sSET_OF)
        {
          s.typePtr = g_abstractType->sParm.t.tParent;
        }
      else
        {
          s.typePtr = g_abstractType;
        }
    }
  else
    {
      s.typePtr   = NULL;
    }

  /* Now, get the associated type and MIN/MAX values */

  if (s.typePtr && s.typePtr->sParm.t.tType == sSCALAR)
    {
      s.typeFound = true;
      s.setType   = sSCALAR;
      s.minValue  = s.typePtr->sParm.t.tMinValue;
      s.maxValue  = s.typePtr->sParm.t.tMaxValue;
    }
  else if (s.typePtr && s.typePtr->sParm.t.tType == sSUBRANGE)
    {
      s.typeFound = true;
      s.setType   = s.typePtr->sParm.t.tSubType;
      s.minValue  = s.typePtr->sParm.t.tMinValue;
      s.maxValue  = s.typePtr->sParm.t.tMaxValue;
    }
  else
    {
      error(eSET);
      s.typeFound = false;
      s.typePtr   = NULL;
      s.minValue  = 0;
      s.maxValue  = BITS_IN_INTEGER - 1;
    }

  /* Get the first element of the set */

  pas_GetSetElement(&s);

  /* Incorporate each additional element into the set */
  /* NOTE:  The optimizer will combine sets of constant elements into a */
  /* single PUSH! */

  while (g_token == ',')
    {
      /* Get the next element of the set */

      getToken();
      pas_GetSetElement(&s);

      /* OR it with the previous element */

      pas_GenerateSimple(opOR);
    }
}

/****************************************************************************/
static void pas_GetSetElement(setType_t *s)
{
  uint16_t setValue;
  int16_t firstValue;
  int16_t lastValue;
  symbol_t  *setPtr;

  TRACE(g_lstFile,"[pas_GetSetElement]");

  switch (g_token)
    {
      case sSCALAR_OBJECT : /* A scalar or scalar subrange constant */
        firstValue = g_tknPtr->sParm.c.val.i;
        if (!s->typeFound)
          {
            s->typeFound = true;
            s->typePtr   = g_tknPtr->sParm.c.parent;
            s->setType   = sSCALAR;
            s->minValue  = s->typePtr->sParm.t.tMinValue;
            s->maxValue  = s->typePtr->sParm.t.tMaxValue;
          }
        else if (s->setType != sSCALAR ||
                 s->typePtr != g_tknPtr->sParm.c.parent)
          {
            error(eSET);
          }

        goto addBit;

      case tINT_CONST : /* An integer subrange constant ? */
        firstValue = g_tknInt;
        if (!s->typeFound)
          {
            s->typeFound = true;
            s->setType   = sINT;
          }
        else if (s->setType != sINT)
          {
            error(eSET);
          }

        goto addBit;

      case tCHAR_CONST : /* A character subrange constant */
        firstValue = g_tknInt;
        if (!s->typeFound)
          {
            s->typeFound = true;
            s->setType   = sCHAR;
          }
        else if (s->setType != sCHAR)
          {
            error(eSET);
          }

      addBit:
        /* Check if the constant set element is the first value in a
         * subrange of values.
         */

        getToken();
        if (g_token != tSUBRANGE)
          {
            /* Verify that the new value is in range */

            if (firstValue < s->minValue || firstValue > s->maxValue)
              {
               error(eSETRANGE);
               setValue = 0;
              }
            else
              {
                setValue = (1 << (firstValue - s->minValue));
              }

            /* Now, generate P-Code to push the set value onto the stack */

            pas_GenerateDataOperation(opPUSH, setValue);
          }
        else
          {
            if (!s->typeFound) error(eSUBRANGETYPE);

            /* Skip over the tSUBRANGE token */

            getToken();

            /* TYPE check */

            switch (g_token)
              {
                case sSCALAR_OBJECT : /* A scalar or scalar subrange constant */
                  lastValue = g_tknPtr->sParm.c.val.i;
                  if (s->setType != sSCALAR ||
                      s->typePtr != g_tknPtr->sParm.c.parent)
                    {
                      error(eSET);
                    }

                  goto addLottaBits;

                case tINT_CONST : /* An integer subrange constant ? */
                   lastValue = g_tknInt;
                   if (s->setType != sINT) error(eSET);
                   goto addLottaBits;

                case tCHAR_CONST : /* A character subrange constant */
                   lastValue = g_tknInt;
                   if (s->setType != sCHAR) error(eSET);

                addLottaBits :
                   /* Verify that the first value is in range */

                    if (firstValue < s->minValue)
                      {
                        error(eSETRANGE);
                        firstValue = s->minValue;
                      }
                    else if (firstValue > s->maxValue)
                      {
                        error(eSETRANGE);
                        firstValue = s->maxValue;
                      }

                    /* Verify that the last value is in range */

                    if (lastValue < firstValue)
                      {
                        error(eSETRANGE);
                        lastValue = firstValue;
                      }
                    else if (lastValue > s->maxValue)
                      {
                        error(eSETRANGE);
                        lastValue = s->maxValue;
                      }

                    /* Set all bits from firstValue through lastValue */

                    setValue  = (0xffff << (firstValue - s->minValue));
                    setValue &= (0xffff >> ((BITS_IN_INTEGER - 1) -
                                            (lastValue - s->minValue)));

                    /* Now, generate P-Code to push the set value onto the
                     * stack.
                     */

                    pas_GenerateDataOperation(opPUSH, setValue);
                    break;

                 case sSCALAR :
                    if (!s->typePtr ||
                        s->typePtr != g_tknPtr->sParm.v.vParent)
                      {
                        error(eSET);

                        if (!s->typePtr)
                          {
                            s->typeFound = true;
                            s->typePtr   = g_tknPtr->sParm.v.vParent;
                            s->setType   = sSCALAR;
                            s->minValue  = s->typePtr->sParm.t.tMinValue;
                            s->maxValue  = s->typePtr->sParm.t.tMaxValue;
                          }
                      }

                    goto addVarToBits;

                  case sINT : /* An integer subrange variable ? */
                  case sCHAR : /* A character subrange variable? */
                    if (s->setType != g_token) error(eSET);
                    goto addVarToBits;

                  case sSUBRANGE :
                    if (!s->typePtr || s->typePtr != g_tknPtr->sParm.v.vParent)
                      {
                        if (g_tknPtr->sParm.v.vParent->sParm.t.tSubType == sSCALAR ||
                            g_tknPtr->sParm.v.vParent->sParm.t.tSubType != s->setType)
                          {
                            error(eSET);
                          }

                        if (!s->typePtr)
                          {
                            s->typeFound = true;
                            s->typePtr   = g_tknPtr->sParm.v.vParent;
                            s->setType   = s->typePtr->sParm.t.tSubType;
                            s->minValue  = s->typePtr->sParm.t.tMinValue;
                            s->maxValue  = s->typePtr->sParm.t.tMaxValue;
                          }
                      }

                  addVarToBits:
                    /* Verify that the first value is in range */

                    if (firstValue < s->minValue)
                      {
                        error(eSETRANGE);
                        firstValue = s->minValue;
                      }
                    else if (firstValue > s->maxValue)
                      {
                        error(eSETRANGE);
                        firstValue = s->maxValue;
                      }

                    /* Set all bits from firstValue through maxValue */

                    setValue  = (0xffff >> ((BITS_IN_INTEGER -1 ) -
                                            (s->maxValue - s->minValue)));
                    setValue &= (0xffff << (firstValue - s->minValue));

                    /* Generate run-time logic to get all bits from firstValue
                     * through last value, i.e., need to generate logic to get:
                     * 0xffff >> ((BITS_IN_INTEGER - 1) - (lastValue - minValue))
                     */

                    pas_GenerateDataOperation(opPUSH, 0xffff);
                    pas_GenerateDataOperation(opPUSH, ((BITS_IN_INTEGER - 1) +
                                                        s->minValue));
                    pas_GenerateStackReference(opLDS, g_tknPtr);
                    pas_GenerateSimple(opSUB);
                    pas_GenerateSimple(opSRL);

                    /* Then AND this with the setValue */

                    if (setValue != 0xffff)
                      {
                        pas_GenerateDataOperation(opPUSH, setValue);
                        pas_GenerateSimple(opAND);
                      }

                    getToken();
                    break;

                  default :
                    error(eSET);
                    pas_GenerateDataOperation(opPUSH, 0);
                    break;
              }
          }
          break;

      case sSCALAR :
        if (s->typeFound)
          {
            if ((!s->typePtr) || (s->typePtr != g_tknPtr->sParm.v.vParent))
              {
               error(eSET);
              }
          }
        else
          {
            s->typeFound = true;
            s->typePtr   = g_tknPtr->sParm.v.vParent;
            s->setType   = sSCALAR;
            s->minValue  = s->typePtr->sParm.t.tMinValue;
            s->maxValue  = s->typePtr->sParm.t.tMaxValue;
          }

        goto addVar;

      case sINT : /* An integer subrange variable ? */
      case sCHAR : /* A character subrange variable? */
        if (!s->typeFound)
          {
            s->typeFound = true;
            s->setType   = g_token;
          }
        else if (s->setType != g_token)
          {
            error(eSET);
          }

         goto addVar;

      case sSUBRANGE :
        if (s->typeFound)
          {
             if ((!s->typePtr) || (s->typePtr != g_tknPtr->sParm.v.vParent))
               {
                 error(eSET);
               }
          }
        else
          {
            s->typeFound = true;
            s->typePtr   = g_tknPtr->sParm.v.vParent;
            s->setType   = s->typePtr->sParm.t.tSubType;
            s->minValue  = s->typePtr->sParm.t.tMinValue;
            s->maxValue  = s->typePtr->sParm.t.tMaxValue;
          }

      addVar:
        /* Check if the variable set element is the first value in a */
        /* subrange of values */

        setPtr = g_tknPtr;
        getToken();
        if (g_token != tSUBRANGE)
          {
            /* Generate P-Code to push the set value onto the stack */
            /* FORM:  1 << (firstValue - minValue) */

            pas_GenerateDataOperation(opPUSH, 1);
            pas_GenerateStackReference(opLDS, setPtr);
            pas_GenerateDataOperation(opPUSH, s->minValue);
            pas_GenerateSimple(opSUB);
            pas_GenerateSimple(opSLL);
          }
        else
          {
            if (!s->typeFound) error(eSUBRANGETYPE);

            /* Skip over the tSUBRANGE token */

            getToken();

            /* TYPE check */

            switch (g_token)
              {
                case sSCALAR_OBJECT : /* A scalar or scalar subrange constant */
                  lastValue = g_tknPtr->sParm.c.val.i;
                  if (s->setType != sSCALAR ||
                      s->typePtr != g_tknPtr->sParm.c.parent)
                    {
                      error(eSET);
                    }

                  goto addBitsToVar;

                case tINT_CONST : /* An integer subrange constant ? */
                  lastValue = g_tknInt;
                  if (s->setType != sINT) error(eSET);
                  goto addBitsToVar;

                case tCHAR_CONST : /* A character subrange constant */
                  lastValue = g_tknInt;
                  if (s->setType != sCHAR) error(eSET);

                addBitsToVar :
                  /* Verify that the last value is in range */

                  if (lastValue < s->minValue)
                    {
                      error(eSETRANGE);
                      lastValue = s->minValue;
                    }
                  else if (lastValue > s->maxValue)
                    {
                      error(eSETRANGE);
                      lastValue = s->maxValue;
                    }

                  /* Set all bits from minValue through lastValue */

                  setValue  = (0xffff >> ((BITS_IN_INTEGER - 1) -
                                        (lastValue - s->minValue)));

                  /* Now, generate P-Code to push the set value onto the stack */
                  /* First generate: 0xffff << (firstValue - minValue) */

                  pas_GenerateDataOperation(opPUSH, 0xffff);
                  pas_GenerateStackReference(opLDS, setPtr);
                  if (s->minValue)
                    {
                      pas_GenerateDataOperation(opPUSH, s->minValue);
                      pas_GenerateSimple(opSUB);
                   }

                   pas_GenerateSimple(opSLL);

                  /* Then and this with the pre-computed constant set value */

                  if (setValue != 0xffff)
                    {
                      pas_GenerateDataOperation(opPUSH, setValue);
                      pas_GenerateSimple(opAND);
                    }

                  getToken();
                  break;

                case sINT : /* An integer subrange variable ? */
                case sCHAR : /* A character subrange variable? */
                  if (s->setType != g_token) error(eSET);
                  goto addVarToVar;

                case sSCALAR :
                  if (s->typePtr != g_tknPtr->sParm.v.vParent) error(eSET);
                  goto addVarToVar;

                case sSUBRANGE :
                  if (s->typePtr != g_tknPtr->sParm.v.vParent &&
                     (g_tknPtr->sParm.v.vParent->sParm.t.tSubType == sSCALAR ||
                      g_tknPtr->sParm.v.vParent->sParm.t.tSubType != s->setType))
                    {
                      error(eSET);
                    }

                addVarToVar:

                  /* Generate run-time logic to get all bits from firstValue */
                  /* through lastValue */
                  /* First generate: 0xffff << (firstValue - minValue) */

                  pas_GenerateDataOperation(opPUSH, 0xffff);
                  pas_GenerateStackReference(opLDS, setPtr);
                  if (s->minValue)
                    {
                      pas_GenerateDataOperation(opPUSH, s->minValue);
                      pas_GenerateSimple(opSUB);
                    }

                  pas_GenerateSimple(opSLL);

                  /* Generate logic to get: */
                  /* 0xffff >> ((BITS_IN_INTEGER-1)-(lastValue - minValue)) */

                  pas_GenerateDataOperation(opPUSH, 0xffff);
                  pas_GenerateDataOperation(opPUSH, ((BITS_IN_INTEGER - 1) + s->minValue));
                  pas_GenerateStackReference(opLDS, g_tknPtr);
                  pas_GenerateSimple(opSUB);
                  pas_GenerateSimple(opSRL);

                  /* Then AND the two values */

                  pas_GenerateSimple(opAND);

                  getToken();
                  break;

                default :
                  error(eSET);
                  pas_GenerateDataOperation(opPUSH, 0);
                  break;
              }
          }
          break;

      default :
        error(eSET);
        pas_GenerateDataOperation(opPUSH, 0);
        break;
    }
}

/****************************************************************************/

/* Check if this is a ordinal type.  This is what is needed, for
 * example, as an argument to ord(), pred(), succ(), or odd().
 * This is the kind of expression we need in a CASE statement
 * as well.
 */

static bool pas_IsOrdinalType(exprType_t testExprType)
{
  if (testExprType == exprInteger || /* integer value */
      testExprType == exprChar ||    /* character value */
      testExprType == exprBoolean || /* boolean(integer) value */
      testExprType == exprScalar)    /* scalar(integer) value */
    {
      return true;
    }
  else
    {
      return false;
    }
}

/****************************************************************************/
/* This is a hack to handle calls to system functions that return
 * exprCString pointers that must be converted to exprString
 * records upon assignment.
 */

static bool pas_IsAnyStringType(exprType_t testExprType)
{
  if (testExprType == exprString ||
      testExprType == exprCString)
    {
      return true;
    }
  else
    {
      return false;
    }
}

static bool  pas_IsStringReference (exprType_t testExprType)
{
  if (testExprType == exprString)
    {
      return true;
    }
  else
    {
      return false;
    }
}
