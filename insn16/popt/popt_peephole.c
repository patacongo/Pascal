/****************************************************************************
 * popt_peephole.c
 * Optimizer "Peephole" Utility Functions
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
#include <stdio.h>
#include <stdlib.h>

#include "pas_debug.h"
#include "pas_pcode.h"
#include "insn16.h"

#include "pofflib.h"
#include "paslib.h"
#include "pas_insn.h"
#include "pas_errcodes.h"
#include "pas_error.h"
#include "pas_machine.h"

#include "popt.h"
#include "popt_reloc.h"
#include "popt_peephole.h"

/****************************************************************************
 * Public Data
 ****************************************************************************/

opTypeR_t  g_opTable[WINDOW];    /* Pcode Table */
opTypeR_t *g_opPtr[WINDOW];      /* Valid Pcode Pointers */

int16_t    g_nOpPtrs = 0;        /* Number of valid Pcode pointers */
bool       g_endOut  = 0;        /* 1 = oEND pcode has been output */

/****************************************************************************
 * Private Variables
 ****************************************************************************/

static poffHandle_t     g_poffHandle;        /* Handle to POFF object */
static poffProgHandle_t g_poffProgHandle;    /* Handle to temporary program data */
static int16_t          g_nBufferedOpCodes;  /* Number of opcodes in Pcode table */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************/

static void popt_CheckRelocation(opTypeR_t *opCode)
{
  /* Check if the current section offset matches a relocation entry */

  if (g_nextRelocationIndex >= 0 &&
      g_nextRelocation.rl_offset == opCode->offset)
    {
      uint32_t saveRlIndex = g_nextRelocation.rl_offset;

      /* Adjust the relocation section offset so that it will match the
       * location in the file after this optimization pass.
       */

      g_nextRelocation.rl_offset -= (opCode->offset - g_outSectionOffset);

      /* Add the modified relocation to the temporary, output file */

      poffAddTmpRelocation(g_tmpRelocationHandle, &g_nextRelocation);

      /* Get the next relocation entry from the previous pass */

      g_nextRelocationIndex =
        poffNextTmpRelocation(g_prevTmpRelocationHandle, &g_nextRelocation);

      /* Sanity check */

      if (g_nextRelocationIndex >= 0 &&
          g_nextRelocation.rl_offset <= saveRlIndex)
        {
          /* There is no requirement of the format that relocations be
           * ordered by section offset, however, this is how they are
           * generated by the compiler and the current logic depends on
           * this fact.
           */

          error(eBADRELOCDATA);
        }
    }
}

/****************************************************************************/

static void popt_DiscardRelocation(opTypeR_t *opCode)
{
  /* Check if the current section offset matches a relocation entry */

  if (g_nextRelocationIndex >= 0 &&
      g_nextRelocation.rl_offset == opCode->offset)
    {
      uint32_t saveRlIndex = g_nextRelocation.rl_offset;

      /* Discard this relocation and just get the next relocation entry from
       * the previous pass
       */

      g_nextRelocationIndex =
        poffNextTmpRelocation(g_prevTmpRelocationHandle, &g_nextRelocation);

      /* Sanity check */

      if (g_nextRelocationIndex >= 0 &&
          g_nextRelocation.rl_offset <= saveRlIndex)
        {
          /* There is no requirement of the format that relocations be
           * ordered by section offset, however, this is how they are
           * generated by the compiler and the current logic depends on
           * this fact.
           */

          error(eBADRELOCDATA);
        }
    }
}

/****************************************************************************/

