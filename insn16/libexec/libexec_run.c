/****************************************************************************
 * libexec_run.c
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

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <ctype.h>
#include <math.h>

#include "paslib.h"
#include "execlib.h"

#include "pas_debug.h"
#include "pas_machine.h"
#include "insn16.h"
#include "longops.h"
#include "pas_fpops.h"
#include "pas_sysio.h"
#include "pas_errcodes.h"

#include "libexec_float.h"
#include "libexec_sysio.h"
#include "libexec_setops.h"
#include "libexec_longops.h"
#include "libexec_stringlib.h"
#include "libexec_oslib.h"
#include "libexec_heap.h"
#include "libexec.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Size of frame info at the beginning of each frame:
 *
 *        |  Base Address  | + 4 * BPERI
 *        +----------------+
 *        |  Nesting Level | + 3 * BPERI
 *        +----------------+
 *        | Return Address | + 2 * BPERI
 *        +----------------+
 *        |  Dynamic Link  | + BPERI
 *        +----------------+
 *  FP -> |  Static Link   | 0
 *        +----------------+
 */

/* Offsets relative to the frame pointer */

#define _FSLINK (0)
#define _FDLINK (BPERI)
#define _FRET   (2 * BPERI)
#define _FLEVEL (3 * BPERI)

#define _FBASE  (4 * BPERI)
#define _FSIZE  (4 * BPERI)

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int      libexec_ProcedureCall(struct libexec_s *st, level_t nestingLevel);
static ustack_t libexec_GetBaseAddress(struct libexec_s *st, level_t levelOffset,
                                       int32_t stackOffset);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: libexec_ProcedureCall
 *
 * Description:
 *   This function builds a new frame at the top of the stack as part of the
 *   procedure call logic.
 *
 ****************************************************************************/

static int libexec_ProcedureCall(struct libexec_s *st, level_t nestingLevel)
{
  uint16_t *current;
  uint16_t *previous;
  uint16_t findLevel;
  uint16_t frameAddr;
  uint16_t prevLevel;
  uint16_t newFP;

  /* The nesting level should be some value greater than zero */

  if (nestingLevel == 0)
    {
      return eNESTINGLEVEL;
    }

  /* We need to find the preceding nesting level */

  findLevel = nestingLevel - 1;

  /* Search back through the frames to find the correct frame for this static
   * nesting level.  Normally this will be the previous frame, but we need
   * to allow for support of recursion which forces us to search further back.
   */

  /* At this pointer st->fp refers to the calling frame. */

  frameAddr = st->fp - _FSLINK;

  for (; ; )
    {
      previous  = &st->dstack.i[BTOISTACK(frameAddr)];
      prevLevel = previous[BTOISTACK(_FLEVEL)] & 0xff;

      /* Protection against garbage (Stack corruption?) */

      if (prevLevel > UINT8_MAX)
        {
          return eNESTINGLEVEL;
        }

      /* It would be an error if we went all the way to level 0 and never
       * found the frame we are looking for.
       */

      else if (findLevel != 0 && prevLevel == 0)
        {
          return eNESTINGLEVEL;
        }

      /* Set up for the next time through the loop */

      if (findLevel == prevLevel)
        {
          break;
        }

      frameAddr = previous[BTOISTACK(_FSLINK)] - _FSLINK;
    }

  /* Set up the new FRAME info.
   *
   *        |  Base Address  | + 4 * BPERI
   *        +----------------+
   *   lsp  |  Nesting Level | + 3 * BPERI
   *        +----------------+
   *        | Return Address | + 2 * BPERI
   *        +----------------+
   *        |  Dynamic Link  | + BPERI
   *        +----------------+
   *  FP -> |  Static Link   | 0
   *        +----------------+
   *  SP -> |  Caller TOS    |
   */

  st->sp                      += BPERI;
  current                      = &st->dstack.i[BTOISTACK(st->sp)];
  newFP                        = st->sp + _FSLINK;
  st->sp                      += _FSIZE - BPERI;

  current[BTOISTACK(_FSLINK)]  = frameAddr;
  current[BTOISTACK(_FDLINK)]  = st->fp;
  current[BTOISTACK(_FRET)]    = st->pc + 4;
  current[BTOISTACK(_FLEVEL)]  = st->lsp << 8 | nestingLevel;

  st->lsp                      = nestingLevel;
  st->fp                       = newFP;
  return eNOERROR;
}

