/**********************************************************************
 * pas_codegen.c
 * P-Code generation logic
 *
 *   Copyright (C) 2008-2009, 2021 Gregory Nutt. All rights reserved.
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
#include <string.h>
#include <errno.h>

#include "config.h"        /* Configuration */
#include "pas_debug.h"     /* Standard types */
#include "pas_defns.h"     /* Common types */
#include "pas_tkndefs.h"   /* Token / symbol table definitions */
#include "pas_pcode.h"     /* Logical opcode definitions */
#include "pas_errcodes.h"  /* error code definitions */

#include "pas_main.h"      /* Global variables */
#include "poff.h"          /* For POFF file format */
#include "pofflib.h"       /* For poff*() functions*/
#include "pas_insn.h"      /* (DEBUG only) */
#include "pas_error.h"     /* error() */

#include "pas_procedure.h" /* for pas_ActualParameterSize */
#include "pas_codegen.h"   /* (to verify prototypes in this file) */

/**********************************************************************
 * Pre-processor Definitions
 **********************************************************************/

#define UNDEFINED_LEVEL (-1)
#define INVALID_PCODE   (-1)

#define LEVEL_DEFINED(l)  ((int32_t)(l) >= 0)
#define PCODE_VALID(p)    ((int32_t)(p) >= 0)

/***********************************************************************
 * Private Function Prototypes
 ***********************************************************************/

/***********************************************************************/
/* Generate a stack reference opcode to a global variable residing at
 * static nesting level zero.
 */

static void
pas_GenerateLevel0StackReference(enum pcode_e eOpCode, symbol_t *varPtr)
{
  /* Sanity checking.  Double check nesting level and also since this is
   * a level zero reference, then the offset must be positive
   */

  if (varPtr->sLevel != 0 || varPtr->sParm.v.vOffset < 0)
    {
      error(eHUH);
    }
  else
    {
      /* Generate the P-code */

      insn_GenerateDataOperation(eOpCode, varPtr->sParm.v.vOffset);

      /* If the variable is undefined, also generate a relocation
       * record.
       */

      if ((varPtr->sParm.v.vFlags & SVAR_EXTERNAL) != 0)
        {
          (void)poffAddRelocation(g_poffHandle, RLT_LDST,
                                  varPtr->sParm.v.vSymIndex, 0);
        }
    }
}

/***********************************************************************/
/* There are some special P-codes for accessing stack data at static
 * nesting level 0.  Check if the specified opcode is one of those.  If
 * so, return the mapped opcode.  Otherwise, return INVALID_PCODE.
 */

static int32_t
pas_GetLevel0Opcode(enum pcode_e eOpCode)
{
  switch (eOpCode)
    {
    case opLDS:   return opLD;
    case opLDSB:  return opLDB;
    case opLDSM:  return opLDM;
    case opSTS:   return opST;
    case opSTSB:  return opSTB;
    case opSTSM:  return opSTM;
    case opLDSX:  return opLDX;
    case opLDSXB: return opLDXB;
    case opLDSXM: return opLDXM;
    case opSTSX:  return opSTX;
    case opSTSXB: return opSTXB;
    case opSTSXM: return opSTXM;
    case opLAS:   return opLA;
    case opLASX:  return opLAX;
    default:      return INVALID_PCODE;
    }
}

/***********************************************************************
 * Public Functions
 ***********************************************************************/

/***********************************************************************/
/* Generate the most simple of all P-codes */

void pas_GenerateSimple(enum pcode_e eOpCode)
{
  insn_GenerateSimple(eOpCode);
}

/***********************************************************************/
/* Generate a P-code with a single data argument */

void pas_GenerateDataOperation(enum pcode_e eOpCode, int32_t dwData)
{
  insn_GenerateDataOperation(eOpCode, dwData);
}

/***********************************************************************/
/* This function is called just before a multiple register operation is
 * is generated.  This should generate logic to specify the size of the
 * multiple register operation (in bytes, not registers). This may translate
 * into different operations on different architectures.  Typically,
 * this would generate a push of the size onto the stack or, perhaps,
 * setting of a dedicated count register.
 */

void pas_GenerateDataSize(int32_t dwDataSize)
{
  insn_GenerateDataSize(dwDataSize);
}

/***********************************************************************/
/* Generate a floating point operation */

void pas_GenerateFpOperation(uint8_t fpOpcode)
{
  insn_GenerateFpOperation(fpOpcode);
}

/***********************************************************************/
/* Generate a pseudo call to a built-in, set operator/functionon */

void pas_GenerateSetOperation(uint8_t setOpcode)
{
  insn_GenerateSetOperation(setOpcode);
}

/***********************************************************************/
/* Generate an IO operation */

void pas_GenerateIoOperation(uint16_t ioOpcode)
{
  insn_GenerateIoOperation(ioOpcode);
}

