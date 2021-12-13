/****************************************************************************
 * pfile.c
 *
 *   Copyright (C) 2021 Gregory Nutt. All rights reserved.
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "keywords.h"
#include "config.h"
#include "pas_errcodes.h"
#include "pas_error.h"

#include "pfile.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_FILE_NAME 64

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct pexecFileTable_s
{
  char fileName[MAX_FILE_NAME];
  FILE *stream;
  openMode_t openMode;
  uint16_t recordSize;
  bool text;
};

typedef struct pexecFileTable_s pexecFileTable_t;

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* The runtime file table.  Maps a fileNumber to the current state of the
 * file.
 */

static pexecFileTable_t g_fileTable[MAX_OPEN_FILES];

/****************************************************************************
 * Public Functions
 ****************************************************************************/

void pexec_assignfile(uint16_t fileNumber, bool text, const char *fileName)
{
  if (fileNumber >= MAX_OPEN_FILES)
    {
       fprintf(stderr, "ERROR: bad file number: %u\n", fileNumber);
    }
  else
    {
      strncpy(g_fileTable[fileNumber].fileName, fileName, MAX_FILE_NAME);
      g_fileTable[fileNumber].text = text;
    }
}

void pexec_openfile(uint16_t fileNumber, openMode_t openMode)
{
  const char *modeString;

  if (fileNumber >= MAX_OPEN_FILES)
    {
       fprintf(stderr, "ERROR: bad file number: %u\n", fileNumber);
    }
  else if (g_fileTable[fileNumber].stream == NULL)
    {
      fprintf(stderr, "ERROR: File not open: %u\n", fileNumber);
    }
  else
    {
      switch (openMode)
        {
          case eOPEN_READ :
            modeString = "r";
            break;

          case eOPEN_WRITE :
            modeString = "w";
            break;

          case eOPEN_APPEND :
            modeString = "a";
            break;

          default :
            fprintf(stderr, "ERROR: Bad open mode %d: %u\n",
                    (int)openMode, fileNumber);
            return;
        }

      g_fileTable[fileNumber].stream = fopen(g_fileTable[fileNumber].fileName,
                                             modeString);
      if (g_fileTable[fileNumber].stream == NULL)
        {
          fprintf(stderr, "ERROR: Failed to open %s: %u\n",
                  g_fileTable[fileNumber].fileName, fileNumber);
        }
      else
        {
          g_fileTable[fileNumber].openMode = openMode;
        }
    }
}

void pexec_closefile(uint16_t fileNumber)
{
  if (fileNumber >= MAX_OPEN_FILES)
    {
       fprintf(stderr, "ERROR: bad file number: %u\n", fileNumber);
    }
  else if (g_fileTable[fileNumber].stream == NULL)
    {
      fprintf(stderr, "ERROR: File not open: %u\n", fileNumber);
    }
  else
    {
      (void)fclose(g_fileTable[fileNumber].stream);
      g_fileTable[fileNumber].stream = NULL;
    }
}

void pexec_recordsize(uint16_t fileNumber, uint16_t size)
{
  if (fileNumber >= MAX_OPEN_FILES)
    {
       fprintf(stderr, "ERROR: bad file number: %u\n", fileNumber);
    }
  else
    {
      g_fileTable[fileNumber].recordSize = size;
    }
}