/****************************************************************************
 * Name: libexec_GetBaseAddress
 *
 * Description:
 *   This function binds the base address corresponding to a given level
 *   offset.  This establishes a static link that is used to access data
 *   in outer layers.
 *
 *   The static link is set on each procedure call.  It is accessed on load
 *   and store instructions as an offset from the current static nesting
 *   level.
 *
 ****************************************************************************/

static ustack_t libexec_GetBaseAddress(struct libexec_s *st, level_t leveloffset,
                                       int32_t stackOffset)
 {
   /* Start with the base register of the current frame */

  ustack_t frameBase = st->fp;

  /* Search backware "leveloffset" frames until the correct frame is
   * found
   */

   while (leveloffset > 0)
     {
       frameBase = st->dstack.i[BTOISTACK(frameBase)];
       leveloffset--;
     }

   /* Offset that value to get the address of the stack region of interest.
    * There are two disjoint regions:
    *
    *   1. At offset _FBASE 'above' the frame info.  Positive variable
    *      offsets lie in this region.
    *   2. 'Below" the frame is a return value areg of size sRETURN_SIZE
    *      and then actual parameter values are below this.  Negative
    *      stack offsets refer to this region.
    */

   frameBase += stackOffset;
   if (stackOffset >= 0)
     {
       frameBase += _FBASE;
     }

   return frameBase;
}

/****************************************************************************
 * Name: pexec8
 *
 * Descripton:
 *   Handle 8-bit instructions with no immediate data
 *
 ****************************************************************************/

static inline int pexec8(struct libexec_s *st, uint8_t opcode)
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

    case oUMUL :
      POP(st, uparm1);
      TOS(st, 0) = ((ustack_t)TOS(st, 0)) * uparm1;
      break;

    case oDIV :
      POP(st, sparm);
      TOS(st, 0) = (ustack_t)(((sstack_t)TOS(st, 0)) / sparm);
      break;

    case oUDIV :
      POP(st, uparm1);
      TOS(st, 0) = ((ustack_t)TOS(st, 0)) / uparm1;
      break;

    case oMOD :
      POP(st, sparm);
      TOS(st, 0) = (ustack_t)(((sstack_t)TOS(st, 0)) % sparm);
      break;

    case oUMOD :
      POP(st, uparm1);
      TOS(st, 0) = ((ustack_t)TOS(st, 0)) % uparm1;
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

    case oXOR :
      POP(st, uparm1);
      TOS(st, 0) = (TOS(st, 0) ^ uparm1);
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
      if (sparm > (sstack_t)TOS(st, 0))
        {
          uparm1 = PASCAL_TRUE;
        }

      TOS(st, 0) = uparm1;
      break;

    case oGTE  :
      POP(st, sparm);
      uparm1 = PASCAL_FALSE;
      if (sparm <= (sstack_t)TOS(st, 0))
        {
          uparm1 = PASCAL_TRUE;
        }

      TOS(st, 0) = uparm1;
      break;

    case oGT   :
      POP(st, sparm);
      uparm1 = PASCAL_FALSE;
      if (sparm < (sstack_t)TOS(st, 0))
        {
          uparm1 = PASCAL_TRUE;
        }

      TOS(st, 0) = uparm1;
      break;

    case oLTE  :
      POP(st, sparm);
      uparm1 = PASCAL_FALSE;
      if (sparm >= (sstack_t)TOS(st, 0))
        {
          uparm1 = PASCAL_TRUE;
        }

      TOS(st, 0) = uparm1;
      break;

    case oULT   :
      POP(st, uparm1);
      uparm2 = PASCAL_FALSE;
      if (uparm1 > (ustack_t)TOS(st, 0))
        {
          uparm2 = PASCAL_TRUE;
        }

      TOS(st, 0) = uparm2;
      break;

    case oUGTE  :
      POP(st, uparm1);
      uparm2 = PASCAL_FALSE;
      if (uparm1 <= (ustack_t)TOS(st, 0))
        {
          uparm2 = PASCAL_TRUE;
        }

      TOS(st, 0) = uparm2;
      break;

    case oUGT   :
      POP(st, uparm1);
      uparm2 = PASCAL_FALSE;
      if (uparm1 < (ustack_t)TOS(st, 0))
        {
          uparm2 = PASCAL_TRUE;
        }

      TOS(st, 0) = uparm2;
      break;

    case oULTE  :
      POP(st, uparm1);
      uparm2 = PASCAL_FALSE;
      if (uparm1 >= (ustack_t)TOS(st, 0))
        {
          uparm2 = PASCAL_TRUE;
        }

      TOS(st, 0) = uparm2;
      break;

      /* Load (One stack argument) */

    case oLDI  :
      TOS(st, 0) = GETSTACK(st, TOS(st, 0));
      break;

    case oLDIB :
      uparm1     = GETBSTACK(st, TOS(st, 0));
      sparm      = signExtend8(uparm1);
      TOS(st, 0) = (ustack_t)sparm;
      break;

    case oULDIB :
      TOS(st, 0) = GETBSTACK(st, TOS(st, 0));
      break;

    case oLDIM :
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
      PUSH(st, uparm1);
      break;

    case oXCHG :
      uparm1 = TOS(st, 0);
      uparm2 = TOS(st, 1);
      TOS(st, 0) = uparm2; /* Swap 16-bit values */
      TOS(st, 1) = uparm1;
      break;

    case oPUSHS :
      PUSH(st, st->csp);
      break;

    case oPOPS :
      POP(st, st->csp);
      break;

      /* Store (Two stack arguments) */

    case oSTI  :
      POP(st, uparm1);
      POP(st, uparm2);
      PUTSTACK(st, uparm1, uparm2);
      break;

    case oSTIB :
      POP(st, uparm1);
      POP(st, uparm2);
      PUTBSTACK(st, uparm1, uparm2);
      break;

    case oSTIM :
      POP(st, uparm1);               /* Size in bytes */
      uparm3 = uparm1;               /* Save for stack discard */
      sparm = ROUNDBTOI(uparm1);     /* Size in words */
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
      /*
       *        +----------------+
       * TOS -> |  Nesting Level | + 3 * BPERI
       *        +----------------+
       *        | Return Address | + 2 * BPERI
       *        +----------------+
       *        |  Dynamic Link  | + BPERI
       *        +----------------+
       *  FP -> |  Static Link   | 0
       *        +----------------+
       *        |   Caller TOS   |
       */

      POP(st, uparm1);        /* Restore the nesting level in the LSP */
      st->lsp = uparm1 >> 8;

      POP(st, st->pc);        /* Set the PC to the return address */
      POP(st, st->fp);        /* Set the FP back to the dynamic link */
      DISCARD(st, 1);         /* Discard the static link */
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

