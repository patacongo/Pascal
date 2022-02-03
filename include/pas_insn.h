/***************************************************************************
 * pas_insn.h
 * External Declarations associated libinsn.a
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
 ***************************************************************************/

#ifndef __PAS_INSN_H
#define __PAS_INSN_H

/***************************************************************************
 * Included Files
 ***************************************************************************/

#include <stdint.h>
#include "pas_pcode.h"
#include "pas_longops.h"

/***************************************************************************
 * Public Function Prototypes
 ***************************************************************************/

/* Opcode generators */

void insn_GenerateSimple(enum pcode_e opcode);
void insn_GenerateDataOperation(enum pcode_e opcode, int32_t data);
void insn_GenerateDataSize(uint32_t dwDataSize);
void insn_GenerateFpOperation(uint8_t fpOpcode);
void insn_GenerateSetOperation(uint8_t setOpcode);
void insn_GenerateIoOperation(uint16_t ioOpcode);
void insn_StandardFunctionCall(uint16_t libOpcode);
void insn_GenerateLevelReference(enum pcode_e opcode, uint16_t level,
                                 int32_t offset);
void insn_GenerateProcedureCall(uint16_t level, int32_t offset);
void insn_GenerateLineNumber(uint16_t includeNumber, uint32_t lineNumber);

void insn_GenerateSimpleLongOperation(enum longops_e longOpCode);
void insn_GenerateDataLongOperation(enum longops_e longOpCode, int32_t data);
void insn_GenerateLongLevelReference(enum longops_e longOpCode, uint16_t level,
                                     int32_t offset);

/* Opcode relocation */

int  insn_Relocate(opType_t *op, uint32_t pcOffset, uint32_t roOffset);
void insn_FixupProcedureCall(uint8_t *progData, uint32_t symValue);

/* POFF-wrapped INSNS access helpers */

uint32_t insn_GetOpCode(poffHandle_t handle, opType_t *ptr);
void insn_ResetOpCodeRead(poffHandle_t handle);
void insn_AddOpCode(poffHandle_t handle, opType_t *ptr);
void insn_ResetOpCodeWrite(poffHandle_t handle);
void insn_AddTmpOpCode(poffProgHandle_t progHandle, opType_t *ptr);
void insn_ResetTmpOpCodeWrite(poffProgHandle_t progHandle);

/* INSN-specific disassembler */

void insn_DisassemblePCode(FILE* lfile, opType_t *pop);
void insn_DisassembleLongOpCode(FILE* lfile, opType_t *pop);

#endif /* __PAS_INSN_H */
