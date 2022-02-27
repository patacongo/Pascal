/****************************************************************************
 * popt_longconst.c
 * Long Constant Expression Optimizations
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

#include "pas_debug.h"
#include "pas_machine.h"
#include "insn16.h"
#include "longops.h"

#include "paslib.h"
#include "popt.h"
#include "popt_peephole.h"
#include "popt_util.h"
#include "popt_local.h"

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
 * Public Functions
 ****************************************************************************/

/****************************************************************************/

int16_t popt_LongUnaryOptimize(void)
{
  int16_t nchanges = 0;
  int i;

  /* At least two pcodes are need to perform unary optimizations */

  i = 1;
  while (i < g_nOpPtrs)
    {
      /* Check for a long operation. */

      if (g_opPtr[i]->op == oLONGOP8)
        {
          int j1 = i - 1;
          int j0 = i - 2;

          /* Check for a long operation on a constant (PUSHed) value.  All Unary
            * operators are LONGOP8 types.
            */

          if (g_opPtr[j1]->op == oPUSH || g_opPtr[j1]->op == oPUSHB ||
              g_opPtr[j1]->op == oUPUSHB)
            {
              /* With two opcodes, only the 16- to 32-bit conversion long
               * operations can be optimized.
               */

              /* Conversion of signed 16-bit value to a signed long value */

              if (g_opPtr[i]->arg1 == oCNVD)
                {
                  uWord_t word;

                  /* Turn the oPUSHB or oUPUSHB into an oPUSH op (PUSH) */

                  popt_ExpandPush(g_opPtr[j1]);
                  word.sData = signExtend16(g_opPtr[j1]->arg2);

                  g_opPtr[j1]->arg2 = word.word[0];
                  popt_OptimizePush(g_opPtr[j1]);

                  g_opPtr[i]->op   = oPUSH;
                  g_opPtr[i]->arg1 = 0;
                  g_opPtr[i]->arg2 = word.word[1];
                  popt_OptimizePush(g_opPtr[i]);

                  i++;
                  continue;
                }

              /* Conversion of unsigned 16-bit value to an unsigned long value */

              if (g_opPtr[i]->arg1 == oUCNVD)
                {
                  uWord_t word;

                  /* Turn the oPUSHB or oUPUSHB into an oPUSH op (PUSH) */

                  popt_ExpandPush(g_opPtr[j1]);
                  word.uData = (uint32_t)g_opPtr[j1]->arg2;

                  g_opPtr[j1]->arg2 = word.word[0];
                  popt_OptimizePush(g_opPtr[j1]);

                  g_opPtr[i]->op   = oPUSH;
                  g_opPtr[i]->arg1 = 0;
                  g_opPtr[i]->arg2 = word.word[1];
                  popt_OptimizePush(g_opPtr[i]);

                  i++;
                  continue;
                }

              /* All other Unary optimatizations required 2 constant pushes */

              if (j0 >= 0 &&
                  (g_opPtr[j0]->op == oPUSH || g_opPtr[j0]->op == oPUSHB ||
                   g_opPtr[j0]->op == oUPUSHB))
                {
                  uWord_t word;

                  /* Turn the oPUSHB or oUPUSHB into an oPUSH op (PUSH) */

                  popt_ExpandPush(g_opPtr[j1]);
                  popt_ExpandPush(g_opPtr[j0]);

                  word.word[0] = g_opPtr[j0]->arg2;
                  word.word[1] = g_opPtr[j1]->arg2;

                  /* Handle according to the specific long operation in arg1 */

                  switch (g_opPtr[i]->arg1)
                    {
                      /* Convert 32-bit value to 16-bit value (signed or unsigned) */

                      case oDCNV :
                        g_opPtr[j0]->arg2 = (uint16_t)(word.uData & 0xffff);

                        popt_DeletePCode(j1);
                        popt_DeletePCode(i);
                        nchanges++;
                        continue;

                      /* Delete unary operators on constants */

                      case oDNEG :
                        word.sData        = -word.sData;
                        g_opPtr[j0]->arg2 = word.word[0];
                        g_opPtr[j1]->arg2 = word.word[1];

                        popt_DeletePCode(i);
                        nchanges++;
                        continue;

                      case oDABS :
                        if (word.sData < 0)
                          {
                            word.sData        = -word.sData;
                            g_opPtr[j0]->arg2 = word.word[0];
                            g_opPtr[j1]->arg2 = word.word[1];
                          }

                        popt_DeletePCode(i);
                        nchanges++;
                        continue;

                     case oDINC :
                        word.uData++;
                        g_opPtr[j0]->arg2 = word.word[0];
                        g_opPtr[j1]->arg2 = word.word[1];

                        popt_DeletePCode(i);
                        nchanges++;
                        continue;

                      case oDDEC :
                        word.uData--;
                        g_opPtr[j0]->arg2 = word.word[0];
                        g_opPtr[j1]->arg2 = word.word[1];

                        popt_DeletePCode(i);
                        nchanges++;
                        continue;

                      case oDNOT   :
                        word.sData        = ~word.sData;
                        g_opPtr[j0]->arg2 = word.word[0];
                        g_opPtr[j1]->arg2 = word.word[1];

                        popt_DeletePCode(i);
                        nchanges++;
                        continue;

                      /* Simplify binary operations on constants */

                     case oDADD :
                        if (word.sData == 0)
                          {
                            popt_DeletePCodeTrio(i, j1, j0);
                            nchanges++;
                            continue;
                          }
                        else if (word.sData == 1)
                          {
                            g_opPtr[i]->arg1 = oDINC;
                            popt_DeletePCodePair(j1, j0);
                            nchanges++;
                          }
                        else if (word.sData == -1)
                          {
                            g_opPtr[i]->arg1 = oDDEC;
                            popt_DeletePCodePair(j1, j0);
                            nchanges++;
                          }
                       break;

                      case oDSUB :
                        if (word.sData == 0)
                          {
                            popt_DeletePCodeTrio(i, j1, j0);
                            nchanges++;
                            continue;
                          }
                        else if (word.sData == 1)
                          {
                            g_opPtr[i]->arg1 = oDDEC;
                            popt_DeletePCodePair(j1, j0);
                            nchanges++;
                          }
                        else if (word.sData == -1)
                          {
                            g_opPtr[i]->arg1 = oDINC;
                            popt_DeletePCodePair(j1, j0);
                            nchanges++;
                          }
                        break;

                      case oDMUL :
                      case oDUMUL :
                      case oDDIV :
                      case oDUDIV :
                        if (word.sData == 1)
                          {
                            popt_DeletePCodeTrio(i, j1, j0);
                            nchanges++;
                            continue;
                          }
                        else
                          {
                            int16_t temp =
                              popt_PowerOfTwo((uint32_t)g_opPtr[i]->arg2);

                            if (temp >= 1)
                              {
                                g_opPtr[j1]->arg2 = temp;
                                if (g_opPtr[i]->arg1 == oDMUL ||
                                    g_opPtr[i]->arg1 == oDUMUL)
                                  {
                                    g_opPtr[i]->arg1 = oDSLL;
                                  }
                                else if (g_opPtr[i]->arg1 == oDDIV)
                                  {
                                    g_opPtr[i]->arg1 = oDSRA;
                                  }
                                else /* if g_opPtr[i]->arg1 == oDUDIV) */
                                 {
                                    g_opPtr[i]->arg1 = oDSRL;
                                  }

                                nchanges++;
                              }
                          }
                        break;

                      case oDSLL :
                      case oDSRL :
                      case oDSRA :
                      case oDOR  :
                        if (word.sData == 0)
                          {
                            popt_DeletePCodeTrio(i, j1, j0);
                            nchanges++;
                            continue;
                          }
                        break;

                      case oDAND :
                        if (word.uData == 0xffffffff)
                          {
                            popt_DeletePCodeTrio(i, j1, j0);
                            nchanges++;
                            continue;
                          }
                        break;

                      /* Delete comparisons of constants to zero */

                      case oDEQUZ  :
                        if (word.sData == 0)
                          {
                            word.sData = PASCAL_TRUE;
                          }
                        else
                          {
                            word.sData = PASCAL_FALSE;
                          }

                        g_opPtr[j0]->arg2 = word.word[0];
                        g_opPtr[j1]->arg2 = word.word[1];

                        popt_DeletePCode(i);
                        nchanges++;
                        continue;

                      case oDNEQZ  :
                        if (word.sData != 0)
                          {
                            word.sData = PASCAL_TRUE;
                          }
                        else
                          {
                            word.sData = PASCAL_FALSE;
                          }

                        g_opPtr[j0]->arg2 = word.word[0];
                        g_opPtr[j1]->arg2 = word.word[1];

                        popt_DeletePCode(i);
                        nchanges++;
                        continue;

                      case oDLTZ   :
                        if (word.sData < 0)
                          {
                            word.sData = PASCAL_TRUE;
                          }
                        else
                          {
                            word.sData = PASCAL_FALSE;
                          }

                        g_opPtr[j0]->arg2 = word.word[0];
                        g_opPtr[j1]->arg2 = word.word[1];

                        popt_DeletePCode(i);
                        nchanges++;
                        continue;

                      case oDGTEZ  :
                        if (word.sData >= 0)
                          {
                            word.sData = PASCAL_TRUE;
                          }
                        else
                          {
                            word.sData = PASCAL_FALSE;
                          }

                        g_opPtr[j0]->arg2 = word.word[0];
                        g_opPtr[j1]->arg2 = word.word[1];

                        popt_DeletePCode(i);
                        nchanges++;
                        continue;

                      case oDGTZ   :
                        if (word.sData > 0)
                          {
                            word.sData = PASCAL_TRUE;
                          }
                        else
                          {
                            word.sData = PASCAL_FALSE;
                          }

                        g_opPtr[j0]->arg2 = word.word[0];
                        g_opPtr[j1]->arg2 = word.word[1];

                        popt_DeletePCode(i);
                        nchanges++;
                        continue;

                      case oDLTEZ :
                        if (word.sData <= 0)
                          {
                            word.sData = PASCAL_TRUE;
                          }
                        else
                          {
                            word.sData = PASCAL_FALSE;
                          }

                        g_opPtr[j0]->arg2 = word.word[0];
                        g_opPtr[j1]->arg2 = word.word[1];

                        popt_DeletePCode(i);
                        nchanges++;
                        continue;

                        /* Simplify comparisons with certain constants */

                      case oDEQU :
                        if (word.sData == 0)
                          {
                            g_opPtr[i]->arg1    = oDEQUZ;
                            popt_DeletePCodePair(i, j1);
                            nchanges++;
                            continue;
                          }
                        else if (word.sData == 1)
                          {
                            g_opPtr[j1]->op     = oLONGOP8;
                            g_opPtr[j1]->arg1   = oDDEC;
                            g_opPtr[j1]->arg2   = 0;

                            g_opPtr[i]->arg1    = oDEQUZ;

                            popt_DeletePCode(j0);
                            nchanges++;
                          }
                        else if (word.sData == -1)
                          {
                            g_opPtr[j1]->op     = oLONGOP8;
                            g_opPtr[j1]->arg1   = oDINC;
                            g_opPtr[j1]->arg2   = 0;

                            g_opPtr[i]->arg1    = oDEQUZ;

                            popt_DeletePCode(j0);
                            nchanges++;
                          }
                        break;

                      case oDNEQ :
                        if (word.sData == 0)
                          {
                            g_opPtr[i]->arg1   = oDNEQZ;
                            popt_DeletePCodePair(i, j1);
                            nchanges++;
                            continue;
                          }
                        else if (word.sData == 1)
                          {
                            g_opPtr[j1]->op     = oLONGOP8;
                            g_opPtr[j1]->arg1   = oDDEC;
                            g_opPtr[j1]->arg2   = 0;

                            g_opPtr[i]->arg1    = oDNEQZ;

                            popt_DeletePCode(j0);
                            nchanges++;
                          }
                        else if (word.sData == -1)
                          {
                            g_opPtr[j1]->op     = oLONGOP8;
                            g_opPtr[j1]->arg1   = oDINC;
                            g_opPtr[j1]->arg2   = 0;

                            g_opPtr[i]->arg1   = oDNEQZ;

                            popt_DeletePCode(j0);
                            nchanges++;
                          }
                        break;

                      case oDLT :
                        if (word.sData == 0)
                          {
                            g_opPtr[i]->arg1   = oDLTZ;
                            popt_DeletePCodePair(i, j1);
                            nchanges++;
                            continue;
                          }
                        else if (word.sData == 1)
                          {
                            g_opPtr[j1]->op     = oLONGOP8;
                            g_opPtr[j1]->arg1   = oDDEC;
                            g_opPtr[j1]->arg2   = 0;

                            g_opPtr[i]->arg1    = oDLTZ;

                            popt_DeletePCode(j0);
                            nchanges++;
                          }
                        else if (word.sData == -1)
                          {
                            g_opPtr[j1]->op     = oLONGOP8;
                            g_opPtr[j1]->arg1   = oDINC;
                            g_opPtr[j1]->arg2   = 0;

                            g_opPtr[i]->arg1    = oDLTZ;

                            popt_DeletePCode(j0);
                            nchanges++;
                          }
                        break;

                      case oDGTE :
                        if (word.sData == 0)
                          {
                            g_opPtr[i]->arg1   = oDGTEZ;
                            popt_DeletePCodePair(i, j1);
                            nchanges++;
                            continue;
                          }
                        else if (word.sData == 1)
                          {
                            g_opPtr[j1]->op     = oLONGOP8;
                            g_opPtr[j1]->arg1   = oDDEC;
                            g_opPtr[j1]->arg2   = 0;

                            g_opPtr[i]->arg1    = oDGTEZ;

                            popt_DeletePCode(j0);
                            nchanges++;
                          }
                        else if (word.sData == -1)
                          {
                            g_opPtr[j1]->op     = oLONGOP8;
                            g_opPtr[j1]->arg1   = oDINC;
                            g_opPtr[j1]->arg2   = 0;

                            g_opPtr[i]->arg1    = oDGTEZ;

                            popt_DeletePCode(j0);
                            nchanges++;
                          }
                        break;

                      case oDGT :
                        if (word.sData == 0)
                          {
                            g_opPtr[i]->arg1   = oDGTZ;
                            popt_DeletePCodePair(i, j1);
                            nchanges++;
                            continue;
                          }
                        else if (word.sData == 1)
                          {
                            g_opPtr[j1]->op     = oLONGOP8;
                            g_opPtr[j1]->arg1   = oDDEC;
                            g_opPtr[j1]->arg2   = 0;

                            g_opPtr[i]->arg1    = oDGTZ;

                            popt_DeletePCode(j0);
                            nchanges++;
                          }
                        else if (word.sData == -1)
                          {
                            g_opPtr[j1]->op     = oLONGOP8;
                            g_opPtr[j1]->arg1   = oDINC;
                            g_opPtr[j1]->arg2   = 0;

                            g_opPtr[i]->arg1    = oDGTZ;

                            popt_DeletePCode(j0);
                            nchanges++;
                          }
                        break;

                      case oDLTE :
                        if (word.sData == 0)
                          {
                            g_opPtr[i]->arg1   = oDLTEZ;
                            popt_DeletePCodePair(i, j1);
                            nchanges++;
                            continue;
                          }
                        else if (word.sData == 1)
                          {
                            g_opPtr[j1]->op     = oLONGOP8;
                            g_opPtr[j1]->arg1   = oDDEC;
                            g_opPtr[j1]->arg2   = 0;

                            g_opPtr[i]->arg1    = oDLTEZ;

                            popt_DeletePCode(j0);
                            nchanges++;
                          }
                        else if (word.sData == -1)
                          {
                            g_opPtr[j1]->op     = oLONGOP8;
                            g_opPtr[j1]->arg1   = oDINC;
                            g_opPtr[j1]->arg2   = 0;

                            g_opPtr[i]->arg1   = oDLTEZ;

                            popt_DeletePCode(j0);
                            nchanges++;
                          }
                        break;

                      /* Simplify or delete condition branches on constants */

                      case oDJEQUZ :
                        if (word.sData == 0)
                          {
                            g_opPtr[i]->op   = oJMP;
                            g_opPtr[i]->arg1 = 0;
                            popt_DeletePCodePair(j1, j0);
                            i++;
                          }
                        else
                          {
                            popt_DeletePCodeTrio(i, j1, j0);
                          }

                        nchanges++;
                        continue;

                      case oDJNEQZ :
                        if (word.sData != 0)
                          {
                            g_opPtr[i]->op   = oJMP;
                            g_opPtr[i]->arg1 = 0;
                            popt_DeletePCodePair(j1, j0);
                            i++;
                          }
                        else
                          {
                            popt_DeletePCodeTrio(i, j1, j0);
                          }

                        nchanges++;
                        continue;

                      case oDJLTZ  :
                        if (word.sData < 0)
                          {
                            g_opPtr[i]->op   = oJMP;
                            g_opPtr[i]->arg1 = 0;
                            popt_DeletePCodePair(j1, j0);
                            i++;
                          }
                        else
                          {
                            popt_DeletePCodeTrio(i, j1, j0);
                          }

                        nchanges++;
                        continue;

                      case oDJGTEZ :
                        if (word.sData >= 0)
                          {
                            g_opPtr[i]->op   = oJMP;
                            g_opPtr[i]->arg1 = 0;
                            popt_DeletePCodePair(j1, j0);
                            i++;
                          }
                        else
                          {
                            popt_DeletePCodeTrio(i, j1, j0);
                          }

                        nchanges++;
                        continue;

                      case oDJGTZ  :
                        if (word.sData > 0)
                          {
                            g_opPtr[i]->op   = oJMP;
                            g_opPtr[i]->arg1 = 0;
                            popt_DeletePCodePair(j1, j0);
                            i++;
                          }
                        else
                          {
                            popt_DeletePCodeTrio(i, j1, j0);
                          }

                        nchanges++;
                        continue;
                        break;

                      case oDJLTEZ :
                        if (word.sData <= 0)
                          {
                            g_opPtr[i]->op   = oJMP;
                            g_opPtr[i]->arg1 = 0;
                            popt_DeletePCodePair(j1, j0);
                            i++;
                          }
                        else
                          {
                            popt_DeletePCodeTrio(i, j1, j0);
                          }

                        nchanges++;
                        continue;

                      default:
                        break;
                    }
                }

              /* If the oPUSH instructions are still there, see if we can now
               * represent them with a oPUSHB or oUPUSHB instruction.
               */

              popt_OptimizePush(g_opPtr[j1]);
              popt_OptimizePush(g_opPtr[j0]);

              /* If the LONGOP8 instruction is still there, well will need to
               * increment the index over it.
               */

              if (g_opPtr[i] == NULL || g_opPtr[i]->op != oLONGOP8)
                {
                  i++;
                  continue;
                }
            }

          /* Perform optimizations on back-to-back Unary operators */

          if (g_opPtr[j1]->op == oLONGOP8)
            {
              /* Check for the effect of an INC reversed by a following DEC, and
               * vice versa.
               */

              if (g_opPtr[i]->arg1 == oDINC)
                {
                  if (g_opPtr[j1]->arg1 == oDDEC)
                    {
                      popt_DeletePCodePair(i, j1);
                      nchanges++;
                    }
                }
              else if (g_opPtr[i]->op == oDDEC)
                {
                  if (g_opPtr[j1]->op == oDINC)
                    {
                     popt_DeletePCodePair(i, i + 1);
                     nchanges++;
                    }
               }
            }
        }

      /* No long optiminzation -- try the next opcode */


      i++;
    }

   return nchanges;
}