static inline int pexec16(struct libexec_s *st, uint8_t opcode, uint8_t imm8)
{
  int ret = eNOERROR;

  st->pc += 2;
  switch (opcode)
    {
      /* Data stack:  imm8 = 8 bit  data (no stack arguments) */

    case oPUSHB  :
      PUSH(st, signExtend8(imm8));
      break;

    case oUPUSHB  :
      PUSH(st, imm8);
      break;

      /* Floating Point:  imm8 = FP op-code (varying number of stack arguments) */

    case oFLOAT  :
      ret = libexec_FloatOps(st, imm8);
      break;

      /* Set operations:  imm8 = SET op-code (varying number of stack arguments) */

    case oSETOP :
      ret = libexec_SetOperations(st, imm8);
      break;

    case oOSOP :
      ret = libexec_OsOperations(st, imm8);
      break;

    case oLONGOP8 :
      ret = libexec_LongOperation8(st, (enum longOp8_e)imm8);
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

static inline int pexec24(struct libexec_s *st, uint8_t opcode,
                          uint16_t imm16)
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
      goto branchOut;

      /* Program control:  imm16 = unsigned label (One stack argument) */

    case oJEQUZ :
      POP(st, sparm1);
      if (sparm1 == 0)
        {
          goto branchOut;
        }
      break;

    case oJNEQZ :
      POP(st, sparm1);
      if (sparm1 != 0)
        {
          goto branchOut;
        }
      break;

    case oJLTZ  :
      POP(st, sparm1);
      if (sparm1 < 0)
        {
          goto branchOut;
        }
      break;

    case oJGTEZ :
      POP(st, sparm1);
      if (sparm1 >= 0)
        {
          goto branchOut;
        }
      break;

    case oJGTZ  :
      POP(st, sparm1);
      if (sparm1 > 0)
        {
          goto branchOut;
        }
      break;

    case oJLTEZ :
      POP(st, sparm1);
      if (sparm1 <= 0)
        {
          goto branchOut;
        }
      break;

      /* Program control:  imm16 = unsigned label (Two stack arguments) */

    case oJEQU :
      POP(st, sparm1);
      POP(st, sparm2);
      if (sparm2 == sparm1)
        {
          goto branchOut;
        }
      break;

    case oJNEQ :
      POP(st, sparm1);
      POP(st, sparm2);
      if (sparm2 != sparm1)
        {
          goto branchOut;
        }
      break;

    case oJLT  :
      POP(st, sparm1);
      POP(st, sparm2);
      if (sparm2 < sparm1)
        {
          goto branchOut;
        }
      break;

    case oJGTE :
      POP(st, sparm1);
      POP(st, sparm2);
      if (sparm2 >= sparm1)
        {
          goto branchOut;
        }
      break;

    case oJGT  :
      POP(st, sparm1);
      POP(st, sparm2);
      if (sparm2 > sparm1)
        {
          goto branchOut;
        }
      break;

    case oJLTE :
      POP(st, sparm1);
      POP(st, sparm2);
      if (sparm2 <= sparm1)
        {
          goto branchOut;
        }
      break;

    case oJULT  :
      POP(st, uparm1);
      POP(st, uparm2);
      if (uparm2 < uparm1)
        {
          goto branchOut;
        }
      break;

    case oJUGTE :
      POP(st, uparm1);
      POP(st, uparm2);
      if (uparm2 >= uparm1)
        {
          goto branchOut;
        }
      break;

    case oJUGT  :
      POP(st, uparm1);
      POP(st, uparm2);
      if (uparm2 > uparm1)
        {
          goto branchOut;
        }
      break;

    case oJULTE :
      POP(st, uparm1);
      POP(st, uparm2);
      if (uparm2 <= uparm1)
        {
          goto branchOut;
        }
      break;

      /* Load:  imm16 = usigned offset (no stack arguments) */

    case oLD :
      uparm1 = st->spb + imm16;
      PUSH(st, GETSTACK(st, uparm1));
      break;

    case oLDB :
      uparm1 = st->spb + imm16;
      uparm1 = GETBSTACK(st, uparm1);
      sparm1 = signExtend8(uparm1);
      PUSH(st, (ustack_t)sparm1);
      break;

    case oULDB :
      uparm1 = st->spb + imm16;
      PUSH(st, GETBSTACK(st, uparm1));
      break;

    case oLDM :
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
      POP(st, uparm1);            /* Size */
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
      break;

    case oLDXB :
      uparm1     = st->spb + imm16 + TOS(st, 0);
      uparm1     = GETBSTACK(st, uparm1);
      sparm1     = signExtend8(uparm1);
      TOS(st, 0) = (ustack_t)sparm1;
      break;

    case oULDXB :
      uparm1     = st->spb + imm16 + TOS(st, 0);
      TOS(st, 0) = GETBSTACK(st, uparm1);
      break;

    case oLDXM  :
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

    case oSTX  :
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
      POP(st, uparm1);                /* Size */
      uparm3 = uparm1;                /* Save for stack discard */
      sparm1 = ROUNDBTOI(uparm1);     /* Size in 16-bit words */
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

    case oSTRLIB :
      ret = libexec_StringOperations(st, imm16);
      break;

      /* System Functions:
       * For SYSIO:   imm16 = sub-function code
       *              File number on stack
       */

    case oSYSIO :
      ret = libexec_sysio(st, imm16);
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

branchOut:
  st->pc = (pasSize_t)imm16;
  return ret;
}

/****************************************************************************
 * Name: pexec32
 *
 * Descripton:
 *   Handle 32-bit instructions with 24-bits of immediate data (imm8+imm16)
 *
 ****************************************************************************/

static int pexec32(struct libexec_s *st, uint8_t opcode, uint8_t imm8,
                   uint16_t imm16)
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
      uparm1 = libexec_GetBaseAddress(st, imm8, signExtend16(imm16));
      PUSH(st, GETSTACK(st, uparm1));
      break;

    case oLDSB :
      uparm1 = libexec_GetBaseAddress(st, imm8, signExtend16(imm16));
      uparm1 = GETBSTACK(st, uparm1);
      sparm  = signExtend8(uparm1);
      PUSH(st, (ustack_t)sparm);
      break;

    case oULDSB :
      uparm1 = libexec_GetBaseAddress(st, imm8, signExtend16(imm16));
      PUSH(st, GETBSTACK(st, uparm1));
      break;

    case oLDSM :
      POP(st, uparm1);
      uparm2 = libexec_GetBaseAddress(st, imm8, signExtend16(imm16));
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

    case oSTS   :
      uparm1  = libexec_GetBaseAddress(st, imm8, signExtend16(imm16));
      POP(st, uparm2);
      PUTSTACK(st, uparm2, uparm1);
      break;

    case oSTSB  :
      uparm1  = libexec_GetBaseAddress(st, imm8, signExtend16(imm16));
      POP(st, uparm2);
      PUTBSTACK(st, uparm2, uparm1);
      break;

    case oSTSM :
      POP(st, uparm1);            /* Size */
      uparm3 = uparm1;            /* Save for stack discard */
      uparm2 = libexec_GetBaseAddress(st, imm8, signExtend16(imm16));
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
      uparm1 = libexec_GetBaseAddress(st, imm8, signExtend16(imm16) + TOS(st, 0));
      TOS(st, 0) = GETSTACK(st, uparm1);
      break;

    case oLDSXB :
      uparm1     = libexec_GetBaseAddress(st, imm8,
                               signExtend16(imm16) + TOS(st, 0));
      uparm1     = GETBSTACK(st, uparm1);
      sparm      = signExtend8(uparm1);
      TOS(st, 0) = (ustack_t)sparm;
      break;

    case oULDSXB :
      uparm1     = libexec_GetBaseAddress(st, imm8,
                               signExtend16(imm16) + TOS(st, 0));
      TOS(st, 0) = GETBSTACK(st, uparm1);
      break;

    case oLDSXM  :
      POP(st, uparm1);
      POP(st, uparm2);
      uparm2 += libexec_GetBaseAddress(st, imm8, signExtend16(imm16));
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

    case oSTSX  :
      POP(st, uparm1);
      POP(st, uparm2);
      uparm2 += libexec_GetBaseAddress(st, imm8, signExtend16(imm16));
      PUTSTACK(st, uparm1,uparm2);
      break;

    case oSTSXB :
      POP(st, uparm1);
      POP(st, uparm2);
      uparm2 += libexec_GetBaseAddress(st, imm8, signExtend16(imm16));
      PUTBSTACK(st, uparm1, uparm2);
      break;

    case oSTSXM :
      POP(st, uparm1);            /* Size */
      uparm3 = uparm1;            /* Save for stack discard */
      sparm = ROUNDBTOI(uparm1);  /* Size in 16-bit words */
      uparm2 = TOS(st, sparm);    /* index */
      sparm--;
      uparm2 += libexec_GetBaseAddress(st, imm8,  signExtend16(imm16));
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
      uparm1 = libexec_GetBaseAddress(st, imm8, signExtend16(imm16));
      PUSH(st, uparm1);
      break;

    case oLASX :
      TOS(st, 0) = libexec_GetBaseAddress(st, imm8,
                                          signExtend16(imm16) + TOS(st, 0));
      break;

      /* Program Control:  imm8 = level; imm16 = unsigned label (No stack
       * arguments)
       */

    case oPCAL  :
      ret    = libexec_ProcedureCall(st, imm8);
      st->pc = (pasSize_t)imm16;
      return ret;

      /* Long branch operations:  imm8 = long opcode; imm16 = unsigned label. */

    case oLONGOP24 :
      ret = libexec_LongOperation24(st, (enum longOp24_e)imm8, imm16);
      return ret;

      /* Pseudo-operations:  (No stack arguments)
       * For LINE:    imm8 = file number; imm16 = line number
       */

    case oLINE   :
    default :
      ret = eILLEGALOPCODE;
      break;
    }

  /* All non-branching operations exit through here */

  st->pc += 4;
  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: libexec_Initialize
 ****************************************************************************/

