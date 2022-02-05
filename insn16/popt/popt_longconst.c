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

#include "paslib.h"
#include "popt.h"
#include "popt_local.h"
#include "popt_constants.h"

/****************************************************************************/

int16_t popt_LongUnaryOptimize(void)
{
  int16_t nchanges = 0;
  register uint16_t temp;
  register int16_t i;
  register int16_t j;
  register int16_t k;

  TRACE(stderr, "[popt_LongUnaryOptimize]");

  /* At least two pcodes are need to perform unary optimizations */

  i = 1;
  while (i < g_nOpPtrs)
    {
      /* Check for a long operation. */

      if (g_opPtr[i] == oLONGOP8)
       {
          j = i - 1;

          /* Check for a long operation on a constant (PUSHed) value.  All Unary
            * operators are LONGOP8 types.
            */

          if (g_opPtr[j]->op == oPUSH || g_opPtr[j]->op == oPUSHB ||
              g_opPtr[j]->op == oUPUSHB)
           {
              /* Turn the oPUSHB or oUPUSHB into an oPUSH op (temporarily) */

              if (g_opPtr[j]->op == oPUSHB)
                {
                  g_opPtr[j]->op   = oPUSH;
                  g_opPtr[j]->arg2 = signExtend8(g_opPtr[j]->arg1);
                  g_opPtr[j]->arg1 = 0;
                }
              else if (g_opPtr[j]->op == oUPUSHB)
                {
                  g_opPtr[j]->op   = oPUSH;
                  g_opPtr[j]->arg2 = g_opPtr[j]->arg1;
                  g_opPtr[j]->arg1 = 0;
                }

              /* With two opcodes, only the 16- to 32-bit conversion long
               * operations can be optimized.
               */

              /* Handle according to the specific long operation in arg1 */
LEFT OFF!!!
               switch (g_opPtr[i]->arg1)
                 {
                   case oCNVD :   /* Convert signed 16-bit value to signed long value */
                     break;

                   case oUCNVD :  /* Convert unsigned 16-bit value to unsigned long value */
                     break;

                   case oDCNV :   /* Convert 32-bit value to 16-bit value (signed or unsigned) */
                     break;
                 }

              /* Handle according to the specific long operation in arg1 */

               switch (g_opPtr[i]->arg1)
                 {
                   /* Delete unary operators on constants */
LEFT OFF!!!

                   case oDNEG   :
                   g_opPtr[i]->arg2 = -(g_opPtr[i]->arg2);
                   popt_DeletePCode(i + 1);
                   nchanges++;
                   break;

##############################################################################
LEFT OFF!!!

             case oDABS   :
               if (signExtend16(g_opPtr[i]->arg2) < 0)
                 {
                   g_opPtr[i]->arg2 = -signExtend16(g_opPtr[i]->arg2);
                 }

               popt_DeletePCode(i + 1);
               nchanges++;
               break;

             case oDINC   :
               (g_opPtr[i]->arg2)++;
               popt_DeletePCode(i + 1);
               nchanges++;
               break;

             case oDDEC   :
               (g_opPtr[i]->arg2)--;
               popt_DeletePCode(i + 1);
               nchanges++;
               break;

             case oDNOT   :
               g_opPtr[i]->arg2 = ~(g_opPtr[i]->arg2);
               popt_DeletePCode(i + 1);
               nchanges++;
               break;

               /* Simplify binary operations on constants */

             case oDADD :
               if (g_opPtr[i]->arg2 == 0)
                 {
                   popt_DeletePCodePair(i, i + 1);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == 1)
                 {
                   g_opPtr[i + 1]->arg1 = oDINC;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == (uint16_t)-1)
                 {
                   g_opPtr[i + 1]->arg1 = oDDEC;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               break;

             case oDSUB :
               if (g_opPtr[i]->arg2 == 0)
                 {
                   popt_DeletePCodePair(i, i + 1);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == 1)
                 {
                   g_opPtr[i + 1]->arg1 = oDDEC;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == (uint16_t)-1)
                 {
                   g_opPtr[i + 1]->arg1 = oDINC;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               break;

             case oDMUL :
             case oDUMUL :
             case oDDIV :
             case oDUDIV :
               temp = 0;
               switch (g_opPtr[i]->arg2)
                 {
                 case 1 :
                   popt_DeletePCodePair(i, i + 1);
                   nchanges++;
                   break;

                 case 16384 : temp++;
                 case  8192 : temp++;
                 case  4096 : temp++;
                 case  2048 : temp++;
                 case  1024 : temp++;
                 case   512 : temp++;
                 case   256 : temp++;
                 case   128 : temp++;
                 case    64 : temp++;
                 case    32 : temp++;
                 case    16 : temp++;
                 case     8 : temp++;
                 case     4 : temp++;
                 case     2 : temp++;
                   g_opPtr[i]->arg2 = temp;
                   if (g_opPtr[i + 1]->op == oDMUL ||
                       g_opPtr[i + 1]->op == oDUMUL)
                     {
                       g_opPtr[i + 1]->op = oSLL;
                     }
                   else if (g_opPtr[i + 1]->op == oDDIV)
                     {
                       g_opPtr[i + 1]->op = oSRA;
                     }
                   else /* if g_opPtr[i + 1]->op == oDUDIV) */
                     {
                       g_opPtr[i + 1]->op = oSRL;
                     }

                   nchanges++;
                   break;

                 default :
                   break;
                 }
               break;

             case oDSLL :
             case oDSRL :
             case oDSRA :
             case oDOR  :
               if (g_opPtr[i]->arg2 == 0)
                 {
                   popt_DeletePCodePair(i, i + 1);
                   nchanges++;
                 }
               break;

             case oDAND :
               if (g_opPtr[i]->arg2 == 0xffff)
                 {
                   popt_DeletePCodePair(i, i + 1);
                   nchanges++;
                 }
               break;

               /* Delete comparisons of constants to zero */

             case oDEQUZ  :
               if (g_opPtr[i]->arg2 == 0) g_opPtr[i]->arg2 = -1;
               else g_opPtr[i]->arg2 = 0;
               popt_DeletePCode(i + 1);
               nchanges++;
               break;

             case oDNEQZ  :
               if (g_opPtr[i]->arg2 != 0)
                 {
                   g_opPtr[i]->arg2 = -1;
                 }
               else
                 {
                   g_opPtr[i]->arg2 = 0;
                 }

               popt_DeletePCode(i + 1);
               nchanges++;
               break;

             case oDLTZ   :
               if (signExtend16(g_opPtr[i]->arg2) < 0)
                 {
                   g_opPtr[i]->arg2 = -1;
                 }
               else
                 {
                   g_opPtr[i]->arg2 = 0;
                 }

               popt_DeletePCode(i + 1);
               nchanges++;
               break;

             case oDGTEZ  :
               if (signExtend16(g_opPtr[i]->arg2) >= 0)
                 {
                   g_opPtr[i]->arg2 = -1;
                 }
               else
                 {
                   g_opPtr[i]->arg2 = 0;
                 }

               popt_DeletePCode(i + 1);
               nchanges++;
               break;

             case oDGTZ   :
               if (g_opPtr[i]->arg2 > 0) g_opPtr[i]->arg2 = -1;
               else g_opPtr[i]->arg2 = 0;

               popt_DeletePCode(i + 1);
               nchanges++;
               break;

             case oDLTEZ :
               if (g_opPtr[i]->arg2 <= 0) g_opPtr[i]->arg2 = -1;
               else g_opPtr[i]->arg2 = 0;

               popt_DeletePCode(i + 1);
               nchanges++;
               break;

               /*  Simplify comparisons with certain constants */

             case oDEQU   :
               if (g_opPtr[i]->arg2 == 0)
                 {
                   g_opPtr[i + 1]->arg1 = oDEQUZ;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == 1)
                 {
                   g_opPtr[i]->arg1     = oDDEC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->arg1 = oDEQUZ;
                   nchanges++;
                 }
               else if (signExtend16(g_opPtr[i]->arg2) == -1)
                 {
                   g_opPtr[i]->arg1     = oDINC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->arg1 = oDEQUZ;
                   nchanges++;
                 }
               break;

             case oDNEQ   :
               if (g_opPtr[i]->arg2 == 0)
                 {
                   g_opPtr[i + 1]->arg1 = oDNEQZ;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == 1)
                 {
                   g_opPtr[i]->arg1     = oDDEC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->arg1 = oDNEQZ;
                   nchanges++;
                 }
               else if (signExtend16(g_opPtr[i]->arg2) == -1)
                 {
                   g_opPtr[i]->arg1     = oDINC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->arg1 = oDNEQZ;
                   nchanges++;
                 }
               break;

             case oDLT    :
               if (g_opPtr[i]->arg2 == 0)
                 {
                   g_opPtr[i + 1]->arg1 = oDLTZ;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == 1)
                 {
                   g_opPtr[i]->arg1     = oDDEC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->arg1 = oDLTZ;
                   nchanges++;
                 }
               else if (signExtend16(g_opPtr[i]->arg2) == -1)
                 {
                   g_opPtr[i]->arg1     = oDINC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->arg1 = oDLTZ;
                   nchanges++;
                 }
               break;

             case oDGTE   :
               if (g_opPtr[i]->arg2 == 0)
                 {
                   g_opPtr[i + 1]->arg1 = oDGTEZ;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == 1)
                 {
                   g_opPtr[i]->arg1     = oDDEC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->arg1 = oDGTEZ;
                   nchanges++;
                 }
               else if (signExtend16(g_opPtr[i]->arg2) == -1)
                 {
                   g_opPtr[i]->arg1     = oDINC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->arg1 = oDGTEZ;
                   nchanges++;
                 }
               break;

             case oDGT    :
               if (g_opPtr[i]->arg2 == 0)
                 {
                   g_opPtr[i + 1]->op = oGTZ;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == 1)
                 {
                   g_opPtr[i]->arg1     = oDDEC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->arg1 = oDGTZ;
                   nchanges++;
                 }
               else if (signExtend16(g_opPtr[i]->arg2) == -1)
                 {
                   g_opPtr[i]->arg1     = oDINC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->arg1 = oDGTZ;
                   nchanges++;
                 }
               break;

             case oDLTE   :
               if (g_opPtr[i]->arg2 == 0)
                 {
                   g_opPtr[i + 1]->arg1 = oDLTEZ;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == 1)
                 {
                   g_opPtr[i]->arg1     = oDDEC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->arg1 = oDLTEZ;
                   nchanges++;
                 }
               else if (signExtend16(g_opPtr[i]->arg2) == -1)
                 {
                   g_opPtr[i]->arg1     = oDINC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->arg1 = oDLTEZ;
                   nchanges++;
                 }
               break;

             /* Simplify or delete condition branches on constants */

             case oDJEQUZ :
               if (g_opPtr[i]->arg2 == 0)
                 {
                   g_opPtr[i + 1]->op = oJMP;
                   popt_DeletePCode(i);
                 }
               else
                 {
                   popt_DeletePCodePair(i, i + 1);
                 }

               nchanges++;
               break;

             case oDJNEQZ :
               if (g_opPtr[i]->arg2 != 0)
                 {
                   g_opPtr[i + 1]->op = oJMP;
                   popt_DeletePCode(i);
                 }
               else
                 {
                   popt_DeletePCodePair(i, i + 1);
                 }

               nchanges++;
               break;

             case oDJLTZ  :
               if (signExtend16(g_opPtr[i]->arg2) < 0)
                 {
                   g_opPtr[i + 1]->op = oJMP;
                   popt_DeletePCode(i);
                 }
               else
                 {
                   popt_DeletePCodePair(i, i + 1);
                 }

               nchanges++;
               break;

             case oDJGTEZ :
               if (signExtend16(g_opPtr[i]->arg2) >= 0)
                 {
                   g_opPtr[i + 1]->op = oJMP;
                   popt_DeletePCode(i);
                 }
               else
                 {
                   popt_DeletePCodePair(i, i + 1);
                 }

               nchanges++;
               break;

             case oDJGTZ  :
               if (g_opPtr[i]->arg2 > 0)
                 {
                   g_opPtr[i + 1]->op = oJMP;
                   popt_DeletePCode(i);
                 }
               else
                 {
                   popt_DeletePCodePair(i, i + 1);
                 }

               nchanges++;
               break;

             case oDJLTEZ :
               if (g_opPtr[i]->arg2 <= 0)
                 {
                   g_opPtr[i + 1]->op = oJMP;
                   popt_DeletePCode(i);
                 }
               else
                 {
                   popt_DeletePCodePair(i, i + 1);
                 }

               nchanges++;
               break;

             default     :
               break;
             }

           /* If the oPUSH instruction is still there, well will need to
            * increment the index over it.
            */

           if (g_opPtr[i] != NULL)
             {
               /* If the oPUSH instruction is still there, see if we can now
                * represent it with a oPUSHB or oUPUSHB instruction.
                */

               if (g_opPtr[i]->op == oPUSH)
                 {
                   if ((int16_t)g_opPtr[i]->arg2 >= MINSHORTINT &&
                       (int16_t)g_opPtr[i]->arg2 <= MAXSHORTINT)
                     {
                       g_opPtr[i]->op   = oPUSHB;
                       g_opPtr[i]->arg1 = g_opPtr[i]->arg2;
                       g_opPtr[i]->arg2 = 0;
                     }
                   else if ((uint16_t)g_opPtr[i]->arg2 <= MAXSHORTWORD)
                     {
                       g_opPtr[i]->op   = oUPUSHB;
                       g_opPtr[i]->arg1 = g_opPtr[i]->arg2;
                       g_opPtr[i]->arg2 = 0;
                     }
                 }

               i++;
             }
         }
##############################################################################
                }
            }
LEFT OFF!!!

          /* Perform optimizations on back-to-back Unary operators */

          else if (g_opPtr[i] == oLONGOP8)
           {
              /* Check for the effect of an INC reversed by a following DEC, and
               * vice versa.
               */

              if (g_opPtr[i]->op == oDINC)
                {
                  if (g_opPtr[i + 1]->op == oDDEC)
                    {
                      popt_DeletePCodePair(i, i + 1);
                      nchanges++;
                    }
                  else
                    {
                      i++;
                    }
                }
              else if (g_opPtr[i]->op == oDDEC)
                {
                  if (g_opPtr[i + 1]->op == oDINC)
                    {
                     popt_DeletePCodePair(i, i + 1);
                     nchanges++;
                    }
                  else
                   {
                     i++;
                   }
               }
             else
               {
                 i++;
               }
            }
          else
            {
              i++
            }
        }
     }

   return nchanges;
}

