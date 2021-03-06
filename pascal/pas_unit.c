/**********************************************************************
 * pas_unit.c
 * Parse a pascal unit file
 *
 *   Copyright (C) 2008-2009, 2021-2022 Gregory Nutt. All rights reserved.
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
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "pas_debug.h"
#include "pas_defns.h"
#include "pas_tkndefs.h"
#include "pas_pcode.h"
#include "pas_errcodes.h"

#include "pofflib.h"         /* For poff*() functions*/
#include "poff.h"            /* FHT_ definitions */

#include "pas_main.h"        /* for globals */
#include "pas_block.h"       /* for pas_ConstantDefinitionGroup(), etc. */
#include "pas_initializer.h" /* for initialzer stack variables */
#include "pas_codegen.h"     /* for pas_Generate*() */
#include "pas_token.h"       /* for getToken() */
#include "pas_symtable.h"    /* for pas_AddProcedure() */
#include "pas_error.h"       /* for error() */
#include "pas_program.h"     /* for pas_UsesSection() */
#include "pas_statement.h"   /* for pas_CompoundStatement() */
#include "pas_machine.h"     /* for INT_ALIGNUP() */
#include "pas_unit.h"

/***********************************************************************
 * Private Function Prototypes
 ***********************************************************************/

static void pas_InterfaceSection          (void);
static void pas_ExportedProcedureHeading  (void);
static void pas_ExportedFunctionHeading   (void);

/***********************************************************************
 * Private Functions
 ***********************************************************************/

static void pas_InterfaceSection(void)
{
  unsigned int saveNSym;        /* Save top of symbol table */
  unsigned int saveNConst;      /* Save top of constant table */
  unsigned int saveSymOffset;   /* Save previous level symbol offset */
  unsigned int saveConstOffset; /* Save previous level constant offset */

  /* Save the current top-of-stack data for symbols, constants, and
   * initializers.
   */

  saveNSym           = g_nSym;
  saveNConst         = g_nConst;

  saveSymOffset      = g_levelSymOffset;
  saveConstOffset    = g_levelConstOffset;

  /* Set the current symbol and constant table offsets for this level */

  g_levelSymOffset   = saveNSym;
  g_levelConstOffset = saveNConst;

  /*  FORM: interface-section =
   *       'interface' [ uses-section ] interface-declaration
   *
   * On entry, the unit-heading keyword has already been parsed.  The
   * current token should point to the identifier following unit.
   */

  if (g_token != tINTERFACE) error(eINTERFACE);
  else getToken();

  FP->section = eIsInterfaceSection;

  /* Check for the presence of an optional uses-section */

  if (g_token == tUSES)
    {
      /* Process the uses-section */

      getToken();
      pas_UsesSection();
    }

  /* Process the interface-declaration
   *
   * FORM: interface-declaration =
   *       [ constant-definition-group ] [ type-definition-group ]
   *       [ variable-declaration-group ] exported-heading
   */

   /* Process optional constant-definition-group.
    * FORM: constant-definition-group =
    *       'const' constant-definition ';' { constant-definition ';' }
    */

   if (g_token == tCONST)
     {
       /* Limit search to present level */

       getLevelToken();

       /* Process constant-definition.
        * FORM: constant-definition = identifier '=' constant
        */

       pas_ConstantDefinitionGroup();

     }

   /* Process type-definition-group
    * FORM: type-definition-group =
    *       'type' type-definition ';' { type-definition ';' }
    */

   if (g_token == tTYPE)
     {
       /* Limit search to present level */

       getLevelToken();

       /* Process the type-definitions in the type-definition-group
        * FORM: type-definition = identifier '=' type-denoter
        */

       pas_TypeDefinitionGroup();
     }

   /* Process the optional variable-declaration-group
    * FORM: variable-declaration-group =
    *       'var' variable-declaration { ';' variable-declaration }
    */

   if (g_token == tVAR)
     {
       /* Limit search to present level */

       getLevelToken();

       /* Process the variable declarations
        * FORM: variable-declaration = identifier-list ':' type-denoter
        * FORM: identifier-list = identifier { ',' identifier }
        */

       pas_VariableDeclarationGroup();
     }

   /* Process the exported-heading
    *
    * FORM: exported-heading =
    *       procedure-heading ';' [ directive ] |
    *       function-heading ';' [ directive ]
    */

   for (; ; )
     {
       /* FORM: function-heading =
        *       'function' function-identifier [ formal-parameter-list ]
        *        ':' result-type
        */

       if (g_token == tFUNCTION)
         {
           /* Limit search to present level */

           getLevelToken();

           /* Process the interface declaration */

           pas_ExportedFunctionHeading();
         }

       /* FORM: procedure-heading =
        *       'procedure' procedure-identifier [ formal-parameter-list ]
        */

       else if (g_token == tPROCEDURE)
         {
           /* Limit search to present level */

           getLevelToken();

           /* Process the interface declaration */

           pas_ExportedProcedureHeading();
         }
       else break;
     }

   /* We are finished with the interface section */

   FP->section = eIsOtherSection;

  /* Restore the symbol/constant table offsets for the previous level */

  g_levelSymOffset   = saveSymOffset;
  g_levelConstOffset = saveConstOffset;
}

