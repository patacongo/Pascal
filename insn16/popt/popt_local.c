/**********************************************************************
 * popt_local.c
 * P-Code Local Optimizer
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
 **********************************************************************/

/**********************************************************************
 * Included Files
 **********************************************************************/

#include <stdint.h>
#include <stdio.h>

#include "pas_debug.h"
#include "pas_pcode.h"
#include "pinsn16.h"

#include "pofflib.h"
#include "paslib.h"
#include "pas_insn.h"
#include "popt_constants.h"
#include "popt_loadstore.h"
#include "popt_branch.h"
#include "popt_local.h"

/**********************************************************************
 * Private Function Prototypes
 **********************************************************************/

static void initPTable        (void);
static void putPCodeFromTable (void);
static void setupPointer      (void);

/**********************************************************************
 * Public Data
 **********************************************************************/

opType_t  ptable[WINDOW];               /* Pcode Table */
opType_t *pptr[WINDOW];                 /* Valid Pcode Pointers */

int16_t   nops    = 0;                  /* No. Valid Pcode Pointers */
int16_t   end_out = 0;                  /* 1 = oEND pcode has been output */

/**********************************************************************
 * Private Variables
 **********************************************************************/

static poffHandle_t     myPoffHandle;         /* Handle to POFF object */
static poffProgHandle_t myPoffProgHandle;/* Handle to temporary POFF object */


/**********************************************************************
 * Private Functions
 **********************************************************************/

/***********************************************************************/

static void putPCodeFromTable(void)
{
  register int16_t i;

  TRACE(stderr, "[putPCodeFromTable]");

  /* Transfer all buffered P-Codes (except NOPs) to the optimized file */
  do
    {
      if ((ptable[0].op != oNOP) && !(end_out))
        {
          (void)poffAddTmpProgByte(myPoffProgHandle, ptable[0].op);

          if (ptable[0].op & o8)
            (void)poffAddTmpProgByte(myPoffProgHandle, ptable[0].arg1);

          if (ptable[0].op & o16)
            {
              (void)poffAddTmpProgByte(myPoffProgHandle,
                                       (ptable[0].arg2 >> 8));
              (void)poffAddTmpProgByte(myPoffProgHandle,
                                       (ptable[0].arg2 & 0xff));
            }

          end_out =(ptable[0].op == oEND);
        }

      /* Move all P-Codes down one slot */

      for (i = 1; i < WINDOW; i++)
        {
          ptable[i-1].op   = ptable[i].op ;
          ptable[i-1].arg1 = ptable[i].arg1;
          ptable[i-1].arg2 = ptable[i].arg2;
        }

      /* Then fill the end slot with a new P-Code from the input file */

      insn_GetOpCode(myPoffHandle, &ptable[WINDOW-1]);

    } while (ptable[0].op == oNOP);

  setupPointer();
}

/**********************************************************************/

static void setupPointer(void)
{
  register int16_t pindex;

  TRACE(stderr, "[setupPointer]");

  for (pindex = 0; pindex < WINDOW; pindex++)
    {
      pptr[pindex] = (opType_t *) NULL;
    }

  nops = 0;
  for (pindex = 0; pindex < WINDOW; pindex++)
    {
      switch (ptable[pindex].op)
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
          pptr[nops] = &ptable[pindex];
          nops++;
          return;

          /* Skip over NOPs and comment class pcodes */

        case oNOP   :
        case oLINE  :
          break;

          /* Include all other pcodes in the optimization list and continue */

        default     :
          pptr[nops] = &ptable[pindex];
          nops++;
        }
    }
}

/**********************************************************************/

static void initPTable(void)
{
  register int16_t i;

  TRACE(stderr, "[intPTable]");

  /* Skip over leading pcodes.  NOTE:  assumes executable begins after
   * the first oLABEL pcode
   */

  do
    {
      insn_GetOpCode(myPoffHandle, &ptable[0]);

      (void)poffAddTmpProgByte(myPoffProgHandle, ptable[0].op);

      if (ptable[0].op & o8)
        {
          (void)poffAddTmpProgByte(myPoffProgHandle, ptable[0].arg1);
        }

      if (ptable[0].op & o16)
        {
          (void)poffAddTmpProgByte(myPoffProgHandle, (ptable[0].arg2 >> 8));
          (void)poffAddTmpProgByte(myPoffProgHandle, (ptable[0].arg2 & 0xff));
        } /* end if */
    }
  while ((ptable[0].op != oLABEL) && (ptable[0].op != oEND));

  /* Fill the pcode window and setup pointers to working section */

  for (i = 0; i < WINDOW; i++)
    {
      insn_GetOpCode(myPoffHandle, &ptable[i]);
    }

  setupPointer();
}

/**********************************************************************
 * Public Functions
 **********************************************************************/

/***********************************************************************/

void localOptimization(poffHandle_t poffHandle,
                       poffProgHandle_t poffProgHandle)
{
  int16_t nchanges;

  TRACE(stderr, "[pass2]");

  /* Save the handles for use by other, private functions */

  myPoffHandle     = poffHandle;
  myPoffProgHandle = poffProgHandle;

  /* Initialization */

  initPTable();

  /* Outer loop traverse the file op-code by op-code until the oEND P-Code
   * has been output.  NOTE:  it is assumed throughout that oEND is the
   * final P-Code in the program data section.
   */

  while (!(end_out))
    {
      /* The inner loop optimizes the buffered P-Codes until no further
       * changes can be made.  Then the outer loop will advance the buffer
       * by one P-Code
       */

      do
        {
          nchanges  = popt_UnaryOptimize ();
          nchanges += popt_BinaryOptimize();
          nchanges += popt_BranchOptimize();
          nchanges += popt_LoadOptimize();
          nchanges += popt_StoreOptimize();
          nchanges += popt_ExchangeOptimize();
        } while (nchanges);

      putPCodeFromTable();
    }
}

/***********************************************************************/

void deletePcode(int16_t delIndex)
{
  TRACE(stderr, "[deletePcode]");

  pptr[delIndex]->op   = oNOP;
  pptr[delIndex]->arg1 = 0;
  pptr[delIndex]->arg2 = 0;
  setupPointer();
}

/**********************************************************************/

void deletePcodePair(int16_t delIndex1, int16_t delIndex2)
{
  TRACE(stderr, "[deletePcodePair]");

  pptr[delIndex1]->op   = oNOP;
  pptr[delIndex1]->arg1 = 0;
  pptr[delIndex1]->arg2 = 0;
  pptr[delIndex2]->op   = oNOP;
  pptr[delIndex2]->arg1 = 0;
  pptr[delIndex2]->arg2 = 0;
  setupPointer();
}

/**********************************************************************/

void swapPcodePair(int16_t swapIndex1, int16_t swapIndex2)
{
  opType_t opCode;

  opCode.op              = pptr[swapIndex2]->op;
  opCode.arg1            = pptr[swapIndex2]->arg1;
  opCode.arg2            = pptr[swapIndex2]->arg2;

  pptr[swapIndex2]->op   = pptr[swapIndex1]->op;
  pptr[swapIndex2]->arg1 = pptr[swapIndex1]->arg1;
  pptr[swapIndex2]->arg2 = pptr[swapIndex1]->arg2;

  pptr[swapIndex1]->op   = opCode.op;
  pptr[swapIndex1]->arg1 = opCode.arg1;
  pptr[swapIndex1]->arg2 = opCode.arg2;

  setupPointer();  /* Shouldn't be necessary */
}