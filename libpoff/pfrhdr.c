/**********************************************************************
 * pfrhdr.c
 * Read info from a POFF file header
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
/* Get the type of the file from the POFF file header */

uint8_t poffGetFileType(poffHandle_t handle)
{
  poffInfo_t *poffInfo = (poffInfo_t*)handle;
  return poffInfo->fileHeader.fh_type;
}

/***********************************************************************/
/* Get the machine architecture from the POFF file header */

uint8_t poffGetArchitecture(poffHandle_t handle)
{
  poffInfo_t *poffInfo = (poffInfo_t*)handle;
  return poffInfo->fileHeader.fh_arch;
}

/***********************************************************************/
/* Get the program entry point */

uint32_t poffGetEntryPoint(poffHandle_t handle)
{
  poffInfo_t *poffInfo = (poffInfo_t*)handle;
  return poffInfo->fileHeader.fh_entry;
}

/***********************************************************************/
/* Return the name associated with the file in the file header */

const char *poffGetFileHdrName(poffHandle_t handle)
{
  poffInfo_t *poffInfo = (poffInfo_t*)handle;
  return poffGetString(handle, poffInfo->fileHeader.fh_name);
}

/***********************************************************************/

uint32_t poffGetRoDataSize(poffHandle_t handle)
{
  poffInfo_t *poffInfo = (poffInfo_t*)handle;
  return poffInfo->roDataSection.sh_size;
}

