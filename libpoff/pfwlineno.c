/**********************************************************************
 * pfwlineno.c
 * Write line number data to a POFF file
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

static void poffCheckLineNumberAllocation(poffInfo_t *poffInfo)
{
  /* Check if we have allocated a line number buffer yet */

  if (!poffInfo->lineNumberTable)
    {
      /* No, allocate it now */

      poffInfo->lineNumberTable = (uint8_t*)
        malloc(INITIAL_LINENUMBER_TABLE_SIZE);

      if (!poffInfo->lineNumberTable)
        {
          fatal(eNOMEMORY);
        }

      poffInfo->lineNumberSection.sh_size = 0;
      poffInfo->lineNumberTableAlloc      = INITIAL_LINENUMBER_TABLE_SIZE;
    }
}

/***********************************************************************/

static void poffCheckLineNumberReallocation(poffInfo_t *poffInfo)
{
  if (poffInfo->lineNumberSection.sh_size + sizeof(poffLineNumber_t) >
      poffInfo->lineNumberTableAlloc)
    {
      uint32_t newAlloc =
        poffInfo->lineNumberTableAlloc +
        LINENUMBER_TABLE_INCREMENT;

      void *tmp;

      /* Reallocate the line number buffer */

      tmp = realloc(poffInfo->lineNumberTable, newAlloc);
      if (!tmp)
        {
          fatal(eNOMEMORY);
        }

      /* And set the new size */

      poffInfo->lineNumberTableAlloc = newAlloc;
      poffInfo->lineNumberTable      = (uint8_t*)tmp;
    }
}

/***********************************************************************
 * Public Functions
 ***********************************************************************/

/***********************************************************************/
/* Add a line number to the line number table.  Returns index value
 * associated with the line number entry in the line number table.
 */

uint32_t poffAddLineNumber(poffHandle_t handle,
                         uint16_t lineNumber, uint16_t fileNumber,
                         uint32_t progSectionDataOffset)
{
  poffInfo_t *poffInfo = (poffInfo_t*)handle;
  poffLineNumber_t *pln;
  uint32_t index;

  /* Verify that the line number table has been allocated */

  poffCheckLineNumberAllocation(poffInfo);

  /* Verify that the line number table is large enough to hold
   * information about another line.
   */

  poffCheckLineNumberReallocation(poffInfo);

  /* Save the line number information in the line number table */

  index = poffInfo->lineNumberSection.sh_size;
  pln   = (poffLineNumber_t*)&poffInfo->lineNumberTable[index];

  pln->ln_lineno  = lineNumber;
  pln->ln_fileno  = fileNumber;
  pln->ln_poffset = progSectionDataOffset;

  /* Set the new size of the line number table */

  poffInfo->lineNumberSection.sh_size += sizeof(poffLineNumber_t);
  return index;
}

