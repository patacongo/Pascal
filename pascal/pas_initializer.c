/****************************************************************************
 * pas_initializer.c
 * Handle initialization of level variables
 *
 *   Copyright (C) 2021-2022 Gregory Nutt. All rights reserved.
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

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

#include "pas_main.h"
#include "pas_error.h"
#include "pas_codegen.h"
#include "pas_expression.h"
#include "pas_initializer.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MAX_INITIALIZERS 32

/****************************************************************************
 * Private Type Definitions
 ****************************************************************************/

/* Initializer type */

enum initializerType_e
{
  VAR_INITIALIZER = 0,
  FILE_INITIALIZER,
  STRING_INITIALIZER,
  RECORD_OBJECT_INITIALIZER
};

typedef enum initializerType_e initializerType_t;

/* This structure describes one initializer */

struct initializer_s
{
  symbol_t          variable;      /* Copy of the symbol table entry */
  initializerType_t kind;          /* Kind of initializer */

  /* Information specific to a symbol type */

  union
    {
      /* Variable value initialization data */

      struct
        {
          uint32_t       baseType; /* Base type of the variable */
          int            strLen;   /* Length of the string */
          varInitValue_t value;    /* Initial value of the variable */
        } v;

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

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static initializer_t *pas_AddInitializer(symbol_t *varPtr,
                                         initializerType_t kind);
static void pas_BasicInitialization(void);
static void pas_SetInitialValues(void);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static initializer_t g_initializers[MAX_INITIALIZERS];

/****************************************************************************
 * Public Data
 ****************************************************************************/

int g_nInitializer           = 0;  /* The top of the initializer stack */
int g_levelInitializerOffset = 0;  /* Index to initializers for this level */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************/

static initializer_t *pas_AddInitializer(symbol_t *varPtr,
                                         initializerType_t kind)
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
      initializer->kind   = kind;
      g_nInitializer++;
    }

  return initializer;
}

/****************************************************************************/
/* Perform basic initialization needed by files and string:
 *
 * - Files need to have a file number assigned to them,
 * - Strings need to have a string buffer assigned to them
 *
 * This basic initialization must be done before we attempt to assign
 * initialization values to variables.
 */