/****************************************************************************/

int16_t popt_LongBinaryOptimize(void)
{
  int16_t nchanges = 0;
  int i;

  /* At least two pcodes are needed to perform any of the following binary
   * operator optimizations.
   */

  i = 1;
  while (i < g_nOpPtrs)
    {
      /* Check for a long operation. */

      if (g_opPtr[i]->op == oLONGOP8)
       {
          /* Check for a long operation on a constant (PUSHed) value.  All Binary
           * operators are oLONGOP8 types.
           *
           * All long operations with two 32-bit arguments, we need five opcodes.
           */

          if (i >= 4 &&
              (g_opPtr[i - 1]->op == oPUSH || g_opPtr[i - 1]->op == oPUSHB ||
               g_opPtr[i - 1]->op == oUPUSHB) &&
              (g_opPtr[i - 2]->op == oPUSH || g_opPtr[i - 2]->op == oPUSHB ||
               g_opPtr[i - 2]->op == oUPUSHB) &&
              (g_opPtr[i - 3]->op == oPUSH || g_opPtr[i - 3]->op == oPUSHB ||
               g_opPtr[i - 3]->op == oUPUSHB) &&
              (g_opPtr[i - 4]->op == oPUSH || g_opPtr[i - 4]->op == oPUSHB ||
               g_opPtr[i - 4]->op == oUPUSHB))
            {
              uWord_t wordJ;
              uWord_t wordK;

              int j1 = i - 1;
              int j0 = i - 2;
              int k1 = i - 3;
              int k0 = i - 4;

              /* Turn the oPUSHB or oUPUSHB instructions into an oPUSH
               * instructions (temporarily)
               */

              popt_ExpandPush(g_opPtr[j1]);
              popt_ExpandPush(g_opPtr[j0]);
              popt_ExpandPush(g_opPtr[k1]);
              popt_ExpandPush(g_opPtr[k0]);

              wordJ.word[0] = g_opPtr[j0]->arg2;
              wordJ.word[1] = g_opPtr[j1]->arg2;
              wordK.word[0] = g_opPtr[k0]->arg2;
              wordK.word[1] = g_opPtr[k1]->arg2;

              switch (g_opPtr[i]->arg1)
                {
                  case oDADD :
                    wordK.sData       += wordJ.sData;
                    g_opPtr[k0]->arg2  = wordK.word[0];
                    g_opPtr[k1]->arg2  = wordK.word[1];

                    popt_DeletePCodeTrio(i, j1, j0);
                    nchanges++;
                    i++;
                    continue;

                  case oDSUB :
                    wordK.sData       -= wordJ.sData;
                    g_opPtr[k0]->arg2  = wordK.word[0];
                    g_opPtr[k1]->arg2  = wordK.word[1];

                    popt_DeletePCodeTrio(i, j1, j0);
                    nchanges++;
                    i++;
                    continue;

                  case oDMUL :
                    wordK.sData       *= wordJ.sData;
                    g_opPtr[k0]->arg2  = wordK.word[0];
                    g_opPtr[k1]->arg2  = wordK.word[1];

                    popt_DeletePCodeTrio(i, j1, j0);
                    nchanges++;
                    i++;
                    continue;

                  case oDUMUL :
                    wordK.uData       *= wordJ.uData;
                    g_opPtr[k0]->arg2  = wordK.word[0];
                    g_opPtr[k1]->arg2  = wordK.word[1];

                    popt_DeletePCodeTrio(i, j1, j0);
                    nchanges++;
                    i++;
                    continue;

                  case oDDIV :
                    wordK.sData       /= wordJ.sData;
                    g_opPtr[k0]->arg2  = wordK.word[0];
                    g_opPtr[k1]->arg2  = wordK.word[1];

                    popt_DeletePCodeTrio(i, j1, j0);
                    nchanges++;
                    i++;
                    continue;

                  case oUDIV :
                    wordK.uData       /= wordJ.uData;
                    g_opPtr[k0]->arg2  = wordK.word[0];
                    g_opPtr[k1]->arg2  = wordK.word[1];

                    popt_DeletePCodeTrio(i, j1, j0);
                    nchanges++;
                    i++;
                    continue;

                  case oDMOD :
                    wordK.sData       %= wordJ.sData;
                    g_opPtr[k0]->arg2  = wordK.word[0];
                    g_opPtr[k1]->arg2  = wordK.word[1];

                    popt_DeletePCodeTrio(i, j1, j0);
                    nchanges++;
                    i++;
                    continue;

                  case oDUMOD :
                    wordK.uData       %= wordJ.uData;
                    g_opPtr[k0]->arg2  = wordK.word[0];
                    g_opPtr[k1]->arg2  = wordK.word[1];

                    popt_DeletePCodeTrio(i, j1, j0);
                    nchanges++;
                    i++;
                    continue;

                  case oDOR  :
                    wordK.uData       |= wordJ.uData;
                    g_opPtr[k0]->arg2  = wordK.word[0];
                    g_opPtr[k1]->arg2  = wordK.word[1];

                    popt_DeletePCodeTrio(i, j1, j0);
                    nchanges++;
                    i++;
                    continue;

                  case oDAND :
                    wordK.uData       &= wordJ.uData;
                    g_opPtr[k0]->arg2  = wordK.word[0];
                    g_opPtr[k1]->arg2  = wordK.word[1];

                    popt_DeletePCodeTrio(i, j1, j0);
                    nchanges++;
                    i++;
                    continue;

                  case oDEQU :
                    g_opPtr[i]->op   = oPUSH;
                    g_opPtr[i]->arg1 = 0;

                    if (wordK.uData == wordJ.uData)
                      {
                        g_opPtr[i]->arg2 = PASCAL_TRUE;
                      }
                    else
                      {
                        g_opPtr[i]->arg2 = 0;
                      }

                    popt_OptimizePush(g_opPtr[i]);
                    popt_DeletePCodeQuartet(j1, j0, k1, k0);
                    nchanges++;
                    continue;

                  case oDNEQ :
                    g_opPtr[i]->op   = oPUSH;
                    g_opPtr[i]->arg1 = 0;

                    if (wordK.uData != wordJ.uData)
                      {
                        g_opPtr[i]->arg2 = PASCAL_TRUE;
                      }
                    else
                      {
                        g_opPtr[i]->arg2 = 0;
                      }

                    popt_OptimizePush(g_opPtr[i]);
                    popt_DeletePCodeQuartet(j1, j0, k1, k0);
                    nchanges++;
                    continue;

                  case oDLT  :
                    g_opPtr[i]->op   = oPUSH;
                    g_opPtr[i]->arg1 = 0;

                    if (wordK.sData < wordJ.sData)
                      {
                        g_opPtr[i]->arg2 = PASCAL_TRUE;
                      }
                    else
                      {
                        g_opPtr[i]->arg2 = 0;
                      }

                    popt_OptimizePush(g_opPtr[i]);
                    popt_DeletePCodeQuartet(j1, j0, k1, k0);
                    nchanges++;
                    continue;

                  case oDULT :
                    g_opPtr[i]->op   = oPUSH;
                    g_opPtr[i]->arg1 = 0;

                    if (wordK.uData < wordJ.uData)
                      {
                        g_opPtr[i]->arg2 = PASCAL_TRUE;
                      }
                    else
                      {
                        g_opPtr[i]->arg2 = 0;
                      }

                    popt_OptimizePush(g_opPtr[i]);
                    popt_DeletePCodeQuartet(j1, j0, k1, k0);
                    nchanges++;
                    continue;

                  case oDGTE :
                    g_opPtr[i]->op   = oPUSH;
                    g_opPtr[i]->arg1 = 0;

                    if (wordK.sData >= wordJ.sData)
                      {
                        g_opPtr[i]->arg2 = PASCAL_TRUE;
                      }
                    else
                      {
                        g_opPtr[i]->arg2 = 0;
                      }

                    popt_OptimizePush(g_opPtr[i]);
                    popt_DeletePCodeQuartet(j1, j0, k1, k0);
                    nchanges++;
                    continue;

                  case oDUGTE :
                    g_opPtr[i]->op   = oPUSH;
                    g_opPtr[i]->arg1 = 0;

                    if (wordK.uData >= wordJ.uData)
                      {
                        g_opPtr[i]->arg2 = PASCAL_TRUE;
                      }
                    else
                      {
                        g_opPtr[i]->arg2 = 0;
                      }

                    popt_OptimizePush(g_opPtr[i]);
                    popt_DeletePCodeQuartet(j1, j0, k1, k0);
                    nchanges++;
                    continue;

                  case oDGT :
                    g_opPtr[i]->op   = oPUSH;
                    g_opPtr[i]->arg1 = 0;

                    if (wordK.sData > wordJ.sData)
                      {
                        g_opPtr[i]->arg2 = PASCAL_TRUE;
                      }
                    else
                      {
                        g_opPtr[i]->arg2 = 0;
                      }

                    popt_OptimizePush(g_opPtr[i]);
                    popt_DeletePCodeQuartet(j1, j0, k1, k0);
                    nchanges++;
                    continue;

                  case oDUGT :
                    g_opPtr[i]->op   = oPUSH;
                    g_opPtr[i]->arg1 = 0;

                    if (wordK.uData > wordJ.uData)
                      {
                        g_opPtr[i]->arg2 = PASCAL_TRUE;
                      }
                    else
                      {
                        g_opPtr[i]->arg2 = 0;
                      }

                    popt_OptimizePush(g_opPtr[i]);
                    popt_DeletePCodeQuartet(j1, j0, k1, k0);
                    nchanges++;
                    continue;

                  case oDLTE :
                    g_opPtr[i]->op   = oPUSH;
                    g_opPtr[i]->arg1 = 0;

                    if (wordK.sData <= wordJ.sData)
                      {
                        g_opPtr[i]->arg2 = PASCAL_TRUE;
                      }
                    else
                      {
                        g_opPtr[i]->arg2 = 0;
                      }

                    popt_OptimizePush(g_opPtr[i]);
                    popt_DeletePCodeQuartet(j1, j0, k1, k0);
                    nchanges++;
                    continue;

                  case oDULTE :
                    g_opPtr[i]->op   = oPUSH;
                    g_opPtr[i]->arg1 = 0;

                    if (wordK.uData <= wordJ.uData)
                      {
                        g_opPtr[i]->arg2 = PASCAL_TRUE;
                      }
                    else
                      {
                        g_opPtr[i]->arg2 = 0;
                      }

                    popt_OptimizePush(g_opPtr[i]);
                    popt_DeletePCodeQuartet(j1, j0, k1, k0);
                    nchanges++;
                    continue;
                }

              /* If the oPUSH instructions are still there, see if we can now
               * represent it with a oPUSHB or oUPUSHB instruction.
               */

              popt_OptimizePush(g_opPtr[j1]);
              popt_OptimizePush(g_opPtr[j0]);
              popt_OptimizePush(g_opPtr[k1]);
              popt_OptimizePush(g_opPtr[k0]);

              /* If the LONGOP8 instruction is still there, well will need to
               * increment the index over it.
               */

              if (g_opPtr[i] == NULL || g_opPtr[i]->op != oLONGOP8)
                {
                  i++;
                  continue;
                }
            }

          /* Shift operations have a 16-bit shift and a 32-bit value to be shifted. */

          if (i >= 3 &&
              (g_opPtr[i - 1]->op == oPUSH || g_opPtr[i - 1]->op == oPUSHB ||
               g_opPtr[i - 1]->op == oUPUSHB) &&
              (g_opPtr[i - 2]->op == oPUSH || g_opPtr[i - 2]->op == oPUSHB ||
               g_opPtr[i - 2]->op == oUPUSHB) &&
              (g_opPtr[i - 3]->op == oPUSH || g_opPtr[i - 3]->op == oPUSHB ||
               g_opPtr[i - 3]->op == oUPUSHB))
            {
              uWord_t wordK;

              int j  = i - 1;
              int k1 = i - 2;
              int k0 = i - 3;

              /* Turn the oPUSHB or oUPUSHB instructions into an oPUSH
               * instructions (temporarily)
               */

              popt_ExpandPush(g_opPtr[j]);
              popt_ExpandPush(g_opPtr[k1]);
              popt_ExpandPush(g_opPtr[k0]);

              wordK.word[0] = g_opPtr[k0]->arg2;
              wordK.word[1] = g_opPtr[k1]->arg2;

              switch (g_opPtr[i]->arg1)
                {
                  case oDSLL :
                    wordK.uData <<= g_opPtr[j]->arg2;
                    g_opPtr[k0]->arg2  = wordK.word[0];
                    g_opPtr[k1]->arg2  = wordK.word[1];

                    popt_DeletePCodePair(i, j);
                    nchanges++;
                    break;

                  case oDSRL :
                    wordK.uData >>= g_opPtr[j]->arg2;
                    g_opPtr[k0]->arg2  = wordK.word[0];
                    g_opPtr[k1]->arg2  = wordK.word[1];

                    popt_DeletePCodePair(i, j);
                    nchanges++;
                    break;

                  case oDSRA :
                    wordK.sData >>= g_opPtr[j]->arg2;
                    g_opPtr[k0]->arg2  = wordK.word[0];
                    g_opPtr[k1]->arg2  = wordK.word[1];

                    popt_DeletePCodePair(i, j);
                    nchanges++;
                    break;
                }

              /* If the oPUSH instructions are still there, see if we can now
               * represent it with a oPUSHB or oUPUSHB instruction.
               */

              popt_OptimizePush(g_opPtr[j]);
              popt_OptimizePush(g_opPtr[k1]);
              popt_OptimizePush(g_opPtr[k0]);

              /* If the LONGOP8 instruction is still there, well will need to
               * increment the index over it.
               */

              if (g_opPtr[i] == NULL || g_opPtr[i]->op != oLONGOP8)
                {
                  i++;
                  continue;
                }
            }
        }

      /* Misc improvements on binary operators */

      if (g_opPtr[i - 1]->op == oLONGOP8 && g_opPtr[i - 1]->op == oDNEG)
        {
          /* Negation followed by add is subtraction */

          if (g_opPtr[i]->op == oDADD)
            {
               g_opPtr[i]->arg1 = oDSUB;
               popt_DeletePCode(i - 1);
               nchanges++;
            }

          /* Negation followed by subtraction is addition */

          else if (g_opPtr[i]->op == oDSUB)
            {
               g_opPtr[i]->arg1 = oDADD;
               popt_DeletePCode(i - 1);
               nchanges++;
            }
        }

      i++;
    }

  return nchanges;
}