struct libexec_s *libexec_Initialize(struct libexec_attr_s *attr)
{
  struct libexec_s *st;
  pasSize_t stackSize;
  pasSize_t adjustedStrSize;
  pasSize_t adjustedRoSize;
  pasSize_t adjustedStkSize;
  pasSize_t adjustedHpSize;

  /* Allocate the p-machine state stucture */

  st = (struct libexec_s *)malloc(sizeof(struct libexec_s));
  if (st == NULL)
    {
      return NULL;
    }

  /* Set up I-Space */

  st->ispace = attr->ispace;
  st->maxpc  = attr->maxpc;

  /* Align sizes of memory regions to 16-bit boundaries. */

  adjustedStrSize  = INT_ALIGNUP(attr->strSize);
  adjustedRoSize   = INT_ALIGNUP(attr->roSize);
  adjustedStkSize  = INT_ALIGNUP(attr->stkSize);
  adjustedHpSize   = INT_ALIGNUP(attr->hpSize);

  /* Allocate the pascal stack.  Organization is string stack, then
   * constant data, then "normal" pascal stack, ending with the heap area.
   */

  stackSize    = adjustedStrSize + adjustedRoSize + adjustedStkSize +
                 adjustedHpSize;
  st->dstack.b = (uint8_t *)malloc(stackSize);
  if (!st->dstack.b)
    {
      free(st);
      return NULL;
    }

