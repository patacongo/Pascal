/**********************************************************************
 *  pfsymhandle.c
 *  POFF temporary symdata support
 *
 *   Copyright (C) 2008 Gregory Nutt. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pas_debug.h"    /* Standard types */
#include "pas_errcodes.h" /* Pascal error codes */

#include "pfprivate.h"    /* POFF private definitions */
#include "pofflib.h"      /* Public interfaces */

/***********************************************************************
 * Public Functions
 ***********************************************************************/

/***********************************************************************/
/* Create a handle to manage temporary symbol table data */

poffSymHandle_t poffCreateSymHandle(void)
{
  poffSymInfo_t *poffSymInfo;

  /* Create a new POFF handle */

  poffSymInfo = (poffSymInfo_t*)malloc(sizeof(poffSymInfo_t));
  if (poffSymInfo != NULL)
    {
      /* Set everthing to zero */

      memset(poffSymInfo, 0, sizeof(poffSymInfo_t));
    }

  return poffSymInfo;
}

/***********************************************************************/

void poffDestroySymHandle(poffSymHandle_t handle)
{
  /* Free all of the allocated, in-memory data */

  poffResetSymHandle(handle);

  /* Free the container */

  free(handle);
}

/***********************************************************************/

void poffResetSymHandle(poffSymHandle_t handle)
{
  poffSymInfo_t *poffSymInfo = (poffSymInfo_t*)handle;

  /* Free all of the allocated, in-memory data */

  if (poffSymInfo->symbolTable)
    {
      free(poffSymInfo->symbolTable);
    }

  /* Reset everything to the initial state */

  poffSymInfo->symbolTable      = NULL;
  poffSymInfo->symbolTableSize  = 0;
  poffSymInfo->symbolTableAlloc = 0;
}

