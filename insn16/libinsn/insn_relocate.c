/**********************************************************************
 * insn_relocate.c
 * Perform P-Code relocations
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
 **********************************************************************/

/**********************************************************************
 * Included Files
 **********************************************************************/

#include <stdint.h>

#include "pas_debug.h"
#include "pas_machine.h"
#include "pas_errcodes.h"
#include "pas_pcode.h"
#include "insn16.h"

#include "pofflib.h"
#include "pas_error.h"
#include "pas_insn.h"

/**********************************************************************
 * Private Functions
 **********************************************************************/

int insn_Relocate(opType_t *op, uint32_t pcOffset, uint32_t roOffset)
{
  switch (op->op)
    {
      /* Catch each instruction that references the read-only data
       * section.
       */

    case oLAC:
      /* Add the offset to the read-only data section */

      op->arg2 += roOffset;
      break;

      /* Catch each instruction that references the text section
       * data via an offset.
       */

    case oPCAL:  /* Procedure / Function calls */
    case oJMP:   /* Unconditional jump */
    case oJEQUZ: /* Jump on unary comparisons with zero */
    case oJNEQZ:
    case oJLTZ:
    case oJGTEZ:
    case oJGTZ:
    case oJLTEZ:
    case oJEQU:  /* Jump on binary comparisons */
    case oJNEQ:
    case oJLT:
    case oJGTE:
    case oJGT:
    case oJLTE:
      /* Add the offset to the text section */

      op->arg2 += pcOffset;
      break;

      /* Return an end of file indication if oEND encountered */

    case oEND:
      return 1;

      /* Otherwise, it is not an interesting opcode */
    default:
      break;
    }

  /* Return 0 on all opcodes other than oEND */

  return 0;
}

/***********************************************************************/

void insn_FixupProcedureCall(uint8_t *progData, uint32_t symValue)
{

  /* Sanity checking */

  if (progData[0] != oPCAL)
    {
      fatal(ePOFFCONFUSION);
    }

  if (symValue > 0xffff)
    {
      fatal(eBADINTRANGE);
    }

  /* Perform the relocation */

  progData[2] = symValue >> 8;
  progData[3] = symValue & 0xff;
}

/***********************************************************************/

void insn_FixupFrameOffset(uint8_t *progData, uint32_t symValue)
{
  int index;

  /* Sanity checking */

  /* Must be an instruction with arg16 equal to a 16-bit frame pointer
   * offset.  The arg8 may or my not be present as a level offset.
   */

  if ((progData[0] & o16) == 0)
    {
      fatal(ePOFFCONFUSION);
    }

  if (symValue > 0xffff)
    {
      fatal(eBADINTRANGE);
    }

  /* Perform the relocation */

  index = ((progData[0] & o8) == 0) ? 1 : 2;

  progData[index]     = symValue >> 8;
  progData[index + 1] = symValue & 0xff;
}
