/****************************************************************************
 * libexec_oslib.c
 * Pascal run-time library
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

#define _GNU_SOURCE

#include <sys/types.h>
#include <sys/wait.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <spawn.h>
#include <string.h>

#include "pas_nuttx.h"
#include "pas_machine.h"
#include "pas_oslib.h"
#include "pas_errcodes.h"

#include "libexec.h"
#include "libexec_heap.h"
#include "libexec_longops.h"   /* For libexec_UPop32() */
#include "libexec_stringlib.h"
#include "libexec_oslib.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* One of the several differences between the Linus posix_spawnp() and the
 * NuttX task_spawn() is that the argv[] includes the program name for
 * posix_spawnp(), but task_spawn() does not.
 */

#ifdef USE_BUILTIN
#  define ARG1NDX 0 /* argv[0] is the first argument */
#else
#  define ARG1NDX 1 /* argv[0] is the program name */
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int libexec_GetEnv(struct libexec_s *st, const uint16_t *nameString,
                          uint16_t *valueString)
{
  uint16_t    nameAlloc;
  uint16_t    nameAddr;
  uint16_t    nameSize;
  const char *nameSrc;
  char       *cName;
  int         errorCode = eNOERROR;

  /* Make a C string out of the pascal name string */

  nameAlloc = nameString[BTOISTACK(sSTRING_ALLOC_OFFSET)];
  nameAddr  = nameString[BTOISTACK(sSTRING_DATA_OFFSET)];
  nameSize  = nameString[BTOISTACK(sSTRING_SIZE_OFFSET)];
  nameSrc   = (const char *)ATSTACK(st, nameAddr);
  cName     = libexec_MkCString(st, nameSrc, nameSize, false);

  if (cName == NULL)
    {
      errorCode = eSTRSTKOVERFLOW;
    }
  else
    {
      uint16_t    valueAlloc;
      uint16_t    valueAddr;
      uint16_t    valueSize;
      const char *valueSrc;
      char       *valueDest;

      /* Make the C-library call to get the value C string and free the name
       * string copy.
       */

      valueSrc = (const char *)getenv((char *)cName);

      /* We have consumed the name string container, check if we need to free
       * its string buffer allocation as well.
       */

      errorCode = libexec_FreeTmpString(st, nameAddr, nameAlloc);

      /* Is the environment variable defined? */

      valueSize = 0;
      if (valueSrc != NULL)
        {
          /* Allocate tempororary string memory from the heap */

          valueAddr = libexec_AllocTmpString(st, STRING_BUFFER_SIZE,
                                             &valueAlloc);
          if (valueAddr == 0)
            {
              errorCode = eNOMEMORY;
            }
          else
            {
              /* Copy the string into the allocated string stack memory */

              valueSize = strlen((char *)valueSrc);
              if (valueSize > STRING_BUFFER_SIZE)
                {
                  valueSize = STRING_BUFFER_SIZE;
                }

              valueDest = (char *)ATSTACK(st, valueAddr);
              memcpy(valueDest, valueSrc, valueSize);

              /* Convert the environment variable C string to a pascal string
               * and save it on the stack.
               */

              valueString[BTOISTACK(sSTRING_SIZE_OFFSET)]  = valueSize;
              valueString[BTOISTACK(sSTRING_DATA_OFFSET)]  = valueAddr;
              valueString[BTOISTACK(sSTRING_ALLOC_OFFSET)] = valueAlloc;
            }
        }
    }

  return errorCode;
}

