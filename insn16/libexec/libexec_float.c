/****************************************************************************
 * libexec_float.c
 *
 *   Copyright (C) 2008-2009, 2021 Gregory Nutt. All rights reserved.
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

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>
#include <math.h>

#include "pas_debug.h"
#include "pas_machine.h"
#include "insn16.h"
#include "pas_fpops.h"
#include "pas_sysio.h"
#include "pas_errcodes.h"

#include "paslib.h"
#include "libexec_sysio.h"
#include "libexec_stringlib.h"
#include "libexec.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void libexec_GetFpArguments(struct libexec_s *st, uint8_t fpop,
                                   fparg_t *arg1, fparg_t *arg2);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: libexec_GetFpArguments
 *
 * Description:
 *   This function retrieves the floating point arguments and performs
 *   integer to REAL conversions as necessary
 *
 ****************************************************************************/

static void libexec_GetFpArguments(struct libexec_s *st, uint8_t fpop,
                                   fparg_t *arg1, fparg_t *arg2)
{
  int16_t sparm;

  /* Extract arg2 from the stack */

  if (arg2)
    {
      /* Convert an integer argument to type REAL */

      if ((fpop & fpARG2) != 0)
        {
          POP(st, sparm);
          arg2->f = (double)sparm;
        }
      else
        {
          POP(st, arg2->hw[3]);
          POP(st, arg2->hw[2]);
          POP(st, arg2->hw[1]);
          POP(st, arg2->hw[0]);
        }
    }

  /* Extract arg1 from the stack */

  if (arg1)
    {
      /* Convert an integer argument to type REAL */

      if ((fpop & fpARG1) != 0)
        {
          POP(st, sparm);
          arg1->f = (double)sparm;
        }
      else
        {
          POP(st, arg1->hw[3]);
          POP(st, arg1->hw[2]);
          POP(st, arg1->hw[1]);
          POP(st, arg1->hw[0]);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: libexec_FloatOps
 *
 * Description:
 *   This function processes a floating point operation.
 *
 ****************************************************************************/

int libexec_FloatOps(struct libexec_s *st, uint8_t fpop)
{
  int16_t intValue;
  fparg_t arg1;
  fparg_t arg2;
  fparg_t result;

  switch (fpop & fpMASK)
    {
      /* Floating Pointer Conversions (On stack argument:  FP or Integer) */

    case fpFLOAT :
      POP(st, intValue);
      result.f = (double)intValue;
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;

    case fpTRUNC :
    case fpROUND :
      libexec_GetFpArguments(st, fpop, &arg1, NULL);
      intValue = (int16_t)arg1.f;
      PUSH(st, intValue);
      break;

      /* Floating Point arithmetic instructions (Two FP stack arguments) */

    case fpADD :
      libexec_GetFpArguments(st, fpop, &arg1, &arg2);
      result.f = arg1.f + arg2.f;
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;

    case fpSUB :
      libexec_GetFpArguments(st, fpop, &arg1, &arg2);
      result.f = arg1.f - arg2.f;
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;

    case fpMUL :
      libexec_GetFpArguments(st, fpop, &arg1, &arg2);
      result.f = arg1.f * arg2.f;
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;

    case fpDIV :
      libexec_GetFpArguments(st, fpop, &arg1, &arg2);
      result.f = arg1.f / arg2.f;
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;

    case fpMOD :
      return eBADFPOPCODE;
#if 0 /* Not yet */
      libexec_GetFpArguments(st, fpop, &arg1, &arg2);
      result.f = arg1.f % arg2.f;
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;
#endif

      /* Floating Point Comparisons (Two FP stack arguments) */

    case fpEQU :
      libexec_GetFpArguments(st, fpop, &arg1, &arg2);
      intValue = PASCAL_FALSE;
      if (arg1.f == arg2.f)
        {
          intValue = PASCAL_TRUE;
        }

      PUSH(st, intValue);
      break;

    case fpNEQ :
      libexec_GetFpArguments(st, fpop, &arg1, &arg2);
      intValue = PASCAL_FALSE;
      if (arg1.f != arg2.f)
        {
          intValue = PASCAL_TRUE;
        }

      PUSH(st, intValue);
      break;

    case fpLT :
      libexec_GetFpArguments(st, fpop, &arg1, &arg2);
      intValue = PASCAL_FALSE;
      if (arg1.f < arg2.f)
        {
          intValue = PASCAL_TRUE;
        }

      PUSH(st, intValue);
      break;

    case fpGTE :
      libexec_GetFpArguments(st, fpop, &arg1, &arg2);
      intValue = PASCAL_FALSE;
      if (arg1.f >= arg2.f)
        {
          intValue = PASCAL_TRUE;
        }

      PUSH(st, intValue);
      break;

    case fpGT :
      libexec_GetFpArguments(st, fpop, &arg1, &arg2);
      intValue = PASCAL_FALSE;
      if (arg1.f > arg2.f)
        {
          intValue = PASCAL_TRUE;
        }

      PUSH(st, intValue);
      break;

    case fpLTE :
      libexec_GetFpArguments(st, fpop, &arg1, &arg2);
      intValue = PASCAL_FALSE;
      if (arg1.f <= arg2.f)
        {
          intValue = PASCAL_TRUE;
        }

      PUSH(st, intValue);
      break;

      /* Floating Point arithmetic instructions (One FP stack arguments) */

    case fpNEG :
      libexec_GetFpArguments(st, fpop, &arg1, NULL);
      result.f = -arg1.f;
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;

    case fpABS :
      libexec_GetFpArguments(st, fpop, &arg1, NULL);
      result.f = fabs(arg1.f);
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;

    case fpSQR :
      libexec_GetFpArguments(st, fpop, &arg1, NULL);
      result.f = arg1.f * arg1.f;
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;

    case fpSQRT :
      libexec_GetFpArguments(st, fpop, &arg1, NULL);
      result.f = sqrt(arg1.f);
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;

    case fpSIN :
      libexec_GetFpArguments(st, fpop, &arg1, NULL);
      result.f = sin(arg1.f);
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;

    case fpCOS :
      libexec_GetFpArguments(st, fpop, &arg1, NULL);
      result.f = cos(arg1.f);
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;

    case fpATAN :
      libexec_GetFpArguments(st, fpop, &arg1, NULL);
      result.f = atan(arg1.f);
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;

    case fpLN :
      libexec_GetFpArguments(st, fpop, &arg1, NULL);
      result.f = log(arg1.f);
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;

    case fpEXP :
      libexec_GetFpArguments(st, fpop, &arg1, NULL);
      result.f = exp(arg1.f);
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;

    default :
      return eBADFPOPCODE;

    }

  return eNOERROR;
}
