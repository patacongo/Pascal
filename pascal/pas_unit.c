/**********************************************************************
 * pas_unit.c
 * Parse a pascal unit file
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

#include "pofflib.h"      /* For poff*() functions*/
#include "poff.h"         /* FHT_ definitions */

#include "pas_main.h"     /* for globals */
#include "pas_block.h"    /* for block(), constantDefinitionGroup(), etc. */
#include "pas_codegen.h"  /* for pas_Generate*() */
#include "pas_token.h"    /* for getToken() */
#include "pas_symtable.h" /* for addFile() */
#include "pas_error.h"    /* for error() */
#include "pas_program.h"  /* for usesSection() */
#include "pas_unit.h"

/***********************************************************************
 * Pre-processor Definitions
 ***********************************************************************/

#define intAlign(x)     (((x) + (sINT_SIZE-1)) & (~(sINT_SIZE-1)))

/***********************************************************************
 * Private Function Prototypes
 ***********************************************************************/

static void interfaceSection          (void);
static void exportedProcedureHeading  (void);
static void exportedFunctionHeading   (void);

/***********************************************************************
 * Public Functions
 ***********************************************************************/
/* This function is called only main() when the first token parsed out
 * the specified file is 'unit'.  In this case, we are parsing a unit file
 * and generating a unit binary.
 */

void unitImplementation(void)
{
  char   *saveTknStart = g_tokenString;

  TRACE(g_lstFile, "[unitImplementation]");

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

  poffSetFileType(poffHandle, FHT_UNIT, 0, g_tokenString);
  poffSetArchitecture(poffHandle, FHA_PCODE);

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

  interfaceSection();

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
          usesSection();
        }

      /* Now, process the declaration-group */

      declarationGroup(0);
    }

  /* Check for the presence of an initialization section
   *
   * FORM: init-section = 'initialization' statement-sequence
   */

  if (g_token == tINITIALIZATION)
    {
      FP->section = eIsInitializationSection;
      getToken();

      /* REVISIT:  Initialization section is not implemented */

      error(eNOTYET);
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
/* This logic is called from usersSection after any a uses-section is
 * encountered in any file at any level.  In this case, we are only
 * going to parse the interface section from the unit file.
 */

void unitInterface(void)
{
  int32_t savedDStack  = g_dStack;
  TRACE(g_lstFile, "[unitInterface]");

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

  interfaceSection();

  /* Verify that the implementation section is present
   * FORM: implementation-section =
   *       'implementation' [ uses-section ] declaration-group
   */

  if (g_token != tIMPLEMENTATION) error(eIMPLEMENTATION);

  /* Then just ignore the rest of the file.  We'll let the compilation
   * of the unit file check the correctness of the implementation.
   */

  FP->section = eIsOtherSection;

  /* If we are generating a program binary, then all variables declared
   * by this logic a bonafide.  But if are generating UNIT binary, then
   * all variables declared as imported with a relative stack offset.
   * In this case, we must release any data stack allocated in this
   * process.
   */

  g_dStack = savedDStack;
}

/***********************************************************************
 * Private Functions
 ***********************************************************************/

static void interfaceSection(void)
{
  unsigned int saveNSym        = g_nSym;             /* Save top of symbol table */
  unsigned int saveNConst      = g_nConst;           /* Save top of constant table */
  unsigned int saveSymOffset   = g_levelSymOffset;   /* Save previous level symbol offset */
  unsigned int saveConstOffset = g_levelConstOffset; /* Save previous level constant offset */

  TRACE(g_lstFile, "[interfaceSection]");

  /* Set the current symbol/constant table offsets for this level */

  g_levelSymOffset             = saveNSym;
  g_levelConstOffset           = saveNConst;

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
      usesSection();
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

       constantDefinitionGroup();

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

       typeDefinitionGroup();
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

       variableDeclarationGroup();
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

           exportedFunctionHeading();
         }

       /* FORM: procedure-heading =
        *       'procedure' procedure-identifier [ formal-parameter-list ]
        */

       else if (g_token == tPROCEDURE)
         {
           /* Limit search to present level */

           getLevelToken();

           /* Process the interface declaration */

           exportedProcedureHeading();
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

static void exportedProcedureHeading(void)
{
   uint16_t  procLabel = ++g_label;
   char     *saveChSp;
   symbol_t *procPtr;
   int       i;

   TRACE(g_lstFile,"[exportedProcedureHeading]");

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
     } /* endif */

   procPtr = addProcedure(g_tokenString, sPROC, procLabel, 0, NULL);

   /* Mark the procedure as external */

   procPtr->sParm.p.flags |= SPROC_EXTERNAL;

   /* Save the string stack pointer so that we can release all
    * formal parameter strings later.  Then get the next token.
    */

   saveChSp = g_stringSP;
   getToken();

   /* NOTE:  The level associated with the PROCEDURE symbol is the level
    * At which the procedure was declared.  Everything declare within the
    * PROCEDURE is at the next level
    */

   g_level++;

   /* Process parameter list */

   (void)formalParameterList(procPtr);

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

   for (i = 1; i <= procPtr->sParm.p.nParms; i++)
     {
       procPtr[i].sName = NULL;
     }

   g_stringSP = saveChSp;

   /* Drop the level back to where it was */

   g_level--;
}

/***************************************************************/
/* Process Function Declaration Block */

static void exportedFunctionHeading(void)
{
   uint16_t  funcLabel = ++g_label;
   int16_t   parameterOffset;
   char     *saveChSp;
   symbol_t *funcPtr;
   int       i;

   TRACE(g_lstFile,"[exportedFunctionHeading]");

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
     } /* endif */

   funcPtr = addProcedure(g_tokenString, sFUNC, funcLabel, 0, NULL);

   /* Mark the procedure as external */

   funcPtr->sParm.p.flags |= SPROC_EXTERNAL;

   /* NOTE:  The level associated with the FUNCTION symbol is the level
    * At which the procedure was declared.  Everything declare within the
    * PROCEDURE is at the next level
    */

   g_level++;

   /* Save the string stack pointer so that we can release all
    * formal parameter strings later.  Then get the next token.
    */

   saveChSp = g_stringSP;
   getToken();

   /* Process parameter list */

   parameterOffset = formalParameterList(funcPtr);

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

       parameterOffset        -= g_tknPtr->sParm.t.rsize;
       parameterOffset         = intAlign(parameterOffset);

       /* Save the TYPE for the function */

       funcPtr->sParm.p.parent = g_tknPtr;

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

   for (i = 1; i <= funcPtr->sParm.p.nParms; i++)
     {
       funcPtr[i].sName = ((char *) NULL);
     }

   g_stringSP = saveChSp;

   /* Restore the original level */

   g_level--;
}
