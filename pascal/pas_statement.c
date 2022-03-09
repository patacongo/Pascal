/****************************************************************************
 * pas_statement.c
 * Pascal Statements
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
#include <inttypes.h>

#include "pas_debug.h"
#include "pas_defns.h"
#include "pas_tkndefs.h"
#include "pas_pcode.h"
#include "pas_errcodes.h"
#include "pas_library.h"

#include "pas_main.h"
#include "pas_statement.h"
#include "pas_procedure.h"
#include "pas_function.h"   /* For pas_StandardFunctionOfConstant() */
#include "pas_expression.h"
#include "pas_codegen.h"
#include "pas_token.h"
#include "pas_symtable.h"
#include "pas_insn.h"
#include "pas_error.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Assignment flags.  These options apply primarily for complex assignments
 * involving ARRAYs, POINTERs, and VAR parameters.  The simplests assignment
 * form (no flags) is:
 *
 *     <expression>   - Push expression value
 *     opSTS          - Store to variable address
 *
 * ASSIGN_DEREFERENCE (only)
 * - Means to load the address of a pointer (LDS), then store to the
 *   expression value to that address(STI).  For example, assignment of a
 *   value to the target address of a pointer.
 *
 *     <address>      - Target address of pointer
 *     <expression>   - Push expression value
 *     opSTI          - Save to the address
 *
 * ASSIGN_INDEXED (only)
 * - Save value to indexed stack address(STSX)
 *
 *     <expression>   - Push expression value
 *     <index-offset> - Address offset derived from the array index
 *     opSTSX         - Save to the indexed element of the array
 *
 * ASSIGN_DEREFERENCE + ASSIGN_INDEXED
 * - The pointer address and index address offset are on the stack.  We need
 *   to load address first with index (LDSX), then store value to that address
 *   (STI).  For example, assignment of an expression value to a pointer to
 *   array (such as a VAR parameter).
 *
 *     <address>      - Target address of pointer
 *     <index-offset> - Address offset derived from the array index
 *     opLDSX         - Load address from indexed pointer to an array.
 *     <expression>   - Push expression value
 *     opSTI          - Save to the address
 *
 * ASSIGN_DEREFERENCE + ASSIGN_INDEXED + ASSIGN_STORE_INDEXED
 * - The pointer address and index address offset are on the stack.  We need
 *   To add the offset to the address to get the address to store the result.
 *   For example, assignment to an expression that is an array of pointers
 *
 *     <expression>   - Push expression value
 *     <address>      - Target address of pointer
 *     <index-offset> - Address offset derived from the array index
 *     opADD          - Get the address from index into an array of pointers
 *     opSTI          - Save the expression to that address
 *
 * ASSIGN_ADDRESS
 * - Assign a pointer address, rather than a value.  The only effect is to
 *   assume a pointer expression rather than a value expression.
 *
 * ASSIGN_VAR_PARM
 * - Does very little differently compared to ASSIGN_DEREFERENCE, but
 *   distinguish between if we are working with a pointer or with a VAR
 *   parameter.
 *
 * ASSIGN_LVALUE_ADDR
 * - LValue address was pushed on stack BEFORE the RValue expression.
 *   The is necessary when the LValue is complex.  For example,
 *   ptr^.next^.next^.value = expression.
 *
 *     opLDS          - Pointer target address
 *    [opLDI          - Pointer-to-pointer target address]
 *     <expression>   - Push expression value
 *     opSTI          - Save to the indexed element of the array
 *
 * ASSIGN_LVALUE_ADDR + ASSIGN_INDEXED
 * - LValue address was pushed on stack BEFORE the RValue expression.
 *   The is necessary when the LValue is complex.  For example,
 *   ptr^.next^.next^.value = expression.
 *
 *     opLDS          - Pointer target address
 *    [opLDI          - Pointer-to-pointer target address]
 *     <expression>   - Push expression value
 *     opXCHG         - Change ordering on stack
 *     <index-offset> - Address offset derived from the array index
 *     opADD          - Get the address from index into an array of pointers
 *     opSTI          - Save to the indexed element of the array
 *
 * ASSIGN_PTR2PTR
 * - LValue is a pointer to a pointer.
 */

#define ASSIGN_DEREFERENCE   (1 << 0)
#define ASSIGN_ADDRESS       (1 << 1)
#define ASSIGN_INDEXED       (1 << 2)
#define ASSIGN_STORE_INDEXED (1 << 3)
#define ASSIGN_OUTER_INDEXED (1 << 4)
#define ASSIGN_VAR_PARM      (1 << 5)
#define ASSIGN_LVALUE_ADDR   (1 << 6)
#define ASSIGN_PTR2PTR       (1 << 7)

#define IS_CONSTANT(x) \
        (  ((x) == tINT_CONST) \
        || ((x) == tBOOLEAN_CONST) \
        || ((x) == tCHAR_CONST) \
        || ((x) == tREAL_CONST) \
        || ((x) == sSCALAR_OBJECT))

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Assignment Statements */

static void       pas_ComplexAssignment(void);
static void       pas_SimpleAssignment (symbol_t *varPtr,
                                        uint8_t assignFlags);
static void       pas_Assignment       (uint16_t storeOp,
                                        exprType_t assignType,
                                        symbol_t *varPtr, symbol_t *typePtr);
static void       pas_PointerAssignment(symbol_t *varPtr, symbol_t *typePtr,
                                        uint8_t assignFlags);
static void       pas_StringAssignment (symbol_t *varPtr, symbol_t *typePtr,
                                        uint8_t assignFlags);
static void       pas_LargeAssignment  (uint16_t storeOp,
                                        exprType_t assignType,
                                        symbol_t *varPtr, symbol_t *typePtr);
static void       pas_ArrayAssignment  (symbol_t *varPtr, symbol_t *typePtr,
                                        uint8_t assignFlags);
static exprType_t pas_AssignExprType   (exprType_t baseExprType,
                                        uint8_t assignFlags);

/* Other Statements */

static void       pas_GotoStatement    (void);  /* GOTO statement */
static void       pas_LabelStatement   (void);  /* Label statement */
static void       pas_ProcStatement    (void);  /* Procedure method statement */
static void       pas_IfStatement      (void);  /* IF-THEN[-ELSE] statement */
static void       pas_CaseStatement    (void);  /* Case statement */
static void       pas_RepeatStatement  (void);  /* Repeat statement */
static void       pas_WhileStatement   (void);  /* While statement */
static void       pas_ForStatement     (void);  /* For statement */
static void       pas_WithStatement    (void);  /* With statement */

/****************************************************************************/

