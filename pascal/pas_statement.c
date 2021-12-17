/****************************************************************************
 * pas_statement.c
 * Pascal Statements
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
#include <inttypes.h>

#include "pas_debug.h"
#include "pas_defns.h"
#include "pas_tkndefs.h"
#include "pas_pcode.h"
#include "pas_errcodes.h"
#include "pas_sysio.h"

#include "pas_main.h"
#include "pas_statement.h"
#include "pas_procedure.h"
#include "pas_expression.h"
#include "pas_codegen.h"
#include "pas_token.h"
#include "pas_symtable.h"
#include "pas_insn.h"
#include "pas_error.h"

/****************************************************************************
 * Private Definitions
 ****************************************************************************/

/* REVIST: duplicated in pas_expression.h */

#define ADDRESS_DEREFERENCE  0x01
#define ADDRESS_ASSIGNMENT   0x02
#define INDEXED_ASSIGNMENT   0x04
#define VAR_PARM_ASSIGNMENT  0x08

#define isConstant(x) \
        (  ((x) == tINT_CONST) \
        || ((x) == tBOOLEAN_CONST) \
        || ((x) == tCHAR_CONST) \
        || ((x) == tREAL_CONST) \
        || ((x) == sSCALAR_OBJECT))

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Assignment Statements */

static void pas_ComplexAssignment(void);
static void pas_SimpleAssignment (symbol_t *varPtr, uint8_t assignFlags);
static void pas_Assignment       (uint16_t storeOp, exprType_t assignType,
                                  symbol_t *varPtr, symbol_t *typePtr);
static void pas_StringAssignment (symbol_t *varPtr, symbol_t *typePtr);
static void pas_LargeAssignment  (uint16_t storeOp, exprType_t assignType,
                                  symbol_t *varPtr, symbol_t *typePtr);

/* Other Statements */

static void pas_GotoStatement    (void);  /* GOTO statement */
static void pas_LabelStatement   (void);  /* Label statement */
static void pas_ProcStatement    (void);  /* Procedure method statement */
static void pas_IfStatement      (void);  /* IF-THEN[-ELSE] statement */
static void pas_CaseStatement    (void);  /* Case statement */
static void pas_RepeatStatement  (void);  /* Repeat statement */
static void pas_WhileStatement   (void);  /* While statement */
static void pas_ForStatement     (void);  /* For statement */
static void pas_WithStatement    (void);  /* With statement */

/****************************************************************************/

void pas_Statement(void)
{
  symbol_t *symPtr;     /* Save Symbol Table pointer to token */

  TRACE(g_lstFile,"[pas_Statement");

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
      /* Simple assignment statements */

    case sINT :
      symPtr = g_tknPtr;
      getToken();
      pas_Assignment(opSTS, exprInteger, symPtr, symPtr->sParm.v.parent);
      break;

    case sCHAR :
      symPtr = g_tknPtr;
      getToken();
      pas_Assignment(opSTSB, exprChar, symPtr, symPtr->sParm.v.parent);
      break;

    case sBOOLEAN :
      symPtr = g_tknPtr;
      getToken();
      pas_Assignment(opSTSB, exprBoolean, symPtr, NULL);
      break;

    case sREAL :
      symPtr = g_tknPtr;
      getToken();
      pas_LargeAssignment(opSTSM, exprReal, symPtr, symPtr->sParm.v.parent);
      break;

    case sSCALAR :
      symPtr = g_tknPtr;
      getToken();
      pas_Assignment(opSTS, exprScalar, symPtr, symPtr->sParm.v.parent);
      break;

    case sSET_OF :
      symPtr = g_tknPtr;
      getToken();
      pas_Assignment(opSTS, exprSet, symPtr, symPtr->sParm.v.parent);
      break;

    case sSTRING :
    case sRSTRING :
      symPtr = g_tknPtr;
      getToken();
      pas_StringAssignment(symPtr, symPtr->sParm.v.parent);
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

  TRACE(g_lstFile,"]");
}

/***********************************************************************/
/* Process a complex assignment statement */

