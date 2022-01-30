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
#include "pinsn16.h"

#include "popt.h"
#include "popt_local.h"
#include "popt_loadstore.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************/
/* Check if the opcode at this index:  (1) pushs some data on the stack and
 * (2) does not depend on prior stack content.
 */

static inline bool popt_CheckDataOperation(int16_t index)
{
  return (g_opPtr[index]->op == oPUSHB || g_opPtr[index]->op == oPUSH ||
          g_opPtr[index]->op == oLD    || g_opPtr[index]->op == oLDH  ||
          g_opPtr[index]->op == oLDB   ||
          g_opPtr[index]->op == oLDS   || g_opPtr[index]->op == oLDSH ||
          g_opPtr[index]->op == oLDSB  ||
          g_opPtr[index]->op == oLA    || g_opPtr[index]->op == oLAS  ||
          g_opPtr[index]->op == oLAC);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

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
          /* Eliminate duplicate loads */

        case oLDSH   :
          if ((g_opPtr[i + 1]->op   == oLDSH) &&
              (g_opPtr[i + 1]->arg1 == g_opPtr[i]->arg1) &&
              (g_opPtr[i + 1]->arg2 == g_opPtr[i]->arg2))
            {
              g_opPtr[i + 1]->op   = oDUPH;
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
          /* Get the index value */

          if (g_opPtr[i]->op == oPUSH)
            {
              val = g_opPtr[i]->arg2;
            }
          else
            {
              val = g_opPtr[i]->arg1;
            }

          /* If the following instruction is a load, add the constant
           * index value to the address and switch the opcode to the
           * unindexed form.
           */

          if (g_opPtr[i + 1]->op == oLDSXH)
            {
              g_opPtr[i + 1]->op    = oLDSH;
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
          else if (g_opPtr[i + 1]->op == oLDSXM)
            {
              g_opPtr[i + 1]->op    = oLDSM;
              g_opPtr[i + 1]->arg2 += val;
              popt_DeletePCode(i);
              nchanges++;
            }
          else if (val < 256)
            {
              g_opPtr[i]->op   = oPUSHB;
              g_opPtr[i]->arg1 = val;
              g_opPtr[i]->arg2 = 0;
              i++;
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
          /* Eliminate store followed by load */

        case oSTSH :
          if ((g_opPtr[i + 1]->op   == oLDSH) &&
              (g_opPtr[i + 1]->arg1 == g_opPtr[i]->arg1) &&
              (g_opPtr[i + 1]->arg2 == g_opPtr[i]->arg2))
            {
              g_opPtr[i + 1]->op = oSTSH;
              g_opPtr[i]->op     = oDUPH;
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
              if (g_opPtr[i + 2]->op == oSTSXH)
                {
                  g_opPtr[i + 2]->op    = oSTSH;
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
          if (i < g_nOpPtrs - 2)
            {
              if (g_opPtr[i + 2]->op == oSTSXB)
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

        default :
          i++;
          break;
        }
    }

  return nchanges;
}

int16_t popt_ExchangeOptimize(void)
{
  int16_t  nchanges = 0;
  register int16_t i;

  TRACE(stderr, "[popt_ExchangeOptimize]");

  /* At least three pcodes are needed to perform the following Store
   * optimizations.
   */

  i = 2;
  while (i < g_nOpPtrs)
    {
      /* Search for an exchange instruction */

      switch (g_opPtr[i]->op)
        {
        case oXCHGH :  /* (Two 16-bit stack arguments) */
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

        case oXCHG :   /* (Two 32-bit stack arguments) */
        default :
          i++;
          break;
        }
    }

  return nchanges;
}
