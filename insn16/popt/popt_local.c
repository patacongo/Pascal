/****************************************************************************
 * popt_local.c
 * P-Code Local Optimizer
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
#include <stdio.h>

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
#include "popt_constants.h"
#include "popt_longconst.h"
#include "popt_loadstore.h"
#include "popt_branch.h"
#include "popt_local.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void popt_InitPTable        (void);
static void popt_PutPCodeFromTable (void);
static void popt_SetupPointer      (void);

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

static poffHandle_t     g_myPoffHandle;        /* Handle to POFF object */
static poffProgHandle_t g_myPoffProgHandle;    /* Handle to temporary program data */
static poffRelocation_t g_nextRelocation;
static uint32_t         g_inSectionOffset;     /* Running input section offset */
static uint32_t         g_outSectionOffset;    /* Running output section offset */
static int32_t          g_nextRelocationIndex;

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

/*****************************************************************************/

static void popt_PutPCodeFromTable(void)
{
  uint32_t opCodeSize;
  int16_t i;

  TRACE(stderr, "[popt_PutPCodeFromTable]");

  /* Transfer all buffered P-Codes (except NOPs) to the optimized file */
  do
    {
      if (g_opTable[0].op != oNOP && !g_endOut)
        {
          /* Check for a relocation at this instruction */

          popt_CheckRelocation(&g_opTable[0]);

          /* Transfer the variable-length opcode */

          poffAddTmpProgByte(g_myPoffProgHandle, g_opTable[0].op);
          g_outSectionOffset++;

          if (g_opTable[0].op & o8)
            {
              poffAddTmpProgByte(g_myPoffProgHandle, g_opTable[0].arg1);
              g_outSectionOffset++;
            }

          if (g_opTable[0].op & o16)
            {
              poffAddTmpProgByte(g_myPoffProgHandle,
                                 (g_opTable[0].arg2 >> 8));
              poffAddTmpProgByte(g_myPoffProgHandle,
                                 (g_opTable[0].arg2 & 0xff));
              g_outSectionOffset += 2;
            }

          g_endOut = (g_opTable[0].op == oEND);
        }

      /* What if we deleted an opcode that has a relocation associated with
       * it?  At a minimum, we need to discard that relocation entry.
       */

      else
        {
          popt_DiscardRelocation(&g_opTable[0]);
        }

      /* Move all P-Codes down one slot */

      for (i = 1; i < WINDOW; i++)
        {
          g_opTable[i - 1].op   = g_opTable[i].op ;
          g_opTable[i - 1].arg1 = g_opTable[i].arg1;
          g_opTable[i - 1].arg2 = g_opTable[i].arg2;
        }

      /* Then fill the end slot with a new P-Code from the input file */

      opCodeSize = insn_GetOpCode(g_myPoffHandle,
                                  (opType_t *)&g_opTable[WINDOW - 1]);

      g_opTable[WINDOW - 1].offset = g_inSectionOffset;
      g_inSectionOffset += opCodeSize;
    }
  while (g_opTable[0].op == oNOP);

  popt_SetupPointer();
}

/****************************************************************************/

