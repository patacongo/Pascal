/****************************************************************************
 * popt_loadstore.c
 * Load/Store Optimizations
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

#include "pas_debug.h"
#include "pas_machine.h"
#include "insn16.h"

#include "paslib.h"
#include "popt.h"
#include "popt_peephole.h"
#include "popt_local.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************/
/* Check if the opcode at this index:  (1) pushs some data on the stack and
 * (2) does not depend on prior stack content.
 */

static bool popt_CheckDataOperation(int16_t index)
{
  return (g_opPtr[index]->op == oPUSH   || g_opPtr[index]->op == oPUSHB ||
          g_opPtr[index]->op == oUPUSHB ||
          g_opPtr[index]->op == oLD     || g_opPtr[index]->op == oLDB  ||
          g_opPtr[index]->op == oULDB   ||
          g_opPtr[index]->op == oLDS    || g_opPtr[index]->op == oLDSB ||
          g_opPtr[index]->op == oULDSB  ||
          g_opPtr[index]->op == oLA     || g_opPtr[index]->op == oLAS   ||
          g_opPtr[index]->op == oLAC);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/


/****************************************************************************/

int16_t popt_StackOrderOptimize(void)
{
  int16_t  nchanges = 0;
  register int16_t i;

  TRACE(stderr, "[popt_StackOrderOptimize]");

  /* At least three pcodes are needed to perform the following Store
   * optimizations.
   */

  i = 2;
  while (i < g_nOpPtrs)
    {
      /* Search for an exchange instruction */

      switch (g_opPtr[i]->op)
        {
        case oXCHG :  /* (Two 16-bit stack arguments) */
          if (popt_CheckDataOperation(i - 1) &&
              popt_CheckDataOperation(i - 2))
            {
              popt_SwapPCodePair(i - 1, i - 2);
              popt_DeletePCode(i);
            }
          else
            {
              i++;
            }
          break;

        case oDISCARD :  /* (One 16-bit stack argument) */
          if (popt_CheckDataOperation(i - 1))
            {
              popt_DeletePCodePair(i, i - 1);
            }
          else
            {
              i++;
            }
          break;

        default :
          i++;
          break;
        }
    }

  return nchanges;
}
/****************************************************************************/

int16_t popt_LoadOptimize(void)
{
  uint16_t val;
  int16_t  nchanges = 0;
  register int16_t i;

  TRACE(stderr, "[popt_LoadOptimize]");

  /* At least two pcodes are need to perform Load optimizations */

  i = 0;
  while (i < g_nOpPtrs - 1)
    {
      switch (g_opPtr[i]->op)
        {
          /* Eliminate duplicate loads.  Limited to simple, un-indexed loads
           * that result in 16-bit values on the stack.
           */

        case oLD    :
        case oLDB   :
        case oULDB  :
        case oLDS   :
        case oLDSB  :
        case oULDSB :
          if ((g_opPtr[i + 1]->op   == g_opPtr[i]->op) &&
              (g_opPtr[i + 1]->arg1 == g_opPtr[i]->arg1) &&
              (g_opPtr[i + 1]->arg2 == g_opPtr[i]->arg2))
            {
              g_opPtr[i + 1]->op   = oDUP;
              g_opPtr[i + 1]->arg1 = 0;
              g_opPtr[i + 1]->arg2 = 0;
              nchanges++;
              i += 2;
            }
          else i++;
          break;

          /* Convert loads indexed by a constant to unindexed loads */

        case oPUSH  :
        case oPUSHB  :
        case oUPUSHB  :
          /* Get the index value */

          if (g_opPtr[i]->op == oPUSH)
            {
              val = g_opPtr[i]->arg2;
            }
          else if (g_opPtr[i]->op == oPUSHB)
            {
              val = signExtend8(g_opPtr[i]->arg1);
            }
          else /* if (g_opPtr[i]->op == oUPUSHB) */
            {
              val = g_opPtr[i]->arg1;
            }

          /* If the following instruction is a load, add the constant
           * index value to the address and switch the opcode to the
           * unindexed form.
           */

          if (g_opPtr[i + 1]->op == oLDSX)
            {
              g_opPtr[i + 1]->op    = oLDS;
              g_opPtr[i + 1]->arg2 += val;
              popt_DeletePCode(i);
              nchanges++;
            }
          else if (g_opPtr[i + 1]->op == oLASX)
            {
              g_opPtr[i + 1]->op    = oLAS;
              g_opPtr[i + 1]->arg2 += val;
              popt_DeletePCode(i);
              nchanges++;
            }
          else if (g_opPtr[i + 1]->op == oLDSXB)
            {
              g_opPtr[i + 1]->op    = oLDSB;
              g_opPtr[i + 1]->arg2 += val;
              popt_DeletePCode(i);
              nchanges++;
            }
          else if (g_opPtr[i + 1]->op == oULDSXB)
            {
              g_opPtr[i + 1]->op    = oULDSB;
              g_opPtr[i + 1]->arg2 += val;
              popt_DeletePCode(i);
              nchanges++;
            }
          else if (g_opPtr[i + 1]->op == oLDSXM)
            {
              g_opPtr[i + 1]->op    = oLDSM;
              g_opPtr[i + 1]->arg2 += val;
              popt_DeletePCode(i);
              nchanges++;
            }

#if 0 /* Done elsewhere */
          else if (g_opPtr[i]->op == oPUSH)
            {
              if ((int16_t)val >= MINSHORTINT &&
                  (int16_t)val <= MAXSHORTINT)
                {
                  g_opPtr[i]->op   = oPUSHB;
                  g_opPtr[i]->arg1 = val;
                  g_opPtr[i]->arg2 = 0;
                  nchanges++;
                }
              else if ((uint16_t)val <= MAXSHORTWORD)
                {
                  g_opPtr[i]->op   = oUPUSHB;
                  g_opPtr[i]->arg1 = val;
                  g_opPtr[i]->arg2 = 0;
                  nchanges++;
                }

              i++;
            }
#endif
          else
            {
              i++;
            }
          break;

        default :
          i++;
          break;
        }
    }

  return nchanges;
}

/****************************************************************************/

int16_t popt_StoreOptimize (void)
{
  int16_t  nchanges = 0;
  register int16_t i;

  TRACE(stderr, "[popt_StoreOptimize]");

  /* At least two pcodes are needed to perform the following Store
   * optimizations.
   */

  i = 0;
  while (i < g_nOpPtrs - 1)
    {
      switch (g_opPtr[i]->op)
        {
          /* Eliminate store followed by load.  Excludes multi-word access
           * because they require additional stack information (the number
           * of bytes to store).  Also excludes unsigned loads which could
           * result in a different value when following a signed agnostic
           * store.
           */

        case oST :
        case oSTB :
        case oSTS :
        case oSTSB :
          if (g_opPtr[i]->arg1 ==          g_opPtr[i + 1]->arg1         &&
              g_opPtr[i]->arg2 ==          g_opPtr[i + 1]->arg2         &&
             ((g_opPtr[i]->op  == oST   && g_opPtr[i + 1]->op == oLD)   ||
              (g_opPtr[i]->op  == oSTB  && g_opPtr[i + 1]->op == oLDB)  ||
              (g_opPtr[i]->op  == oSTS  && g_opPtr[i + 1]->op == oLDS)  ||
              (g_opPtr[i]->op  == oSTSB && g_opPtr[i + 1]->op == oLDSB)))
            {
              g_opPtr[i + 1]->op = g_opPtr[i]->op;
              g_opPtr[i]->op     = oDUP;
              g_opPtr[i]->arg1   = 0;
              g_opPtr[i]->arg2   = 0;
              nchanges++;
              i += 2;
            }
          else
            {
              i++;
            }
          break;

          /* Convert stores indexed by a constant to unindexed stores */

       case oPUSH :
          /* If the following instruction is a store, add the constant
           * index value to the address and switch the opcode to the
           * unindexed form.
           */

          if (i < g_nOpPtrs - 2)
            {
              if (g_opPtr[i + 2]->op == oSTSX)
                {
                  g_opPtr[i + 2]->op    = oSTS;
                  g_opPtr[i + 2]->arg2 += g_opPtr[i]->arg2;
                  popt_DeletePCode(i);
                  nchanges++;
                }
              else if (g_opPtr[i + 2]->op == oSTSXB)
                {
                  g_opPtr[i + 2]->op    = oSTSB;
                  g_opPtr[i + 2]->arg2 += g_opPtr[i]->arg2;
                  popt_DeletePCode(i);
                  nchanges++;
                }
              else
                {
                   i++;
                }
            }
          else
            {
              i++;
            }
          break;

        case oPUSHB :
        case oUPUSHB :
          if (i < g_nOpPtrs - 2)
            {
              if (g_opPtr[i + 2]->op == oSTSXB)
                {
                  int16_t offset;
                  if (g_opPtr[i]->op == oPUSHB)
                    {
                      offset = signExtend8(g_opPtr[i]->arg1);
                    }
                  else
                    {
                      offset = g_opPtr[i]->arg1;
                    }

                  g_opPtr[i + 2]->op    = oSTSB;
                  g_opPtr[i + 2]->arg2 += offset;
                  popt_DeletePCode(i);
                  nchanges++;
                }
              else
                {
                  i++;
                }
            }
          else
            {
              i++;
            }
          break;

        default :
          i++;
          break;
        }
    }

  return nchanges;
}