static void popt_SetupOpcodePointerList(void)
{
  register int16_t pindex;

  for (pindex = 0; pindex < g_nBufferedOpCodes; pindex++)
    {
      g_opPtr[pindex] = NULL;
    }

  g_nOpPtrs = 0;
  for (pindex = 0; pindex < g_nBufferedOpCodes; pindex++)
    {
      switch (g_opTable[pindex].op)
        {
          /* Terminate list when a break from sequential logic is
           * encountered
           */

        case oRET   :
        case oEND   :
        case oLABEL :
        case oPCAL  :
          return;

          /* Terminate list when a conditional break from sequential logic
           * due to a brnch is encountered but include the branch instruction
           * in the in the list
           */

        case oJMP   :
        case oJEQUZ :
        case oJNEQZ :
        case oJLTZ  :
        case oJGTEZ :
        case oJGTZ  :
        case oJLTEZ :
          g_opPtr[g_nOpPtrs] = &g_opTable[pindex];
          g_nOpPtrs++;
          return;

          /* Skip over NOPs and comment class pcodes */

        case oNOP   :
        case oLINE  :
          break;

          /* Include all other pcodes in the optimization list and continue */

        default     :
          g_opPtr[g_nOpPtrs] = &g_opTable[pindex];
          g_nOpPtrs++;
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************/

void popt_SetupPeephole(poffHandle_t poffHandle,
                        poffProgHandle_t poffProgHandle)
{
  uint32_t opCodeSize;
  int16_t nOpCodes;
  int16_t i;

  g_poffHandle       = poffHandle;
  g_poffProgHandle   = poffProgHandle;
  g_inSectionOffset  = 0;
  g_outSectionOffset = 0;
  g_endOut           = false;

  /* Get the first relocation entry */

  g_nextRelocationIndex  =
    poffNextTmpRelocation(g_prevTmpRelocationHandle, &g_nextRelocation);

  /* Skip over leading pcodes.  NOTE:  assumes executable begins after
   * the first oLABEL pcode
   */

  do
    {
      opCodeSize = insn_GetOpCode(g_poffHandle, (opType_t *)&g_opTable[0]);

      g_opTable[0].offset = g_inSectionOffset;
      g_inSectionOffset += opCodeSize;

      /* Check for a relocation at this instruction */

      popt_CheckRelocation(&g_opTable[0]);

      /* Write the opcode to the temporary section data */

      g_outSectionOffset +=
        insn_AddTmpOpCode(g_poffProgHandle, (opType_t *)&g_opTable[0]);
    }
  while (g_opTable[0].op != oLABEL && g_opTable[0].op != oEND);

  /* Fill the pcode window and setup pointers to working section */

  for (i = 0, nOpCodes = 0; i < WINDOW; i++)
    {
      /* Read the next opcode into the optimization buffer */

      opCodeSize = insn_GetOpCode(g_poffHandle, (opType_t *)&g_opTable[i]);
      if (opCodeSize == EOF)
        {
          break;
        }

      g_opTable[i].offset = g_inSectionOffset;
      g_inSectionOffset += opCodeSize;
      nOpCodes++;
    }

  g_nBufferedOpCodes = nOpCodes;
  popt_SetupOpcodePointerList();
}

/*****************************************************************************/
/* Transfer all buffered P-Codes (except NOPs) to the optimized file */

void popt_UpdatePeephole(void)
{
  uint32_t opCodeSize;
  int16_t nOpCodes;
  int16_t i;
  int16_t j;

  /* Transfer one buffered P-Code (except NOPs) to the optimized file */

  if (g_opTable[0].op != oNOP && !g_endOut)
    {
      /* Check for a relocation at this instruction */

      popt_CheckRelocation(&g_opTable[0]);

      /* Write the opcode to the temporary program data */

      g_outSectionOffset +=
        insn_AddTmpOpCode(g_poffProgHandle, (opType_t *)&g_opTable[0]);

      g_endOut = (g_opTable[0].op == oEND);
    }

  /* What if we deleted an opcode that has a relocation associated with
   * it?  At a minimum, we need to discard that relocation entry.
   */

  else
    {
      popt_DiscardRelocation(&g_opTable[0]);
    }

  /* Move all of the remaining P-Codes (except oNOP's) down one slot */

  for (i = 1, j = 0; i < WINDOW; i++)
    {
      if (g_opTable[i].op != oNOP)
        {
          g_opTable[j].op     = g_opTable[i].op ;
          g_opTable[j].arg1   = g_opTable[i].arg1;
          g_opTable[j].arg2   = g_opTable[i].arg2;
          g_opTable[j].offset = g_opTable[i].offset;
          j++;
        }
    }

  /* Then fill the end slot(s) with a new P-Code from the input file */

  for (i = j, nOpCodes = j; i < WINDOW; i++)
    {
      opCodeSize = insn_GetOpCode(g_poffHandle, (opType_t *)&g_opTable[i]);
      if (opCodeSize == EOF)
        {
          break;
        }

      g_opTable[i].offset = g_inSectionOffset;
      g_inSectionOffset  += opCodeSize;
      nOpCodes++;
    }

  g_nBufferedOpCodes = nOpCodes;
  popt_SetupOpcodePointerList();
}

/*****************************************************************************/

void popt_DeletePCode(int16_t delIndex)
{
  g_opPtr[delIndex]->op   = oNOP;
  g_opPtr[delIndex]->arg1 = 0;
  g_opPtr[delIndex]->arg2 = 0;
  popt_SetupOpcodePointerList();
}

/****************************************************************************/

void popt_DeletePCodePair(int16_t delIndex1, int16_t delIndex2)
{
  g_opPtr[delIndex1]->op   = oNOP;
  g_opPtr[delIndex1]->arg1 = 0;
  g_opPtr[delIndex1]->arg2 = 0;
  g_opPtr[delIndex2]->op   = oNOP;
  g_opPtr[delIndex2]->arg1 = 0;
  g_opPtr[delIndex2]->arg2 = 0;
  popt_SetupOpcodePointerList();
}

/****************************************************************************/

void popt_DeletePCodeTrio(int16_t delIndex1, int16_t delIndex2,
                          int16_t delIndex3)
{
  g_opPtr[delIndex1]->op   = oNOP;
  g_opPtr[delIndex1]->arg1 = 0;
  g_opPtr[delIndex1]->arg2 = 0;
  g_opPtr[delIndex2]->op   = oNOP;
  g_opPtr[delIndex2]->arg1 = 0;
  g_opPtr[delIndex2]->arg2 = 0;
  g_opPtr[delIndex3]->op   = oNOP;
  g_opPtr[delIndex3]->arg1 = 0;
  g_opPtr[delIndex3]->arg2 = 0;
  popt_SetupOpcodePointerList();
}

/****************************************************************************/

void popt_DeletePCodeQuartet(int16_t delIndex1, int16_t delIndex2,
                             int16_t delIndex3, int16_t delIndex4)
{
  g_opPtr[delIndex1]->op   = oNOP;
  g_opPtr[delIndex1]->arg1 = 0;
  g_opPtr[delIndex1]->arg2 = 0;
  g_opPtr[delIndex2]->op   = oNOP;
  g_opPtr[delIndex2]->arg1 = 0;
  g_opPtr[delIndex2]->arg2 = 0;
  g_opPtr[delIndex3]->op   = oNOP;
  g_opPtr[delIndex3]->arg1 = 0;
  g_opPtr[delIndex3]->arg2 = 0;
  g_opPtr[delIndex4]->op   = oNOP;
  g_opPtr[delIndex4]->arg1 = 0;
  g_opPtr[delIndex4]->arg2 = 0;
  popt_SetupOpcodePointerList();
}

/****************************************************************************/

void popt_SwapPCodePair(int16_t swapIndex1, int16_t swapIndex2)
{
  opTypeR_t opCode;

  opCode.op                   = g_opPtr[swapIndex2]->op;
  opCode.arg1                 = g_opPtr[swapIndex2]->arg1;
  opCode.arg2                 = g_opPtr[swapIndex2]->arg2;
  opCode.offset               = g_opPtr[swapIndex2]->offset;

  g_opPtr[swapIndex2]->op     = g_opPtr[swapIndex1]->op;
  g_opPtr[swapIndex2]->arg1   = g_opPtr[swapIndex1]->arg1;
  g_opPtr[swapIndex2]->arg2   = g_opPtr[swapIndex1]->arg2;
  g_opPtr[swapIndex2]->offset = g_opPtr[swapIndex1]->offset;

  g_opPtr[swapIndex1]->op     = opCode.op;
  g_opPtr[swapIndex1]->arg1   = opCode.arg1;
  g_opPtr[swapIndex1]->arg2   = opCode.arg2;
  g_opPtr[swapIndex1]->offset = opCode.offset;

  popt_SetupOpcodePointerList();  /* Shouldn't be necessary */
}

/****************************************************************************/
/* Check if the opcode at this peephole index:  (1) pushs some data on the
 * stack and (2) does not depend on prior stack content.
 */

bool popt_CheckLoadOperation(int16_t chkIndex)
{
  return (g_opPtr[chkIndex]->op == oPUSH   || g_opPtr[chkIndex]->op == oPUSHB ||
          g_opPtr[chkIndex]->op == oUPUSHB ||
          g_opPtr[chkIndex]->op == oLD     || g_opPtr[chkIndex]->op == oLDB  ||
          g_opPtr[chkIndex]->op == oULDB   ||
          g_opPtr[chkIndex]->op == oLDS    || g_opPtr[chkIndex]->op == oLDSB ||
          g_opPtr[chkIndex]->op == oULDSB  ||
          g_opPtr[chkIndex]->op == oLA     || g_opPtr[chkIndex]->op == oLAS   ||
          g_opPtr[chkIndex]->op == oLAC);
}

/****************************************************************************/
/* Check if the opcode at this peephole index:  (1) removes some data on the
 * stack and (2) does not depend on any other prior stack content.
 */

bool popt_CheckStoreOperation(int16_t chkIndex)
{
  return (g_opPtr[chkIndex]->op == oST  || g_opPtr[chkIndex]->op == oSTB  ||
          g_opPtr[chkIndex]->op == oSTS || g_opPtr[chkIndex]->op == oSTSB);
}

/****************************************************************************/
/* Check if the opcode at this peephole index loads an address on the stack.
 * That address is in the 16-bit argument to the instruction (adjusted at
 * run-time for indexing, memory organization, and static nesting level).
 */

bool popt_CheckAddressOperation(int16_t chkIndex)
{
  return (g_opPtr[chkIndex]->op == oLA  || g_opPtr[chkIndex]->op == oLAX ||
          g_opPtr[chkIndex]->op == oLAS || g_opPtr[chkIndex]->op == oLASX  ||
          g_opPtr[chkIndex]->op == oLAC);
}

/****************************************************************************/
/* Check if the opcode at this peephole index is a binary operator.  This
 * excludes the shift instructions (oSLL, oSRL, oSRA) which are really unary
 * operators with an argument.
 */

bool popt_CheckBinaryOperator(int16_t chkIndex)
{
  return (g_opPtr[chkIndex]->op == oADD  || g_opPtr[chkIndex]->op == oSUB  ||
          g_opPtr[chkIndex]->op == oMUL  || g_opPtr[chkIndex]->op == oDIV  ||
          g_opPtr[chkIndex]->op == oMOD  || g_opPtr[chkIndex]->op == oOR   ||
          g_opPtr[chkIndex]->op == oAND  || g_opPtr[chkIndex]->op == oEQU  ||
          g_opPtr[chkIndex]->op == oNEQ  || g_opPtr[chkIndex]->op == oLT   ||
          g_opPtr[chkIndex]->op == oGTE  || g_opPtr[chkIndex]->op == oGT   ||
          g_opPtr[chkIndex]->op == oLTE  || g_opPtr[chkIndex]->op == oUMUL ||
          g_opPtr[chkIndex]->op == oUDIV || g_opPtr[chkIndex]->op == oUMOD ||
          g_opPtr[chkIndex]->op == oULT  || g_opPtr[chkIndex]->op == oUGTE ||
          g_opPtr[chkIndex]->op == oUGT  || g_opPtr[chkIndex]->op == oULTE);
}

/****************************************************************************/
/* Check if the opcode at this peephole index is a transitive binary
 * operator (i.e., the optimizer can swap the order of the arguments and
 * the result is the same).
 */

bool popt_CheckTransitiveOperator(int16_t chkIndex)
{
  return (g_opPtr[chkIndex]->op == oADD  || g_opPtr[chkIndex]->op == oMUL  ||
          g_opPtr[chkIndex]->op == oOR   || g_opPtr[chkIndex]->op == oAND  ||
          g_opPtr[chkIndex]->op == oEQU  || g_opPtr[chkIndex]->op == oNEQ  ||
          g_opPtr[chkIndex]->op == oUMUL);
}

/****************************************************************************/
/* Check if the opcode at this peephole index is a PUSH of a constant value
 * onto the stack.
 */

bool popt_CheckPushConstant(int16_t chkIndex)
{
  return (g_opPtr[chkIndex]->op == oPUSH  ||
          g_opPtr[chkIndex]->op == oPUSHB ||
          g_opPtr[chkIndex]->op == oUPUSHB);
}
