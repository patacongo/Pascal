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

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pas_machine.h"
#include "pas_oslib.h"
#include "pas_errcodes.h"

#include "libexec.h"
#include "libexec_heap.h"
#include "libexec_longops.h"   /* For libexec_UPop32() */
#include "libexec_stringlib.h"
#include "libexec_oslib.h"

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int libexec_GetEnv(struct libexec_s *st, const uint16_t *nameString,
                          uint16_t *valueString)
{
  uint16_t    nameAddr;
  uint16_t    nameSize;
  const char *nameSrc;
  char       *cName;
  int         errorCode = eNOERROR;

  /* Make a C string out of the pascal name string */

  nameAddr = nameString[BTOISTACK(sSTRING_DATA_OFFSET)];
  nameSize = nameString[BTOISTACK(sSTRING_SIZE_OFFSET)];
  nameSrc  = (const char *)ATSTACK(st, nameAddr);
  cName    = libexec_MkCString(st, nameSrc, nameSize, false);

  if (cName == NULL)
    {
      errorCode = eNOMEMORY;
    }
  else
    {
      uint16_t    valueAddr;
      uint16_t    valueSize;
      const char *valueSrc;
      char       *valueDest;

      /* Make the C-library call to get the value C string and free the name
       * string copy.
       */

      valueSrc = (const char *)getenv((char *)cName);

      /* Is the environment variable defined? */

     valueSize = 0;
     if (valueSrc != NULL)
        {
          /* Allocate tempororary string stack */

          if (st->csp + STRING_BUFFER_SIZE >= st->spb)
            {
              errorCode = eSTRSTKOVERFLOW;
            }
          else
            {
              /* Allocate a string buffer on the string stack for the new
               * string.
               */

              valueAddr = INT_ALIGNUP(st->csp);
              st->csp = valueAddr + STRING_BUFFER_SIZE;

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
              valueString[BTOISTACK(sSTRING_ALLOC_OFFSET)] = STRING_BUFFER_SIZE;
            }
        }
    }

  return errorCode;
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

    default :
      errorCode = eBADSYSLIBCALL;
      break;
    }

  return errorCode;
}
