/***************************************************************************
 * pas_main.h
 * External Declarations associated with pas_main.c
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

#ifndef __PAS_MAIN_H
#define __PAS_MAIN_H

/***************************************************************************
 * Compilation Switches
 ***************************************************************************/

#define LSTTOFILE 1

/***************************************************************************
 * Included Files
 ***************************************************************************/

#include <sys/types.h>
#include <stdint.h>
#include "pas_defns.h"
#include "pofflib.h"

/***************************************************************************
 * Pre-processor Definitions
 ***************************************************************************/

/* This is a helper macro just to make things pretty in the source code */

#define FP0 (&g_fileState[0])                  /* Main file description */
#define FP  (&g_fileState[g_includeIndex])     /* Current file description */
#define FPP (&g_fileState[g_includeIndex - 1]) /* Previous file description */
#define IS_NESTED_UNIT ((g_includeIndex > 0) && (FP->kind == eIsUnit))

/***************************************************************************
 * Public Types
 ***************************************************************************/

/***************************************************************************
 * Public Data
 ***************************************************************************/

extern uint16_t    g_token;                /* Current token */
extern uint16_t    g_tknSubType;           /* Extended token type */
extern uint32_t    g_tknUInt;              /* Unsigned integer token value */
extern double      g_tknReal;              /* Real token value */
extern symbol_t   *g_tknPtr;               /* Pointer to symbol token */
extern fileState_t g_fileState[MAX_INCL];  /* State of all open files */

/* g_sourceFileName : Source file name from command line
 * g_includePath[]  : Pathes to search when including file
 */

extern char       *g_sourceFileName;
extern char       *g_includePath[MAX_INCPATHES];

extern poffHandle_t g_poffHandle;          /* Handle for POFF object */

extern FILE       *g_poffFile;             /* POFF output file */
extern FILE       *g_errFile;              /* Error file pointer */
extern FILE       *g_lstFile;              /* List file pointer */

extern with_t      g_withRecord;           /* RECORD of WITH statement */
extern int16_t     g_level;                /* Static nesting level */
extern int16_t     g_includeIndex;         /* Include file index */
extern int16_t     g_nIncPathes;           /* Number pathes in g_includePath[] */
extern uint16_t    g_label;                /* Last label number */
extern int16_t     g_errCount;             /* Error counter */
extern int32_t     g_warnCount;            /* Warning counter */
extern int32_t     g_dStack;               /* Data stack size */

/***************************************************************************
 * Public Function Prototypes
 ***************************************************************************/

void pas_OpenNestedFile(const char *fileName);
void pas_CloseNestedFile(void);

#endif /* __PAS_MAIN_H */
