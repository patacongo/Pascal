/****************************************************************************
 *  popt_strings.c
 *  String-related Optimizations
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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "pofflib.h"
#include "pas_debug.h"
#include "pas_machine.h"
#include "pas_insn.h"

#include "popt.h"
#include "popt_reloc.h"
#include "popt_peephole.h"
#include "popt_strings.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************/

void popt_StringOptimization(poffHandle_t poffHandle)
{
  poffProgHandle_t poffProgHandle; /* Handle to temporary POFF object */

  TRACE(stderr, "[popt_StringOptimize]");

  /* Create a handle to a temporary object to store new POFF program
   * data.
   */

  poffProgHandle = poffCreateProgHandle();
  if (!poffProgHandle)
    {
      fprintf(stderr, "ERROR: Could not get POFF handle\n");
      exit(1);
    }

#if 0
  /* Perform some early local, peephole string optimization before we get
   * down to working on the string stack management.
   */

  popt_StringLocalOptimization(poffHandle, poffProgHandle);

  /* Setup for string stack optimization */

  swapRelocationHandles();
  insn_ResetOpCodeRead(poffHandle);
#endif

  /* Clean up garbage left from the wasteful string stack logic */

  popt_StringStackOptimize(poffHandle, poffProgHandle);

  /* Replace the original program data with the new program data */

  poffReplaceProgData(poffHandle, poffProgHandle);

  /* Release the temporary POFF object */

  poffDestroyProgHandle(poffProgHandle);
}