static void pas_ComplexAssignment(void)
{
   symbol_t symbolSave;
   TRACE(g_lstFile,"[pas_ComplexAssignment]");

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
  symbol_t *typePtr;
  symbol_t *indexTypePtr;

  TRACE(g_lstFile,"[pas_SimpleAssignment]");

  /* FORM:  <variable OR function identifer> := <expression> */

  /* Get the parent type */

  typePtr = varPtr->sParm.v.parent;

  /* Now, handle the variable by its type */

  switch (varPtr->sKind)
    {
      /* Check if we have reduce the complex assignment to a simple
       * assignment yet
       */

    case sINT :
      if ((assignFlags & INDEXED_ASSIGNMENT) != 0)
        {
          if ((assignFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              pas_Assignment(opSTI, exprInteger, varPtr, typePtr);
            }
          else if ((assignFlags & ADDRESS_ASSIGNMENT) != 0)
            {
              pas_Assignment(opSTSX, exprIntegerPtr, varPtr, typePtr);
            }
          else
            {
              pas_Assignment(opSTSX, exprInteger, varPtr, typePtr);
            }
        }
      else
        {
          if ((assignFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              pas_Assignment(opSTI, exprInteger, varPtr, typePtr);
            }
          else if ((assignFlags & ADDRESS_ASSIGNMENT) != 0)
            {
              pas_Assignment(opSTS, exprIntegerPtr, varPtr, typePtr);
            }
          else
            {
              pas_Assignment(opSTS, exprInteger, varPtr, typePtr);
            }
        }
      break;

    case sCHAR :
      if ((assignFlags & INDEXED_ASSIGNMENT) != 0)
        {
          if ((assignFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              pas_Assignment(opSTIB, exprChar, varPtr, typePtr);
            }
          else if ((assignFlags & ADDRESS_ASSIGNMENT) != 0)
            {
              pas_Assignment(opSTSX, exprCharPtr, varPtr, typePtr);
            }
          else
            {
              pas_Assignment(opSTSXB, exprChar, varPtr, typePtr);
            }
        }
      else
        {
          if ((assignFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              pas_Assignment(opSTIB, exprChar, varPtr, typePtr);
            }
          else if ((assignFlags & ADDRESS_ASSIGNMENT) != 0)
            {
              pas_Assignment(opSTS, exprCharPtr, varPtr, typePtr);
            }
          else
            {
              pas_Assignment(opSTSB, exprChar, varPtr, typePtr);
            }
        }
      break;

    case sBOOLEAN :
      if ((assignFlags & INDEXED_ASSIGNMENT) != 0)
        {
          if ((assignFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              pas_Assignment(opSTI, exprBoolean, varPtr, NULL);
            }
          else if ((assignFlags & ADDRESS_ASSIGNMENT) != 0)
            {
              pas_Assignment(opSTSX, exprBooleanPtr, varPtr, typePtr);
            }
          else
            {
              pas_Assignment(opSTSX, exprBoolean, varPtr, NULL);
            }
        }
      else
        {
          if ((assignFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              pas_Assignment(opSTI, exprBoolean, varPtr, NULL);
            }
          else if ((assignFlags & ADDRESS_ASSIGNMENT) != 0)
            {
              pas_Assignment(opSTS, exprBooleanPtr, varPtr, typePtr);
            }
          else
            {
              pas_Assignment(opSTS, exprBoolean, varPtr, NULL);
            }
        }
      break;

    case sREAL         :
      if ((assignFlags & INDEXED_ASSIGNMENT) != 0)
        {
          if ((assignFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              pas_LargeAssignment(opSTIM, exprReal, varPtr, typePtr);
            }
          else if ((assignFlags & ADDRESS_ASSIGNMENT) != 0)
            {
              pas_Assignment(opSTSX, exprRealPtr, varPtr, typePtr);
            }
          else
            {
              pas_LargeAssignment(opSTSXM, exprReal, varPtr, typePtr);
            }
        }
      else
        {
          if ((assignFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              pas_LargeAssignment(opSTIM, exprReal, varPtr, typePtr);
            }
          else if ((assignFlags & ADDRESS_ASSIGNMENT) != 0)
            {
              pas_Assignment(opSTS, exprRealPtr, varPtr, typePtr);
            }
          else
            {
              pas_LargeAssignment(opSTSM, exprReal, varPtr, typePtr);
            }
        }
      break;

    case sSCALAR :
      if ((assignFlags & INDEXED_ASSIGNMENT) != 0)
        {
          if ((assignFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              pas_Assignment(opSTI, exprScalar, varPtr, typePtr);
            }
          else if ((assignFlags & ADDRESS_ASSIGNMENT) != 0)
            {
              pas_Assignment(opSTSX, exprScalarPtr, varPtr, typePtr);
            }
          else
            {
              pas_Assignment(opSTSX, exprScalar, varPtr, typePtr);
            }
        }
      else
        {
          if ((assignFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              pas_Assignment(opSTI, exprScalar, varPtr, typePtr);
            }
          else if ((assignFlags & ADDRESS_ASSIGNMENT) != 0)
            {
              pas_Assignment(opSTS, exprScalarPtr, varPtr, typePtr);
            }
          else
            {
              pas_Assignment(opSTS, exprScalar, varPtr, typePtr);
            }
        }
      break;

    case sSET_OF :
      if ((assignFlags & INDEXED_ASSIGNMENT) != 0)
        {
          if ((assignFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDSX, varPtr);
              pas_Assignment(opSTI, exprSet, varPtr, typePtr);
            }
          else if ((assignFlags & ADDRESS_ASSIGNMENT) != 0)
            {
              pas_Assignment(opSTSX, exprSetPtr, varPtr, typePtr);
            }
          else
            {
              pas_Assignment(opSTSX, exprSet, varPtr, typePtr);
            }
        }
      else
        {
          if ((assignFlags & ADDRESS_DEREFERENCE) != 0)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              pas_Assignment(opSTI, exprSet, varPtr, typePtr);
            }
          else if ((assignFlags & ADDRESS_ASSIGNMENT) != 0)
            {
              pas_Assignment(opSTS, exprSetPtr, varPtr, typePtr);
            }
          else
            {
              pas_Assignment(opSTS, exprSet, varPtr, typePtr);
            }
        }
      break;

      /* NOPE... recurse until it becomes a simple assignment */

    case sSUBRANGE :
      varPtr->sKind = typePtr->sParm.t.subType;
      pas_SimpleAssignment(varPtr, assignFlags);
      break;

    case sRECORD :
      /* FORM: <record identifier>.<field> := <expression>
       * OR:   <record pointer identifier> := <pointer expression>
       */

      /* Check if this is a pointer to a record */

      if ((assignFlags & ADDRESS_ASSIGNMENT) != 0)
        {
          if (g_token == '.') error(ePOINTERTYPE);

          if ((assignFlags & INDEXED_ASSIGNMENT) != 0)
            {
              pas_Assignment(opSTSX, exprRecordPtr, varPtr, typePtr);
            }
          else
            {
              pas_Assignment(opSTS, exprRecordPtr, varPtr, typePtr);
            }
        }
      else if (((assignFlags & ADDRESS_DEREFERENCE) != 0) &&
               ((assignFlags & VAR_PARM_ASSIGNMENT) == 0))
        {
          error(ePOINTERTYPE);
        }

      /* Check if a period separates the RECORD identifier from the
       * record field identifier
       */

      else if (g_token == '.')
        {
          /* Skip over the period */

          getToken();

          /* Verify that a field identifier associated with this record
           * follows the period.
           */

          if ((g_token != sRECORD_OBJECT) ||
              (g_tknPtr->sParm.r.record != typePtr))
            {
              error(eRECORDOBJECT);
            }
          else
            {
              /* Modify the variable so that it has the characteristics of the
               * the field but with level and offset associated with the record
               */

              typePtr                 = g_tknPtr->sParm.r.parent;
              varPtr->sKind           = typePtr->sParm.t.type;
              varPtr->sParm.v.parent  = typePtr;

              /* Special case:  The record is a VAR parameter. */

              if (assignFlags == (INDEXED_ASSIGNMENT | ADDRESS_DEREFERENCE |
                                  VAR_PARM_ASSIGNMENT))
                {
                  pas_GenerateDataOperation(opPUSH, g_tknPtr->sParm.r.offset);
                  pas_GenerateSimple(opADD);
                }
              else
                {
                 varPtr->sParm.v.offset += g_tknPtr->sParm.r.offset;
                }

              getToken();
              pas_SimpleAssignment(varPtr, assignFlags);
            }
        }

      /* It must be a RECORD assignment */

      else
        {
          /* Special case:  The record is a VAR parameter. */

          if (assignFlags == (INDEXED_ASSIGNMENT | ADDRESS_DEREFERENCE |
                              VAR_PARM_ASSIGNMENT))
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
      /* FORM: <field> := <expression>
       * NOTE:  This must have been preceeded with a WITH statement
       * defining the RECORD type
       */

      if (!g_withRecord.parent)
        {
          error(eINVTYPE);
        }
      else if ((assignFlags && (ADDRESS_DEREFERENCE | ADDRESS_ASSIGNMENT)) != 0)
        {
          error(ePOINTERTYPE);
        }
      else if ((assignFlags && INDEXED_ASSIGNMENT) != 0)
        {
          error(eARRAYTYPE);
        }

      /* Verify that a field identifier is associated with the RECORD
       * specified by the WITH statement.
       */

      else if (varPtr->sParm.r.record != g_withRecord.parent)
        {
          error(eRECORDOBJECT);
        }
      else
        {
          int16_t tempOffset;

          /* Now there are two cases to consider:  (1) the g_withRecord is a
           * pointer to a RECORD, or (2) the g_withRecord is the RECORD itself
           */

          if (g_withRecord.pointer)
            {
              /* If the pointer is really a VAR parameter, then other syntax
               * rules will apply
               */

              if (g_withRecord.varParm)
                {
                  assignFlags |= (INDEXED_ASSIGNMENT | ADDRESS_DEREFERENCE |
                                  VAR_PARM_ASSIGNMENT);
                }
              else
                {
                  assignFlags |= (INDEXED_ASSIGNMENT | ADDRESS_DEREFERENCE);
                }

              pas_GenerateDataOperation(opPUSH, (varPtr->sParm.r.offset + g_withRecord.index));
              tempOffset = g_withRecord.offset;
            }
          else
            {
              tempOffset = varPtr->sParm.r.offset + g_withRecord.offset;
            }

          /* Modify the variable so that it has the characteristics of the
           * the field but with level and offset associated with the record
           * NOTE:  We have to be careful here because the structure
           * associated with sRECORD_OBJECT is not the same as for
           * variables!
           */

          typePtr                 = varPtr->sParm.r.parent;

          varPtr->sKind           = typePtr->sParm.t.type;
          varPtr->sLevel          = g_withRecord.level;
          varPtr->sParm.v.size    = typePtr->sParm.t.asize;
          varPtr->sParm.v.offset  = tempOffset;
          varPtr->sParm.v.parent  = typePtr;

          pas_SimpleAssignment(varPtr, assignFlags);
        }
      break;

    case sPOINTER :
      /* FORM: <pointer identifier>^ := <expression>
       * OR:   <pointer identifier> := <pointer expression>
       */

      if (g_token == '^') /* value assignment? */
        {
          getToken();
          assignFlags |= ADDRESS_DEREFERENCE;
        }
      else
        {
          assignFlags |= ADDRESS_ASSIGNMENT;
        }

      /* If the parent type is itself a typed pointer, then get the
       * pointed-at type.
       */

      if (/* typePtr->sKind == sTYPE && */ typePtr->sParm.t.type == sPOINTER)
        {
          symbol_t *baseTypePtr = typePtr->sParm.t.parent;

          varPtr->sKind = baseTypePtr->sParm.t.type;

          /* REVISIT:  What if the type is a pointer to a pointer? */

           if (varPtr->sKind == sPOINTER) fatal(eNOTYET);
        }
      else
        {
          /* Get the kind of parent type */

          varPtr->sKind = typePtr->sParm.t.type;
        }

      pas_SimpleAssignment(varPtr, assignFlags);
      break;

    case sVAR_PARM :
      if (assignFlags != 0) error(eVARPARMTYPE);
      assignFlags |= (ADDRESS_DEREFERENCE | VAR_PARM_ASSIGNMENT);

      varPtr->sKind = typePtr->sParm.t.type;
      pas_SimpleAssignment(varPtr, assignFlags);
      break;

    case sARRAY :
      {
        symbol_t *baseTypePtr;
        symbol_t *nextType;
        uint16_t arrayKind;
        uint16_t size;

        /* FORM: <array identifier> := <expression>
         * OR:   <pointer array identifier>[<index>]^ := <expression>
         * OR:   <pointer array identifier>[<index>] := <pointer expression>
         * OR:   <record array identifier>[<index>].<field identifier> := <expression>
         * OR:   etc., etc., etc.
         */

        if (assignFlags != 0) error(eARRAYTYPE);
        assignFlags |= INDEXED_ASSIGNMENT;

        indexTypePtr = typePtr->sParm.t.index;
        if (indexTypePtr == NULL) error(eHUH);

        /* Get a pointer to the base type symbol of the array */

        baseTypePtr = typePtr;
        nextType    = typePtr->sParm.t.parent;

        while (nextType != NULL && baseTypePtr->sKind == sTYPE)
          {
            baseTypePtr = nextType;
            nextType    = baseTypePtr->sParm.t.parent;
          }

        /* Handle the array index */

        pas_ArrayIndex(indexTypePtr, baseTypePtr->sParm.t.asize);

        /* REVISIT:  Missing generation of the indexed load? */

        /* Get the aize and base type of the array */

        size      = baseTypePtr->sParm.t.asize;
        arrayKind = baseTypePtr->sParm.t.type;

        /* REVISIT:  For subranges, we use the base type of the subrange. */

        if (arrayKind == sSUBRANGE)
          {
            arrayKind = baseTypePtr->sParm.t.subType;
          }

        varPtr->sKind        = arrayKind;
        varPtr->sParm.v.size = size;
        pas_SimpleAssignment(varPtr, assignFlags);
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
   TRACE(g_lstFile,"[pas_Assignment]");

   /* FORM:  <variable OR function identifer> := <expression> */

   if (g_token != tASSIGN) error (eASSIGN);
   else getToken();

   pas_Expression(assignType, typePtr);
   pas_GenerateStackReference(storeOp, varPtr);
}

/***********************************************************************/
/* Process the assignment to a variable length string record */

static void pas_StringAssignment(symbol_t *varPtr, symbol_t *typePtr)
{
  exprType_t stringKind;

   TRACE(g_lstFile,"[pas_StringAssignment]");

   /* FORM:  <variable OR function identifer> := <expression> */

   /* Verify that the assignment token follows the indentifier */

   if (g_token != tASSIGN) error (eASSIGN);
   else getToken();

   /* Get the expression after assignment token. We'll take any kind
    * of string expression.  This is a hack to handle calls to system
    * functions that return exprCString pointers that must be converted
    * to exprString records upon assignment.
    */

   stringKind = pas_Expression(exprAnyString, typePtr);

   /* Place the address of the destination string structure instance on the
    * stack.
    */

   pas_GenerateStackReference(opLAS, varPtr);

   /* Check if this is an assignment to a global allocated string, or
    * to a stack reference to an allocated string.
    */

   if (varPtr->sKind == sRSTRING)
     {
       /* It is an assignment to a string reference --
        * Generate a runtime library call to copy the destination
        * string string into the pascal string instance.  The particular
        * runtime call will account for any necesary string type conversion.
        */

       if ((stringKind == exprString) || (stringKind == exprStkString))
         {
           /* It is a pascal string type. Current stack representation is:
            *
            *   TOS(0)=address of dest string reference
            *   TOS(1)=length of source string
            *   TOS(2)=pointer to source string
            */

           pas_StandardFunctionCall(lbSTR2RSTR);
         }
       else if (stringKind == exprCString)
         {
           /* It is a 32-bit C string point.  Current stack representation is:
            *
            *   TOS(0)=address of dest string reference
            *   TOS(1)=MS 16-bits of 32-bit C source string pointer
            *   TOS(2)=LS 16-bits of 32-bit C source string pointer
            */

           pas_StandardFunctionCall(lbCSTR2RSTR);
         }
     }
   else
     {
       /* It is an assignment to a allocated Pascal string --
        * Generate a runtime library call to copy the destination
        * string string into the pascal string instance.  The particular
        * runtime call will account for any necesary string type conversion.
        */

       if ((stringKind == exprString) || (stringKind == exprStkString))
         {
           /* It is a pascal string type. Current stack representation is:
            *
            *   TOS(0)=address of dest string hdr
            *   TOS(1)=length of source string
            *   TOS(2)=pointer to source string
            */

           pas_StandardFunctionCall(lbSTR2STR);
         }
       else if (stringKind == exprCString)
         {
           /* It is a 32-bit C string point.  Current stack representation is:
            *
            *   TOS(0)=address of dest string hdr
            *   TOS(1)=MS 16-bits of 32-bit C source string pointer
            *   TOS(2)=LS 16-bits of 32-bit C source string pointer
            */

           pas_StandardFunctionCall(lbCSTR2STR);
         }
     }

   /* else ... type mismatch error already reported by pas_Expression() */
}

/***********************************************************************/
/* Process a multiple word assignment statement */

static void pas_LargeAssignment(uint16_t storeOp, exprType_t assignType,
                                symbol_t *varPtr, symbol_t *typePtr)
{
   TRACE(g_lstFile,"[pas_LargeAssignment]");

   /* FORM:  <variable OR function identifer> := <expression> */

   if (g_token != tASSIGN) error (eASSIGN);
   else getToken();

   pas_Expression(assignType, typePtr);
   pas_GenerateDataSize(varPtr->sParm.v.size);
   pas_GenerateStackReference(storeOp, varPtr);
}

/***********************************************************************/

static void pas_GotoStatement(void)
{
   char     labelname [8];             /* Label symbol table name */
   symbol_t *label_ptr;                /* Pointer to Label Symbol */

   TRACE(g_lstFile,"[pas_GotoStatement]");

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
       /* The integer label must be non-negative */

       if (g_tknInt < 0)
         {
           error(eINVLABEL);
         }
       else
         {
           /* Find and verify the symbol associated with the label */

           (void)sprintf (labelname, "%" PRId32, g_tknInt);
           if (!(label_ptr = pas_FindSymbol(labelname, 0)))
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

               pas_GenerateDataOperation(opJMP, label_ptr->sParm.l.label);
             }
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

   TRACE(g_lstFile,"[pas_LabelStatement]");

   /* FORM:  <integer> : */

   /* Verify that the integer is a label name */

   (void)sprintf (labelName, "%" PRId32, g_tknInt);
   if (!(labelPtr = pas_FindSymbol(labelName, 0)))
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

   else if(!(labelPtr->sParm.l.unDefined))
     {
       error(eMULTLABEL);
     }
   else
     {
       /* Generate the label and indicate that it has been defined */

       pas_GenerateDataOperation(opLABEL, labelPtr->sParm.l.label);
       labelPtr->sParm.l.unDefined = false;

       /* We have to assume that we got here via a goto statement.
        * We don't have logic in place to track changes to the level
        * stack pointer (LSP) register, so we have no choice but to
        * invalidate that register now.
        */

       pas_InvalidateCurrentStackLevel();
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

  TRACE(g_lstFile,"[pas_ProcStatement]");

  /* FORM: procedure-method-statement =
   * procedure-method-specifier [ actual-parameter-list ]
   *
   * Skip over the procedure-method-statement
   */

  getToken();

  /* Get the actual parameters (if any) associated with the procedure
   * call.
   */

  size = actualParameterList(procPtr);

  /* Generate procedure call and stack adjustment (if required)
   * Upon return from the procedure, the level stack pointer (LSP)
   * may also be invalid.  However, we rely on level level logic in
   * pas_codegen.c to manage this case (as well as the function call
   * case).
   */

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
  int32_t thenLSP;
  int32_t elseLSP;

  TRACE(g_lstFile,"[pas_IfStatement]");

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

      /* Save the value of the Level Stack Pointer (LSP) here.  This will be
       * the value of the LSP at the ENDIF label if there is no ELSE <statement>
       * presentl.  We will compare the elseLSP to the thenLSP at that point.
       */

      elseLSP = pas_GetCurrentStackLevel();

      /* Parse the <statment> following the THEN token */

      pas_Statement();

      /* Save the LSP after generating the THEN <statement>.  We will compare the
       * elseLSP to the thenLSP below.
       */

      thenLSP = pas_GetCurrentStackLevel();

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

          /* Save the LSP after generating the ELSE <statement>.  We will
           * compare elseLSP to the thenLSP below.
           */

          elseLSP = pas_GetCurrentStackLevel();
        }

      /* Generate the ENDIF label here.  Note that if no ELSE <statement>
       * is present, this will be the same as the else_label.
       */

      pas_GenerateDataOperation(opLABEL, endif_label);

      /* We can get to this location through two of three pathes:  (1) through the
       * THEN <statement>, (2) from the IF <expression> if no ELSE <statement>
       * is present, or (3) from the ELSE <statement>.  If the LSP is different
       * through these two pathes, then we will have to invalidate it.
       */

      if (thenLSP != elseLSP)
        {
          pas_InvalidateCurrentStackLevel();
        }
    }
}

/***********************************************************************/

void pas_CompoundStatement(void)
{
   TRACE(g_lstFile,"[pas_CompoundStatement]");

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

   TRACE(g_lstFile,"[pas_RepeatStatement]");

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

   /* NOTE:  The current LSP setting will be correct after the repeat
    * loop because we fall through from the bottom of the loop after
    * executing the body at least once.
    */
}

/***********************************************************************/

static void pas_WhileStatement(void)
{
   uint16_t while_label    = ++g_label;  /* Top of loop label */
   uint16_t endwhile_label = ++g_label;  /* End of loop label */
   uint32_t nLspChanges;
   int32_t  topOfLoopLSP;
   bool     bCheckLSP      = false;

   TRACE(g_lstFile,"[pas_WhileStatement]");

   /* Generate WHILE <expression> DO <statement> */

   /* Skip over WHILE token */

   getToken();

   /* Set top of loop label */

   pas_GenerateDataOperation(opLABEL, while_label);

   /* Evaluate the WHILE <expression> */

   nLspChanges = pas_GetNStackLevelChanges();
   pas_Expression(exprBoolean, NULL);

   /* Generate a conditional jump to the end of the loop */

   pas_GenerateDataOperation(opJEQUZ, endwhile_label);

   /* Save the level stack pointer (LSP) at the top of the
    * loop.  When first executed, this value will depend on
    * logic prior to the loop or on values set in the
    * WHILE <expression>.  On subsequent loops, this value
    * may be determined by logic within the loop body or
    * have to restore this value when the loop terminates.
    */

   topOfLoopLSP =  pas_GetCurrentStackLevel();

   /* Does the WHILE <expression> logic set the LSP? */

   if (nLspChanges == pas_GetNStackLevelChanges())
     {
       /* Yes, then the value set in the WHILE <expression>
        * is the one that will be in effect at the end_while
        * label.
        */

       bCheckLSP = true;
     }

   /* Verify that the DO token follows the expression */

   if (g_token !=  tDO) error(eDO);
   else getToken();

   /* Generate the <statement> following the DO token */

   pas_Statement();

   /* Generate a branch to the top of the loop */

   pas_GenerateDataOperation(opJMP, while_label);

   /* Set the bottom of loop label */

   pas_GenerateDataOperation(opLABEL, endwhile_label);

   /* We always get here from the check at the top of the loop.
    * Normally this will be from the branch from the bottom of
    * the loop to the top of the loop.  Then from the conditional
    * branch at the top of the loop to here.
    *
    * But, we need to allow for the special case when the body
    * of the while loop never executed.  The flag bCheckLSP is
    * set true if the conditional expression evaluation does not
    * set the LSP.  In the case, the current LSP will be either
    * the LSP at the top of the loop (if he body was never executed)
    * or the current LSP (the body executes at least once).
    */

   if (bCheckLSP)
     {
       if (topOfLoopLSP != pas_GetCurrentStackLevel())
         {
           /* In thise case, there is uncertainty in the value of the
            * LSP and we must invalidate it.  It will be reset to the
            * correct the next time that a level stack reference is
            * performed.
            */

           pas_InvalidateCurrentStackLevel();
         }
     }
   else
     {
       /* Otherwise, make sure that the code generation logic knows
        * the correct value of the LSP at this point.
        */

       pas_SetCurrentStackLevel(topOfLoopLSP);
     }
}

/***********************************************************************/
/* This is helper function for pas_CaseStatement */

static bool pas_CheckInvalidateLSP(int32_t *pTerminalLSP)
{
  /* Check the LSP after evaluating the case <statement>. */

  int32_t caseLSP = pas_GetCurrentStackLevel();
  if (caseLSP < 0)
    {
      /* If the LSP is invalid after any case <statement>, then it could
       * be invalid at the end_case label as well.
       */

      return true;
    }
  else if (*pTerminalLSP < 0)
    {
      /* The value of the LSP at the end_case label has not
       * yet been determined.  It must be the value at the
       * end of this case <statement> (or else it is invalid)
       */

      *pTerminalLSP = caseLSP;
    }
  else if (*pTerminalLSP != caseLSP)
    {
      /* The value of the LSP at the end of this case <statement> is
       * different from the value of the LSP at the end of some other
       * case <statement>.  The value of the LSP at the end_case label
       * will be indeterminate and must be invalidated.
       */

      return true;
    }
  /* So far so good */

  return false;
}

static void pas_CaseStatement(void)
{
   uint16_t this_case;
   uint16_t next_case      = ++g_label;
   uint16_t end_case       = ++g_label;
   int32_t  terminalLSP    = -1;
   bool     bInvalidateLSP = false;

   TRACE(g_lstFile,"[pas_CaseStatement]");

   /* Process "CASE <expression> OF" */

   /* Skip over the CASE token */

   getToken();

   /* Evaluate the CASE <expression> */

   pas_Expression(exprAnyOrdinal, NULL);

   /* Verify that CASE <expression> is followed with the OF token */

   if (g_token !=  tOF) error (eOF);
   else getToken();

   /* Loop to process each case until END encountered */

   for (;;)
     {
       this_case = next_case;
       next_case = ++g_label;

       /* Process NON-STANDARD ELSE <statement> END */

       if (g_token == tELSE)
         {
           getToken();

           /* Set ELSE statement label */

           pas_GenerateDataOperation(opLABEL, this_case);

           /* Evaluate ELSE statement */

           pas_Statement();

           /* Check the LSP after evaluating the ELSE <statement>. */

           if (pas_CheckInvalidateLSP(&terminalLSP))
             {
               /* The LSP will be invalid at the end case label.  Set
                * a flag so that we can handle invalidation of the LSP when
                * we get to the end case label.
                */

               bInvalidateLSP = true;
             }

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
           /* Loop for each <constant> in the case list */

           for(;;)
             {
               /* Verify that we have a constant */

               if (!isConstant(g_token))
                 {
                   error(eINTCONST);
                   break;
                 }

               /* Generate a comparison of the CASE expression and the constant.
                *
                * First duplicate the value to be compared (from the CASE <expression>)
                * and push the comparison value (from the <constant>:)
                */

               pas_GenerateSimple(opDUP);
               pas_GenerateDataOperation(opPUSH, g_tknInt);

               /* The kind of comparison we generate depends on if we have to
                * jump over other case selector comparsions to the statement
                * or if we can just fall through to the statement
                */

               /* Skip over the constant */

               getToken();

               /* If there are multiple constants, they will be separated with
                * commas.
                */

               if (g_token == ',')
                 {
                   /* Generate jump to <statement> */

                   pas_GenerateDataOperation(opJEQUZ, this_case);

                   /* Skip over comma */

                   getToken();
                 }
               else
                 {
                   /* else jump to the next case */

                   pas_GenerateDataOperation(opJNEQZ, next_case);
                   break;
                 }
             }

           /* Then process ... : <statement> */

           /* Verify colon presence */

           if (g_token != ':') error(eCOLON);
           else getToken();

           /* Set CASE label */

           pas_GenerateDataOperation(opLABEL, this_case);

           /* Evaluate <statement> */

           pas_Statement();

           /* Jump to exit CASE */

           pas_GenerateDataOperation(opJMP, end_case);

           /* Check the LSP after evaluating the case <statement>. */

           if (pas_CheckInvalidateLSP(&terminalLSP))
             {
               /* If the LSP will be invalid at the end case label.  Set
                * a flag so that we can handle invalidation of the LSP when
                * we get to the end case label.
                */

               bInvalidateLSP = true;
             }
         }

       /* Check if there are more statements.  If not, verify END present */

       if (g_token == ';')
         {
           getToken();
         }
       else if (g_token == tEND)
         {
           getToken();
           break;
         }
       else
         {
           error (eEND);
           break;
         }
     }

   /* Generate ENDCASE label and Pop CASE <expression> from stack */

   pas_GenerateDataOperation(opLABEL, end_case);
   pas_GenerateDataOperation(opINDS, -sINT_SIZE);

   /* We may have gotten to this point from many different case <statements>.
    * The flag bInvalidateLSP will be set if the LSP is not the same for
    * each of these pathes.  Invalidating the LSP will force it to be reloaded
    * when the next level stack access is done.
    */

   if (bInvalidateLSP)
     {
       pas_InvalidateCurrentStackLevel();
     }
}

/***********************************************************************/
static void pas_ForStatement(void)
{
   symbol_t *varPtr;
   uint16_t forLabel    = ++g_label;
   uint16_t endForLabel = ++g_label;
   uint16_t jmpOp;
   uint16_t modOp;
   int32_t topOfLoopLSP;

   TRACE(g_lstFile,"[pas_ForStatement]");

   /* FOR <assigment statement> <TO, DOWNTO> <expression> DO <statement> */

   /* Skip over the FOR token */

   getToken();

   /* Get and verify the left side of the assignment. */

   if (g_token != sINT && g_token != sSUBRANGE)
     {
       error(eINTVAR);
     }
   else
     {
       /* Save the token associated with the left side of the assignment
        * and evaluate the integer assignment.
        */

       varPtr = g_tknPtr;
       getToken();

       /* Generate the assignment to the integer variable */

       pas_Assignment(opSTS, exprInteger, varPtr, varPtr->sParm.v.parent);

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

       pas_Expression(exprInteger, varPtr->sParm.v.parent);

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

       /* Save the level stack pointer (LSP) at the top of the FOR
        * loop.  When first executed, this value will depend on
        * logic prior to the loop body. On subsequent loops, this
        * value may be determined by logic within the loop body.
        */

       topOfLoopLSP = pas_GetCurrentStackLevel();

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

       /* We always get here from the check at the top of the loop.
        * Normally this will be from the branch from the bottom of
        * the loop to the top of the loop.  Then from the conditional
        * branch at the top of the loop to here.
        *
        * But, we need to allow for the special case when the body
        * of the for loop never executed.  In this case, the LSP at
        * the first time into the loop may differ from the LSP at
        * subsequent times into the loop.  If this is the case, then
        * will will have to invalidate the LSP.
        */

       if (topOfLoopLSP != pas_GetCurrentStackLevel())
         {
           /* In thise case, there is uncertainty in the value of the
            * LSP and we must invalidate it.  It will be reset to the
            * correct the next time that a level stack reference is
            * performed.
            */

           pas_InvalidateCurrentStackLevel();
         }
     }
}

/***********************************************************************/
static void pas_WithStatement(void)
{
   with_t saveWithRecord;

   TRACE(g_lstFile,"[pas_WithStatement]");

   /* Generate WITH <variable[,variable[...]] DO <statement> */

   /* Save the current WITH pointer.  Only one WITH can be active at
    * any given time.
    */

   saveWithRecord = g_withRecord;

   /* Process each RECORD or RECORD OBJECT in the <variable> list */

   getToken();
   for(;;)
     {
       /* A RECORD type variable may be used in the WITH statement only if
        * there is no other WITH active
        */

       if ((g_token == sRECORD) && (!g_withRecord.parent))
         {
           /* Save the RECORD variable as the new with record */

           g_withRecord.level   = g_tknPtr->sLevel;
           g_withRecord.pointer = false;
           g_withRecord.varParm = false;
           g_withRecord.offset  = g_tknPtr->sParm.v.offset;
           g_withRecord.parent  = g_tknPtr->sParm.v.parent;

           /* Skip over the RECORD variable */

           getToken();
         }

       /* A RECORD VAR parameter may also be used in the WITH statement
        * (again only if there is no other WITH active)
        */

       else if ((g_token == sVAR_PARM) &&
                (!g_withRecord.parent) &&
                (g_tknPtr->sParm.v.parent->sParm.t.type == sRECORD))
         {
           /* Save the RECORD VAR parameter as the new with record */

           g_withRecord.level   = g_tknPtr->sLevel;
           g_withRecord.pointer = true;
           g_withRecord.varParm = true;
           g_withRecord.offset  = g_tknPtr->sParm.v.offset;
           g_withRecord.parent  = g_tknPtr->sParm.v.parent;

           /* Skip over the RECORD VAR parameter */

           getToken();
         }

       /* A pointer to a RECORD may also be used in the WITH statement
        * (again only if there is no other WITH active)
        */

       else if ((g_token == sPOINTER) &&
                (!g_withRecord.parent) &&
                (g_tknPtr->sParm.v.parent->sParm.t.type == sRECORD))
         {
           /* Save the RECORD pointer as the new with record */

           g_withRecord.level   = g_tknPtr->sLevel;
           g_withRecord.pointer = true;
           g_withRecord.pointer = false;
           g_withRecord.offset  = g_tknPtr->sParm.v.offset;
           g_withRecord.parent  = g_tknPtr->sParm.v.parent;

           /* Skip over the RECORD pointer */

           getToken();

           /* Verify that deferencing is specified! */

           if (g_token != '^') error(eRECORDVAR);
           else getToken();
         }

       /* A RECORD_OBJECT may be used in the WITH statement if the field
        * is from the same sRECORD type and is itself of type RECORD.
        */

       else if ((g_token == sRECORD_OBJECT) &&
                (g_tknPtr->sParm.r.record == g_withRecord.parent) &&
                (g_tknPtr->sParm.r.parent->sParm.t.type == sRECORD))
         {
           /* Okay, update the with record to use this record field */

           if (g_withRecord.pointer)
             g_withRecord.index += g_tknPtr->sParm.r.offset;
           else
             g_withRecord.offset += g_tknPtr->sParm.r.offset;

           g_withRecord.parent  = g_tknPtr->sParm.r.parent;

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
