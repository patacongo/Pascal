/****************************************************************************
 * pas_error.c
 * Error Handlers
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
#include <stdarg.h>
#include <inttypes.h>

#include "config.h"
#include "pas_debug.h"
#include "pas_defns.h"
#include "pas_errcodes.h"

#include "pas_main.h"
#include "pas_token.h"
#include "pas_error.h"
#if CONFIG_DEBUG
#  include "pas_symtable.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if CONFIG_DEBUG
#  define DUMPTABLES pas_DumpTables()
#else
#  define DUMPTABLES
#endif

/****************************************************************************
 * Private Types
 ****************************************************************************/

enum errLevel_s
{
  ERROR_LEVEL_WARNING = 0,
  ERROR_LEVEL_ERROR,
  ERROR_LEVEL_FATAL,
  ERROR_NUM_LEVELS
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const char g_fmtErrNoToken[] =
   "Line %d:%04" PRIu32 " %s %02x Token %02x\n";
static const char g_fmtErrWithToken[] =
   "Line %d:%04" PRIu32 " %s %02x Token %02x (%s)\n";
static const char g_fmtErrAbort[] =
   "Fatal Error %d -- Compilation aborted\n";

static const char *g_errLevelStrings[ERROR_NUM_LEVELS] =
{
  "Warning",
  "Error",
  "Fatal"
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static void pas_PrintError(uint16_t errCode, int errLevel);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************/

static void pas_PrintError(uint16_t errCode, int errLevel)
{
  /* Write error record to the error and list files */

  const char *errLevelString = g_errLevelStrings[errLevel];

  if ((g_tokenString) && (g_tokenString < g_stringSP))
    {
      fprintf (g_errFile, g_fmtErrWithToken,
               FP->include, FP->line, errLevelString, errCode, g_token,
               g_tokenString);
      fprintf (g_lstFile, g_fmtErrWithToken,
               FP->include, FP->line, errLevelString, errCode, g_token,
               g_tokenString);
      g_stringSP = g_tokenString; /* Clean up string stack */
    }
  else
    {
      fprintf (g_errFile, g_fmtErrNoToken,
               FP->include, FP->line, errLevelString, errCode, g_token);
      fprintf (g_lstFile, g_fmtErrNoToken,
               FP->include, FP->line, errLevelString, errCode, g_token);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************/

void errmsg(char *fmt, ...)
{
  char buffer[1024];
  va_list ap;

  /* Get the full string */

  va_start(ap, fmt);
  (void)vsprintf(buffer, fmt, ap);

  /* Then output the string to stderr, the err file, and the list file */

  fputs(buffer, stderr);
  fputs(buffer, g_errFile);
  fputs(buffer, g_lstFile);

  va_end(ap);
}

/****************************************************************************/


void warn(uint16_t errCode)
{
   TRACE(g_lstFile,"[warn:%04x]", errCode);

   /* Write error record to the error and list files */

   pas_PrintError(errCode, ERROR_LEVEL_WARNING);

   /* Increment the count of warning */

   g_warnCount++;
}

/****************************************************************************/


void error(uint16_t errCode)
{
   TRACE(g_lstFile,"[error:%04x]", errCode);

#if CONFIG_DEBUG
   fatal(errCode);
#else
   /* Write error record to the error and list files */

   pas_PrintError(errCode, ERROR_LEVEL_ERROR);

   /* Check if the error count has been execeeded the max allowable */

   if ((++g_errCount) > MAX_ERRORS)
     {
       fatal(eCOUNT);
     }
#endif

} /* end error */

/****************************************************************************/


void fatal(uint16_t errCode)
{
   TRACE(g_lstFile,"[fatal:%04x]", errCode);

   /* Write error record to the error and list files */

   pas_PrintError(errCode, ERROR_LEVEL_FATAL);

   /* Dump the tables (if CONFIG_DEBUG) */

   DUMPTABLES;

   /* And say goodbye */

   printf(g_fmtErrAbort, errCode);
   fprintf(g_lstFile, g_fmtErrAbort, errCode);

   exit(1);

} /* end fatal */

