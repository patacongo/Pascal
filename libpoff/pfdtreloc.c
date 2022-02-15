/**********************************************************************
 * pfdtreloc.c
 * Dump contents of a temporary relocation buffer
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
 **********************************************************************/

/**********************************************************************
 * Included Files
 **********************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "pas_debug.h" /* Standard types */
#include "pfprivate.h" /* POFF private definitions */
#include "pofflib.h"   /* Public interfaces */

/**********************************************************************
 * Private Constant Data
 **********************************************************************/

static const char *g_relocationTypeNames[RLT_NTYPES] =
{
  "NULL",    /* Shouldn't happen */
  "PCAL",    /* Procedure/Function call */
  "LDST"     /* Load from stack base */
};

/***********************************************************************
 * Public Functions
 ***********************************************************************/

void poffDumpTmpRelocTable(poffRelocHandle_t relocHandle, FILE *outFile)
{
  poffRelocInfo_t  *relocInfo = (poffRelocInfo_t*)relocHandle;
  poffRelocation_t *prel;
  uint32_t          index;

  fprintf(outFile, "\nTmp Relocation Buffer:\n");
  fprintf(outFile, "RELO   SYMBOL     SECTION\n");
  fprintf(outFile, "TYPE   TBL INDEX  DATA OFFSET\n");

  for (index = 0;
       index < relocInfo->relocSize;
       index += sizeof(poffRelocation_t))
    {
      prel = (poffRelocation_t*)&relocInfo->relocTable[index];

      fprintf(outFile, "%-6s 0x%08" PRIx32 " 0x%08" PRIx32 "\n",
              g_relocationTypeNames[RLI_TYPE(prel->rl_info)],
              RLI_SYM(prel->rl_info),
              prel->rl_offset);
    }
}