  /* Copy the rodata into the stack */

  if (attr->rodata != NULL && attr->roSize > 0)
    {
      memcpy(&st->dstack.b[attr->strSize], attr->rodata, attr->roSize);
    }

  /* Set up info needed to perform a simulated reset */

  st->strSize      = adjustedStrSize;
  st->roSize       = adjustedRoSize;
  st->stkSize      = adjustedStkSize;
  st->hpSize       = adjustedHpSize;
  st->stackSize    = stackSize;

  st->entry        = attr->entry;

  /* Set certain critical variables to a known state */

  st->freeChunks   = 0;
#ifdef CONFIG_PASCAL_DEBUGGER
  st->lastCmd      = eCMD_NONE;
  st->traceIndex   = 0;
  st->nTracePoints = 0;
  st->untilPoint   = 0;
  st->nBreakPoints = 0;
  st->nWatchPoints = 0;
  st->bExecStop    = 0;
#endif

  memset(st->fileTable, 0, MAX_OPEN_FILES * sizeof(execFileTable_t));

  /* Then perform a simulated reset */

  libexec_Reset(st);
  return st;
}

/****************************************************************************
 * Name: libexec_Execute
 ****************************************************************************/

int libexec_Execute(struct libexec_s *st)
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
 * Name: libexec_Reset
 ****************************************************************************/

