/****************************************************************************
 * pexec.c
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

#include "pas_debug.h"
#include "pas_machine.h"
#include "pinsn16.h"
#include "pas_fpops.h"
#include "pas_sysio.h"
#include "pas_errcodes.h"

#include "paslib.h"
#include "psysio.h"
#include "plib.h"
#include "pexec.h"

#ifdef CONFIG_HAVE_LIBM
#include <math.h>
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static uint16_t pexec_execfp(struct pexec_s *st, uint8_t fpop);
static void     pexec_getfparguments(struct pexec_s *st, uint8_t fpop,
                                     fparg_t *arg1, fparg_t *arg2);
static ustack_t pexec_getbaseaddress(struct pexec_s *st, level_t leveloffset);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pexec_execfp
 *
 * Description:
 *   This function processes a floating point operation.
 *
 ****************************************************************************/

static uint16_t pexec_execfp(struct pexec_s *st, uint8_t fpop)
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
      pexec_getfparguments(st, fpop, &arg1, NULL);
      intValue = (int16_t)arg1.f;
      PUSH(st, intValue);
      break;

      /* Floating Point arithmetic instructions (Two FP stack arguments) */

    case fpADD :
      pexec_getfparguments(st, fpop, &arg1, &arg2);
      result.f = arg1.f + arg2.f;
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;
    case fpSUB :
      pexec_getfparguments(st, fpop, &arg1, &arg2);
      result.f = arg1.f - arg2.f;
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;
    case fpMUL :
      pexec_getfparguments(st, fpop, &arg1, &arg2);
      result.f = arg1.f * arg2.f;
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;
    case fpDIV :
      pexec_getfparguments(st, fpop, &arg1, &arg2);
      result.f = arg1.f / arg2.f;
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;
    case fpMOD :
      return eBADFPOPCODE;
#if 0 /* Not yet */
      pexec_getfparguments(st, fpop, &arg1, &arg2);
      result.f = arg1.f % arg2.f;
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;
#endif

      /* Floating Point Comparisons (Two FP stack arguments) */

    case fpEQU :
      pexec_getfparguments(st, fpop, &arg1, &arg2);
      intValue = PASCAL_FALSE;
      if (arg1.f == arg2.f)
        intValue = PASCAL_TRUE;
      PUSH(st, intValue);
      break;
    case fpNEQ :
      pexec_getfparguments(st, fpop, &arg1, &arg2);
      intValue = PASCAL_FALSE;
      if (arg1.f != arg2.f)
        intValue = PASCAL_TRUE;
      PUSH(st, intValue);
      break;
    case fpLT :
      pexec_getfparguments(st, fpop, &arg1, &arg2);
      intValue = PASCAL_FALSE;
      if (arg1.f < arg2.f)
        intValue = PASCAL_TRUE;
      PUSH(st, intValue);
      break;
    case fpGTE :
      pexec_getfparguments(st, fpop, &arg1, &arg2);
      intValue = PASCAL_FALSE;
      if (arg1.f >= arg2.f)
        intValue = PASCAL_TRUE;
      PUSH(st, intValue);
      break;
    case fpGT :
      pexec_getfparguments(st, fpop, &arg1, &arg2);
      intValue = PASCAL_FALSE;
      if (arg1.f > arg2.f)
        intValue = PASCAL_TRUE;
      PUSH(st, intValue);
      break;
    case fpLTE :
      pexec_getfparguments(st, fpop, &arg1, &arg2);
      intValue = PASCAL_FALSE;
      if (arg1.f <= arg2.f)
        intValue = PASCAL_TRUE;
      PUSH(st, intValue);
      break;

      /* Floating Point arithmetic instructions (One FP stack arguments) */

    case fpNEG :
      pexec_getfparguments(st, fpop, &arg1, NULL);
      result.f = -arg1.f;
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;
#ifdef CONFIG_HAVE_LIBM
    case fpABS :
      pexec_getfparguments(st, fpop, &arg1, NULL);
      result.f = fabs(arg1.f);
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;
#endif
    case fpSQR :
      pexec_getfparguments(st, fpop, &arg1, NULL);
      result.f = arg1.f * arg1.f;
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;
#ifdef CONFIG_HAVE_LIBM
    case fpSQRT :
      pexec_getfparguments(st, fpop, &arg1, NULL);
      result.f = sqrt(arg1.f);
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;
    case fpSIN :
      pexec_getfparguments(st, fpop, &arg1, NULL);
      result.f = sin(arg1.f);
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;
    case fpCOS :
      pexec_getfparguments(st, fpop, &arg1, NULL);
      result.f = cos(arg1.f);
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;
    case fpATAN :
      pexec_getfparguments(st, fpop, &arg1, NULL);
      result.f = atan(arg1.f);
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;
    case fpLN :
      pexec_getfparguments(st, fpop, &arg1, NULL);
      result.f = log(arg1.f);
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;
    case fpEXP :
      pexec_getfparguments(st, fpop, &arg1, NULL);
      result.f = exp(arg1.f);
      PUSH(st, result.hw[0]);
      PUSH(st, result.hw[1]);
      PUSH(st, result.hw[2]);
      PUSH(st, result.hw[3]);
      break;