void pas_Statement(void)
{
  symbol_t  *symPtr;     /* Save Symbol Table pointer to token */
  exprType_t exprType;

  /* Generate file/line number pseudo-operation to facilitate P-Code testing */

  pas_GenerateLineNumber(FP->include, FP->line);

  /* We will push the string stack pointer at the beginning of each
   * statement and pop the string stack pointer at the end of each
   * statement.  Subsequent optimization logic will scan the generated
   * pcode to ascertain if the push and pops were necessary.  They
   * would be necessary if expression parsing generated temporary usage
   * of string stack storage.  In this case, the push will save the
   * value before the temporary usage and the pop will release the
   * temporaray storage.
   */

  pas_GenerateSimple(opPUSHS);

  /* Process the statement according to the type of the leading token */

  switch (g_token)
    {
      /* Simple, ordinal assignment statements */

    case sINT :
    case sWORD :
    case sBOOLEAN :
      symPtr = g_tknPtr;
      exprType = pas_MapVariable2ExprType(g_token, true);
      getToken();
      pas_Assignment(opSTS, exprType, symPtr, symPtr->sParm.v.vParent);
      break;

    case sSHORTINT :
    case sSHORTWORD :
    case sCHAR :
      symPtr = g_tknPtr;
      exprType = pas_MapVariable2ExprType(g_token, true);
      getToken();
      pas_Assignment(opSTSB, exprType, symPtr, symPtr->sParm.v.vParent);
      break;

    case sLONGINT :
    case sLONGWORD :
      symPtr = g_tknPtr;
      exprType = pas_MapVariable2ExprType(g_token, true);
      getToken();
      pas_LargeAssignment(opSTSM, exprType, symPtr, symPtr->sParm.v.vParent);
      break;

      /* The only thing that SETs and REAL have in common is that they both
       * require larger, multi-word assignments.  Same for long integers/word,
       * but those are grouped with the ordinal types.
       */

    case sSET :
    case sREAL :
      symPtr = g_tknPtr;
      exprType = pas_MapVariable2ExprType(g_token, false);
      getToken();
      pas_LargeAssignment(opSTSM, exprType, symPtr, symPtr->sParm.v.vParent);
      break;

    case sSCALAR :
      symPtr = g_tknPtr;
      getToken();
      pas_Assignment(opSTS, exprScalar, symPtr, symPtr->sParm.v.vParent);
      break;

    case sSTRING :
      symPtr = g_tknPtr;
      getToken();
      pas_StringAssignment(symPtr, symPtr->sParm.v.vParent, 0);
      break;

      /* Complex assignments statements */

    case sSUBRANGE :
    case sRECORD :
    case sRECORD_OBJECT :
    case sPOINTER :
    case sVAR_PARM :
    case sARRAY :
      pas_ComplexAssignment();
      break;

      /* Branch, Call and Label statements */

    case sPROC         : pas_ProcStatement(); break;
    case tGOTO         : pas_GotoStatement(); break;
    case tINT_CONST    : pas_LabelStatement(); break;

      /* Conditional Statements */

    case tIF           : pas_IfStatement(); break;
    case tCASE         : pas_CaseStatement(); break;

      /* Loop Statements */

    case tREPEAT       : pas_RepeatStatement(); break;
    case tWHILE        : pas_WhileStatement(); break;
    case tFOR          : pas_ForStatement(); break;

      /* Other Statements */

    case tBEGIN        : pas_CompoundStatement(); break;
    case tWITH         : pas_WithStatement(); break;

      /* None of the above, try standard procedures */

    default            : pas_StandardProcedure(); break;
  }

  /* Generate the POPS that matches the PUSHS generated at the begining
   * of this function (see comments above).
   */

  pas_GenerateSimple(opPOPS);
}

/***********************************************************************/
/* Process a complex assignment statement */

static void pas_ComplexAssignment(void)
{
   symbol_t symbolSave;

   /* FORM:  <variable OR function identifer> := <expression>
    * First, make a copy of the symbol table entry because the call to
    * pas_SimpleAssignment() will modify it.
    */

   symbolSave = *g_tknPtr;
   getToken();

   /* Then process the complex assignment until it is reduced to a simple
    * assignment (like int, char, etc.)
    */

   pas_SimpleAssignment(&symbolSave, 0);
}

/***********************************************************************/
/* Process a complex assignment (recursively) until it becomes a
 * simple assignment statement.
 *
 * Called only from pas_ComplexAssignment() (and recursively) with abort
 * snapshot of the symbol on the stack.  Hence, it is safe to modify the
 * content of the structure reffered to by varPtr.
 */

