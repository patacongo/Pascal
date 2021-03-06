/**********************************************************************
 * insn_addopcode
 * P-Code access utilities
 *
 *   Copyright (C) 2008, 2021-2022 Gregory Nutt. All rights reserved.
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

#include "pas_debug.h"
#include "pas_pcode.h"
#include "insn16.h"

#include "paslib.h"
#include "pofflib.h"
#include "pas_insn.h"

/**********************************************************************
 * Public Functions
 **********************************************************************/

/**********************************************************************/

uint32_t insn_AddOpCode(poffHandle_t handle, opType_t *ptr)
{
  uint32_t opSize;

  /* Write the opcode which is always present */

  (void)poffAddProgByte(handle, ptr->op);
  opSize = 1;

  /* Write the 8-bit argument if present */

  if (ptr->op & o8)
    {
      (void)poffAddProgByte(handle, ptr->arg1);
      opSize++;
    }

  /* Write the 16-bit argument if present */

  if (ptr->op & o16)
    {
      (void)poffAddProgByte(handle, (ptr->arg2 >> 8));
      (void)poffAddProgByte(handle, (ptr->arg2 & 0xff));
      opSize += 2;
    }

  return opSize;
}

/**********************************************************************/

void insn_ResetOpCodeWrite(poffHandle_t handle)
{
  poffResetAccess(handle);
}

/***********************************************************************/
