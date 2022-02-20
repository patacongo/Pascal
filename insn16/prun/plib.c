/****************************************************************************
 * plib.c
 * Pascal run-time library
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

#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pas_debug.h"
#include "pas_machine.h"
#include "pas_library.h"
#include "pas_errcodes.h"

#include "pexec.h"
#include "pmmgr.h"
#include "plib.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static uint8_t *pexec_mkcstring(uint8_t *buffer, int buflen);
static int      pas_strinit(struct pexec_s *st, uint16_t strVarAddr,
                            uint16_t strAllocSize);
static void     pas_strcpy(struct pexec_s *st, uint16_t srcBufferAddr,
                           uint16_t srcStringSize, uint16_t destVarAddr,
                           uint16_t destBufferSize, uint16_t varOffset);
static int      pas_Bstr2str(struct pexec_s *st, uint16_t arrayAddress,
                             uint16_t arraySize);
static int      pas_Str2bstr(struct pexec_s *st, uint16_t arrayAddress,
                             uint16_t arraySize, uint16_t stringBufferAddress,
                             uint16_t stringSize, uint16_t offset);
static int      pas_strcat(struct pexec_s *st, uint16_t srcStringAddr,
                           uint16_t srcStringSize, uint16_t destStringAddr,
                           uint16_t *pDestStringSize, uint16_t destStrAlloc);
static int      pas_strcatc(struct pexec_s *st, char srcChar,
                            uint16_t destStringAddr, uint16_t *pDestStringSize,
                            uint16_t destStrAlloc);

/****************************************************************************
 * Public Data
 ****************************************************************************/

int16_t g_exitCode;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pexec_mkcstring
 ****************************************************************************/

static uint8_t *pexec_mkcstring(uint8_t *buffer, int buflen)
{
  uint8_t *string;

  string = malloc(buflen + 1);
  if (string != NULL)
    {
      memcpy(string, buffer, buflen);
      string[buflen] = '\0';
    }

  return string;
}

static int pas_strinit(struct pexec_s *st, uint16_t strVarAddr,
                       uint16_t strAllocSize)
{
  uint16_t strAllocAddr = INT_ALIGNUP(st->csp);
  int errorCode = eNOERROR;

  /* Check if there is space on the string stack for the new string buffer. */

  if (st->csp + strAllocSize >= st->spb)
    {
      errorCode = eSTRSTKOVERFLOW;
    }
  else
    {
      /* Allocate a string buffer on the string stack for the new string. */

      st->csp = strAllocAddr + strAllocSize;

      /* Initialize the new string.  Order:
       *
       *   TOS(n)     = 16-bit pointer to the string data.
       *   TOS(n + 1) = String size
       *
       * NOTE:  This depends on the fact that these two fields appear at the
       * same offset for both sSTRING and sSHORTSTRING.
       */

      PUTSTACK(st, strAllocAddr, strVarAddr + sSTRING_DATA_OFFSET);
      PUTSTACK(st, 0,            strVarAddr + sSTRING_SIZE_OFFSET);
    }

  return errorCode;
}

static void pas_strcpy(struct pexec_s *st, uint16_t srcBufferAddr,
                       uint16_t srcStringSize, uint16_t destVarAddr,
                       uint16_t destBufferSize, uint16_t varOffset)
{
  /* Copy pascal string to a pascal string */

  uint16_t destBufferAddr;
  const uint8_t *src;
  uint8_t *dest;

  /* Offset the destination address */

  destVarAddr += varOffset;

  /* Do nothing if the source and destination buffer addresses are the
   * same string buffer.  This happens normally on cases like:
   *
   *   string name;
   *   char   c;
   *   name := name + c;
   */

  destBufferAddr = GETSTACK(st, destVarAddr + sSTRING_DATA_OFFSET);
  if (destBufferAddr != srcBufferAddr)
    {
      /* The source and destination strings are different.
       * Make sure that the string length will fit into the destination
       * string buffer.
       */

      if (srcStringSize > destBufferSize)
        {
          /* Clip to the maximum size */

          srcStringSize = destBufferSize;
        }

      /* Transfer the string buffer contents */

      dest = ATSTACK(st, destBufferAddr);
      src  = ATSTACK(st, srcBufferAddr);
      memcpy(dest, src, srcStringSize);

      /* And set the new string size */

      PUTSTACK(st, srcStringSize, destVarAddr + sSTRING_SIZE_OFFSET);
    }
}

static int pas_Bstr2str(struct pexec_s *st, uint16_t arrayAddress,
                        uint16_t arraySize)
{
  const char *src;
  char *dest;
  uint16_t bufferAddress;
  int errorCode = eNOERROR;
  int len;

  /* Get a pointer to the array in the stack */

  src  = (const char *)ATSTACK(st, arrayAddress);

  /* Get the length of the string in the array.  Here we assume that the
   * string is represented as a NUL-terminated C string.
   */

  len = strnlen(src, arraySize);

  /* Clip the string if necessary to fit into the string buffer allocation */

  if (len > st->stralloc)
   {
     len = st->stralloc;
   }

  /* Check if there is space on the string stack for the new string buffer. */


  if (st->csp + st->stralloc >= st->spb)
    {
      errorCode = eSTRSTKOVERFLOW;
    }
  else
    {
      /* Allocate a string buffer on the string stack for the new string. */

      bufferAddress = INT_ALIGNUP(st->csp);
      st->csp       = bufferAddress + st->stralloc;

      /* Copy the array into the string buffer */

      dest = (char *)ATSTACK(st, bufferAddress);
      memcpy(dest, src, len);

      /* Put the new string at the top of the stack.  Order:
       *
       *   TOS(n)     = 16-bit pointer to the string data.
       *   TOS(n + 1) = String size
       */

      PUSH(st, len);           /* String size */
      PUSH(st, bufferAddress); /* String buffer address */
    }

  return errorCode;
}

