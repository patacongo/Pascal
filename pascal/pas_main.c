/***************************************************************************
 * pas_main.c
 * Main process
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
 ***************************************************************************/

/***************************************************************************
 * Included Files
 ***************************************************************************/

#define _GNU_SOURCE
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#include "config.h"
#include "pas_debug.h"
#include "pas_defns.h"
#include "pas_tkndefs.h"
#include "pas_pcode.h"
#include "pas_errcodes.h"

#include "pofflib.h"       /* For poffInitializeForOutput() */
#include "poff.h"          /* For POFF definitions */
#include "pas_machine.h"   /* Characteristics of the virtual machine */

#include "pas_main.h"
#include "paslib.h"        /* For extension */
#include "pas_token.h"     /* For pas_PrimeTokenizer */
#include "pas_symtable.h"  /* For pas_PrimeSymbolTable */
#include "pas_program.h"   /* for pas_Program() */
#include "pas_unit.h"      /* for unit() */
#include "pas_error.h"     /* for error() */

/***************************************************************************
 * Public Data
 ***************************************************************************/

uint16_t    g_token;                 /* Current token */
uint16_t    g_tknSubType;            /* Extended token type */
uint32_t    g_tknUInt;               /* Integer token value */
double      g_tknReal;               /* Real token value */
symbol_t   *g_tknPtr;                /* Pointer to symbol token*/
fileState_t g_fileState[MAX_INCL];   /* State of all open files */

/* g_sourceFileName : Program name from command line
 * g_includePath[] : Pathes to search when including file
 */

char       *g_sourceFileName;
char       *g_includePath[MAX_INCPATHES];

poffHandle_t g_poffHandle;           /* Handle for POFF object */

FILE       *g_poffFile;              /* Pass1 POFF output file */
FILE       *g_lstFile;               /* List File pointer */
FILE       *g_errFile;               /* Error file pointer */

with_t      g_withRecord;            /* RECORD used with WITH statement */
int16_t     g_level        = 0;      /* Static nesting level */
int16_t     g_includeIndex = 0;      /* Include file index */
int16_t     g_nIncPathes   = 0;      /* Number pathes in g_includePath[] */
uint16_t    g_label        = 0;      /* Last label number */
int16_t     g_errCount     = 0;      /* Error counter */
int32_t     g_warnCount    = 0;      /* Warning counter */
int32_t     g_dStack       = 0;      /* Data stack size */

/***************************************************************************
 * Private Type Definitions
 ***************************************************************************/

struct outFileDesc_s
{
  const char *extension;
  const char *flags;
  FILE      **stream;
};
typedef struct outFileDesc_s outFileDesc_t;

/***************************************************************************
 * Private Variables
 ***************************************************************************/

static const outFileDesc_t g_outFiles[] =
{
  { "o1",  "wb", &g_poffFile },    /* Pass 1 POFF object file */
#if LSTTOFILE
  { "lst", "w",  &g_lstFile },     /* List file */
#endif
  { "err", "w",  &g_errFile },     /* Error file */
  { NULL,  NULL }                  /* (terminates list */
};

static const char *g_programName;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void pas_CloseFiles(void);
static void pas_OpenOutputFiles(void);
static void pas_ShowUsage(void);
static void pas_ParseArguments(int argc, char **argv);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void pas_CloseFiles(void)
{
  const outFileDesc_t *outFile;

  /* Close input source files */

  for(; g_includeIndex >= 0; g_includeIndex--)
    {
      if (FP->stream)
        {
          (void)fclose(FP->stream);
          FP->stream = NULL;
        }
    }

  /* Close output files */

  for (outFile = g_outFiles; outFile->extension; outFile++)
    {
      if (*outFile->stream)
        {
          (void)fclose(*outFile->stream);
          *outFile->stream = NULL;
        }
    }
}

/****************************************************************************/

static void pas_OpenOutputFiles(void)
{
  const outFileDesc_t *outFile;
  char tmpname[PATH_MAX];

  /* Open output files */

  for (outFile = g_outFiles; outFile->extension; outFile++)
    {
      /* Generate an output file name from the source file
       * name and an extension associated with the output file.
       */

      (void)extension(g_sourceFileName, outFile->extension, tmpname,
                      PATH_MAX, 1);
      *outFile->stream = fopen(tmpname, outFile->flags);
      if (*outFile->stream == NULL)
        {
          fprintf(stderr, "Could not open output file '%s': %s\n",
                  tmpname, strerror(errno));
          pas_ShowUsage();
        }
    }
}

