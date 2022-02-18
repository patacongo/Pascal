/****************************************************************************
 * pflineno.c
 * Dump contents of a POFF file line number table
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
 ***************************************************************************/

/***************************************************************************
 * Included Files
 ***************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "pas_debug.h" /* Standard types */
#include "pfprivate.h" /* POFF private definitions */
#include "pofflib.h"   /* Public interfaces */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void poffDumpLineNumberTable(poffHandle_t handle, FILE *outFile)
{
  poffLibLineNumber_t lineInfo;
  int32_t             index;

  fprintf(outFile, "\nPOFF Line Number Table:\n");
  fprintf(outFile, "INDEX  LINE   PROGRAM    FILENAME\n");
  fprintf(outFile, "       NUMBER OFFSET\n");

  for (; ; )
    {
      index = poffGetLineNumber(handle, &lineInfo);
      if (index < 0)
        {
          break;
        }

      fprintf(outFile, "%-6" PRId32 " %-6" PRIu32 " 0x%08" PRIx32 " %s\n",
              index, lineInfo.lineno, lineInfo.offset, lineInfo.filename);
    }
}