static int pas_Str2bstr(struct pexec_s *st, uint16_t arrayAddress,
                        uint16_t arraySize, uint16_t stringBufferAddress,
                        uint16_t stringSize, uint16_t offset)
{
  const char *src;
  char *dest;
  int errorCode = eNOERROR;
  int len;

  /* Get a pointer to the source string buffer and the dest array. */

  src  = (const char *)ATSTACK(st, stringBufferAddress);
  dest = (char *)ATSTACK(st, arrayAddress + offset);

  /* Get the length of the string to transfer. */

  len = stringSize;

  /* Clip the string if necessary to fit into the array */

  if (len > arraySize)
    {
      stringSize = arraySize;
    }

  /* Copy the string buffer into the array */

  memcpy(dest, src, len);
  return errorCode;
}

static int pas_strcat(struct pexec_s *st, uint16_t srcStringAddr,
                      uint16_t srcStringSize, uint16_t destStringAddr,
                      uint16_t *pDestStringSize, uint16_t destStrAlloc)
{
  uint16_t destStringSize = *pDestStringSize;
  const char *src;
  char *dest;
  int errorCode = eNOERROR;

  /* Check for string overflow. */

  if (srcStringSize + destStringSize > destStrAlloc)
    {
      errorCode = eSTRSTKOVERFLOW;
    }
  else
    {
      /* Append the data from the source string buffer to dest string buffer. */

      src  = (const char *)ATSTACK(st, srcStringAddr);
      dest = (char *)ATSTACK(st, destStringAddr);
      memcpy(&dest[destStringSize], src, srcStringSize);

      /* Set the new dest string size. */

      *pDestStringSize = srcStringSize + destStringSize;
    }

  return errorCode;
}