static void popt_SetupPointer(void)
{
  register int16_t pindex;

  TRACE(stderr, "[popt_SetupPointer]");

  for (pindex = 0; pindex < WINDOW; pindex++)
    {
      g_opPtr[pindex] = NULL;
    }

  g_nOpPtrs = 0;
  for (pindex = 0; pindex < WINDOW; pindex++)
    {
      switch (g_opTable[pindex].op)
        {
          /* Terminate list when a break from sequential logic is
           * encountered
           */

        case oRET   :
        case oEND   :
        case oJMP   :
        case oLABEL :
        case oPCAL  :
          return;

          /* Terminate list when a condition break from sequential logic is
           * encountered but include the conditional branch in the list
           */

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

/****************************************************************************/

static void popt_InitPTable(void)
{
  uint32_t opCodeSize;
  int16_t i;

  TRACE(stderr, "[intPTable]");

  g_inSectionOffset  = 0;
  g_outSectionOffset = 0;

  /* Get the first relocation entry */

  g_nextRelocationIndex  =
    poffNextTmpRelocation(g_tmpRelocationHandle, &g_nextRelocation);

  /* Skip over leading pcodes.  NOTE:  assumes executable begins after
   * the first oLABEL pcode
   */

  do
    {
      opCodeSize = insn_GetOpCode(g_myPoffHandle, (opType_t *)&g_opTable[0]);

      g_opTable[0].offset = g_inSectionOffset;
      g_inSectionOffset += opCodeSize;

      /* Check for a relocation at this instruction */

      popt_CheckRelocation(&g_opTable[0]);

      /* Write the opcode to the temporary section data */

      (void)poffAddTmpProgByte(g_myPoffProgHandle, g_opTable[0].op);
      g_outSectionOffset++;

      if (g_opTable[0].op & o8)
        {
          (void)poffAddTmpProgByte(g_myPoffProgHandle, g_opTable[0].arg1);
          g_outSectionOffset++;
        }

      if (g_opTable[0].op & o16)
        {
          (void)poffAddTmpProgByte(g_myPoffProgHandle, (g_opTable[0].arg2 >> 8));
          (void)poffAddTmpProgByte(g_myPoffProgHandle, (g_opTable[0].arg2 & 0xff));
          g_outSectionOffset += 2;
        }
    }
  while ((g_opTable[0].op != oLABEL) && (g_opTable[0].op != oEND));

  /* Fill the pcode window and setup pointers to working section */

  for (i = 0; i < WINDOW; i++)
    {
      /* Read the next opcode into the optimization buffer */

      opCodeSize = insn_GetOpCode(g_myPoffHandle, (opType_t *)&g_opTable[i]);

      g_opTable[i].offset = g_inSectionOffset;
      g_inSectionOffset += opCodeSize;
    }

  popt_SetupPointer();
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/*****************************************************************************/

void popt_LocalOptimization(poffHandle_t poffHandle,
                            poffProgHandle_t poffProgHandle)
{
  int16_t nchanges;

  TRACE(stderr, "[pass2]");

  /* Save the handles for use by other, private functions */

  g_myPoffHandle     = poffHandle;
  g_myPoffProgHandle = poffProgHandle;

  /* Initialization */

  popt_InitPTable();

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
          nchanges  = popt_UnaryOptimize ();
          nchanges += popt_LongUnaryOptimize();
          nchanges += popt_BinaryOptimize();
          nchanges += popt_LongBinaryOptimize();
          nchanges += popt_BranchOptimize();
          nchanges += popt_LoadOptimize();
          nchanges += popt_StoreOptimize();
          nchanges += popt_ExchangeOptimize();
        }
      while (nchanges);

      popt_PutPCodeFromTable();
    }

  /* All of the relocations should have been adjusted and copied to the
   * optimized output.
   */

  if (g_nextRelocationIndex >= 0)
    {
      error(eEXTRARELOCS);
    }
}

/*****************************************************************************/

void popt_DeletePCode(int16_t delIndex)
{
  TRACE(stderr, "[popt_DeletePCode]");

  g_opPtr[delIndex]->op   = oNOP;
  g_opPtr[delIndex]->arg1 = 0;
  g_opPtr[delIndex]->arg2 = 0;
  popt_SetupPointer();
}

/****************************************************************************/

void popt_DeletePCodePair(int16_t delIndex1, int16_t delIndex2)
{
  TRACE(stderr, "[popt_DeletePCodePair]");

  g_opPtr[delIndex1]->op   = oNOP;
  g_opPtr[delIndex1]->arg1 = 0;
  g_opPtr[delIndex1]->arg2 = 0;
  g_opPtr[delIndex2]->op   = oNOP;
  g_opPtr[delIndex2]->arg1 = 0;
  g_opPtr[delIndex2]->arg2 = 0;
  popt_SetupPointer();
}

/****************************************************************************/

void popt_DeletePCodeTrio(int16_t delIndex1, int16_t delIndex2,
                          int16_t delIndex3)
{
  TRACE(stderr, "[popt_DeletePCodeTrio]");

  g_opPtr[delIndex1]->op   = oNOP;
  g_opPtr[delIndex1]->arg1 = 0;
  g_opPtr[delIndex1]->arg2 = 0;
  g_opPtr[delIndex2]->op   = oNOP;
  g_opPtr[delIndex2]->arg1 = 0;
  g_opPtr[delIndex2]->arg2 = 0;
  g_opPtr[delIndex3]->op   = oNOP;
  g_opPtr[delIndex3]->arg1 = 0;
  g_opPtr[delIndex3]->arg2 = 0;
  popt_SetupPointer();
}

/****************************************************************************/

void popt_DeletePCodeQuartet(int16_t delIndex1, int16_t delIndex2,
                             int16_t delIndex3, int16_t delIndex4)
{
  TRACE(stderr, "[popt_DeletePCodeTrio]");

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
  popt_SetupPointer();
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

  popt_SetupPointer();  /* Shouldn't be necessary */
}
