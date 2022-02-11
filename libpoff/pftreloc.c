/**********************************************************************
 * pftreloc.c
 * Write relocation information to a temporary container
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
 * Public Functions
 ***********************************************************************/

/***********************************************************************/

void poffCloneRelocations(poffHandle_t handle,
                          poffRelocHandle_t relocHandle)
{
  poffRelocation_t reloc;
  int32_t          index;

  /* Discard any existing relocation table */

  poffResetRelocHandle(relocHandle);

  /* Traverse the input file relocation table from the beginning */

  poffResetRelocationTraversal(handle);
  do
    {
      /* Read each relocation record from the input File */

      index = poffGetRelocation(handle, &reloc);
      if (index >= 0)
        {
          /* Add the relocation to the temporary relocation table */

          poffAddTmpRelocation(relocHandle, &reloc);
        }
    }
  while (index >= 0);
}

/***********************************************************************/

#if 0 /* Not used */
void poffCloneTmpRelocations(poffRelocHandle_t parentHandle,
                             poffRelocHandle_t cloneHandle)
{
  poffRelocInfo_t *parentRelocInfo = (poffRelocInfo_t*)parentHandle;
  poffRelocInfo_t *cloneRelocInfo = (poffRelocInfo_t*)cloneHandle;

  /* Discard any existing relocation data in the clone */

  if (cloneRelocInfo->relocTable != NULL)
    {
      free(cloneRelocInfo->relocTable);
    }

  /* Duplicate the relocation data */

  cloneRelocInfo->relocSize  = parentRelocInfo->relocSize;
  cloneRelocInfo->relocAlloc = parentRelocInfo->relocAlloc;

  cloneRelocInfo->relocTable = (uint8_t*)
    malloc(parentRelocInfo->relocAlloc);
  if (cloneRelocInfo->relocTable == NULL)
    {
      fatal(eNOMEMORY);
    }

  memcpy(cloneRelocInfo->relocTable, parentRelocInfo->relocTable,
         parentRelocInfo->relocSize);
}
#endif

/***********************************************************************/
/* Add a relocation entry to the relocation table section data.  Returns
 * the index value associated with the relocation entry in the
 * relocation table section data.
 */

uint32_t poffAddTmpRelocation(poffRelocHandle_t relocHandle,
                              const poffRelocation_t *reloc)
{
  poffRelocInfo_t *poffRelocInfo = (poffRelocInfo_t*)relocHandle;
  poffRelocation_t *preloc;
  uint32_t index;

  /* Check if we have allocated a relocation table buffer yet */

  if (poffRelocInfo->relocTable == NULL)
    {
      /* No, allocate it now */

      poffRelocInfo->relocTable = (uint8_t*)malloc(INITIAL_RELOC_TABLE_SIZE);
      if (poffRelocInfo->relocTable == NULL)
        {
          fatal(eNOMEMORY);
        }

      poffRelocInfo->relocSize  = 0;
      poffRelocInfo->relocAlloc = INITIAL_RELOC_TABLE_SIZE;
    }

  /* Check if there is room for a new relocation entry */

  if (poffRelocInfo->relocSize + sizeof(poffRelocation_t) >
      poffRelocInfo->relocAlloc)
    {
      uint32_t newAlloc = poffRelocInfo->relocAlloc + RELOC_TABLE_INCREMENT;
      uint8_t *tmp;

      /* Reallocate the relocation table buffer */

      tmp = (uint8_t*)realloc(poffRelocInfo->relocTable, newAlloc);
      if (!tmp)
        {
          fatal(eNOMEMORY);
        }

      /* And set the new size */

      poffRelocInfo->relocAlloc = newAlloc;
      poffRelocInfo->relocTable = tmp;
    }

  /* Save the new relocation information in the relocation table data */

  index             = poffRelocInfo->relocSize;
  preloc            = (poffRelocation_t*)&poffRelocInfo->relocTable[index];

  preloc->rl_info   = reloc->rl_info;
  preloc->rl_offset = reloc->rl_offset;

  /* Set the new size of the file name table */

  poffRelocInfo->relocSize += sizeof(poffRelocation_t);
  return index;
}

/***********************************************************************/
/* Check if there is relocation data for the provided section data
 * offset.  If so, return the relocation information for that offset.
 */

int32_t poffFindTmpRelocation(poffRelocHandle_t relocHandle,
                              uint32_t offset,
                              poffRelocation_t *reloc)
{
  poffRelocInfo_t *poffRelocInfo = (poffRelocInfo_t*)relocHandle;
  uint32_t index;

  /* Check if we have allocated a relocation table buffer yet */

  if (poffRelocInfo->relocTable != NULL)
    {
      /* No, allocate it now */

      poffRelocInfo->relocTable = (uint8_t*)malloc(INITIAL_RELOC_TABLE_SIZE);
      if (!poffRelocInfo->relocTable)
        {
          fatal(eNOMEMORY);
        }

      poffRelocInfo->relocSize  = 0;
      poffRelocInfo->relocAlloc = INITIAL_RELOC_TABLE_SIZE;
    }

  /* Check each relocation entry or until we get a match */

  for (index = 0;
       index < poffRelocInfo->relocSize;
       index += sizeof(poffRelocation_t))
    {
      poffRelocation_t *candidate = (poffRelocation_t *)
        (&poffRelocInfo->relocTable + index);

      /* Is this entry at the requested offset? */

      if (candidate->rl_offset == offset)
        {
           /* Yes... return it */

           memcpy(reloc, candidate, sizeof(poffRelocation_t));
           return index;
        }
    }

  /* No match */

  return -1;
}

/***********************************************************************/

void poffReplaceRelocationTable(poffHandle_t handle,
                                poffRelocHandle_t relocHandle)
{
  poffInfo_t      *poffInfo         = (poffInfo_t*)handle;
  poffRelocInfo_t *poffRelocInfo    = (poffRelocInfo_t*)relocHandle;

  /* Discard any existing relocation table */

  if (poffInfo->relocTable != NULL)
    {
      free(poffInfo->relocTable);
    }

  /* Replace the relocation section data with the tmp data */

  poffInfo->relocTable              = poffRelocInfo->relocTable;
  poffInfo->relocSection.sh_size    = poffRelocInfo->relocSize;
  poffInfo->relocSection.sh_entsize = sizeof(poffRelocation_t);
  poffInfo->relocAlloc              = poffRelocInfo->relocAlloc;

  /* Reset the read index */

  poffInfo->relocIndex              = 0;

  /* Then nullify the tmp data */

  poffRelocInfo->relocTable         = NULL;
  poffRelocInfo->relocSize          = 0;
  poffRelocInfo->relocAlloc         = 0;
}