static int pas_strcatc(struct pexec_s *st, char srcChar,
                       uint16_t destStringAddr, uint16_t *pDestStringSize,
                       uint16_t destStrAlloc)
{
  uint16_t destStringSize = *pDestStringSize;
  char *dest;
  int errorCode = eNOERROR;

  /* Check for string overflow. */

  if (destStringSize + sCHAR_SIZE > destStrAlloc)
    {
      errorCode = eSTRSTKOVERFLOW;
    }
  else
    {
      /* Append the character to the dest string buffer. */

      dest                = (char *)ATSTACK(st, destStringAddr);
      dest[destStringSize] = srcChar;

      /* Set the new dest string size. */

      *pDestStringSize = destStringSize + sCHAR_SIZE;
    }

  return errorCode;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pexec_libcall
 *
 * Description:
 *   This function process a system I/O operation
 *
 ****************************************************************************/

uint16_t pexec_libcall(struct pexec_s *st, uint16_t subfunc)
{
  ustack_t  uparm1;
  ustack_t  uparm2;
  ustack_t  offset;
  ustack_t  size;
  paddr_t   addr1;
  paddr_t   addr2;
  uint8_t  *src;
  uint8_t  *dest;
  uint8_t  *name;
  int       errorCode = eNOERROR;

  switch (subfunc)
    {
      /* Exit processing
       *
       *   procedure hist(exitCode : integer);
       *
       * ON INPUT:
       *   TOS(0) - Exit code
       * ON RETURN:
       *   Does not return
       */

    case lbEXIT:
      POP(st, g_exitCode);
      errorCode = eEXIT;
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

    case lbNEW :
      POP(st, size);  /* Size of allocation */

      /* Allocate the memory */

      errorCode = pexec_New(st, size);
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

    case lbDISPOSE :
      POP(st, addr1);  /* Address of allocation to be freed */

      /* Free the memory */

      errorCode = pexec_Dispose(st, addr1);
      break;

      /* Get the value of an environment string
       *
       *   function getent(name : string) : string;
       *
       * ON INPUT:
       *   TOS(0) = Address of variable name string
       *   TOS(1) = Length of variable name string
       * ON RETURN:
       *   TOS(0) = Address of variable value string
       *   TOS(1) = Length of variable value string
       */

    case lbGETENV :
      {
        addr1 = TOS(st, 0);
        size  = TOS(st, 1);

        /* Make a C string out of the pascal string */

        src = (uint8_t *)ATSTACK(st, addr1);
        name = pexec_mkcstring(src, size);
        if (name == NULL)
          {
            errorCode = eNOMEMORY;
          }
        else
          {
            /* Make the C-library call and free the string copy */

            src = (uint8_t *)getenv((char *)name);
            free_cstring(name);

            /* Is the environment variable defined? */

            size = 0;
            if (src != NULL)
              {
                /* Allocate tempororary string stack */

                if (st->csp + st->stralloc >= st->spb)
                  {
                    errorCode = eSTRSTKOVERFLOW;
                  }
                else
                  {
                    /* Allocate a string buffer on the string stack for the
                     * new string.
                     */

                    addr2   = INT_ALIGNUP(st->csp);
                    st->csp = addr2 + st->stralloc;

                    if (size > st->stralloc)
                      {
                        size = st->stralloc;
                      }

                    /* Copy the string into the string stack */

                    size = strlen((char *)src);
                    if (size > st->stralloc)
                      {
                        size = st->stralloc;
                      }

                    dest  = (uint8_t *)ATSTACK(st, addr2);
                    memcpy(dest, src, size);

                    /* Save the allocated string buffer pointer */

                    TOS(st, 1) = addr2;
                  }
              }

            /* Save the environment variable length */

            TOS(st, 1) = size;
          }
      }
      break;

      /* Copy pascal standard string to a pascal standard string
       *
       *   procedure strcpy(src : string; var dest : string)
       *
       * ON INPUT:
       *   TOS(0) = address of dest string variable
       *   TOS(1) = pointer to source string buffer
       *   TOS(2) = length of source string
       * ON RETURN (input consumed):
       *
       * NOTE:  The alternate version is equivalent but has the dest
       * address and source string reversed.
       */

    case lbSTRCPY :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Address of dest string variable  */
      POP(st, addr2);  /* Address of source string buffer */
      POP(st, size);   /* Length of valid source data */

      /* And perform the string copy */

      pas_strcpy(st, addr2, size, addr1, st->stralloc, 0);
      break;

    case lbSTRCPY2 :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr2);  /* Address of source string buffer */
      POP(st, size);   /* Length of valid source data */
      POP(st, addr1);  /* Address of dest string variable  */

      /* And perform the string copy */

      pas_strcpy(st, addr2, size, addr1, st->stralloc, 0);
      break;

      /* Copy pascal standard string to a element of a pascal standard string
       * array
       *
       *   procedure strcpyx(src : string; var dest : string;
       *                     offset : integer)
       *
       * ON INPUT:
       *   TOS(0) = Address of dest string variable
       *   TOS(1) = Pointer to source string buffer
       *   TOS(2) = Length of source string
       *   TOS(3) = Dest string variable address offset
       * ON RETURN: actual parameters released.
       *
       * NOTE:  The alternate version is equivalent but has the dest
       * address and source string reversed.
       */

    case lbSTRCPYX :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Address of dest string variable  */
      POP(st, addr2);  /* Address of source string buffer */
      POP(st, size);   /* Length of valid source data */
      POP(st, offset); /* Offset from dest string address */

      /* And perform the string copy */

      pas_strcpy(st, addr2, size, addr1, st->stralloc, offset);
      break;

    case lbSTRCPYX2 :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr2);  /* Address of source string buffer */
      POP(st, size);   /* Length of valid source data */
      POP(st, offset); /* Offset from dest string address */
      POP(st, addr1);  /* Address of dest string variable  */

      /* And perform the string copy */

      pas_strcpy(st, addr2, size, addr1, st->stralloc, offset);
      break;

      /* Copy pascal short string to a pascal short string
       *
       * ON INPUT:
       *   TOS(0) = Address of dest short short string variable
       *   TOS(1) = Short string buffer size
       *   TOS(2) = Pointer to source short string buffer
       *   TOS(3) = Length of source short string
       * ON RETURN (input consumed):
       *
       * NOTE:  The alternate version is equivalent but has the dest
       * address and source string reversed.
       */

    case lbSSTRCPY2 :

      /* "Pop" in the input parameters from the stack */

      DISCARD(st, 1);  /* Source short string buffer allocation */
      POP(st, addr2);  /* Address of source short string buffer */
      POP(st, size);   /* Length of valid source data */
      POP(st, addr1);  /* Address of dest short string variable  */
      goto sstrcpy_common;

    case lbSSTRCPY :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Address of dest short string variable  */
      DISCARD(st, 1);  /* Source short string buffer allocation */
      POP(st, addr2);  /* Address of source short string buffer */
      POP(st, size);   /* Length of valid source data */
      goto sstrcpy_common;

      /* Copy pascal short string to a element of a pascal short string
       * array
       *
       * ON INPUT:
       *   TOS(0) = Address of dest short short string variable
       *   TOS(1) = Short string buffer size
       *   TOS(2) = Pointer to source short string buffer
       *   TOS(3) = Length of source short string
       *   TOS(4) = Dest short string variable address offset
       * ON RETURN (input consumed):
       *
       * NOTE:  The alternate version is equivalent but has the dest
       * address and source string reversed.
       */

    case lbSSTRCPYX2 :

      /* "Pop" in the input parameters from the stack */

      DISCARD(st, 1);  /* Source short string buffer allocation */
      POP(st, addr2);  /* Address of source short string buffer */
      POP(st, size);   /* Length of valid source data */
      POP(st, offset); /* Offset from dest string address */
      POP(st, addr1);  /* Address of dest short string variable  */
      goto sstrcpyx_common;

    case lbSSTRCPYX :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Address of dest short string variable  */
      DISCARD(st, 1);  /* Source short string buffer allocation */
      POP(st, addr2);  /* Address of source short string buffer */
      POP(st, size);   /* Length of valid source data */
      POP(st, offset); /* Offset from dest string address */
      goto sstrcpyx_common;

      /* Copy pascal short string to a pascal standard string
       *
       * ON INPUT:
       *   TOS(0) = Address of dest standard standard string variable
       *   TOS(1) = Short string buffer size
       *   TOS(2) = Pointer to source short string buffer
       *   TOS(3) = Length of source short string
       * ON RETURN (input consumed):
       *
       * NOTE:  The alternate version is equivalent but has the dest
       * address and source string reversed.
       */

    case lbSSTR2STR :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Address of dest standard string variable  */
      DISCARD(st, 1);  /* Short string buffer size */
      POP(st, addr2);  /* Address of source short string buffer */
      POP(st, size);   /* Length of valid short string source data */

      /* And perform the string copy */

      pas_strcpy(st, addr2, size, addr1, st->stralloc, 0);
      break;

    case lbSSTR2STR2 :

      /* "Pop" in the input parameters from the stack */

      DISCARD(st, 1);  /* Short string buffer size */
      POP(st, addr2);  /* Address of source short string buffer */
      POP(st, size);   /* Length of valid short string source data */
      POP(st, addr1);  /* Address of dest standard string variable  */

      /* And perform the string copy */

      pas_strcpy(st, addr2, size, addr1, st->stralloc, 0);
      break;

      /* Copy pascal short string to an element of a pascal standard string
       * array
       *
       * ON INPUT:
       *   TOS(0) = Address of dest standard standard string variable
       *   TOS(1) = Short string buffer size
       *   TOS(2) = Pointer to source short string buffer
       *   TOS(3) = Length of source short string
       *   TOS(4) = Dest standard string variable address offset
       * ON RETURN (input consumed):
       *
       * NOTE:  The alternate version is equivalent but has the dest
       * address and source string reversed.
       */

    case lbSSTR2STRX :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Address of dest standard string variable  */
      DISCARD(st, 1);  /* Short string buffer size */
      POP(st, addr2);  /* Address of source short string buffer */
      POP(st, size);   /* Length of valid short string source data */
      POP(st, offset); /* Offset from dest string address */

      /* And perform the string copy */

      pas_strcpy(st, addr2, size, addr1, st->stralloc, offset);
      break;

    case lbSSTR2STRX2 :

      /* "Pop" in the input parameters from the stack */

      DISCARD(st, 1);  /* Short string buffer size */
      POP(st, addr2);  /* Address of source short string buffer */
      POP(st, size);   /* Length of valid short string source data */
      POP(st, offset); /* Offset from dest string address */
      POP(st, addr1);  /* Address of dest standard string variable  */

      /* And perform the string copy */

      pas_strcpy(st, addr2, size, addr1, st->stralloc, offset);
      break;

      /* Copy pascal standard string to a pascal short string
       *
       * ON INPUT:
       *   TOS(0) = Address of dest short short string variable
       *   TOS(1) = Pointer to source standard string buffer
       *   TOS(2) = Length of source standard string
       * ON RETURN (input consumed):
       *
       * NOTE:  The alternate version is equivalent but has the dest
       * address and source string reversed.
       */

    case lbSTR2SSTR2 :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr2);  /* Address of source standard string buffer */
      POP(st, size);   /* Length of source standard string */
      POP(st, addr1);  /* Address of dest short string variable  */
      goto sstrcpy_common;

    case lbSTR2SSTR :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Address of dest short string variable  */
      POP(st, addr2);  /* Address of source standard string buffer */
      POP(st, size);   /* Length of source standard string */

    sstrcpy_common :
      {
        uint16_t *strPtr;
        uint16_t  strAlloc;

        /* Get the allocation size of the short string destination */

        strPtr = (uint16_t *)&st->dstack.b[addr1];
        strAlloc = strPtr[sSHORTSTRING_ALLOC_OFFSET / sINT_SIZE];

        /* And perform the string copy */

        pas_strcpy(st, addr2, size, addr1, strAlloc, 0);
      }
      break;

      /* Copy pascal standard string to an element of a pascal short string
       * array
       *
       * ON INPUT:
       *   TOS(0) = Address of dest short short string variable
       *   TOS(1) = Pointer to source standard string buffer
       *   TOS(2) = Length of source standard string
       *   TOS(3) = Dest short string variable address offset
       * ON RETURN (input consumed):
       *
       * NOTE:  The alternate version is equivalent but has the dest
       * address and source string reversed.
       */

    case lbSTR2SSTRX2 :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr2);  /* Address of source standard string buffer */
      POP(st, size);   /* Length of source standard string */
      POP(st, offset); /* Dest short string variable address offset */
      POP(st, addr1);  /* Address of dest short string variable  */
      goto sstrcpyx_common;

    case lbSTR2SSTRX :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Address of dest short string variable  */
      POP(st, addr2);  /* Address of source standard string buffer */
      POP(st, size);   /* Length of source standard string */
      POP(st, offset); /* Dest short string variable address offset */

    sstrcpyx_common :
      {
        uint16_t *strPtr;
        uint16_t  strAlloc;

        /* Get the allocation size of the short string destination.
         * REVISIT:  This is wrong.  We need to apply the indexing before
         * accing the dest short string array entry.
         */

        strPtr = (uint16_t *)&st->dstack.b[addr1];
        strAlloc = strPtr[sSHORTSTRING_ALLOC_OFFSET / sINT_SIZE];

        /* And perform the string copy */

        pas_strcpy(st, addr2, size, addr1, strAlloc, offset);
      }
      break;

    /* Copy binary file character array to a pascal string.  Used when a non-
     * indexed PACKED ARRAY[] OF CHAR appears as a factor in an RVALUE.
     *
     *   function bstr2str(fileNumber : Integer, arraySize : Integer,
     *                     arrayAddress : Integer) : String;
     *
     * ON INPUT:
     *   TOS(0) = Array address
     *   TOS(1) = Array size
     * ON RETURN:
     *   TOS(0) = String character buffer address
     *   TOS(1) = String size
     */

    case lbBSTR2STR :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Address of the array */
      POP(st, size);   /* Size of the array */

      /* And perform the copy */

      errorCode = pas_Bstr2str(st, addr1, size);
      break;

    /* Copy a pascal string into a binary file character array.  Use when a
     * non-indexed PACKED ARRAY[] OF CHAR appears as the LVALUE in an
     * assignment.
     *
     *   function str2bstr(arraySize : Integer, arrayAddress : Integer,
     *                     source : String);
     *
     * ON INPUT:
     *   TOS(0) = Address of the array (destination)
     *   TOS(1) = Size of the array
     *   TOS(2) = Address of the string (source)
     *   TOS(3) = Size of the string
     * ON RETURN:
     *   All inputs consumbed
     */

    case lbSTR2BSTR :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Address of the array */
      POP(st, uparm1); /* Size of the array */

      POP(st, addr2);  /* Address of the array */
      POP(st, uparm2); /* Size of the array */

      /* And perform the copy */

      errorCode = pas_Str2bstr(st, addr1, uparm1, addr2, uparm2, 0);
      break;

    /* Copy a pascal string into a binary file character array.  Use when a
     * non-indexed PACKED ARRAY[] OF CHAR appears within an array element
     * (using as a field of an array of records) as the LVALUE in an
     * assignment.
     *
     *   function str2bstr(arraySize : Integer, arrayAddress : Integer,
     *                     source : String, offset : Integer);
     *
     * ON INPUT:
     *   TOS(0) = Address of the array (destination)
     *   TOS(1) = Size of the array
     *   TOS(2) = Address of the string (source)
     *   TOS(3) = Size of the string
     *   TOS(4) = Array address offset
     * ON RETURN:
     *   All inputs consumbed
     */

    case lbSTR2BSTRX :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Address of the array */
      POP(st, uparm1); /* Size of the array */

      POP(st, addr2);  /* Address of the array */
      POP(st, uparm2); /* Size of the array */

      POP(st, offset); /* Address offset */

      /* And perform the copy */

      errorCode = pas_Str2bstr(st, addr1, uparm1, addr2, uparm2, offset);
      break;

      /* Initialize a new string variable. Create a string buffer.
       *   procedure mkstk(VAR str : string);
       *
       * ON INPUT
       *   TOS(1) = pointer to the newly string variable to be initialized
       * ON RETURN
       */

    case lbSTRINIT :

      /* Get input parameters */

      POP(st, addr1);  /* Address of dest string variable */

      /* And perform the variable initialization */

      errorCode = pas_strinit(st, addr1, INT_ALIGNUP(st->stralloc));
      break;

      /* Initialize a new short string variable. Create a string buffer.  This
       * is called only at entrance into a new Pascal block.
       *
       *   TYPE
       *     shortstring : string[size]
       *   procedure sstrinit(VAR str : shortstring);
       *
       * ON INPUT
       *   TOS(0) = address of the newly string variable to be initialized
       *   TOS(1) = size of the short string memory allocation
       * ON RETURN
       */

    case lbSSTRINIT :

      /* Get input parameters */

      POP(st, addr1);  /* Address of dest string variable */
      POP(st, size);   /* Size of string memory allocation */

      /* And perform the variable initialization */

      size      = INT_ALIGNUP(size);
      errorCode = pas_strinit(st, addr1, size);

      /* And save the allocated size in the variable's memory */

      PUTSTACK(st, size, addr1 + sSHORTSTRING_ALLOC_OFFSET);
      break;

      /* Initialize a temporary string variable on the stack. It is similar
       * to lbSTRINIT except that the form of its arguments are different.
       * This is currently used only when calling a function that returns a
       * string in order to catch the returned string value in an initialized
       * container.
       *
       *   function strtmp : string;
       *
       * ON INPUT
       * ON RETURN
       *   TOS(0) = Pointer to the string buffer on the stack.
       *   TOS(1) = String size (zero)
       */

    case lbSTRTMP :
      /* Check if there is space on the string stack for the new string
       * buffer.
       */

      if (st->csp + st->stralloc >= st->spb)
        {
          errorCode = eSTRSTKOVERFLOW;
        }
      else
        {
          /* Allocate a string buffer on the string stack for the new string. */

          addr1   = INT_ALIGNUP(st->csp);
          st->csp = addr1 + st->stralloc;

          /* Create the new string.  Order:
           *
           *   TOS(n)     = 16-bit pointer to the string data.
           *   TOS(n + 1) = String size
           */

          PUSH(st, 0);     /* String size */
          PUSH(st, addr1); /* String buffer address */
        }
      break;

      /* Replace a standard string with a duplicate string residing in
       * allocated string stack.
       *
       *   function strdup(name : string) : string;
       *
       * ON INPUT
       *   TOS(0) = pointer to original string data
       *   TOS(1) = length of original string
       * ON RETURN
       *   TOS(0) = pointer to new string data
       *   TOS(1) = length of new string (unchanged)
       */

    case lbSTRDUP :

      /* Get the parameters from the stack (leaving the string reference
       * in place.
       */

      addr1 = TOS(st, 0);     /* Original string data pointer */
      size  = TOS(st, 1);     /* Original string size */

      /* Check if there is space on the string stack for the new string */

      if (st->csp + st->stralloc >= st->spb)
        {
          errorCode = eSTRSTKOVERFLOW;
        }
      else
        {
          /* Allocate space on the string stack for the new string */

          addr2    = INT_ALIGNUP(st->csp);
          st->csp += st->stralloc;                    /* Allocate max size */

          /* Limit the size to the maximum size of a standard string.  This
           * can happen in cases where the string address lies in RO string
           * memory.
           */

          if (size > st->stralloc)
            {
              size = st->stralloc;
            }

          /* Copy the string into the string stack */

          src      = (uint8_t *)ATSTACK(st, addr1); /* Pointer to original string */
          dest     = (uint8_t *)ATSTACK(st, addr2); /* Pointer to new string */
          memcpy(dest, src, size);

          /* Update the string buffer address */

          TOS(st, 0) = addr2;
        }
      break;

      /* Replace a short string with a duplicate string residing in allocated
       * string stack.
       *
       *   function strdup(name : shortstring) : shortstring;
       *
       * ON INPUT
       *   TOS(0) = allocation of original short string
       *   TOS(1) = pointer to original short string
       *   TOS(2) = length of original short string
       * ON RETURN
       *   TOS(0) = allocation of new short string (unchanged)
       *   TOS(1) = pointer to new short string
       *   TOS(2) = length of new short string
       */

    case lbSSTRDUP :
      errorCode = eNOTYET;
      break;

      /* Replace a character with a string residing in allocated string stack.
       *   function mkstkc(c : char) : string;
       * ON INPUT
       *   TOS(0) = Character value
       * ON RETURN
       *   TOS(0) = pointer to new string
       *   TOS(1) = length of new string
       */

    case lbMKSTKC :
      /* Check if there is space on the string stack for the new string */

      if (st->csp + st->stralloc >= st->spb)
        {
          errorCode = eSTRSTKOVERFLOW;
        }
      else
        {
          /* Allocate space on the string stack for the new string */

          addr2    = INT_ALIGNUP(st->csp);
          st->csp += st->stralloc;                   /* Allocate max size */

          /* Save the length at the beginning of the copy */

          dest     = (uint8_t *)ATSTACK(st, addr2);  /* Pointer to new string */

          /* Copy the character into the string stack */

          *dest++  = TOS(st, 0);                     /* Save character as string */

          /* Update the stack content */

          TOS(st, 0) = 1;                            /* String length */
          PUSH(st, addr2);                           /* String address */
        }
      break;

      /* Concatenate a standard string to the end of a standard string.
       *
       *   function strcat(string1 : string, string2 : string) : string;
       *
       * ON INPUT
       *   TOS(0) = pointer to source standard string2 data
       *   TOS(1) = length of source standard string2
       *   TOS(2) = pointer to dest standard string1 data
       *   TOS(3) = length of dest standard string1
       * ON OUTPUT
       *   TOS(0) = pointer to dest standard string1 (unchanged)
       *   TOS(1) = new length of dest standard string1
       */

    case lbSTRCAT :

      /* Get the parameters from the stack (leaving the dest string info in
       * place).
       */

      POP(st, addr1);      /* source string1 string stack address */
      POP(st, uparm1);     /* source string1 size */

      /* Concatenate the strings */

      errorCode = pas_strcat(st, addr1, uparm1, TOS(st, 0), &TOS(st, 1),
                             st->stralloc);
      break;

      /* Concatenate a short string to the end of a short string.
       *
       *   function sstrcat(string1 : shortstring, string2 : shortstring) : shortstring;
       *
       * ON INPUT
       *   TOS(0) = string1 allocation size
       *   TOS(1) = pointer to source short string1 data
       *   TOS(2) = length of source short string1
       *   TOS(3) = string2 allocation size
       *   TOS(4) = pointer to dest short string2 data
       *   TOS(5) = length of dest short string2
       * ON OUTPUT
       *   TOS(0) = string2 allocation size (unchanged)
       *   TOS(1) = pointer to dest short string2 (unchanged)
       *   TOS(2) = new length of dest short string2
       */

    case lbSSTRCAT :

      /* Get the parameters from the stack (leaving the dest string info in
       * place).
       */

      DISCARD(st, 1);      /* discard the source string allocation size */
      POP(st, addr1);      /* source short string stack address */
      POP(st, uparm1);     /* source short string size */

      /* Concatenate the strings */

      errorCode = pas_strcat(st, addr1, uparm1, TOS(st, 2), &TOS(st, 1),
                             TOS(st, 0));
      break;

      /* Concatenate a standard string to the end of a short string.
       *
       *   function sstrcat(string1 : shortstring, string2 : standard) : shortstring;
       *
       * ON INPUT
       *   TOS(0) = pointer to source standard string1 data
       *   TOS(1) = length of source standard string1
       *   TOS(2) = string2 allocation size
       *   TOS(3) = pointer to dest short string2 data
       *   TOS(4) = length of dest short string2
       * ON OUTPUT
       *   TOS(0) = string2 allocation size (unchanged)
       *   TOS(1) = pointer to dest short string2 (unchanged)
       *   TOS(2) = new length of dest short string2
       */

    case lbSSTRCATSTR :

      /* Get the parameters from the stack (leaving the dest string info in
       * place).
       */

      POP(st, addr1);      /* source short string stack address */
      POP(st, uparm1);     /* source short string size */

      /* Concatenate the strings */

      errorCode = pas_strcat(st, addr1, uparm1, TOS(st, 2), &TOS(st, 1),
                             TOS(st, 0));
      break;

      /* Concatenate a short string to the end of a standard string.
       *
       *   function sstrcat(string1 : shortstring, string2 : standard) : shortstring;
       *
       * ON INPUT
       *   TOS(0) = string1 allocation size
       *   TOS(1) = pointer to source short string1 data
       *   TOS(2) = length of source short string1
       *   TOS(3) = pointer to dest standard string2 data
       *   TOS(4) = length of dest standard string2
       * ON OUTPUT
       *   TOS(0) = pointer to dest standard string2 (unchanged)
       *   TOS(1) = new length of dest standard string2
       */

    case lbSTRCATSSTR :

      /* Get the parameters from the stack (leaving the dest string info in
       * place).
       */

      DISCARD(st, 1);      /* discard the source string allocation size */
      POP(st, addr1);      /* source short string stack address */
      POP(st, uparm1);     /* source short string size */

      /* Concatenate the strings */

      errorCode = pas_strcat(st, addr1, uparm1, TOS(st, 0), &TOS(st, 1),
                             st->stralloc);
      break;

      /* Concatenate a character  to the end of a string.
       *   function strcatc(name : string, c : char) : string;
       *
       * ON INPUT
       *   TOS(0) = character to concatenate
       *   TOS(1) = pointer to string
       *   TOS(2) = length of string
       * ON OUTPUT
       *   TOS(0) = pointer to string
       *   TOS(1) = new length of string
       */

    case lbSTRCATC :

      /* Get the parameters from the stack (leaving the string reference
       * in place.
       */

      POP(st, uparm1);    /* Character to concatenate */

      errorCode = pas_strcatc(st, uparm1, TOS(st, 0), &TOS(st, 1),
                              st->stralloc);
      break;

      /* Concatenate a character to the end of a short string.
       *
       *   function strcatc(name : shortstring, c : char) : shortstring;
       *
       * ON INPUT
       *   TOS(0) = character to concatenate
       *   TOS(1) = short string allocation
       *   TOS(2) = pointer to short string allocation
       *   TOS(3) = length of short string
       * ON OUTPUT
       *   TOS(0) = short string allocation (unchanged)
       *   TOS(1) = pointer to short string allocation (unchanged)
       *   TOS(2) = new length of short string
       */

    case lbSSTRCATC :

      /* Get the parameters from the stack (leaving the string reference
       * in place.
       */

      POP(st, uparm1);    /* Character to concatenate */

      errorCode = pas_strcatc(st, uparm1, TOS(st, 2), &TOS(st, 1),
                              TOS(st, 0));
      break;

      /* Compare two pascal standard strings
       *
       *   function strcmp(name1 : string, name2 : string) : integer;
       *
       * ON INPUT
       *   TOS(0) = address of standard string2 data
       *   TOS(1) = length of standard string2
       *   TOS(2) = address of standard string1 data
       *   TOS(3) = length of sstandard tring1
       * ON OUTPUT
       *   TOS(0) = (-1=less than, 0=equal, 1=greater than}
       */

    case lbSTRCMP :
      {
        int result;

        /* Get the parameters from the stack (leaving space for the
         * return value);
         */

        POP(st, addr2);      /* Address of string2 data */
        POP(st, uparm2);     /* Length of string2 */
        POP(st, addr1);      /* Address of string1 data */
        uparm1 = TOS(st, 0); /* Length of string1 */

        /* Get full address */

        dest   = ATSTACK(st, addr1);
        src    = ATSTACK(st, addr2);

        /* If name1 is shorter than name2, then we can only return
         * -1 (less than) or +1 greater than.  If the substrings
         * of length of name1 are equal, then we return less than.
         */

        if (uparm1 < uparm2)
          {
            result = memcmp(dest, src, uparm1);
            if (result == 0) result = -1;
          }

        /* If name1 is longer than name2, then we can only return
         * -1 (less than) or +1 greater than.  If the substrings
         * of length of name2 are equal, then we return greater than.
         */

        else if (uparm1 > uparm2)
          {
            result = memcmp(dest, src, uparm2);
            if (result == 0) result = 1;
          }

        /* The strings are of equal length. Return the result of
         * the comparison.
         */

        else
          {
            result = memcmp(dest, src, uparm1);
          }

        TOS(st, 0) = result;
      }
      break;

      /* Compare two pascal short strings
       *
       *   function sstrcmp(name1 : shortstring, name2 : shortstring) : integer;
       *
       * ON INPUT
       *   TOS(0) = size of short string2 allocation
       *   TOS(1) = address of short string2 data
       *   TOS(2) = length of short string2
       *   TOS(3) = size of short string1 allocation
       *   TOS(4) = address of short string1 data
       *   TOS(5) = length of short string1
       * ON OUTPUT
       *   TOS(0) = (-1=less than, 0=equal, 1=greater than}
       */

    case lbSSTRCMP :
      errorCode = eNOTYET;
      break;

      /* Compare a pascal short string to a pascal standard string
       *
       *   function sstrcmpstr(name1 : shortstring, name2 : string) : integer;
       *
       * ON INPUT
       *   TOS(0) = address of standard string2 data
       *   TOS(1) = length of standard string2
       *   TOS(2) = size of short string1 allocation
       *   TOS(3) = address of short string1 data
       *   TOS(4) = length of short string1
       * ON OUTPUT
       *   TOS(0) = (-1=less than, 0=equal, 1=greater than}
       */

    case lbSSTRCMPSTR :
      errorCode = eNOTYET;
      break;


      /* Compare a pascal standard string to a pascal short string
       *
       *   function sstrcmpstr(name1 : string, name2 : shortstring) : integer;
       *
       * ON INPUT
       *   TOS(0) = address of standard string2 data
       *   TOS(1) = length of standard string2
       *   TOS(2) = size of short string1 allocation
       *   TOS(3) = address of short string1 data
       *   TOS(4) = length of short string1
       * ON OUTPUT
       *   TOS(0) = (-1=less than, 0=equal, 1=greater than}
       */

    case lbSTRCMPSSTR :
      errorCode = eNOTYET;
      break;

      /* Copy a substring from a string.
       *
       *   Copy(from : string, from, howmuch: integer) : string
       *
       * ON INPUT
       *   TOS(0) = Integer value that provides the length of the substring
       *   TOS(1) = Integer value that provides the (1-based) string position
       *   TOS(2) = Address of string data
       *   TOS(3) = Length of the string
       * ON OUTPUT
       *   TOS(0) = Address of the substring data
       *   TOS(1) = Length of the substring
       */

    case lbCOPYSUBSTR :
      POP(st, size);
      POP(st, offset);
      addr1  = TOS(st, 0);
      uparm1 = TOS(st, 1);

      /* Initialize a temporary string on the stack.
       *
       * First, check if there is space on the string stack for the new string
       * buffer.
       */

      if (st->csp + st->stralloc >= st->spb)
        {
          errorCode = eSTRSTKOVERFLOW;
        }
      else
        {
          /* Allocate and initialize a string buffer on the string stack for
           * the new string.
           */

          addr2   = INT_ALIGNUP(st->csp);
          st->csp = addr2 + st->stralloc;

          TOS(st, 0) = addr2;
          TOS(st, 1) = 0;

          /* Limit the indices to fit within the string */

          if (offset >= 1 && offset <= uparm1 && size > 0)
            {
              /* Make the character position a zero-based index */

              offset--;

              /* Limit the substring size if necesssary */

              if (size > st->stralloc)
                {
                  size = st->stralloc;
                }

              if (offset + size > uparm1)
                {
                  size = uparm1 - offset;
                }

              /* And copy the substring */

              src  = ATSTACK(st, addr1);
              dest = ATSTACK(st, addr2);
              memcpy(dest, &src[offset], size);

              TOS(st, 1) = size;
            }
        }
      break;

      /* Find a substring in a string.  Returns the (1-based) character position of
       * the substring or zero if the substring is not found.
       *
       *   Pos(substr, s : string) : integer
       *
       * ON INPUT
       *   TOS(2) = Address of string data
       *   TOS(3) = Length of the string
       *   TOS(2) = Address of substring data
       *   TOS(3) = Length of the substring
       * ON OUTPUT
       *   TOS(0) = Position of the substring (or zero if not present)
       */

    case lbFINDSUBSTR :
      {
        char *str;
        char *substr;
        char *result;

        POP(st, addr1);
        POP(st, uparm1);
        POP(st, addr2);
        POP(st, uparm2);

        /* Find the substring in the string */

        str    = (char *)ATSTACK(st, addr1);
        substr = (char *)ATSTACK(st, addr2);
        result = strstr(str, substr);

        /* strstr will NULL if the stubstring is not found but, oddly, will
         * return result == str if substr is 0.
         */

        if (result != NULL)
          {
            offset = (uintptr_t)result - (uintptr_t)str + 1;
          }
        else
          {
            /* Character position zero means that the substring was not
             * found.
             */

            offset = 0;
          }

        PUSH(st, offset);
      }
      break;

      /* Insert a string into another string.
       *
       *   Insert(source : string, VAR target : string, index: integer) : string
       *
       * ON INPUT
       *   TOS(0) = Integer value that provides the (1-based) string position
       *   TOS(1) = Address of the target string to be modified
       *   TOS(2) = Address of source string data
       *   TOS(3) = Length of the source string
       * ON OUTPUT
       */

    case lbINSERTSTR :
      {
        ustack_t *dptr;
        ustack_t ulimit1;
        ustack_t ulimit2;
        int i;
        int j;

        POP(st, offset);   /* One-based position of first character to delete */
        POP(st, addr2);    /* Address of target string variable */

        /* Get the string to be modified */

        dptr     = (ustack_t *)ATSTACK(st, addr2);
        addr1    = dptr[BTOISTACK(sSTRING_DATA_OFFSET)];
        uparm1   = dptr[BTOISTACK(sSTRING_SIZE_OFFSET)];

        /* Get the source string to be inserted */

        POP(st, addr2);
        POP(st, uparm2);

        /* Make the character position a zero-based index */

        if (--offset < 0)
          {
            offset = 0;
          }

        /* Open up a space for the string by movi text at the end of the
         * string.
         */

        dest = (uint8_t *)ATSTACK(st, addr1);

        ulimit1 = uparm1 + uparm2;
        if (ulimit1 > st->stralloc)
          {
            ulimit1 = st->stralloc;
          }

        for (i = ulimit1 - uparm2 - 1, j = ulimit1 - 1; i >= offset; i--, j--)
          {
            dest[j] = dest[i];
          }

        /* Copy the source string into this space. */

        src = (uint8_t *)ATSTACK(st, addr2);

        ulimit2 = uparm2 + offset;
        if (ulimit2 > ulimit1)
          {
            ulimit1 = ulimit1;
          }

        for (i = 0, j = offset; j < ulimit2; i++, j++)
          {
            dest[j] = src[i];
          }

        /* Adjust the size of string */

        dptr[BTOISTACK(sSTRING_SIZE_OFFSET)] = ulimit1;
      }
      break;

      /* Delete a substring from a string.
       *
       *   Delete(VAR from : string, from, howmuch: integer) : string
       *
       * ON INPUT
       *   TOS(0) = Integer value that provides the length of the substring
       *   TOS(1) = Integer value that provides the (1-based) string position
       *   TOS(3) = Address of the string variable to modify
       * ON OUTPUT
       */

    case lbDELSUBSTR :
      {
        ustack_t *sptr;
        int i;
        int j;

        POP(st, size);     /* Number of characters to delete */
        POP(st, offset);   /* One-based position of first character to delete */
        POP(st, addr1);    /* Address of the string to be modified */

        /* Get the string to be modified */

        sptr     = (ustack_t *)ATSTACK(st, addr1);
        addr2    = sptr[BTOISTACK(sSTRING_DATA_OFFSET)];
        uparm2   = sptr[BTOISTACK(sSTRING_SIZE_OFFSET)];

        /* Make the character position a zero-based index */

        if (--offset < 0)
          {
            offset = 0;
          }

        /* Move text at the end of the string to fill the gap. */

        dest = (uint8_t *)ATSTACK(st, addr2);

        for (i = offset, j = offset + size; j < uparm2; i++, j++)
          {
            dest[i] = dest[j];
          }

        /* Adjust the size of string */

        if (offset + size > uparm2)
          {
            size = uparm2 - offset;
          }

        sptr[BTOISTACK(sSTRING_SIZE_OFFSET)] -= size;
      }
      break;

      /* Convert a string to a numeric value
       *   procedure val(const s : string; var v; var code : word);
       *
       * Description:
       * val() converts the value represented in the string S to a numerical
       * value, and stores this value in the variable V, which can be of type
       * LInteger, Longinteger, ShortInteger, or Real. If the conversion isn't
       * succesfull, then the parameter Code contains the index of the character
       * in S which prevented the conversion. The string S is allowed to contain
       * spaces in the beginning.
       *
       * The string S can contain a number in decimal, hexadecimal, binary or
       * octal format, as described in the language reference.
       *
       * Errors:
       * If the conversion doesn't succeed, the value of Code indicates the
       * position where the conversion went wrong.
       *
       * ON INPUT
       *   TOS(0) = address of code
       *   TOS(1) = address of value
       *   TOS(2) = length of source string
       *   TOS(3) = pointer to source string
       * ON RETURN: actual parameters released
       */

    case lbVAL :
      POP(st, addr1);                            /* Pointer to error code */
      POP(st, addr2);                            /* Pointer to string value */
      POP(st, size);                             /* Size of string */
      POP(st, uparm1);                           /* Address of string buffer */

      /* Make a C string out of the pascal string */

      src       = (uint8_t *)ATSTACK(st, uparm1);
      src[size] = '\0';
      name      = pexec_mkcstring(src, size);
      if (name == NULL)
        {
          errorCode = eNOMEMORY;
        }
      else
        {
          long longValue;
          char *endptr;

          /* Convert the string to an integer */

          longValue = strtol((char *)name, &endptr, 0);
          if (longValue < MININT || longValue > MAXINT)
            {
              errorCode = eINTEGEROVERFLOW;
            }
          else
            {
              PUTSTACK(st, (ustack_t)*endptr, addr1);
              PUTSTACK(st, (ustack_t)longValue, addr2);
            }
        }
      break;

    default :
      errorCode = eBADSYSLIBCALL;
      break;
    }

  return errorCode;
}