static void pas_BasicInitialization(void)
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

      /* Skip over setting of inital values on this first pass */

      if (initializer->kind == VAR_INITIALIZER)
        {
          continue;
        }

      /* Initialize files and strings (include files and strings within records). */

      switch (varPtr->sKind)
        {
          /* Handle kind == FILE_INITIALIZER */

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

          /* Handle kind == STRING_INITIALIZER */

          case sSTRING:
            {
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
            }
            break;

          case sSHORTSTRING:
            {
              symbol_t *baseTypePtr;

              /* Generate a PUSHS (push string stack pointer) if we have not
               * already done so.
               */

              if (!pushs)
                {
                  /* Generate a PUSHS (push string stack pointer) */

                  pas_GenerateSimple(opPUSHS);
                }

              /* Get TOS = Size of the short string's string memory
               * allocation.
               */

              baseTypePtr = pas_GetBaseTypePointer(varPtr->sParm.v.vParent);
              pas_GenerateDataOperation(opPUSH,
                                        baseTypePtr->sParm.t.tMaxValue);

              /* Get the ddress of string variable to be initialized */

              pas_GenerateStackReference(opLAS, varPtr);
              pas_StandardFunctionCall(lbSSTRINIT);
              pushs = true;
            }
            break;

          /* Handle kind == RECORD_OBJECT_INITIALIZER, i.e., file and
           * string fields in RECORDS.
           */

          case sRECORD:
            {
              symbol_t *objectPtr     = initializer->r.recordObjectPtr;
              symbol_t *objectTypePtr = objectPtr->sParm.r.rParent;
              symbol_t *baseTypePtr;
              int fOffset             = objectPtr->sParm.r.rOffset;

              /* Deals with cases where the type of the field is a series
               * of type definitions.
               */

              baseTypePtr = pas_GetBaseTypePointer(objectTypePtr);

              /* Check if the base type of the field is a string */

              if (baseTypePtr->sParm.t.tType == sSTRING)
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

              /* Check if the base type of the field is a short string */

              else if (baseTypePtr->sParm.t.tType == sSHORTSTRING)
                {
                  /* Get TOS = Size of the short string's string memory
                   * allocation.
                   */

                  pas_GenerateDataOperation(opPUSH,
                                            baseTypePtr->sParm.t.tMaxValue);

                  /* Add the address of string variable to be initialized */

                  pas_GenerateStackReference(opLAS, varPtr);

                  /* Add the offset to the string field to be initialized */

                  pas_GenerateDataOperation(opPUSH, fOffset);
                  pas_GenerateSimple(opADD);

                  /* Then initialize the short string */

                  pas_StandardFunctionCall(lbSSTRINIT);
                  pushs = true;
                }

              /* Check if the base type of the field is a file */

              else if (baseTypePtr->sParm.t.tType == sFILE ||
                       baseTypePtr->sParm.t.tType == sTEXTFILE)
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

/****************************************************************************/

static void pas_SetInitialValues(void)
{
  int index;

  /* Generate each index */

  for (index = g_levelInitializerOffset;
       index < g_nInitializer;
       index++)
    {
      initializer_t *initializer = &g_initializers[index];
      symbol_t      *varPtr      = &initializer->variable;

      /* Only process setting of initial values of variables */

      if (initializer->kind == VAR_INITIALIZER)
        {
          switch (initializer->v.baseType)
            {
              /* Ordinal types */

              case sINT :
              case sWORD :
              case sBOOLEAN :
              case sSCALAR :
                pas_GenerateDataOperation(opPUSH, initializer->v.value.iOrdinal);
                pas_GenerateStackReference(opSTS, varPtr);
                break;

              case sCHAR:
                pas_GenerateDataOperation(opPUSH, initializer->v.value.iOrdinal);
                pas_GenerateStackReference(opSTSB, varPtr);
                break;

              /* Real values */

              case sREAL:
                pas_GenerateDataOperation(opPUSH, initializer->v.value.iAltReal[0]);
                pas_GenerateDataOperation(opPUSH, initializer->v.value.iAltReal[1]);
                pas_GenerateDataOperation(opPUSH, initializer->v.value.iAltReal[2]);
                pas_GenerateDataOperation(opPUSH, initializer->v.value.iAltReal[3]);
                pas_GenerateDataOperation(opPUSH, sREAL_SIZE);
                pas_GenerateStackReference(opSTSM, varPtr);
                break;

              /* Strings.  In all cases, the initial value of the string is a
               * C string in read-only memory.
               */

              case sSTRING :
              case sSHORTSTRING :
                {
                  uint16_t opCode;

                  /* Create a standard string.  Get the offset then size of
                   * the string on the stack.
                   */

                  pas_GenerateDataOperation(opPUSH, initializer->v.strLen);
                  pas_GenerateDataOperation(opLAC, initializer->v.value.iRoOffset);

                  /* And copy the string to the string valiable */

                  pas_GenerateStackReference(opLAS, varPtr);

                  if (initializer->v.baseType == sSTRING)
                    {
                      opCode = lbSTRCPY;
                    }
                  else
                    {
                      opCode = lbSTR2SSTR;
                    }

                  pas_StandardFunctionCall(opCode);
                }
                break;

              case sSET :
                pas_GenerateDataOperation(opPUSH, initializer->v.value.iSet[0]);
                pas_GenerateDataOperation(opPUSH, initializer->v.value.iSet[1]);
                pas_GenerateDataOperation(opPUSH, initializer->v.value.iSet[2]);
                pas_GenerateDataOperation(opPUSH, initializer->v.value.iSet[3]);
                pas_GenerateDataOperation(opPUSH, sSET_SIZE);
                pas_GenerateStackReference(opSTSM, varPtr);
                break;

              case sPOINTER :
                pas_GenerateDataOperation(opPUSH, initializer->v.value.iPointer);
                pas_GenerateStackReference(opSTS, varPtr);
                break;

              default:
                error(eHUH);
                break;
            }
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************/

void pas_AddInitialValue(varInitializer_t *varInitializer)
{
  initializer_t *initializer;

  initializer = pas_AddInitializer(varInitializer->iVarPtr,
                                   VAR_INITIALIZER);
  if (initializer != NULL)
    {
      /* Remember the file variable info so that we can use it at the
       * appropriate time.
       */

      initializer->v.baseType = varInitializer->iBaseType;
      initializer->v.strLen   = varInitializer->iStrLen;
      initializer->v.value    = varInitializer->iValue;
    }
}

/****************************************************************************/

void pas_AddFileInitializer(symbol_t *filePtr, bool preallocated,
                            uint16_t fileNumber)
{
  initializer_t *initializer;

  initializer = pas_AddInitializer(filePtr, FILE_INITIALIZER);
  if (initializer != NULL)
    {
      /* Remember the file variable info so that we can use it at the
       * appropriate time.
       */

      initializer->f.preallocated  = preallocated;
      initializer->f.fileNumber    = fileNumber;
    }
}

/****************************************************************************/

void pas_AddStringInitializer(symbol_t *stringPtr)
{
  (void)pas_AddInitializer(stringPtr, STRING_INITIALIZER);
}

/****************************************************************************/

void pas_AddRecordObjectInitializer(symbol_t *recordVarPtr,
                                    symbol_t *recordObjectPtr)
{
  initializer_t *initializer;

  initializer = pas_AddInitializer(recordVarPtr, RECORD_OBJECT_INITIALIZER);
  if (initializer != NULL)
    {
      /* Remember the record object too.*/

      initializer->r.recordObjectPtr = recordObjectPtr;
    }
}

/****************************************************************************/

void pas_Initialization(void)
{
  /* Perform basic initialization needed by files and string:
   *
   * - Files need to have a file number assigned to them,
   * - Strings need to have a string buffer assigned to them
   *
   * This basic initialization must be done before we attempt to assign
   * initialization values to variables.
   */

  pas_BasicInitialization();

  /* Now we can initialize the variable values */

  pas_SetInitialValues();
}

/****************************************************************************/
/* Initialize a new string type created with new().  That happens AFTER the
 * normal initialization of pas_Initialization().  Note that no special
 * finalization is required for strings since POPS will be generated
 * automatically.
 */

void pas_InitializeNewString(symbol_t *typePtr)
{
  symbol_t *baseTypePtr;

  /* At run-time, the address of the allocated string variable will be at
   * the top of the stack.
   */

  baseTypePtr = pas_GetBaseTypePointer(typePtr);
  if (baseTypePtr->sParm.t.tType == sSTRING)
    {
      /* Initialize the string */

      pas_StandardFunctionCall(lbSTRINIT);
    }
  else if (baseTypePtr->sParm.t.tType == sSHORTSTRING)
    {
      /* Get TOS = Size of the short string's string memory allocation. */

      pas_GenerateDataOperation(opPUSH, baseTypePtr->sParm.t.tMaxValue);

      /* Correct the order of the stack variables so that we have:
       *
       *   TOS(0) = Address of short string variable
       *   TOS(1) = Short string memory allocation.
       */

      pas_GenerateSimple(opXCHG);

      /* Initialize the short string */

      pas_StandardFunctionCall(lbSSTRINIT);
    }
}

/****************************************************************************/
/* Initialize a new file type created with new().  That happens AFTER the
 * normal initialization of pas_Initialization().
 */

void pas_InitializeNewFile(symbol_t *typePtr)
{
  /* At run-time, the address of the allocated file variable will be at
   * the top of the stack.
   */

  pas_GenerateStackReference(opLDS, g_tknPtr);
  pas_GenerateIoOperation(xALLOCFILE);
  pas_GenerateSimple(opSTI);
}

/****************************************************************************/

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
              case sSHORTSTRING :
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

/****************************************************************************/
/* Finalize a file type created with dispose().  That happens BEFORE
 * pas_Finalization() when DISPOSE() is called.
 */

void pas_FinalizeNewFile(symbol_t *varPtr)
{
  pas_GenerateStackReference(opLDS, varPtr);
  pas_GenerateIoOperation(xFREEFILE);
}

