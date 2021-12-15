/***********************************************************************
 * pas_expression.h
 * External Declarations associated with pas_expression.c
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
 ***********************************************************************/

#ifndef __PAS_EXPRESSION_H
#define __PAS_EXPRESSION_H

/***********************************************************************
 * Included Files
 ***********************************************************************/

#include <stdint.h>
#include <stdbool.h>

/***********************************************************************
 * Pre-processor Definitions
 ***********************************************************************/

/* Bit 7 is set in all pointer expression types.  If bit 7 is cleared,
 * then the base expression type can be recovered.
 *
 * REVISIT: How are pointers to pointers handled?
 */

#define EXPRTYPE_POINTER       0x80
#define IS_POINTER_EXPRTYPE(t) (((uint8_t)(t) & EXPRTYPE_POINTER) != 0)

/* Factor treatment flags */

/* REVIST: duplicated in pas_statement.c and pas_constexpr.c */

#define ADDRESS_DEREFERENCE 0x01
#define ADDRESS_FACTOR      0x02
#define INDEXED_FACTOR      0x04
#define VAR_PARM_FACTOR     0x08

/***********************************************************************
 * Type Definitions
 ***********************************************************************/

enum exprType_e
{
  /* General expression type */

  exprUnknown    = 0x00,    /* TOS value unknown */
  exprAnyOrdinal = 0x01,    /* TOS = any ordinal type */
  exprAnyString  = 0x02,    /* TOS = any string type */

  /* Standard expression types */

  exprInteger    = 0x03,    /* TOS = integer value */
  exprReal       = 0x04,    /* TOS = real value */
  exprChar       = 0x05,    /* TOS = character value */
  exprBoolean    = 0x06,    /* TOS = boolean(integer) value */
  exprScalar     = 0x07,    /* TOS = scalar(integer) value */
  exprString     = 0x08,    /* TOS = variable length string reference */
  exprStkString  = 0x09,    /* TOS = reference to string on string stack */
  exprCString    = 0x0a,    /* TOS = pointer to C string */
  exprSet        = 0x0b,    /* TOS = set(integer) value */
  exprFile       = 0x0c,    /* TOS = file */
  exprRecord     = 0x0d,    /* TOS = record */

  /* Expressions that evaluate to pointers to standard type */

  exprIntegerPtr = 0x83,    /* TOS = pointer to integer value */
  exprRealPtr    = 0x84,    /* TOS = pointer to a real value */
  exprCharPtr    = 0x85,    /* TOS = pointer to a character value */
  exprBooleanPtr = 0x86,    /* TOS = pointer to a boolean value */
  exprScalarPtr  = 0x87,    /* TOS = pointer to a scalar value */
  exprSetPtr     = 0x8b,    /* TOS = pointer to a set value */
  exprFilePtr    = 0x8c,    /* TOS = pointer to a file */
  exprRecordPtr  = 0x8d     /* TOS = pointer to a record */
};

typedef enum exprType_e exprType_t;

/***********************************************************************
 * Public Datas
 ***********************************************************************/

extern int     constantToken;
extern int32_t constantInt;
extern double  constantReal;
extern char   *constantStart;

/***********************************************************************
 * Public Function Protothypes
 ***********************************************************************/

exprType_t pas_Exression(exprType_t findExprType, symbol_t *typePtr);
exprType_t pas_VarParameter(exprType_t varExprType, symbol_t *typePtr);
void       pas_ArrayIndex(symbol_t *indexTypePtr, uint16_t elemSize);
exprType_t pas_GetExpressionType(symbol_t *sType);
exprType_t pas_MapVariable2ExprType(uint16_t varType, bool ordinal);
exprType_t pas_MapVariable2ExprPtrType(uint16_t varType, bool ordinal);
void       pas_ConstantExression(void);

#endif /* __PAS_EXPRESSION_H */
