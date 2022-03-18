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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "paslib.h"
#include "execlib.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MIN_STACK_SIZE       1024
#define DEFAULT_STACK_SIZE   4096
#define DEFAULT_STKSTR_SIZE     0
#define DEFAULT_HPSTK_SIZE      0

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct prunArgs_s
{
  const char *poffFileName;  /* Input POFF file name */
  int32_t     strStackSize;  /* String stack size to allocate */
  int32_t     pasStackSize;  /* Pascal run-time stack to allocate */
  int32_t     hpStackSize;   /* Heap memory to allocate */
#ifdef CONFIG_PASCAL_DEBUGGER
  int         debugger;      /* > 0:  Run the debug monitor */
#endif
};

typedef struct prunArgs_s prunArgs_t;

/****************************************************************************
 * Private Constant Data
 ****************************************************************************/

static const struct option long_options[] =
{
  {"alloc",  1, NULL, 'a'},
  {"stack",  1, NULL, 's'},
  {"string", 1, NULL, 't'},
  {"new",    1, NULL, 'n'},
#ifdef CONFIG_PASCAL_DEBUGGER
  {"debug",  0, NULL, 'd'},
#endif
  {"help",   0, NULL, 'h'},
  {NULL,     0, NULL, 0}
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: prun_showusage
 ****************************************************************************/

static void prun_showusage(const char *progname)
{
  fprintf(stderr, "USAGE:\n");
  fprintf(stderr, "  %s [OPTIONS] <program-filename>\n",
          progname);
  fprintf(stderr, "OPTIONS:\n");
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
  fprintf(stderr, "    heap use for new() and temporary strings (default is\n");
  fprintf(stderr, "    %d bytes)\n",
          DEFAULT_HPSTK_SIZE);
#ifdef CONFIG_PASCAL_DEBUGGER
  fprintf(stderr, "  -d\n");
  fprintf(stderr, "  --debug\n");
  fprintf(stderr, "    Enable PCode program debugger\n");
#endif
  fprintf(stderr, "  -h\n");
  fprintf(stderr, "  --help\n");
  fprintf(stderr, "    Shows this message\n");
  exit(1);
}

/****************************************************************************
 * Name: prun_ParseArgs
 ****************************************************************************/

static void prun_ParseArgs(int argc, char **argv, prunArgs_t *args)
{
  int option_index;
  int size;
  int c;

  /* Set up default value */

  args->poffFileName = NULL;
  args->strStackSize = DEFAULT_STKSTR_SIZE;
  args->pasStackSize = DEFAULT_STACK_SIZE;
  args->hpStackSize  = DEFAULT_HPSTK_SIZE;
#ifdef CONFIG_PASCAL_DEBUGGER
  args->debugger     = 0;
#endif

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
            case 'n' :
              size = atoi(optarg);
              if (size < 0)
                {
                  fprintf(stderr, "ERROR: Invalid heap size\n");
                  prun_showusage(argv[0]);
                }

              args->hpStackSize = ((size + 1) & ~1);
              break;

            case 's' :
              size = atoi(optarg);
              if (size < MIN_STACK_SIZE)
                {
                  fprintf(stderr, "ERROR: Invalid stack size\n");
                  prun_showusage(argv[0]);
                }

              args->pasStackSize = (size + 3) & ~3;
              break;

            case 't' :
              size = atoi(optarg);
              if (size < 0)
                {
                  fprintf(stderr, "ERROR: Invalid string storage size\n");
                  prun_showusage(argv[0]);
                }

              args->strStackSize = ((size + 3) & ~3);
              break;

#ifdef CONFIG_PASCAL_DEBUGGER
            case 'd' :
              args->debugger++;
              break;
#endif
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

  args->poffFileName = argv[argc - 1];
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 ****************************************************************************/

int main(int argc, char *argv[], char *envp[])
{
  EXEC_HANDLE_t handle;
  char fileName[FNAME_SIZE + 1];  /* Object file name */
  prunArgs_t args;

  /* Parse the command line arguments */

  prun_ParseArgs(argc, argv, &args);

  /* Load the POFF files specified on the command line */
  /* Use .o or command line extension, if supplied */

  (void)extension(args.poffFileName, "o", fileName, FNAME_SIZE + 1, 0);

  /* Initialize the P-machine and load the POFF file */

  handle = libexec_Load(fileName, args.strStackSize, args.pasStackSize,
                        args.hpStackSize);
  if (handle == NULL)
    {
      fprintf(stderr, "ERROR: Could not load %s\n", fileName);
      exit(1);
    }

  printf("%s Loaded\n", fileName);

  /* And start program execution in the specified mode */

#ifdef CONFIG_PASCAL_DEBUGGER
  if (args.debugger)
    {
      libexec_DebugLoop(handle);
    }
  else
#endif
    {
      libexec_RunLoop(handle);
    }

  /* Clean up resources used by the interpreter */

  libexec_Release(handle);
  return 0;
}
