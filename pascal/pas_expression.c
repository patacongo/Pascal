/****************************************************************************
 * pas_expression.c
 * Integer Expression
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
#include "pas_pcode.h"      /* P-code operation codes */
#include "pas_longops.h"    /* Long integer operation codes */
#include "pas_fpops.h"      /* floating point operations */
#include "pas_setops.h"     /* set operators */
#include "pas_stringlib.h"  /* library operations */
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
 * Private Type Definitions
 ****************************************************************************/

/* This structure holds a writable copy of a symbol table variable plus
 * additional information to help with evaluation of expressions.
 */

struct varInfo_s
{
  symbol_t  variable;   /* Writable copy of symbol table variable entry */
  int16_t   ptrDepth;   /* 0=value; 1=pointer, 2=pointer-to-pointer, etc. */
  int16_t   fOffset;    /* Record field offset into variable */
};

typedef struct varInfo_s varInfo_t;

/* Used for dealing with LongInteger and LongWord types */

union uWord_u
{
  int32_t  sData;
  uint32_t uData;
  uint16_t word[2];
};

typedef union uWord_u uWord_t;

/* Identifies valid OpCodes for each expression type */

struct exprOpcodes_s
{
  enum pcode_e   intOpCode;
  enum pcode_e   wordOpCode;
  enum pcode_e   ptrOpCode;
  enum pcode_e   charOpCode;
  enum pcode_e   boolOpCode;
  enum longops_e longIntOpCode;
  enum longops_e longWordOpCode;
  uint16_t       fpOpCode;
  uint16_t       strOpCode;
  uint16_t       setOpCode;
};

typedef struct exprOpcodes_s exprOpCodes_t;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static exprType_t pas_SimpleExpression(exprType_t findExprType);
static exprType_t pas_Term(exprType_t findExprType);
static exprType_t pas_Factor(exprType_t findExprType);
static exprType_t pas_ComplexFactor(void);
static exprType_t pas_SimpleFactor(varInfo_t *varInfo,
                    exprFlag_t factorFlags);
static exprType_t pas_BaseFactor(varInfo_t *varInfo, exprFlag_t factorFlags);
static exprType_t pas_PointerFactor(void);
static exprType_t pas_ComplexPointerFactor(exprFlag_t factorFlags);
static exprType_t pas_SimplePointerFactor(varInfo_t *varInfo,
                    exprFlag_t factorFlags);
static exprType_t pas_ArrayPointerFactor(varInfo_t *varInfo,
                    exprFlag_t factorFlags);
static exprType_t pas_BasePointerFactor(symbol_t *varPtr,
                    exprFlag_t factorFlags);
static exprType_t pas_FunctionDesignator(void);
static exprType_t pas_FactorExprType(exprType_t baseExprType,
                                     uint8_t factorFlags);
static void       pas_SetAbstractType(symbol_t *sType);
static exprType_t pas_GetSetFactor(void);
static bool       pas_GetSubSet(symbol_t *setTypePtr, bool first);
static exprType_t pas_TypeCast(symbol_t *typePtr);
static bool       pas_IsOrdinalExpression(exprType_t testExprType);

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* The abstract types - SETs, RECORDS, etc - require an exact
 * match in type.  This variable points to the symbol table
 * sTYPE entry associated with the expression.
 */

symbol_t *g_abstractTypePtr;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************/
/* Process Simple Expression */