void libexec_Reset(struct libexec_s *st)
{
  int dndx;

  /* Set up the memory map.  Memory organization will be:
   *
   *  0                                   : String stack
   *  strSize                             : RO-only data
   *  strSize + roSize                    : "Normal" Pascal stack
   *  strSize + roSize + stkSize          : Heap stack
   *  strSize + roSize + stkSize + hpSize : "Normal" Pascal stack
  */

  st->rop   = st->strSize;
  st->spb   = st->rop + st->roSize;
  st->hpb   = st->spb + st->stkSize;

  /* Initialize the emulated P-Machine registers */

  st->csp   = 0;
  st->sp    = st->spb + _FBASE;
  st->fp    = st->spb + _FSLINK;
  st->hsp   = st->hpb;
  st->pc    = st->entry;
  st->lsp   = 0;

  /* Initialize the P-Machine stack
   *
   *         |  Base Address  | + 4 * BPERI
   *         +----------------+
   *         |  Nesting Level | + 3 * BPERI
   *         +----------------+
   *         | Return Address | + 2 * BPERI
   *         +----------------+
   *         |  Dynamic Link  | + BPERI
   *         +----------------+
   *  FP  -> |  Static Link   | 0
   *         +----------------+
   */

  dndx                   = BTOISTACK(st->spb);
  st->dstack.i[dndx + 0] = 0;      /* FP */
  st->dstack.i[dndx + 1] = -1;     /* Return address */
  st->dstack.i[dndx + 2] = 0;      /* Return Address */
  st->dstack.i[dndx + 3] = 0;      /* Nesting Level */

  st->spb               += _FSIZE;

  st->exitCode           = 0;

  /* [Re]-initialize the memory manager */

  libexec_InitializeHeap(st);

  /* [Re]-initialize the file I/O logic */

  libexec_InitializeFile(st);
}

/****************************************************************************
 * Name: libexec_Release
 ****************************************************************************/

void libexec_Release(EXEC_HANDLE_t handle)
{
  struct libexec_s *st = (struct libexec_s *)handle;
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