#endif

    default :
      return eBADFPOPCODE;

    }

  return eNOERROR;
}

/****************************************************************************
 * Name: pexec_getfparguments
 *
 * Description:
 *   This function retrieves the floating point arguments and performs
 *   integer to REAL conversions as necessary
 *
 ****************************************************************************/

static void pexec_getfparguments(struct pexec_s *st, uint8_t fpop, fparg_t *arg1, fparg_t *arg2)
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
 * Name: pexec_getbaseaddress
 *
 * Description:
 *   This function binds the base address corresponding to a given level
 *   offset.
 *
 ****************************************************************************/

static ustack_t pexec_getbaseaddress(struct pexec_s *st, level_t leveloffset)
{
  /* Start with the base register of the current frame */

  ustack_t baseAddress = st->fp;

  /* Search backware "leveloffset" frames until the correct frame is
   * found
   */

   while (leveloffset > 0)
     {
       baseAddress = st->dstack.i[BTOISTACK(baseAddress)];
       leveloffset--;
     }

   /* Offset that value by two words (one for the st->fp and one for the
    * return value
    */

   return baseAddress + 2 * BPERI;
}

/****************************************************************************
 * Name: pexec8
 *
 * Descripton:
 *   Handle 8-bit instructions with no immediate data
 *
 ****************************************************************************/

