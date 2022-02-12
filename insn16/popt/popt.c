/****************************************************************************
 * popt.c
 * P-Code Optimizer Main Logic
 *
 *   Copyright (C) 2008-2009, 2022 Gregory Nutt. All rights reserved.
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
#include <stdio.h>
#include <stdlib.h>

#include "pas_debug.h"
#include "pas_pcode.h"
#include "paslib.h"
#include "pofflib.h"

#include "pas_insn.h"
#include "popt.h"
#include "popt_strings.h"
#include "popt_local.h"
#include "popt_reloc.h"
#include "popt_finalize.h"

/**********************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void readPoffFile  (const char *filename);
static void pass1         (void);
static void pass2         (void);
static void pass3         (void);
static void writePoffFile (const char *filename);

/**********************************************************************
 * Private Variables
 ****************************************************************************/

static poffHandle_t g_poffHandle; /* Handle to POFF object */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************/

static void readPoffFile(const char *filename)
{
  char    objname [FNAME_SIZE+1];
  FILE   *objFile;
  uint16_t errcode;

  TRACE(stderr, "[readPoffFile]");

  /* Open the pass1 POFF object file -- Use .o1 extension */

  (void)extension(filename, "o1", objname, 1);
  if (!(objFile = fopen(objname, "rb")))
    {
      fprintf(stderr, "ERROR: Error Opening %s\n", objname);
      exit(1);
    }

  /* Get a handle to a POFF input object */

  g_poffHandle = poffCreateHandle();
  if (g_poffHandle == NULL)
    {
      fprintf(stderr, "ERROR: Could not get POFF handle\n");
      exit(1);
    }

  /* Read the POFF file into memory */

  errcode = poffReadFile(g_poffHandle, objFile);
  if (errcode != 0)
    {
      fprintf(stderr, "ERROR: Could not read POFF file, errcode=0x%02x\n",
              errcode);
      exit(1);
    }

  /* Close the input file */

  fclose(objFile);
}

/****************************************************************************/

static void pass1(void)
{
  poffProgHandle_t poffProgHandle; /* Handle to temporary POFF object */

  TRACE(stderr, "[pass1]");

  /* Create a handle to a temporary object to store new POFF program
   * data.
   */

  poffProgHandle = poffCreateProgHandle();
  if (!poffProgHandle)
    {
      fprintf(stderr, "ERROR: Could not get POFF handle\n");
      exit(1);
    }

  /* Clean up garbage left from the wasteful string stack logic */

  popt_StringStackOptimize(g_poffHandle, poffProgHandle);

  /* Replace the original program data with the new program data */

  poffReplaceProgData(g_poffHandle, poffProgHandle);

  /* Release the temporary POFF object */

  poffDestroyProgHandle(poffProgHandle);
}

/****************************************************************************/

static void pass2(void)
{
  poffProgHandle_t poffProgHandle; /* Handle to temporary POFF object */

  TRACE(stderr, "[pass2]");

  /* Create a handle to a temporary object to store new POFF program
   * data.
   */

  poffProgHandle = poffCreateProgHandle();
  if (!poffProgHandle)
    {
      fprintf(stderr, "ERROR: Could not get POFF handle\n");
      exit(1);
    }

  /* Swap the relocation container handles.  The relocations accumulated
   * in "current" container are now the relocations from the "previous" pass.
   * The "current" container will be empty at the start of the pass.
   */

  swapRelocationHandles();

  /* Perform Local Optimizatin Initialization */

  popt_LocalOptimization(g_poffHandle, poffProgHandle);

  /* Replace the original program data with the new program data */

  poffReplaceProgData(g_poffHandle, poffProgHandle);

  /* Release the temporary POFF object */

  poffDestroyProgHandle(poffProgHandle);
}

/****************************************************************************/

static void pass3 (void)
{
  poffProgHandle_t poffProgHandle; /* Handle to temporary POFF object */
  TRACE(stderr, "[pass3]");

  /* Create a handle to a temporary object to store new POFF program
   * data.
   */

  poffProgHandle = poffCreateProgHandle();
  if (!poffProgHandle)
    {
      fprintf(stderr, "ERROR: Could not get POFF handle\n");
      exit(1);
    }

  /* Finalize program section, create relocation and line number
   * sections.
   */

  optFinalize(g_poffHandle, poffProgHandle);

  /* Release the temporary POFF object */

  poffDestroyProgHandle(poffProgHandle);
}

/****************************************************************************/

static void writePoffFile(const char *filename)
{
  char  optname [FNAME_SIZE+1];
  FILE *optFile;

  TRACE(stderr, "[writePoffFile]");

  /* Open optimized p-code file -- Use .o extension */

  (void)extension(filename, "o", optname, 1);
  if (!(optFile = fopen(optname, "wb")))
    {
      fprintf(stderr, "ERROR: Error Opening %s\n", optname);
      exit(1);
    }

  /* Then write the new POFF file */

  poffWriteFile(g_poffHandle, optFile);

  /* Destroy the POFF object */

  poffDestroyHandle(g_poffHandle);

  /* Close the files used on writePoffFile */

  (void)fclose(optFile);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************/

int main(int argc, char *argv[], char *envp[])
{
  TRACE(stderr, "[main]");

  /* Check for existence of filename argument */

  if (argc < 2)
    {
      fprintf(stderr, "ERROR: Filename Required\n");
      exit (1);
    }

  /* Read the POFF file into memory */

  readPoffFile(argv[1]);

  /* Initialize relocation support */

  createRelocationHandles(g_poffHandle);

  /* Performs pass1 optimization */

  pass1();

  /* Performs pass2 optimization */

  insn_ResetOpCodeRead(g_poffHandle);
  pass2();

  /* Create final section offsets and relocation entries */

  insn_ResetOpCodeRead(g_poffHandle);
  pass3();

  /* Write the POFF file */

  writePoffFile(argv[1]);

  /* And clean up */

  destroyRelocationHandles();
  return 0;
}
