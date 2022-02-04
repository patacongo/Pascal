/****************************************************************************
 * plongops.c
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

#include "longops.h"
#include "pas_longops.h"
#include "pas_errcodes.h"

#include "paslib.h"
#include "pexec.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

union uWord_u
{
  int32_t  sData;
  uint32_t uData;
  uint16_t word[2];
};

typedef union uWord_u uWord_t;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pexec_UPop32
 *
 * Descripton:
 *   Pop a 32-bit unsigned value from the top of the stack
 *
 ****************************************************************************/

static uint32_t pexec_UPop32(struct pexec_s *st)
{
  uWord_t uWord;

  POP(st, uWord.word[1]);
  POP(st, uWord.word[0]);
  return uWord.uData;
}

/****************************************************************************
 * Name: pexec_UPush32
 *
 * Descripton:
 *   Push a 32-bit unsigned value at the top of the stack
 *
 ****************************************************************************/

static void pexec_UPush32(struct pexec_s *st, uint32_t value)
{
  uWord_t uWord;

  uWord.uData = value;
  PUSH(st, uWord.word[0]);
  PUSH(st, uWord.word[1]);
}

/****************************************************************************
 * Name: pexec_UGetTos32
 *
 * Descripton:
 *   Get a copy of the 32-bit unsigned value at the top of the stack
 *
 ****************************************************************************/

static uint32_t pexec_UGetTos32(struct pexec_s *st, int offset32)
{
  uWord_t uWord;

  uWord.word[1] = TOS(st, offset32);
  uWord.word[0] = TOS(st, offset32 + 1);
  return uWord.uData;
}

/****************************************************************************
 * Name: pexec_UPutTos32
 *
 * Descripton:
 *   Write a 32-bit unsigned value to the top of the stack
 *
 ****************************************************************************/

