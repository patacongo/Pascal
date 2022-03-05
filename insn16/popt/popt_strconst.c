/****************************************************************************
 * popt_strconst.c
 * Constant String Optimizations
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

/* Constant strings are fixed size, NULL terminated and read-only.  Standard
 * Pascal strings, by constrast, are variable size with no termination and
 * are modifiable.  To make constant strings compatible with other string
 * usage, the compiler converts all constant strings to standard strings
 * using the MKSTKSTR library call.
 *
 * The representation of the raw, read-only string is the same as a stanadard
 * string, but string resides in memory that is not permissible to modify.
 * If the read-only string is not modified, then it is permissible to
 * removed the costly MKSTKSTR.  By looking at the context where MKSTKSTR is
 * used, logic in this file can selectively remove the MKSTKSTR.
 *
 * This really should be done BEFORE the logic of popt_strstack.c because
 * the presence or absence of the MKSTKSTR can affect that optimization as
 * well.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include "pas_debug.h"
#include "pas_machine.h"
#include "pas_library.h"
#include "pas_errcodes.h"
#include "pas_error.h"
#include "insn16.h"

#include "paslib.h"
#include "popt.h"
#include "popt_peephole.h"
#include "popt_strings.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************/

static int16_t popt_StringConstOptimize(void)
{
  int16_t  nchanges = 0;
  register int16_t i;

  /* At least three pcodes are needed to perform these optimizations.  We are
   * looking for these sequences:
   *
   * CASE 1:  Standard strings:
   *    lbSTRCPY2, lbSTRCPYX2, lbSTRDUP, lbSTRCAT, lbSTRCMP
   *
   *    LAC
   *    LIB STRDUP
   *    LIB aaa
   *
   * This must be preceded by that puts a standard (but constant) string on
   * the stack and aaa is some operation that takes a standard string as a
   * read-only (final) parameter.
   *
   * CASE 2: Alternatively:
   *   lbSTRCPY, lbSTRCPYX
   *
   *    LAC
   *    LIB STRDUP
   *    LA/LAS
   *    LIB bbb
   *
   * REVIST:  Other cases to be addressed:
   *
   * - There are two read-only parameters to all string comparison library functions:
   *   lbSTRCMP.
   * - lbSTR2BSTR, lbSTR2BSTRX have a slightly incompatible form.
   */

  for (i = 0; i < g_nOpPtrs - 2; i++)
    {
      /* Check for LAC followed by LIB STRDUP.  That is a constant string
       * being converted to a standard string.
       */

      if (g_opPtr[i]->op != oLAC || g_opPtr[i + 1]->op != oLIB ||
          g_opPtr[i + 1]->arg2 != lbSTRDUP)
        {
        }

      /* CASE 1:  Standard strings:
       *
       *    LAC
       *    LIB STRDUP
       *    LD/LDS/PUSH string-allocation
       *    LIB aaa
       */

      else if (i < g_nOpPtrs - 3 &&
               (g_opPtr[i + 2]->op   == oLD          ||
                g_opPtr[i + 2]->op   == oLDS         ||
                g_opPtr[i + 2]->op   == oPUSH        ||
                g_opPtr[i + 2]->op   == oPUSHB)      &&
                g_opPtr[i + 3]->op   == oLIB         &&
               (g_opPtr[i + 3]->arg2 == lbSTRCPY2    ||
                g_opPtr[i + 3]->arg2 == lbSTRCPYX2   ||
                g_opPtr[i + 3]->arg2 == lbSTRDUP     ||
                g_opPtr[i + 3]->arg2 == lbSTRCAT     ||
                g_opPtr[i + 3]->arg2 == lbSTRCMP))
        {
          popt_DeletePCode(i + 1);
        }

      /* CASE 2:
       *
       *    LAC
       *    LIB STRDUP
       *    LD/LDS/PUSH string-allocation
       *    LA/LAS
       *    LIB aabbba
       */

      else if (i < g_nOpPtrs - 4 &&
               (g_opPtr[i + 2]->op   == oLD          ||
                g_opPtr[i + 2]->op   == oLDS         ||
                g_opPtr[i + 2]->op   == oPUSH        ||
                g_opPtr[i + 2]->op   == oPUSHB)      &&
               (g_opPtr[i + 3]->op   == oLA          ||
                g_opPtr[i + 3]->op   == oLAS)        &&
                g_opPtr[i + 4]->op   == oLIB         &&
               (g_opPtr[i + 4]->arg2 == lbSTRCPY    ||
                g_opPtr[i + 4]->arg2 == lbSTRCPYX))
        {
          popt_DeletePCode(i + 1);
          nchanges++;
        }
    }

  return nchanges;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************/

void popt_StringLocalOptimization(poffHandle_t poffHandle,
                                  poffProgHandle_t poffProgHandle)
{
  int16_t nchanges;

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
          nchanges  = popt_StringConstOptimize();
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
}
