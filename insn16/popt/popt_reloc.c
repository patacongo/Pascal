/****************************************************************************
 * popt_reloc.c
 * Helpers for management of relocation data
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "pas_errcodes.h"
#include "pofflib.h"

#include "pas_error.h"
#include "popt_reloc.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

/* This is the buffered relocation data from the last pass */

poffRelocHandle_t g_prevTmpRelocationHandle;

/* This accumulates new relocation data for the current pass.  After the
 * final pass, this holds the final relocation data to be written to the
 * optimized object file.
 */

poffRelocHandle_t g_tmpRelocationHandle;

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************/
/* Initialize relocation support */

void createRelocationHandles(poffHandle_t poffHandle)
{
  /* Create handles for the empty containers */

  g_prevTmpRelocationHandle = poffCreateTmpRelocHandle();
  g_tmpRelocationHandle     = poffCreateTmpRelocHandle();
  if (g_prevTmpRelocationHandle == NULL || g_tmpRelocationHandle == NULL)
    {
      fatal(eNOMEMORY);
    }

  /* Clone the relocations from the input program file into the "previous"
   * tmp relocation container.  During pass1, relocation data will be
   * accumulated in the other, empty container.
   */

  poffCloneRelocations(poffHandle, g_prevTmpRelocationHandle);
}

/****************************************************************************/
/* Swap temporary relocation container handlers.  At the end of each pass,
 * the data in the temporary relocation container becomes the previous
 * relocation for the next pass.  The temporary relocation container must be
 * cleared to accumulate new relocation data for the next pass.
 */

void swapRelocationHandles(void)
{
  poffRelocHandle_t tmp;

  /* Swap containers */

  tmp                       = g_tmpRelocationHandle;
  g_tmpRelocationHandle     = g_prevTmpRelocationHandle;
  g_prevTmpRelocationHandle = tmp;

  /* Make sure that traversal of the previous relocation container (should
   * not be necessary).
   */

  poffResetTmpRelocationTraversal(g_prevTmpRelocationHandle);

  /* And reset the new, current relocation container to empty */

  poffResetTmpRelocHandle(g_tmpRelocationHandle);
}
