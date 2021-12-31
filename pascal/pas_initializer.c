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
#include <string.h>

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
  symbol_t variable;               /* Copy of the symbol table entry */

  /* Information specific to a symbol type */

  union
    {
      /* File variable initialization data */

      struct
        {
          bool      preallocated;  /* File number is pre-allocated */
          uint16_t  fileNumber;    /* Pre-allocted file number */
        } f;

      /* String variable initialization data */

      struct
        {
        } s;

      /* Record file/string initialization data */

      struct
        {
          symbol_t *recordObjectPtr;
        } r;
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

static initializer_t *pas_AddInitializer(symbol_t *varPtr)
{
  initializer_t *initializer = NULL;

  /* Remember the variable information so that we can use it at the
   * appropriate time.
   */

  if (g_nInitializer >= MAX_INITIALIZERS) error(eTOOMANYINIT);
  else
    {
      initializer         = &g_initializers[g_nInitializer];

      memset(initializer, 0, sizeof(initializer_t));
      memcpy(&initializer->variable, varPtr, sizeof(symbol_t));
      g_nInitializer++;
    }

  return initializer;
}

/**********************************************************************
 * Public Functions
 **********************************************************************/

void pas_AddFileInitializer(symbol_t *filePtr, bool preallocated,
                            uint16_t fileNumber)
{
  initializer_t *initializer = pas_AddInitializer(filePtr);
  if (initializer != NULL)
    {
      /* Remember the file variable info so that we can use it at the
       * appropriate time.
       */

      initializer->f.preallocated  = preallocated;
      initializer->f.fileNumber    = fileNumber;
    }
}

void pas_AddStringInitializer(symbol_t *stringPtr)
{
  (void)pas_AddInitializer(stringPtr);
}

void pas_AddRecordObjectInitializer(symbol_t *recordVarPtr,
                                    symbol_t *recordObjectPtr)
{
  initializer_t *initializer = pas_AddInitializer(recordVarPtr);
  if (initializer != NULL)
    {
      /* Remember the record object too.*/

      initializer->r.recordObjectPtr = recordObjectPtr;
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
      symbol_t      *varPtr      = &initializer->variable;

      switch (varPtr->sKind)
        {
          /* REVISIT:  How about ARRAYs of files. */

          case sFILE :
          case sTEXTFILE :
            if (initializer->f.preallocated)
              {
                pas_GenerateDataOperation(opPUSH, initializer->f.fileNumber);
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

            /* Get TOS = Address of string variable to be initialized */

            pas_GenerateStackReference(opLAS, varPtr);
            pas_StandardFunctionCall(lbSTRINIT);
            pushs = true;
            break;

          case sRECORD:
            {
              symbol_t *objectPtr     = initializer->r.recordObjectPtr;
              symbol_t *objectTypePtr = objectPtr->sParm.r.rParent;
              symbol_t *baseTypePtr;
              symbol_t *nextTypeptr;
              int fOffset             = objectPtr->sParm.r.rOffset;

              /* Deals with cases where the type of the field is a series
               * of type definitions.
               */

              nextTypeptr     = objectTypePtr;
              baseTypePtr     = objectTypePtr;
              while (nextTypeptr != NULL && nextTypeptr->sKind == sTYPE)
                {
                  baseTypePtr = nextTypeptr;
                  nextTypeptr = baseTypePtr->sParm.t.parent;
                }

              /* Check if the base type of the field is a string */

              if (baseTypePtr->sParm.t.type == sSTRING)
                {
                  /* Get TOS = Address of string variable to be initialized */

                  pas_GenerateStackReference(opLAS, varPtr);

                  /* Add the offset to the string field to be initialized */

                  pas_GenerateDataOperation(opPUSH, fOffset);
                  pas_GenerateSimple(opADD);

                  /* Then initialize the string */

                  pas_StandardFunctionCall(lbSTRINIT);
                  pushs = true;
                }

              /* Check if the base type of the field is a file */

              else if (baseTypePtr->sParm.t.type == sFILE ||
                       baseTypePtr->sParm.t.type == sTEXTFILE)
                {
                  /* Allocate a file number */

                  pas_GenerateIoOperation(xALLOCFILE);

                  /* Get TOS = Address of file variable to be initialized */

                  pas_GenerateStackReference(opLAS, varPtr);

                  /* Add the offset to the file field to be initialized */

                  pas_GenerateDataOperation(opPUSH, fOffset);
                  pas_GenerateSimple(opADD);

                  /* Then initialize the file */

                  pas_GenerateStackReference(opSTS, varPtr);
                }
            }
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

      if (!initializer->f.preallocated)
        {
          symbol_t  *varPtr      = &initializer->variable;

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