static exprType_t pas_SimpleExpression(exprType_t findExprType)
{
  int16_t    operation = '+';
  uint16_t   arg8FpBits;
  exprType_t term1Type;
  exprType_t term2Type;

  /* FORM: [+|-] <term> [{+|-} <term> [{+|-} <term> [...]]]
   *
   * Get +/- unary operation
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
      if (term1Type == exprInteger      || term1Type == exprWord     ||
          term1Type == exprShortInteger || term1Type == exprShortWord)
        {
          pas_GenerateSimple(opNEG);
        }
      else if (term1Type == exprLongInteger || term1Type == exprLongWord)
        {
          pas_GenerateSimpleLongOperation(opDNEG);
        }
      else if (term1Type == exprReal)
        {
          pas_GenerateFpOperation(fpNEG);
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

  /* If the findExprType is exprUnknown, then force the findExprType to be
   * the same as the first term.  At least the two terms should agree.
   */

  if (findExprType == exprUnknown)
    {
      findExprType = term1Type;
    }

  /* Process subsequent (optional) terms and binary operations */

  for (; ; )
    {
      /* Check for binary operator at this level of precedence:
       *
       *   +, -, or, xor, ><
       */

      if (g_token == '+'  || g_token == '-' || g_token == tOR ||
          g_token == tXOR || g_token == tSYMDIFF || g_token == '|')
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

      if (operation == '+')
        {
          if (term1Type == exprString)
            {
              /* Duplicate the sting on the string stack. */

              pas_StringLibraryCall(lbSTRDUP);
            }

          /* If we are going to add something to a char, then the result
           * must be a string.  We will similarly have to convert the
           * character to a string.
           */

          else if (term1Type == exprChar)
            {
              /* Expand the character to a string on the string stack.  And
               * change the expression type to reflect this.
               */

              pas_StringLibraryCall(lbMKSTKC);
              term1Type = exprString;
            }
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

      if (term1Type != exprString)
        {
          /* Handle the case where the type of the terms differ. */

          if (term1Type != term2Type)
            {
              /* Handle the case where the 1st argument is REAL and the
               * second is INTEGER.
               *
               * REVISIT:  Need to deal will conversion of LongIntegers as well.
               */

              if (term1Type == exprReal && term2Type == exprInteger)
                {
                  arg8FpBits = fpARG2;
                  term2Type  = exprReal;
                }

              /* Handle the case where the 1st argument is Integer and the
               * second is REAL.
               *
               * REVISIT:  Need to deal will conversion of LongIntegers as well.
               */

              else if (term1Type == exprInteger && term2Type == exprReal)
                {
                  arg8FpBits = fpARG1;
                  term1Type = exprReal;
                }

              /* Allow automatic integer conversions if the integer types have
               * same size in the stack representation.  The type of TERM1 wins
               * arbitrarily.
               */

              else if ((term1Type == exprInteger      || term1Type == exprWord ||
                        term1Type == exprShortInteger || term1Type == exprShortWord) &&
                       (term2Type == exprInteger      || term2Type == exprWord ||
                        term2Type == exprShortInteger || term2Type == exprShortWord))
                {
                  term2Type = term1Type;
                }
              else if ((term1Type == exprLongInteger || term1Type == exprLongWord) &&
                       (term2Type == exprLongInteger || term2Type == exprLongWord))
                {
                  term2Type = term1Type;
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
            case exprWord :
            case exprShortInteger :
            case exprShortWord :
              pas_GenerateSimple(opADD);
              break;

            case exprLongInteger :
            case exprLongWord :
              pas_GenerateSimpleLongOperation(opDADD);
              break;

              /* Floating point addition */

            case exprReal :
              pas_GenerateFpOperation(fpADD | arg8FpBits);
              break;

              /* Set 'addition' */

            case exprSet :
            case exprEmptySet :
              pas_GenerateSetOperation(setUNION);
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

                  pas_StringLibraryCall(lbSTRCAT);
                }
              else if (term2Type == exprChar)
                {
                  /* We are concatenating a character to the end of a string */

                  pas_StringLibraryCall(lbSTRCATC);
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

          if (term1Type == exprInteger      || term1Type == exprWord ||
              term1Type == exprShortInteger || term1Type == exprShortWord)
            {
              pas_GenerateSimple(opSUB);
            }
          else if (term1Type == exprLongInteger || term1Type == exprLongWord)
            {
              pas_GenerateSimpleLongOperation(opDSUB);
            }

           /* Floating point subtraction */

          else if (term1Type == exprReal)
            {
              pas_GenerateFpOperation(fpSUB | arg8FpBits);
            }

          /* Set 'subtraction' */

          else if (term1Type == exprSet ||
                   term1Type == exprEmptySet)
            {
              pas_GenerateSetOperation(setDIFFERENCE);
            }

          /* Otherwise, the '-' operation is not permitted */

          else
            {
              error(eTERMTYPE);
            }
          break;

        case '|' :
        case tOR :
          /* Integer/boolean 'OR' */

          if (term1Type == exprInteger      || term1Type == exprWord      ||
              term1Type == exprShortInteger || term1Type == exprShortWord ||
              term1Type == exprBoolean)
            {
              pas_GenerateSimple(opOR);
            }
          else if (term1Type == exprLongInteger || term1Type == exprLongWord)
            {
              pas_GenerateSimpleLongOperation(opDOR);
            }

          /* Otherwise, the 'OR' operation is not permitted */

          else
            {
              error(eTERMTYPE);
            }
          break;

        case tXOR :
          /* Integer/boolean 'XOR' */

          if (term1Type == exprInteger      || term1Type == exprWord      ||
              term1Type == exprShortInteger || term1Type == exprShortWord ||
              term1Type == exprBoolean)
            {
              pas_GenerateSimple(opXOR);
            }
          else if (term1Type == exprLongInteger || term1Type == exprLongWord)
            {
              pas_GenerateSimpleLongOperation(opDXOR);
            }

          /* Otherwise, the 'XOR' operation is not permitted */

          else
            {
              error(eTERMTYPE);
            }
          break;

        case tSYMDIFF :
          /* Set symmetric difference */

          if (term1Type == exprSet || term1Type == exprEmptySet)
            {
              pas_GenerateSetOperation(setSYMMETRICDIFF);
            }

          /* Otherwise, the 'OR' operation is not permitted */

          else
            {
              error(eTERMTYPE);
            }
          break;

        default :
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

  /* FORM:  <factor> [<operator> <factor>[<operator><factor>[...]]] */

  factor1Type = pas_Factor(findExprType);
  for (; ; )
    {
      /* Check for binary operator at this level of precedence:
       *
       *   *, /, div, mod, and, shl, shr, as, <<, >>, &
       */

      if (g_token == tMUL  || g_token == tDIV  || g_token == tFDIV ||
          g_token == tMOD  || g_token == tAND  || g_token == tSHL  ||
          g_token == tSHR  || g_token == '&')
        {
          operation = g_token;
        }
      else
        {
          break;
        }

      /* Get the next factor.  The shift instructions are different in that
       * there is an asymmetry:  The type of the shift value is a 16-bit
       * ordinal type regardless of what is being shifted.
       */

      getToken();

      if (operation == tSHL || operation == tSHR)
        {
          factor2Type = pas_Factor(exprAnyOrdinal);

          /* All ordinals are 16-bits in width (at least on the stack) except
           * for the long integer types.
           */

          if (factor2Type == exprLongInteger || factor2Type == exprLongWord ||
              factor2Type == exprUnknown)
            {
              error(eSHIFTTYPE);
              factor2Type = exprInteger;
            }
        }
      else
        {
          /* Otherwise, the type of the second factor should, generally, match
           * the type of the first.
           */

          factor2Type = pas_Factor(findExprType);
        }

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
           *
           * REVISIT:  Need to deal will conversion of LongIntegers as well.
           */

          if (factor1Type == exprReal && factor2Type == exprInteger)
            {
              arg8FpBits = fpARG2;
            }

          /* Handle the case where the 1st argument is Integer and the
           * second is REAL.
           *
           * REVISIT:  Need to deal will conversion of LongIntegers as well.
           */

          else if (factor1Type == exprInteger && factor2Type == exprReal)
            {
              arg8FpBits = fpARG1;
              factor1Type = exprReal;
            }

          /* Otherwise, the two factors must agree in type (except for the
           * case of the SHIFT instructions as described above.
           */

          else if (operation == tSHL || operation == tSHR)
            {
              error(eFACTORTYPE);
            }
        }

    /* Handle the cases for conversions when the two string types are the
     * same type.
     */

      else
        {
          /* There is only one interesting case:  When the
           * expected expression is real and both arguments are
           * integer.  In this case, for example, 1/2 must yield
           * 0.5, not 0.
           *
           * REVISIT:  Need to deal will conversion of LongIntegers as well.
           */

          if ((factor1Type == exprInteger || factor1Type == exprShortInteger) &&
              findExprType == exprReal)
            {
              /* However, we will perform this conversin only for the
               * arithmetic operations: tMUL, tDIV/tFDIV, and tMOD.
               * The logical operations must be performed on integer
               * types with the result converted to a real type afterward.
               */

              if (operation == tMUL  || operation == tDIV  ||
                  operation == tFDIV || operation == tMOD)
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
          if (factor1Type == exprInteger || factor1Type == exprShortInteger)
            {
              pas_GenerateSimple(opMUL);
            }
          else if (factor1Type == exprWord || factor1Type == exprShortWord)
            {
              pas_GenerateSimple(opUMUL);
            }
          else if (factor1Type == exprLongInteger)
            {
              pas_GenerateSimpleLongOperation(opDMUL);
            }
          else if (factor1Type == exprLongWord)
            {
              pas_GenerateSimpleLongOperation(opDUMUL);
            }
          else if (factor1Type == exprReal)
            {
              pas_GenerateFpOperation(fpMUL | arg8FpBits);
            }
          else if (factor1Type == exprSet ||
                   factor1Type == exprEmptySet)
            {
              pas_GenerateSetOperation(setINTERSECTION);
            }
          else
            {
              error(eFACTORTYPE);
            }
          break;

        case tDIV :
          if (factor1Type == exprInteger || factor1Type == exprShortInteger)
            {
              pas_GenerateSimple(opDIV);
            }
          else if (factor1Type == exprWord || factor1Type == exprShortWord)
            {
              pas_GenerateSimple(opUDIV);
            }
          else if (factor1Type == exprLongInteger)
            {
              pas_GenerateSimpleLongOperation(opDDIV);
            }
          else if (factor1Type == exprLongWord)
            {
              pas_GenerateSimpleLongOperation(opDUDIV);
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
          if (factor1Type == exprInteger || factor1Type == exprShortInteger)
            {
              pas_GenerateSimple(opMOD);
            }
          else if (factor1Type == exprWord || factor1Type == exprShortWord)
            {
              pas_GenerateSimple(opUMOD);
            }
          else if (factor1Type == exprLongInteger)
            {
              pas_GenerateSimpleLongOperation(opDMOD);
            }
          else if (factor1Type == exprLongWord)
            {
              pas_GenerateSimpleLongOperation(opDUMOD);
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

        case '&'  :
        case tAND :
          if (factor1Type == exprInteger      || factor1Type == exprWord      ||
              factor1Type == exprShortInteger || factor1Type == exprShortWord ||
              factor1Type == exprBoolean)
            {
              pas_GenerateSimple(opAND);
            }
          else if (factor1Type == exprLongInteger || factor1Type == exprLongWord)
            {
              pas_GenerateSimpleLongOperation(opDAND);
            }
          else
            {
              error(eFACTORTYPE);
            }
          break;

        case tSHL :
          if (factor1Type == exprInteger || factor1Type == exprWord ||
              factor1Type == exprShortInteger || factor1Type == exprShortWord)
            {
              pas_GenerateSimple(opSLL);
            }
         else  if (factor1Type == exprLongInteger || factor1Type == exprLongWord)
            {
              pas_GenerateSimpleLongOperation(opDSLL);
            }
          else
            {
              error(eFACTORTYPE);
            }
          break;

        case tSHR :
          if (factor1Type == exprInteger || factor1Type == exprShortInteger)
            {
              pas_GenerateSimple(opSRA);
            }
          else if (factor1Type == exprWord || factor1Type == exprShortWord)
            {
              pas_GenerateSimple(opSRL);
            }
         else if (factor1Type == exprLongInteger)
            {
              pas_GenerateSimpleLongOperation(opDSRA);
            }
         else if (factor1Type == exprLongWord)
            {
              pas_GenerateSimpleLongOperation(opDSRL);
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
      {
        uWord_t lword;

        switch (findExprType)
          {
            /* Check if the caller asked for long integer type. */

            case exprLongInteger :      /* signed short integer value */
            case exprLongWord :         /* unsigned short integer value */
              lword.uData = g_tknUInt;  /* g_tknUInt is unsigned */
              pas_GenerateDataOperation(opPUSH, lword.word[0]);
              pas_GenerateDataOperation(opPUSH, lword.word[1]);
              factorType = findExprType;
              break;

            default :
              if ((g_tknUInt & ~0xffff) == 0)  /* g_tknUInt is unsigned */
                {
                  pas_GenerateDataOperation(opPUSH, g_tknUInt);

                  if ((g_tknUInt & ~0xff) == 0 && findExprType == exprShortInteger)
                    {
                      factorType = exprShortInteger;
                    }
                  else if ((uint16_t)g_tknUInt <= MAXWORD &&
                            findExprType == exprShortWord)
                    {
                      factorType = exprShortWord;
                    }
                  else if ((uint16_t)g_tknUInt > MAXWORD &&
                           (findExprType == exprLongWord ||
                            findExprType == exprLongInteger))
                    {
                      factorType = exprShortWord;
                    }
                  else
                    {
                      factorType = exprInteger;
                    }
                }

              /* Allow out-of-range, non-negative integer constants to
               * automatically cast to unsigned integer type.
               */

              else if (g_tknUInt <= MAXWORD)
                {
                  pas_GenerateDataOperation(opPUSH, g_tknUInt);
                  factorType = exprWord;
                }

              /* Allow out-of-range integer constants to automatically cast
               * to long integer type.
               */

              else /* if (g_tknUInt >= MINLONGINT && g_tknUInt <= MAXLONGINT) */
                {
                  lword.uData = g_tknUInt;
                  pas_GenerateDataOperation(opPUSH, lword.word[0]);
                  pas_GenerateDataOperation(opPUSH, lword.word[1]);
                  factorType = exprLongInteger;
                }
              break;
          }

        /* Skip over the constant */

        getToken();
      }
      break;

    case tBOOLEAN_CONST :
      pas_GenerateDataOperation(opPUSH, g_tknUInt);
      getToken();
      factorType = exprBoolean;
      break;

    case tCHAR_CONST :
      pas_GenerateDataOperation(opPUSH, g_tknUInt);
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
      if (g_abstractTypePtr != NULL)
        {
          if (g_tknPtr->sParm.c.cParent != g_abstractTypePtr) error(eSCALARTYPE);
        }
      else
        {
         g_abstractTypePtr = g_tknPtr->sParm.c.cParent;
        }

      pas_GenerateDataOperation(opPUSH, g_tknPtr->sParm.c.cValue.i);
      getToken();
      factorType = exprScalar;
      break;

      /* Simple Factors */

    case sINT :
    case sWORD :
    case sBOOLEAN :
      factorType = pas_MapVariable2ExprType(g_token, true);
      pas_GenerateStackReference(opLDS, g_tknPtr);
      getToken();
      break;

    case sSHORTINT :
      pas_GenerateStackReference(opLDSB, g_tknPtr);
      getToken();
      factorType = exprShortInteger;
      break;

    case sSHORTWORD :
    case sCHAR :
      factorType = pas_MapVariable2ExprType(g_token, true);
      pas_GenerateStackReference(opULDSB, g_tknPtr);
      getToken();
      break;

    case sLONGINT :
    case sLONGWORD :
    case sREAL :
      factorType = pas_MapVariable2ExprType(g_token, (g_token != sREAL));
      pas_GenerateDataSize(g_tknPtr->sParm.v.vSize);
      pas_GenerateStackReference(opLDSM, g_tknPtr);
      getToken();
      break;

      /* Strings -- constant and variable */

    case tSTRING_CONST :
      {
        /* Final run-time stack representation is:
         *
         *   TOS(0) : Fake buffer allocation size
         *   TOS(1) : Pointer to string to be copied
         *   TOS(2) : Size of the string in bytes
         *
         * Add the string to the RO data section of the output
         * and get the offset to the string location.
         */

        uint32_t offset = poffAddRoDataString(g_poffHandle, g_tokenString);

        /* Push the offset then size of the string on the stack */

        pas_GenerateDataOperation(opPUSH, strlen(g_tokenString));
        pas_GenerateDataOperation(opLAC, offset);
        pas_GenerateDataOperation(opPUSH, strlen(g_tokenString));

        /* And copy the string to string memory.  NOTE:  In many cases this
         * STRDUP is not necessary.  It is not necessary when the string is
         * not modified.  Logic in the optimizer will attempt to identifier
         * such cases when the STRDUP is not needed and remove it.
         */

        pas_StringLibraryCall(lbSTRDUP);

        /* Release the tokenized string */

        g_stringSP = g_tokenString;
        getToken();
        factorType = exprString;
      }
      break;

    case sSTRING_CONST :
      /* Final stack representation is:
       *
       *   TOS(0) : Fake buffer alloction size
       *   TOS(1) : Pointer to string
       *   TOS(2) : Size of string in bytes
       */

      pas_GenerateDataOperation(opPUSH, g_tknPtr->sParm.s.roSize);
      pas_GenerateDataOperation(opLAC, g_tknPtr->sParm.s.roOffset);
      pas_GenerateDataOperation(opPUSH, g_tknPtr->sParm.s.roSize);
      getToken();
      factorType = exprString;
      break;

    case sSTRING :
      /* Stack representation for sSTRING is:
       *
       *   TOS(0) = Size of the allocated string buffer
       *   TOS(1) = Pointer to string data
       *   TOS(2) = Length of the string in bytes
       */

      pas_GenerateDataSize(g_tknPtr->sParm.v.vSize);
      pas_GenerateStackReference(opLDSM, g_tknPtr);

      factorType = pas_MapVariable2ExprType(g_token, false);
      getToken();
      break;

    case sSCALAR :
      if (g_abstractTypePtr != NULL)
        {
          if (g_tknPtr->sParm.v.vParent != g_abstractTypePtr) error(eSCALARTYPE);
        }
      else
        {
          g_abstractTypePtr = g_tknPtr->sParm.v.vParent;
        }

      pas_GenerateStackReference(opLDS, g_tknPtr);
      getToken();
      factorType = exprScalar;
      break;

    case sSET :
      /* If an g_abstractTypePtr is specified then it should either be the
       * same SET OF <object> -OR- the same <object>
       */

      if (g_abstractTypePtr != NULL)
        {
          if ((g_tknPtr->sParm.v.vParent != g_abstractTypePtr) &&
              (g_tknPtr->sParm.v.vParent->sParm.t.tParent != g_abstractTypePtr))
            {
              error(eSET);
            }
        }
      else
        {
          g_abstractTypePtr = g_tknPtr->sParm.v.vParent;
        }

      pas_GenerateDataSize(g_tknPtr->sParm.v.vSize);
      pas_GenerateStackReference(opLDSM, g_tknPtr);
      getToken();
      factorType = exprSet;
      break;

      /* SET factors */

    case '[' : /* Set constant */
      getToken();
      factorType = pas_GetSetFactor();
      if (g_token != ']') error(eRBRACKET);
      else getToken();
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
      factorType = pas_Expression(exprUnknown, g_abstractTypePtr);
      if (g_token == ')') getToken();
      else error (eRPAREN);
      break;

      /* Address references */

    case '^' :
    case tNIL :
      factorType = pas_PointerFactor();
      break;

      /* Highest Priority Operators */

    case '@':
      /* The address operator @ returns the address of a variable, procedure
       * or function.
       *
       * Verify that the expression expects a pointer type.
       */

      if (!IS_POINTER_EXPRTYPE(findExprType) &&
          findExprType != exprAnyPointer)
        {
          error(ePOINTERTYPE);
        }

      /* Then handle the pointer factor */

      getToken();
      factorType = pas_ComplexPointerFactor(FACTOR_PTREXPR);
      break;

    case '~' :
    case tNOT :
      getToken();
      factorType = pas_Factor(findExprType);
      if (factorType != exprInteger      && factorType != exprWord      &&
          factorType != exprShortInteger && factorType != exprShortWord &&
          factorType != exprLongInteger  && factorType != exprLongWord &&
          factorType != exprBoolean)
        {
          error(eFACTORTYPE);
        }

      if (factorType == exprLongInteger || factorType == exprLongWord)
        {
          pas_GenerateSimpleLongOperation(opDNOT);
        }
      else
        {
          pas_GenerateSimple(opNOT);
        }
      break;

      /* Standard or Built-in function? */

    case tSTDFUNC:
      factorType = pas_StandardFunction();
      break;

    /* Type case? */

    case sTYPE :
      factorType = pas_TypeCast(g_tknPtr);
      break;

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

  /* First, make a copy of the symbol table entry because the call to
   * pas_SimpleFactor() will modify it.
   */

  if (g_tknPtr == NULL)
    {
      error(eEXPRTYPE);
      return exprUnknown;
    }
  else
    {
      varInfo.variable = *g_tknPtr;
      varInfo.ptrDepth = 0;
      varInfo.fOffset  = 0;

      /* Since we have saved the token information, we can skip to the next
       * token.
       */

      getToken();

      /* Then process the complex factor until it is reduced to a simple
       * factor (like int, char, etc.)
       */

      return pas_SimpleFactor(&varInfo, 0);
    }
}

/****************************************************************************/
/* Process a complex factor (recursively) until it becomes a simple factor */

static exprType_t pas_SimpleFactor(varInfo_t *varInfo,
                                   exprFlag_t factorFlags)
{
  symbol_t  *varPtr = &varInfo->variable;
  symbol_t  *typePtr;
  symbol_t  *baseTypePtr;
  exprType_t factorType;
  uint16_t   arrayKind;

  /* Check if it has been reduced to a simple factor. */

  factorType = pas_BaseFactor(varInfo, factorFlags);
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
      if (g_abstractTypePtr == NULL)
        {
          g_abstractTypePtr = typePtr;
        }

      varPtr->sKind = typePtr->sParm.t.tSubType;
      factorType    = pas_SimpleFactor(varInfo, factorFlags);
      break;

    case sRECORD :
      /* Check if this is a pointer to a record.  Both pointer
       * expression and defererence can get set in some situations, for
       * example, when we are processing a pointer to a RECORD and the
       * field of the  RECORD is a pointer.
       */

      if ((factorFlags & FACTOR_PTREXPR) != 0 &&
          (factorFlags & FACTOR_DEREFERENCE) == 0)
        {
          if (g_token == '.') error(ePOINTERTYPE);

          if ((factorFlags & FACTOR_INDEXED) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
            }
          else if ((factorFlags & FACTOR_FIELD_OFFSET) != 0)
            {
              pas_GenerateDataOperation(opPUSH, varInfo->fOffset);
              pas_GenerateSimple(opADD);
              pas_GenerateSimple(opLDI);
            }
          else
            {
              pas_GenerateStackReference(opLDS, varPtr);
            }

          factorType = exprRecordPtr;
        }

      /* Verify that a period separates the RECORD identifier from the
       * record field identifier.
       */

      else if (g_token == '.')
        {
          /* Skip over the period. */

          getToken();

          /* Verify that a field identifier associated with this record.
           * follows the period.
           */

          baseTypePtr = pas_GetBaseTypePointer(typePtr);

          if (g_token != sRECORD_OBJECT ||
              g_tknPtr->sParm.r.rRecord != baseTypePtr)
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

              /* If there is no abstract type pointer, then set it now to
               * force any subsequent RECORD references to match.
               */

              if (g_abstractTypePtr == NULL)
                {
                  g_abstractTypePtr   = typePtr;
                }

              /* Adjust the variable size and offset.  Add the RECORD offset
               * to the RECORD data stack offset to get the data stack
               * offset to the record object; Change the size to match the
               * size of RECORD object.
               */

              varPtr->sParm.v.vSize   = g_tknPtr->sParm.r.rSize;

              if (factorFlags == (FACTOR_INDEXED | FACTOR_DEREFERENCE |
                                  FACTOR_VAR_PARM))
                {
                  /* Add the offset to the record field to the RECORD address
                   * that should already be on the stack.
                   */

                  pas_GenerateDataOperation(opPUSH, g_tknPtr->sParm.r.rOffset);
                  pas_GenerateSimple(opADD);
                }
              else if ((factorFlags & (FACTOR_DEREFERENCE | FACTOR_VAR_PARM)) != 0)
                {
                  /* Remember the offset to RECORD object to RECORD data stack
                   * offset so that we can apply it later.
                   */

                  varInfo->fOffset = g_tknPtr->sParm.r.rOffset;
                  factorFlags     |= FACTOR_FIELD_OFFSET;
                }
              else
                {
                  /* Add the offset to RECORD object to RECORD data stack
                   * offset.
                   */

                  varPtr->sParm.v.vOffset += g_tknPtr->sParm.r.rOffset;
                }

              getToken();
              factorType = pas_SimpleFactor(varInfo, factorFlags);
            }
        }

      /* A RECORD name may be a valid factor -- as the input parameter of a
       * function or in an assignment.
       */

      else if (g_abstractTypePtr == typePtr)
        {
          /* Special case:  The record is a VAR parameter. */

          if (factorFlags == (FACTOR_INDEXED | FACTOR_DEREFERENCE |
                              FACTOR_VAR_PARM))
            {
              /* The VAR parameter address is already on the stack */

              pas_GenerateSimple(opADD);
              pas_GenerateDataSize(varPtr->sParm.v.vSize);
              pas_GenerateSimple(opLDIM);
              factorType = exprRecord;
            }
          else if ((factorFlags & (FACTOR_DEREFERENCE | FACTOR_VAR_PARM)) != 0 &&
                   (factorFlags & FACTOR_FIELD_OFFSET) != 0)
            {
              uint16_t baseType;

              baseTypePtr = pas_GetBaseTypePointer(typePtr);
              baseType    = baseTypePtr->sParm.t.tType;

              /* The RECORD pointer should already be on the stack.  Now we
               * need to add the field offset to the RECORD address and load
               * the field.
               */

              pas_GenerateDataSize(varInfo->fOffset);
              pas_GenerateSimple(opADD);

              switch (typePtr->sParm.t.tAllocSize)
                {
                  case sCHAR_SIZE :
                    if (baseType == sSHORTINT)
                      {
                        pas_GenerateSimple(opLDIB);  /* Sign-extend */
                      }
                    else
                      {
                        pas_GenerateSimple(opULDIB); /* No sign extension */
                      }
                    break;

                  case sINT_SIZE :
                    pas_GenerateSimple(opLDI);
                    break;

                  default :
                    pas_GenerateDataOperation(opPUSH,
                                              baseTypePtr->sParm.t.tAllocSize);
                    pas_GenerateSimple(opLDIM);
                    break;
                }

              if (typePtr->sParm.t.tType == sPOINTER)
                {
                  factorType = pas_MapVariable2ExprPtrType(baseType, false);
                }
              else
                {
                  factorType = pas_MapVariable2ExprType(baseType, false);
                }
            }
          else
            {
              pas_GenerateDataSize(varPtr->sParm.v.vSize);
              pas_GenerateStackReference(opLDSM, varPtr);
              factorType = exprRecord;
            }
        }
      else
        {
          error(ePERIOD);
        }
      break;

    case sRECORD_OBJECT :
      /* NOTE:  This must have been preceeded with a WITH statement
       * defining the RECORD type
       */

      if (!g_withRecord.wParent)
        {
          error(eINVTYPE);
        }
      else if ((factorFlags && (FACTOR_DEREFERENCE | FACTOR_PTREXPR)) != 0)
        {
          error(ePOINTERTYPE);
        }
      else if ((factorFlags && FACTOR_INDEXED) != 0)
        {
          error(eARRAYTYPE);
        }

      /* Verify that a field identifier is associated with the RECORD
       * specified by the WITH statement.
       */

      else if (varPtr->sParm.r.rRecord != g_withRecord.wParent)
        {
          error(eRECORDOBJECT);
        }
      else
        {
          int16_t tempOffset;

          /* Get the parent type and address offset of the record object */

          typePtr          = varPtr->sParm.r.rParent;
          varInfo->fOffset = varPtr->sParm.r.rOffset;

          /* A record object is a different member of the union than a
           * variable.  We will be calling functions below, pass varPtr
           * as a pointer to a variable instance.  Let's set all of the
           * unused variable fields to zero now so that they do not pick
           * up garbage when the variable is aliased to the record object.
           */

          varPtr->sParm.v.vFlags    = 0;
          varPtr->sParm.v.vXfrUnit  = 0;
          varPtr->sParm.v.vSymIndex = 0;

          /* Then let's set some fields that are common to both calls. */

          varPtr->sLevel            = g_withRecord.wLevel;
          varPtr->sParm.v.vParent   = typePtr;

          /* There are two cases to consider:  (1) the g_withRecord is a
           * pointer to a RECORD, or (2) the g_withRecord is the RECORD
           * itself.
           */

          if (g_withRecord.wPointer)
            {
              /* If the pointer is really a VAR parameter, then other syntax
               * rules will apply
               */

              if (g_withRecord.wVarParm)
                {
                  /* In this case g_withRecord.wLevel is the level of the
                   * procedure that receives the VAR parameterer, and
                   * g_withRecord.wOffset is the (negative) offset to the
                   * VAR parameter from the level of the procedure.
                   */

                  varPtr->sKind  = sVAR_PARM;
                  factorFlags   |= (FACTOR_DEREFERENCE | FACTOR_FIELD_OFFSET |
                                    FACTOR_VAR_PARM);
                }
              else
                {
                  /* In this case g_withRecord.wLevel is the level of the
                   * pointer variable, and  g_withRecord.wOffset is the
                   * (positive) offset to the pointer from that level.
                   */

                  varPtr->sKind  = sPOINTER;
                  factorFlags   |= (FACTOR_DEREFERENCE | FACTOR_FIELD_OFFSET);
                }

              /* Modify the variable so that it has the characteristics of the
               * the pointer.
               */

              varPtr->sParm.v.vOffset = g_withRecord.wOffset;
              varPtr->sParm.v.vSize   = sPTR_SIZE;

              pas_GenerateStackReference(opLDS, varPtr);

              factorFlags |= FACTOR_FIELD_OFFSET;
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
           */

          varPtr->sKind           = typePtr->sParm.t.tType;
          varPtr->sParm.v.vOffset = tempOffset;
          varPtr->sParm.v.vSize   = typePtr->sParm.t.tAllocSize;

          factorType = pas_SimpleFactor(varInfo, factorFlags);
        }
      break;

    case sPOINTER :
      {
        symbol_t *parentTypePtr = typePtr;
        int ptrDepth = 0;

        /* Get the pointer depth.  Pointer = 1, pointer-to-pointer = 2, etc. */

        while (parentTypePtr->sParm.t.tType == sPOINTER)
          {
            if (ptrDepth > 1)
              {
                error(eNOTYET);
              }

            ptrDepth++;
            parentTypePtr = parentTypePtr->sParm.t.tParent;
          }

        /* Are we dereferencing a pointer? */

        while (g_token == '^')
          {
            /* Since we are de-referencing, we can decrement the pointer depth. */

            if (ptrDepth > 0)
              {
                ptrDepth--;
              }
            else
              {
                error(ePOINTERDEREF);
              }

            /* In a sequence of record pointers like head^.link^.link, we must
             * explicitly load the first 'head' pointer variable.
             */

            if ((factorFlags & FACTOR_DEREFERENCE) == 0)
              {
                /* Load the address value of the pointer onto the stack now */

                pas_GenerateStackReference(opLDS, varPtr);
                factorFlags |= FACTOR_DEREFERENCE;
              }
            else
              {
                /* Load the value pointed at by the pointer value previously
                 * obtained with opLDS.
                 */

                pas_GenerateSimple(opLDI);
              }

            /* Skip over the '^' */

            getToken();
          }

        /* Now set the variable type to that of the parent and continue to
         * simplify the factor.
         */

        varPtr->sKind = parentTypePtr->sParm.t.tType;

        /* Is this going to be a pointer assignment?  Or a value? */

        if (ptrDepth > 0)
          {
            /* No more derererencing; we are assigning a pointer address. */

            factorFlags &= ~FACTOR_DEREFERENCE;
            factorFlags |= FACTOR_PTREXPR;
          }
        else
          {
            /* The size of the variable value is no longer the size of the
             * pointer, but rather it is now the full allocation size of the
             * parent type.
             */

            varPtr->sParm.v.vSize = parentTypePtr->sParm.t.tAllocSize;
          }

        factorType = pas_SimpleFactor(varInfo, factorFlags);

        /* Adjust the expression type if this is still a pointer */

        if (ptrDepth > 0)
          {
            factorType = MK_POINTER_EXPRTYPE(factorType);
          }
      }
      break;

    case sVAR_PARM :
      if ((factorFlags & (FACTOR_DEREFERENCE | FACTOR_LOAD_ADDRESS |
                          FACTOR_VAR_PARM)) != 0)
        {
          error(eVARPARMTYPE);
        }

      /* Load the address provided by the VAR parameter now */

      if ((factorFlags & FACTOR_DEREFERENCE) == 0)
        {
          pas_GenerateStackReference(opLDS, varPtr);
        }

      factorFlags          |= (FACTOR_DEREFERENCE | FACTOR_LOAD_ADDRESS |
                               FACTOR_VAR_PARM);

      varPtr->sKind         = typePtr->sParm.t.tType;
      varPtr->sParm.v.vSize = typePtr->sParm.t.tAllocSize;
      factorType            = pas_SimpleFactor(varInfo, factorFlags);
      break;

    case sARRAY :
      if ((factorFlags & FACTOR_INDEXED) != 0)
        {
          error(eARRAYTYPE);
        }

      /* Get a pointer to the underlying base type of the array */

      baseTypePtr = pas_GetBaseTypePointer(typePtr);

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
          factorFlags |= FACTOR_INDEXED;

          /* Generate the array offset calculation and indexed load */

          pas_ArrayIndex(typePtr);

          /* We have reduced this to a base type.  So we can generate the
           * indexed load from that base type.
           */

          varPtr->sKind  = arrayKind;
          factorType     = pas_SimpleFactor(varInfo, factorFlags);

          if (factorType == exprUnknown)
            {
              error(eHUH);  /* Should never happen */
            }

          /* Return the parent type of the array */

          varPtr->sKind         = typePtr->sParm.t.tType;
          varPtr->sParm.v.vSize = typePtr->sParm.t.tAllocSize;
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

          /* This could be either a simple packed array of char, or it
           * could a packed array of char field of a RECORD.
           */

          if ((factorFlags & (FACTOR_DEREFERENCE | FACTOR_VAR_PARM)) != 0)
            {
              uint16_t fieldOffset;

              /* The pointer for VAR parm address should already be on the
               * stack.  We need only to add the file offset.
               */

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

          /* Push the size and exchange the stack values to get them into
           * the expected order.
           */

          pas_GenerateDataOperation(opPUSH, varPtr->sParm.v.vSize);
          pas_GenerateSimple(opXCHG);

          pas_StringLibraryCall(lbBSTR2STR);
          factorType = exprString;
        }

      /* An ARRAY name may be a valid factor as the input parameter of
       * a function.
       */

      else if (g_abstractTypePtr == typePtr)
        {
          if ((factorFlags & (FACTOR_DEREFERENCE | FACTOR_VAR_PARM)) != 0)
            {
              pas_GenerateStackReference(opLAS, varPtr);
              factorType = pas_MapVariable2ExprPtrType(varPtr->sKind, false);
            }
          else
            {
              pas_GenerateDataSize(varPtr->sParm.v.vSize);
              pas_GenerateStackReference(opLDSM, varPtr);
              factorType = pas_MapVariable2ExprType(varPtr->sKind, false);
            }
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

/****************************************************************************/

static exprType_t pas_BaseFactor(varInfo_t *varInfo, exprFlag_t factorFlags)
{
  symbol_t  *varPtr = &varInfo->variable;
  symbol_t  *typePtr;
  exprType_t factorType;

  /* Process according to the current variable sKind */

  typePtr = varPtr->sParm.v.vParent;
  switch (varPtr->sKind)
    {
      /* Check if we have reduced the complex factor to a simple factor */

    case sINT :
    case sWORD :
    case sBOOLEAN :
      factorType = pas_MapVariable2ExprType(varPtr->sKind, true);
      factorType = pas_FactorExprType(factorType, factorFlags);

      if ((factorFlags & FACTOR_INDEXED) != 0)
        {
          if ((factorFlags & FACTOR_DEREFERENCE) != 0)
            {
              if ((factorFlags & FACTOR_LOAD_ADDRESS) != 0)
                {
                  pas_GenerateSimple(opADD);
                  pas_GenerateStackReference(opLDS, varPtr);
                }
              else
                {
                  pas_GenerateStackReference(opLDSX, varPtr);
                }

              pas_GenerateSimple(opLDI);
            }
          else
            {
              pas_GenerateStackReference(opLDSX, varPtr);
            }
        }
      else
        {
          if ((factorFlags & FACTOR_DEREFERENCE) != 0)
            {
              if ((factorFlags & FACTOR_FIELD_OFFSET) != 0)
                {
                  pas_GenerateDataOperation(opPUSH, varInfo->fOffset);
                  pas_GenerateSimple(opADD);
                }

              pas_GenerateSimple(opLDI);
            }
          else if ((factorFlags & FACTOR_PTREXPR) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opLDS, varPtr);
            }
        }
      break;

    case sSHORTINT :
      factorType = exprShortInteger;

      if ((factorFlags & FACTOR_INDEXED) != 0)
        {
          if ((factorFlags & FACTOR_DEREFERENCE) != 0)
            {
              if ((factorFlags & FACTOR_LOAD_ADDRESS) != 0)
                {
                  pas_GenerateSimple(opADD);
                  pas_GenerateStackReference(opLDS, varPtr);
                }
              else
                {
                  pas_GenerateStackReference(opLDSX, varPtr);
                }

              pas_GenerateSimple(opLDIB);
            }
          else if ((factorFlags & FACTOR_PTREXPR) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opLDSXB, varPtr);
            }
        }
      else
        {
          if ((factorFlags & FACTOR_DEREFERENCE) != 0)
            {
              /* The address of pointer we are de-referencing should already
               * be on the stack.
               */

              if ((factorFlags & FACTOR_FIELD_OFFSET) != 0)
                {
                  pas_GenerateDataOperation(opPUSH, varInfo->fOffset);
                  pas_GenerateSimple(opADD);
                }

              pas_GenerateSimple(opLDIB);
            }
          else if ((factorFlags & FACTOR_PTREXPR) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opLDSB, varPtr);
            }
        }
      break;

    case sSHORTWORD :
    case sCHAR :
      factorType = pas_MapVariable2ExprType(varPtr->sKind, true);
      factorType = pas_FactorExprType(factorType, factorFlags);

      if ((factorFlags & FACTOR_INDEXED) != 0)
        {
          if ((factorFlags & FACTOR_DEREFERENCE) != 0)
            {
              if ((factorFlags & FACTOR_LOAD_ADDRESS) != 0)
                {
                  pas_GenerateSimple(opADD);
                  pas_GenerateStackReference(opLDS, varPtr);
                }
              else
                {
                  pas_GenerateStackReference(opLDSX, varPtr);
                }

              pas_GenerateSimple(opULDIB);
            }
          else if ((factorFlags & FACTOR_PTREXPR) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opULDSXB, varPtr);
            }
        }
      else
        {
          if ((factorFlags & FACTOR_DEREFERENCE) != 0)
            {
              /* The address of pointer we are de-referencing should already
               * be on the stack.
               */

              if ((factorFlags & FACTOR_FIELD_OFFSET) != 0)
                {
                  pas_GenerateDataOperation(opPUSH, varInfo->fOffset);
                  pas_GenerateSimple(opADD);
                }

              pas_GenerateSimple(opULDIB);
            }
          else if ((factorFlags & FACTOR_PTREXPR) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opULDSB, varPtr);
            }
        }
      break;

    /* The only thing that REAL, STRING, and SET types have in common is that
     * they are all represented by a multi-word objects.
     */

    case sSET :
      if (g_abstractTypePtr == NULL)
        {
          g_abstractTypePtr = typePtr;
        }
      else if ((typePtr != g_abstractTypePtr) &&
               (typePtr->sParm.v.vParent != g_abstractTypePtr))
        {
          error(eSCALARTYPE);
        }

      /* Fall through */

    case sLONGINT      :
    case sLONGWORD     :
    case sREAL         :
    case sSTRING       :
      if ((factorFlags & FACTOR_INDEXED) != 0)
        {
          symbol_t *baseTypePtr;

          /* In the case of an array, the size of the variable refers to the
           * size of the array.  We need to traverse back to the base type of
           * the array to get the size of an element.
           */

          baseTypePtr = pas_GetBaseTypePointer(typePtr);

          /* Now we can handle all of the ornaments */

          factorType  = pas_MapVariable2ExprType(varPtr->sKind, false);

          if ((factorFlags & FACTOR_DEREFERENCE) != 0)
            {
              if ((factorFlags & FACTOR_LOAD_ADDRESS) != 0)
                {
                  pas_GenerateSimple(opADD);
                  pas_GenerateStackReference(opLDS, varPtr);
                }
              else
                {
                  pas_GenerateStackReference(opLDSX, varPtr);
                }

              pas_GenerateDataSize(baseTypePtr->sParm.t.tAllocSize);
              pas_GenerateSimple(opLDIM);
            }
          else if ((factorFlags & FACTOR_PTREXPR) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
            }
          else
            {
              pas_GenerateDataSize(baseTypePtr->sParm.t.tAllocSize);
              pas_GenerateStackReference(opLDSXM, varPtr);
            }
        }
      else
        {
          factorType  = pas_MapVariable2ExprType(varPtr->sKind, false);

           if ((factorFlags & FACTOR_DEREFERENCE) != 0)
            {
              /* The address of pointer we are de-referencing should already
               * be on the stack.
               */

              if ((factorFlags & FACTOR_FIELD_OFFSET) != 0)
                {
                  pas_GenerateDataOperation(opPUSH, varInfo->fOffset);
                  pas_GenerateSimple(opADD);
                }

              pas_GenerateDataSize(varPtr->sParm.v.vSize);
              pas_GenerateSimple(opLDIM);
            }
          else if ((factorFlags & FACTOR_PTREXPR) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
            }
          else
            {
              pas_GenerateDataSize(varPtr->sParm.v.vSize);
              pas_GenerateStackReference(opLDSM, varPtr);
            }
        }
      break;

    case sSCALAR :
      if (g_abstractTypePtr == NULL)
        {
          g_abstractTypePtr = typePtr;
        }
      else if (typePtr != g_abstractTypePtr)
        {
          error(eSCALARTYPE);
        }

      factorType = pas_FactorExprType(exprScalar, factorFlags);
      if ((factorFlags & FACTOR_INDEXED) != 0)
        {
          if ((factorFlags & FACTOR_DEREFERENCE) != 0)
            {
              if ((factorFlags & FACTOR_LOAD_ADDRESS) != 0)
                {
                  pas_GenerateSimple(opADD);
                  pas_GenerateStackReference(opLDS, varPtr);
                }
              else
                {
                  pas_GenerateStackReference(opLDSX, varPtr);
                }

              pas_GenerateSimple(opLDI);
            }
          else
            {
              pas_GenerateStackReference(opLDSX, varPtr);
            }
        }
      else
        {
          if ((factorFlags & FACTOR_DEREFERENCE) != 0)
            {
              /* The address of pointer we are de-referencing should already
               * be on the stack.
               */

              if ((factorFlags & FACTOR_FIELD_OFFSET) != 0)
                {
                  pas_GenerateDataOperation(opPUSH, varInfo->fOffset);
                  pas_GenerateSimple(opADD);
                }

              pas_GenerateSimple(opLDI);
            }
          else
            {
              pas_GenerateStackReference(opLDS, varPtr);
            }
        }
      break;

    case sFILE :
    case sTEXTFILE :
      factorType = pas_FactorExprType(exprFile, factorFlags);
      if ((factorFlags & FACTOR_INDEXED) != 0)
        {
          if ((factorFlags & FACTOR_DEREFERENCE) != 0)
            {
              if ((factorFlags & FACTOR_LOAD_ADDRESS) != 0)
                {
                  pas_GenerateSimple(opADD);
                  pas_GenerateStackReference(opLDS, varPtr);
                }
              else
                {
                  pas_GenerateStackReference(opLDSX, varPtr);
                }

              pas_GenerateSimple(opLDI);
            }
          else
            {
              pas_GenerateStackReference(opLDSX, varPtr);
            }
        }
      else
        {
          if ((factorFlags & FACTOR_DEREFERENCE) != 0)
            {
              /* The address of pointer we are de-referencing should already
               * be on the stack.
               */

              if ((factorFlags & FACTOR_FIELD_OFFSET) != 0)
                {
                  pas_GenerateDataOperation(opPUSH, varInfo->fOffset);
                  pas_GenerateSimple(opADD);
                }

              pas_GenerateSimple(opLDI);
            }
          else
            {
              pas_GenerateStackReference(opLDS, varPtr);
            }
        }
      break;

    /* REVISIT:  A RECORD name may be a base factor -- as the VAR input
     * parameter of a function or in an assignment
     */

    case sRECORD :

    default :
      factorType = exprUnknown;
      break;
    }

  return factorType;
}

/****************************************************************************/
/* Process a factor of the for ^variable OR a VAR parameter (where the
 * ^ is implicit).
 */

static exprType_t pas_PointerFactor(void)
{
  exprType_t factorType;

  /* Process by token type */

  switch (g_token)
    {
      /* Pointers to simple types */

      case sINT :
      case sWORD :
      case sSHORTINT :
      case sSHORTWORD :
      case sLONGINT :
      case sLONGWORD :
      case sBOOLEAN :
      case sCHAR :
        pas_GenerateStackReference(opLAS, g_tknPtr);
        factorType = pas_MapVariable2ExprPtrType(g_token, true);
        getToken();
        break;

      case sSCALAR :
        if (g_abstractTypePtr != NULL)
          {
            if (g_tknPtr->sParm.v.vParent != g_abstractTypePtr) error(eSCALARTYPE);
          }
        else
          {
           g_abstractTypePtr = g_tknPtr->sParm.v.vParent;
          }

        pas_GenerateStackReference(opLAS, g_tknPtr);
        getToken();
        factorType = exprScalarPtr;
        break;

      case sSET :
        /* If an g_abstractTypePtr is specified then it should either be the
         * same SET OF <object> -OR- the same <object>
         */

        if (g_abstractTypePtr != NULL)
          {
            if (g_tknPtr->sParm.v.vParent != g_abstractTypePtr &&
                g_tknPtr->sParm.v.vParent->sParm.t.tParent != g_abstractTypePtr)
              {
                 error(eSET);
              }
          }
        else
          {
             g_abstractTypePtr = g_tknPtr->sParm.v.vParent;
          }

        /* Fall through */

      case sREAL :
      case sSTRING :
      case sFILE :
      case sTEXTFILE :
        pas_GenerateStackReference(opLAS, g_tknPtr);
        factorType = pas_MapVariable2ExprPtrType(g_token, false);
        getToken();
        break;

      /* Complex factors */

      case sSUBRANGE :
      case sRECORD :
      case sRECORD_OBJECT :
      case sVAR_PARM :
      case sPOINTER :
      case sARRAY :
        factorType = pas_ComplexPointerFactor(0);
        break;

      /* References to address of a pointer */

      case '^' :
        error(eNOTYET);
        getToken();
        factorType = pas_PointerFactor();
        break;

      case tNIL :
        getToken();
        pas_GenerateDataOperation(opPUSH, 0);
        factorType = exprAnyPointer;
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

static exprType_t pas_ComplexPointerFactor(exprFlag_t factorFlags)
{
  varInfo_t varInfo;

  /* First, make a copy of the symbol table entry because the call to
   * pas_SimplePointerFactor() will modify it.
   */

  if (g_tknPtr == NULL)
    {
      error(eEXPRTYPE);
      return exprUnknown;
    }
  else
    {
      varInfo.variable = *g_tknPtr;
      varInfo.ptrDepth = 0;
      varInfo.fOffset  = 0;

      /* Since we have saved the token information, we can skip to the next
       * token.
       */

      getToken();

      /* Then process the complex factor until it is reduced to a simple
       * factor (like int, char, etc.)
       */

      return pas_SimplePointerFactor(&varInfo, factorFlags);
    }
}

/****************************************************************************/
/* Process a complex factor (recursively) until it becomes a simple
 * factor.
 */

static exprType_t pas_SimplePointerFactor(varInfo_t *varInfo,
                                          exprFlag_t factorFlags)
{
  symbol_t  *varPtr = &varInfo->variable;
  symbol_t  *typePtr;
  exprType_t factorType;

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
      if (g_abstractTypePtr == NULL) g_abstractTypePtr = typePtr;
      varPtr->sKind = typePtr->sParm.t.tSubType;
      factorType = pas_SimplePointerFactor(varInfo, factorFlags);
      break;

    case sRECORD :
      /* Check if this is the address of a record */

      if (g_token != '.')
        {
          if ((factorFlags & FACTOR_DEREFERENCE) != 0)
            {
              error(ePOINTERTYPE);
            }

          if ((factorFlags & FACTOR_VAR_PARM) != 0)
            {
              /* Load the address from the VAR parameter */

              if ((factorFlags & FACTOR_INDEXED) != 0)
                {
                  pas_GenerateStackReference(opLDSX, varPtr);
                }
              else
                {
                  pas_GenerateStackReference(opLDS, varPtr);
                }

              /* I would expect FACTOR_LOAD_ADDRESS to be set in this case */

              factorType = exprRecordPtr;
            }
          else
            {
              /* Get the address from the variable */

              if ((factorFlags & FACTOR_INDEXED) != 0)
                {
                  pas_GenerateStackReference(opLASX, varPtr);
                }
              else
                {
                  pas_GenerateStackReference(opLAS, varPtr);
                }

              factorType = exprRecordPtr;
            }
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
              factorFlags            |= FACTOR_FIELD_OFFSET;

              getToken();
              factorType = pas_SimplePointerFactor(varInfo, factorFlags);
            }
        }
      break;

    case sRECORD_OBJECT :
      /* NOTE:  This must have been preceeded with a WITH statement
       * defining the RECORD type
       */

      if (!g_withRecord.wParent)
        {
          error(eINVTYPE);
        }
      else if ((factorFlags && FACTOR_DEREFERENCE) != 0)
        {
          error(ePOINTERTYPE);
        }
      else if ((factorFlags && FACTOR_INDEXED) != 0)
        {
          error(eARRAYTYPE);
        }

      /* Verify that a field identifier is associated with the RECORD
       * specified by the WITH statement.
       */

      else if (varPtr->sParm.r.rRecord != g_withRecord.wParent)
        {
          error(eRECORDOBJECT);
        }
      else
        {
          int16_t tempOffset;

          /* Now there are two cases to consider:  (1) the g_withRecord is a
           * pointer to a RECORD, or (2) the g_withRecord is the RECORD itself
           */

          if (g_withRecord.wPointer)
            {
              pas_GenerateDataOperation(opPUSH,
                                        varPtr->sParm.r.rOffset +
                                        g_withRecord.wIndex);
              factorFlags |= (FACTOR_INDEXED | FACTOR_DEREFERENCE);
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
           */

          typePtr                 = varPtr->sParm.r.rParent;

          varPtr->sKind           = typePtr->sParm.t.tType;
          varPtr->sLevel          = g_withRecord.wLevel;
          varPtr->sParm.v.vSize   = typePtr->sParm.t.tAllocSize;
          varPtr->sParm.v.vOffset = tempOffset;
          varPtr->sParm.v.vParent = typePtr;

          factorType = pas_SimplePointerFactor(varInfo, factorFlags);
        }
      break;

    case sPOINTER :
      {
        symbol_t *parentTypePtr = typePtr;

        /* Get the parent (non-pointer) type */

        while (parentTypePtr->sParm.t.tType == sPOINTER)
          {
            if (varInfo->ptrDepth > 1)
              {
                error(eNOTYET);
              }

            varInfo->ptrDepth++;
            parentTypePtr = parentTypePtr->sParm.t.tParent;
          }

        /* Do we want the address or the pointer? Or the pointer value that
         * it points to?
         */

        if ((factorFlags & FACTOR_PTREXPR) == 0)
          {
            varInfo->ptrDepth--;
            factorFlags |= FACTOR_DEREFERENCE;
          }

        /* Verify that we are returing some kind of pointer */

        if (varInfo->ptrDepth == 0 ||
            (varInfo->ptrDepth == 1 && g_token == '^'))
          {
            error(ePTRADR);
          }

        /* And process a pointer to that parent type */

        varPtr->sKind  = parentTypePtr->sParm.t.tType;
        factorType     = pas_SimplePointerFactor(varInfo, factorFlags);
      }
      break;

    case sVAR_PARM :
      /* Factor Flags:
       *
       *   FACTOR_VAR_PARM     - This is a VAR parameter.
       *   FACTOR_LOAD_ADDRESS - Does nothing unless we discover later that
       *                         this is an ARRAY VAR parameter then this
       *                         will issue the correct order of indexing.
       */

      if (factorFlags != 0) error(eVARPARMTYPE);

      factorFlags  |= (FACTOR_LOAD_ADDRESS | FACTOR_VAR_PARM);

      /* Then recurse to simplify the VAR parameter */

      varPtr->sKind = typePtr->sParm.t.tType;
      factorType    = pas_SimplePointerFactor(varInfo, factorFlags);
      break;

    case sARRAY :
      factorType    = pas_ArrayPointerFactor(varInfo, factorFlags);
      break;

    default :
      error(eINVTYPE);
      factorType = exprInteger;
      break;
    }

  return factorType;
}

/****************************************************************************/

static exprType_t pas_ArrayPointerFactor(varInfo_t *varInfo,
                                         exprFlag_t factorFlags)
{
  symbol_t  *varPtr      = &varInfo->variable;
  symbol_t  *typePtr     = varPtr->sParm.v.vParent;
  symbol_t  *baseTypePtr;
  exprType_t factorType  = exprUnknown;
  uint16_t   arrayKind;

  /* Reduce the array to its base type. */

  baseTypePtr = pas_GetBaseTypePointer(typePtr);
  arrayKind   = baseTypePtr->sParm.t.tType;

  /* REVISIT:  For subranges, we use the base type of the subrange. */

  if (arrayKind == sSUBRANGE)
    {
      arrayKind = baseTypePtr->sParm.t.tSubType;
    }

  /* The array may be followed by braces to dereference a particular element. */

  if (g_token == '[')
    {
      /* Is FACTOR_INDEXED already selected? */

      if ((factorFlags & FACTOR_INDEXED) != 0)
        {
          error(eARRAYTYPE);
        }

      /* Indicate that this is an array (or an array VAR parameter) and
       * indexing is required.
       */

      factorFlags  |= FACTOR_INDEXED;

      /* Generate the array offset calculation and indexed load */

      pas_ArrayIndex(typePtr);

      /* If this is an array of records, then are not finished. */

      varPtr->sKind = arrayKind;
      if (arrayKind == sRECORD)
        {
          factorType =
            pas_SimplePointerFactor(varInfo, factorFlags);
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

  /* Perhaps this is a pointer to an array or an array VAR parameter.
   * In that case, we need to load the address of the array by
   * dereferencing the pointer/VAR parameter.
   */

  else if ((factorFlags & (FACTOR_DEREFERENCE | FACTOR_VAR_PARM)) != 0)
    {
     /* Load the address of the array by dereferencing the pointer. */

     pas_GenerateStackReference(opLDS, varPtr);

     /* If the varPtr offset is really to a RECORD and if the array
      * type is really a field of the RECORD, then we need to add
      * the offset of the array field to this address.
      */

     if ((factorFlags & FACTOR_FIELD_OFFSET) != 0)
       {
         pas_GenerateDataOperation(opPUSH, varInfo->fOffset);
         pas_GenerateSimple(opADD);
       }

     /* The expression type is still a pointer to the base type of the
      * array.
      */

     factorType = pas_MapVariable2ExprPtrType(baseTypePtr->sParm.t.tType,
                                              false);
    }

  /* But a more typical case in the context of this function is to have no
   * index on the array.  This would be the case if the array is passed by
   * reference as a VAR.
   */

  else
   {
     /* Just load the address of the array. */

     pas_GenerateStackReference(opLAS, varPtr);

     /* The expression type is then a pointer to the base type of the
      * array.
      */

     factorType = pas_MapVariable2ExprPtrType(baseTypePtr->sParm.t.tType,
                                              false);
   }

  return factorType;
}

/****************************************************************************/

static exprType_t pas_BasePointerFactor(symbol_t *varPtr,
                                        exprFlag_t factorFlags)
{
  symbol_t  *typePtr;
  exprType_t factorType;

  /* NOPE... recurse until it becomes a simple pointer factor
   *
   * Process the complex factor according to the current variable sKind.
   */

  typePtr = varPtr->sParm.v.vParent;
  switch (varPtr->sKind)
    {
      /* Check if we have reduced the complex factor to a simple factor */

    case sINT :
    case sWORD :
    case sSHORTINT :
    case sSHORTWORD :
    case sLONGINT :
    case sLONGWORD :
    case sCHAR :
    case sBOOLEAN :
      if ((factorFlags & FACTOR_INDEXED) != 0)
        {
          if ((factorFlags & (FACTOR_DEREFERENCE | FACTOR_VAR_PARM)) != 0)
            {
              if ((factorFlags & FACTOR_LOAD_ADDRESS) != 0)
                {
                  pas_GenerateStackReference(opLDS, varPtr);
                  pas_GenerateSimple(opADD);
                }
              else
                {
                  pas_GenerateStackReference(opLDSX, varPtr);
                }
            }
          else
            {
              pas_GenerateStackReference(opLASX, varPtr);
            }
        }
      else
        {
          if ((factorFlags & (FACTOR_DEREFERENCE | FACTOR_VAR_PARM)) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opLAS, varPtr);
            }
        }

      factorType = pas_MapVariable2ExprPtrType(varPtr->sKind, true);
      break;

    /* The only thing that REAL, STRING and SET types have in common is that
     * they are all represented by multi-word objects.
     */

    case sSET :
      /* If an g_abstractTypePtr is specified then it should either be the
       * same SET OF <object> -OR- the same <object>
       */

      if (g_abstractTypePtr == NULL)
        {
          g_abstractTypePtr = typePtr;
        }
      else if ((typePtr != g_abstractTypePtr) &&
               (typePtr->sParm.v.vParent != g_abstractTypePtr))
        {
          error(eSCALARTYPE);
        }

      /* Fall through */

    case sREAL         :
    case sSTRING       :
      if ((factorFlags & FACTOR_INDEXED) != 0)
        {
          if ((factorFlags & (FACTOR_DEREFERENCE | FACTOR_VAR_PARM)) != 0)
            {
              if ((factorFlags & FACTOR_LOAD_ADDRESS) != 0)
                {
                  pas_GenerateStackReference(opLDS, varPtr);
                  pas_GenerateSimple(opADD);
                }
              else
                {
                  pas_GenerateStackReference(opLDSX, varPtr);
                }
            }
          else
            {
              pas_GenerateStackReference(opLASX, varPtr);
            }
        }
      else
        {
          if ((factorFlags & (FACTOR_DEREFERENCE | FACTOR_VAR_PARM)) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
            }
          else
            {
              pas_GenerateStackReference(opLAS, varPtr);
            }
        }

      factorType = pas_MapVariable2ExprPtrType(varPtr->sKind, false);
      break;

    case sSCALAR :
      if (g_abstractTypePtr == NULL)
        {
          g_abstractTypePtr = typePtr;
        }
      else if (typePtr != g_abstractTypePtr)
        {
          error(eSCALARTYPE);
        }

      if ((factorFlags & FACTOR_INDEXED) != 0)
        {
          if ((factorFlags & (FACTOR_DEREFERENCE | FACTOR_VAR_PARM)) != 0)
            {
              if ((factorFlags & FACTOR_LOAD_ADDRESS) != 0)
                {
                  pas_GenerateStackReference(opLDS, varPtr);
                  pas_GenerateSimple(opADD);
                }
              else
                {
                  pas_GenerateStackReference(opLDSX, varPtr);
                }
            }
          else
            {
              pas_GenerateStackReference(opLASX, varPtr);
            }

          factorType = exprScalarPtr;
        }
      else
        {
          if ((factorFlags & (FACTOR_DEREFERENCE | FACTOR_VAR_PARM)) != 0)
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

    case sFILE :
    case sTEXTFILE :
      if ((factorFlags & FACTOR_INDEXED) != 0)
        {
          if ((factorFlags & (FACTOR_DEREFERENCE | FACTOR_VAR_PARM)) != 0)
            {
              if ((factorFlags & FACTOR_LOAD_ADDRESS) != 0)
                {
                  pas_GenerateStackReference(opLDS, varPtr);
                  pas_GenerateSimple(opADD);
                }
              else
                {
                  pas_GenerateStackReference(opLDSX, varPtr);
                }

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
          if ((factorFlags & (FACTOR_DEREFERENCE | FACTOR_VAR_PARM)) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              factorType = exprFilePtr;
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
  symbol_t  *typePtr      = funcPtr->sParm.p.pParent;
  exprType_t factorType;
  uint16_t   savedFixup;
  int        size         = 0;

  /* FORM: function-designator =
   *       function-identifier [ actual-parameter-list ]
   */

  /* Initialize the string stack pointer fixup for this level */

  savedFixup      = g_strStackFixup;
  g_strStackFixup = 0;

  /* Allocate stack space for a reference instance of the type returned by
   * the function.  This is an "container" that will catch the valued
   * returned by the function.
   *
   * STRING return value containers need some special initialization.
   */

  if (typePtr->sKind == sTYPE && typePtr->sParm.t.tType == sSTRING)
    {
      pas_StringLibraryCall(lbSTRTMP);
    }
  else
    {
      pas_GenerateDataOperation(opINDS, INT_ALIGNUP(typePtr->sParm.t.tAllocSize));
    }

  /* Get the type of the function */

  factorType = pas_GetExpressionType(typePtr);
  pas_SetAbstractType(typePtr);

   /* Skip over the function-identifier */

  getToken();

  /* Get the actual parameters (if any) associated with the procedure
   * call.  This will lie in the stack "above" the function return
   * value container.  The returned size accounts for all of the parameters
   * with each aligned in integer-size address boundaries.
   */

  size = pas_ActualParameterList(funcPtr);

  /* Generate function call and stack adjustment (if required) */

  pas_GenerateProcedureCall(funcPtr);

  /* If there was persistent string storage used in function call, then
   * free that now.
   */

  if (g_strStackFixup > 0)
    {
       pas_GenerateDataOperation(opINCS, -g_strStackFixup);
    }

  /* Restore the previous stack fixup */

  g_strStackFixup = savedFixup;

  /* Release memory used by the actual parameter list (if any). */

  if (size)
    {
      pas_GenerateDataOperation(opINDS, -size);
    }

  return factorType;
}

/****************************************************************************/

static exprType_t pas_FactorExprType(exprType_t baseExprType,
                                     uint8_t factorFlags)
{
  return ((factorFlags & FACTOR_PTREXPR) == 0) ? baseExprType :
          MK_POINTER_EXPRTYPE(baseExprType);
}

/****************************************************************************/
/* Determine the expression type associated with a pointer to a type
 * symbol
 */

static void pas_SetAbstractType(symbol_t *sType)
{
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
          if (g_abstractTypePtr != NULL)
            {
              if (sType != g_abstractTypePtr) error(eSCALARTYPE);
            }
          else
            {
              g_abstractTypePtr = sType;
            }
          break;

        case sSUBRANGE :
          if (g_abstractTypePtr == NULL)
            {
              g_abstractTypePtr = sType;
            }
          else if (g_abstractTypePtr->sParm.t.tType != sSUBRANGE ||
                   g_abstractTypePtr->sParm.t.tSubType != sType->sParm.t.tSubType)
            {
              error(eSUBRANGETYPE);
            }

          switch (sType->sParm.t.tSubType)
            {
              case sINT :
              case sWORD :
              case sSHORTINT :
              case sSHORTWORD :
              case sLONGINT :
              case sLONGWORD :
              case sCHAR :
                break;

              case sSCALAR :
                if (g_abstractTypePtr != sType) error(eSUBRANGETYPE);
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

static exprType_t pas_GetSetFactor(void)
{
  symbol_t *abstractTypePtr;
  symbol_t *setTypePtr;
  bool first;

  /* FORM: '['[ set-subset [',' set-subset [',' ...]]]']'
   *       set-subset = set-element | set-subrange
   *       set-element = set-constant | set-ordinal-variable
   *       set-subrange = set-element '..' set-element
   *
   * ASSUMPTION:  The first '[' has already been processed
   */

  /* Check for the empty set [] */

  if (g_token == ']')
    {
      /* Generate the empty set.  An empty set differs from other sets in
       * that there is no abstract base type for the empty set.
       */

      pas_GenerateSetOperation(setEMPTY);
      return exprEmptySet;
    }

  /* If we are here processing a non-empty SET, then the g_abstractTypePtr
   * should be valid.
   */

  abstractTypePtr = g_abstractTypePtr;
  if (abstractTypePtr == NULL)
    {
      error(eHUH);
    }

  /* Verify that a scalar expression type has been specified.  If the
   * g_abstractTypePtr is a SET, then we will need to get the TYPE
   * that it is a SET of.
   */

  setTypePtr = g_abstractTypePtr;

  /* This is a little like pas_GetBaseType(), but does not traverse
   * all the way to the base type, only to the parent of the SET.
   */

  /* It might be an array of SETs? */

  if (setTypePtr->sParm.t.tType == sARRAY)
    {
      setTypePtr = setTypePtr->sParm.t.tParent;
    }

  if (setTypePtr->sParm.t.tType == sSET)
    {
      setTypePtr = setTypePtr->sParm.t.tParent;
    }

  /* Get the first element of the set */

  first = pas_GetSubSet(setTypePtr, true);

  /* Incorporate each additional element into the set */

  while (g_token == ',')
    {
      /* Get the next element of the set */

      getToken();
      first = pas_GetSubSet(setTypePtr, first);
    }

  /* Restore the abstract type pointer */

  g_abstractTypePtr = abstractTypePtr;
  return exprSet;
}

/****************************************************************************/

static bool pas_GetSubSet(symbol_t *setTypePtr, bool first)
{
  exprType_t subset1ExprType;
  exprType_t findExprType;
  uint16_t   findType;

  /* Process a subset:
   *
   * FORM: set-subset = set-element | set-subrange
   *       set-element = set-constant | set-ordinal-variable
   *       set-subrange = set-element '..' set-element
   */

  /* Get the first set-element.  Whatever that first element is, it will be
   * pushed at the top of the stack if pas_Expression() is successful.
   */

  if (setTypePtr->sParm.t.tType == sSUBRANGE)
    {
      findType = setTypePtr->sParm.t.tSubType;
    }
  else
    {
      findType = setTypePtr->sParm.t.tType;
    }

  findExprType = pas_MapVariable2ExprType(findType, true);
  if (findExprType == exprUnknown)
    {
      error(eSETELEMENT);
    }

  subset1ExprType = pas_Expression(findExprType, setTypePtr);
  if (subset1ExprType == exprUnknown)
    {
      error(eSETELEMENT);
    }

  /* Check if what we just pushed is the first value in a sub-range */

  if (g_token != tSUBRANGE)
    {
      /* Yes, then expand the pushed value into a singleton set */

      pas_GenerateDataOperation(opPUSH, setTypePtr->sParm.t.tMinValue);
      pas_GenerateSetOperation(setSINGLETON);
    }
  else
    {
      exprType_t subset2ExprType;

      /* No, then get the upper value of the subrange.  That value will also be
       * pushed at the top of the stack of pas_Expression() is successful.
       */

      getToken();
      subset2ExprType = pas_Expression(subset1ExprType, setTypePtr);
      if (subset2ExprType == exprUnknown)
        {
          error(eSETELEMENT);
        }

       /* Then convert the two values at the top of the stand into a SET that
        * represents the subrange.
        */

      pas_GenerateDataOperation(opPUSH, setTypePtr->sParm.t.tMinValue);
      pas_GenerateSetOperation(setSUBRANGE);
    }

  /* If this was not the first set-subset, then push an operation to OR it
   * it with the previous subset.
   */

  if (!first)
    {
      pas_GenerateSetOperation(setUNION);
    }

  return false;
}

/****************************************************************************/
/* A type name may be part of a valid factor if it is a type case */

static exprType_t pas_TypeCast(symbol_t *typePtr)
{
  /* Form type-name '(' expression ')' */

  getToken();
  if (g_token != '(') error(eLPAREN);
  else
    {
      exprType_t newExprType;
      exprType_t originalExprType;
      exprType_t findExprType;
      uint16_t castType;
      bool newOrdinal;

      /* At present we are accepting only casts between ordinal type of the
       * same size (and real types).
       */

      castType = typePtr->sParm.t.tType;

      /* Handle casts between pointer types.
       * REVISIT:  How badly is this breaking Pascal's strong typing?  At least
       * we don't support any casts of pointers outside the run-time sandbox.
       */

      if (castType == sPOINTER)
        {
          symbol_t *parent = typePtr->sParm.t.tParent;
          uint16_t parentType = parent->sParm.t.tType;
          if (parentType == sPOINTER)
            {
              /* No pointers-to-pointers yet */

              error(eNOTYET);
              return exprUnknown;
            }

          newExprType = pas_MapVariable2ExprPtrType(parentType, false);
          if (newExprType == exprUnknown)
            {
              error(eEXPRTYPE);
              return exprUnknown;
            }

          newOrdinal   = false;
          findExprType = exprAnyPointer;  /* Expect a pointer */
        }

      /* Handle casts from ordinal (or REAL) types to REAL type */

      else if (castType == sREAL)
        {
          newExprType  = exprReal;
          newOrdinal   = false;
          findExprType = exprUnknown;  /* Expect ordinal or real */
        }

      /* Handle casts from REAL (or ordinal) types to ordinal types */

      else
        {
          newExprType = pas_MapVariable2ExprType(castType, true);
          if (newExprType == exprUnknown)
            {
              error(eEXPRTYPE);
              return exprUnknown;
            }

          newOrdinal   = true;
          findExprType = exprUnknown;  /* Expect ordinal or real */
        }

      /* Skip over the left parenthesis and evaluate the expression */

      getToken();
      originalExprType = pas_Expression(findExprType, NULL);

      if (g_token != ')') error(eRPAREN);
      else getToken();

      if (originalExprType != exprUnknown)
        {
          bool originalOrdinal = pas_IsOrdinalExpression(originalExprType);

          /* Check for conversion of ordinal value to REAL */

          if (newExprType == exprReal && originalOrdinal)
            {
              /* REVISIT: Won't work for long integer types */

              if (originalExprType != exprLongInteger &&
                  originalExprType != exprLongWord)
                {
                  /* For now, convert to a 16-bit integer.  Might overflow! */

                  pas_GenerateSimpleLongOperation(opDCNV);
                }

              pas_GenerateFpOperation(fpFLOAT);
            }

          /* Check for conversion of REAL to ordinal value */

          else if (newOrdinal && originalExprType == exprReal)
            {
              /* REVISIT: Won't work for long integer types */

              if (originalExprType != exprLongInteger &&
                  originalExprType != exprLongWord)
                {
                  /* For now, convert to a 16-bit integer.  Might overflow! */

                  pas_GenerateSimpleLongOperation(opDCNV);
                }

              pas_GenerateFpOperation(fpROUND);
            }

          /* REVISIT:  Check for conversion of REAL to long integer value */

          /* Conversions between ordinal types or pointers */

          else if (newOrdinal && originalOrdinal)
            {
              /* All ordinal types EXCEPT for the long integers are the
               * same size and, hence, require no real effort.
               */

              if ((newExprType     == exprLongInteger ||
                   newExprType     == exprLongWord) &&
                  originalExprType != exprLongInteger &&
                  originalExprType != exprLongWord)
                {
                  /* Was the original type a signed integer type? */

                  if (originalExprType == exprInteger ||
                      originalExprType == exprShortInteger)
                    {
                      /* Sign extend the 16-bit signed integer value to a 32-
                       * bit signed/unsiged integer type
                       */

                      pas_GenerateSimpleLongOperation(opCNVD);
                    }
                  else
                    {
                      /* Convert the 16-bit unsigned ordinal value to a 32-
                       * bit signed/unsiged integer type
                       */

                      pas_GenerateSimpleLongOperation(opUCNVD);
                    }

                }
              else if (newExprType      != exprLongInteger &&
                       newExprType      != exprLongWord &&
                       (originalExprType == exprLongInteger ||
                        originalExprType == exprLongWord))
                {
                  /* Convert the 32-bit integer value to a 16-bit ordinal type */

                  pas_GenerateSimpleLongOperation(opDCNV);
                }
              else
                {
                  /* Both are 16-bit ordinal types */
                }
            }

          /* Conversions between pointers */

          else if (IS_POINTER_EXPRTYPE(newExprType) &&
                   IS_POINTER_EXPRTYPE(originalExprType))
            {
            }
          else
            {
              error(eEXPRTYPE);
              return exprUnknown;
            }

          return newExprType;
        }
    }

  return exprUnknown;
}

/****************************************************************************/
/* Check if this is a ordinal expression.  This is what is needed, for
 * example, as an argument to ord(), pred(), succ(), or odd().
 * This is the kind of expression we need in a CASE statement
 * as well.
 */

static bool pas_IsOrdinalExpression(exprType_t testExprType)
{
  if (testExprType == exprInteger      || /* signed integer value */
      testExprType == exprWord         || /* unsigned integer value */
      testExprType == exprShortInteger || /* signed short integer value */
      testExprType == exprShortWord    || /* unsigned short integer value */
      testExprType == exprLongInteger  || /* signed long integer value */
      testExprType == exprLongWord     || /* unsigned long integer value */
      testExprType == exprChar         || /* character value */
      testExprType == exprBoolean      || /* boolean(integer) value */
      testExprType == exprScalar)         /* scalar(integer) value */
    {
      return true;
    }
  else
    {
      return false;
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************/
/* Evaluate (boolean) Expression */

exprType_t pas_Expression(exprType_t findExprType, symbol_t *typePtr)
{
  exprOpCodes_t  exprOpCodes;
  uint8_t        operation;
  exprType_t     simple1Type;
  exprType_t     simple2Type;
  bool           haveSimple2;
  bool           handled;

  /* The abstract types - SETs, RECORDS, etc - require an exact
   * match in type.  Save the symbol table sTYPE entry associated
   * with the expression.
   */

  if (typePtr != NULL && typePtr->sKind != sTYPE) error(eINVTYPE);
  g_abstractTypePtr = typePtr;

  /* FORM <simple expression> [<relational operator> <simple expression>]
   *
   * Get the first <simple expression>
   */

  simple1Type = pas_SimpleExpression(findExprType);

  /* Set all opcodes to no-op or invalid */

  memset(&exprOpCodes, 0, sizeof(exprOpCodes_t));

  /* Get the optional <relational operator> which may follow */

  operation = g_token;
  switch (operation)
    {
    case tEQ :
      /* Select all opcodes for all cases */

      exprOpCodes.intOpCode      = opEQU;
      exprOpCodes.wordOpCode     = opEQU;
      exprOpCodes.ptrOpCode      = opEQU;
      exprOpCodes.charOpCode     = opEQU;
      exprOpCodes.boolOpCode     = opEQU;
      exprOpCodes.longIntOpCode  = opDEQU;
      exprOpCodes.longWordOpCode = opDEQU;
      exprOpCodes.fpOpCode       = fpEQU;
      exprOpCodes.strOpCode      = opEQUZ;
      exprOpCodes.setOpCode      = setEQUALITY;

      /* Skip over the operator */

      getToken();
      break;

    case tNE :
      /* Select all opcodes for all cases */

      exprOpCodes.intOpCode      = opNEQ;
      exprOpCodes.wordOpCode     = opNEQ;
      exprOpCodes.ptrOpCode      = opNEQ;
      exprOpCodes.charOpCode     = opNEQ;
      exprOpCodes.boolOpCode     = opNEQ;
      exprOpCodes.longIntOpCode  = opDNEQ;
      exprOpCodes.longWordOpCode = opDNEQ;
      exprOpCodes.fpOpCode       = fpNEQ;
      exprOpCodes.strOpCode      = opNEQZ;
      exprOpCodes.setOpCode      = setNONEQUALITY;

      /* Skip over the operator */

      getToken();
      break;

    case tLT :
      /* Select all opcodes for all cases */

      exprOpCodes.intOpCode      = opLT;
      exprOpCodes.wordOpCode     = opULT;
      exprOpCodes.longIntOpCode  = opDLT;
      exprOpCodes.longWordOpCode = opDULT;
      exprOpCodes.fpOpCode       = fpLT;
      exprOpCodes.strOpCode      = opLTZ;
      exprOpCodes.setOpCode      = setINVALID;

      /* Skip over the operator */

      getToken();
      break;

    case tLE :
      /* Select all opcodes for all cases */

      exprOpCodes.intOpCode      = opLTE;
      exprOpCodes.wordOpCode     = opULTE;
      exprOpCodes.longIntOpCode  = opDLTE;
      exprOpCodes.longWordOpCode = opDULTE;
      exprOpCodes.fpOpCode       = fpLTE;
      exprOpCodes.strOpCode      = opLTEZ;
      exprOpCodes.setOpCode      = setCONTAINS;

      /* Skip over the operator */

      getToken();
      break;

    case tGT :
      /* Select all opcodes for all cases */

      exprOpCodes.intOpCode      = opGT;
      exprOpCodes.wordOpCode     = opUGT;
      exprOpCodes.longIntOpCode  = opDGT;
      exprOpCodes.longWordOpCode = opDUGT;
      exprOpCodes.fpOpCode       = fpGT;
      exprOpCodes.strOpCode      = opGTZ;
      exprOpCodes.setOpCode      = setINVALID;

      /* Skip over the operator */

      getToken();
      break;

    case tGE :
      /* Select all opcodes for all cases */

      exprOpCodes.intOpCode      = opGTE;
      exprOpCodes.wordOpCode     = opUGTE;
      exprOpCodes.longIntOpCode  = opDGTE;
      exprOpCodes.longWordOpCode = opDUGTE;
      exprOpCodes.fpOpCode       = fpGTE;
      exprOpCodes.strOpCode      = opGTEZ;
      exprOpCodes.setOpCode      = setINVALID;

      /* Skip over the operator */

      getToken();
      break;

    case tIN :
      /* Select all opcodes for all cases */

      exprOpCodes.setOpCode      = setMEMBER;

      /* Skip over the operator */

      getToken();
      break;

    default  :
      break;
    }

  /* Check if there is a 2nd simple expression needed.  This depends on the
   * kind of expression we found for the first expression and the kind of
   * operator that was found.
   *
   * Check for operations on sets first.  These may be:
   *
   *   FORM:  set-expression set-operator set-expression
   *          set-operator = '=' | '<>' | '<='
   *   FORM:  set-member 'in' set-expression
   *
   * The set member may be any value for the sub-range of the ordinal type
   * that underlies the set.
   */

  haveSimple2 = false;
  handled     = false;

  if (exprOpCodes.setOpCode != setINVALID &&
      (((simple1Type == exprSet || simple1Type == exprEmptySet) &&
        exprOpCodes.setOpCode != setMEMBER) ||
       (pas_IsOrdinalExpression(simple1Type) &&
        exprOpCodes.setOpCode == setMEMBER)))
    {
      symbol_t *abstract1Type = g_abstractTypePtr;
      symbol_t *abstract2Type = NULL;

      /* The top of the stack may hold either (1) the first set in a binary
       * operation or (2) an integer-size, subrange member as the first part of
       * the set-member.  g_abstractTypePtr will be NULL in that latter case.
       *
       * Get the second simple expression which should be a SET in all cases
       * and should have a non-NULL g_abstractTypePtr.
       */

      g_abstractTypePtr = NULL;
      simple2Type       = pas_SimpleExpression(exprSet);
      haveSimple2       = true;
      abstract2Type     = g_abstractTypePtr;

      /* In all cases, the second expression must always be a SET */

      if (simple2Type == exprSet || simple2Type == exprEmptySet)
        {
          switch (exprOpCodes.setOpCode)
            {
              case setEQUALITY :
              case setNONEQUALITY :
              case setCONTAINS :
                /* FORM:  set1 comparison-operator set2
                 * FORM:  comparison-operator = '=' | '<>' | '<='
                 *
                 * The two set expressions must refer to the same, underlying
                 * abstract type (unless one is the empty set which has no
                 * base abstract type).
                 */

                if ((simple1Type == exprReal && simple2Type == exprReal) &&
                    abstract1Type != abstract2Type)
                  {
                    error(eEXPRTYPE);
                  }
                else
                  {
                    pas_GenerateSetOperation(exprOpCodes.setOpCode);
                    simple1Type = exprBoolean;
                    handled     = true;
                  }
                break;

              case setMEMBER :
                /* FORM:  member 'in' set
                 *
                 * The parent of the set should be a subrange and
                 * the member should be an in-range ordinal value with
                 * same type as the base type of the subrange.
                 *
                 * A perverse case is when set is the empty set.  The
                 * result is always false in that case.
                 */

                if (abstract2Type == NULL)
                  {
                    if (simple2Type != exprEmptySet)
                      {
                        error(eHUH);
                      }
                    else
                      {
                        /* Check if a value is a member of an empty set???
                         *
                         * REVISIT:  This bogus logic but should provide the
                         * correct result.
                         */

                         pas_GenerateSimple(opDUP);
                         pas_GenerateSetOperation(exprOpCodes.setOpCode);
                         simple1Type = exprBoolean;
                         handled     = true;
                      }
                  }
                else
                  {
                    symbol_t *subRangePtr;

                    subRangePtr = pas_GetBaseTypePointer(abstract2Type);
                    if (subRangePtr->sParm.t.tType == sSET)
                      {
                        subRangePtr = subRangePtr->sParm.t.tParent;
                      }

                    if (subRangePtr->sParm.t.tType != sSUBRANGE)
                      {
                        error(eHUH);
                      }
                    else
                      {
                        uint16_t baseType = subRangePtr->sParm.t.tSubType;

                        if (simple1Type != pas_MapVariable2ExprType(baseType, true))
                          {
                            error(eEXPRTYPE);
                          }
                        else
                          {
                            /* Push the minimum value of the set-member.  This
                             * will be used to make the sub-range zero-based.
                             */

                            pas_GenerateDataOperation(opPUSH,
                              g_abstractTypePtr->sParm.t.tMinValue);

                            /* Then generate the set operation */

                            pas_GenerateSetOperation(exprOpCodes.setOpCode);
                            simple1Type = exprBoolean;
                            handled     = true;
                          }

                        g_abstractTypePtr = abstract1Type; /* Restore */
                      }
                  }
                  break;

              default :
                error(eHUH);
                g_abstractTypePtr = abstract1Type; /* Restore */
                break;
            }
        }
      else
        {
          /* Hmmm..  Some error occurred, either:
           *
           * 1. The first expression of '=', '<>", or "<=" was a SET, but
           *    the second is not.
           * 2. The second expression of 'IN' is not set.
           */

          error(eEXPRTYPE);
          g_abstractTypePtr = abstract1Type; /* Restore */
        }
    }

  /* Check for operations on strings next.  These may be:
   *
   *   FORM:  string-expression string-operator string-expression
   *          string-expression = standard-string-expression |
   *            short-string-expression
   *          string-operator = '=', '<>', '<', '<=', '>', '>='
   *
   * The set member may be any value for the sub-range of the ordinal type
   * that underlies the set.
   */

  if (exprOpCodes.strOpCode != opNOP && !handled)
    {
      /* Get the second simple expression (if we did not already) */

      if (!haveSimple2)
        {
          simple2Type = pas_SimpleExpression(findExprType);
          haveSimple2 = true;
        }

      /* Was the first expression a sting? */

      if (simple1Type == exprString)
        {
          /* What kind of string was the second expression? */

          if (simple2Type == exprString)
            {
              pas_StringLibraryCall(lbSTRCMP);
              pas_GenerateSimple(exprOpCodes.strOpCode);

              /* The resulting type is boolean */

              simple1Type = exprBoolean;
              handled     = true;
            }
          else if (simple2Type == exprChar)
            {
              /* Add the character of the second simple expression to the
               * string of the first expression.
               */

              pas_StringLibraryCall(lbSTRCATC);
              pas_GenerateSimple(exprOpCodes.strOpCode);

              /* The resulting type is boolean */

              simple1Type = exprBoolean;
              handled     = true;
            }
          else
            {
              error(eCOMPARETYPE);
            }
        }
    }

  /* There are a limited number of operations on CHAR and boolean types.  Of
   * course, they can always be converted to integer (with the CHR function)
   * for more extensive operations.
   */

  /* We need an exact type match for these */

  if (simple1Type == simple2Type && !handled)
    {
      if (exprOpCodes.charOpCode != opNOP && simple1Type == exprChar)
        {
          /* Generate the comparison */

          pas_GenerateSimple(exprOpCodes.charOpCode);

          /* The resulting type is boolean */

          simple1Type = exprBoolean;
          handled     = true;
        }
      else if (exprOpCodes.boolOpCode != opNOP && simple1Type == exprBoolean)
        {
          /* Generate the comparison */

          pas_GenerateSimple(exprOpCodes.boolOpCode);

          /* The resulting type is still boolean */

          handled = true;
        }
    }

  /* Deal with integer and real arithmetic */

  if ((exprOpCodes.intOpCode      != opNOP  ||
       exprOpCodes.wordOpCode     != opNOP  ||
       exprOpCodes.longIntOpCode  != opDNOP ||
       exprOpCodes.longWordOpCode != opDNOP) && !handled)
    {
      /* Get the second simple expression (if we did not already) */

      if (!haveSimple2)
        {
          getToken();
          simple2Type = pas_SimpleExpression(findExprType);
          haveSimple2 = true;
        }

      /* Perform automatic type conversion from INTEGER to REAL
       * for integer vs. real comparisons.
       */

      if (simple1Type != simple2Type)
        {
          /* Handle the case where the 1st argument is REAL and the
           * second is INTEGER.
           *
           * REVIST:  Needs to handle long integer conversions as well.
           */

          if (simple1Type  == exprReal         &&
              (simple2Type == exprInteger      ||
               simple2Type == exprWord         ||
               simple2Type == exprShortInteger ||
               simple2Type == exprShortWord)   &&
              exprOpCodes.fpOpCode     != fpINVLD)
            {
              exprOpCodes.fpOpCode   |= fpARG2;
              simple2Type = exprReal;
            }

          /* Handle the case where the 1st argument is Integer and the
           * second is REAL.
           *
           * REVIST:  Needs to handle long integer conversions as well.
           */

          else if ((simple1Type == exprInteger      ||
                    simple2Type == exprWord         ||
                    simple2Type == exprShortInteger ||
                    simple2Type == exprShortWord)   &&
                   simple2Type  == exprReal         &&
                   exprOpCodes.fpOpCode     != fpINVLD)
            {
              exprOpCodes.fpOpCode   |= fpARG1;
              simple1Type = exprReal;
            }

          /* Generic points (like NIL) should take the whatver pointer
           * type is needed.
           */

          else if ((simple1Type == exprAnyPointer &&
                    IS_POINTER_EXPRTYPE(simple2Type)))
            {
              simple1Type = simple2Type;
            }
          else if ((simple2Type == exprAnyPointer &&
                    IS_POINTER_EXPRTYPE(simple1Type)))
            {
              simple2Type = simple1Type;
            }

          /* The stack representation of integer and short integers is the
           * same... At least if they are both signed or unsigned.  Short
           * unsigned integers can also be safely treated as signed integers
           * due to the extended range of signed integers.  Other conversions
           * may be erroneous.
           */

          else if (simple1Type == exprInteger &&
                   (simple2Type == exprShortInteger ||
                    simple2Type == exprShortWord))
            {
              simple2Type = exprInteger;
            }
          else if ((simple1Type == exprShortInteger ||
                    simple1Type == exprShortWord) &&
                   simple2Type == exprInteger)
            {
              simple1Type = exprInteger;
            }

          /* Otherwise, the two terms must agree in type */

          else
            {
              error(eEXPRTYPE);
            }
        }

      /* Generate the comparison */

      if (simple1Type == exprReal)
        {
          if (exprOpCodes.fpOpCode == fpINVLD)
            {
              error(eEXPRTYPE);
            }
          else
            {
              pas_GenerateFpOperation(exprOpCodes.fpOpCode);

              /* The resulting type is boolean */

              simple1Type = exprBoolean;
              handled     = true;
            }
        }
      else if ((simple1Type == exprInteger ||
                simple1Type == exprShortInteger) &&
               exprOpCodes.intOpCode != opNOP)
        {
          pas_GenerateSimple(exprOpCodes.intOpCode);

          /* The resulting type is boolean */

          simple1Type = exprBoolean;
          handled     = true;
        }
      else if ((simple1Type == exprLongInteger) &&
               exprOpCodes.longIntOpCode != opDNOP)
        {
          pas_GenerateSimpleLongOperation(exprOpCodes.longIntOpCode);

          /* The resulting type is boolean */

          simple1Type = exprBoolean;
          handled     = true;
        }
      else if ((simple1Type == exprWord ||
                simple1Type == exprShortWord ||
                simple1Type == exprScalar) &&
               exprOpCodes.wordOpCode != opNOP)
        {
          pas_GenerateSimple(exprOpCodes.wordOpCode);

          /* The resulting type is boolean */

          simple1Type = exprBoolean;
          handled     = true;
        }
      else if ((simple1Type == exprLongWord) &&
               exprOpCodes.longWordOpCode != opDNOP)
        {
          pas_GenerateSimpleLongOperation(exprOpCodes.longWordOpCode);

          /* The resulting type is boolean */

          simple1Type = exprBoolean;
          handled     = true;
        }
      else if (IS_POINTER_EXPRTYPE(simple1Type) &&
               exprOpCodes.ptrOpCode != opNOP)
        {
          pas_GenerateSimple(exprOpCodes.ptrOpCode);

          /* The resulting type is boolean */

          simple1Type = exprBoolean;
          handled     = true;
        }
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
   *    we found a string expression.
   *
   * Special case:
   *
   *    We will perform automatic conversions to real from integer
   *    if the requested type is a real expression.
   *
   * Condition:
   * 1) NOT Any expression
   * 2) NOT Matched expression
   * 3) NOT any ordinal type OR type is NOT ordinal
   * 4) NOT any string type  OR type is NOT string
   * 5) NOT looking for string ref OR type is NOT string ref
   * 6) NOT looking for a pointer type OR type is not any pointer type.
   */

  if (findExprType != exprUnknown &&
      findExprType != simple1Type &&
      (findExprType != exprAnyOrdinal ||
       !pas_IsOrdinalExpression(simple1Type)) &&
      (findExprType != exprAnyPointer ||
       !IS_POINTER_EXPRTYPE(simple1Type)))
    {
      /* Automatic conversions from INTEGER to REAL will be performed */

      if (findExprType == exprReal && simple1Type == exprInteger)
        {
          pas_GenerateFpOperation(fpFLOAT);
          simple1Type = exprReal;
        }

      /* Outside of this logic, an empty set is just treated like a set. */

      else if (simple1Type == exprEmptySet)
        {
          simple1Type = exprSet;
        }

      /* The NIL pointer is treated like whatever pointer you the caller
       * needs.
       */

      else if (IS_POINTER_EXPRTYPE(findExprType) &&
               simple1Type == exprAnyPointer)
        {
          simple1Type = findExprType;
        }

      /* If the caller needs a string type and a bare character type was
       * found, then we should* convert that character to a string.
       */

      else if (findExprType == exprString && simple1Type == exprChar)
        {
          /* Expand the character to a string on the string stack.  And
           * change the expression type to reflect this.
           */

          pas_StringLibraryCall(lbMKSTKC);
          simple1Type = exprString;
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
  g_abstractTypePtr = typePtr;

  /* This function is really just an interface to the static function
   * pas_PointerFactor with some extra error checking.
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

void pas_ArrayIndex(symbol_t *arrayTypePtr)
{
  /* Parse the index-type-list.
   *
   *   FORM:  array-type = 'array' '[' index-type-list ']' 'of' type-denoter
   *   FORM:  index-type-list = index-type { ',' index-type }
   *
   * On entry 'g_token' should refer to the '[' token.
   */

  if (g_token != '[') error(eLBRACKET);
  else
    {
      symbol_t *indexTypePtr = arrayTypePtr->sParm.t.tIndex;
      uint16_t  dimension    = 1;
      uint16_t  elemSize;

      do
        {
          exprType_t exprType;
          uint16_t   indexType;
          uint16_t   offset;

          /* Sanity checks */

          if (dimension > arrayTypePtr->sParm.t.tDimension)
            {
              /* Program apparently has more indices that dimensions. */

              error(eTOOMANYINDICES);
            }
          else if (indexTypePtr == NULL)
            {
              /* Not enough index types for dimensionality of the array.
               * This should never happen.
               */

              error(eHUH);
            }

          /* Get the type of the index */

          if (indexTypePtr->sKind != sTYPE)
            {
              error(eINDEXTYPE);
              exprType = exprUnknown;
            }
          else
            {
              indexType = indexTypePtr->sParm.t.tType;

              /* REVISIT:  For subranges, we use the base type of the
               * subrange.
               */

              if (indexType == sSUBRANGE)
                {
                  indexType = indexTypePtr->sParm.t.tSubType;
                }

              /* Get the expression type from the index type */

              exprType = pas_MapVariable2ExprType(indexType, true);
            }

          /* Skip over the initial '[' or subsequent ',' and evaluate the
           * index expression.
           */

          getToken();
          pas_Expression(exprType, NULL);

          /* We now have the array element at the top of the stack.  If the
           * index is not zero-based, the we need to offset the index value
           * so that it is.
           */

          offset = indexTypePtr->sParm.t.tMinValue;
          if (offset != 0)
            {
              pas_GenerateDataOperation(opPUSH, offset);
              pas_GenerateSimple(opSUB);
            }

          /* The first index is in units of the base type of the elements of
           * array.  But the next index is in units of the index range of the
           * first element times the size of the base type.
           *
           * We need to multiply the zero-based index by the element size
           * (unless, of course, the element size is one).
           */

          elemSize = indexTypePtr->sParm.t.tAllocSize;
          if (elemSize != 1)
            {
              pas_GenerateDataOperation(opPUSH, elemSize);
              pas_GenerateSimple(opMUL);
            }

          /* If this is not the first dimension, then we need to add the
           * offset that we just calculated to the offset calculated from
           * the previous dimension.
           */

          if (dimension > 1)
            {
              pas_GenerateSimple(opADD);
            }

          /* Set up for the next time through the loop.  */

          indexTypePtr = indexTypePtr->sParm.t.tIndex;
          dimension++;
        }
      while (g_token == ',');

      /* Verify that a right bracket terminates the index-type-list. */

      if (g_token !=  ']') error(eRBRACKET);
      else getToken();
    }
}

/****************************************************************************/
/* Determine the expression type associated with a pointer to a type
 * symbol
 */

exprType_t pas_GetExpressionType(symbol_t *sType)
{
  exprType_t factorType = sINT;

  if (sType != NULL && sType->sKind == sTYPE)
    {
      switch (sType->sParm.t.tType)
        {
        case sINT :
          factorType = exprInteger;
          break;

        case sWORD :
          factorType = exprWord;
          break;

        case sSHORTINT :
          factorType = exprShortInteger;
          break;

        case sSHORTWORD :
          factorType = exprShortWord;
          break;

        case sLONGINT :
          factorType = exprLongInteger;
          break;

        case sLONGWORD :
          factorType = exprLongWord;
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

            case sWORD :
              factorType = exprWord;
              break;

            case sSHORTINT :
              factorType = exprShortInteger;
              break;

            case sSHORTWORD :
              factorType = exprShortWord;
              break;

            case sLONGINT :
              factorType = exprLongInteger;
              break;

            case sLONGWORD :
              factorType = exprLongWord;
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

                case sWORD :
                  factorType = exprWordPtr;
                  break;

                case sSHORTINT :
                  factorType = exprShortIntegerPtr;
                  break;

                case sSHORTWORD :
                  factorType = exprShortWordPtr;
                  break;

                case sLONGINT :
                  factorType = exprLongIntegerPtr;
                  break;

                case sLONGWORD :
                  factorType = exprLongWordPtr;
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

/****************************************************************************/

exprType_t pas_MapVariable2ExprType(uint16_t varType, bool ordinal)
{
  switch (varType)
    {
      /* Ordinal type mappings */

      case sINT :
      case sSUBRANGE :
        return exprInteger;           /* signed integer value */

      case sWORD :
        return exprWord;              /* unsigned integer value */

      case sSHORTINT :
        return exprShortInteger;      /* signed short integer value */

      case sSHORTWORD :
        return exprShortWord;         /* unsigned short integer value */

      case sLONGINT :
        return exprLongInteger;       /* signed long integer value */

      case sLONGWORD :
        return exprLongWord;          /* unsigned long integer value */

      case sCHAR :
        return exprChar;              /* character value */

      case sBOOLEAN :
        return exprBoolean;           /* boolean(integer) value */

      case sSCALAR :
      case sSCALAR_OBJECT :
        return exprScalar;            /* scalar(integer) value */

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

                case sSET :
                  return exprSet;     /* set(integer) value */

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
/* The base type of complex, defined type */

symbol_t *pas_GetBaseTypePointer(symbol_t *typePtr)
{
  symbol_t  *baseTypePtr;
  symbol_t  *nextTypePtr;

  /* Get a pointer to the underlying base type symbol */

  baseTypePtr     = typePtr;
  nextTypePtr     = typePtr->sParm.t.tParent;

  /* Loop until the terminal type is found.  Exception:  A SET is not really
   * reducible.  The parent type of the sSET characterizes the SEt but is
   * not the base type of the set (which will be a sub-range or a scalar).
   */

  while (nextTypePtr != NULL &&
         nextTypePtr->sKind == sTYPE &&
         baseTypePtr->sParm.t.tType != sSET)
    {
      baseTypePtr = nextTypePtr;
      nextTypePtr = baseTypePtr->sParm.t.tParent;
    }

  return baseTypePtr;
}