/****************************************************************************/

static void signalHandler(int signo)
{
#ifdef  _GNU_SOURCE
  fprintf(g_errFile, "Received signal: %s\n", strsignal(signo));
  fprintf(g_lstFile, "Received signal: %s\n", strsignal(signo));
#else
  fprintf(g_errFile, "Received signal %d\n", signo);
  fprintf(g_lstFile, "Received signal %d\n", signo);
#endif
  pas_CloseFiles();
  error(eRCVDSIGNAL);
  exit(1);
}

/****************************************************************************/
/* NOTE that some signals are not available in the NuttX build */

static void primeSignalHandlers(void)
{
#ifndef CONFIG_PASCAL_BUILD_NUTTX
  (void)signal(SIGHUP,  signalHandler);
  (void)signal(SIGILL,  signalHandler);
  (void)signal(SIGABRT, signalHandler);
  (void)signal(SIGSEGV, signalHandler);
#endif

  (void)signal(SIGINT,  signalHandler);
  (void)signal(SIGQUIT, signalHandler);
  (void)signal(SIGTERM, signalHandler);
}

/****************************************************************************/

static void pas_ShowUsage(void)
{
  fprintf(stderr, "USAGE:\n");
  fprintf(stderr, "  %s [OPTIONS] <program-filename>\n", g_programName);
  fprintf(stderr, "[OPTIONS]\n");
  fprintf(stderr, "  -I<include-path>\n");
  fprintf(stderr, "    Search in <include-path> for additional Unit files\n");
  fprintf(stderr, "    needed byte program file.\n");
  fprintf(stderr, "    A maximum of %d pathes may be specified\n",
          MAX_INCPATHES);
  fprintf(stderr, "    (default is current directory)\n");
  pas_CloseFiles();
  exit(1);
}

/****************************************************************************/

static void pas_ParseArguments(int argc, char **argv)
{
  int i;

  g_programName = argv[0];

  /* Check for existence of at least the filename argument */

  if (argc < 2)
    {
      fprintf(stderr, "Invalid number of arguments\n");
      pas_ShowUsage();
    }

  /* Parse any optional command line arguments */

  for (i = 1; i < argc-1; i++)
    {
      char *ptr = argv[i];
      if (ptr[0] == '-')
        {
          switch (ptr[1])
            {
            case 'I' :
              if (g_nIncPathes >= MAX_INCPATHES)
                {
                  fprintf(stderr, "Unrecognized [option]\n");
                  pas_ShowUsage();
                }
              else
                {
                  g_includePath[g_nIncPathes] = &ptr[2];
                  g_nIncPathes++;
                }
              break;

            default:
              fprintf(stderr, "Unrecognized [option]\n");
              pas_ShowUsage();
            }
        }
      else
        {
          fprintf(stderr, "Unrecognized [option]\n");
          pas_ShowUsage();
        }
    }

  /* Extract the Pascal program name from the command line */

  g_sourceFileName = argv[argc - 1];
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char *argv[])
{
  char filename[PATH_MAX];

  /* Parse command line arguments */

  pas_ParseArguments(argc, argv);

  /* Open all output files */

  pas_OpenOutputFiles();

#if !LSTTOFILE
  g_lstFile = stdout;
#endif

  /* Open source file -- Use .PAS or command line extension, if supplied */

  (void)extension(g_sourceFileName, "PAS", filename, PATH_MAX, 0);
  fprintf(g_errFile, "%01x=%s\n", FP->include, filename);

  memset(FP, 0, sizeof(fileState_t));
  FP->stream = fopen(filename, "r");
  if (!FP->stream)
    {
      errmsg("Could not open source file '%s': %s\n",
             filename, strerror(errno));
      pas_ShowUsage();
    }

  /* Initialization */

  primeSignalHandlers();
  pas_PrimeSymbolTable(MAX_SYM);
  pas_PrimeTokenizer(MAX_STRINGS);

  /* Initialize the POFF object */

  g_poffHandle = poffCreateHandle();
  if (g_poffHandle == NULL)
    {
      fatal(eNOMEMORY);
    }

  /* Save the soure file name in the POFF output file */

  FP->include = poffAddFileName(g_poffHandle, filename);

  /* We need the following in order to calculate relative stack positions. */

  FP->dstack       = g_dStack;

  /* Indicate that no WITH statement has been processed */

  memset(&g_withRecord, 0, sizeof(with_t));

  /* Process the pascal program
   *
   * FORM: pascal = program | unit
   * FORM: program = program-heading ';' [uses-section ] block '.'
   * FORM: program-heading = 'program' identifier [ '(' identifier-list ')' ]
   * FORM: unit = unit-heading ';' interface-section implementation-section init-section
   * FORM: unit-heading = 'unit' identifer
   */

  getToken();
  if (g_token == tPROGRAM)
    {
      /* Compile a pascal program */

      FP->kind    = eIsProgram;
      FP->section = eIsProgramSection;
      getToken();
      pas_Program();
    }
  else if (g_token == tUNIT)
    {
      /* Compile a pascal unit */

      FP->kind    = eIsUnit;
      FP->section = eIsOtherSection;
      getToken();
      pas_UnitImplementation();
    }
  else
    {
      /* Expected 'program' or 'unit' */

      error(ePROGRAM);
    }

  /* Dump the symbol table content (debug only) */

#if CONFIG_DEBUG
  pas_DumpTables();
#endif

  /* Write the POFF output file */

  poffWriteFile(g_poffHandle, g_poffFile);
  poffDestroyHandle(g_poffHandle);

  /* Close all output files */

  pas_CloseFiles();

  /* Write Closing Message */

  if (g_warnCount > 0)
    {
      printf("  %" PRId32 " Warnings Issued\n", g_warnCount);
    }

  if (g_errCount > 0)
    {
      printf("  %" PRId32 " Errors Detected\n\n", g_errCount);
      return -1;
    }

  return 0;
}

