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
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "pas_debug.h"
#include "pas_defns.h"
#include "pas_tkndefs.h"
#include "pas_sysio.h"
#include "pas_library.h"
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
  symbol_t *symbol;                 /* Symbol table reference */

  union
    {
      /* File variable initialization data */

      struct
        {
          bool      preallocated;  /* File number is pre-allocated */
          uint16_t  fileNumber;    /* Pre-allocted file number */
        };

      /* String variable initialization data */

      struct
        {
        };
    };
};

typedef struct initializer_s initializer_t;

/**********************************************************************
 * Private Data
 **********************************************************************/

static initializer_t g_initializers[MAX_INITIALIZERS];

/**********************************************************************
 * Public Data
 **********************************************************************/

int g_nInitializer           = 0;  /* The top of the initializer stack */
int g_levelInitializerOffset = 0;  /* Index to initializers for this level */

/**********************************************************************
 * Private Functions
 **********************************************************************/

void pas_AddFileInitializer(symbol_t *filePtr, bool preallocated,
                            uint16_t fileNumber)
{
  /* Remember the file variable info so that we can use it at the appropriate
   * time.
   */

  if (g_nInitializer >= MAX_INITIALIZERS) error(eTOOMANYINIT);
  else
    {
      g_initializers[g_nInitializer].symbol       = filePtr;
      g_initializers[g_nInitializer].preallocated = preallocated;
      g_initializers[g_nInitializer].fileNumber   = fileNumber;
      g_nInitializer++;
    }
}

void pas_AddStringInitializer(symbol_t *filePtr)
{
  /* Remember the file variable info so that we can use it at the appropriate
   * time.
   */

  if (g_nInitializer >= MAX_INITIALIZERS) error(eTOOMANYINIT);
  else
    {
      g_initializers[g_nInitializer].symbol       = filePtr;
      g_initializers[g_nInitializer].preallocated = false;
      g_initializers[g_nInitializer].fileNumber   = 0;
      g_nInitializer++;
    }
}

void pas_Initialization(void)
{
  bool pushs = false;
  int  index;

  /* Generate each index */

  for (index = g_levelInitializerOffset;
       index < g_nInitializer;
       index++)
    {
      initializer_t *initializer = &g_initializers[index];
      symbol_t      *varPtr      = initializer->symbol;

      switch (varPtr->sKind)
        {
          /* REVISIT:  How about ARRAYs of files. */

          case sFILE :
          case sTEXTFILE :
            if (initializer->preallocated)
              {
                pas_GenerateDataOperation(opPUSH, initializer->fileNumber);
              }
            else
              {
                pas_GenerateIoOperation(xALLOCFILE);
              }

            pas_GenerateStackReference(opSTS, varPtr);
            break;

          case sSTRING:
            /* Generate a PUSHS (push string stack pointer) if we have not
             * already done so.
             */

            if (!pushs)
              {
                /* Generate a PUSHS (push string stack pointer) */

                pas_GenerateSimple(opPUSHS);
              }

            /* Generate:
             *
             *   TOS = Address of string variable to be initialized
             */

             pas_GenerateStackReference(opLAS, varPtr);
             pas_StandardFunctionCall(lbSTRINIT);
             pushs = true;
             break;

          default:
            error(eHUH);
            break;
        }
    }
}

void pas_Finalization(void)
{
  bool pops = false;
  int index;

  /* Free resources used by pas_Initialization */

  for (index = g_levelInitializerOffset;
       index < g_nInitializer;
       index++)
    {
      initializer_t *initializer = &g_initializers[index];

      /* If the variable uses all pre-allocated resources, then do nothing. */

      if (!initializer->preallocated)
        {
          symbol_t   *varPtr      = initializer->symbol;

          switch (varPtr->sKind)
            {
              case sFILE :
              case sTEXTFILE :
                  pas_GenerateStackReference(opLDS, varPtr);
                  pas_GenerateIoOperation(xFREEFILE);
                  break;

              case sSTRING :
                  pops = true;
                  break;

              default:
                  break;
            }
        }
    }

  /* If there are any strings, generate a POPS (pop the saved string stack). */

  if (pops)
    {
      pas_GenerateSimple(opPOPS);
    }
}
