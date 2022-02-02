/**********************************************************************
 * popt_constants.c
 * Constant Expression Optimizations
 *
 *   Copyright (C) 2008-2009, 2022 Gregory Nutt. All rights reserved.
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
#include "pas_machine.h"
#include "pinsn16.h"

#include "paslib.h"
#include "popt.h"
#include "popt_local.h"
#include "popt_constants.h"

/**********************************************************************/

int16_t popt_UnaryOptimize(void)
{
  int16_t nchanges = 0;
  register uint16_t temp;
  register int16_t i;

  TRACE(stderr, "[popt_UnaryOptimize]");

  /* At least two pcodes are need to perform unary optimizations */

   i = 0;
   while (i < g_nOpPtrs - 1)
     {
       /* Check for a constant value being pushed onto the stack */

       if (g_opPtr[i]->op == oPUSH || g_opPtr[i]->op == oPUSHB ||
           g_opPtr[i]->op == oUPUSHB)
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

           switch (g_opPtr[i + 1]->op)
             {
               /* Delete unary operators on constants */

             case oNEG   :
               g_opPtr[i]->arg2 = -(g_opPtr[i]->arg2);
               popt_DeletePCode(i + 1);
               nchanges++;
               break;

             case oABS   :
               if (signExtend16(g_opPtr[i]->arg2) < 0)
                 {
                   g_opPtr[i]->arg2 = -signExtend16(g_opPtr[i]->arg2);
                 }

               popt_DeletePCode(i + 1);
               nchanges++;
               break;

             case oINC   :
               (g_opPtr[i]->arg2)++;
               popt_DeletePCode(i + 1);
               nchanges++;
               break;

             case oDEC   :
               (g_opPtr[i]->arg2)--;
               popt_DeletePCode(i + 1);
               nchanges++;
               break;

             case oNOT   :
               g_opPtr[i]->arg2 = ~(g_opPtr[i]->arg2);
               popt_DeletePCode(i + 1);
               nchanges++;
               break;

               /* Simplify binary operations on constants */

             case oADD :
               if (g_opPtr[i]->arg2 == 0)
                 {
                   popt_DeletePCodePair(i, i + 1);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == 1)
                 {
                   g_opPtr[i + 1]->op = oINC;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == (uint16_t)-1)
                 {
                   g_opPtr[i + 1]->op = oDEC;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               else i++;
               break;

             case oSUB :
               if (g_opPtr[i]->arg2 == 0)
                 {
                   popt_DeletePCodePair(i, i + 1);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == 1)
                 {
                   g_opPtr[i + 1]->op = oDEC;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == (uint16_t)-1)
                 {
                   g_opPtr[i + 1]->op = oINC;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               else i++;
               break;

             case oMUL :
             case oUMUL :
             case oDIV :
             case oUDIV :
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
                   if (g_opPtr[i + 1]->op == oMUL ||
                       g_opPtr[i + 1]->op == oUMUL)
                     {
                       g_opPtr[i + 1]->op = oSLL;
                     }
                   else if (g_opPtr[i + 1]->op == oDIV)
                     {
                       g_opPtr[i + 1]->op = oSRA;
                     }
                   else /* if g_opPtr[i + 1]->op == oUDIV) */
                     {
                       g_opPtr[i + 1]->op = oSRL;
                     }

                   nchanges++;
                   i++;
                   break;

                 default :
                   i++;
                   break;
                 }
               break;

             case oSLL :
             case oSRL :
             case oSRA :
             case oOR  :
               if (g_opPtr[i]->arg2 == 0)
                 {
                   popt_DeletePCodePair(i, i + 1);
                   nchanges++;
                 }
               else i++;
               break;

             case oAND :
               if (g_opPtr[i]->arg2 == 0xffff)
                 {
                   popt_DeletePCodePair(i, i + 1);
                   nchanges++;
                 }
               else i++;
               break;

               /* Delete comparisons of constants to zero */

             case oEQUZ  :
               if (g_opPtr[i]->arg2 == 0) g_opPtr[i]->arg2 = -1;
               else g_opPtr[i]->arg2 = 0;
               popt_DeletePCode(i + 1);
               nchanges++;
               break;

             case oNEQZ  :
               if (g_opPtr[i]->arg2 != 0)
                 g_opPtr[i]->arg2 = -1;
               else
                 g_opPtr[i]->arg2 = 0;

               popt_DeletePCode(i + 1);
               nchanges++;
               break;

             case oLTZ   :
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

             case oGTEZ  :
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

             case oGTZ   :
               if (g_opPtr[i]->arg2 > 0) g_opPtr[i]->arg2 = -1;
               else g_opPtr[i]->arg2 = 0;

               popt_DeletePCode(i + 1);
               nchanges++;
               break;

             case oLTEZ :
               if (g_opPtr[i]->arg2 <= 0) g_opPtr[i]->arg2 = -1;
               else g_opPtr[i]->arg2 = 0;

               popt_DeletePCode(i + 1);
               nchanges++;
               break;

               /*  Simplify comparisons with certain constants */

             case oEQU   :
               if (g_opPtr[i]->arg2 == 0)
                 {
                   g_opPtr[i + 1]->op = oEQUZ;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == 1)
                 {
                   g_opPtr[i]->op     = oDEC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->op = oEQUZ;
                   nchanges++;
                 }
               else if (signExtend16(g_opPtr[i]->arg2) == -1)
                 {
                   g_opPtr[i]->op     = oINC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->op = oEQUZ;
                   nchanges++;
                 }
               else
                 {
                   i++;
                 }
               break;

             case oNEQ   :
               if (g_opPtr[i]->arg2 == 0)
                 {
                   g_opPtr[i + 1]->op = oNEQZ;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == 1)
                 {
                   g_opPtr[i]->op     = oDEC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->op = oNEQZ;
                   nchanges++;
                 }
               else if (signExtend16(g_opPtr[i]->arg2) == -1)
                 {
                   g_opPtr[i]->op     = oINC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->op = oNEQZ;
                   nchanges++;
                 }
               else
                 {
                   i++;
                 }
               break;

             case oLT    :
               if (g_opPtr[i]->arg2 == 0)
                 {
                   g_opPtr[i + 1]->op = oLTZ;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == 1)
                 {
                   g_opPtr[i]->op     = oDEC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->op = oLTZ;
                   nchanges++;
                 }
               else if (signExtend16(g_opPtr[i]->arg2) == -1)
                 {
                   g_opPtr[i]->op     = oINC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->op = oLTZ;
                   nchanges++;
                 }
               else
                 {
                   i++;
                 }
               break;

             case oGTE   :
               if (g_opPtr[i]->arg2 == 0)
                 {
                   g_opPtr[i + 1]->op = oGTEZ;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == 1)
                 {
                   g_opPtr[i]->op     = oDEC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->op = oGTEZ;
                   nchanges++;
                 }
               else if (signExtend16(g_opPtr[i]->arg2) == -1)
                 {
                   g_opPtr[i]->op     = oINC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->op = oGTEZ;
                   nchanges++;
                 }
               else
                 {
                   i++;
                 }
               break;

             case oGT    :
               if (g_opPtr[i]->arg2 == 0)
                 {
                   g_opPtr[i + 1]->op = oGTZ;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == 1)
                 {
                   g_opPtr[i]->op     = oDEC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->op = oGTZ;
                   nchanges++;
                 }
               else if (signExtend16(g_opPtr[i]->arg2) == -1)
                 {
                   g_opPtr[i]->op     = oINC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->op = oGTZ;
                   nchanges++;
                 }
               else
                 {
                   i++;
                 }
               break;

             case oLTE   :
               if (g_opPtr[i]->arg2 == 0)
                 {
                   g_opPtr[i + 1]->op = oLTEZ;
                   popt_DeletePCode(i);
                   nchanges++;
                 }
               else if (g_opPtr[i]->arg2 == 1)
                 {
                   g_opPtr[i]->op     = oDEC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->op = oLTEZ;
                   nchanges++;
                 }
               else if (signExtend16(g_opPtr[i]->arg2) == -1)
                 {
                   g_opPtr[i]->op     = oINC;
                   g_opPtr[i]->arg2   = 0;
                   g_opPtr[i + 1]->op = oLTEZ;
                   nchanges++;
                 }
               else
                 {
                   i++;
                 }
               break;

             /* Simplify or delete condition branches on constants */

             case oJEQUZ :
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

             case oJNEQZ :
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

             case oJLTZ  :
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

             case oJGTEZ :
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

             case oJGTZ  :
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

             case oJLTEZ :
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
               i++;
               break;
             }

           /* If the oPUSH instruction is still there, see if we can now
            * represent it with a oPUSHB or oUPUSHB instruction.
            */

           if (g_opPtr[i] != NULL && g_opPtr[i]->op == oPUSH)
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
         }

       /* Delete multiple modifications of DSEG pointer */

       else if (g_opPtr[i]->op == oINDS)
         {
           if (g_opPtr[i + 1]->op == oINDS)
             {
               g_opPtr[i]->arg2 += g_opPtr[i + 1]->arg2;
               popt_DeletePCode(i + 1);
               nchanges++;
             }
           else
             {
               i++;
             }
         }

       /* Check for the effect of an INC reversed by a following DEC, and
        * vice versa.
        */

       else if (g_opPtr[i]->op == oINC)
         {
           if (g_opPtr[i + 1]->op == oDEC)
             {
               popt_DeletePCodePair(i, i + 1);
               nchanges++;
             }
           else
             {
               i++;
             }
         }
       else if (g_opPtr[i]->op == oDEC)
         {
           if (g_opPtr[i + 1]->op == oINC)
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

   return nchanges;
}

/**********************************************************************/

int16_t popt_BinaryOptimize(void)
{
  int16_t nchanges = 0;
  register int16_t stmp16;
  register int16_t i;

  TRACE(stderr, "[popt_BinaryOptimize]");

  /* At least two pcodes are needed to perform the following binary
   * operator optimizations.
   */

  i = 0;
  while (i < g_nOpPtrs - 2)
    {
      if (g_opPtr[i]->op == oPUSH || g_opPtr[i]->op == oPUSHB ||
          g_opPtr[i]->op == oUPUSHB)
        {
          if (g_opPtr[i + 1]->op == oPUSH || g_opPtr[i + 1]->op == oPUSHB ||
              g_opPtr[i + 1]->op == oUPUSHB)
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

              if (g_opPtr[i + 1]->op == oPUSHB)
                {
                  g_opPtr[i + 1]->op   = oPUSH;
                  g_opPtr[i + 1]->arg2 = signExtend8(g_opPtr[i + 1]->arg1);
                  g_opPtr[i + 1]->arg1 = 0;
                }
              else if (g_opPtr[i + 1]->op == oUPUSHB)
                {
                  g_opPtr[i + 1]->op   = oPUSH;
                  g_opPtr[i + 1]->arg2 = g_opPtr[i + 1]->arg1;
                  g_opPtr[i + 1]->arg1 = 0;
                }

              switch (g_opPtr[i + 2]->op)
                {
                case oADD :
                  g_opPtr[i]->arg2 += g_opPtr[i + 1]->arg2;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oSUB :
                  g_opPtr[i]->arg2 -= g_opPtr[i + 1]->arg2;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oMUL :   /* REVISIT:  arg2 is unsigned */
                case oUMUL :
                  g_opPtr[i]->arg2 *= g_opPtr[i + 1]->arg2;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oDIV :
                  stmp16 = g_opPtr[i]->arg2 / signExtend16(g_opPtr[i + 1]->arg2);
                  g_opPtr[i]->arg2 = stmp16;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oMOD : /* REVISIT:  arg2 is unigned */
                case oUMOD :
                  g_opPtr[i]->arg2 %= g_opPtr[i + 1]->arg2;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oSLL :
                  g_opPtr[i]->arg2 <<= g_opPtr[i + 1]->arg2;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oSRL :
                  g_opPtr[i]->arg2 >>= g_opPtr[i + 1]->arg2;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oSRA :
                  stmp16 = (((int16_t)g_opPtr[i]->arg2) >> g_opPtr[i + 1]->arg2);
                  g_opPtr[i]->arg2 = (uint16_t)stmp16;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oOR  :
                  g_opPtr[i]->arg2 |= g_opPtr[i + 1]->arg2;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oAND :
                  g_opPtr[i]->arg2 &= g_opPtr[i + 1]->arg2;
                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oEQU :
                  if (g_opPtr[i]->arg2 == g_opPtr[i + 1]->arg2) g_opPtr[i]->arg2 = -1;
                  else g_opPtr[i]->arg2 = 0;

                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oNEQ :
                  if ((int16_t)g_opPtr[i]->arg2 != (int16_t)g_opPtr[i + 1]->arg2)
                    g_opPtr[i]->arg2 = -1;
                  else
                    g_opPtr[i]->arg2 = 0;

                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oLT  :  /* REVISIT:  arg2 is unsigned */
                case oULT :
                  if ((int16_t)g_opPtr[i]->arg2 < (int16_t)g_opPtr[i + 1]->arg2)
                    g_opPtr[i]->arg2 = -1;
                  else
                    g_opPtr[i]->arg2 = 0;

                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oGTE :  /* REVISIT:  arg2 is unsigned */
                case oUGTE :
                  if ((int16_t)g_opPtr[i]->arg2 >= (int16_t)g_opPtr[i + 1]->arg2)
                    g_opPtr[i]->arg2 = -1;
                  else
                    g_opPtr[i]->arg2 = 0;

                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oGT  :  /* REVISIT:  arg2 is unsigned */
                case oUGT :
                  if ((int16_t)g_opPtr[i]->arg2 > (int16_t)g_opPtr[i + 1]->arg2)
                    g_opPtr[i]->arg2 = -1;
                  else
                    g_opPtr[i]->arg2 = 0;

                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                case oLTE :  /* REVISIT:  arg2 is unsigned */
                case oULTE :
                  if ((int16_t)g_opPtr[i]->arg2 <= (int16_t)g_opPtr[i + 1]->arg2)
                    g_opPtr[i]->arg2 = -1;
                  else
                    g_opPtr[i]->arg2 = 0;

                  popt_DeletePCodePair(i + 1, i + 2);
                  nchanges++;
                  break;

                default   :
                  i++;
                  break;
                }

              /* If the oPUSH instruction(s) are still there, see if we can now
               * represent them with an oUPUSHB instructions
               */

              if (g_opPtr[i] != NULL && g_opPtr[i]->op == oPUSH)
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

              if (g_opPtr[i + 1] != NULL && g_opPtr[i + 1]->op == oPUSH)
                {
                  if ((int16_t)g_opPtr[i + 1]->arg2 >= MINSHORTINT &&
                      (int16_t)g_opPtr[i + 1]->arg2 <= MAXSHORTINT)
                    {
                      g_opPtr[i + 1]->op   = oPUSHB;
                      g_opPtr[i + 1]->arg1 = g_opPtr[i + 1]->arg2;
                      g_opPtr[i + 1]->arg2 = 0;
                    }
                  else if ((uint16_t)g_opPtr[i + 1]->arg2 <= MAXSHORTWORD)
                    {
                      g_opPtr[i + 1]->op   = oUPUSHB;
                      g_opPtr[i + 1]->arg1 = g_opPtr[i + 1]->arg2;
                      g_opPtr[i + 1]->arg2 = 0;
                    }
                }
            }

          /* A single (constant) pcode is sufficient to perform the
           * following binary operator optimizations.
           */

          else if (g_opPtr[i + 1]->op == oLDS   ||
                   g_opPtr[i + 1]->op == oLDSB  ||
                   g_opPtr[i + 1]->op == oULDSB ||
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
                case oADD :
                  if (g_opPtr[i]->arg2 == 0)
                    {
                      popt_DeletePCodePair(i, i + 2);
                      nchanges++;
                    }
                  else if (g_opPtr[i]->arg2 == 1)
                    {
                      g_opPtr[i + 2]->op = oINC;
                      popt_DeletePCode(i);
                      nchanges++;
                    }
                  else if (g_opPtr[i]->arg2 == (uint16_t)-1)
                    {
                      g_opPtr[i + 2]->op = oDEC;
                      popt_DeletePCode(i);
                      nchanges++;
                    }
                  else i++;
                  break;

                case oSUB :
                  if (g_opPtr[i]->arg2 == 0)
                    {
                      g_opPtr[i]->op = oNEG;
                      popt_DeletePCode(i);
                      nchanges++;
                    }
                  else i++;
                  break;

                case oMUL :
                case oUMUL :
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
                      g_opPtr[i + 2]->op   = oSLL;
                      nchanges++;
                      i++;
                      break;

                    default :
                      i++;
                      break;
                    }
                  break;

                case oOR  :
                  if (g_opPtr[i]->arg2 == 0)
                    {
                      popt_DeletePCodePair(i, i + 2);
                      nchanges++;
                    }
                  else i++;
                  break;

                case oAND :
                  if (g_opPtr[i]->arg2 == 0xffff)
                    {
                      popt_DeletePCodePair(i, i + 2);
                      nchanges++;
                    }
                  else i++;
                  break;

                case oEQU :
                  if (g_opPtr[i]->arg2 == 0)
                    {
                      g_opPtr[i + 2]->op = oEQUZ;
                      popt_DeletePCode(i);
                      nchanges++;
                    }
                  else i++;
                  break;

                case oNEQ :
                  if (g_opPtr[i]->arg2 == 0)
                    {
                      g_opPtr[i + 2]->op = oNEQZ;
                      popt_DeletePCode(i);
                      nchanges++;
                    }
                  else i++;
                  break;

                case oLT  :
                  if (g_opPtr[i]->arg2 == 0)
                    {
                      g_opPtr[i + 2]->op = oGTEZ;
                      popt_DeletePCode(i);
                      nchanges++;
                    }
                  else i++;
                  break;

                case oGTE :
                  if (g_opPtr[i]->arg2 == 0)
                    {
                      g_opPtr[i + 2]->op = oLTZ;
                      popt_DeletePCode(i);
                      nchanges++;
                    }
                  else i++;
                  break;

                case oGT  :
                  if (g_opPtr[i]->arg2 == 0)
                    {
                      g_opPtr[i + 2]->op = oLTEZ;
                      popt_DeletePCode(i);
                      nchanges++;
                    }
                  else i++;
                  break;

                case oLTE :
                  if (g_opPtr[i]->arg2 == 0)
                    {
                      g_opPtr[i + 2]->op = oGTZ;
                      popt_DeletePCode(i);
                      nchanges++;
                    }
                  else i++;
                  break;

                default   :
                  i++;
                  break;
                }

             /* If the oPUSH instruction is still there, see if we can now
              * represent it with a oPUSHB or oUPUSHB instruction.
              */

             if (g_opPtr[i] != NULL && g_opPtr[i]->op == oPUSH)
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
            }
          else i++;
        }

      /* Misc improvements on binary operators */

      else if (g_opPtr[i]->op == oNEG)
        {
          /* Negation followed by add is subtraction */

          if (g_opPtr[i + 1]->op == oADD)
            {
               g_opPtr[i + 1]->op = oSUB;
               popt_DeletePCode(i);
               nchanges++;
            }

          /* Negation followed by subtraction is addition */

          else if (g_opPtr[i]->op == oSUB)
            {
               g_opPtr[i + 1]->op = oADD;
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
