/****************************************************************************
 * libexec_stringlib.c
 * Pascal run-time library
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

#include "pas_debug.h"
#include "pas_machine.h"
#include "pas_stringlib.h"
#include "pas_errcodes.h"

#include "libexec.h"
#include "libexec_heap.h"
#include "libexec_longops.h"   /* For libexec_UPop32() */
#include "libexec_sysio.h"     /* For libexec_GetFormat() */
#include "libexec_stringlib.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int      libexec_StrInit(struct libexec_s *st, uint16_t strVarAddr,
                   uint16_t strAllocSize);
static void     libexec_StrCpy(struct libexec_s *st, uint16_t srcBufferAddr,
                   uint16_t srcStringSize, uint16_t destVarAddr,
                   uint16_t varOffset);
static int      libexec_BStr2Str(struct libexec_s *st, uint16_t arrayAddress,
                   uint16_t arraySize);
static int      libexec_Str2BStr(struct libexec_s *st, uint16_t arrayAddress,
                   uint16_t arraySize, uint16_t stringBufferAddress,
                   uint16_t stringSize, uint16_t offset);
static int      libexec_StrDup(struct libexec_s *st, uint16_t strAddr,
                   uint16_t strSize, uint16_t *strClone);
static int      libexec_StrCat(struct libexec_s *st, uint16_t srcStringAddr,
                   uint16_t srcStringSize, uint16_t destStringAddr,
                   uint16_t *pDestStringSize, uint16_t destStrAlloc);
static int      libexec_StrCatC(struct libexec_s *st, char srcChar,
                   uint16_t destStringAddr, uint16_t *pDestStringSize,
                   uint16_t destStrAlloc);
static int      libexec_FillChar(struct libexec_s *st, ustack_t *sptr,
                   uint16_t count, uint8_t value);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int libexec_StrInit(struct libexec_s *st, uint16_t strVarAddr,
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
       */

      PUTSTACK(st, strAllocAddr, strVarAddr + sSTRING_DATA_OFFSET);
      PUTSTACK(st, 0,            strVarAddr + sSTRING_SIZE_OFFSET);
    }

  return errorCode;
}

static void libexec_StrCpy(struct libexec_s *st, uint16_t srcBufferAddr,
                           uint16_t srcStringSize, uint16_t destVarAddr,
                           uint16_t varOffset)
{
  /* Copy pascal string to a pascal string */

  uint16_t   *strPtr;
  uint16_t    strAlloc;
  uint16_t    destBufferAddr;
  uint16_t    destBufferSize;
  const char *src;
  char       *dest;

  /* Offset the destination address */

  destVarAddr += varOffset;

  /* Get the allocation size of the string destination */

  strPtr         = (uint16_t *)&st->dstack.b[destVarAddr];
  strAlloc       = strPtr[sSTRING_ALLOC_OFFSET / sINT_SIZE];
  destBufferSize = strAlloc & HEAP_SIZE_MASK;

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

      dest = (char *)ATSTACK(st, destBufferAddr);
      src  = (char *)ATSTACK(st, srcBufferAddr);
      memcpy(dest, src, srcStringSize);

      /* And set the new string size */

      PUTSTACK(st, srcStringSize, destVarAddr + sSTRING_SIZE_OFFSET);
    }
}

static int libexec_BStr2Str(struct libexec_s *st, uint16_t arrayAddress,
                            uint16_t arraySize)
{
  const char *src;
  char *dest;
  uint16_t bufferAddress;
  uint16_t bufferAllocation;
  int errorCode = eNOERROR;
  int stringLength;

  /* Get a pointer to the array in the stack */

  src = (const char *)ATSTACK(st, arrayAddress);

  /* Get the length of the string in the array.  Here we assume that the
   * string is represented as a NUL-terminated C string.
   */

  stringLength = strnlen(src, arraySize);

  /* Clip the string if necessary to fit into the string buffer allocation */

  if (stringLength > STRING_BUFFER_SIZE)
   {
     stringLength = STRING_BUFFER_SIZE;
   }

  /* Allocate the temporary string from heap */

  bufferAddress = libexec_AllocTmpString(st, STRING_BUFFER_SIZE,
                                         &bufferAllocation);
  if(bufferAddress == 0)
    {
      errorCode = eNOMEMORY;
    }
  else
    {
      /* Copy the array into the string buffer */

      dest = (char *)ATSTACK(st, bufferAddress);
      memcpy(dest, src, stringLength);

      /* Put the new string at the top of the stack.  Order:
       *
       *   TOS(n)     = 16-bit pointer to the string data.
       *   TOS(n + 1) = String size
       */

      PUSH(st, stringLength);     /* String length */
      PUSH(st, bufferAddress);    /* String buffer address */
      PUSH(st, bufferAllocation); /* String buffer allocated size */
    }

  return errorCode;
}

