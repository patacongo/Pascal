/***************************************************************
 * pas_builtin.c
 * Functions built into the code at compile time
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
 ***************************************************************/

/***************************************************************
 * Included Files
 ***************************************************************/

#include <stdint.h>
#include <stdio.h>

#include "pas_main.h"
#include "pas_defns.h"
#include "pas_tkndefs.h"
#include "pas_pcode.h"
#include "pas_errcodes.h"
#include "pas_error.h"
#include "pas_token.h"
#include "pas_expression.h"
#include "pas_codegen.h"
#include "pas_function.h"

/***************************************************************
 * Private Function Prototypes
 ***************************************************************/

static exprType_t builtInSizeOf(void);
static exprType_t builtInLength(void);

/***************************************************************
 * Private Functions
 ***************************************************************/

/***********************************************************************/

static exprType_t builtInSizeOf(void)
{
  uint16_t size;

  /* FORM:  sizeof '(' variable | type ')' */

  pas_CheckLParen();
  switch (g_token)
    {
      /* Variables */

      case sFILE :
      case sTEXTFILE :
      case sINT :
      case sBOOLEAN :
      case sCHAR :
      case sREAL :
      case sSTRING :
      case sSCALAR :
      case sSUBRANGE :
      case sSET_OF :
      case sARRAY :
      case sRECORD :
        size = g_tknPtr->sParm.v.size;
        break;

      /* Pointers variables and VAR parameters are always the size of a point */

      case sPOINTER :
      case sVAR_PARM :
        size = sPTR_SIZE;
        break;

      /* Types */

      case sTYPE :
        size = g_tknPtr->sParm.t.asize;
        break;

      default:
        error(eINVARG);
        size = 0;
        break;;
    }

  /* Push the size on the stack */

  pas_GenerateDataOperation(opPUSH, size);

  getToken();
  pas_CheckRParen();
  return exprInteger;
}

/***********************************************************************/

static exprType_t builtInLength(void)
{
  exprType_t exprType;

  /* FORM:  length '(' string-expression ')' */

  pas_CheckLParen();

  /* Process the string-expression */

  exprType = pas_Expression(exprString, g_tknPtr);
  if (exprType != exprString)
    {
      error(eSTRING);
    }
  else
    {
      /* The top of the stack now holds:
       *
       *   TOS(0) - String buffer address
       *   TOS(1) - String length
       *
       * Just pop off the string address, leaving the length at the top of
       * the stack.
       */

      pas_GenerateDataOperation(opINDS, -sINT_SIZE);
    }

  pas_CheckRParen();
  return exprType;
}

/***************************************************************
 * Public Functions
 ***************************************************************/

/***************************************************************/
/* Process a built-in function */

exprType_t pas_BuiltInFunction(void)
{
  exprType_t exprType = exprUnknown;

  TRACE(g_lstFile,"[pas_BuiltInFunction]");

  /* Is the token a builtin function? */

  if (g_token == tBUILTIN)
    {
      /* Yes, process it procedure according to the extended token type */

      switch (g_tknSubType)
        {
          /* Functions returning an integer */

        case txSIZEOF :
          exprType = builtInSizeOf();
          break;

        case txLENGTH :
          exprType = builtInLength();
          break;

        default :
          error(eINVALIDFUNC);
          break;
        }
    }

  return exprType;
}
