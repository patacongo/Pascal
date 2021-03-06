/***************************************************************************
 * pas_initializer.h
 * External Declarations associated with pas_initializer.c
 *
 *   Copyright (C) 2021-2022 Gregory Nutt. All rights reserved.
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
 ***************************************************************************/

#ifndef __PAS_INITIIALIZER_H
#define __PAS_INITIIALIZER_H

/***************************************************************************
 * Public Data
 ***************************************************************************/

/* Describes the initialization of one variable */

union varInitValue_u
{
  int16_t  iOrdinal;         /* Integer/char/boolean/scalar value */
  double   iReal;            /* Real value */
  uint32_t iRoOffset;        /* Offset to read-only string */
  uint16_t iPointer;         /* Pointer value (NIL) */
  uint32_t iLongInt;         /* Long integer */
  uint16_t iSet[sSET_WORDS]; /* Set value */
  uint16_t iAltAccess[4];    /* Alternative access to large values */
};

typedef union varInitValue_u varInitValue_t;

struct varInitializer_s
{
  symbol_t      *iVarPtr;    /* Variable to be initialized */
  uint32_t       iBaseType;  /* Base type of the variable */
  int            iStrLen;    /* Length of string */
  varInitValue_t iValue;     /* Initial value of the variable */
};

typedef struct varInitializer_s varInitializer_t;

/***************************************************************************
 * Public Data
 ***************************************************************************/

extern int g_nInitializer;           /* The top of the initializer stack */
extern int g_levelInitializerOffset; /* Index to initializers for this level */

/***************************************************************************
 * Public Function Prototypes
 ***************************************************************************/

void pas_AddInitialValue(varInitializer_t *varInitializer);
void pas_AddFileInitializer(symbol_t *filePtr, bool preallocated,
       uint16_t fileNumber);
void pas_AddStringInitializer(symbol_t *stringPtr);
void pas_AddRecordObjectInitializer(symbol_t *recordVarPtr,
       symbol_t *recordObjectPtr);
void pas_Initialization(void);
void pas_InitializeNewString(symbol_t *typePtr);
void pas_InitializeNewFile(symbol_t *typePtr);
void pas_Finalization(void);
void pas_FinalizeNewFile(symbol_t *varPtr);

#endif /* __PAS_INITIIALIZER_H */
