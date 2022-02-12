/***************************************************************************
 * pofflib.h
 * Interfaces to the POFF library
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

#ifndef __POFFLIB_H
#define __POFFLIB_H

/***************************************************************************
 * Included Files
 ***************************************************************************/

#include <stdint.h>
#include "pas_debug.h"
#include "poff.h"

/***************************************************************************
 * Pre-processor Definitions
 ***************************************************************************/

/***************************************************************************
 * Public Types
 ***************************************************************************/

/* The internal form of the POFF data structures are hidden from the caller
 * in these "handles"
 */

typedef void *poffHandle_t;
typedef void *poffProgHandle_t;
typedef void *poffSymHandle_t;
typedef void *poffRelocHandle_t;

/* This is a externally visible form of a symbol table entry that is
 * not entangled in the POFF internal string table logic.
 */

struct poffLibSymbol_s
{
  /* type is the type of symbol described by this entry.
   * See the STT_ definitions in poff.h.
   */

  uint8_t type;

  /* For data section symbols, the following provides the required
   * data space alignment for the symbol memory representation.  For
   * procedures and functions, this value is ignored. See the STT_
   * definitions in poff.h
   */

  uint8_t align;

  /* These flags describe the characteristics of the symbol.  See the
   * STF_ definitions above.
   */

  uint8_t flags;

  /* name is a reference to the symbol name in the string table
   * section data.
   */

  const char *name;

  /* value is the value associated with symbol.  For defined data
   * section symbols, this is the offset into the initialized data
   * section data; for defined procedures and functions, this the
   * offset into program section data.  For undefined symbols, this
   * valid can be used as as addend.
   */

  uint32_t value;

  /* For data section symbols, this is the size of the initialized
   * data region associated with the symbol.
   */

  uint32_t size;
};

typedef struct poffLibSymbol_s poffLibSymbol_t;

/* The externally visible form of a line number structure.  Line numbers
 * are associated with executable program data sections.
 */

struct poffLibLineNumber_s
{
  /* This is the source file line number */

  uint32_t lineno;

  /* This is the full filename of the file containing the line number. */

  const char *filename;

  /* This is an offset to the beginning code in the program data section
   * associated with this line number.
   */

   uint32_t offset;
};

typedef struct poffLibLineNumber_s poffLibLineNumber_t;

/* The externally visible form of a debug function info structure.
 */

struct poffLibDebugFuncInfo_s
{
  /* For use outside of libpoff so that the allocated debug
   * information can be retained in a list.
   */

  struct poffLibDebugFuncInfo_s *next;

  /* This is the address or label of the function/procedure entry
   * point.
   */

  uint32_t value;

  /* This is the size of the value returned by the function in
   * bytes (zero for procedures).
   */

  uint32_t retsize;

  /* This is the number of parameters accepted by the function/
   * procedure.
   */

  uint32_t nparms;

  /* This is the beginning of a table of input parameter sizes
   * the actually allocate size will be nparms entries.
   */

  uint32_t argsize[1];
};

typedef struct poffLibDebugFuncInfo_s poffLibDebugFuncInfo_t;

#define SIZEOFDEBUFINFO(n) (sizeof(poffLibDebugFuncInfo_t) + ((n)-1)*sizeof(uint32_t))

/***************************************************************************
 * Public Variables
 ***************************************************************************/

/***************************************************************************
 * Public Function Prototypes
 ***************************************************************************/

/* Functions to create/destroy a handle to POFF file data */

poffHandle_t poffCreateHandle(void);
void         poffDestroyHandle(poffHandle_t handle);
void         poffResetAccess(poffHandle_t handle);

/* Functions to manage writing a POFF file */

void         poffSetFileType(poffHandle_t handle, uint8_t fh_type,
               uint16_t nfiles, const char *name);
void         poffSetArchitecture(poffHandle_t handle, uint8_t fh_arch);
void         poffSetEntryPoint(poffHandle_t handle, uint32_t entryPoint);
int32_t      poffFindString(poffHandle_t handle, const char *string);
uint32_t     poffAddString(poffHandle_t handle, const char *string);
uint32_t     poffAddFileName(poffHandle_t handle, const char *name);
void         poffAddProgByte(poffHandle_t handle, uint8_t progByte);
#if 0 /* not used */
uint32_t     poffAddRoDataByte(poffHandle_t handle, uint8_t dataByte);
#endif
uint32_t     poffAddRoDataString(poffHandle_t handle,
               const char *string);
uint32_t     poffAddSymbol(poffHandle_t handle, poffLibSymbol_t *symbol);
uint32_t     poffAddLineNumber(poffHandle_t handle, uint16_t lineNumber,
                uint16_t fileNumber, uint32_t progSectionDataOffset);
uint32_t     poffAddDebugFuncInfo(poffHandle_t handle,
                poffLibDebugFuncInfo_t *pContainer);
uint32_t     poffAddRelocation(poffHandle_t handle, uint8_t relocType,
                uint32_t symIndex, uint32_t sectionDataOffset);
void         poffWriteFile(poffHandle_t handle, FILE *poffFile);

/* Functions to manage reading a POFF file */