static void pexec_UPutTos32(struct pexec_s *st, uint32_t value, int offset32)
{
  uWord_t uWord;

  uWord.uData            = value;
  TOS(st, offset32)      = uWord.word[1];
  TOS(st, offset32 + 1)  = uWord.word[0];
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pexec_LongOperation8
 *
 * Descripton:
 *   Handle LONGOP + 8-bit operation with no immediate data
 *
 ****************************************************************************/

int pexec_LongOperation8(struct pexec_s *st, uint8_t opcode)
{
  int32_t  sparm1;
  int32_t  sparm2;
  uint32_t uparm1;
  uint32_t uparm2;
  uint16_t result;

  switch (opcode)
    {
    case oDNOP  :
      break;

    /* Arithmetic & logical & and integer conversions (One stack argument) */

    case oDNEG  :
      pexec_UPutTos32(st, -(int32_t)pexec_UGetTos32(st, 0), 0);
      break;

    case oDABS  :
      sparm1 = (int32_t)pexec_UGetTos32(st, 0);
      if (sparm1 < 0)
        {
          pexec_UPutTos32(st, -sparm1, 0);
        }
      break;

    case oDINC  :
      pexec_UPutTos32(st, pexec_UGetTos32(st, 0) + 1, 0);
      break;

    case oDDEC  :
      pexec_UPutTos32(st, pexec_UGetTos32(st, 0) - 1, 0);
      break;

    case oDNOT  :
      pexec_UPutTos32(st, ~pexec_UGetTos32(st, 0), 0);
      break;

    /* Arithmetic & logical (Two stack arguments) */

    case oDADD :
      sparm1 = (int32_t)pexec_UPop32(st);
      pexec_UPutTos32(st, (int32_t)pexec_UGetTos32(st, 0) + sparm1, 0);
      break;

    case oDSUB :
      sparm1 = (int32_t)pexec_UPop32(st);
      pexec_UPutTos32(st, (int32_t)pexec_UGetTos32(st, 0) - sparm1, 0);
      break;

    case oDMUL :
      sparm1 = (int32_t)pexec_UPop32(st);
      pexec_UPutTos32(st, (int32_t)pexec_UGetTos32(st, 0) * sparm1, 0);
      break;

    case oDUMUL :
      uparm1 = pexec_UPop32(st);
      pexec_UPutTos32(st, pexec_UGetTos32(st, 0) * uparm1, 0);
      break;

    case oDDIV :
      sparm1 = (int32_t)pexec_UPop32(st);
      pexec_UPutTos32(st, (int32_t)pexec_UGetTos32(st, 0) / sparm1, 0);
      break;

    case oDUDIV :
      uparm1 = pexec_UPop32(st);
      pexec_UPutTos32(st, pexec_UGetTos32(st, 0) / uparm1, 0);
      break;

    case oDMOD :
      sparm1 = (int32_t)pexec_UPop32(st);
      pexec_UPutTos32(st, (int32_t)pexec_UGetTos32(st, 0) % sparm1, 0);
      break;

    case oDUMOD :
      uparm1 = pexec_UPop32(st);
      pexec_UPutTos32(st, pexec_UGetTos32(st, 0) % uparm1, 0);
      break;

    case oDSLL :
      sparm1 = (int32_t)pexec_UPop32(st);
      pexec_UPutTos32(st, (int32_t)pexec_UGetTos32(st, 0) << sparm1, 0);
      break;

    case oDSRL :
      sparm1 = (int32_t)pexec_UPop32(st);
      pexec_UPutTos32(st, pexec_UGetTos32(st, 0) >> sparm1, 0);
      break;

    case oDSRA :
      sparm1 = (int32_t)pexec_UPop32(st);
      pexec_UPutTos32(st, (int32_t)pexec_UGetTos32(st, 0) >> sparm1, 0);
      break;

    case oDOR  :
      uparm1 = pexec_UPop32(st);
      pexec_UPutTos32(st, pexec_UGetTos32(st, 0) | uparm1, 0);
      break;

    case oDAND :
      uparm1 = pexec_UPop32(st);
      pexec_UPutTos32(st, pexec_UGetTos32(st, 0) & uparm1, 0);
      break;

    case oDXOR :
      uparm1 = pexec_UPop32(st);
      pexec_UPutTos32(st, pexec_UGetTos32(st, 0) ^ uparm1, 0);
      break;

    /* Comparisons (One stack argument) */

    case oDEQUZ :
      sparm1 = (int32_t)pexec_UPop32(st);
      result = PASCAL_FALSE;
      if (sparm1 == 0)
        {
          result = PASCAL_TRUE;
        }

      PUSH(st, result);
      break;

    case oDNEQZ :
      sparm1 = (int32_t)pexec_UPop32(st);
      result = PASCAL_FALSE;
      if (sparm1 != 0)
        {
          result = PASCAL_TRUE;
        }

      PUSH(st, result);
      break;

    case oDLTZ  :
      sparm1 = (int32_t)pexec_UPop32(st);
      result = PASCAL_FALSE;
      if (sparm1 < 0)
        {
          result = PASCAL_TRUE;
        }

      PUSH(st, result);
      break;

    case oDGTEZ :
      sparm1 = (int32_t)pexec_UPop32(st);
      result = PASCAL_FALSE;
      if (sparm1 >= 0)
        {
          result = PASCAL_TRUE;
        }

      PUSH(st, result);
      break;

    case oDGTZ  :
      sparm1 = (int32_t)pexec_UPop32(st);
      result = PASCAL_FALSE;
      if (sparm1 > 0)
        {
          result = PASCAL_TRUE;
        }

      PUSH(st, result);
      break;

    case oDLTEZ :
      sparm1 = (int32_t)pexec_UPop32(st);
      result = PASCAL_FALSE;
      if (sparm1 <= 0)
        {
          result = PASCAL_TRUE;
        }

      PUSH(st, result);
      break;

    /* Comparisons (Two stack arguments) */

    case oDEQU  :
      sparm1 = (int32_t)pexec_UPop32(st);
      sparm2 = (int32_t)pexec_UPop32(st);
      result = PASCAL_FALSE;
      if (sparm1 == sparm2)
        {
          result = PASCAL_TRUE;
        }

      PUSH(st, result);
      break;

    case oDNEQ  :
      sparm1 = (int32_t)pexec_UPop32(st);
      sparm2 = (int32_t)pexec_UPop32(st);
      result = PASCAL_FALSE;
      if (sparm1 != sparm2)
        {
          result = PASCAL_TRUE;
        }

      PUSH(st, result);
      break;

    case oDLT   :
      sparm1 = (int32_t)pexec_UPop32(st);
      sparm2 = (int32_t)pexec_UPop32(st);
      result = PASCAL_FALSE;
      if (sparm1 < sparm2)
        {
          result = PASCAL_TRUE;
        }

      PUSH(st, result);
      break;

    case oDGTE  :
      sparm1 = (int32_t)pexec_UPop32(st);
      sparm2 = (int32_t)pexec_UPop32(st);
      result = PASCAL_FALSE;
      if (sparm1 >= sparm2)
        {
          result = PASCAL_TRUE;
        }

      PUSH(st, result);
      break;

    case oDGT   :
      sparm1 = (int32_t)pexec_UPop32(st);
      sparm2 = (int32_t)pexec_UPop32(st);
      result = PASCAL_FALSE;
      if (sparm1 > sparm2)
        {
          result = PASCAL_TRUE;
        }

      PUSH(st, result);
      break;

    case oDLTE  :
      sparm1 = (int32_t)pexec_UPop32(st);
      sparm2 = (int32_t)pexec_UPop32(st);
      result = PASCAL_FALSE;
      if (sparm1 <= sparm2)
        {
          result = PASCAL_TRUE;
        }

      PUSH(st, result);
      break;

    case oDULT   :
      uparm1 = pexec_UPop32(st);
      uparm2 = pexec_UPop32(st);
      result = PASCAL_FALSE;
      if (result < uparm2)
        {
          result = PASCAL_TRUE;
        }

      PUSH(st, result);
      break;

    case oDUGTE  :
      uparm1 = pexec_UPop32(st);
      uparm2 = pexec_UPop32(st);
      result = PASCAL_FALSE;
      if (uparm1 >= uparm2)
        {
          result = PASCAL_TRUE;
        }

      PUSH(st, result);
      break;

    case oDUGT   :
      uparm1 = pexec_UPop32(st);
      uparm2 = pexec_UPop32(st);
      result = PASCAL_FALSE;
      if (uparm1 > uparm2)
        {
          result = PASCAL_TRUE;
        }

      PUSH(st, result);
      break;

    case oDULTE  :
      uparm1 = pexec_UPop32(st);
      uparm2 = pexec_UPop32(st);
      result = PASCAL_FALSE;
      if (uparm1 <= uparm2)
        {
          result = PASCAL_TRUE;
        }

      PUSH(st, result);
      break;

    /* Stack operations */

    case oDDUP :
      uparm1 = pexec_UGetTos32(st, 0);
      pexec_UPush32(st, uparm1);
      break;

    case oDXCHG :
      uparm1 = pexec_UGetTos32(st, 0);
      uparm2 = pexec_UGetTos32(st, 1);
      pexec_UPutTos32(st, uparm2, 0); /* Swap 16-bit values */
      pexec_UPutTos32(st, uparm1, 1);
      break;

    default :
      return eILLEGALOPCODE;
    }

  return eNOERROR;
}

/****************************************************************************
 * Name: pexec_LongOperation24
 *
 * Descripton:
 *   Handle LONGOP + 24-bit operation with 16-bits of immediate data (imm16)
 *
 ****************************************************************************/

int pexec_LongOperation24(struct pexec_s *st, uint8_t opcode, uint16_t imm16)
{
  int32_t  sparm1;
  int32_t  sparm2;
  uint32_t uparm1;
  uint32_t uparm2;
  int      ret = eNOERROR;

  switch (opcode)
    {
    /* Program control:  imm16 = unsigned label (One stack argument) */

    case oDJEQUZ :
      sparm1 = (int32_t)pexec_UPop32(st);
      if (sparm1 == 0)
        {
          goto branchOut;
        }
      break;

    case oDJNEQZ :
      sparm1 = (int32_t)pexec_UPop32(st);
      if (sparm1 != 0)
        {
          goto branchOut;
        }
      break;

    case oDJLTZ  :
      sparm1 = (int32_t)pexec_UPop32(st);
      if (sparm1 < 0)
        {
          goto branchOut;
        }
      break;

    case oDJGTEZ :
      sparm1 = (int32_t)pexec_UPop32(st);
      if (sparm1 >= 0)
        {
          goto branchOut;
        }
      break;

    case oDJGTZ  :
      sparm1 = (int32_t)pexec_UPop32(st);
      if (sparm1 > 0)
        {
          goto branchOut;
        }
      break;

    case oDJLTEZ :
      sparm1 = (int32_t)pexec_UPop32(st);
      if (sparm1 <= 0)
        {
          goto branchOut;
        }
      break;

      /* Program control:  imm16 = unsigned label (Two stack arguments) */

    case oDJEQU :
      sparm1 = (int32_t)pexec_UPop32(st);
      sparm2 = (int32_t)pexec_UPop32(st);
      if (sparm2 == sparm1)
        {
          goto branchOut;
        }
      break;

    case oDJNEQ :
      sparm1 = (int32_t)pexec_UPop32(st);
      sparm2 = (int32_t)pexec_UPop32(st);
      if (sparm2 != sparm1)
        {
          goto branchOut;
        }
      break;

    case oDJLT  :
      sparm1 = (int32_t)pexec_UPop32(st);
      sparm2 = (int32_t)pexec_UPop32(st);
      if (sparm2 < sparm1)
        {
          goto branchOut;
        }
      break;

    case oDJGTE :
      sparm1 = (int32_t)pexec_UPop32(st);
      sparm2 = (int32_t)pexec_UPop32(st);
      if (sparm2 >= sparm1)
        {
          goto branchOut;
        }
      break;

    case oDJGT  :
      sparm1 = (int32_t)pexec_UPop32(st);
      sparm2 = (int32_t)pexec_UPop32(st);
      if (sparm2 > sparm1)
        {
          goto branchOut;
        }
      break;

    case oDJLTE :
      sparm1 = (int32_t)pexec_UPop32(st);
      sparm2 = (int32_t)pexec_UPop32(st);
      if (sparm2 <= sparm1)
        {
          goto branchOut;
        }
      break;

    case oDJULT  :
      uparm1 = pexec_UPop32(st);
      uparm2 = pexec_UPop32(st);
      if (uparm2 < uparm1)
        {
          goto branchOut;
        }
      break;

    case oDJUGTE :
      uparm1 = pexec_UPop32(st);
      uparm2 = pexec_UPop32(st);
      if (uparm2 >= uparm1)
        {
          goto branchOut;
        }
      break;

    case oDJUGT  :
      uparm1 = pexec_UPop32(st);
      uparm2 = pexec_UPop32(st);
      if (uparm2 > uparm1)
        {
          goto branchOut;
        }
      break;

    case oDJULTE :
      uparm1 = pexec_UPop32(st);
      uparm2 = pexec_UPop32(st);
      if (uparm2 <= uparm1)
        {
          goto branchOut;
        }
      break;

    default:
      ret = eILLEGALOPCODE;
      break;
    }

  st->pc += 4;
  return ret;

branchOut:
  st->pc = (paddr_t)imm16;
  return ret;
}