static inline int pexec8(FAR struct pexec_s *st, uint8_t opcode)
{
  sstack_t sparm;
  ustack_t uparm1;
  ustack_t uparm2;
  ustack_t uparm3;

  switch (opcode)
    {
      /* Arithmetic & logical & and integer conversions (One stack argument) */
    case oNEG  :
      TOS(st, 0) = (ustack_t)(-(sstack_t)TOS(st, 0));
      break;

    case oABS  :
      if (signExtend16(TOS(st, 0)) < 0)
        {
          TOS(st, 0) = (ustack_t)(-signExtend16(TOS(st, 0)));
        }
      break;

    case oINC  :
      TOS(st, 0)++;
      break;

    case oDEC  :
      TOS(st, 0)--;
      break;

    case oNOT  :
      TOS(st, 0) = ~TOS(st, 0);
      break;

      /* Arithmetic & logical (Two stack arguments) */

    case oADD :
      POP(st, sparm);
      TOS(st, 0) = (ustack_t)(((sstack_t)TOS(st, 0)) + sparm);
      break;

    case oSUB :
      POP(st, sparm);
      TOS(st, 0) = (ustack_t)(((sstack_t)TOS(st, 0)) - sparm);
      break;

    case oMUL :
      POP(st, sparm);
      TOS(st, 0) = (ustack_t)(((sstack_t)TOS(st, 0)) * sparm);
      break;

    case oDIV :
      POP(st, sparm);
      TOS(st, 0) = (ustack_t)(((sstack_t)TOS(st, 0)) / sparm);
      break;

    case oMOD :
      POP(st, sparm);
      TOS(st, 0) = (ustack_t)(((sstack_t)TOS(st, 0)) % sparm);
      break;

    case oSLL :
      POP(st, sparm);
      TOS(st, 0) = (ustack_t)(((sstack_t)TOS(st, 0)) << sparm);
      break;

    case oSRL :
      POP(st, sparm);
      TOS(st, 0) = (TOS(st, 0) >> sparm);
      break;

    case oSRA :
      POP(st, sparm);
      TOS(st, 0) = (ustack_t)(((sstack_t)TOS(st, 0)) >> sparm);
      break;

    case oOR  :
      POP(st, uparm1);
      TOS(st, 0) = (TOS(st, 0) | uparm1);
      break;

    case oAND :
      POP(st, uparm1);
      TOS(st, 0) = (TOS(st, 0) & uparm1);
      break;

    case oBIT :
      POP(st, uparm1);
      uparm2 = TOS(st, 0);
      if ((uparm1 & (1 << uparm2)) != 0)
        {
          TOS(st, 0) = PASCAL_TRUE;
        }
      else
        {
          TOS(st, 0) = PASCAL_FALSE;
        }
      break;

      /* Comparisons (One stack argument) */

     case oEQUZ :
      POP(st, sparm);
      uparm1 = PASCAL_FALSE;
      if (sparm == 0)
        {
          uparm1 = PASCAL_TRUE;
        }

      PUSH(st, uparm1);
      break;

    case oNEQZ :
      POP(st, sparm);
      uparm1 = PASCAL_FALSE;
      if (sparm != 0)
        {
          uparm1 = PASCAL_TRUE;
        }

      PUSH(st, uparm1);
      break;

    case oLTZ  :
      POP(st, sparm);
      uparm1 = PASCAL_FALSE;
      if (sparm < 0)
        {
          uparm1 = PASCAL_TRUE;
        }

      PUSH(st, uparm1);
      break;

    case oGTEZ :
      POP(st, sparm);
      uparm1 = PASCAL_FALSE;
      if (sparm >= 0)
        {
          uparm1 = PASCAL_TRUE;
        }

      PUSH(st, uparm1);
      break;

    case oGTZ  :
      POP(st, sparm);
      uparm1 = PASCAL_FALSE;
      if (sparm > 0)
        {
          uparm1 = PASCAL_TRUE;
        }

      PUSH(st, uparm1);
      break;

    case oLTEZ :
      POP(st, sparm);
      uparm1 = PASCAL_FALSE;
      if (sparm <= 0)
        {
          uparm1 = PASCAL_TRUE;
        }

      PUSH(st, uparm1);
      break;

      /* Comparisons (Two stack arguments) */

    case oEQU  :
      POP(st, sparm);
      uparm1 = PASCAL_FALSE;
      if (sparm == (sstack_t)TOS(st, 0))
        {
          uparm1 = PASCAL_TRUE;
        }

      TOS(st, 0) = uparm1;
      break;

    case oNEQ  :
      POP(st, sparm);
      uparm1 = PASCAL_FALSE;
      if (sparm != (sstack_t)TOS(st, 0))
        {
          uparm1 = PASCAL_TRUE;
        }

      TOS(st, 0) = uparm1;
      break;

    case oLT   :
      POP(st, sparm);
      uparm1 = PASCAL_FALSE;
      if (sparm < (sstack_t)TOS(st, 0))
        {
          uparm1 = PASCAL_TRUE;
        }

      TOS(st, 0) = uparm1;
      break;

    case oGTE  :
      POP(st, sparm);
      uparm1 = PASCAL_FALSE;
      if (sparm >= (sstack_t)TOS(st, 0))
        {
          uparm1 = PASCAL_TRUE;
        }

      TOS(st, 0) = uparm1;
      break;

    case oGT   :
      POP(st, sparm);
      uparm1 = PASCAL_FALSE;
      if (sparm > (sstack_t)TOS(st, 0))
        {
          uparm1 = PASCAL_TRUE;
        }

      TOS(st, 0) = uparm1;
      break;

    case oLTE  :
      POP(st, sparm);
      uparm1 = PASCAL_FALSE;
      if (sparm <= (sstack_t)TOS(st, 0))
        {
          uparm1 = PASCAL_TRUE;
        }

      TOS(st, 0) = uparm1;
      break;

      /* Load (One stack argument) */

    case oLDI  :
      POP(st, uparm1);                   /* Address */
      PUSH(st, GETSTACK(st, uparm1));
      PUSH(st, GETSTACK(st, uparm1 + BPERI));
      break;

    case oLDIH  :
      TOS(st, 0) = GETSTACK(st, TOS(st, 0));
      break;

    case oLDIB :
      TOS(st, 0) = GETBSTACK(st, TOS(st, 0));
      break;

    case oLDIM :
 /* FIX ME --> Need to handle the unaligned case */
      POP(st, uparm1); /* Size */
      POP(st, uparm2); /* Stack offset */
      while (uparm1 > 0)
        {
          if (uparm1 >= BPERI)
            {
              PUSH(st, GETSTACK(st, uparm2));
              uparm2 += BPERI;
              uparm1 -= BPERI;
            }
          else
            {
              PUSH(st, GETBSTACK(st, uparm2));
              uparm2++;
              uparm1--;
            }
        }
      break;

    case oDUP :
      uparm1 = TOS(st, 0);
      uparm2 = TOS(st, 1);
      PUSH(st, uparm2);
      PUSH(st, uparm1);
      break;

    case oDUPH :
      uparm1 = TOS(st, 0);
      PUSH(st, uparm1);
      break;

    case oPUSHS :
      PUSH(st, st->csp);
      break;

    case oPOPS :
      POP(st, st->csp);
      break;

      /* Store (Two stack arguments) */

    case oSTIH  :
      POP(st, uparm1);
      POP(st, uparm2);
      PUTSTACK(st, uparm1,uparm2);
      break;

    case oSTIB :
      POP(st, uparm1);
      POP(st, uparm2);
      PUTBSTACK(st, uparm1, uparm2);
      break;

    case oSTIM :
 /* FIX ME --> Need to handle the unaligned case */
      POP(st, uparm1);                /* Size in bytes */
      uparm3 = uparm1;            /* Save for stack discard */
      sparm = ROUNDBTOI(uparm1); /* Size in words */
      uparm2 = TOS(st, sparm);       /* Stack offset */
      sparm--;
      while (uparm1 > 0)
        {
          if (uparm1 >= BPERI)
            {
              PUTSTACK(st, TOS(st, sparm), uparm2);
              uparm2 += BPERI;
              uparm1 -= BPERI;
              sparm--;
            }
          else
            {
              PUTBSTACK(st, TOS(st, sparm), uparm2);
              uparm2++;
              uparm1--;
            }
        }

      /* Discard the stored data + the stack offset */

      DISCARD(st, (ROUNDBTOI(uparm3) + 1));
      break;

      /* Program control (No stack arguments) */

    case oNOP   :
      break;

    case oRET   :
      POP(st, st->pc);
      POP(st, st->fp);
      DISCARD(st, 1);
      return eNOERROR;

      /* System Functions (No stack arguments) */

    case oEND   :
      return eEXIT;

    default :
      return eILLEGALOPCODE;
    }

  st->pc += 1;
  return eNOERROR;
}

