/****************************************************************************
 * pas_mkconfig.c
 * Convert a .config file into a config.h file
 *
 *   Copyright (C) 2007-2013, 2022 Gregory Nutt. All rights reserved.
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
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define DEFCONFIG ".config"
#define MAX_LINE  (PATH_MAX > 256 ? PATH_MAX : 256)

/****************************************************************************
 * Private Data
 ****************************************************************************/

static char g_ConfigLine[MAX_LINE + 1];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void pas_ShowUsage(const char *progname)
{
  fprintf(stderr, "USAGE: %s <abs path to .config>\n", progname);
  exit(1);
}

static inline char *pas_GetFilePath(const char *name)
{
  snprintf(g_ConfigLine, PATH_MAX, "%s/" DEFCONFIG, name);
  g_ConfigLine[PATH_MAX] = '\0';
  return strdup(g_ConfigLine);
}

/* Skip over any spaces */

static char *pas_SkipSpaces(char *ptr)
{
  while (*ptr && isspace((int)*ptr)) ptr++;
  return ptr;
}

/* Find the end of a variable string */

static char *pas_FindNameEnd(char *ptr)
{
  while (*ptr && (isalnum((int)*ptr) || *ptr == '_')) ptr++;
  return ptr;
}

/* Find the end of a value string */

static char *pas_FindValueEnd(char *ptr)
{
  while (*ptr && !isspace((int)*ptr))
    {
      if (*ptr == '"')
        {
          do ptr++; while (*ptr && (*ptr != '"' || *(ptr - 1) == '\\'));
          if (*ptr) ptr++;
        }
      else
        {
          do ptr++; while (*ptr && !isspace((int)*ptr) && *ptr != '"');
        }
    }

  return ptr;
}

/* Read the next line from the configuration file */

static char *pas_ReadLine(FILE *stream)
{
  char *ptr;

  for (; ; )
    {
      g_ConfigLine[MAX_LINE] = '\0';
      if (!fgets(g_ConfigLine, MAX_LINE, stream))
        {
          return NULL;
        }
      else
        {
          ptr = pas_SkipSpaces(g_ConfigLine);
          if (*ptr && *ptr != '#' && *ptr != '\n')
            {
              return ptr;
            }
        }
    }
}

/* Parse the line from the configuration file into a variable name
 * string and a value string.
 */

static void pas_ParseLine(char *ptr, char **varname, char **varval)
{
  *varname = ptr;
  *varval = NULL;

  /* Parse to the end of the variable name */

  ptr = pas_FindNameEnd(ptr);

  /* An equal sign is expected next, perhaps after some white space */

  if (*ptr && *ptr != '=')
    {
      /* Some else follows the variable name.  Terminate the variable
       * name and skip over any spaces.
       */

      *ptr = '\0';
       ptr = pas_SkipSpaces(ptr + 1);
    }

  /* Verify that the equal sign is present */

  if (*ptr == '=')
    {
      /* Make sure that the variable name is terminated (this was already
       * done if the name was followed by white space.
       */

      *ptr = '\0';

      /* The variable value should follow =, perhaps separated by some
       * white space.
       */

      ptr = pas_SkipSpaces(ptr + 1);
      if (*ptr)
        {
          /* Yes.. a variable follows.  Save the pointer to the start
           * of the variable string.
           */

          *varval = ptr;

          /* Find the end of the variable string and make sure that it
           * is terminated.
           */

          ptr = pas_FindValueEnd(ptr);
          *ptr = '\0';
        }
    }
}

static void pas_GenerateDefinitions(FILE *stream)
{
  char *varname;
  char *varval;
  char *ptr;

  /* Loop until the entire file has been parsed. */

  do
    {
      /* Read the next line from the file */

      ptr = pas_ReadLine(stream);
      if (ptr)
        {
          /* Parse the line into a variable and a value field */

          pas_ParseLine(ptr, &varname, &varval);

          /* Was a variable name found? */

          if (varname)
            {
              /* If no value was provided or if the value 'n' was provided,
               * then undefine the configuration variable.
               */

              if (varval == NULL || strcmp(varval, "n") == 0)
                {
                  printf("#undef %s\n", varname);
                }

              /* Simply define the configuration variable to '1' if it has
               * the value "y"
               */

              else if (strcmp(varval, "y") == 0)
                {
                  printf("#define %s 1\n", varname);
                }

              /* Or to '2' if it has the special value 'm' */

              else if (strcmp(varval, "m") == 0)
                {
                  printf("#define %s 2\n", varname);
                }

              /* Otherwise, use the value as provided */

              else
                {
                  printf("#define %s %s\n", varname, varval);
                }
            }
        }
    }
  while (ptr);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, char **argv, char **envp)
{
  char *filepath;
  FILE *stream;

  if (argc != 2)
    {
      fprintf(stderr, "Unexpected number of arguments\n");
      pas_ShowUsage(argv[0]);
    }

  filepath = pas_GetFilePath(argv[1]);
  if (filepath == NULL)
    {
      fprintf(stderr, "pas_GetFilePath failed\n");
      exit(2);
    }

  stream = fopen(filepath, "r");
  if (stream == NULL)
    {
      fprintf(stderr, "open %s failed: %s\n", filepath, strerror(errno));
      exit(3);
    }

  printf("/* Automatically generated file; DO NOT EDIT. */\n\n");
  printf("#ifndef __CONFIG_H\n");
  printf("#define __CONFIG_H\n\n");

  pas_GenerateDefinitions(stream);

  printf("#endif /* __CONFIG_H */\n");
  fclose(stream);

  free(filepath);
  return 0;
}
