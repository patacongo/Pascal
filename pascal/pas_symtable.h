/***************************************************************************
 * pas_symtable.h
 * External Declarations associated with pas_symtable.c
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
 ***************************************************************************/

#ifndef __PAS_SYMTABLE_H
#define __PAS_SYMTABLE_H

/***************************************************************************
 * Included Files
 ***************************************************************************/

#include <stdint.h>
#include "config.h"

/***************************************************************************
 * Public Data
 ***************************************************************************/

extern symbol_t    *g_parentInteger;
extern symbol_t    *g_parentString;
extern symbol_t    *g_inputFile;     /* Shortcut to the INPUT file symbol */
extern symbol_t    *g_outputFile;    /* Shortcut to the OUTPUT file symbol */
extern unsigned int g_nSym;          /* Number symbol table entries */
extern unsigned int g_nConst;        /* Number constant table entries */

/***************************************************************************
 * Public Function Prototypes
 ***************************************************************************/

const char *pas_MapToAlias(const char *name);
const reservedWord_t *
          pas_FindReservedWord(const char *name);
symbol_t *pas_FindSymbol(const char *inName, int tableOffset,
            int *foundIndex);
symbol_t *pas_FindNextSymbol(const char *inName, int tableOffset,
            int lastIndex, int *foundIndex);
symbol_t *pas_AddTypeDefine(char *name, uint8_t type, uint16_t size,
            symbol_t *parent);
symbol_t *pas_AddConstant(char *name, uint8_t type, int32_t *value,
            symbol_t *parent);
symbol_t *pas_AddStringConstant(char *name, uint32_t offset, uint32_t size);
symbol_t *pas_AddFile(char *name, uint16_t kind, uint16_t offset,
            uint16_t xfrUnit, symbol_t *typePtr);
symbol_t *pas_AddUnitName(char *name, uint16_t index);
symbol_t *pas_AddLabel(char *name, uint16_t label);
symbol_t *pas_AddProcedure(char *name, uint8_t type, uint16_t label,
            uint16_t nParms, symbol_t *parent);
symbol_t *pas_AddVariable(char *name, uint8_t type, uint16_t offset,
            uint16_t size, symbol_t *parent);
symbol_t *pas_AddField(char *name, symbol_t *record, symbol_t *lastField);
void      pas_PrimeSymbolTable(unsigned long symbolTableSize);
void      pas_VerifyLabels(int32_t symIndex);

#if CONFIG_DEBUG
void   pas_DumpTables(void);
#endif

#endif /* __PAS_SYMTABLE_H */
