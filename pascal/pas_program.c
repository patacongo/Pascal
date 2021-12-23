/**********************************************************************
 * pas_program.c
 * main - process PROGRAM
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
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#include "pas_debug.h"
#include "pas_defns.h"
#include "pas_tkndefs.h"
#include "pas_pcode.h"
#include "pas_errcodes.h"
#include "poff.h"            /* FHT_ definitions */

#include "pas_main.h"        /* for globals + pas_OpenNestedFile */
#include "pas_block.h"       /* for pas_Block() */
#include "pas_initializer.h" /* for pas_AddFileInitializer() */
#include "pas_codegen.h"     /* for pas_Generate*() */
#include "pas_token.h"       /* for getToken() */
#include "pas_symtable.h"    /* for pas_AddFile() */
#include "pofflib.h"         /* For poff*() functions*/
#include "paslib.h"          /* for extension() */
#include "pas_error.h"       /* for error() */
#include "pas_unit.h"        /* for unit() */
#include "pas_program.h"

/***********************************************************************
 * Public Functions
 ***********************************************************************/

void pas_Program(void)
{
  char *programName = NULL;
  uint16_t nFiles   = 2;

  TRACE(g_lstFile, "[pas_Program]");

  /* FORM: program = program-heading ';' [uses-section ] block '.'
   * FORM: program-heading = 'program' identifier [ '(' identifier-list ')' ]
   *
   * On entry, 'program' has already been identified and g_token refers to
   * the next token after 'program'
   */

  if (g_token != tIDENT) error(eIDENT);      /* Verify <program name> */
  else
    {
      programName = g_tokenString;           /* Save program name */
      getToken();
    }

  /* Process optional file list (allow re-declaration of INPUT & OUTPUT) */

  if (g_token == '(')
    {
      do
        {
          /* Each file should appear as an identifier and will be assigned
           * file numbers dynamically.
           */

          getToken();
          if (g_token == tIDENT)
            {
              symbol_t *filePtr;

              filePtr = pas_AddFile(g_tokenString, sTEXTFILE, g_dStack,
                                    sCHAR_SIZE, NULL);
              nFiles++;
              pas_AddFileInitializer(filePtr, false, 0);
              g_dStack  += sINT_SIZE;
              g_stringSP = g_tokenString;
              getToken();
            }

          /* INPUT and OUTPUT will appear as variable symbols since they were
           * pre-defined (non-standard).
           */

          else if (g_token == sFILE || g_token == sTEXTFILE)
            {
              getToken();
            }
          else
            {
              error(eIDENT);
            }
        }
      while (g_token == ',');

      if (g_token != ')') error(eRPAREN);
      else getToken();
    }

  /* Make sure that a semicolon follows the program-heading */

  if (g_token != ';') error(eSEMICOLON);
  else getToken();

  /* Set the POFF file header type */

  poffSetFileType(poffHandle, FHT_PROGRAM, nFiles, programName);
  poffSetArchitecture(poffHandle, FHA_PCODE);

  /* Discard the program name string */

  g_stringSP = programName;

  /* Process the optional 'uses-section'
   * FORM: uses-section = 'uses' [ uses-unit-list ] ';'
   */

  if (g_token == tUSES)
    {
      getToken();
      pas_UsesSection();
    }

  /* Process the block */

  pas_Block(g_dStack);

  if (g_token !=  '.') error(ePERIOD);
  pas_GenerateSimple(opEND);
}

/***********************************************************************/

void pas_UsesSection(void)
{
  uint16_t saveToken;
  char defaultUnitFileName[FNAME_SIZE + 1];
  char *unitFileName = NULL;
  char *saveTknStrt;
  char *unitName;

  TRACE(g_lstFile, "[pas_UsesSection]");

  /* FORM: uses-section = 'uses' [ uses-unit-list ] ';'
   * FORM: uses-unit-list = unit-import {';' uses-unit-list }
   * FORM: unit-import = identifier ['in' non-empty-string ]
   *
   * On entry, g_token will point to the token just after
   * the 'uses' reservers word.
   */

  while (g_token == tIDENT)
    {
      /* Save the unit name identifier and skip over the identifier */

      unitName = g_tokenString;
      getToken();

      /* Check for the optional 'in' */

      saveTknStrt = g_tokenString;
      if (g_token == tIN)
        {
          /* Skip over 'in' and verify that a string constant representing
           * the file name follows.
           */

          getToken();
          if (g_token != tSTRING_CONST) error(eSTRING);
          else
            {
              /* Save the unit file name and skip to the
               * next token.
               */

              unitFileName = g_tokenString;
              saveTknStrt  = g_tokenString;
              getToken();
            }
        }

      /* If the file name is not present following the IN token, then form
       * the file name from the unit name with the extension .pas.
       */

      else
        {
          /* Create a default filename */

          (void)extension(unitName, "pas", defaultUnitFileName, 1);
          unitFileName = defaultUnitFileName;
        }

      /* Open the unit file */

      saveToken   = g_token;
      pas_OpenNestedFile(unitFileName);
      FP->kind    = eIsUnit;
      FP->section = eIsOtherSection;

      /* Verify that this is a unit file */

      if (g_token != tUNIT) error(eUNIT);
      else getToken();

      /* Release the file name from the string stack */

      g_stringSP = saveTknStrt;

      /* Verify that the file provides the unit that we are looking
       * for (only one unit per file is supported).
       *
       * Note that this is case sensitive.
       */

      if (g_token != tIDENT) error(eIDENT);
      else if (strcmp(unitName, g_tokenString) != 0) error(eUNITNAME);

      /* Parse the interface from the unit file (token must refer
       * to the unit name on entry into unit().
       */

      pas_UnitInterface();
      pas_CloseNestedFile();

      /* Verify the terminating semicolon */

      g_token = saveToken;
      if (g_token !=  ';') error(eSEMICOLON);
      else getToken();
    }
}

/***********************************************************************/
