/**********************************************************************
 * pfrsymbol
 * Read symbols from a POFF file
 *
 *   Copyright (C) 2008-2009 Gregory Nutt. All rights reserved.
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "pas_debug.h"    /* Standard types */
#include "pas_errcodes.h" /* error code definitions */

#include "pas_error.h"    /* error() */
#include "pofflib.h"      /* POFF library interface */
#include "pfprivate.h"    /* POFF private definitions */

/***********************************************************************
 * Private Functions
 ***********************************************************************/

/***********************************************************************/

int32_t poffGetSymbol(poffHandle_t handle, poffLibSymbol_t *symbol)
{
  poffInfo_t   *poffInfo = (poffInfo_t*)handle;
  poffSymbol_t *psym;
  uint32_t      index;

  /* First, check if there is another symbol in the table to be had.
   * This check is a little sloppy in that it assumes the the size in
   * in the section header is an even multiple of symbol table entries
   */

  index = poffInfo->symbolIndex;
  if (index >= poffInfo->symbolTableSection.sh_size)
    {
      memset(symbol, 0, sizeof(poffLibSymbol_t));
      return -1;
    }
  else
    {
      psym           = (poffSymbol_t*)&poffInfo->symbolTable[index];
      symbol->type   = psym->st_type;
      symbol->align  = psym->st_align;
      symbol->flags  = psym->st_flags;
      symbol->name   = poffGetString(handle, psym->st_name);
      symbol->value  = psym->st_value;
      symbol->size   = psym->st_size;

      poffInfo->symbolIndex += poffInfo->symbolTableSection.sh_entsize;
      return index / poffInfo->symbolTableSection.sh_entsize;
    }
}

