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
static void     pas_Cstr2str(struct pexec_s *st, uint8_t *srcCString,
                             uint16_t destVarAddr, uint16_t varOffset);
static int      pas_Bstr2str(struct pexec_s *st, uint16_t arrayAddress,
                             uint16_t arraySize);
static int      pas_Str2bstr(struct pexec_s *st, uint16_t arrayAddress,
                             uint16_t arraySize, uint16_t stringBufferAddress,
                             uint16_t stringSize, uint16_t offset);

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

static void pas_Cstr2str(struct pexec_s *st, uint8_t *srcCString,
                         uint16_t destVarAddr, uint16_t varOffset)
{
  /* Copy C string to a pascal string */

  uint8_t *destStringBuffer;
  int len;

  /* Offset the destination address */

  destVarAddr += varOffset;

  /* Get the destination string pointer */

  destStringBuffer = ATSTACK(st, destVarAddr + sSTRING_DATA_OFFSET);

  /* Handle null src pointer */

  if (srcCString == NULL)
    {
      *destStringBuffer = 0;
    }
  else
    {
      /* Get the length of the string */

      len = strlen((char *)srcCString);

      /* Make sure that the string length will fit into the destination. */

      if (len >= st->stralloc)
        {
          /* Clip to the maximum size */

          len  = st->stralloc;
        }

      /* Then transfer the string contents */

      memcpy(destStringBuffer, srcCString, len);
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

  src  = (const char *)&GETSTACK(st, arrayAddress);

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

      dest = (char *)&GETSTACK(st, bufferAddress);
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

  src  = (const char *)&GETSTACK(st, stringBufferAddress);
  dest = (char *)&GETSTACK(st, arrayAddress + offset);

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
  int32_t   value;
  int       errorCode = eNOERROR;

  switch (subfunc)
    {
      /* Halt processing */

    case lbHALT:
      errorCode = eEXIT;
      break;

      /* Get the value of an environment string
       *
       * ON INPUT:
       *   TOS(0) = Number of bytes in environment identifier string
       *   TOS(1) = Address environment identifier string
       * ON RETURN (above replaced with):
       *   TOS(0) = MS 16-bits of 32-bit C string pointer
       *   TOS(1) = LS 16-bits of 32-bit C string pointer
       */

    case lbGETENV :
      size = TOS(st, 0);                          /* Number of bytes in string */
      src = (uint8_t *)&GETSTACK(st, TOS(st, 1)); /* Pointer to string */

      /* Make a C string out of the pascal string */

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

          /* Save the returned pointer in the stack */

          TOS(st, 0) = (ustack_t)((uintptr_t)src >> 16);
          TOS(st, 1) = (ustack_t)((uintptr_t)src & 0x0000ffff);
        }
      break;

      /* Copy pascal standard string to a pascal standard string
       *   procedure strcpy(src : string; var dest : string)
       *
       * ON INPUT:
       *   TOS(0) = address of dest string variable
       *   TOS(1) = pointer to source string buffer
       *   TOS(2) = length of source string
       * ON RETURN (input consumed):
       */

    case lbSTRCPY :
      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Address of dest string variable  */
      POP(st, addr2);  /* Address of source string buffer */
      POP(st, size);   /* Length of valid source data */

      /* And perform the string copy */

      pas_strcpy(st, addr2, size, addr1, st->stralloc, 0);
      break;

      /* Copy pascal standard string to a element of a pascal standard string
       * array
       *
       *   procedure strcpy(src : string; var dest : string;
       *                    offset : integer)
       *
       * ON INPUT:
       *   TOS(0) = Address of dest string variable
       *   TOS(1) = Pointer to source string buffer
       *   TOS(2) = Length of source string
       *   TOS(3) = Dest string variable address offset
       * ON RETURN: actual parameters released.
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

      /* Copy pascal short string to a pascal short string
       *
       * ON INPUT:
       *   TOS(0) = Address of dest short short string variable
       *   TOS(1) = Pointer to source short string buffer
       *   TOS(2) = Length of source short string
       *   TOS(3) = Short string buffer size
       * ON RETURN (input consumed):
       */

    case lbSSTRCPY :
      {
        uint16_t *strPtr;
        uint16_t  strAlloc;

        /* "Pop" in the input parameters from the stack */

        POP(st, addr1);  /* Address of dest short string variable  */
        POP(st, addr2);  /* Address of source short string buffer */
        POP(st, size);   /* Length of valid source data */
        DISCARD(st, 1);  /* Source short string buffer allocation */

        /* Get the allocation size of the short string destination */

        strPtr = (uint16_t *)&st->dstack.b[addr1];
        strAlloc = strPtr[sSHORTSTRING_ALLOC_OFFSET / sINT_SIZE];

        /* And perform the string copy */

        pas_strcpy(st, addr2, size, addr1, strAlloc, 0);
      }
      break;

      /* Copy pascal short string to a element of a pascal short string
       * array
       *
       * ON INPUT:
       *   TOS(0) = Address of dest short short string variable
       *   TOS(1) = Pointer to source short string buffer
       *   TOS(2) = Length of source short string
       *   TOS(3) = Short string buffer size
       *   TOS(4) = Dest short string variable address offset
       * ON RETURN (input consumed):
       */

    case lbSSTRCPYX :
      {
        uint16_t *strPtr;
        uint16_t  strAlloc;

        /* "Pop" in the input parameters from the stack */

        POP(st, addr1);  /* Address of dest short string variable  */
        POP(st, addr2);  /* Address of source short string buffer */
        POP(st, size);   /* Length of valid source data */
        DISCARD(st, 1);  /* Source short string buffer allocation */
        POP(st, offset); /* Offset from dest string address */

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

      /* Copy pascal short string to a pascal standard string
       *
       * ON INPUT:
       *   TOS(0) = Address of dest standard standard string variable
       *   TOS(1) = Pointer to source short string buffer
       *   TOS(2) = Length of source short string
       *   TOS(3) = Short string buffer size
       * ON RETURN (input consumed):
       */

    case lbSSTR2STR :
      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Address of dest standard string variable  */
      POP(st, addr2);  /* Address of source short string buffer */
      POP(st, size);   /* Length of valid short string source data */
      DISCARD(st, 1);  /* Short string buffer size */

      /* And perform the string copy */

      pas_strcpy(st, addr2, size, addr1, st->stralloc, 0);
      break;

      /* Copy pascal short string to an element of a pascal standard string
       * array
       *
       * ON INPUT:
       *   TOS(0) = Address of dest standard standard string variable
       *   TOS(1) = Pointer to source short string buffer
       *   TOS(2) = Length of source short string
       *   TOS(3) = Short string buffer size
       *   TOS(4) = Dest standard string variable address offset
       * ON RETURN (input consumed):
       */

    case lbSSTR2STRX :
      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Address of dest standard string variable  */
      POP(st, addr2);  /* Address of source short string buffer */
      POP(st, size);   /* Length of valid short string source data */
      DISCARD(st, 1);  /* Short string buffer size */
      POP(st, offset); /* Offset from dest string address */

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
       */

    case lbSTR2SSTR :
      {
        uint16_t *strPtr;
        uint16_t  strAlloc;

        /* "Pop" in the input parameters from the stack */

        POP(st, addr1);  /* Address of dest short string variable  */
        POP(st, addr2);  /* Address of source standard string buffer */
        POP(st, size);   /* Length of source standard string */

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
       */

    case lbSTR2SSTRX :
      {
        uint16_t *strPtr;
        uint16_t  strAlloc;

        /* "Pop" in the input parameters from the stack */

        POP(st, addr1);  /* Address of dest short string variable  */
        POP(st, addr2);  /* Address of source standard string buffer */
        POP(st, size);   /* Length of source standard string */
        POP(st, offset); /* Dest short string variable address offset */

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

      /* Copy C string to a pascal standard string
       *
       * ON INPUT:
       *   TOS(0) = address of dest standard string
       *   TOS(1) = MS 16-bits of 32-bit C string pointer
       *   TOS(2) = LS 16-bits of 32-bit C string pointer
       * ON RETURN (input consumed):
       */

    case lbCSTR2STR :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* addr of dest standard string */
      POP(st, uparm1); /* MS 16-bits of 32-bit C string pointer */
      POP(st, uparm2); /* LS 16-bits of 32-bit C string pointer */

      /* Get the source string pointer */

      src  = (uint8_t *)((uintptr_t)uparm1 << 16 |
                         (uintptr_t)uparm2);

      /* And perform the copy */

      pas_Cstr2str(st, src, addr1, 0);
      break;

    /* Copy pascal string to a element of a pascal string array
     *
     *   procedure strcpy(src : string; var dest : string; offset : integer)
     *
     * ON INPUT:
     *   TOS(0) = Address of dest string variable
     *   TOS(1) = Pointer to source string buffer
     *   TOS(2) = Length of source string
     *   TOS(3) = Dest string variable address offset
     * ON RETURN: actual parameters released.
     */

    case lbCSTR2STRX :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Addr of dest string header */
      POP(st, uparm1); /* MS 16-bits of 32-bit C string pointer */
      POP(st, uparm2); /* LS 16-bits of 32-bit C string pointer */
      POP(st, offset); /* Offset from dest string address */

      /* Get the source string pointer */

      src  = (uint8_t *)((uintptr_t)uparm1 << 16 |
                         (uintptr_t)uparm2);

      /* And perform the copy */

      pas_Cstr2str(st, src, addr1, offset);
      break;

      /* Copy C string to a pascal short string
       *
       * ON INPUT:
       *   TOS(0) = address of dest short string
       *   TOS(1) = MS 16-bits of 32-bit C string pointer
       *   TOS(2) = LS 16-bits of 32-bit C string pointer
       * ON RETURN (input consumed):
       */

    case lbCSTR2SSTR :
      errorCode = eNOTYET;
      break;

      /* Copy C string to an element of a pascal short string array
       *
       * ON INPUT:
       *   TOS(0) = address of dest short string
       *   TOS(1) = MS 16-bits of 32-bit C string pointer
       *   TOS(2) = LS 16-bits of 32-bit C string pointer
       *   TOS(3)   = Dest short string variable address offset
       * ON RETURN (input consumed):
       */

    case lbCSTR2SSTRX :
      errorCode = eNOTYET;
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

      /* Convert a string to a numeric value
       *   procedure val(const s : string; var v; var code : word);
       *
       * Description:
       * val() converts the value represented in the string S to a numerical
       * value, and stores this value in the variable V, which can be of type
       * Longint, Real and Byte. If the conversion isn't succesfull, then the
       * parameter Code contains the index of the character in S which
       * prevented the conversion. The string S is allowed to contain spaces
       * in the beginning.
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
      /* Get the string information */

      size = TOS(st, 2);                           /* Number of bytes in string */
      src  = (uint8_t *)&GETSTACK(st, TOS(st, 3)); /* Pointer to string */

      /* Make a C string out of the pascal string */

      name = pexec_mkcstring(src, size);
      if (name == NULL)
        {
          errorCode = eNOMEMORY;
        }
      else
        {
          /* Convert the string to an integer */

          value = atoi((char *)name);
          if ((value < MININT) || (value > MAXINT))
            {
              errorCode = eINTEGEROVERFLOW;
            }
          else
            {
              PUTSTACK(st, TOS(st, 0), 0);
              PUTSTACK(st, TOS(st, 1), value);
              DISCARD(st, 4);
            }
        }
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

      /* Check if there is space on the string stack for the new string
       * FIXME:  This logic does not handle strings with other than the
       * default size!
       */

      if (st->csp + st->stralloc >= st->spb)
        {
          errorCode = eSTRSTKOVERFLOW;
        }
      else
        {
          /* Allocate space on the string stack for the new string */

          addr2    = INT_ALIGNUP(st->csp);
          st->csp += st->stralloc;                    /* Allocate max size */

          /* Copy the string into the string stack */

          src      = (uint8_t *)&GETSTACK(st, addr1); /* Pointer to original string */
          dest     = (uint8_t *)&GETSTACK(st, addr2); /* Pointer to new string */
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
       *   TOS(0) = pointer to original short string
       *   TOS(1) = length of original short string
       *   TOS(2) = allocation of original short string
       * ON RETURN
       *   TOS(0) = pointer to new short string
       *   TOS(1) = length of new short string
       *   TOS(2) = allocation of new short string (unchanged)
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
          st->csp += st->stralloc;                     /* Allocate max size */

          /* Save the length at the beginning of the copy */

          dest     = (uint8_t *)&GETSTACK(st, addr2);  /* Pointer to new string */

          /* Copy the character into the string stack */

          *dest++  = TOS(st, 0);                       /* Save character as string */

          /* Update the stack content */

          TOS(st, 0) = 1;                             /* String length */
          PUSH(st, addr2);                            /* String address */
        }
      break;

      /* Concatenate a standard string to the end of a standard string.
       *
       *   function strcat(string1 : string, string2 : string) : string;
       *
       * ON INPUT
       *   TOS(0) = pointer to source standard string1 data
       *   TOS(1) = length of source standard string1
       *   TOS(2) = pointer to dest standard string2 data
       *   TOS(3) = length of dest standard string2
       * ON OUTPUT
       *   TOS(0) = pointer to dest standard string2 (unchanged)
       *   TOS(1) = new length of dest standard string2
       */

    case lbSTRCAT :
      /* Get the parameters from the stack (leaving the string reference
       * in place).
       */

      POP(st, addr1);       /* string1 data stack addr */
      POP(st, uparm1);      /* string1 size */
      uparm2 = TOS(st, 1);  /* string2 size */

      /* Check for string overflow. */

      if (uparm1 + uparm2 > st->stralloc)
        {
          errorCode = eSTRSTKOVERFLOW;
        }
      else
        {
          /* Get a pointer to string1 data */

          src  = ATSTACK(st, addr1);

          /* Get a pointer to string2 buffer, set new size then, get
           * a pointer to string2 data.
           */

          dest = (uint8_t *)&GETSTACK(st, TOS(st, 0));
          memcpy(&dest[uparm2], src, uparm1);  /* cat strings */
          TOS(st, 1) = uparm1 + uparm2;        /* Save new size */
        }
      break;

      /* Concatenate a short string to the end of a short string.
       *
       *   function sstrcat(string1 : shortstring, string2 : shortstring) : shortstring;
       *
       * ON INPUT
       *   TOS(0) = pointer to source short string1 data
       *   TOS(1) = length of source short string1
       *   TOS(2) = string1 allocation size
       *   TOS(3) = pointer to dest short string2 data
       *   TOS(4) = length of dest short string2
       *   TOS(5) = string2 allocation size
       * ON OUTPUT
       *   TOS(0) = pointer to dest short string2 (unchanged)
       *   TOS(1) = new length of dest short string2
       *   TOS(2) = string2 allocation size (unchanged)
       */

    case lbSSTRCAT :
      errorCode = eNOTYET;
      break;

      /* Concatenate a standard string to the end of a short string.
       *
       *   function sstrcat(string1 : shortstring, string2 : standard) : shortstring;
       *
       * ON INPUT
       *   TOS(0) = pointer to source standard string1 data
       *   TOS(1) = length of source standard string1
       *   TOS(2) = pointer to dest short string2 data
       *   TOS(3) = length of dest short string2
       *   TOS(4) = string2 allocation size
       * ON OUTPUT
       *   TOS(0) = pointer to dest short string2 (unchanged)
       *   TOS(1) = new length of dest short string2
       *   TOS(2) = string2 allocation size (unchanged)
       */

    case lbSSTRCATSTR :
      errorCode = eNOTYET;
      break;

      /* Concatenate a short string to the end of a standard string.
       *
       *   function sstrcat(string1 : shortstring, string2 : standard) : shortstring;
       *
       * ON INPUT
       *   TOS(0) = pointer to source short string1 data
       *   TOS(1) = length of source short string1
       *   TOS(2) = string1 allocation size
       *   TOS(3) = pointer to dest standard string2 data
       *   TOS(4) = length of dest standard string2
       * ON OUTPUT
       *   TOS(0) = pointer to dest standard string2 (unchanged)
       *   TOS(1) = new length of dest standard string2
       */

    case lbSTRCATSSTR :
      errorCode = eNOTYET;
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
      size = TOS(st, 1);  /* Current length of string */

      /* Check for string overflow.  FIXME:  This logic does not handle
       * strings with other than the default size!
       */

      if (size + 1 >= st->stralloc)
        {
          errorCode = eSTRSTKOVERFLOW;
        }
      else
        {
          /* Add the new charcter */

          dest       = ((uint8_t *)&GETSTACK(st, TOS(st, 0)));
          dest[size] = (uint8_t)uparm1;

          /* Save the new string size */

          TOS(st, 1) = size + 1;
        }
      break;

      /* Concatenate a character to the end of a short string.
       *
       *   function strcatc(name : shortstring, c : char) : shortstring;
       *
       * ON INPUT
       *   TOS(0) = character to concatenate
       *   TOS(1) = pointer to short string allocation
       *   TOS(2) = length of short string
       *   TOS(3) = short string allocation
       * ON OUTPUT
       *   TOS(0) = pointer to short string allocation (unchanged)
       *   TOS(1) = new length of short string
       *   TOS(2) = short string allocation (unchanged)
       */

    case lbSSTRCATC :
      errorCode = eNOTYET;
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
       *   TOS(0) = address of short string2 data
       *   TOS(1) = length of short string2
       *   TOS(2) = size of short string2 allocation
       *   TOS(3) = address of short string1 data
       *   TOS(4) = length of short string1
       *   TOS(3) = size of short string1 allocation
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
       *   TOS(2) = address of short string1 data
       *   TOS(3) = length of short string1
       *   TOS(4) = size of short string1 allocation
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
       *   TOS(2) = address of short string1 data
       *   TOS(3) = length of short string1
       *   TOS(4) = size of short string1 allocation
       * ON OUTPUT
       *   TOS(0) = (-1=less than, 0=equal, 1=greater than}
       */

    case lbSTRCMPSSTR :
      errorCode = eNOTYET;
      break;

    default :
      errorCode = eBADSYSLIBCALL;
      break;
    }

  return errorCode;
}