static void pas_SimpleAssignment(symbol_t *varPtr, uint8_t assignFlags)
{
  symbol_t   *typePtr;
  exprType_t  exprType;

  /* FORM:  <variable OR function identifer> := <expression> */

  /* Get the parent type */

  typePtr = varPtr->sParm.v.vParent;

  /* Now, handle the variable by its type */

  switch (varPtr->sKind)
    {
      /* Check if we have reduce the complex assignment to a simple
       * assignment yet
       */

    case sINT :
    case sWORD :
    case sBOOLEAN :
      exprType = pas_MapVariable2ExprType(varPtr->sKind, true);
      exprType = pas_AssignExprType(exprType, assignFlags);

      /* Check for indexed variants */

      if ((assignFlags & (ASSIGN_INDEXED | ASSIGN_OUTER_INDEXED)) != 0)
        {
          /* Are we assigning to a pointer to an array? Or to an array
           * of pointers?
           */

          if ((assignFlags & ASSIGN_DEREFERENCE) != 0)
            {
              if ((assignFlags & ASSIGN_STORE_INDEXED) != 0)
                {
                  /* The pointer value and the index value both on the stack.
                   * Expect:
                   *
                   *   TOS(0)  <index-offset> Address offset derived from
                   *                          the array index
                   *   TOS(2)  <expression>   Evaluated expression
                   */

                  pas_GenerateSimple(opADD);
                }
              else
                {
                  /* Expect:
                   *
                   *   TOS(0)  <index-offset> Address offset derived from
                   *                          the array index
                   *   TOS(1)  <expression>   Evaluated expression
                   */

                  pas_GenerateStackReference(opLDSX, varPtr);
                }

              pas_Assignment(opSTI, exprType, varPtr, typePtr);
            }
          else if ((assignFlags & ASSIGN_LVALUE_ADDR) != 0)
            {
              /* Expect:
               *
               *   TOS(1)  <expression>   Evaluated expression
               *   TOS(0)  <index-offset> Address offset derived from
               *                          the array index
               *   TOS(2)  <address>      Target address of pointer
               */

              pas_Assignment(opSTI, exprType, varPtr, typePtr);
            }
          else
            {
              /* Expect:
               *
               *   TOS(0)  <expression>   Push expression value
               *   TOS(1)  <index-offset> Address offset derived from the
               *                          array index
               */

              pas_Assignment(opSTSX, exprType, varPtr, typePtr);
            }
        }

      /* Not indexed */

      else
        {
          if ((assignFlags & (ASSIGN_DEREFERENCE | ASSIGN_LVALUE_ADDR)) != 0)
            {
              /* Address of pointer is on the stack.
               * Expect:
               *
               *   TOS(0)  <expression>   Evaluated LValue expression
               *   TOS(1)  <address>      Target address of pointer
               */

              pas_Assignment(opSTI, exprType, varPtr, typePtr);
            }
          else
            {
              /* Use the variable address.
               * Expect only the evaluated expression at the top of the stack.
               */

              pas_Assignment(opSTS, exprType, varPtr, typePtr);
            }
        }
      break;

    case sSHORTINT :
    case sSHORTWORD :
    case sCHAR :
      exprType = pas_MapVariable2ExprType(varPtr->sKind, true);
      exprType = pas_AssignExprType(exprType, assignFlags);

      /* Check for indexed variants */

      if ((assignFlags & ASSIGN_INDEXED) != 0)
        {
          /* Are we assigning to a pointer to an array? Or to an array
           * of pointers?
           */

          if ((assignFlags & ASSIGN_DEREFERENCE) != 0)
            {
              if ((assignFlags & ASSIGN_STORE_INDEXED) != 0)
                {
                  /* The pointer value and the index value both on the stack.
                   * Expect:
                   *
                   *   TOS(0)  <index-offset> Address offset derived from
                   *                          the array index
                   *   TOS(2)  <expression>   Evaluated expression
                   */

                  pas_GenerateStackReference(opLDS, varPtr);
                  pas_GenerateSimple(opADD);
                }
              else
                {
                  /* Expect:
                   *
                   *   TOS(0)  <index-offset> Address offset derived from
                   *                          the array index
                   *   TOS(1)  <expression>   Evaluated expression
                   */

                  pas_GenerateStackReference(opLDSX, varPtr);
                }

              pas_Assignment(opSTIB, exprType, varPtr, typePtr);
            }
          else if ((assignFlags & ASSIGN_LVALUE_ADDR) != 0)
            {
              /* Expect:
               *
               *   TOS(1)  <expression>   Evaluated expression
               *   TOS(0)  <index-offset> Address offset derived from
               *                          the array index
               *   TOS(2)  <address>      Target address of pointer
               */

              pas_Assignment(opSTIB, exprType, varPtr, typePtr);
            }
          else if ((assignFlags & ASSIGN_ADDRESS) != 0)
            {
              /* Expect:
               *
               *   TOS(0)  <expression>   Push expression value
               *   TOS(1)  <index-offset> Address offset derived from the
               *                          array index
               */

              pas_Assignment(opSTSX, exprType, varPtr, typePtr);
            }
          else
            {
              pas_Assignment(opSTSXB, exprType, varPtr, typePtr);
            }
        }

      /* Not indexed */

      else
        {
          if ((assignFlags & ASSIGN_DEREFERENCE) != 0)
            {
              if ((assignFlags & ASSIGN_STORE_INDEXED) != 0)
                {
                  /* Address of pointer is on the stack.
                   * Expect:
                   *
                   *   TOS(0)  <expression>   Evaluated LValue expression
                   *   TOS(1)  <address>      Target address of pointer
                   */

                  pas_GenerateStackReference(opLDS, varPtr);
                  pas_GenerateSimple(opADD);
                }
              else
                {
                  pas_GenerateStackReference(opLDS, varPtr);
                }

              pas_Assignment(opSTIB, exprType, varPtr, typePtr);
            }
          else if ((assignFlags & ASSIGN_LVALUE_ADDR) != 0)
            {
              /* Address of pointer is on the stack.
               * Expect:
               *
               *   TOS(0)  <expression>   Evaluated LValue expression
               *   TOS(1)  <address>      Target address of pointer
               */

              pas_Assignment(opSTIB, exprType, varPtr, typePtr);
            }
          else if ((assignFlags & ASSIGN_ADDRESS) != 0)
            {
              /* Use the variable address.
               * Expect only the evaluated pointer expression at the top of
               * the stack.
               */

              pas_Assignment(opSTS, exprType, varPtr, typePtr);
            }
          else
            {
              /* Use the variable address.
               * Expect only the evaluated expression at the top of the stack.
               */

              pas_Assignment(opSTSB, exprType, varPtr, typePtr);
            }
        }
      break;

    /* The only thing that Long integer/word, REALc and SET types have in
     * common is that they all both require the same multi-word assignment.
     */

    case sLONGINT :
    case sLONGWORD :
    case sSET :
    case sREAL :
      exprType = pas_MapVariable2ExprType(varPtr->sKind, false);
      exprType = pas_AssignExprType(exprType, assignFlags);

      /* Check for indexed variants */

      if ((assignFlags & ASSIGN_INDEXED) != 0)
        {
          /* Are we assigning to a pointer to an array? Or to an array
           * of pointers?
           */

          if ((assignFlags & ASSIGN_DEREFERENCE) != 0)
            {
              if ((assignFlags & ASSIGN_STORE_INDEXED) != 0)
                {
                  /* The pointer value and the index value both on the stack.
                   * Expect:
                   *
                   *   TOS(0)  <index-offset> Address offset derived from
                   *                          the array index
                   *   TOS(2)  <expression>   Evaluated expression
                   */

                  pas_GenerateStackReference(opLDS, varPtr);
                  pas_GenerateSimple(opADD);
                }
              else
                {
                  /* Expect:
                   *
                   *   TOS(0)  <index-offset> Address offset derived from
                   *                          the array index
                   *   TOS(1)  <expression>   Evaluated expression
                   */

                  pas_GenerateStackReference(opLDSX, varPtr);
                }

              pas_LargeAssignment(opSTIM, exprType, varPtr, typePtr);
            }
          else if ((assignFlags & ASSIGN_LVALUE_ADDR) != 0)
            {
              /* Expect:
               *
               *   TOS(1)  <expression>   Evaluated expression
               *   TOS(0)  <index-offset> Address offset derived from
               *                          the array index
               *   TOS(2)  <address>      Target address of pointer
               */

              pas_LargeAssignment(opSTIM, exprType, varPtr, typePtr);
            }
          else if ((assignFlags & ASSIGN_ADDRESS) != 0)
            {
              pas_Assignment(opSTSX, exprType, varPtr, typePtr);
            }
          else
            {
              pas_LargeAssignment(opSTSXM, exprType, varPtr, typePtr);
            }
        }

      /* Not indexed */

      else
        {
          if ((assignFlags & ASSIGN_DEREFERENCE) != 0)
            {
              /* Address of pointer is on the stack.
               * Expect:
               *
               *   TOS(0)  <expression>   Evaluated LValue expression
               *   TOS(1)  <address>      Target address of pointer
               */

              pas_GenerateStackReference(opLDS, varPtr);
              pas_LargeAssignment(opSTIM, exprType, varPtr, typePtr);
            }
          else if ((assignFlags & ASSIGN_LVALUE_ADDR) != 0)
            {
              /* Address of pointer is on the stack.
               * Expect:
               *
               *   TOS(0)  <expression>   Evaluated LValue expression
               *   TOS(1)  <address>      Target address of pointer
               */

              pas_LargeAssignment(opSTIM, exprType, varPtr, typePtr);
             }
          else if ((assignFlags & ASSIGN_ADDRESS) != 0)
            {
              /* Use the variable address.
               * Expect only the evaluated pointer expression at the top of
               * the stack.
               */

              pas_Assignment(opSTS, exprType, varPtr, typePtr);
            }
          else
            {
              /* Use the variable address.
               * Expect only the evaluated expression at the top of the stack.
               */

              pas_LargeAssignment(opSTSM, exprType, varPtr, typePtr);
            }
        }
      break;

    case sSCALAR :
      exprType = pas_AssignExprType(exprScalar, assignFlags);
      if ((assignFlags & ASSIGN_INDEXED) != 0)
        {
          if ((assignFlags & ASSIGN_DEREFERENCE) != 0)
            {
              if ((assignFlags & ASSIGN_STORE_INDEXED) != 0)
                {
                  pas_GenerateStackReference(opLDS, varPtr);
                  pas_GenerateSimple(opADD);
                }
              else
                {
                  pas_GenerateStackReference(opLDSX, varPtr);
                }

              pas_Assignment(opSTI, exprType, varPtr, typePtr);
            }
          else
            {
              pas_Assignment(opSTSX, exprType, varPtr, typePtr);
            }
        }
      else
        {
          if ((assignFlags & ASSIGN_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              pas_Assignment(opSTI, exprType, varPtr, typePtr);
            }
          else
            {
              pas_Assignment(opSTS, exprType, varPtr, typePtr);
            }
        }
      break;

      /* NOPE... recurse until it becomes a simple assignment */

    case sSUBRANGE :
      varPtr->sKind = typePtr->sParm.t.tSubType;
      pas_SimpleAssignment(varPtr, assignFlags);
      break;

    case sSTRING :
      pas_StringAssignment(varPtr, typePtr, assignFlags);
      break;

    case sRECORD :
      /* FORM: <record identifier>.<field> := <expression>
       * OR:   <record pointer identifier> := <pointer expression>
       */

      /* Check if we are assigning as address to a pointer to a record */

      if ((assignFlags & ASSIGN_ADDRESS) != 0)
        {
          if (g_token == '.') error(ePOINTERTYPE);

          if ((assignFlags & (ASSIGN_INDEXED | ASSIGN_OUTER_INDEXED)) != 0)
            {
              pas_Assignment(opSTSX, exprRecordPtr, varPtr, typePtr);
            }
          else if ((assignFlags & (ASSIGN_DEREFERENCE | ASSIGN_VAR_PARM |
                                   ASSIGN_LVALUE_ADDR)) != 0)
            {
              /* In these cases, the destination, LValue address of the
               * assignment is on the stack.
               */

              pas_Assignment(opSTI, exprRecordPtr, varPtr, typePtr);
            }
          else
            {
              pas_Assignment(opSTS, exprRecordPtr, varPtr, typePtr);
            }
        }

      /* We are either assigning a value to a field of the record variable
       * or, perhaps, we are de-referencing a pointer to a RECORD (we can
       * distinguish these cases by settings in the assignFlags).  In
       * either case, a RECORD field selector should follow.
       *
       * Check if a period separates the RECORD from the record field
       * selector.
       */

      else if (g_token == '.')
        {
          symbol_t *baseTypePtr;

          /* Get a pointer to the underlying base type symbol */

          baseTypePtr = pas_GetBaseTypePointer(typePtr);

          /* Skip over the period */

          getToken();

          /* Verify that a field identifier associated with this record
           * follows the period.
           */

          if (g_token != sRECORD_OBJECT ||
              g_tknPtr->sParm.r.rRecord != baseTypePtr)
            {
              error(eRECORDOBJECT);
            }
          else
            {
              /* Modify the variable so that it has the characteristics of the
               * the field but with level and offset associated with the record
               */

              typePtr                 = g_tknPtr->sParm.r.rParent;
              varPtr->sKind           = typePtr->sParm.t.tType;
              varPtr->sParm.v.vParent = typePtr;

              /* REVISIT:  If this is a string VAR parm, then we already
               * made a mistake:  Strings are handled differently; the value
               * is assigned via STRCPY and not via a store to an address.
               * But we already screwed up while processing the VAR parameter
               * before we need it was a string field of a RECORD.  The
               * following is a hack that might undo that mistake.
               */

              if (varPtr->sKind == sSTRING &&
                  (assignFlags & ASSIGN_VAR_PARM) != 0)
                {
                  assignFlags |= ASSIGN_LVALUE_ADDR;
                }

              /* Adjust the variable size and offset.  Add the RECORD offset
               * to the RECORD data stack offset to get the data stack
               * offset to the record object; Change the size to match the
               * size of RECORD object.
               */

              varPtr->sParm.v.vSize   = g_tknPtr->sParm.r.rSize;

              /* Special case:  The record is a de-referenced pointer or a
               * VAR parameter.
               */

              if ((assignFlags & (ASSIGN_DEREFERENCE | ASSIGN_VAR_PARM |
                                  ASSIGN_LVALUE_ADDR)) != 0)
                {
                  /* Add the offset to the record field to the RECORD address
                   * that should already be on the stack.
                   */

                  pas_GenerateDataOperation(opPUSH, g_tknPtr->sParm.r.rOffset);
                  pas_GenerateSimple(opADD);
                }
              else
                {
                  /* Add the offset to RECORD object to RECORD data stack
                   * offset.
                   */

                  varPtr->sParm.v.vOffset += g_tknPtr->sParm.r.rOffset;
                }

              /* The RECORD OBJECT should not be indexed, even if the "outer",
               * destination RECORD must be.
               */

              if ((assignFlags & ASSIGN_INDEXED) != 0)
                {
                  assignFlags &= ~ASSIGN_INDEXED;
                  assignFlags |= ASSIGN_OUTER_INDEXED;
                }

              /* Recurse to handle the RECORD OBJECT */

              getToken();
              pas_SimpleAssignment(varPtr, assignFlags);
            }
        }

      /* It must be a RECORD assignment */

      else
        {
          /* Special case:  The record is a VAR parameter. */

          if ((assignFlags & ASSIGN_VAR_PARM) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              pas_GenerateSimple(opADD);
              pas_LargeAssignment(opSTIM, exprRecord, varPtr, typePtr);
            }
          else
            {
              pas_LargeAssignment(opSTSM, exprRecord, varPtr, typePtr);
            }
        }
      break;

    case sRECORD_OBJECT :
      {
        /* FORM: <field> := <expression>
         * NOTE:  This must have been preceeded with a WITH statement
         * defining the RECORD type
         */

        symbol_t *baseTypePtr;

        /* Get a pointer to the underlying type of the RECORD */

        baseTypePtr = pas_GetBaseTypePointer(varPtr->sParm.r.rRecord);
        if (baseTypePtr->sParm.t.tType != sRECORD)
          {
            error(eRECORDTYPE);
          }
        else if (!g_withRecord.wParent)
          {
            error(eINVTYPE);
          }
        else if ((assignFlags && (ASSIGN_DEREFERENCE | ASSIGN_ADDRESS |
                                  ASSIGN_LVALUE_ADDR)) != 0)
          {
            error(ePOINTERTYPE);
          }
        else if ((assignFlags && ASSIGN_INDEXED) != 0)
          {
            error(eARRAYTYPE);
          }

        /* Verify that a field identifier is associated with the RECORD
         * specified by the WITH statement.
         */

        else if (g_withRecord.wParent != baseTypePtr)
          {
            error(eRECORDOBJECT);
          }
        else
          {
            int16_t tempOffset;

            /* Now there are two cases to consider:  (1) the g_withRecord is
             * a pointer to a RECORD, or (2) the g_withRecord is the RECORD
             * itself
             */

            if (g_withRecord.wPointer)
              {
                /* If the pointer is really a VAR parameter, then other
                 * syntax rules will apply
                 */

                if (g_withRecord.wVarParm)
                  {
                    assignFlags |= (ASSIGN_INDEXED | ASSIGN_DEREFERENCE |
                                    ASSIGN_STORE_INDEXED | ASSIGN_VAR_PARM);
                  }
                else
                  {
                    assignFlags |= (ASSIGN_INDEXED | ASSIGN_DEREFERENCE |
                                    ASSIGN_STORE_INDEXED);
                  }

                pas_GenerateDataOperation(opPUSH,
                                          varPtr->sParm.r.rOffset +
                                          g_withRecord.wIndex);
                tempOffset = g_withRecord.wOffset;
              }
            else
              {
                tempOffset = varPtr->sParm.r.rOffset + g_withRecord.wOffset;
              }

            /* Modify the variable so that it has the characteristics of the
             * the field but with level and offset associated with the record
             * NOTE:  We have to be careful here because the structure
             * associated with sRECORD_OBJECT is not the same as for
             * variables!  All variable fields need valid values.
             */

            typePtr                   = varPtr->sParm.r.rParent;

            varPtr->sKind             = typePtr->sParm.t.tType;
            varPtr->sLevel            = g_withRecord.wLevel;
            varPtr->sParm.v.vFlags    = 0;
            varPtr->sParm.v.vXfrUnit  = 0;
            varPtr->sParm.v.vOffset   = tempOffset;
            varPtr->sParm.v.vSize     = typePtr->sParm.t.tAllocSize;
            varPtr->sParm.v.vSymIndex = 0;
            varPtr->sParm.v.vParent   = typePtr;

            pas_SimpleAssignment(varPtr, assignFlags);
          }
      }
      break;

    case sPOINTER :
      /* FORM: <pointer identifier>^ := <expression>
       * OR:   <pointer identifier> := <pointer expression>
       */

      pas_PointerAssignment(varPtr, typePtr, assignFlags);
      break;

    case sVAR_PARM :
      {
        symbol_t *baseTypePtr;
        uint16_t  baseType;

        /* Dereference the VAR parameter and assign a value to the target
         * address.  If the VAR parameter is an array, derefence first, then
         * index to store value.
         */

        if (assignFlags != 0) error(eVARPARMTYPE);

        /* Load the address provided by the VAR parameter now.  An exception
         * is for string assignments; they work differently:  The RValue is
         * not simply stored to the LValue, rather the RValue string is
         * copied to the LValue string reference through a run-time library
         * call.
         *
         * A problematic case is if the base type is a RECORD and we will
         * eventually find that the record field is a string.  Then the
         * generation of this opLDS will be an error.
         */

        baseTypePtr = pas_GetBaseTypePointer(typePtr);
        baseType    = baseTypePtr->sParm.t.tType;

        if (baseType != sSTRING &&
            (assignFlags & ASSIGN_DEREFERENCE) == 0)
          {
            pas_GenerateStackReference(opLDS, varPtr);
          }

        /* Set up to save the RValue to the VAR parmeter address */

        assignFlags |= (ASSIGN_DEREFERENCE | ASSIGN_STORE_INDEXED |
                        ASSIGN_VAR_PARM);

        varPtr->sKind         = typePtr->sParm.t.tType;
        varPtr->sParm.v.vSize = typePtr->sParm.t.tAllocSize;

        pas_SimpleAssignment(varPtr, assignFlags);
      }
      break;

    case sARRAY :
      {
        symbol_t *baseTypePtr;
        uint16_t arrayKind;
        uint16_t size;

        /* FORM: <array identifier> := <expression>
         * OR:   <pointer array identifier>[<index>]^ := <expression>
         * OR:   <pointer array identifier>[<index>] := <pointer expression>
         * OR:   <record array identifier>[<index>].<field identifier> := <expression>
         * OR:   etc., etc., etc.
         */

        /* Special assignFlags:
         *
         * - ASSIGN_DEREFERENCE and ASSIGN_VAR_PARM if the array identifier
         *   is an array VAR parameter.
         * - ASSIGN_INDEXED should not be set (we set that flag here).
         * - The ASSIGN_OUTER_INDEXED flag may be set on entry in certain,
         *   more complex situations.  For example, an "outer" ARRAY of
         *   RECORDS needs to be indexed to assign a value to an "inner
         *   RECORD OBJECT.  However, that inner RECORD OBJECT may itself be
         *   an ARRAY that requires indexing.
         */

        if ((assignFlags & ASSIGN_INDEXED) != 0)
          {
            error(eARRAYTYPE);
          }

        /* Get a pointer to the base type symbol of the array */

        baseTypePtr  = pas_GetBaseTypePointer(typePtr);

        /* Get the size and base type of the array */

        size         = baseTypePtr->sParm.t.tAllocSize;
        arrayKind    = baseTypePtr->sParm.t.tType;

        /* REVISIT:  For subranges, we use the base type of the subrange. */

        if (arrayKind == sSUBRANGE)
          {
            arrayKind = baseTypePtr->sParm.t.tSubType;
          }

        /* Handle the array index if present */

        if (g_token == '[')
          {
            pas_ArrayIndex(typePtr);

            varPtr->sKind         = arrayKind;
            varPtr->sParm.v.vSize = size;
            assignFlags          |= ASSIGN_INDEXED;
            pas_SimpleAssignment(varPtr, assignFlags);
          }

        /* For old-time Pascal support, we need to be able to handler
         * assignments to 'PACKED ARRAY[] OF CHAR'.
         */

        else if (arrayKind == sCHAR)
          {
            /* This should be followed by ':=' */

            pas_ArrayAssignment(varPtr, typePtr, assignFlags);
          }
        else
          {
            error(eLBRACKET);
          }
      }
      break;

    default :
      error(eINVTYPE);
      break;
    }
}

/***********************************************************************/
/* Process simple assignment statement */

static void pas_Assignment(uint16_t storeOp, exprType_t assignType,
                           symbol_t *varPtr, symbol_t *typePtr)
{
  /* FORM:  <variable OR function identifer> := <expression> */

  if (g_token != tASSIGN) error (eASSIGN);
  else getToken();

  pas_Expression(assignType, typePtr);
  pas_GenerateStackReference(storeOp, varPtr);
}

/***********************************************************************/
/* Process the assignment to a pointer, either the pointer address or
 * the value of the dereferenced pointer.
 */

static void pas_PointerAssignment(symbol_t *varPtr, symbol_t *typePtr,
                                  uint8_t assignFlags)
{
  symbol_t *parentTypePtr;
  int ptrDepth = 1;

  /* FORM: <pointer identifier>^ := <expression>
   * OR:   <pointer identifier> := <pointer expression>
   */

  /* Is this a pointer to a pointer? */

  parentTypePtr = typePtr->sParm.t.tParent;

  while (parentTypePtr->sParm.t.tType == sPOINTER)
    {
      parentTypePtr = parentTypePtr->sParm.t.tParent;

      /* No pointers-to-pointers-to-pointers-to-... yet */

      if (ptrDepth > 1)
        {
          error(eNOTYET);
        }
      else
        {
          ptrDepth++;
        }
    }

  /* Are we de-referencing the pointer to assign a value to the pointed-at
   * object?  Or are we assigning an address to the pointer variable.
   *
   * Possibilities:
   *
   *  1) Dereferencing a pointer:            ptr^      := value-expression
   *  2) Assigning an address to a pointer:  ptr       := pointer-expression
   *  3) Dereferencing a pointer to a        ptr2ptr^^ := value-expression
   *     pointer:                            ptr2ptr^  := pointer-expression
   *  4) Assigning an address to a pointer   ptr2ptr   := pointer-to-pointer-
   *     to a pointer:                                    expression
   */

  /* A pointer LValue should be followed by either one or more '^' or by ':='
   * introducing the RValue.
   */

  if (g_token != '^' && g_token != tASSIGN)
    {
      error(ePOINTERTYPE);
    }

  /* Process one or more '^' following the pointer or pointer-to-pointer */

  while (g_token == '^')
    {
      /* If the pointer depth goes to zero, then we will expect a value
       * expression.
       */

      if (ptrDepth > 0)
        {
          ptrDepth--;
        }
      else
        {
          error(ePOINTERDEREF);
        }

      /* In a sequence of record pointers like head^.link^.link or ptr2ptr^^,
       * we must load the first 'head' pointer from the variable address.
       */

      if ((assignFlags & ASSIGN_LVALUE_ADDR) == 0)
        {
          /* Load the address value of the pointer onto the stack now */

          pas_GenerateStackReference(opLDS, varPtr);
          assignFlags |= ASSIGN_LVALUE_ADDR;
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

  /* Get the kind of the pointed-at object */

  varPtr->sKind = parentTypePtr->sParm.t.tType;

  if (ptrDepth == 0)
    {
      /* Indicate that we are dereferencing a pointer.  This will cause
       * the RValue to be assigned to the target address of the pointer
       * that we just pushed onto the stack.
       */

      assignFlags &= ~ASSIGN_ADDRESS;
      assignFlags |= ASSIGN_LVALUE_ADDR;

      /* The size of the variable value is no longer the size of the
       * pointer, but rather it is now the full allocation size of the
       * parent type.
       */

      varPtr->sParm.v.vSize = parentTypePtr->sParm.t.tAllocSize;
    }
  else
    {
      /* No, this is a pointer assignment.  Assign an address to a pointer
       * or pointer-to-a-pointer.
       */

      assignFlags |= ASSIGN_ADDRESS;

      if (ptrDepth > 1)
        {
          assignFlags |= ASSIGN_PTR2PTR;
        }
    }

  /* And process the assignment (indirect recursion). */

  pas_SimpleAssignment(varPtr, assignFlags);
}

/***********************************************************************/
/* Process the assignment to a variable length string record. */

static void pas_StringAssignment(symbol_t *varPtr, symbol_t *typePtr,
                                 uint8_t assignFlags)
{
  exprType_t rValueExprType;
  uint16_t   lValueType;
  uint16_t   libOpcode;
  bool       destFirst = false;

  /* FORM:  <variable OR function identifer> := <expression> */

  /* Verify that the assignment token follows the identifier */

  if (g_token != tASSIGN) error (eASSIGN);
  else getToken();

  /* Get the expression after assignment token.  We'll take any kind
   * of string expression.
   */

  rValueExprType = pas_Expression(exprString, typePtr);
  lValueType     = varPtr->sKind;

  /* Is the address of the LValue already on the stack?  This is usually
   * the case for complex assignments involving pointer and record LValues.
   */

  if ((assignFlags & ASSIGN_LVALUE_ADDR) == 0)
    {
      /* No..Place the address of the destination string structure instance
       * on the stack.  In the normal case, this means taking the address of
       * the dest string variable (opLAS).  But in the case of a VAR
       * parameter or a pointer, then we need, instead, to load the value of
       * the pointer.
       */

      if ((assignFlags & ASSIGN_DEREFERENCE) != 0)
        {
          pas_GenerateStackReference(opLDS, varPtr);
        }
      else
        {
          pas_GenerateStackReference(opLAS, varPtr);
        }

      /* Remember that the TOS is the source */

      destFirst = true;
    }

  /* This is an assignment to a allocated Pascal string --
   * Generate a runtime library call to copy the destination
   * string string into the pascal string instance.  The particular
   * runtime call will account for any necesary string type conversion.
   */

  if (lValueType == sSTRING && rValueExprType == exprString)
    {
      /* It is a pascal string type. Current stack representation is:
       *
       *   TOS(0) = Address of dest sting variable
       *   TOS(1) = Size of string buffer allocation
       *   TOS(1) = Pointer to source sting buffer
       *   TOS(2) = Length of source sting
       *
       * And in the indexed case:
       *
       *   TOS(3) = Dest sting variable address offset
       *
       * REVISIT:  This is awkward.  Life would be much easier if the
       * array index could be made to be emitted later in the stack and
       * so could be added to the dest sting variable address easily.
       */

      if ((assignFlags & (ASSIGN_INDEXED | ASSIGN_OUTER_INDEXED)) != 0)
        {
          libOpcode = destFirst ? lbSTRCPYX : lbSTRCPYX2;
        }
      else
        {
          libOpcode = destFirst ? lbSTRCPY : lbSTRCPY2;
        }

      pas_StandardFunctionCall(libOpcode);
    }
}

/***********************************************************************/
/* Process a multiple word assignment statement */

static void pas_LargeAssignment(uint16_t storeOp, exprType_t assignType,
                                symbol_t *varPtr, symbol_t *typePtr)
{
  /* FORM:  <variable OR function identifer> := <expression> */

  if (g_token != tASSIGN) error (eASSIGN);
  else getToken();

  pas_Expression(assignType, typePtr);
  pas_GenerateDataSize(varPtr->sParm.v.vSize);
  pas_GenerateStackReference(storeOp, varPtr);
}

/***********************************************************************/
/* Special Case: Process assignment to a 'PACKED ARRAY[] OF CHAR' */

static void pas_ArrayAssignment(symbol_t *varPtr, symbol_t *typePtr,
                                uint8_t assignFlags)
{
  uint16_t opCode;

  /* FORM:  <variable OR function identifer> := <expression> */

  if (g_token != tASSIGN) error (eASSIGN);
  else getToken();

  pas_Expression(exprString, typePtr);

  /* Set up the run-time library function call:
   *
   *    TOS(0) = Address of the array (destination)
   *    TOS(1) = Size of the array
   *    TOS(2) = Address of the string (source)
   *    TOS(3) = Size of the string
   */

  pas_GenerateDataOperation(opPUSH, varPtr->sParm.v.vSize);
  pas_GenerateStackReference(opLAS, varPtr);

  if ((assignFlags & ASSIGN_OUTER_INDEXED) != 0)
    {
      opCode = lbSTR2BSTRX;
    }
  else
    {
      opCode = lbSTR2BSTR;
    }

  pas_StandardFunctionCall(opCode);
}

/***********************************************************************/

static exprType_t pas_AssignExprType(exprType_t baseExprType,
                                     uint8_t assignFlags)
{
  return ((assignFlags & ASSIGN_ADDRESS) == 0) ? baseExprType :
          MK_POINTER_EXPRTYPE(baseExprType);
}

/***********************************************************************/

static void pas_GotoStatement(void)
{
  char     labelname [8];             /* Label symbol table name */
  symbol_t *label_ptr;                /* Pointer to Label Symbol */

  /* FORM:  GOTO <integer> */

  /* Get the token after the goto reserved word. It should be an <integer> */

  getToken();
  if (g_token != tINT_CONST)
    {
      /* Token following the goto is not an integer */

      error(eINVLABEL);
    }
  else
    {
      /* Find and verify the symbol associated with the label */

      (void)sprintf (labelname, "%" PRIu32, g_tknUInt);
      if (!(label_ptr = pas_FindSymbol(labelname, 0, NULL)))
        {
          error(eUNDECLABEL);
        }
      else if (label_ptr->sKind != sLABEL)
        {
          error(eINVLABEL);
        }
      else
        {
          /* Generate the branch to the label */

          pas_GenerateDataOperation(opJMP, label_ptr->sParm.l.lLabel);
        }

      /* Get the token after the <integer> value */

      getToken();
    }
}

/***********************************************************************/

static void pas_LabelStatement(void)
{
   char labelName [8];               /* Label symbol table name */
   symbol_t *labelPtr;               /* Pointer to Label Symbol */

   /* FORM:  <integer> : */

   /* Verify that the integer is a label name */

   (void)sprintf (labelName, "%" PRIu32, g_tknUInt);
   if (!(labelPtr = pas_FindSymbol(labelName, 0, NULL)))
     {
       error(eUNDECLABEL);
     }
   else if(labelPtr->sKind != sLABEL)
     {
       error(eINVLABEL);
     }

   /* And also verify that the label symbol has not been previously
    * defined.
    */

   else if(!(labelPtr->sParm.l.lUnDefined))
     {
       error(eMULTLABEL);
     }
   else
     {
       /* Generate the label and indicate that it has been defined */

       pas_GenerateDataOperation(opLABEL, labelPtr->sParm.l.lLabel);
       labelPtr->sParm.l.lUnDefined = false;
     }

   /* Skip over the label integer */

   getToken();

   /* Make sure that the label is followed by a colon */

   if (g_token != ':') error (eCOLON);
   else getToken();
}

/***********************************************************************/

static void pas_ProcStatement(void)
{
  symbol_t *procPtr = g_tknPtr;
  int size = 0;

  /* FORM: procedure-method-statement =
   * procedure-method-specifier [ actual-parameter-list ]
   *
   * Skip over the procedure-method-statement
   */

  getToken();

  /* Get the actual parameters (if any) associated with the procedure
   * call.  The returned size accounts for all of the parameters with
   * each aligned in integer-size address boundaries.
   */

  size = pas_ActualParameterList(procPtr);

  /* Generate procedure call and stack adjustment. */

  pas_GenerateProcedureCall(procPtr);
  if (size)
    {
      pas_GenerateDataOperation(opINDS, -size);
    }
}

/***********************************************************************/

static void pas_IfStatement(void)
{
  uint16_t else_label  = ++g_label;
  uint16_t endif_label = else_label;

  /* FORM: IF <expression> THEN <statement> [ELSE <statement>] */

  /* Skip over the IF token */

  getToken();

  /* Evaluate the boolean expression */

  pas_Expression(exprBoolean, NULL);

  /* Make sure that the boolean expression is followed by the THEN token */

  if (g_token !=  tTHEN)
    error (eTHEN);
  else
    {
      /* Skip over the THEN token */

      getToken();

      /* Generate a conditional branch to the "else_label."  This will be a
       * branch to either the ENDIF or to the ELSE location (if present).
       */

      pas_GenerateDataOperation(opJEQUZ, else_label);

      /* Parse the <statment> following the THEN token */

      pas_Statement();

      /* Check for optional ELSE <statement> */

      if (g_token == tELSE)
        {
          /* Change the ENDIF label.  Now instead of branching to
           * the ENDIF, the logic above will branch to the ELSE
           * logic generated here.
           */

          endif_label = ++g_label;

          /* Skip over the ELSE token */

          getToken();

          /* Generate Jump to ENDIF label after the  THEN <statement> */

          pas_GenerateDataOperation(opJMP, endif_label);

          /* Generate the ELSE label here.  This is where we will go if
           * the IF <expression> evaluates to false.
           */

          pas_GenerateDataOperation(opLABEL, else_label);

          /* Generate the ELSE <statement> then fall through to the
           * ENDIF label.
           */

          pas_Statement();
        }

      /* Generate the ENDIF label here.  Note that if no ELSE <statement>
       * is present, this will be the same as the else_label.
       */

      pas_GenerateDataOperation(opLABEL, endif_label);
    }
}

/***********************************************************************/

void pas_CompoundStatement(void)
{
   /* Process statements until END encountered */

   do
     {
       getToken();
       pas_Statement();
     }
   while (g_token == ';');

   /* Verify that it really was END */

   if (g_token != tEND) error (eEND);
   else getToken();
}

/***********************************************************************/

void pas_RepeatStatement ()
{
  uint16_t rpt_label = ++g_label;

  /* REPEAT <statement[;statement[statement...]]> UNTIL <expression> */

  /* Generate top of loop label */

  pas_GenerateDataOperation(opLABEL, rpt_label);
  do
    {
      getToken();

      /* Process <statement> */

      pas_Statement();
    }
  while (g_token == ';');

  /* Verify UNTIL follows */

  if (g_token !=  tUNTIL) error (eUNTIL);
  else getToken();

 /* Generate UNTIL <expression> */

  pas_Expression(exprBoolean, NULL);

  /* Generate conditional branch to the top of loop */

  pas_GenerateDataOperation(opJEQUZ, rpt_label);
}

/***********************************************************************/

static void pas_WhileStatement(void)
{
   uint16_t while_label    = ++g_label;  /* Top of loop label */
   uint16_t endwhile_label = ++g_label;  /* End of loop label */

   /* Generate WHILE <expression> DO <statement> */

   /* Skip over WHILE token */

   getToken();

   /* Set top of loop label */

   pas_GenerateDataOperation(opLABEL, while_label);

   /* Evaluate the WHILE <expression> */

   pas_Expression(exprBoolean, NULL);

   /* Generate a conditional jump to the end of the loop */

   pas_GenerateDataOperation(opJEQUZ, endwhile_label);

   /* Verify that the DO token follows the expression */

   if (g_token !=  tDO) error(eDO);
   else getToken();

   /* Generate the <statement> following the DO token */

   pas_Statement();

   /* Generate a branch to the top of the loop */

   pas_GenerateDataOperation(opJMP, while_label);

   /* Set the bottom of loop label */

   pas_GenerateDataOperation(opLABEL, endwhile_label);
}

/***********************************************************************/

static void pas_CaseStatement(void)
{
  uint16_t this_case;
  uint16_t next_case  = ++g_label;
  uint16_t end_case   = ++g_label;

  /* Process "CASE <expression> OF" */

  /* Skip over the CASE token */

  getToken();

  /* Evaluate the CASE <expression> */

  pas_Expression(exprAnyOrdinal, NULL);

  /* Verify that CASE <expression> is followed with the OF token */

  if (g_token !=  tOF) error (eOF);
  else getToken();

  /* Loop to process each case until END encountered */

  for (; ; )
    {
      this_case = next_case;
      next_case = ++g_label;

      /* Process optional ELSE <statement> END */

      if (g_token == tELSE)
        {
          getToken();

          /* Set ELSE statement label */

          pas_GenerateDataOperation(opLABEL, this_case);

          /* Evaluate ELSE statement */

          pas_Statement();

          /* Allow ELSE statement to be followed with NULL statement. */

          if (g_token == ';') getToken();

          /* Verify that END follows the ELSE <statement> */

          if (g_token != tEND) error(eEND);
          else getToken();

          /* Terminate FOR loop */

          break;
        }

      /* Process "<constant>[,<constant>[,...]] : <statement>"
       * NOTE:  We accept any kind of constant for the case selector; there
       * really should be some check to assure that the constant is of the
       * same type as the expression!
       */

      else
        {
          uint16_t statement_label  = ++g_label;

          /* Generate the CASE label */

          pas_GenerateDataOperation(opLABEL, this_case);

          /* Loop for each <constant> in the case list */

          for (; ; )
            {
              /* Generate a comparison of the CASE expression and the
               * constant.  Duplicate the case expression value at the TOP
               * of the stack.  The "dangling" value will be discarded by
               * end-of-case logic below.
               */

              pas_GenerateSimple(opDUP);

              /* Verify that we have a constant.  This could be  literal constant,
               * defined constant, or perhaps a standard function operating on
               * a constant.
               */

              if (IS_CONSTANT(g_token))
                {
                  uint16_t value;

                  if ((g_tknUInt & ~0xffff) != 0)
                    {
                      error(eINTOVF);
                      value = UINT16_MAX;
                    }
                  else
                    {
                      value = (uint16_t)g_tknUInt;
                    }

                  /* It is a literal constant value */

                  pas_GenerateDataOperation(opPUSH, value);

                  /* Skip over the constant */

                  getToken();
                }
              else if (g_token == tSTDFUNC)
                {
                  /* Check if it is a constant standard function.  If not,
                   * pas_StandardFunctionOfConstant will handle the error.
                   * REVISIT:  Won't that cause us to hang here in an
                   * infinite loop?
                   */

                  pas_StandardFunctionOfConstant();
                  pas_GenerateDataOperation(opPUSH, g_constantInt);
                }
              else
                {
                  error(eINTCONST);
                  break;
                }

              /* The kind of comparison we generate depends on if we have to
               * jump over other case selector comparsions to the statement
               * or if we can just fall through to the statement
               */

              /* If there are multiple constants, they will be separated with
               * commas.
               */

              if (g_token == ',')
                {
                  /* Generate jump to <statement> */

                  pas_GenerateDataOperation(opJEQU, statement_label);

                  /* Skip over comma */

                  getToken();
                }
              else
                {
                  /* else jump to the next case */

                  pas_GenerateDataOperation(opJNEQ, next_case);
                  break;
                }
            }

          /* Then process ... : <statement> */

          /* Verify colon presence */

          if (g_token != ':') error(eCOLON);
          else getToken();

          /* Generate the statement label */

          pas_GenerateDataOperation(opLABEL, statement_label);

          /* Evaluate the CASE <statement> */

          pas_Statement();

          /* Jump to exit CASE */

          pas_GenerateDataOperation(opJMP, end_case);
        }

      /* Check if there are more statements.  If not, verify that END is
       * present.
       */

      if (g_token != ';' && g_token != tEND)
        {
          error(eEND);
          break;
        }
      else
        {
          /* If END is encountered, then there are no further case selectors */

          if (g_token == ';')
            {
              getToken();
            }

          /* Permit a null statement (i.e., extra ';') on the last case. */

          if (g_token == tEND)
            {
              /* Generate the next case label for the last case selector */

              pas_GenerateDataOperation(opLABEL, next_case);
              getToken();
              break;
            }
        }
    }

  /* Generate ENDCASE label and Pop CASE <expression> from stack */

  pas_GenerateDataOperation(opLABEL, end_case);
  pas_GenerateDataOperation(opINDS, -sINT_SIZE);
}

/***********************************************************************/

static void pas_ForStatement(void)
{
   symbol_t *varPtr;
   uint16_t forLabel    = ++g_label;
   uint16_t endForLabel = ++g_label;
   uint16_t jmpOp;
   uint16_t modOp;

   /* FOR <assigment statement> <TO, DOWNTO> <expression> DO <statement> */

   /* Skip over the FOR token */

   getToken();

   /* Get and verify the left side of the assignment. */

   if (g_token != sINT      && g_token != sWORD      &&
       g_token != sSHORTINT && g_token != sSHORTWORD &&
       g_token != sLONGINT  && g_token != sLONGWORD &&
       g_token != sSUBRANGE && g_token != sSCALAR)
     {
       error(eINTVAR);
     }
   else
     {
       exprType_t forExprType;
       uint16_t forVarType;

       /* The expression type we need for the FOR index variable type */

       forVarType = g_token;
       varPtr     = g_tknPtr;

       if (forVarType == sSUBRANGE)
         {
           symbol_t *baseTypePtr;

           /* For a sub-range, use the parent type */

           baseTypePtr = pas_GetBaseTypePointer(varPtr->sParm.v.vParent);
           forVarType  = baseTypePtr->sParm.t.tSubType;
         }

       /* Then map the FOR index type to an expression type */

       forExprType = pas_MapVariable2ExprType(forVarType, true);

       /* Generate the assignment to the integer variable */

       getToken();
       pas_Assignment(opSTS, forExprType, varPtr, varPtr->sParm.v.vParent);

       /* Determine if this is a TO or a DOWNTO loop and set up the opCodes
        * to generate appropriately.
        */

       if (g_token == tDOWNTO)
         {
           jmpOp = opJGT;
           modOp = opDEC;
           getToken();
         }
       else if (g_token == tTO)
         {
           jmpOp = opJLT;
           modOp = opINC;
           getToken();
         }
       else
         {
           error (eTOorDOWNTO);
         }

       /* Evaluate <expression> DO */

       pas_Expression(forExprType, varPtr->sParm.v.vParent);

       /* Verify that the <expression> is followed by the DO token */

       if (g_token != tDO) error (eDO);
       else getToken();

       /* Generate top of loop label */

       pas_GenerateDataOperation(opLABEL, forLabel);

       /* Generate the top of loop comparison.  Duplicate the end of loop
        * value, push the current value, and perform the comparison.
        */

       pas_GenerateSimple(opDUP);
       pas_GenerateStackReference(opLDS, varPtr);
       pas_GenerateDataOperation(jmpOp, endForLabel);

       /* Evaluate the for statement <statement> */

       pas_Statement();

       /* Generate end of loop logic:  Load the variable, modify the
        * variable, store the variable, and jump unconditionally to the
        * top of the loop.
        */

       pas_GenerateStackReference(opLDS, varPtr);
       pas_GenerateSimple(modOp);
       pas_GenerateStackReference(opSTS, varPtr);
       pas_GenerateDataOperation(opJMP, forLabel);

       /* Generate the end of loop label.  This is where the conditional
        * branch at the top of the loop will come to.
        */

       pas_GenerateDataOperation(opLABEL, endForLabel);
       pas_GenerateDataOperation(opINDS, -sINT_SIZE);
     }
}

/***********************************************************************/

static void pas_WithStatement(void)
{
  with_t saveWithRecord;

  /* Generate WITH <variable[,variable[...]] DO <statement> */

  /* Save the current WITH pointer.  Only one WITH can be active at
   * any given time.
   */

  saveWithRecord = g_withRecord;

  /* Process each RECORD or RECORD OBJECT in the <variable> list */

  getToken();
  for (; ; )
    {
      symbol_t *withTypePtr;
      symbol_t *baseTypePtr;

      /* Get a pointer to the underlying base type symbol */

      withTypePtr = g_tknPtr;
      baseTypePtr = pas_GetBaseTypePointer(withTypePtr);
      if (baseTypePtr->sParm.t.tType != sRECORD)
        {
          error(eRECORDTYPE);
        }

      /* A RECORD type variable may be used in the WITH statement only if
       * there is no other WITH active
       */

      else if (g_token == sRECORD && !g_withRecord.wParent)
        {
          /* Save the RECORD variable as the new with record */

          g_withRecord.wLevel   = withTypePtr->sLevel;
          g_withRecord.wPointer = false;
          g_withRecord.wVarParm = false;
          g_withRecord.wOffset  = withTypePtr->sParm.v.vOffset;
          g_withRecord.wParent  = baseTypePtr;

          /* Skip over the RECORD variable */

          getToken();
        }

      /* A RECORD VAR parameter may also be used in the WITH statement
       * (again only if there is no other WITH active)
       */

      else if (g_token == sVAR_PARM && !g_withRecord.wParent &&
               baseTypePtr->sParm.t.tType == sRECORD)
        {
          /* Save the RECORD VAR parameter as the new with record */

          g_withRecord.wLevel   = withTypePtr->sLevel;
          g_withRecord.wPointer = true;
          g_withRecord.wVarParm = true;
          g_withRecord.wOffset  = withTypePtr->sParm.v.vOffset;
          g_withRecord.wParent  = baseTypePtr;

          /* Skip over the RECORD VAR parameter */

          getToken();
        }

      /* A pointer to a RECORD may also be used in the WITH statement
       * (again only if there is no other WITH active)
       */

      else if (g_token == sPOINTER && !g_withRecord.wParent &&
               baseTypePtr->sParm.t.tType == sRECORD)
        {
          /* Save the RECORD pointer as the new with record */

          g_withRecord.wLevel   = withTypePtr->sLevel;
          g_withRecord.wPointer = true;
          g_withRecord.wVarParm = false;
          g_withRecord.wOffset  = withTypePtr->sParm.v.vOffset;
          g_withRecord.wParent  = baseTypePtr;

          /* Skip over the RECORD pointer */

          getToken();

          /* Verify that deferencing is specified! */

          if (g_token != '^') error(eRECORDVAR);
          else getToken();
        }

      /* A RECORD_OBJECT may be used in the WITH statement if the field
       * is from the same sRECORD type and is itself of type RECORD.
       */

      else if (g_token == sRECORD_OBJECT &&
               withTypePtr->sParm.r.rRecord == g_withRecord.wParent &&
               baseTypePtr->sParm.t.tType == sRECORD)
        {
          /* Okay, update the with record to use this record field */

          if (g_withRecord.wPointer)
            {
              g_withRecord.wIndex += withTypePtr->sParm.r.rOffset;
            }
          else
            {
              g_withRecord.wOffset += withTypePtr->sParm.r.rOffset;
            }

          g_withRecord.wParent = withTypePtr->sParm.r.rParent;

          /* Skip over the sRECORD_OBJECT */

          getToken();
        }

    /* Anything else is an error */

      else
        {
          error(eRECORDVAR);
          break;
        }

      /* Check if there are multiple variables in the WITH statement */

      if (g_token == ',') getToken();
      else break;
    }

   /* Verify that the RECORD list is terminated with DO */

  if (g_token != tDO) error (eDO);
  else getToken();

  /* Then process the statement following the WITH */

  pas_Statement();

  /* Restore the previous value of the with record */

  g_withRecord = saveWithRecord;
}
