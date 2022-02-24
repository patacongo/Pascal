/****************************************************************************
 * pstart_main.c
 *
 *   Copyright (C) 2022 Gregory Nutt. All rights reserved.
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

#include <stdio.h>
#include <stdlib.h>

#include "paslib.h"
#include "pas_errcodes.h"

#include "pexec.h"
#include "plib.h"
#include "pdbg.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pstart_execute
 *
 * Description:
 *   This function executes the P-Code program until a stopping condition
 *   is encountered.
 *
 ****************************************************************************/

static void pstart_execute(struct pexec_s *st)
{
  int errcode;

  for (; ; )
    {
      /* Execute the instruction; Check for exceptional conditions */

      errcode = pexec_Execute(st);
      if (errcode != eNOERROR) break;
    }

  if (errcode == eEXIT)
    {
      printf("Exit with code %d\n", g_exitCode);
    }
  else
    {
      printf("Runtime error 0x%02x -- Execution Stopped\n", errcode);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 *
 * Description:
 *  This is the application entry when so configured.  It will start an
 *  initial Pascal program.
 *
 ****************************************************************************/

int main(int argc, char *argv[], char *envp[])
{
  struct pexec_s *st;
  char fileName[FNAME_SIZE + 1];  /* Object file name */

  /* Load the POFF files specified on the command line */
  /* Use .o or command line extension, if supplied */

  (void)extension(CONFIG_PASCAL_STARTUP_FILENAME, "o", fileName, 0);

  /* Initialize the P-machine and load the POFF file */

  st = pexec_Load(fileName, g_strallocsize, g_strstacksize, g_passtacksize,
                  g_hpstacksize);
  if (st == NULL)
    {
      fprintf(stderr, "ERROR: Could not load %s\n", fileName);
      exit(1);
    }

  printf("%s Loaded\n", fileName);

  /* And start program execution in the specified mode */

#ifdef CONFIG_PASCAL_STARTUP_DEBUG
  dbg_run(st);
#else
  pstart_execute(st);
#endif

  /* Clean up resources used by the interpreter */

  pexec_Release(st);
  return 0;
}