/* Process Procedure Declaration Block */

static void pas_ExportedProcedureHeading(void)
{
  uint16_t  procLabel = ++g_label;
  char     *saveChSp;
  symbol_t *procPtr;
  int       i;

  /* FORM: procedure-heading =
   *       'procedure' identifier [ formal-parameter-list ]
   * FORM: procedure-identifier = identifier
   *
   * On entry, g_token refers to token AFTER the 'procedure' reserved
   * word.
   */

  /* Process the procedure-heading */

  if (g_token != tIDENT)
    {
      error (eIDENT);
      return;
    }

  procPtr = pas_AddProcedure(g_tokenString, sPROC, procLabel, 0, NULL);

  /* Mark the procedure as external */

  procPtr->sParm.p.pFlags |= SPROC_EXTERNAL;

  /* Save the string stack pointer so that we can release all
   * formal parameter strings later.  Then get the next token.
   */

  saveChSp = g_stringSP;
  getToken();

  /* NOTE:  The level associated with the PROCEDURE symbol is the level
   * At which the procedure was declared.  Everything declared within the
   * PROCEDURE is at the next level
   */

  g_level++;

  /* Process parameter list */

  (void)pas_FormalParameterList(procPtr);

  if (g_token !=  ';') error (eSEMICOLON);
  else getToken();

  /* If we are compiling a program or unit that "imports" the
   * procedure then generate the appropriate symbol table entries
   * in the output file to support relocation when the external
   * procedure is called.
   */

  if (g_includeIndex > 0)
    {
      pas_GenerateProcImport(procPtr);
    }

  /* Destroy formal parameter names */

  for (i = 1; i <= procPtr->sParm.p.pNParms; i++)
    {
      procPtr[i].sName = NULL;
    }

  g_stringSP = saveChSp;

  /* Drop the level back to where it was */

  g_level--;
}

/***************************************************************/
/* Process Function Declaration Block */

static void pas_ExportedFunctionHeading(void)
{
  uint16_t  funcLabel = ++g_label;
  int16_t   parameterOffset;
  char     *saveChSp;
  symbol_t *funcPtr;
  int       i;

  /* FORM: function-declaration =
   *       function-heading ';' directive |
   *       function-heading ';' function-block
   * FORM: function-heading =
   *       'function' function-identifier [ formal-parameter-list ]
   *       ':' result-type
   *
   * On entry token should lrefer to the function-identifier.
   */

  /* Verify function-identifier */

  if (g_token != tIDENT)
    {
      error (eIDENT);
      return;
    }

  funcPtr = pas_AddProcedure(g_tokenString, sFUNC, funcLabel, 0, NULL);

  /* Mark the function as external */

  funcPtr->sParm.p.pFlags |= SPROC_EXTERNAL;

  /* NOTE:  The level associated with the FUNCTION symbol is the level
   * At which the procedure was declared.  Everything declared within the
   * FUNCTION is at the next level
   */

  g_level++;

  /* Save the string stack pointer so that we can release all
   * formal parameter strings later.  Then get the next token.
   */

  saveChSp = g_stringSP;
  getToken();

  /* Process parameter list */

  parameterOffset = pas_FormalParameterList(funcPtr);

  /* Verify that the parameter list is followed by a colon */

  if (g_token !=  ':') error (eCOLON);
  else getToken();

  /* Get function type, return value type/size and offset to return value */

  if (g_token == sTYPE)
    {
      /* The offset to the return value is the offset to the last
       * parameter minus the size of the return value (aligned to
       * multiples of size of INTEGER).
       */

      parameterOffset        -= g_tknPtr->sParm.t.tAllocSize;
      parameterOffset         = INT_ALIGNUP(parameterOffset);

      /* Save the TYPE for the function */

      funcPtr->sParm.p.pParent = g_tknPtr;

      /* Skip over the result-type token */

      getToken();
    }
  else
    {
      error(eINVTYPE);
    }

  /* Verify the final semicolon */

  if (g_token !=  ';') error (eSEMICOLON);
  else getToken();

  /* If we are compiling a program or unit that "imports" the
   * function then generate the appropriate symbol table entries
   * in the output file to support relocation when the external
   * function is called.
   */

  if (g_includeIndex > 0)
    {
      pas_GenerateProcImport(funcPtr);
    }

  /* Destroy formal parameter names and the function return value name */

  for (i = 1; i <= funcPtr->sParm.p.pNParms; i++)
    {
      funcPtr[i].sName = ((char *) NULL);
    }

  g_stringSP = saveChSp;

  /* Restore the original level */

  g_level--;
}

/***********************************************************************
 * Public Functions
 ***********************************************************************/