static int libexec_Str2BStr(struct libexec_s *st, uint16_t arrayAddress,
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

static int libexec_StrDup(struct libexec_s *st, uint16_t strAddr,
                          uint16_t strSize, uint16_t *StrClone)
{
  const char *copySource;
  char *copyDest;
  int errorCode = eNOERROR;
  uint16_t cloneAddr;
  uint16_t strAlloc;

  /* Allocated string memory from the heap for the temporary string.
   *
   * How big should we make the dup'ed clone string?  The same size might to
   * small so let's use the default to be safe.
   */

  cloneAddr = libexec_AllocTmpString(st, STRING_BUFFER_SIZE, &strAlloc);
  if (cloneAddr == 0)
    {
      errorCode = eNOMEMORY;
    }
  else
    {
      /* Limit the size to the maximum size of the allocated string buffer.
       * This can happen in cases where the string address lies in RO string
       * memory.
       */

      if (strSize > STRING_BUFFER_SIZE)
        {
          strSize = STRING_BUFFER_SIZE;
        }

      /* Copy the string into the string stack */

      copySource = (const char *)ATSTACK(st, strAddr); /* Pointer to original string */
      copyDest   = (char *)ATSTACK(st, cloneAddr);     /* Pointer to new string */
      memcpy(copyDest, copySource, strSize);

      /* Return the cloned string */

      StrClone[BTOISTACK(sSTRING_SIZE_OFFSET)]  = strSize;
      StrClone[BTOISTACK(sSTRING_DATA_OFFSET)]  = cloneAddr;
      StrClone[BTOISTACK(sSTRING_ALLOC_OFFSET)] = strAlloc;
    }

  return errorCode;
}

static int libexec_StrCat(struct libexec_s *st, uint16_t srcStringAddr,
                          uint16_t srcStringSize, uint16_t destStringAddr,
                          uint16_t *pDestStringSize, uint16_t destStrAlloc)
{
  uint16_t destStringSize = *pDestStringSize;
  const char *src;
  char *dest;
  int errorCode = eNOERROR;

  /* Will the concatenated string fit in the destination.  No?  Should we
   * force a run-time error?  Or just truncate it to fit?
   */

  if (srcStringSize + destStringSize > destStrAlloc)
    {
      srcStringSize = destStrAlloc - destStringSize;
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

static int libexec_StrCatC(struct libexec_s *st, char srcChar,
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

/* Fill string s with character value until s is count-1 char long. */

static int libexec_FillChar(struct libexec_s *st, ustack_t *sptr, uint16_t count,
                            uint8_t value)
{
  uint16_t  strAlloc;
  uint16_t  strAddr;
  uint16_t  strSize;
  uint16_t  limit;
  char     *dest;
  int i;

  /* Get the existing size of the target string and a pointer to the
   * target string allocation.
   */

  strAlloc = sptr[BTOISTACK(sSTRING_ALLOC_OFFSET)] & HEAP_SIZE_MASK;
  strAddr  = sptr[BTOISTACK(sSTRING_DATA_OFFSET)];
  strSize  = sptr[BTOISTACK(sSTRING_SIZE_OFFSET)];

  dest     = (char *)ATSTACK(st, strAddr) + strSize;

  /* Pad until the length is count - 1 characters long or until there is no
   * available space in the allocated string memory.
   */

  limit = count;
  if (limit > strAlloc)
    {
      limit = strAlloc;
    }

  for (i = strSize; i < limit; i++)
    {
      *dest++ = value;
    }

  /* Save the new size of the string */

  sptr[BTOISTACK(sSTRING_SIZE_OFFSET)] = i;
  return eNOERROR;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: libexec_StringOperations
 *
 * Description:
 *   This function process a string operation
 *
 ****************************************************************************/

uint16_t libexec_StringOperations(struct libexec_s *st, uint16_t subfunc)
{
  ustack_t  uparm1;
  ustack_t  offset;
  ustack_t  size;
  pasSize_t addr1;
  pasSize_t addr2;
  char     *dest;
  char     *name;
  int       errorCode = eNOERROR;

  switch (subfunc)
    {
      /* Copy pascal string to a pascal string
       *
       * ON INPUT:
       *   TOS(0) = Address of dest string variable
       *   TOS(1) = String buffer size
       *   TOS(2) = Pointer to source string buffer
       *   TOS(3) = Length of source string
       * ON RETURN (input consumed):
       *
       * NOTE:  The alternate version is equivalent but has the dest
       * address and source string reversed.
       */

    case lbSTRCPY2 :

      /* "Pop" in the input parameters from the stack */

      POP(st, uparm1); /* Source string buffer allocation */
      POP(st, addr2);  /* Address of source string buffer */
      POP(st, size);   /* Length of valid source data */
      POP(st, addr1);  /* Address of dest string variable  */
      goto strcpy_common;

    case lbSTRCPY :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Address of dest string variable  */
      POP(st, uparm1); /* Source string buffer allocation */
      POP(st, addr2);  /* Address of source string buffer */
      POP(st, size);   /* Length of valid source data */

    strcpy_common :
      {
        /* And perform the string copy */

        libexec_StrCpy(st, addr2, size, addr1, 0);

        /* A string was consumed so we may have to free the temporary
         * heap memory allocation for its string buffer.
         */

        libexec_FreeTmpString(st, addr2, uparm1);
      }
      break;

      /* Copy pascal string to a element of a pascal string
       * array
       *
       * ON INPUT:
       *   TOS(0) = Address of dest string variable
       *   TOS(1) = String buffer size
       *   TOS(2) = Pointer to source string buffer
       *   TOS(3) = Length of source string
       *   TOS(4) = Dest string variable address offset
       * ON RETURN (input consumed):
       *
       * NOTE:  The alternate version is equivalent but has the dest
       * address and source string reversed.
       */

    case lbSTRCPYX2 :

      /* "Pop" in the input parameters from the stack */

      POP(st, uparm1); /* Source string buffer allocation */
      POP(st, addr2);  /* Address of source string buffer */
      POP(st, size);   /* Length of valid source data */
      POP(st, offset); /* Offset from dest string address */
      POP(st, addr1);  /* Address of dest string variable  */
      goto strcpyx_common;

    case lbSTRCPYX :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Address of dest string variable  */
      POP(st, uparm1); /* Source string buffer allocation */
      POP(st, addr2);  /* Address of source string buffer */
      POP(st, size);   /* Length of valid source data */
      POP(st, offset); /* Offset from dest string address */

    strcpyx_common :
      {
        /* And perform the string copy */

        libexec_StrCpy(st, addr2, size, addr1, offset);

        /* A string was consumed so we may have to free the temporary
         * heap memory allocation for its string buffer.
         */

        libexec_FreeTmpString(st, addr2, uparm1);
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
     *   TOS(0) = String character buffer size
     *   TOS(1) = String character buffer address
     *   TOS(2) = String size
     */

    case lbBSTR2STR :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Address of the array */
      POP(st, size);   /* Size of the array */

      /* And perform the copy */

      errorCode = libexec_BStr2Str(st, addr1, size);
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
     *   TOS(2) = Size of the alloated string buffer (source)
     *   TOS(3) = Address of the string buffer address
     *   TOS(4) = Size of the string
     * ON RETURN:
     *   All inputs consumbed
     */

    case lbSTR2BSTR :
      {
        uint16_t arrayAddr;
        uint16_t arraySize;
        uint16_t strAlloc;
        uint16_t strAddr;
        uint16_t strSize;

        /* "Pop" in the input parameters from the stack */

        POP(st, arrayAddr); /* Address of the array */
        POP(st, arraySize); /* Size of the array */

        POP(st, strAlloc);  /* Size of the allocated string buffer */
        POP(st, strAddr);  /* Address of the array */
        POP(st, strSize); /* Size of the array */

        /* And perform the copy */

        errorCode = libexec_Str2BStr(st, arrayAddr, arraySize, strAddr,
                                     strSize, 0);

        /* We consumed a temporary string.  It may be necessary to
         * free memory associated with it.
         */

        libexec_FreeTmpString(st, strAddr, strAlloc);
      }
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
     *   TOS(2) = Size of the allocated string buffer (source)
     *   TOS(3) = Address of the string buffer
     *   TOS(4) = Size of the string
     *   TOS(5) = Array address offset
     * ON RETURN:
     *   All inputs consumed
     */

    case lbSTR2BSTRX :
      {
        uint16_t arrayAddr;
        uint16_t arraySize;
        uint16_t arrayOffset;
        uint16_t strAlloc;
        uint16_t strAddr;
        uint16_t strSize;

        /* "Pop" in the input parameters from the stack */

        POP(st, arrayAddr);   /* Address of the array */
        POP(st, arraySize);   /* Size of the array */

        POP(st, strAlloc);    /* Size of the allocated string buffer */
        POP(st, strAddr);     /* Address of the array */
        POP(st, strSize);     /* Size of the array */

        POP(st, arrayOffset); /* Array address offset */

        /* And perform the copy */

        errorCode = libexec_Str2BStr(st, arrayAddr, arraySize, strAddr,
                                     strSize, arrayOffset);

        /* We consumed a temporary string.  It may be necessary to
         * free memory associated with it.
         */

        libexec_FreeTmpString(st, strAddr, strAlloc);
      }
      break;

      /* Initialize a new string variable. Create a string buffer.  This
       * is called only at entrance into a new Pascal block.
       *
       *   TYPE
       *     string : string[size]
       *   procedure strinit(VAR str : string);
       *
       * ON INPUT
       *   TOS(0) = address of the newly string variable to be initialized
       *   TOS(1) = size of the string memory allocation
       * ON RETURN
       */

    case lbSTRINIT :

      /* Get input parameters */

      POP(st, addr1);  /* Address of dest string variable */
      POP(st, size);   /* Size of string memory allocation */

      /* And perform the variable initialization */

      size      = INT_ALIGNUP(size);
      errorCode = libexec_StrInit(st, addr1, size);

      /* And save the allocated size in the variable's memory */

      PUTSTACK(st, size, addr1 + sSTRING_ALLOC_OFFSET);
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
       *   TOS(0) = Size of the allocated string buffer
       *   TOS(1) = Pointer to the string buffer
       *   TOS(2) = String size (zero)
       */

    case lbSTRTMP :

      /* Allocate a string buffer from the heap for the new temporary string. */

      addr1 = libexec_AllocTmpString(st, STRING_BUFFER_SIZE, &uparm1);
      if (addr1 == 0)
        {
          errorCode = eNOMEMORY;
        }
      else
        {
          /* Create the new string.  Order:
           *
           *   TOS(n)     = 16-bit pointer to the string data.
           *   TOS(n + 1) = String size
           */

          PUSH(st, 0);       /* String size */
          PUSH(st, addr1);   /* String buffer address */
          PUSH(st, uparm1);  /* String buffer address */
        }
      break;

      /* Replace a string with a duplicate string residing in
       * allocated string stack.
       *
       *   function strdup(name : string) : string;
       *
       * ON INPUT
       *   TOS(0) = Allocation size of original string
       *   TOS(1) = Pointer to original string
       *   TOS(2) = Length of original string
       * ON RETURN
       *   TOS(0) = Allocation size of new string (set to default string size)
       *   TOS(1) = Pointer to new string
       *   TOS(2) = Length of new string
       */

    case lbSTRDUP :
      {
        uint16_t *strPtr;
        uint16_t strAlloc;
        uint16_t strAddr;
        uint16_t strSize;

        /* Get the parameters from the stack (leaving the string reference
         * in place).
         */

        strAlloc  = TOS(st, 0);  /* Original string allocated buffer size */
        strAddr   = TOS(st, 1);  /* Original string data pointer */
        strSize   = TOS(st, 2);  /* Original string size */

        strPtr    = (uint16_t *)&TOS(st, 2);
        errorCode = libexec_StrDup(st, strAddr, strSize, strPtr);

        /* A string was consumed so we may have to free the temporary
         * heap memory allocation for its string buffer.
         */

        libexec_FreeTmpString(st, strAddr, strAlloc);
      }
      break;

      /* Replace a character with a string residing in allocated string stack.
       *
       *   function mkstkc(c : char) : string;
       *
       * ON INPUT
       *   TOS(0) = Character value
       * ON RETURN
       *   TOS(0) = Size of the new string buffer (default)
       *   TOS(1) = Address of the new string buffer
       *   TOS(2) = Length of new string (1)
       */

    case lbMKSTKC :
      {
        uint16_t strAlloc;
        uint16_t strAddr;

        /* Allocate string memory for the temporary string from the stack */

        strAddr = libexec_AllocTmpString(st, STRING_BUFFER_SIZE, &strAlloc);
        if (strAddr == 0)
          {
            errorCode = eNOMEMORY;
          }
        else
          {
            /* Copy the character into the string stack */

            dest = (char *)ATSTACK(st, strAddr);  /* Pointer to new string */
           *dest = TOS(st, 0);                    /* Save character as string */

            /* Update the stack content */

            TOS(st, 0) = 1;                       /* String length */
            PUSH(st, strAddr);                    /* String buffer address */
            PUSH(st, strAlloc);                   /* String buffer allocation */
          }
      }
      break;

      /* Concatenate a string to the end of a string.
       *
       *   function strcat(string1 : string, string2 : string) : string;
       *
       * ON INPUT
       *   TOS(0) = string1 allocation size
       *   TOS(1) = pointer to source string1 data
       *   TOS(2) = length of source string1
       *   TOS(3) = string2 allocation size
       *   TOS(4) = pointer to dest string2 data
       *   TOS(5) = length of dest string2
       * ON RETURN:
       *   TOS(0) = string2 allocation size (unchanged)
       *   TOS(1) = pointer to dest string2 (unchanged)
       *   TOS(2) = new length of dest string2
       */

    case lbSTRCAT :
      {
        uint16_t srcAlloc;
        uint16_t srcAddr;
        uint16_t srcSize;
        uint16_t destAlloc;

        /* Get the parameters from the stack (leaving the dest string info
         * in place).
         */

        POP(st, srcAlloc);  /* Source string alloation size */
        POP(st, srcAddr);   /* Source string stack address */
        POP(st, srcSize);   /* Source string size */

        /* Concatenate the strings */

        destAlloc = TOS(st, 0) & HEAP_SIZE_MASK;
        errorCode = libexec_StrCat(st, srcAddr, srcSize, TOS(st, 1),
                                   &TOS(st, 2), destAlloc);

        /* A string was consumed so we may have to free the temporary
         * heap memory allocation for its string buffer.
         */

        libexec_FreeTmpString(st, srcAddr, srcAlloc);
      }
      break;

      /* Concatenate a character to the end of a string.
       *
       *   function strcatc(name : string, c : char) : string;
       *
       * ON INPUT
       *   TOS(0) = Character to concatenate
       *   TOS(1) = String buffer allocation size
       *   TOS(2) = Pointer to string buffer
       *   TOS(3) = Length of string
       * ON RETURN:
       *   TOS(0) = String buffer allocation size (unchanged)
       *   TOS(1) = Pointer to string buffer (unchanged)
       *   TOS(2) = new length of string
       */

    case lbSTRCATC :
      {
        uint16_t ch;

        /* Get the parameters from the stack (leaving the string reference
         * in place).
         */

        POP(st, ch);    /* Character to concatenate */

        errorCode = libexec_StrCatC(st, ch, TOS(st, 1), &TOS(st, 2),
                                    TOS(st, 0));
      }
      break;

      /* Compare two pascal strings
       *
       *   function strcmp(name1 : string, name2 : string) : integer;
       *
       * ON INPUT
       *   TOS(0) = Size of string2 allocation
       *   TOS(1) = Address of string2 data
       *   TOS(2) = Length of string2
       *   TOS(3) = Size of string1 allocation
       *   TOS(4) = Address of string1 data
       *   TOS(5) = Length of string1
       * ON RETURN:
       *   TOS(0) = (-1=less than, 0=equal, 1=greater than}
       */

    case lbSTRCMP :
      {
        const char *str1Ptr;
        const char *str2Ptr;
        uint16_t str1Alloc;
        uint16_t str1Addr;
        uint16_t str1Size;
        uint16_t str2Alloc;
        uint16_t str2Addr;
        uint16_t str2Size;
        int result;

        /* Get the parameters from the stack (leaving space for the
         * return value);
         */

        POP(st, str2Alloc);    /* Discard allocation size of string2 buffer */
        POP(st, str2Addr);     /* Address of string2 data */
        POP(st, str2Size);     /* Length of string2 */

        POP(st, str1Alloc);    /* Discard allocation size of string1 buffer */
        POP(st, str1Addr);     /* Address of string1 data */
        str1Size = TOS(st, 0); /* Length of string1 */

        /* Get full address */

        str1Ptr = (const char *)ATSTACK(st, str1Addr);
        str2Ptr = (const char *)ATSTACK(st, str2Addr);

        /* If string1 is not the same length as string2, then we can only
         * return -1 (string1 less than string2) or +1 (string1 greater
         * than string2).  If the strings are the same length, then we
         * can also return 0 (string1 equal string2).
         */

        if (str1Size < str2Size)
          {
            result = memcmp(str1Ptr, str2Ptr, str1Size);
            if (result == 0) result = -1;
          }

        /* If name1 is longer than name2, then we can only return
         * -1 (less than) or +1 greater than.  If the substrings
         * of length of name2 are equal, then we return greater than.
         */

        else if (str1Size > str2Size)
          {
            result = memcmp(str1Ptr, str2Ptr, str2Size);
            if (result == 0) result = 1;
          }

        /* The strings are of equal length. Return the result of
         * the comparison.
         */

        else
          {
            result = memcmp(str1Ptr, str2Ptr, str1Size);
          }

        /* We consumed two temporary strings and probably need to free
         * temporary string heap allocations.
         */

        libexec_FreeTmpString(st, str1Addr, str1Alloc);
        libexec_FreeTmpString(st, str2Addr, str2Alloc);

        /* Return the result of the comparison */

        TOS(st, 0) = result;
      }
      break;

      /* Return the length a the string (and free any temporary heap allocations)
       *
       *   procedure strlen(str : string)
       *
       * ON INPUT
       *   TOS(0) = Size of string allocation
       *   TOS(1) = Address of string data
       *   TOS(2) = Length of string
       * ON RETURN:
       *   TOS(0) = Length of the string (same as TOS(2) on input)
       */

    case lbSTRLEN :
      {
        uint16_t strAlloc;
        uint16_t strAddr;

        /* Get the strink buffer parameters from the stack (leaving space
         * for the length as the return value);
         */

        POP(st, strAlloc);    /* Discard allocation size of string1 buffer */
        POP(st, strAddr);     /* Address of string1 data */

        /* We consumed the temporary strings and probably need to free the
         * temporary string heap allocations.
         */

        libexec_FreeTmpString(st, strAddr, strAlloc);
      }
      break;

      /* Copy a substring from a string.
       *
       *   Copy(from : string, from, howmuch: integer) : string
       *

       * ON INPUT
       *   TOS(0) = Integer value that provides the length of the substring
       *   TOS(1) = Integer value that provides the (1-based) string position
       *   TOS(2) = Size of string buffer
       *   TOS(3) = Address of string data
       *   TOS(4) = Length of the string
       * ON RETURN:
       *   TOS(0) = Size of string buffer
       *   TOS(1) = Address of the substring data
       *   TOS(2) = Length of the substring
       */

    case lbCOPYSUBSTR :
      {
        const char *strPtr;
        char       *subStrPtr;
        uint16_t    subStrSize;
        uint16_t    subStrPos;
        uint16_t    srcAlloc;
        uint16_t    srcAddr;
        uint16_t    srcSize;
        uint16_t    subStrAlloc;
        uint16_t    subStrAddr;

        /* Get the parameters from the stack, leaving space for the
         * return substring.
         */

        POP(st, subStrSize);
        POP(st, subStrPos);

        srcAlloc = TOS(st, 0);
        srcAddr  = TOS(st, 1);
        srcSize  = TOS(st, 2);

        /* Allocate string memory for the temporary substring from the heap */

        subStrAddr = libexec_AllocTmpString(st, srcAlloc & HEAP_SIZE_MASK,
                                            &subStrAlloc);
        if (subStrAddr == 0)
          {
            errorCode = eNOMEMORY;
          }
        else
          {
            TOS(st, 0) = subStrAlloc;
            TOS(st, 1) = subStrAddr;
            TOS(st, 2) = 0;

            /* Limit the indices to fit within the string */

            if (subStrPos >= 1 && subStrPos <= srcSize && subStrSize > 0)
              {
                /* Make the character position a zero-based index */

                subStrPos--;

                /* Limit the substring size if necesssary */

                if (subStrSize > srcAlloc)
                  {
                    subStrSize = srcAlloc;
                  }

                if (subStrPos + subStrSize > srcSize)
                  {
                    subStrSize = srcSize - subStrPos;
                  }

                /* And copy the substring */

                strPtr    = (char *)ATSTACK(st, srcAddr);
                subStrPtr = (char *)ATSTACK(st, subStrAddr);
                memcpy(subStrPtr, &strPtr[subStrPos], subStrSize);

                TOS(st, 2) = subStrSize;
              }
          }
      }
      break;

      /* Find a substring in a string.  Returns the (1-based) character position of
       * the substring or zero if the substring is not found.
       *
       *   Pos(substr, s : string, start : integer) : integer
       *

       * ON INPUT
       *   TOS(0) = Start position
       *   TOS(1) = Size of string buffer
       *   TOS(2) = Address of string buffer
       *   TOS(3) = Length of the string
       *   TOS(4) = Size of substring buffer
       *   TOS(5) = Address of substring data
       *   TOS(6) = Length of the substring
       * ON RETURN:
       *   TOS(0) = Position of the substring (or zero if not present)
       */

    case lbFINDSUBSTR :
      {
        uint16_t saveCsp;
        uint16_t pos;
        char *strStack;
        char *cStr;
        char *subStrStack;
        char *cSubStr;
        char *result;
        uint16_t strAlloc;
        uint16_t strAddr;
        uint16_t strSize;
        uint16_t subStrAlloc;
        uint16_t subStrAddr;
        uint16_t subStrSize;

        POP(st, pos);

        POP(st, strAlloc);
        POP(st, strAddr);
        POP(st, strSize);

        POP(st, subStrAlloc);
        POP(st, subStrAddr);
        POP(st, subStrSize);

        /* Convert strings to C strings */

        saveCsp     = st->csp;
        strStack    = (char *)ATSTACK(st, strAddr);
        cStr        = libexec_MkCString(st, strStack, strSize, true);

        subStrStack = (char *)ATSTACK(st, subStrAddr);
        cSubStr     = libexec_MkCString(st, subStrStack, subStrSize, false);
        offset      = 0;

        if (pos < 1)
          {
            errorCode = eVALUERANGE;
          }
        else if (cStr == NULL || cSubStr == NULL)
          {
            errorCode = eNOMEMORY;
          }
        else
          {
            /* Find the substring in the string */

            result = strstr(&cStr[pos - 1], cSubStr);

            /* strstr will return NULL if the stubstring is not found but,
             * oddly, will return result == str if substr is 0.
             */

            if (result != NULL)
              {
                offset = (uintptr_t)result - (uintptr_t)cStr + 1;
              }
          }

        st->csp = saveCsp;
        PUSH(st, offset);

        /* We consumed two temporary strings and probably need to free
         * temporary string heap allocations.
         */

        libexec_FreeTmpString(st, strAddr, strAlloc);
        libexec_FreeTmpString(st, subStrAddr, subStrAlloc);
      }
      break;

      /* Insert a string into another string.
       *
       *   Insert(source : string, VAR target : string, index: integer) : string
       *
       * ON INPUT
       *   TOS(0) = Integer value that provides the (1-based) string position
       *   TOS(1) = Address of the target string to be modified
       *   TOS(2) = Size of source string buffer
       *   TOS(3) = Address of source string buffer
       *   TOS(4) = Length of the source string
       * ON RETURN:
       */

    case lbINSERTSTR :
      {
        uint16_t   *destStrPtr;
        char       *destPtr;
        const char *srcPtr;
        uint16_t    newSize;
        uint16_t    copyEnd;
        uint16_t    strPos;
        uint16_t    destStack;
        uint16_t    destAlloc;
        uint16_t    destAddr;
        uint16_t    destSize;
        uint16_t    srcAlloc;
        uint16_t    srcAddr;
        uint16_t    srcSize;
        int         i;
        int         j;

        POP(st, strPos);    /* One-based position of first character to delete */
        POP(st, destStack); /* Stack address of target string variable */

        /* Get the string to be modified */

        destStrPtr = (uint16_t *)ATSTACK(st, destStack);
        destAlloc  = destStrPtr[BTOISTACK(sSTRING_ALLOC_OFFSET)] & HEAP_SIZE_MASK;
        destAddr   = destStrPtr[BTOISTACK(sSTRING_DATA_OFFSET)];
        destSize   = destStrPtr[BTOISTACK(sSTRING_SIZE_OFFSET)];

        /* Get the source string to be inserted */

        POP(st, srcAlloc); /* Size of the source string buffer allocation */
        POP(st, srcAddr);  /* Get the address of the source string buffer */
        POP(st, srcSize);  /* and the length of the source string */

        /* Make the character position a zero-based index */

        if (--strPos < 0)
          {
            strPos = 0;
          }

        /* Open up a space for the string by moving text at the end of the
         * string.
         */

        destPtr = (char *)ATSTACK(st, destAddr);

        newSize = destSize + srcSize;
        if (newSize > destAlloc)
          {
            newSize = destAlloc;
          }

        for (i = newSize - srcSize - 1, j = newSize - 1; i >= strPos; i--, j--)
          {
            destPtr[j] = destPtr[i];
          }

        /* Copy the source string into this space. */

        srcPtr = (const char *)ATSTACK(st, srcAddr);

        copyEnd = srcSize + strPos;
        if (copyEnd > newSize)
          {
            copyEnd = newSize;
          }

        for (i = 0, j = strPos; j < copyEnd; i++, j++)
          {
            destPtr[j] = srcPtr[i];
          }

        /* Adjust the size of string */

        destStrPtr[BTOISTACK(sSTRING_SIZE_OFFSET)] = newSize;

        /* We consumed a temporary string and probably need to free
         * the temporary string's heap allocations\.
         */

        libexec_FreeTmpString(st, srcAddr, srcAlloc);
      }
      break;

      /* Delete a substring from a string.
       *
       *   Delete(VAR from : string, from, howmuch: integer) : string
       *
       * ON INPUT
       *   TOS(0) = Integer value that provides the length of the substring
       *   TOS(1) = Integer value that provides the (1-based) string position
       *   TOS(2) = Address of string variable to be modified
       * ON RETURN:
       */

    case lbDELSUBSTR :
      {
        uint16_t *strPtr;
        uint16_t  numChars;
        uint16_t  strPos;
        uint16_t  strStack;
        uint16_t  strAddr;
        uint16_t  strSize;
        int i;
        int j;

        POP(st, numChars);  /* Number of characters to delete */
        POP(st, strPos);    /* One-based position of first character to delete */
        POP(st, strStack);  /* Address of the string to be modified */

        /* Get the string to be modified */

        strPtr   = (ustack_t *)ATSTACK(st, strStack);
        strAddr  = strPtr[BTOISTACK(sSTRING_DATA_OFFSET)];
        strSize  = strPtr[BTOISTACK(sSTRING_SIZE_OFFSET)];

        /* Make the character position a zero-based index */

        if (--strPos < 0)
          {
            strPos = 0;
          }

        /* Move text at the end of the string to fill the gap. */

        dest = (char *)ATSTACK(st, strAddr);

        for (i = strPos, j = strPos + numChars; j < strSize; i++, j++)
          {
            dest[i] = dest[j];
          }

        /* Adjust the size of string */

        if (strPos + numChars > strSize)
          {
            numChars = strSize - strPos;
          }

        strPtr[BTOISTACK(sSTRING_SIZE_OFFSET)] -= numChars;
      }
      break;

      /* Fill string s with character value until s is count-1 char long.
       *
       *   fillchar(s : string; count : integer; value : shortword)
       *
       * ON INPUT
       *   TOS(0) = Integer 'value' value
       *   TOS(1) = Integer 'count' value
       *   TOS(2) = Address of string (or string) variable
       * ON RETURN:
       */

    case lbFILLCHAR :
      {
        uint16_t *strPtr;
        uint16_t  fillChar;
        uint16_t  fillCount;
        uint16_t  strAddr;

        POP(st, fillChar);   /* Fill character value */
        POP(st, fillCount);  /* Fill count value */
        POP(st, strAddr);    /* Address of the string to be filled */

        /* Get the string to be modified and its allocation size. */

        strPtr = (uint16_t *)ATSTACK(st, strAddr);

        /* Then let common logic do the actual fill */

        errorCode = libexec_FillChar(st, strPtr, fillCount, fillChar);
      }
      break;

      /* Extract a character from a string.
       *
       *   function CharAt(inString : string; charPos : integer) : char
       *
       * ON INPUT
       *   TOS(0) = Integer 'charPos' value
       *   TOS(1) = Size of the string allocations
       *   TOS(2) = Address of the allocated string buffer
       *   TOS(3) = Current size of the string
       * ON RETURN:
       *   TOS(0) = Character from 'inString' at 'charPos'
       */

    case lbCHARAT :
      {
        uint16_t strPos;
        uint16_t strAlloc;
        uint16_t strAddr;
        uint16_t strSize;
        uint16_t result = 0;

        /* Get the parameters off of the stack */

        POP(st, strPos);
        POP(st, strAlloc);
        POP(st, strAddr);
        POP(st, strSize);

        /* Verify that the position is within range */

        if (strPos > 0 && strPos <= strSize)
          {
            const char *sptr = (const char *)ATSTACK(st, strAddr);
            result = (uint16_t)sptr[strPos - 1];
          }

        PUSH(st, result);

        /* We consumed a temporary string and probably need to free
         * the temporary string's heap allocations\.
         */

        libexec_FreeTmpString(st, strAddr, strAlloc);
      }
      break;

      /* Convert a numeric value to a string
       *
       * ON INPUT
       *   TOS(0)   = Address of the string
       *   TOS(1)   = Field width
       *   TOS(2-n) = Numeric value.  The actual length varies with type.
       * ON RETURN:
       */

    case lbINTSTR :
    case lbWORDSTR :
      {
        const char *fmt;
        const char *fmtCh;
        char       *destPtr;
        uint16_t   *strPtr;
        uint16_t    strStack;
        uint16_t    fieldWidth;
        uint16_t    value;
        uint16_t    strAddr;
        uint16_t    strAlloc;
        uint16_t    strSize;

        POP(st, strStack);   /* Stack address of string */
        POP(st, fieldWidth); /* Field width data */
        POP(st, value);      /* Numeric value of the integer */

        /* Get the physical address of the string to be modified and the
         * allocation size.
         */

        strPtr   = (ustack_t *)ATSTACK(st, strStack);
        strAlloc = strPtr[BTOISTACK(sSTRING_ALLOC_OFFSET)];

        /* Get the appropriate format string */

        if (subfunc == lbINTSTR)
          {
            fmtCh = "d";
          }
        else /* if (subfunc == lbWORDSTR) */
          {
            fmtCh = "u";
          }

        fmt = libexec_GetFormat(fmtCh, fieldWidth >> 8, 0);

        /* Now we can perform the conversion */

        strAddr  = strPtr[BTOISTACK(sSTRING_DATA_OFFSET)];
        strSize  = strPtr[BTOISTACK(sSTRING_SIZE_OFFSET)];

        if (strSize < strAlloc)
          {
            /* Convert the string to a numeric value */

            destPtr  = (char *)ATSTACK(st, strAddr) + strSize;
            strSize += snprintf(destPtr, strAlloc - strSize, fmt, value);

            /* Save the updated size of the string */

            strPtr[BTOISTACK(sSTRING_SIZE_OFFSET)] = strSize;
          }
      }
      break;

    case lbLONGSTR :
    case lbULONGSTR :
      {
        const char *fmt;
        const char *fmtCh;
        uint16_t   *stPtr;
        uint16_t    value;
        uint16_t    fieldWidth;
        uint16_t    strAddr;
        uint16_t    strAlloc;
        uint16_t    strSize;

        POP(st, addr1);           /* Stack address of string */
        POP(st, fieldWidth);      /* Field width data */
        value = libexec_UPop32(st); /* Numeric value of the long integer */

        /* Get the physical address of the string to be modified and the
         * allocation size.
         */

        stPtr     = (uint16_t *)ATSTACK(st, addr1);
        strAlloc  = stPtr[BTOISTACK(sSTRING_ALLOC_OFFSET)] & HEAP_SIZE_MASK;

        /* Get the appropriate format string */

        if (subfunc == lbLONGSTR)
          {
            fmtCh = PRId32;
          }
        else /* if (subfunc == lbULONGSTR) */
          {
            fmtCh = PRIu32;
          }

        fmt = libexec_GetFormat(fmtCh, fieldWidth >> 8, 0);

        /* Now we can perform the conversion */

        strAddr = stPtr[BTOISTACK(sSTRING_DATA_OFFSET)];
        strSize = stPtr[BTOISTACK(sSTRING_SIZE_OFFSET)];

        if (strSize < strAlloc)
          {
            /* Convert the string at the end of the string */

            dest     = (char *)ATSTACK(st, strAddr) + strSize;
            strSize += snprintf(dest, strAlloc - strSize, fmt, value);

            /* Save the updated size of the string */

            stPtr[BTOISTACK(sSTRING_SIZE_OFFSET)] = strSize;
          }
      }
      break;

    case lbREALSTR :
      {
        const char *fmt;
        uint16_t   *strPtr;
        fparg_t     value;
        uint16_t    strStack;
        uint16_t    fieldWidth;
        uint16_t    strAddr;
        uint16_t    strAlloc;
        uint16_t    strSize;

        POP(st, strStack);     /* Stack address of string */
        POP(st, fieldWidth);   /* Field width data */
        POP(st, value.hw[3]);  /* Numeric value of the real */
        POP(st, value.hw[2]);
        POP(st, value.hw[1]);
        POP(st, value.hw[0]);

        /* Get the physical address of the string to be modified and the
         * allocation size.
         */

        strPtr   = (uint16_t *)ATSTACK(st, strStack);
        strAlloc = strPtr[BTOISTACK(sSTRING_ALLOC_OFFSET)] & HEAP_SIZE_MASK;

        /* Get the appropriate format string */

        fmt = libexec_GetFormat("f", fieldWidth >> 8, fieldWidth & 0xff);

        /* Now we can perform the conversion */

        strAddr  = strPtr[BTOISTACK(sSTRING_DATA_OFFSET)];
        strSize  = strPtr[BTOISTACK(sSTRING_SIZE_OFFSET)];

        if (strSize < strAlloc)
          {
            /* Convert the string at the end of the string */

            dest     = (char *)ATSTACK(st, strAddr) + strSize;
            strSize += snprintf(dest, strAlloc - strSize, fmt, value.f);

            /* Save the updated size of the string */

            strPtr[BTOISTACK(sSTRING_SIZE_OFFSET)] = strSize;
          }
      }
      break;

      /* Convert a string to a numeric value
       *   procedure val(const s : string; var v; var code : word);
       *
       * Description:
       * val() converts the value represented in the string S to a numerical
       * value, and stores this value in the variable V, which can be of type
       * Integer, Longinteger, ShortInteger, or Real. If the conversion isn't
       * succesful, then the parameter Code contains the index of the character
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
       *   TOS(0) = Address of Code
       *   TOS(1) = Address of valu ,v
       *   TOS(2) = Source string buffer size
       *   TOS(3) = Pointer to source string buffer
       *   TOS(4) = Length of source string
       * ON RETURN: actual parameters released
       */

    case lbVAL :
      {
        char    *strPtr;
        uint16_t codeAddr;
        uint16_t valueAddr;
        uint16_t strAlloc;
        uint16_t strAddr;
        uint16_t strSize;

        POP(st, codeAddr);  /* Address of error code */
        POP(st, valueAddr); /* Address of string value */

        POP(st, strAlloc);  /* String buffer allocation size */
        POP(st, strAddr);   /* Address of string buffer */
        POP(st, strSize);   /* Size of string */

        /* Make a C string out of the pascal string */

        strPtr = (char *)ATSTACK(st, strAddr);
        name   = libexec_MkCString(st, strPtr, strSize, false);

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
                PUTSTACK(st, (ustack_t)*endptr, codeAddr);
                PUTSTACK(st, (ustack_t)longValue, valueAddr);
              }
          }

        /* We consumed a temporary string and probably need to free
         * the temporary string's heap allocations\.
         */

        libexec_FreeTmpString(st, strAddr, strAlloc);
      }
      break;

    default :
      errorCode = eBADSYSLIBCALL;
      break;
    }

  return errorCode;
}

/****************************************************************************
 * Name: libexec_MkCString
 ****************************************************************************/

char *libexec_MkCString(struct libexec_s *st, const char *src, int size,
                        bool keep)
{
  char *dest;

  /* Check if there is free space in the string stack to hold this string */

  if (st->csp + size + 1 >= st->spb)
    {
      return NULL;
    }

  /* Allocate a string buffer on the string stack for the copy. */

  dest = (char *)ATSTACK(st, st->csp);

  /* Make the string persistent if keep is true.  If keep is false, then
   * st->csp is not bumped up so this is a temporary alloc; it will be
   * invalid when the caller returns.
   */

  if (keep)
    {
      st->csp += size + 1;
    }

  /* Copy the originl string, adding C-style NUL termination */

  memcpy(dest, src, size);
  dest[size] = '\0';
  return dest;
}