uint16_t     poffReadFile(poffHandle_t handle, FILE *poffFile);
uint8_t      poffGetFileType(poffHandle_t handle);
uint8_t      poffGetArchitecture(poffHandle_t handle);
uint32_t     poffGetEntryPoint(poffHandle_t handle);
const char  *poffGetFileHdrName(poffHandle_t handle);
uint32_t     poffGetRoDataSize(poffHandle_t handle);
int32_t      poffGetFileName(poffHandle_t handle, const char **fname);
int          poffGetProgByte(poffHandle_t handle);
int32_t      poffGetSymbol(poffHandle_t handle,
               poffLibSymbol_t *symbol);
const char  *poffGetString(poffHandle_t handle, uint32_t index);
int32_t      poffGetLineNumber(poffHandle_t handle,
               poffLibLineNumber_t *lineno);
int32_t      poffGetRawLineNumber(poffHandle_t handle,
               poffLineNumber_t *lineno);
void         poffResetRelocationTraversal(poffHandle_t handle);
int32_t      poffNextRelocation(poffHandle_t handle,
               poffRelocation_t *reloc);
poffLibDebugFuncInfo_t *poffGetDebugFuncInfo(poffHandle_t handle);
poffLibDebugFuncInfo_t *poffCreateDebugInfoContainer(uint32_t nparms);
void         poffReleaseDebugFuncContainer(poffLibDebugFuncInfo_t *pDebugFuncInfo);
void         poffDiscardDebugFuncInfo(poffHandle_t handle);
int32_t      poffProgTell(poffHandle_t handle);
int          poffProgSeek(poffHandle_t handle, uint32_t offset);
uint32_t     poffGetProgSize(poffHandle_t handle);
void         poffReleaseProgData(poffHandle_t handle);

/* Functions used to manage modifications to a POFF file using a
 * temporary container for the new program data.
 */

poffProgHandle_t poffCreateProgHandle(void);
void         poffDestroyProgHandle(poffProgHandle_t handle);
void         poffResetProgHandle(poffProgHandle_t handle);
uint16_t     poffAddTmpProgByte(poffProgHandle_t handle,
               uint8_t progByte);
uint16_t     poffWriteTmpProgBytes(uint8_t *buffer, uint32_t nbyte,
                poffProgHandle_t handle);
void         poffReplaceProgData(poffHandle_t handle,
                poffProgHandle_t progHandle);

/* Functions used to manage modifications to a POFF file using a
 * temporary container for the new symbol data.
 */

poffSymHandle_t poffCreateSymHandle(void);
void         poffDestroySymHandle(poffSymHandle_t handle);
void         poffResetSymHandle(poffSymHandle_t handle);
uint32_t     poffAddTmpSymbol(poffHandle_t handle, poffSymHandle_t symHandle,
                poffLibSymbol_t *symbol);
void         poffReplaceSymbolTable(poffHandle_t handle,
                poffSymHandle_t symHandle);

/* Functions used to manage modifications to a POFF file using a
 * temporary container for the new/modified relocation data.
 */

poffRelocHandle_t poffCreateTmpRelocHandle(void);
void         poffDestroyTmpRelocHandle(poffRelocHandle_t handle);
void         poffResetTmpRelocHandle(poffRelocHandle_t handle);
void         poffCloneRelocations(poffHandle_t handle,
                poffRelocHandle_t relocHandle);
void         poffResetTmpRelocationTraversal(poffRelocHandle_t relocHandle);
int32_t      poffNextTmpRelocation(poffRelocHandle_t relocHandle,
                poffRelocation_t *reloc);
uint32_t     poffAddTmpRelocation(poffRelocHandle_t relocHandle,
                const poffRelocation_t *reloc);
void         poffReplaceRelocationTable(poffHandle_t handle,
                poffRelocHandle_t relocHandle);

/* Functions used to extract/insert whole data sections from/into a POFF
 * file container
 */

uint32_t     poffExtractProgramData(poffHandle_t handle,
                uint8_t **progData);
void         poffInsertProgramData(poffHandle_t handle,
                uint8_t *progData, uint32_t progSize);
uint32_t     poffExtractRoData(poffHandle_t handle,
                uint8_t **roData);
void         poffAppendRoData(poffHandle_t handle,
                uint8_t *roData, uint32_t roDataSize);

/* Functions to manage printing of the POFF file content */

void         poffDumpFileHeader(poffHandle_t handle, FILE *outFile);
void         poffDumpSectionHeaders(poffHandle_t handle, FILE *outFile);
void         poffDumpSymbolTable(poffHandle_t handle, FILE *outFile);
void         poffDumpRelocTable(poffHandle_t handle, FILE *outFile);

/* Helper functions to manage resolution of labels in POFF files.  These
 * just store and retrieve information by label number.
 */

void         poffAddToDefinedLabelTable(uint32_t label, uint32_t pc);
void         poffAddToUndefinedLabelTable(uint32_t label,
                uint32_t symIndex);
int          poffGetSymIndexForUndefinedLabel(uint32_t label);
int          poffGetPcForDefinedLabel(uint32_t label);
void         poffReleaseLabelReferences(void);

/* Helper functions for line numbers */

void         poffReadLineNumberTable(poffHandle_t handle);
poffLibLineNumber_t *poffFindLineNumber(uint32_t offset);
void         poffReleaseLineNumberTable(void);

/* Helper functions for debug information */

void         poffReadDebugFuncInfoTable(poffHandle_t handle);
poffLibDebugFuncInfo_t *poffFindDebugFuncInfo(uint32_t offset);
void         poffReplaceDebugFuncInfo(poffHandle_t handle);
void         poffReleaseDebugFuncInfoTable(void);

#endif /* __POFFLIB_H */