/****************************************************************************/

int16_t popt_LongBinaryOptimize(void)
{
  int16_t nchanges = 0;
  register int16_t stmp16;
  register int16_t i;
  register int16_t j;
  register int16_t k;

  TRACE(stderr, "[popt_LongBinaryOptimize]");

  /* At least three pcodes are needed to perform the following binary
   * operator optimizations.
   */

  i = 2;
  while (i < g_nOpPtrs)
    {
      j = i - 1;
      k = i - 2;

      /* Check for a long operation. */

      if (g_opPtr[i] == oLONGOP8)
       {
          /* Check for a long operation on a constant (PUSHed) value.  All Binary
            * operators are LONGOP8 types.
            */

          if ((g_opPtr[j]->op == oPUSH || g_opPtr[j]->op == oPUSHB ||
               g_opPtr[j]->op == oUPUSHB) &&
              (g_opPtr[k]->op == oPUSH || g_opPtr[k]->op == oPUSHB ||
               g_opPtr[k]->op == oUPUSHB) &&

          /* Turn the oPUSHB or oUPUSHB instructions into an oPUSH
           * instructions (temporarily)
           */

          if (g_opPtr[j]->op == oPUSHB)
            {
              g_opPtr[j]->op   = oPUSH;
              g_opPtr[j]->arg2 = signExtend8(g_opPtr[j]->arg1);
              g_opPtr[j]->arg1 = 0;
            }
          else if (g_opPtr[j]->op == oUPUSHB)
            {
              g_opPtr[j]->op   = oPUSH;
              g_opPtr[j]->arg2 = g_opPtr[j]->arg1;
              g_opPtr[j]->arg1 = 0;
            }

          if (g_opPtr[k]->op == oPUSHB)
            {
              g_opPtr[k]->op   = oPUSH;
              g_opPtr[k]->arg2 = signExtend8(g_opPtr[k]->arg1);
              g_opPtr[k]->arg1 = 0;
            }
          else if (g_opPtr[k]->op == oUPUSHB)
            {
              g_opPtr[k]->op   = oPUSH;
              g_opPtr[k]->arg2 = g_opPtr[k]->arg1;
              g_opPtr[k]->arg1 = 0;
            }
LEFT OFF!!!
##############################################################################

              switch (g_opPtr[i + 2]->op)
                {
                case oDADD :
                  g_opPtr[i]->arg2 += g_opPtr[i + 1]->arg2;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oDSUB :
                  g_opPtr[i]->arg2 -= g_opPtr[i + 1]->arg2;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oDMUL :   /* REVISIT:  arg2 is unsigned */
                case oDUMUL :
                  g_opPtr[i]->arg2 *= g_opPtr[i + 1]->arg2;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oDDIV :
                  stmp16 = g_opPtr[i]->arg2 / signExtend16(g_opPtr[i + 1]->arg2);
                  g_opPtr[i]->arg2 = stmp16;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oDMOD : /* REVISIT:  arg2 is unigned */
                case oDUMOD :
                  g_opPtr[i]->arg2 %= g_opPtr[i + 1]->arg2;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oDSLL :
                  g_opPtr[i]->arg2 <<= g_opPtr[i + 1]->arg2;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oDSRL :
                  g_opPtr[i]->arg2 >>= g_opPtr[i + 1]->arg2;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oDSRA :
                  stmp16 = (((int16_t)g_opPtr[i]->arg2) >> g_opPtr[i + 1]->arg2);
                  g_opPtr[i]->arg2 = (uint16_t)stmp16;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oDOR  :
                  g_opPtr[i]->arg2 |= g_opPtr[i + 1]->arg2;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oDAND :
                  g_opPtr[i]->arg2 &= g_opPtr[i + 1]->arg2;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oDEQU :
                  if (g_opPtr[i]->arg2 == g_opPtr[i + 1]->arg2)
                    {
                      g_opPtr[i]->arg2 = -1;
                    }
                  else
                    {
                      g_opPtr[i]->arg2 = 0;
                    }

                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oDNEQ :
                  if ((int16_t)g_opPtr[i]->arg2 != (int16_t)g_opPtr[i + 1]->arg2)
                    {
                      g_opPtr[i]->arg2 = -1;
                    }
                  else
                    {
                      g_opPtr[i]->arg2 = 0;
                    }

                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oDLT  :  /* REVISIT:  arg2 is unsigned */
                case oDULT :
                  if ((int16_t)g_opPtr[i]->arg2 < (int16_t)g_opPtr[i + 1]->arg2)
                    {
                      g_opPtr[i]->arg2 = -1;
                    }
                  else
                    {
                      g_opPtr[i]->arg2 = 0;
                    }

                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oDGTE :  /* REVISIT:  arg2 is unsigned */
                case oDUGTE :
                  if ((int16_t)g_opPtr[i]->arg2 >= (int16_t)g_opPtr[i + 1]->arg2)
                    {
                      g_opPtr[i]->arg2 = -1;
                    }
                  else
                    {
                      g_opPtr[i]->arg2 = 0;
                    }

                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oDGT  :  /* REVISIT:  arg2 is unsigned */
                case oDUGT :
                  if ((int16_t)g_opPtr[i]->arg2 > (int16_t)g_opPtr[i + 1]->arg2)
                    {
                      g_opPtr[i]->arg2 = -1;
                    }
                  else
                    {
                      g_opPtr[i]->arg2 = 0;
                    }

                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oDLTE :  /* REVISIT:  arg2 is unsigned */
                case oDULTE :
                  if ((int16_t)g_opPtr[i]->arg2 <= (int16_t)g_opPtr[i + 1]->arg2)
                    {
                      g_opPtr[i]->arg2 = -1;
                    }
                  else
                    {
                      g_opPtr[i]->arg2 = 0;
                    }

                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                default   :
                  break;
                }

              /* If the instruction(s) are still there, increment the index. */

              j = i + 1;
              if (g_opPtr[i] != NULL)
                {
                  /* If the oPUSH instruction(s) are still there, see if we can
                   * now represent them with an oUPUSHB instructions
                   */

                  if (g_opPtr[i]->op == oPUSH)
                    {
                      if ((int16_t)g_opPtr[i]->arg2 >= MINSHORTINT &&
                          (int16_t)g_opPtr[i]->arg2 <= MAXSHORTINT)
                        {
                          g_opPtr[i]->op   = oPUSHB;
                          g_opPtr[i]->arg1 = g_opPtr[i]->arg2;
                          g_opPtr[i]->arg2 = 0;
                        }
                      else if ((uint16_t)g_opPtr[i]->arg2 <= MAXSHORTWORD)
                        {
                          g_opPtr[i]->op   = oUPUSHB;
                          g_opPtr[i]->arg1 = g_opPtr[i]->arg2;
                          g_opPtr[i]->arg2 = 0;
                        }
                    }

                  i++;
                }

              if (g_opPtr[j] != NULL && g_opPtr[j]->op == oPUSH)
                {
                  if ((int16_t)g_opPtr[j]->arg2 >= MINSHORTINT &&
                      (int16_t)g_opPtr[j]->arg2 <= MAXSHORTINT)
                    {
                      g_opPtr[j]->op   = oPUSHB;
                      g_opPtr[j]->arg1 = g_opPtr[j]->arg2;
                      g_opPtr[j]->arg2 = 0;
                    }
                  else if ((uint16_t)g_opPtr[j]->arg2 <= MAXSHORTWORD)
                    {
                      g_opPtr[j]->op   = oUPUSHB;
                      g_opPtr[j]->arg1 = g_opPtr[j]->arg2;
                      g_opPtr[j]->arg2 = 0;
                    }
                }
            }

          /* A single (constant) pcode is sufficient to perform the
           * following binary operator optimizations.
           */

          else if (g_opPtr[i + 1]->op == oDLDS   ||
                   g_opPtr[i + 1]->op == oDLDSB  ||
                   g_opPtr[i + 1]->op == oDULDSB ||
                   g_opPtr[i + 1]->op == oLAS   ||
                   g_opPtr[i + 1]->op == oLAC)
            {
              /* Turn the oPUSHB or oUPUSHB into an oPUSH op (temporarily) */

              if (g_opPtr[i]->op == oPUSHB)
                {
                  g_opPtr[i]->op   = oPUSH;
                  g_opPtr[i]->arg2 = signExtend8(g_opPtr[i]->arg1);
                  g_opPtr[i]->arg1 = 0;
                }
              else if (g_opPtr[i]->op == oUPUSHB)
                {
                  g_opPtr[i]->op   = oPUSH;
                  g_opPtr[i]->arg2 = g_opPtr[i]->arg1;
                  g_opPtr[i]->arg1 = 0;
                }

              switch (g_opPtr[i + 2]->op)
                {
                case oDADD :
                  if (g_opPtr[i]->arg2 == 0)
                    {
                      popt_DeletePCodePair(i, i + 2);
                      nchanges++;
                    }
                  else if (g_opPtr[i]->arg2 == 1)
                    {
                      g_opPtr[i + 2]->arg1 = oDINC;
                      popt_DeletePCode(i);
                      nchanges++;
                    }
                  else if (g_opPtr[i]->arg2 == (uint16_t)-1)
                    {
                      g_opPtr[i + 2]->arg1 = oDDEC;
                      popt_DeletePCode(i);
                      nchanges++;
                    }
                  break;

                case oDSUB :
                  if (g_opPtr[i]->arg2 == 0)
                    {
                      g_opPtr[i]->arg1 = oDNEG;
                      popt_DeletePCode(i);
                      nchanges++;
                    }
                  break;

                case oDMUL :
                case oDUMUL :
                  stmp16 = 0;
                  switch (g_opPtr[i]->arg2)
                    {
                    case 1 :
                      popt_DeletePCodePair(i, i + 2);
                      nchanges++;
                      break;

                    case 16384 : stmp16++;
                    case  8192 : stmp16++;
                    case  4096 : stmp16++;
                    case  2048 : stmp16++;
                    case  1024 : stmp16++;
                    case   512 : stmp16++;
                    case   256 : stmp16++;
                    case   128 : stmp16++;
                    case    64 : stmp16++;
                    case    32 : stmp16++;
                    case    16 : stmp16++;
                    case     8 : stmp16++;
                    case     4 : stmp16++;
                    case     2 : stmp16++;
                      g_opPtr[i]->op       = g_opPtr[i + 1]->op;
                      g_opPtr[i]->arg1     = g_opPtr[i + 1]->arg1;
                      g_opPtr[i]->arg2     = g_opPtr[i + 1]->arg2;
                      g_opPtr[i + 1]->op   = oPUSH;
                      g_opPtr[i + 1]->arg1 = 0;
                      g_opPtr[i + 1]->arg2 = stmp16;
                      g_opPtr[i + 2]->arg1 = oDSLL;
                      nchanges++;
                      break;

                    default :
                      break;
                    }
                  break;

                case oDOR  :
                  if (g_opPtr[i]->arg2 == 0)
                    {
                      popt_DeletePCodePair(i, i + 2);
                      nchanges++;
                    }
                  break;

                case oDAND :
                  if (g_opPtr[i]->arg2 == 0xffff)
                    {
                      popt_DeletePCodePair(i, i + 2);
                      nchanges++;
                    }
                  else i++;
                  break;

                case oDEQU :
                  if (g_opPtr[i]->arg2 == 0)
                    {
                      g_opPtr[i + 2]->arg1 = oDEQUZ;
                      popt_DeletePCode(i);
                      nchanges++;
                    }
                  else i++;
                  break;

                case oDNEQ :
                  if (g_opPtr[i]->arg2 == 0)
                    {
                      g_opPtr[i + 2]->arg1 = oDNEQZ;
                      popt_DeletePCode(i);
                      nchanges++;
                    }
                  else i++;
                  break;

                case oDLT  :
                  if (g_opPtr[i]->arg2 == 0)
                    {
                      g_opPtr[i + 2]->arg1 = oDGTEZ;
                      popt_DeletePCode(i);
                      nchanges++;
                    }
                  break;

                case oDGTE :
                  if (g_opPtr[i]->arg2 == 0)
                    {
                      g_opPtr[i + 2]->arg1 = oDLTZ;
                      popt_DeletePCode(i);
                      nchanges++;
                    }
                  else i++;
                  break;

                case oDGT  :
                  if (g_opPtr[i]->arg2 == 0)
                    {
                      g_opPtr[i + 2]->arg1 = oDLTEZ;
                      popt_DeletePCode(i);
                      nchanges++;
                    }
                  break;

                case oDLTE :
                  if (g_opPtr[i]->arg2 == 0)
                    {
                      g_opPtr[i + 2]->arg1 = oDGTZ;
                      popt_DeletePCode(i);
                      nchanges++;
                    }
                  break;

                default   :
                  break;
                }

             /* If the instructions was not deleted, then we need to increment
              * the index.
              */

             if (g_opPtr[i] != NULL)
               {
                 /* If the oPUSH instruction is still there, see if we can now
                  * represent it with a oPUSHB or oUPUSHB instruction.
                  */

                 if (g_opPtr[i]->op == oPUSH)
                   {
                     if ((int16_t)g_opPtr[i]->arg2 >= MINSHORTINT &&
                         (int16_t)g_opPtr[i]->arg2 <= MAXSHORTINT)
                       {
                         g_opPtr[i]->op   = oPUSHB;
                         g_opPtr[i]->arg1 = g_opPtr[i]->arg2;
                         g_opPtr[i]->arg2 = 0;
                       }
                     else if ((uint16_t)g_opPtr[i]->arg2 <= MAXSHORTWORD)
                       {
                         g_opPtr[i]->op   = oUPUSHB;
                         g_opPtr[i]->arg1 = g_opPtr[i]->arg2;
                         g_opPtr[i]->arg2 = 0;
                       }
                   }

                 i++;
               }
            }
          else
            {
              i++;
            }
        }

      /* Misc improvements on binary operators */

      else if (g_opPtr[i]->op == oDNEG)
        {
          /* Negation followed by add is subtraction */

          if (g_opPtr[i + 1]->op == oDADD)
            {
               g_opPtr[i + 1]->arg1 = oDSUB;
               popt_DeletePCode(i);
               nchanges++;
            }

          /* Negation followed by subtraction is addition */

          else if (g_opPtr[i]->op == oDSUB)
            {
               g_opPtr[i + 1]->arg1 = oDADD;
               popt_DeletePCode(i);
               nchanges++;
            }
          else
            {
              i++;
            }
        }
      else
        {
          i++;
        }
    }

  return nchanges;
}
