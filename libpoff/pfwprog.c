/**********************************************************************
 * pfwprog.c
 * Write program data to a POFF file
 *
 *   Copyright (C) 2008 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <spudmonkey@racsa.co.cr>
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
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
#include <errno.h>

#include "keywords.h"  /* Standard types */
#include "pedefs.h"    /* error code definitions */

#include "perr.h"      /* error() */
#include "pofflib.h"   /* POFF library interface */
#include "pfprivate.h" /* POFF private definitions */

/**********************************************************************
 * Definitions
 **********************************************************************/

/**********************************************************************
 * Global Variables
 **********************************************************************/

/**********************************************************************
 * Private Variables
 **********************************************************************/

/***********************************************************************
 * Private Function Prototypes
 ***********************************************************************/

/***********************************************************************
 * Private Functions
 ***********************************************************************/

/***********************************************************************
 * Public Functions
 ***********************************************************************/

void poffAddProgByte(poffHandle_t handle, ubyte progByte)
{
  poffInfo_t *poffInfo = (poffInfo_t*)handle;

  /* Check if we have allocated a program section buffer yet */

  if (!poffInfo->progSectionData)
    {
      /* No, allocate it now */

      poffInfo->progSectionData = (char*)malloc(INITIAL_PROG_SECTION_SIZE);
      if (!poffInfo->progSectionData)
	{
	  fatal(eNOMEMORY);
	}

      poffInfo->progSection.sh_size = 0;
      poffInfo->progSectionAlloc    = INITIAL_STRING_TABLE_SIZE;
    }

  /* Check if there is room for the new string */

  if (poffInfo->progSection.sh_size + 1 > poffInfo->progSectionAlloc)
    {
      uint32 newAlloc = poffInfo->progSectionAlloc + PROG_SECTION_INCREMENT;
      void *tmp;

      /* Reallocate the program data section buffer */

      tmp = realloc(poffInfo->progSectionData, newAlloc);
      if (!tmp)
	{
	  fatal(eNOMEMORY);
	}

      /* And set the new size */

      poffInfo->progSectionAlloc = newAlloc;
      poffInfo->progSectionData  = (ubyte*)tmp;
    }

  /* Copy program data byte into the program data buffer */

  poffInfo->progSectionData[poffInfo->progSection.sh_size] = progByte;

  /* Set the new size of the string table */

  poffInfo->progSection.sh_size++;
}

/***********************************************************************/
