/****************************************************************************
 * pas_expression.h
 * External Declarations associated with pas_expression.c
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

#ifndef __PAS_EXPRESSION_H
#define __PAS_EXPRESSION_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdbool.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Bit 7 is set in all pointer expression types.  If bit 7 is cleared,
 * then the base expression type can be recovered.
 *
 * REVISIT: How are pointers to pointers handled?
 */

#define EXPRTYPE_POINTER       0x80
#define IS_POINTER_EXPRTYPE(t) (((uint8_t)(t) & EXPRTYPE_POINTER) != 0)
#define MK_POINTER_EXPRTYPE(t) (exprType_t)((uint8_t)(t) | EXPRTYPE_POINTER)

/* Factor treatment flags.=  These options apply primarily for complex factors
 * involving ARRAYs, POINTERs, and VAR parameters:
 *
 * FACTOR_DEREFERENCE (only)
 * - Means load the value with a typical load instruction (LDS).  For example,
 *   loading the value of an integer variable.
 * FACTOR_DEREFERENCE + FACTOR_INDEXED
 * - Means load the value with an indexed load (LDSX). For example, loading the
 *   RVALUE from an array.
 * FACTOR_LOAD_ADDRESS + FACTOR_INDEXED
 * - Means load a pointer address value(LDS), then index the loaded address
 *  (ADD).  For example, RVALUE is a pointer to an array of values.
 * FACTOR_PTREXPR
 * - Use a pointer address, rather than a value of a pointer.  The only effect
 *   is to assume a pointer expression rather than a value expression.
 * FACTOR_INDEXED (only)
 * - Load value from an indexed stack address(STSX)
 * FACTOR_FIELD_OFFSET
 * - The factor is a field of a RECORD and requires an offset be applied to
 *   the address of the field in order to access the field.
 * FACTOR_VAR_PARM
 * - Does very little but distinguish if we are working with a pointer or
 *   a VAR parameter.
 */

#define FACTOR_DEREFERENCE   (1 << 0)
#define FACTOR_PTREXPR       (1 << 1)
#define FACTOR_INDEXED       (1 << 2)
#define FACTOR_LOAD_ADDRESS  (1 << 3)
#define FACTOR_FIELD_OFFSET  (1 << 4)
#define FACTOR_VAR_PARM      (1 << 5)

/****************************************************************************
 * Type Definitions
 ****************************************************************************/

/* An integer type big enough to hold all factor flags */

typedef uint8_t exprFlag_t;

/* Enumerates the types of expressions that may be encountered */

enum exprType_e
{
  /* General expression type */

  exprUnknown         = 0x00,  /* value unknown */
  exprAnyOrdinal      = 0x01,  /* any ordinal type */
  exprAnyString       = 0x02,  /* any string type */
  exprAnyPointer      = 0x03,  /* any pointer type */
  exprEmptySet        = 0x04,  /* the empty set */

  /* Standard expression types */

  exprInteger         = 0x10,  /* signed integer value */
  exprWord            = 0x11,  /* unsigned integer value */
  exprShortInteger    = 0x12,  /* signed short integer value */
  exprShortWord       = 0x13,  /* unsigned short integer value */
  exprLongInteger     = 0x14,  /* signed short integer value */
  exprLongWord        = 0x15,  /* unsigned short integer value */
  exprChar            = 0x16,  /* character value */
  exprBoolean         = 0x17,  /* boolean(integer) value */
  exprReal            = 0x18,  /* real value */
  exprScalar          = 0x19,  /* scalar(integer) value */
  exprString          = 0x1a,  /* variable length string reference */
  exprShortString     = 0x1b,  /* variable length string reference */
  exprCString         = 0x1c,  /* pointer to C string */
  exprSet             = 0x1d,  /* set(integer) value */
  exprFile            = 0x1e,  /* file */
  exprRecord          = 0x1f,  /* record */

  /* Expressions that evaluate to pointers to standard type */

  exprIntegerPtr      = 0x90,  /* pointer to a signed integer value */
  exprWordPtr         = 0x91,  /* pointer to an unsigned integer value */
  exprShortIntegerPtr = 0x92,  /* pointer to a signed short integer value */
  exprShortWordPtr    = 0x93,  /* pointer to an unsigned short integer value */
  exprLongIntegerPtr  = 0x94,  /* pointer to a signed short integer value */
  exprLongWordPtr     = 0x95,  /* pointer to an unsigned short integer value */
  exprCharPtr         = 0x96,  /* pointer to a character value */
  exprBooleanPtr      = 0x97,  /* pointer to a boolean value */
  exprRealPtr         = 0x98,  /* pointer to a real value */
  exprScalarPtr       = 0x99,  /* pointer to a scalar value */
  exprStringPtr       = 0x9a,  /* variable length string reference */
  exprShortStringPtr  = 0x9b,  /* variable length string reference */
  exprSetPtr          = 0x9d,  /* pointer to a set value */
  exprFilePtr         = 0x9e,  /* pointer to a file */
  exprRecordPtr       = 0x9f   /* pointer to a record */
};

typedef enum exprType_e exprType_t;

/* 64-bit integer types have not yet been implemented.  These definitions
 * exist as part of the to substitute LongInteger types for the unimplemented
 * Int64 types (see the alias table pas_symtable.c for the other part of the
 * kludge.
 */

#define exprInt64    exprLongInteger
#define exprInt64Ptr exprLongIntegerPtr

/****************************************************************************
 * Public Datas
 ****************************************************************************/

/* Returned characterization of the constant set by pas_ConstantExpression. */

extern int      g_constantToken;
extern int32_t  g_constantInt;
extern double   g_constantReal;
extern char    *g_constantStart;
extern uint32_t g_constantStrOffset;
extern int      g_constantStrLen;
extern uint16_t g_constantSet[sSET_WORDS];

/* The abstract types - SETs, RECORDS, etc - require an exact
 * match in type.  This variable points to the symbol table
 * sTYPE entry associated with the expression.
 *
 * Normally this is used only within pas_expression.c, but it needs to have
 * global scope for generating SET operations.
 */

extern symbol_t *g_abstractTypePtr;

/****************************************************************************
 * Public Function Protothypes
 ***************************************************************************/

exprType_t pas_Expression(exprType_t findExprType, symbol_t *typePtr);
exprType_t pas_VarParameter(exprType_t varExprType, symbol_t *typePtr);
void       pas_ArrayIndex(symbol_t *arrayTypePtr);
exprType_t pas_GetExpressionType(symbol_t *sType);
exprType_t pas_MapVariable2ExprType(uint16_t varType, bool ordinal);
exprType_t pas_MapVariable2ExprPtrType(uint16_t varType, bool ordinal);
symbol_t  *pas_GetBaseTypePointer(symbol_t *typePtr);
void       pas_ConstantExpression(exprType_t findExprType, symbol_t *typePtr);

#endif /* __PAS_EXPRESSION_H */