/***********************************************************************/
/* Generate a pseudo call to a built-in, standard pascal function */

void pas_StandardFunctionCall(uint16_t libOpcode)
{
  insn_StandardFunctionCall(libOpcode);
}

/***********************************************************************/
/* Generate a reference to data on the data stack using the specified
 * level and offset.
 */

void pas_GenerateLevelReference(enum pcode_e eOpCode, uint16_t wLevel,
                                int32_t dwOffset)
{
  /* Is this variable declared at level 0 (i.e., it has global scope)
   * that is being offset via a nesting level?
   */

  if (wLevel == 0)
    {
      int32_t level0Opcode = pas_GetLevel0Opcode(eOpCode);
      if (PCODE_VALID(level0Opcode))
        {
          insn_GenerateDataOperation(level0Opcode, dwOffset);
          return;
        }
    }

  /* Then generate the opcode passing. */

  insn_GenerateLevelReference(eOpCode, wLevel, dwOffset);
}

/***********************************************************************/
/* Generate a stack reference opcode, handling references to undefined
 * stack offsets.
 */

void pas_GenerateStackReference(enum pcode_e eOpCode, symbol_t *varPtr)
{
  /* Is this variable declared at level 0 (i.e., it has global scope)
   * that is being offset via a nesting level?
   */

  if (varPtr->sLevel == 0)
    {
      int32_t level0Opcode = pas_GetLevel0Opcode(eOpCode);
      if (PCODE_VALID(level0Opcode))
        {
          pas_GenerateLevel0StackReference(level0Opcode, varPtr);
          return;
        }
    }

  /* Generate the P-Code at the defined offset and with the specified
   * static level offset.
   */

  insn_GenerateLevelReference(eOpCode, (g_level - varPtr->sLevel),
                              varPtr->sParm.v.vOffset);
}

/***********************************************************************/
/* Generate a procedure call and an associated relocation record if the
 * called procedure is external.
 */

void pas_GenerateProcedureCall(symbol_t *pProc)
{
  /* NOTE:  The level associated with the PROCEDURE symbol, sLevel, is the
   * level At which the procedure was declared.  Everything declare within
   * the PROCEDURE is at the next level
   */

  uint16_t level = pProc->sLevel + 1;

  /* Then generate the procedure call (passing the level again for those
   * architectures that do not support the SLP).
   */

  insn_GenerateProcedureCall(level, pProc->sParm.p.pLabel);

  /* If the variable is undefined, also generate a relocation
   * record.
   */

#if 0 /* Not yet */
  if ((varPtr->sParm.p.pFlags & SVAR_EXTERNAL) != 0)
    {
      /* For now */
# error "Don't know what last parameter should be"
      (void)poffAddRelocation(g_poffHandle, RLT_PCAL,
                              varPtr->sParm.p.pSymIndex,
                              0);
    }
#endif
}

/***********************************************************************/

void pas_GenerateLineNumber(uint16_t wIncludeNumber, uint32_t dwLineNumber)
{
  insn_GenerateLineNumber(wIncludeNumber, dwLineNumber);
}

/***********************************************************************/

void pas_GenerateDebugInfo(symbol_t *pProc, uint32_t dwReturnSize)
{
  int i;

  /* Allocate a container to pass the proc information to the library */

  uint32_t nparms                      = pProc->sParm.p.pNParms;
  poffLibDebugFuncInfo_t *pContainer = poffCreateDebugInfoContainer(nparms);

  /* Put the proc information into the container */

  pContainer->value   = pProc->sParm.p.pLabel;
  pContainer->retsize = dwReturnSize;
  pContainer->nparms  = nparms;

  /* Add the argument information to the container */

   for (i = 0; i < nparms; i++)
     {
       pContainer->argsize[i] = pas_ActualParameterSize(pProc, i+1);
     }

   /* Add the contained information to the library */

   poffAddDebugFuncInfo(g_poffHandle, pContainer);

   /* Release the container */

   poffReleaseDebugFuncContainer(pContainer);
}

/***********************************************************************/
/* Generate description of a level 0 stack variable that can be
 * exported by a unit.
 */

void pas_GenerateStackExport(symbol_t *varPtr)
{
  poffLibSymbol_t symbol;

#if CONFIG_DEBUG
  /* Get the parent type of the variable */

  symbol_t *typePtr = varPtr->sParm.v.vParent;

  /* Perform some sanity checking:
   * - Must have a parent type
   * - Must not be declared external
   * - Must be declared at static nesting level zero
   */

  if (typePtr == NULL ||
      (varPtr->sParm.v.vFlags & SVAR_EXTERNAL) != 0 ||
      varPtr->sLevel != 0)
    {
      error(eSYMTABINTERNAL);
    }
#endif

  /* Create the symbol structure */

  symbol.type  = STT_DATA;
  symbol.align = STA_8BIT; /* for now */
  symbol.flags = STF_NONE;
  symbol.name  = varPtr->sName;
  symbol.value = varPtr->sParm.v.vOffset;
  symbol.size  = varPtr->sParm.v.vSize;

  /* Add the symbol to the symbol table */

  (void)poffAddSymbol(g_poffHandle, &symbol);
}

