/***************************************************************************
 * pas_codegen.h
 * External Declarations associated with pas_codegen.c
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
 ***************************************************************************/

#ifndef __PAS_CODEGEN_H
#define __PAS_CODEGEN_H

/***************************************************************************
 * Included Files
 ***************************************************************************/

#include <stdint.h>
#include "pas_pcode.h"

/***************************************************************************
 * Public Function Prototypes
 ***************************************************************************/

int32_t  pas_GetCurrentStackLevel(void);
void     pas_InvalidateCurrentStackLevel(void);
void     pas_SetCurrentStackLevel(int32_t dwLsp);
uint32_t pas_GetNStackLevelChanges(void);

void     pas_GenerateSimple(enum pcode_e eOpCode);
void     pas_GenerateDataOperation(enum pcode_e eOpCode, int32_t dwData);
void     pas_GenerateDataSize(int32_t dwDataSize);
void     pas_GenerateFpOperation(uint8_t fpOpcode);
void     pas_GenerateIoOperation(uint16_t ioOpcode);
void     pas_StandardFunctionCall(uint16_t libOpcode);
void     pas_GenerateLevelReference(enum pcode_e eOpCode, uint16_t wLevel,
                                    int32_t dwOffset);
void     pas_GenerateStackReference(enum pcode_e eOpCode, symbol_t *pVarPtr);
void     pas_GenerateProcedureCall(symbol_t *pProcPtr);
void     pas_GenerateLineNumber(uint16_t wIncludeNumber,
                                uint32_t dwLineNumber);
void     pas_GenerateStackExport(symbol_t *pVarPtr);
void     pas_GenerateStackImport(symbol_t *pVarPtr);
void     pas_GenerateProcedureCall(symbol_t *pProcPtr);
void     pas_GenerateDebugInfo(symbol_t *pProcPtr, uint32_t dwReturnSize);
void     pas_GenerateProcExport(symbol_t *pProcPtr);
void     pas_GenerateProcImport(symbol_t *pProcPtr);
void     pas_GeneratePoffOutput(void);

#endif /* __PAS_CODEGEN_H */
