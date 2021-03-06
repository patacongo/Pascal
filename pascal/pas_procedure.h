/***************************************************************************
 * pas_procedure.h
 * External Declarations associated with pas_procedure.c
 *
 *   Copyright (C) 2008, 2021 Gregory Nutt. All rights reserved.
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

#ifndef __PAS_PROCEDURE_H
#define __PAS_PROCEDURE_H

/***************************************************************************
 * Included Files
 ***************************************************************************/

#include "pas_defns.h"

/***************************************************************************
 * Public Data
 ***************************************************************************/

/* Persistent string stack data may be need to hold string value parameters.
 * That string stack must be released when the function/procedure returns.
 * This variable holds the size of the fixup.
 */

extern uint16_t g_strStackFixup;

/***************************************************************************
 * Public Function Prototypes
 ***************************************************************************/

void     pas_StandardProcedure(void);
uint16_t pas_GenerateFileNumber(uint16_t *pFileSize,
                                symbol_t *defaultFilePtr);
int      pas_ActualParameterSize(symbol_t *procPtr, int parmNo);
int      pas_ActualParameterList(symbol_t *procPtr);

#endif /* __PAS_PROCEDURE_H */