/***********************************************************************/
/* Generate description of a level 0 stack variable that must be
 * imported by a program or unit from a unit.
 */

void pas_GenerateStackImport(symbol_t *varPtr)
{
  poffLibSymbol_t symbol;

#if CONFIG_DEBUG
  /* Get the parent type of the variable */

  symbol_t *typePtr = varPtr->sParm.v.vParent;

  /* Perform some sanity checking
   * - Must have a parent type
   * - Must be declared external
   * - Must be declared at static nesting level zero
   */

  if (typePtr == NULL ||
      (varPtr->sParm.v.vFlags & SVAR_EXTERNAL) == 0 ||
      varPtr->sLevel != 0)
    {
      error(eSYMTABINTERNAL);
    }
#endif

  /* Create the symbol structure */

  symbol.type  = STT_DATA;
  symbol.align = STA_8BIT; /* for now */
  symbol.flags = STF_UNDEFINED;
  symbol.name  = varPtr->sName;
  symbol.value = varPtr->sParm.v.vOffset; /* for now */
  symbol.size  = varPtr->sParm.v.vSize;

  /* Add the symbol to the symbol table */

  varPtr->sParm.v.vSymIndex= poffAddSymbol(g_poffHandle, &symbol);
}

/***********************************************************************/
/* Generate description of a level 0 procedure or function that can be
 * exported by a unit.
 */

void pas_GenerateProcExport(symbol_t *pProc)
{
  poffLibSymbol_t symbol;

#if CONFIG_DEBUG
  /* Get the parent type of the function (assuming it is a function) */

  symbol_t *typePtr = pProc->sParm.p.pParent;

  /* Perform some sanity checking */

  /* Check for a function reference which must have a valid parent type */

  if (pProc->sKind == sFUNC && typePtr != NULL);

  /* Check for a procedure reference which must not have a valid type */

  else  if (pProc->sKind == sPROC && typePtr == NULL);

  /* Anything else is an error */

  else
    {
      error(eSYMTABINTERNAL);
    }

  /* The function / procedure should NOT be declared external and
   * only procedures declared at static nesting level zero can
   * be exported.
   */

  if ((pProc->sParm.p.pFlags & SPROC_EXTERNAL) != 0 ||
      pProc->sLevel != 0)
    {
      error(eSYMTABINTERNAL);
    }
#endif

  /* Everthing looks okay.  Create the symbol structure */

  if (pProc->sKind == sPROC)
    {
      symbol.type  = STT_PROC;
    }
  else
    {
      symbol.type  = STT_FUNC;
    }

  symbol.align = STA_NONE;
  symbol.flags = STF_NONE;
  symbol.name  = pProc->sName;
  symbol.value = pProc->sParm.p.pLabel;
  symbol.size  = 0;

  /* Add the symbol to the symbol table */

  (void)poffAddSymbol(g_poffHandle, &symbol);
}

/***********************************************************************/
/* Generate description of a level 0 procedure or function that must be
 * imported by a program or unit from a unit.
 */

void pas_GenerateProcImport(symbol_t *pProc)
{
  poffLibSymbol_t symbol;

#if CONFIG_DEBUG
  /* Get the parent type of the function (assuming it is a function) */

  symbol_t *typePtr = pProc->sParm.p.pParent;

  /* Perform some sanity checking */

  /* Check for a function reference which must have a valid parent type */

  if (pProc->sKind == sFUNC && typePtr != NULL);

  /* Check for a procedure reference which must not have a valid type */

  else  if (pProc->sKind == sPROC && typePtr == NULL);

  /* Anything else is an error */

  else
    error(eSYMTABINTERNAL);

  /* The function / procedure should also be declared external and
   * only procedures declared at static nesting level zero can
   * be exported.
   */

  if ((pProc->sParm.p.pFlags & SPROC_EXTERNAL) == 0 ||
      pProc->sLevel != 0)
    {
      error(eSYMTABINTERNAL);
    }
#endif

  /* Everthing looks okay.  Create the symbol structure */

  if (pProc->sKind == sPROC)
    {
      symbol.type  = STT_PROC;
    }
  else
    {
      symbol.type  = STT_FUNC;
    }

  symbol.align = STA_NONE;
  symbol.flags = STF_UNDEFINED;
  symbol.name  = pProc->sName;
  symbol.value = pProc->sParm.p.pLabel;
  symbol.size  = 0;

  /* Add the symbol to the symbol table */

  pProc->sParm.p.pSymIndex = poffAddSymbol(g_poffHandle, &symbol);
}