/* This function is called only main() when the first token parsed out
 * the specified file is 'unit'.  In this case, we are parsing a unit file
 * and generating a unit binary.
 */

void pas_UnitImplementation(void)
{
  char   *saveTknStart = g_tokenString;

  /* FORM: unit =
   *       unit-heading ';' interface-section implementation-section
   *       init-section '.'
   * FORM: unit-heading = 'unit' identifer
   * FORM: interface-section =
   *       'interface' [ uses-section ] interface-declaration
   * FORM: implementation-section =
   *       'implementation' [ uses-section ] declaration-group
   * FORM: init-section =
   *       'initialization statement-sequence
   *       ['finalization' statement-sequence] 'end' |
   *       compound-statement | 'end'
   *
   * On entry, the 'unit' keyword has already been parsed.  The
   * current token should point to the identifier following unit.
   */

  /* Skip over the unit identifier (the caller has already verified
   * that we are processing the correct unit).
   */

  if (g_token != tIDENT) error(eIDENT);

  /* Set a UNIT indication in the output poff file header */

  poffSetFileType(g_poffHandle, FHT_UNIT, 0, g_tokenString);
  poffSetArchitecture(g_poffHandle, FHA_PCODE);

  /* Discard the unit name and get the next token */

  g_stringSP = saveTknStart;
  getToken();

  /* Skip over the semicolon separating the unit-heading from the
   * interface-section.
   */

  if (g_token != ';') error(eSEMICOLON);
  else getToken();

  /* Verify that the interface-section is present
   * FORM: interface-section =
   *       'interface' [ uses-section ] interface-declaration
   */

  pas_InterfaceSection();

  /* Check for the presence of an implementation section */

  if (g_token == tIMPLEMENTATION)
    {
      /* Verify that the implementation section is present
       * FORM: implementation-section =
       *       'implementation' [ uses-section ] declaration-group
       */

      /* Skip over the implementation key word */

      FP->section = eIsImplementationSection;
      getToken();

      /* Check for the presence of an optional uses-section */

      if (g_token == tUSES)
        {
          /* Process the uses-section */

          getToken();
          pas_UsesSection();
        }

      /* Now, process the declaration-group */

      pas_DeclarationGroup(0);
    }

  /* Check for the presence of an initialization section
   *
   *   FORM: init-section = 'initialization' statement-sequence
   *
   * Or the Turbo Pascal form:
   *
   *   FORM: init-section = 'begin' statement-sequence
   *
   * No finalization section is supported in the Turbo Pascal form.
   * The BEGIN will not be terminated with 'END;' if a FINALIZATION
   * section is present.
   */

  if (g_token == tINITIALIZATION || g_token == tBEGIN)
    {
      FP->section = eIsInitializationSection;

      /* Process statements until END or FINALIZATION encountered. */

      do
         {
           getToken();
           pas_Statement();
         }
      while (g_token == ';');
    }

  /* Check for the presence of an finalization section
   *
   * FORM: finalization-section = 'finalization' statement-sequence
   */

  if (g_token == tFINALIZATION)
    {
      FP->section = eIsInitializationSection;
      getToken();

      /* REVISIT:  Finalization section is not implemented */

      error(eNOTYET);
    }

  /* And this shoud all be terminated with END and a period */

  if (g_token != tEND) error(eEND);
  else getToken();

  FP->section = eIsOtherSection;

  /* Verify that the unit file ends with a period */

  if (g_token != '.') error(ePERIOD);
}

/***********************************************************************/
/* This logic is called from pas_UsesSection after any uses-section is
 * encountered in any file at any level.
 *
 * Since we are generating a program binary, then all variables declared
 * by this logic a bonafide.  But if were generating UNIT binary, then
 * all variables declared as imported with a relative stack offset.
 * In this case, we must release any data stack allocated in this
 * process.
 */

void pas_UnitInterface(void)
{
  /* FORM: unit =
   *       unit-heading ';' interface-section implementation-section
   *       init-section
   * FORM: unit-heading = 'unit' identifer
   *
   * On entry, the 'unit' keyword has already been parsed.  The
   * current token should point to the identifier following unit.
   */

  /* Skip over the unit identifier (the caller has already verified
   * that we are processing the correct unit).
   */

  if (g_token != tIDENT) error(eIDENT);
  else getToken();

  /* Skip over the semicolon separating the unit-heading from the
   * interface-section.
   */

  if (g_token != ';') error(eSEMICOLON);
  else getToken();

  /* Process the interface-section
   * FORM: interface-section =
   *       'interface' [ uses-section ] interface-declaration
   */

  pas_InterfaceSection();

  /* Verify that the implementation section is present
   * FORM: implementation-section =
   *       'implementation' [ uses-section ] declaration-group
   */

  if (g_token != tIMPLEMENTATION) error(eIMPLEMENTATION);

  /* Then just ignore the rest of the file.  We'll let the compilation
   * of the unit file check the correctness of the implementation.
   */

  FP->section = eIsOtherSection;
}
