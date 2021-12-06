/***************************************************************************
 * pas.h
 * External Declarations associated with pas.c
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

#ifndef __PAS_H
#define __PAS_H

/***************************************************************************
 * Compilation Switches
 ***************************************************************************/

#define LSTTOFILE 1

/***************************************************************************
 * Included Files
 ***************************************************************************/

#include <sys/types.h>
#include <stdint.h>
#include "pasdefs.h"
#include "pofflib.h"

/***************************************************************************
 * Pre-processor Definitions
 ***************************************************************************/

/* This is a helper macro just to make things pretty in the source code */

#define FP0 (&g_fileState[0])               /* Main file description */
#define FP  (&g_fileState[includeIndex])    /* Current file description */
#define FPP (&g_fileState[includeIndex-1])  /* Previous file description */
#define IS_NESTED_UNIT ((includeIndex > 0) && (FP->kind == eIsUnit))

/***************************************************************************
 * Public Types
 ***************************************************************************/

/***************************************************************************
 * Public Data
 ***************************************************************************/

extern uint16_t    g_token;                /* Current token */
extern uint16_t    g_tknSubType;           /* Extended token type */
extern int32_t     g_tknInt;               /* Integer token value */
extern double      g_tknReal;              /* Real token value */
extern STYPE      *g_tknPtr;               /* Pointer to symbol token */
extern FTYPE       g_files[MAX_FILES + 1]; /* File Table */
extern fileState_t g_fileState[MAX_INCL];  /* State of all open files */

/* g_sourceFileName : Source file name from command line
 * g_includePath[]  : Pathes to search when including file
 */

extern char       *g_sourceFileName;
extern char       *g_includePath[MAX_INCPATHES];

extern poffHandle_t poffHandle;            /* Handle for POFF object */

extern FILE       *poffFile;               /* POFF output file */
extern FILE       *errFile;                /* Error file pointer */
extern FILE       *lstFile;                /* List file pointer */

extern WTYPE       withRecord;             /* RECORD of WITH statement */
extern int16_t     g_level;                /* Static nesting level */
extern int16_t     includeIndex;           /* Include file index */
extern int16_t     nIncPathes;             /* Number pathes in g_includePath[] */
extern uint16_t    label;                  /* Last label number */
extern int16_t     err_count;              /* Error counter */
extern int16_t     nfiles;                 /* Program file counter */
extern int32_t     warn_count;             /* Warning counter */
extern int32_t     dstack;                 /* data stack size */

/***************************************************************************
 * Public Function Prototypes
 ***************************************************************************/

extern void openNestedFile  (const char *fileName);
extern void closeNestedFile (void);

#endif /* __PAS_H */