/****************************************************************************/

void pas_OpenNestedFile(const char *fileName)
{
  fileState_t *prev = FP;
  char fullpath[PATH_MAX];
  int i;

  /* Make sure we can handle another nested file */

  if (++g_includeIndex >= MAX_INCL) fatal(eOVF);
  else
    {
      /* Clear the file state structure for the new include level */

      memset(FP, 0, sizeof(fileState_t));

      /* Try all source include pathes until we find the file or
       * until we exhaust the include path list.
       */

      for (i = 0; ; i++)
        {
          /* Open the nested file -- try all possible pathes or
           * until we successfully open the file.
           */

          /* The final path that we will try is the current directory */

          if (i == g_nIncPathes)
            {
              snprintf(fullpath, PATH_MAX, "./%s", fileName);
            }
          else
            {
              snprintf(fullpath, PATH_MAX, "%s/%s", g_includePath[i],
                       fileName);
            }

          FP->stream = fopen (fullpath, "rb");
          if (!FP->stream)
            {
              /* We failed to open the file.  If there are no more
               * include pathes to examine (including the current directory),
               * then error out.  This is fatal.  Otherwise, continue
               * looping.
               */

              if (i == g_nIncPathes)
                {
                  errmsg("Failed to open '%s': %s\n",
                         fileName, strerror(errno));
                  fatal(eINCLUDE);
                  break; /* Won't get here */
                }
            }
          else
            {
              break;
            }
        }

      /* Setup the newly opened file */

      fprintf(g_errFile, "%01x=%s\n", FP->include, fullpath);
      FP->include = poffAddFileName(g_poffHandle, fullpath);

      /* The caller may change this, but the default behavior is
       * to inherit the kind and section of the including file
       * and the current data stack offset.
       */

      FP->kind    = prev->kind;
      FP->section = prev->section;
      FP->dstack  = g_dStack;

      pas_RePrimeTokenizer();

      /* Get the first token from the file */

      getToken();
    }
}

/****************************************************************************/

void pas_CloseNestedFile(void)
{
  if (FP->stream)
    {
      (void)fclose(FP->stream);
      g_includeIndex--;
    }
}
