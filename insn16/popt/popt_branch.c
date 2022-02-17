/**********************************************************************
 *  popt_branch.c
 *  Branch Optimizations
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
#include "pofflib.h"
#include "insn16.h"

#include "popt.h"
#include "popt_peephole.h"
#include "popt_local.h"

/**********************************************************************/

int16_t popt_BranchOptimize (void)
{
  int16_t nchanges = 0;
  register int16_t i;

  TRACE(stderr, "[popt_BranchOptimize]");

  /* At least two pcodes are need to perform branch optimizations */

  i = 0;
  while (i < g_nOpPtrs - 1)
    {
      switch (g_opPtr[i]->op)
        {
        case oNOT :
          switch (g_opPtr[i + 1]->op)
            {
            case oJEQUZ :
              g_opPtr[i + 1]->op = oJNEQZ;
              popt_DeletePCode(i);
              nchanges++;
              break;

            case oJNEQZ :
              g_opPtr[i + 1]->op = oJEQUZ;
              popt_DeletePCode(i);
              nchanges++;
              break;

            default     :
              i++;
              break;
            }
          break;

        case oNEG :
          switch (g_opPtr[i + 1]->op)
            {
            case oJLTZ  :
              g_opPtr[i + 1]->op = oJGTZ;
              popt_DeletePCode(i);
              nchanges++;
              break;

            case oJGTEZ :
              g_opPtr[i + 1]->op = oJLTEZ;
              popt_DeletePCode(i);
              nchanges++;
              break;

            case oJGTZ  :
              g_opPtr[i + 1]->op = oJLTZ;
              popt_DeletePCode(i);
              nchanges++;
              break;

            case oJLTEZ :
              g_opPtr[i + 1]->op = oJGTEZ;
              popt_DeletePCode(i);
              nchanges++;
              break;

            default     :
              i++;
              break;
            }
          break;

        case oEQU  :
          switch (g_opPtr[i + 1]->op)
            {
            case oNOT :
              g_opPtr[i]->op = oNEQ;
              popt_DeletePCode(i + 1);
              nchanges++;
              break;

            case oJEQUZ :
              g_opPtr[i + 1]->op = oJNEQ;
              popt_DeletePCode(i);
              nchanges++;
              break;

            case oJNEQZ :
              g_opPtr[i + 1]->op = oJEQU;
              popt_DeletePCode(i);
              nchanges++;
              break;

            default     :
              i++;
              break;
            }
          break;

        case oNEQ  :
          switch (g_opPtr[i + 1]->op)
            {
            case oNOT :
              g_opPtr[i]->op = oEQU;
              popt_DeletePCode(i + 1);
              nchanges++;
              break;

            case oJEQUZ :
              g_opPtr[i + 1]->op = oJEQU;
              popt_DeletePCode(i);
              nchanges++;
              break;

            case oJNEQZ :
              g_opPtr[i + 1]->op = oJNEQ;
              popt_DeletePCode(i);
              nchanges++;
              break;

            default     :
              i++;
              break;
            }
          break;

        case oLT   :
          switch (g_opPtr[i + 1]->op)
            {
            case oNOT :
              g_opPtr[i]->op = oGTE;
              popt_DeletePCode(i + 1);
              nchanges++;
              break;

            case oJEQUZ :
              g_opPtr[i + 1]->op = oJGTE;
              popt_DeletePCode(i);
              nchanges++;
              break;

            case oJNEQZ :
              g_opPtr[i + 1]->op = oJLT;
              popt_DeletePCode(i);
              nchanges++;
              break;

            default     :
              i++;
              break;
            }
          break;

        case oGTE  :
          switch (g_opPtr[i + 1]->op)
            {
            case oNOT :
              g_opPtr[i]->op = oLT;
              popt_DeletePCode(i + 1);
              nchanges++;
              break;

            case oJEQUZ :
              g_opPtr[i + 1]->op = oJLT;
              popt_DeletePCode(i);
              nchanges++;
              break;

            case oJNEQZ :
              g_opPtr[i + 1]->op = oJGTE;
              popt_DeletePCode(i);
              nchanges++;
              break;

            default     :
              i++;
              break;
            }
          break;

        case oGT   :
          switch (g_opPtr[i + 1]->op)
            {
            case oNOT :
              g_opPtr[i]->op = oLTE;
              popt_DeletePCode(i + 1);
              nchanges++;
              break;

            case oJEQUZ :
              g_opPtr[i + 1]->op = oJLTE;
              popt_DeletePCode(i);
              nchanges++;
              break;

            case oJNEQZ :
              g_opPtr[i + 1]->op = oJGT;
              popt_DeletePCode(i);
              nchanges++;
              break;

            default     :
              i++;
              break;
            }
          break;

        case oLTE :
          switch (g_opPtr[i + 1]->op)
            {
            case oNOT :
              g_opPtr[i]->op = oGT;
              popt_DeletePCode(i + 1);
              nchanges++;
              break;

            case oJEQUZ :
              g_opPtr[i + 1]->op = oJGT;
              popt_DeletePCode(i);
              nchanges++;
              break;

            case oJNEQZ :
              g_opPtr[i + 1]->op = oJLTE;
              popt_DeletePCode(i);
              nchanges++;
              break;

            default     :
              i++;
              break;
            }
          break;

        case oULT   :
          switch (g_opPtr[i + 1]->op)
            {
            case oNOT :
              g_opPtr[i]->op = oUGTE;
              popt_DeletePCode(i + 1);
              nchanges++;
              break;

            case oJEQUZ :
              g_opPtr[i + 1]->op = oJUGTE;
              popt_DeletePCode(i);
              nchanges++;
              break;

            case oJNEQZ :
              g_opPtr[i + 1]->op = oJULT;
              popt_DeletePCode(i);
              nchanges++;
              break;

            default     :
              i++;
              break;
            }
          break;

        case oUGTE  :
          switch (g_opPtr[i + 1]->op)
            {
            case oNOT :
              g_opPtr[i]->op = oULT;
              popt_DeletePCode(i + 1);
              nchanges++;
              break;

            case oJEQUZ :
              g_opPtr[i + 1]->op = oJULT;
              popt_DeletePCode(i);
              nchanges++;
              break;

            case oJNEQZ :
              g_opPtr[i + 1]->op = oJUGTE;
              popt_DeletePCode(i);
              nchanges++;
              break;

            default     :
              i++;
              break;
            }
          break;

        case oUGT   :
          switch (g_opPtr[i + 1]->op)
            {
            case oNOT :
              g_opPtr[i]->op = oULTE;
              popt_DeletePCode(i + 1);
              nchanges++;
              break;

            case oJEQUZ :
              g_opPtr[i + 1]->op = oJULTE;
              popt_DeletePCode(i);
              nchanges++;
              break;

            case oJNEQZ :
              g_opPtr[i + 1]->op = oJUGT;
              popt_DeletePCode(i);
              nchanges++;
              break;

            default     :
              i++;
              break;
            }
          break;

        case oULTE :
          switch (g_opPtr[i + 1]->op)
            {
            case oNOT :
              g_opPtr[i]->op = oUGT;
              popt_DeletePCode(i + 1);
              nchanges++;
              break;

            case oJEQUZ :
              g_opPtr[i + 1]->op = oJUGT;
              popt_DeletePCode(i);
              nchanges++;
              break;

            case oJNEQZ :
              g_opPtr[i + 1]->op = oJULTE;
              popt_DeletePCode(i);
              nchanges++;
              break;

            default     :
              i++;
              break;
            }
          break;

        case oEQUZ  :
          switch (g_opPtr[i + 1]->op)
            {
            case oNOT :
              g_opPtr[i]->op = oNEQZ;
              popt_DeletePCode(i + 1);
              nchanges++;
              break;

            case oJEQUZ :
              g_opPtr[i + 1]->op = oJNEQZ;
              popt_DeletePCode(i);
              nchanges++;
              break;

            case oJNEQZ :
              g_opPtr[i + 1]->op = oJEQUZ;
              popt_DeletePCode(i);
              nchanges++;
              break;

            default     :
              i++;
              break;
            }
          break;

        case oNEQZ  :
          switch (g_opPtr[i + 1]->op)
            {
            case oNOT :
              g_opPtr[i]->op = oEQUZ;
              popt_DeletePCode(i + 1);
              nchanges++;
              break;

            case oJEQUZ :
            case oJNEQZ :
              popt_DeletePCode(i);
              nchanges++;
              break;

            default     :
              i++;
              break;
            }
          break;

        case oLTZ   :
          switch (g_opPtr[i + 1]->op)
            {
            case oNOT :
              g_opPtr[i]->op = oGTEZ;
              popt_DeletePCode(i + 1);
              nchanges++;
              break;

            case oJEQUZ :
              g_opPtr[i + 1]->op = oJGTEZ;
              popt_DeletePCode(i);
              nchanges++;
              break;

            case oJNEQZ :
              g_opPtr[i + 1]->op = oJLTZ;
              popt_DeletePCode(i);
              nchanges++;
              break;

            default     :
              i++;
              break;
            }
          break;

        case oGTEZ  :
          switch (g_opPtr[i + 1]->op)
            {
            case oNOT :
              g_opPtr[i]->op = oLTZ;
              popt_DeletePCode(i + 1);
              nchanges++;
              break;

            case oJEQUZ :
              g_opPtr[i + 1]->op = oJLTZ;
              popt_DeletePCode(i);
              nchanges++;
              break;

            case oJNEQZ :
              g_opPtr[i + 1]->op = oJGTEZ;
              popt_DeletePCode(i);
              nchanges++;
              break;

            default     :
              i++;
              break;
            }
          break;

        case oGTZ   :
          switch (g_opPtr[i + 1]->op)
            {
            case oNOT :
              g_opPtr[i]->op = oLTEZ;
              popt_DeletePCode(i + 1);
              nchanges++;
              break;

            case oJEQUZ :
              g_opPtr[i + 1]->op = oJLTEZ;
              popt_DeletePCode(i);
              nchanges++;
              break;

            case oJNEQZ :
              g_opPtr[i + 1]->op = oJGTZ;
              popt_DeletePCode(i);
              nchanges++;
              break;

            default     :
              i++;
              break;
            }
          break;

        case oLTEZ :
          switch (g_opPtr[i + 1]->op)
            {
            case oNOT :
              g_opPtr[i]->op = oGTZ;
              popt_DeletePCode(i + 1);
              nchanges++;
              break;

            case oJEQUZ :
              g_opPtr[i + 1]->op = oJGTZ;
              popt_DeletePCode(i);
              nchanges++;
              break;

            case oJNEQZ :
              g_opPtr[i + 1]->op = oJLTEZ;
              popt_DeletePCode(i);
              nchanges++;
              break;

            default     :
              i++;
              break;
            }
          break;

        default     :
          i++;
          break;
        }
    } /* end while */
  return (nchanges);

} /* end popt_BranchOptimize */

/**********************************************************************/