/****************************************************************************
 * Name: pexec16
 *
 * Descripton:
 *   Handle 16-bit instructions with 8-bits of immediate data (imm8)
 *
 ****************************************************************************/

static inline int pexec16(FAR struct pexec_s *st, uint8_t opcode, uint8_t imm8)
{
  int ret = eNOERROR;

  st->pc += 2;
  switch (opcode)
    {
      /* Data stack:  imm8 = 8 bit unsigned data (no stack arguments) */

    case oPUSHB  :
      PUSH(st, imm8);
      break;

      /* Floating Point:  imm8 = FP op-code (varying number of stack arguments) */
    case oFLOAT  :
      ret = pexec_execfp(st, imm8);
      break;

    default :
      ret = eILLEGALOPCODE;
      break;
    }
  return ret;
}

/****************************************************************************
 * Name: pexec24
 *
 * Descripton:
 *   Handle 24-bit instructions with 16-bits of immediate data (imm16)
 *
 ****************************************************************************/

static inline int pexec24(FAR struct pexec_s *st, uint8_t opcode, uint16_t imm16)
{
  sstack_t sparm1;
  sstack_t sparm2;
  ustack_t uparm1;
  ustack_t uparm2;
  ustack_t uparm3;
  int ret = eNOERROR;

  switch (opcode)
    {
      /* Program control:  imm16 = unsigned label (no stack arguments) */

    case oJMP   :
      goto branch_out;

      /* Program control:  imm16 = unsigned label (One stack argument) */

    case oJEQUZ :
      POP(st, sparm1);
      if (sparm1 == 0)
        {
          goto branch_out;
        }
      break;

    case oJNEQZ :
      POP(st, sparm1);
      if (sparm1 != 0)
        {
          goto branch_out;
        }
      break;

    case oJLTZ  :
      POP(st, sparm1);
      if (sparm1 < 0)
        {
          goto branch_out;
        }
      break;

    case oJGTEZ :
      POP(st, sparm1);
      if (sparm1 >= 0)
        {
          goto branch_out;
        }
      break;

    case oJGTZ  :
      POP(st, sparm1);
      if (sparm1 > 0)
        {
          goto branch_out;
        }
      break;

    case oJLTEZ :
      POP(st, sparm1);
      if (sparm1 <= 0)
        {
          goto branch_out;
        }
      break;

      /* Program control:  imm16 = unsigned label (Two stack arguments) */

    case oJEQU :
      POP(st, sparm1);
      POP(st, sparm2);
      if (sparm2 == sparm1)
        {
          goto branch_out;
        }
      break;

    case oJNEQ :
      POP(st, sparm1);
      POP(st, sparm2);
      if (sparm2 != sparm1)
        {
          goto branch_out;
        }
      break;

    case oJLT  :
      POP(st, sparm1);
      POP(st, sparm2);
      if (sparm2 < sparm1)
        {
          goto branch_out;
        }
      break;

    case oJGTE :
      POP(st, sparm1);
      POP(st, sparm2);
      if (sparm2 >= sparm1)
        {
          goto branch_out;
        }
      break;

    case oJGT  :
      POP(st, sparm1);
      POP(st, sparm2);
      if (sparm2 > sparm1)
        {
          goto branch_out;
        }
      break;

    case oJLTE :
      POP(st, sparm1);
      POP(st, sparm2);
      if (sparm2 <= sparm1)
        {
          goto branch_out;
        }
      break;

      /* Load:  imm16 = usigned offset (no stack arguments) */

    case oLD :
      uparm1 = st->spb + imm16;
      PUSH(st, GETSTACK(st, uparm1));
      PUSH(st, GETSTACK(st, uparm1 + BPERI));
      break;

    case oLDH :
      uparm1 = st->spb + imm16;
      PUSH(st, GETSTACK(st, uparm1));
      break;

    case oLDB :
      uparm1 = st->spb + imm16;
      PUSH(st, GETBSTACK(st, uparm1));
      break;

    case oLDM :
 /* FIX ME --> Need to handle the unaligned case */
      POP(st, uparm1);
      uparm2 = st->spb + imm16;
      while (uparm1 > 0)
        {
          if (uparm1 >= BPERI)
            {
              PUSH(st, GETSTACK(st, uparm2));
              uparm2 += BPERI;
              uparm1 -= BPERI;
            }
          else
            {
              PUSH(st, GETBSTACK(st, uparm2));
              uparm2++;
              uparm1--;
            }
        }
      break;

      /* Load & store: imm16 = unsigned base offset (One stack argument) */

    case oST :
      uparm1 = st->spb + imm16;
      POP(st, uparm2);
      PUTSTACK(st, uparm2, uparm1 + BPERI);
      POP(st, uparm2);
      PUTSTACK(st, uparm2, uparm1);
      break;

    case oSTH   :
      uparm1  = st->spb + imm16;
      POP(st, uparm2);
      PUTSTACK(st, uparm2, uparm1);
      break;

    case oSTB  :
      uparm1  = st->spb + imm16;
      POP(st, uparm2);
      PUTBSTACK(st, uparm2, uparm1);
      break;

    case oSTM :
 /* FIX ME --> Need to handle the unaligned case */
      POP(st, uparm1);                /* Size */
      uparm3 = uparm1;            /* Save for stack discard */
      uparm2 = st->spb + imm16;
      sparm1 = ROUNDBTOI(uparm1) - 1;
      while (uparm1 > 0)
        {
          if (uparm1 >= BPERI)
            {
              PUTSTACK(st, TOS(st, sparm1), uparm2);
              uparm2 += BPERI;
              uparm1 -= BPERI;
              sparm1--;
            }
          else
            {
              PUTBSTACK(st, TOS(st, sparm1), uparm2);
              uparm2++;
              uparm1--;
            }
        }

      /* Discard the stored data */

      DISCARD(st, ROUNDBTOI(uparm3));
      break;

    case oLDX  :
      uparm1 = st->spb + imm16 + TOS(st, 0);
      TOS(st, 0) = GETSTACK(st, uparm1);
      PUSH(st, GETSTACK(st, uparm1 + BPERI));
      break;

    case oLDXH  :
      uparm1 = st->spb + imm16 + TOS(st, 0);
      TOS(st, 0) = GETSTACK(st, uparm1);
      break;

    case oLDXB :
      uparm1 = st->spb + imm16 + TOS(st, 0);
      TOS(st, 0) = GETBSTACK(st, uparm1);
      break;

    case oLDXM  :
 /* FIX ME --> Need to handle the unaligned case */
      POP(st, uparm1);
      POP(st, uparm2);
      uparm2 += st->spb + imm16;
      while (uparm1 > 0)
        {
          if (uparm1 >= BPERI)
            {
              PUSH(st, GETSTACK(st, uparm2));
              uparm2 += BPERI;
              uparm1 -= BPERI;
            }
          else
            {
              PUSH(st, GETBSTACK(st, uparm2));
              uparm2++;
              uparm1--;
            }
        }
      break;

      /* Store: imm16 = unsigned base offset (Two stack arguments) */

    case oSTXH  :
      POP(st, uparm1);
      POP(st, uparm2);
      uparm2 += st->spb + imm16;
      PUTSTACK(st, uparm1,uparm2);
      break;

    case oSTXB :
      POP(st, uparm1);
      POP(st, uparm2);
      uparm2 += st->spb + imm16;
      PUTBSTACK(st, uparm1, uparm2);
      break;

    case oSTXM :
/* FIX ME --> Need to handle the unaligned case */
      POP(st, uparm1);                /* Size */
      uparm3 = uparm1;            /* Save for stack discard */
      sparm1 = ROUNDBTOI(uparm1); /* Size in 16-bit words */
      uparm2 = TOS(st, sparm1);       /* index */
      sparm1--;
      uparm2 += st->spb + imm16;
      while (uparm1 > 0)
        {
          if (uparm1 >= BPERI)
            {
              PUTSTACK(st, TOS(st, sparm1), uparm2);
              uparm2 += BPERI;
              uparm1 -= BPERI;
              sparm1--;
            }
          else
            {
              PUTBSTACK(st, TOS(st, sparm1), uparm2);
              uparm2++;
              uparm1--;
            }
        }

      /* Discard the stored data + the index */

      DISCARD(st, (ROUNDBTOI(uparm3) + 1));
      break;

    case oLA  :
      uparm1 = st->spb + imm16;
      PUSH(st, uparm1);
      break;

    case oLAX :
      TOS(st, 0) = st->spb + imm16 + TOS(st, 0);
      break;

      /* Data stack:  imm16 = 16 bit signed data (no stack arguments) */

    case oPUSH  :
      PUSH(st, imm16);
      break;

    case oINDS  :
      st->sp += signExtend16(imm16);
      break;

      /* System Functions:
       * For LIB:        imm16 = sub-function code
       */

    case oLIB  :
      ret = pexec_libcall(st, imm16);
      break;

      /* System Functions:
       * For SYSIO:   imm16 = sub-function code
       *              File number on stack
       */

    case oSYSIO  :
      ret = pexec_sysio(st, imm16);
      break;

      /* Program control:  imm16 = unsigned label (no stack arguments) */

    case oLAC :
      uparm1 = imm16 + st->rop;
      PUSH(st, uparm1);
      break;

    case oLABEL :
    default:
      ret = eILLEGALOPCODE;
      break;
    }

  st->pc += 3;
  return ret;

branch_out:
  st->pc = (paddr_t)imm16;
  return ret;
}

