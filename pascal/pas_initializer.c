/**********************************************************************
 * pas_level.c
 * Handle initialization of level variables
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
 **********************************************************************/

/**********************************************************************
 * Included Files
 **********************************************************************/

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "pas_debug.h"
#include "pas_defns.h"
#include "pas_tkndefs.h"
#include "pas_sysio.h"
#include "pas_errcodes.h"

#include "pas_error.h"
#include "pas_codegen.h"
#include "pas_initializer.h"

/**********************************************************************
 * Pre-processor Definitions
 **********************************************************************/

#define MAX_INITIALIZERS 32

/**********************************************************************
 * Private Type Definitions
 **********************************************************************/

struct initializer_s
{
  symbol_t *symbol;
  bool stdFile;
};

typedef struct initializer_s initializer_t;

/**********************************************************************
 * Private Data
 **********************************************************************/

static initializer_t g_initializers[MAX_INITIALIZERS];
static int g_nFilesInitialized = 0;

/**********************************************************************
 * Public Data
 **********************************************************************/

int g_nInitializer             = 0; /* The top of the initializer stack */
int g_levelInitializerOffset   = 0; /* Index to initializers for this level */

/**********************************************************************
 * Private Functions
 **********************************************************************/

void pas_AddFileInitializer(symbol_t *filePtr)
{
  /* Remember the file variable info so that we can use it at the appropriate
   * time.
   */

  if (g_nInitializer >= MAX_INITIALIZERS) error(eTOOMANYINIT);
  else
    {
      g_initializers[g_nInitializer].symbol  = filePtr;
      g_initializers[g_nInitializer].stdFile = false;
      g_nInitializer++;
    }
}

void pas_Initialization(void)
{
  int initializer;

  /* Generate each initializer */

  for (initializer = g_levelInitializerOffset;
       initializer < g_nInitializer;
       initializer++)
    {
      symbol_t *varPtr = g_initializers[initializer].symbol;
      if (varPtr->sKind == sFILE || varPtr->sKind == sTEXTFILE)
        {
          /* INPUT and OUTPUT are pre-allocated and pre-initialized */
          /* REVISIT:  How about ARRAYs of files. */

          if (g_nFilesInitialized < 2)
            {
              uint32_t fileNumber;

              if (g_nFilesInitialized == 0)
                {
                  fileNumber = INPUT_FILE_NUMBER;
                }
              else
                {
                  fileNumber = OUTPUT_FILE_NUMBER;
                }

              pas_GenerateDataOperation(opPUSH, fileNumber);
              g_initializers[initializer].stdFile = true;
            }
          else
            {
              pas_GenerateIoOperation(xALLOCFILE);
            }

          pas_GenerateStackReference(opSTS, varPtr);
          g_nFilesInitialized++;
        }
    }
}

void pas_Finalization(void)
{
  int initializer;

  /* Free resources used by pas_Initialization */

  for (initializer = g_levelInitializerOffset;
       initializer < g_nInitializer;
       initializer++)
    {
      symbol_t *varPtr = g_initializers[initializer].symbol;

      if (varPtr->sKind == sFILE || varPtr->sKind == sTEXTFILE)
        {
          if (!g_initializers[initializer].stdFile)
            {
              pas_GenerateStackReference(opLDS, varPtr);
              pas_GenerateIoOperation(xFREEFILE);
            }
        }
    }
}
