/****************************************************************************
 * popt_peephole.h
 * External Declarations associated with popt_peephole.c
 *
 *   Copyright (C) 2022 Gregory Nutt. All rights reserved.
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

#ifndef __POPT_PEEPHOLE_H
#define __POPT_PEEPHOLE_H

/****************************************************************************
* Included Files
*****************************************************************************/

#include <stdint.h>
#include <stdbool.h>

#include "paslib.h"
#include "pas_debug.h"
#include "pas_machine.h"
#include "pofflib.h"

/****************************************************************************
* Pre-processor Definitions
*****************************************************************************/

#define WINDOW    16                    /* size of optimization window */

/****************************************************************************
 * Public Data
 ****************************************************************************/

extern opTypeR_t  g_opTable[WINDOW];    /* Pcode Table */
extern opTypeR_t *g_opPtr[WINDOW];      /* Valid Pcode Pointers */

extern int16_t    g_nOpPtrs;            /* No. Valid Pcode Pointers */
extern bool       g_endOut;             /* true: oEND pcode has been output */

/****************************************************************************
* Public Function Prototypes
*****************************************************************************/

/* Setup and teardown */

void popt_SetupPeephole          (poffHandle_t poffHandle,
                                  poffProgHandle_t poffProgHandle);
void popt_UpdatePeephole         (void);

/* Peepole management */

void popt_DeletePCode            (int16_t delIndex);
void popt_DeletePCodePair        (int16_t delIndex1, int16_t delIndex2);
void popt_DeletePCodeTrio        (int16_t delIndex1, int16_t delIndex2,
                                  int16_t delIndex3);
void popt_DeletePCodeQuartet     (int16_t delIndex1, int16_t delIndex2,
                                  int16_t delIndex3, int16_t delIndex4);
void popt_SwapPCodePair          (int16_t swapIndex1, int16_t swapIndex2);

/* Peephole queries */

bool popt_CheckDataOperation     (int16_t chkIndex);
bool popt_CheckAddressOperation  (int16_t chkIndex);
bool popt_CheckBinaryOperator    (int16_t chkIndex);
bool popt_CheckTransitiveOperator(int16_t chkIndex);
bool popt_CheckPushConstant      (int16_t chkIndex);

#endif /* __POPT_PEEPHOLE_H */
