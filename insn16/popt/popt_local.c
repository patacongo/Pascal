/****************************************************************************
 * popt_local.c
 * P-Code Local Optimizer
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "pas_debug.h"
#include "pas_pcode.h"
#include "insn16.h"

#include "pofflib.h"
#include "paslib.h"
#include "pas_insn.h"
#include "pas_errcodes.h"
#include "pas_error.h"
#include "pas_machine.h"

#include "popt.h"
#include "popt_peephole.h"
#include "popt_reloc.h"
#include "popt_local.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/*****************************************************************************/

void popt_LocalOptimization(poffHandle_t poffHandle)
{
  poffProgHandle_t poffProgHandle;
  int16_t nchanges;

  TRACE(stderr, "[popt_LocalOptimization]");

  /* Create a handle to a temporary object to store new POFF program
   * data.
   */

  poffProgHandle = poffCreateProgHandle();
  if (!poffProgHandle)
    {
      fprintf(stderr, "ERROR: Could not get POFF handle\n");
      exit(1);
    }

  /* Swap the relocation container handles.  The relocations accumulated
   * in "current" container are now the relocations from the "previous" pass.
   * The "current" container will be empty at the start of the pass.
   */

  swapRelocationHandles();

  /* Initialization */

  popt_SetupPeephole(poffHandle, poffProgHandle);

  /* Outer loop traverse the file op-code by op-code until the oEND P-Code
   * has been output.  NOTE:  it is assumed throughout that oEND is the
   * final P-Code in the program data section.
   */

  while (!g_endOut)
    {
      /* The inner loop optimizes the buffered P-Codes until no further
       * changes can be made.  Then the outer loop will advance the buffer
       * by one P-Code
       */

      do
        {
          nchanges  = popt_UnaryOptimize();
          nchanges += popt_LongUnaryOptimize();
          nchanges += popt_BinaryOptimize();
          nchanges += popt_LongBinaryOptimize();
          nchanges += popt_BranchOptimize();
          nchanges += popt_LoadOptimize();
          nchanges += popt_StoreOptimize();
          nchanges += popt_ExchangeOptimize();
        }
      while (nchanges);

      popt_UpdatePeephole();
    }

  /* All of the relocations should have been adjusted and copied to the
   * optimized output.
   */

  if (g_nextRelocationIndex >= 0)
    {
      error(eEXTRARELOCS);
    }

  /* Replace the original program data with the new program data */

  poffReplaceProgData(poffHandle, poffProgHandle);

  /* Release the temporary POFF object */

  poffDestroyProgHandle(poffProgHandle);
}
