/****************************************************************************
 * prun.c
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "pas_debug.h"
#include "pas_machine.h"
#include "pinsn16.h"
#include "pas_sysio.h"
#include "pas_errcodes.h"

#include "paslib.h"
#include "pas_error.h"
#include "pexec.h"
#include "pdbg.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MIN_STACK_SIZE       1024
#define DEFAULT_STACK_SIZE   4096
#define DEFAULT_STKSTR_SIZE     0
#define DEFAULT_HPSTK_SIZE      0

/****************************************************************************
 * Private Constant Data
 ****************************************************************************/

static const struct option long_options[] =
{
  {"alloc",  1, NULL, 'a'},
  {"stack",  1, NULL, 's'},
  {"string", 1, NULL, 't'},
  {"new",    1, NULL, 'n'},
  {"debug",  0, NULL, 'd'},
  {"help",   0, NULL, 'h'},
  {NULL,     0, NULL, 0}
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char  *g_pofffilename;
static int32_t      g_strstacksize = DEFAULT_STKSTR_SIZE;
static int32_t      g_passtacksize = DEFAULT_STACK_SIZE;
static int32_t      g_hpstacksize  = DEFAULT_HPSTK_SIZE;
static int32_t      g_strallocsize = STRING_BUFFER_SIZE;
static int          g_debug        = 0;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: prun_showusage
 ****************************************************************************/

static void prun_showusage(const char *progname)
{
  fprintf(stderr, "Usage:\n");
  fprintf(stderr, "  %s [options] <program-filename>\n",
          progname);
  fprintf(stderr, "options:\n");
  fprintf(stderr, "  -a <string-buffer-size>\n");
  fprintf(stderr, "  --alloc <string-buffer-size>\n");
  fprintf(stderr, "    Size of the string buffer to be allocated whenever a\n");
  fprintf(stderr, "    'string' variable is created (default: %d)\n",
          STRING_BUFFER_SIZE);
  fprintf(stderr, "  -s <stack-size>\n");
  fprintf(stderr, "  --stack <stack-size>\n");
  fprintf(stderr, "    Memory in bytes to allocate for the pascal program\n");
  fprintf(stderr, "    stack in bytes (minimum is %d; default is %d bytes)\n",
          MIN_STACK_SIZE, DEFAULT_STACK_SIZE);
  fprintf(stderr, "  -t <string-storage-size>\n");
  fprintf(stderr, "  --string <string-storage-size>\n");
  fprintf(stderr, "    Memory in bytes to allocate for the pascal program\n");
  fprintf(stderr, "    string storage in bytes (default is %d bytes)\n",
          DEFAULT_STKSTR_SIZE);
  fprintf(stderr, "  -n <heap-size>\n");
  fprintf(stderr, "  --new <heap-size>\n");
  fprintf(stderr, "    Memory in bytes to allocate for the pascal program\n");
  fprintf(stderr, "    head use for new() (default is %d bytes)\n",
          DEFAULT_HPSTK_SIZE);
  fprintf(stderr, "  -d\n");
  fprintf(stderr, "  --debug\n");
  fprintf(stderr, "    Enable PCode program debugger\n");
  fprintf(stderr, "  -h\n");
  fprintf(stderr, "  --help\n");
  fprintf(stderr, "    Shows this message\n");
  exit(1);
}

/****************************************************************************
 * Name: prun_parseargs
 ****************************************************************************/

static void prun_parseargs(int argc, char **argv)
{
  int option_index;
  int alloc;
  int size;
  int c;

  /* Check for existence of filename argument */

  if (argc < 2)
    {
      fprintf(stderr, "ERROR: Filename required\n");
      prun_showusage(argv[0]);
    }

  /* Parse the command line options */

  do
    {
      c = getopt_long(argc, argv, "a:t:s:n:dh",
                      long_options, &option_index);
      if (c != -1)
        {
          switch (c)
            {
            case 'a' :
              alloc = atoi(optarg);
              if (alloc < 0)
                {
                  fprintf(stderr, "ERROR: String buffer size\n");
                  prun_showusage(argv[0]);
                }

              g_strallocsize = (alloc + 1) & ~1;
              break;

            case 'n' :
              size = atoi(optarg);
              if (size < 0)
                {
                  fprintf(stderr, "ERROR: Invalid heap size\n");
                  prun_showusage(argv[0]);
                }

              g_hpstacksize = ((size + 1) & ~1);
              break;

            case 's' :
              size = atoi(optarg);
              if (size < MIN_STACK_SIZE)
                {
                  fprintf(stderr, "ERROR: Invalid stack size\n");
                  prun_showusage(argv[0]);
                }

              g_passtacksize = (size + 3) & ~3;
              break;

            case 't' :
              size = atoi(optarg);
              if (size < 0)
                {
                  fprintf(stderr, "ERROR: Invalid string storage size\n");
                  prun_showusage(argv[0]);
                }

              g_strstacksize = ((size + 3) & ~3);
              break;

            case 'd' :
              g_debug++;
              break;

            case 'h' :
              prun_showusage(argv[0]);
              break;

            default:
              /* Shouldn't happen */

              fprintf(stderr, "ERROR: Unrecognized option\n");
              prun_showusage(argv[0]);
            }
        }
    }
  while (c != -1);

  if (optind != argc-1)
    {
      fprintf(stderr, "ERROR: Only one filename permitted on command line\n");
      prun_showusage(argv[0]);
    }

  /* Get the name of the p-code file(s) from the last argument(s) */

  g_pofffilename = argv[argc-1];
}

/****************************************************************************
 * Name: prun
 *
 * Description:
 *   This function executes the P-Code program until a stopping condition
 *   is encountered.
 *
 ****************************************************************************/

static void prun(struct pexec_s *st)
{
  int errcode;

  for (; ; )
    {
      /* Execute the instruction; Check for exceptional conditions */

      errcode = pexec_Execute(st);
      if (errcode != eNOERROR) break;
    }

  if (errcode != eEXIT)
    {
      printf("Runtime error 0x%02x -- Execution Stopped\n", errcode);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, char *argv[], char *envp[])
{
  struct pexec_s *st;
  char fileName[FNAME_SIZE+1];  /* Object file name */

  /* Parse the command line arguments */

  prun_parseargs(argc, argv);

  /* Load the POFF files specified on the command line */
  /* Use .o or command line extension, if supplied */

  (void)extension(g_pofffilename, "o", fileName, 0);

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

  if (g_debug)
    {
      dbg_run(st);
    }
  else
    {
      prun(st);
    }

  /* Clean up resources used by the interpreter */

  pexec_Release(st);
  return 0;
}
