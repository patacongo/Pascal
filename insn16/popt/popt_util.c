/**********************************************************************
 * popt_util.c
 * Helpers for optimization
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

#include "pas_machine.h"
#include "paslib.h"
#include "insn16.h"

#include "popt.h"

/**********************************************************************
 * Public Functions
 **********************************************************************/

/***********************************************************************/
/* Turn the oPUSHB or oUPUSHB into an oPUSH op (temporarily) */

void popt_ExpandPush(opTypeR_t *opPtr)
{
  if (opPtr != NULL)
    {
      if (opPtr->op == oPUSHB)
        {
          opPtr->op   = oPUSH;
          opPtr->arg2 = signExtend8(opPtr->arg1);
          opPtr->arg1 = 0;
        }
      else if (opPtr->op == oUPUSHB)
        {
          opPtr->op   = oPUSH;
          opPtr->arg2 = opPtr->arg1;
          opPtr->arg1 = 0;
        }
    }
}

/***********************************************************************/
/* Optimize a PUSH instruction */

void popt_OptimizePush(opTypeR_t *opPtr)
{
  if (opPtr != NULL)
    {
      if (opPtr->op == oPUSH)
        {
          if ((int16_t)opPtr->arg2 >= MINSHORTINT &&
              (int16_t)opPtr->arg2 <= MAXSHORTINT)
            {
              opPtr->op   = oPUSHB;
              opPtr->arg1 = opPtr->arg2;
              opPtr->arg2 = 0;
            }
          else if ((uint16_t)opPtr->arg2 <= MAXSHORTWORD)
            {
              opPtr->op   = oUPUSHB;
              opPtr->arg1 = opPtr->arg2;
              opPtr->arg2 = 0;
            }
        }
    }
}

/***********************************************************************/
/* Power of two */

int popt_PowerOfTwo(uint32_t value)
{
  int powerOfTwo = 0;

  /* Examples:
   * Binary value    Return value
   *    1           -> 0
   *   10           -> 1
   *   11           -> -1
   *  100           -> 2
   *  101           -> -1
   *  110           -> -1
   *  111           -> -1
   * 1000           -> 3
   * etc.
   */

  while (value != 0 && (value & 1) == 0)
    {
      value >>= 1;
      powerOfTwo++;
    }

  return (value != 1) ? -1 : powerOfTwo;
}