/****************************************************************************
 * Name: pexec32
 *
 * Descripton:
 *   Handle 32-bit instructions with 24-bits of immediate data (imm8+imm16)
 *
 ****************************************************************************/

static int pexec32(FAR struct pexec_s *st, uint8_t opcode, uint8_t imm8, uint16_t imm16)
{
  sstack_t sparm;
  ustack_t uparm1;
  ustack_t uparm2;
  ustack_t uparm3;
  int ret = eNOERROR;

  switch (opcode)
    {
      /* Load:  imm8 = level; imm16 = signed frame offset (no stack arguments) */
    case oLDS :
      uparm1 = pexec_getbaseaddress(st, imm8) + signExtend16(imm16);
      PUSH(st, GETSTACK(st, uparm1));
      PUSH(st, GETSTACK(st, uparm1 + BPERI));
      break;

    case oLDSH :
      uparm1 = pexec_getbaseaddress(st, imm8) + signExtend16(imm16);
      PUSH(st, GETSTACK(st, uparm1));
      break;

    case oLDSB :
      uparm1 = pexec_getbaseaddress(st, imm8) + signExtend16(imm16);
      PUSH(st, GETBSTACK(st, uparm1));
      break;

    case oLDSM :
 /* FIX ME --> Need to handle the unaligned case */
      POP(st, uparm1);
      uparm2 = pexec_getbaseaddress(st, imm8) + signExtend16(imm16);
      while (uparm1 > 0)
        {
          if (uparm1 >= BPERI)
            {
              PUSH(st, GETSTACK(st, uparm2));
              uparm2 += BPERI;
              uparm1 -= BPERI;
            }
          else
            {
              PUSH(st, GETBSTACK(st, uparm2));
              uparm2++;
              uparm1--;
            }
        }
      break;

      /* Load & store: imm8 = level; imm16 = signed frame offset (One stack argument) */

    case oSTSH   :
      uparm1  = pexec_getbaseaddress(st, imm8) + signExtend16(imm16);
      POP(st, uparm2);
      PUTSTACK(st, uparm2, uparm1);
      break;

    case oSTSB  :
      uparm1  = pexec_getbaseaddress(st, imm8) + signExtend16(imm16);
      POP(st, uparm2);
      PUTBSTACK(st, uparm2, uparm1);
      break;

    case oSTSM :
 /* FIX ME --> Need to handle the unaligned case */
      POP(st, uparm1);            /* Size */
      uparm3 = uparm1;            /* Save for stack discard */
      uparm2 = pexec_getbaseaddress(st, imm8) + signExtend16(imm16);
      sparm = ROUNDBTOI(uparm1) - 1;
      while (uparm1 > 0)
        {
          if (uparm1 >= BPERI)
            {
              PUTSTACK(st, TOS(st, sparm), uparm2);
              uparm2 += BPERI;
              uparm1 -= BPERI;
              sparm--;
            }
          else
            {
              PUTBSTACK(st, TOS(st, sparm), uparm2);
              uparm2++;
              uparm1--;
            }
        }

      /* Discard the stored data */

      DISCARD(st, ROUNDBTOI(uparm3));
      break;

    case oLDSX  :
      uparm1 = pexec_getbaseaddress(st, imm8) + signExtend16(imm16) + TOS(st, 0);
      TOS(st, 0) = GETSTACK(st, uparm1);
      PUSH(st, GETSTACK(st, uparm1 + BPERI));
      break;

    case oLDSXH  :
      uparm1 = pexec_getbaseaddress(st, imm8) + signExtend16(imm16) + TOS(st, 0);
      TOS(st, 0) = GETSTACK(st, uparm1);
      break;

    case oLDSXB :
      uparm1 = pexec_getbaseaddress(st, imm8) + signExtend16(imm16) + TOS(st, 0);
      TOS(st, 0) = GETBSTACK(st, uparm1);
      break;

    case oLDSXM  :
 /* FIX ME --> Need to handle the unaligned case */
      POP(st, uparm1);
      POP(st, uparm2);
      uparm2 += pexec_getbaseaddress(st, imm8) + signExtend16(imm16);
      while (uparm1 > 0)
        {
          if (uparm1 >= BPERI)
            {
              PUSH(st, GETSTACK(st, uparm2));
              uparm2 += BPERI;
              uparm1 -= BPERI;
            }
          else
            {
              PUSH(st, GETBSTACK(st, uparm2));
              uparm2++;
              uparm1--;
            }
        }
      break;

      /* Store: imm8 = level; imm16 = signed frame offset (Two stack arguments) */

    case oSTSXH  :
      POP(st, uparm1);
      POP(st, uparm2);
      uparm2 += pexec_getbaseaddress(st, imm8) + signExtend16(imm16);
      PUTSTACK(st, uparm1,uparm2);
      break;

    case oSTSXB :
      POP(st, uparm1);
      POP(st, uparm2);
      uparm2 += pexec_getbaseaddress(st, imm8) + signExtend16(imm16);
      PUTBSTACK(st, uparm1, uparm2);
      break;

    case oSTSXM :
/* FIX ME --> Need to handle the unaligned case */
      POP(st, uparm1);                /* Size */
      uparm3 = uparm1;            /* Save for stack discard */
      sparm = ROUNDBTOI(uparm1); /* Size in 16-bit words */
      uparm2 = TOS(st, sparm);       /* index */
      sparm--;
      uparm2 += pexec_getbaseaddress(st, imm8) + signExtend16(imm16);
      while (uparm1 > 0)
        {
          if (uparm1 >= BPERI)
            {
              PUTSTACK(st, TOS(st, sparm), uparm2);
              uparm2 += BPERI;
              uparm1 -= BPERI;
              sparm--;
            }
          else
            {
              PUTBSTACK(st, TOS(st, sparm), uparm2);
              uparm2++;
              uparm1--;
            }
        }

      /* Discard the stored data + the index */

      DISCARD(st, (ROUNDBTOI(uparm3) + 1));
      break;

    case oLAS  :
      uparm1 = pexec_getbaseaddress(st, imm8) + signExtend16(imm16);
      PUSH(st, uparm1);
      break;

    case oLASX :
      TOS(st, 0) = pexec_getbaseaddress(st, imm8) + signExtend16(imm16) + TOS(st, 0);
      break;

      /* Program Control:  imm8 = level; imm16 = unsigned label (No
       * stack arguments)
       */

    case oPCAL  :
      PUSH(st, pexec_getbaseaddress(st, imm8));
      PUSH(st, st->fp);
      uparm1 = st->sp;
      PUSH(st, st->pc + 4);
      st->fp = uparm1;
      st->pc = (paddr_t)imm16;
      return eNOERROR;

      /* Pseudo-operations:  (No stack arguments)
       * For LINE:    imm8 = file number; imm16 = line number
       */

    case oLINE   :
    default :
      ret = eILLEGALOPCODE;
      break;
    }

  st->pc += 4;
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pexec_init
 ****************************************************************************/

FAR struct pexec_s *pexec_Initialize(struct pexec_attr_s *attr)
{
  struct pexec_s *st;
  paddr_t stacksize;
  paddr_t adjusted_rosize;

  /* Allocate the p-machine state stucture */

  st = (struct pexec_s *)malloc(sizeof(struct pexec_s));
  if (!st)
    {
      return NULL;
    }

  /* Set up I-Space */

  st->ispace = attr->ispace;
  st->maxpc  = attr->maxpc;

  /* Align size of read-only data to 16-bit boundary. */

  adjusted_rosize = (attr->rosize + 1) & ~1;

  /* Allocate the pascal stack.  Organization is string stack, then
   * constant data, then "normal" pascal stack.
   */

  stacksize    = attr->varsize + adjusted_rosize + attr->strsize;
  st->dstack.b = (uint8_t *)malloc(stacksize);
  if (!st->dstack.b)
    {
      free(st);
      return NULL;
    }

  /* Copy the rodata into the stack */

  if (attr->rodata && attr->rosize)
    {
      memcpy(&st->dstack.b[attr->strsize], attr->rodata, attr->rosize);
    }

  /* Set up info needed to perform a simulated reset */

  st->stralloc  = attr->stralloc;
  st->strsize   = attr->strsize;
  st->rosize    = adjusted_rosize;
  st->entry     = attr->entry;
  st->stacksize = stacksize;

  /* Then perform a simulated reset */

  pexec_Reset(st);
  return st;
}

/****************************************************************************
 * Name: pexec
 ****************************************************************************/

int pexec_Execute(FAR struct pexec_s *st)
{
  uint8_t opcode;
  int ret;

  /* Make sure that the program counter is within range */

  if (st->pc >= st->maxpc)
    {
      ret = eBADPC;
    }
  else
    {
      /* Get the instruction to execute */

      opcode = st->ispace[st->pc];
      if ((opcode & o8) != 0)
        {
          /* Get the immediate, 8-bit value */

          uint8_t imm8 = st->ispace[st->pc + 1];
          if ((opcode & o16) != 0)
            {
              /* Get the immediate, big-endian 16-bit value */

              uint16_t imm16  = ((st->ispace[st->pc + 2]) << 8) | st->ispace[st->pc + 3];

              /* Handle 32 bit instructions */

              ret = pexec32(st, opcode, imm8, imm16);
            }
          else
            {
              /* Handle 16-bit instructions */

              ret = pexec16(st, opcode, imm8);
            }
        }
      else if ((opcode & o16) != 0)
        {
          /* Get the immediate, big-endian 16-bit value */

          uint16_t imm16  = ((st->ispace[st->pc + 1]) << 8) | st->ispace[st->pc + 2];

          /* Handle 24-bit instructions */

          ret = pexec24(st, opcode, imm16);
        }
      else
        {
          /* Handle 8-bit instructions */

          ret = pexec8(st, opcode);
        }
    }

  return ret;
}

/****************************************************************************
 * Name: pexec_reset
 ****************************************************************************/

void pexec_Reset(struct pexec_s *st)
{
  int dndx;

  /* Setup the bottom of the "normal" pascal stack */

  st->rop   = st->strsize;
  st->spb   = st->strsize + st->rosize;

  /* Initialize the emulated P-Machine registers */

  st->csp   = 0;
  st->sp    = st->spb + 2 * BPERI;
  st->fp    = st->spb + BPERI;
  st->pc    = st->entry;

  /* Initialize the P-Machine stack */

  dndx                 = BTOISTACK(st->spb);
  st->dstack.i[dndx]   =  0;
  st->dstack.i[dndx+1] =  0;
  st->dstack.i[dndx+2] = -1;

  /* [Re]-initialize the file I/O logic */

  pexec_InitializeFile();
}

/****************************************************************************
 * Name: pexec_release
 ****************************************************************************/

void pexec_Release(struct pexec_s *st)
{
  if (st)
    {
      if (st->dstack.i)
        {
          free(st->dstack.i);
        }

      if (st->ispace)
        {
          free(st->ispace);
        }

      free(st);
    }
}