int libexec_Spawn(struct libexec_s *st, uint16_t *pexNameString,
                  uint16_t stringBufferSize, uint16_t heapSize,
                  bool waitForTask, bool enablePCodeDebugger)
{
  char       *argv[8];
  const char *nameSrc;
  char       *cName;
  char       *pexPath;
  char       *strStringBufferSize;
  char       *strHeapSize;
  pid_t       pid;
  uint16_t    nameAlloc;
  uint16_t    nameAddr;
  uint16_t    nameSize;
  int         status;
  int         ret;

  /* Make a C string out of the pascal name string */

  nameAlloc = pexNameString[BTOISTACK(sSTRING_ALLOC_OFFSET)];
  nameAddr  = pexNameString[BTOISTACK(sSTRING_DATA_OFFSET)];
  nameSize  = pexNameString[BTOISTACK(sSTRING_SIZE_OFFSET)];
  nameSrc   = (const char *)ATSTACK(st, nameAddr);
  cName     = libexec_MkCString(st, nameSrc, nameSize, false);

  if (cName == NULL)
    {
     return eNOMEMORY;
    }

  /* Construct path to the .pex file including the file name */

  pexPath = NULL;
  asprintf(&pexPath, "%s%s", CONFIG_PASCAL_EXECDIR, cName);

  /* We have consumed the name string container, check if we need to free
   * its string buffer allocation as well.
   */

  libexec_FreeTmpString(st, nameAddr, nameAlloc);

  /* Verify that asprintf was able to allocate the .pex file name */

  if (pexPath == NULL)
    {
      return eNOMEMORY;
    }

  strStringBufferSize = NULL;
  asprintf(&strStringBufferSize, "%u", stringBufferSize);
  if (strStringBufferSize == NULL)
    {
      free(pexPath);
      return eNOMEMORY;
    }

  strHeapSize = NULL;
  asprintf(&strHeapSize, "%u", heapSize);
  if (strHeapSize == NULL)
    {
      free(pexPath);
      return eNOMEMORY;
    }

  /* Build the argv list */

#ifndef USE_BUILTIN
  argv[0] = "posix_spawnp";
#endif

  argv[ARG1NDX + 0] = "-t";
  argv[ARG1NDX + 1] = strStringBufferSize;
  argv[ARG1NDX + 2] = "-n";
  argv[ARG1NDX + 3] = strHeapSize;

  if (enablePCodeDebugger)
    {
      argv[ARG1NDX + 4] = "--debug";
      argv[ARG1NDX + 5] = pexPath;
      argv[ARG1NDX + 6] = NULL;
    }
  else
    {
      argv[ARG1NDX + 4] = pexPath;
      argv[ARG1NDX + 5] = NULL;
    }

  /* Spawn the prun program, providing path to the Pascal executable
   * (.pex) program.  Since spawnp is used, "prun" is a simple file name
   * but the PATH variable must set to location the prun executable.
   * The prunPath is an absolute or relative path.  No non-NULL
   * environment variables, special attributes, or file action parameters
   * are passed.
   */

#ifdef USE_BUILTIN
  pid = task_spawn("prun", prun_main, NULL, NULL, argv, NULL);
  ret = (pid < 0) ? -pid : 0;
#else
  ret = posix_spawnp(&pid, "prun", NULL, NULL, argv, NULL);
#endif

  free(pexPath);
  free(strStringBufferSize);
  free(strHeapSize);

  if (ret != 0)
    {
      return eSPAWANFAILED;
    }

   /* Monitor status of the child until it terminates. */

  if (waitForTask)
    {
      do
        {
          ret = waitpid(pid, &status, WUNTRACED | WCONTINUED);
          if (ret == -1)
            {
              return eWAITFAILED;
            }
        }
      while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

  return eNOERROR;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: libexec_OsOperations
 *
 * Description:
 *   This function process a system OS operation
 *
 ****************************************************************************/

uint16_t libexec_OsOperations(struct libexec_s *st, uint8_t subfunc)
{
  int errorCode = eNOERROR;

  switch (subfunc)
    {
      /* Exit processing
       *
       *   procedure exit(exitCode : integer);
       *
       * ON INPUT:
       *   TOS(0) - Exit code
       * ON RETURN:
       *   Does not return
       */

    case osEXIT:
      {
        POP(st, st->exitCode);
        errorCode = eEXIT;
      }
      break;

      /* Heap allocation:
       *
       *   function new(size : integer) : integer;
       *
       * ON INPUT:
       *   TOS(0) - Size of the heap region to allocate
       *
       * ON RETURN:
       *   TOS(0) - The allocated heap region
       */

    case osNEW :
      {
        uint16_t  size;

        POP(st, size);  /* Size of allocation */

        /* Allocate the memory */

        errorCode = libexec_New(st, size);
      }
      break;

      /* Dispose of a previous heap allocation:
       *
       *   procedure despose(VAR alloc : integer);
       *
       * ON INPUT:
       *   TOS(0) - Address of the heap region to dispose of
       *
       * ON RETURN:
       *   No value is returned
       */

    case osDISPOSE :
      {
        uint16_t addr;

        POP(st, addr);  /* Address of allocation to be freed */

        /* Free the memory */

        errorCode = libexec_Dispose(st, addr);
      }
      break;

      /* Get the value of an environment string
       *
       *   function getent(name : string) : string;
       *
       * ON INPUT:
       *   TOS(0) = Size of variable name string memory allocation
       *   TOS(1) = Address of variable name string
       *   TOS(2) = Length of variable name string
       * ON RETURN:
       *   TOS(0) = Size of variable value string memory allocation
       *   TOS11) = Address of variable value string
       *   TOS(2) = Length of variable value string
       */

    case osGETENV :
      {
        const uint16_t *nameString;
        uint16_t      *valueString;

        nameString  = (const uint16_t *)&TOS(st, 2);
        valueString = (uint16_t *)&TOS(st, 2);
        errorCode   = libexec_GetEnv(st, nameString, valueString);
      }
      break;

     /* Spawn a Pascal task.
      *
      *   function spawn(PexFileName : string;
      *                  StringBufferAlloc, HeapAlloc : integer;
      *                  Wait, Debug : boolean) : boolean;
      *
      * PexFileName is a simple file name and does not include any path
      * components.
      *
      * ON INPUT:
      *   TOS(0) = Boolean true: Enable P-Code debugger
      *   TOS(1) = Boolean true: Wait for spawned task to exit
      *   TOS(2) = Size of heap memory to allocate
      *   TOS(3) = Size of string memory to allocate
      *   TOS(4) = Size of PexFileName buffer allocation
      *   TOS(5) = Address of PexFileName name string
      *   TOS(6) = Length of PexFileName name string
      * ON RETURN:
      *   TOS(0) = Boolean true: Successfully started
      */

    case osSPAWN :
      {
        uint16_t enablePCodeDebugger;
        uint16_t waitForTask;
        uint16_t heapSize;
        uint16_t stringBufferAlloc;
        uint16_t pexNameString[3];

        /* Get parameters from the stack */

        POP(st, enablePCodeDebugger);
        POP(st, waitForTask);
        POP(st, heapSize);
        POP(st, stringBufferAlloc);

        POP(st, pexNameString[BTOISTACK(sSTRING_ALLOC_OFFSET)]);
        POP(st, pexNameString[BTOISTACK(sSTRING_DATA_OFFSET)]);
        POP(st, pexNameString[BTOISTACK(sSTRING_SIZE_OFFSET)]);

        /* And spawn the task */

        errorCode =
          libexec_Spawn(st, pexNameString, stringBufferAlloc, heapSize,
                        (waitForTask == PASCAL_FALSE) ? false : true,
                        (enablePCodeDebugger == PASCAL_FALSE) ? false : true);
        PUSH(st, (errorCode == eNOERROR) ? PASCAL_TRUE : PASCAL_FALSE);
      }
      break;

    default :
      errorCode = eBADSYSLIBCALL;
      break;
    }

  return errorCode;
}
