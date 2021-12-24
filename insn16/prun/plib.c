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
static void     pas_strcpy(struct pexec_s *st, uint16_t srcBufferAddr,
                           uint16_t srcStringSize, uint16_t destVarAddr,
                           uint16_t varOffset);
static void     pas_Cstr2str(struct pexec_s *st, uint8_t *srcCString,
                             uint16_t destVarAddr, uint16_t varOffset);

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

static void pas_strcpy(struct pexec_s *st, uint16_t srcBufferAddr,
                       uint16_t srcStringSize, uint16_t destVarAddr,
                       uint16_t varOffset)
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
       * Make sure that the string length will fit into the destination.
       */

      if (srcStringSize >= st->stralloc)
        {
          /* Clip to the maximum size */

          srcStringSize = st->stralloc;
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

  switch (subfunc)
    {
      /* Halt processing */

    case lbHALT:
      return eEXIT;

      /* Get the value of an environment string
       *
       * ON INPUT:
       *   TOS(st, 0) = Number of bytes in environment identifier string
       *   TOS(st, 1) = Address environment identifier string
       * ON RETURN (above replaced with):
       *   TOS(st, 0) = MS 16-bits of 32-bit C string pointer
       *   TOS(st, 1) = LS 16-bits of 32-bit C string pointer
       */

    case lbGETENV :
      size = TOS(st, 0);                          /* Number of bytes in string */
      src = (uint8_t *)&GETSTACK(st, TOS(st, 1)); /* Pointer to string */

      /* Make a C string out of the pascal string */

      name = pexec_mkcstring(src, size);
      if (name == NULL)
        {
          return eNOMEMORY;
        }

      /* Make the C-library call and free the string copy */

      src = (uint8_t *)getenv((char *)name);
      free_cstring(name);

      /* Save the returned pointer in the stack */

      TOS(st, 0) = (ustack_t)((uintptr_t)src >> 16);
      TOS(st, 1) = (ustack_t)((uintptr_t)src & 0x0000ffff);
      break;

      /* Copy pascal string to a pascal string
       *   procedure strcpy(src : string; var dest : string)
       *
       * ON INPUT:
       *   TOS(st, 0) = address of dest string variable
       *   TOS(st, 1) = pointer to source string buffer
       *   TOS(st, 2) = length of source string
       * ON RETURN (input consumed):
       */

    case lbSTRCPY :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Address of dest string variable  */
      POP(st, addr2);  /* Address of source string buffer */
      POP(st, size);   /* Length of valid source data */

      /* And perform the string copy */

      pas_strcpy(st, addr2, size, addr1, 0);
      break;

    /* Copy pascal string to a element of a pascal string array
     *
     *   procedure strcpy(src : string; var dest : string;
     *                    offset : integer)
     *
     * ON INPUT:
     *   TOS(0)=Address of dest string variable
     *   TOS(1)=Pointer to source string buffer
     *   TOS(2)=Length of source string
     *   TOS(3)=Dest string variable address offset
     * ON RETURN: actual parameters released.
     *
     * REVISIT:  This is awkward.  Life would be much easier if the
     * array index could be made to be emitted later in the stack and
     * so could be added to the dest sting variable address easily.
     */

    case lbSTRCPYX :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* Address of dest string variable  */
      POP(st, addr2);  /* Address of source string buffer */
      POP(st, size);   /* Length of valid source data */
      POP(st, offset); /* Offset from dest string address */

      /* And perform the string copy */

      pas_strcpy(st, addr2, size, addr1, offset);
      break;

      /* Copy C string to a pascal string
       *
       * ON INPUT:
       *   TOS(st, 0) = address of dest hdr
       *   TOS(st, 1) = MS 16-bits of 32-bit C string pointer
       *   TOS(st, 2) = LS 16-bits of 32-bit C string pointer
       * ON RETURN (input consumed):
       */

    case lbCSTR2STR :

      /* "Pop" in the input parameters from the stack */

      POP(st, addr1);  /* addr of dest string header */
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
     *   TOS(0)=Address of dest string variable
     *   TOS(1)=Pointer to source string buffer
     *   TOS(2)=Length of source string
     *   TOS(3)=Dest string variable address offset
     * ON RETURN: actual parameters released.
     *
     * REVISIT:  This is awkward.  Life would be much easier if the
     * array index could be made to be emitted later in the stack and
     * so could be added to the dest sting variable address easily.
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
       *   TOS(st, 0)=address of code
       *   TOS(st, 1)=address of value
       *   TOS(st, 2)=length of source string
       *   TOS(st, 3)=pointer to source string
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
          return eNOMEMORY;
        }

      /* Convert the string to an integer */

      value = atoi((char *)name);
      if ((value < MININT) || (value > MAXINT))
        {
          return eINTEGEROVERFLOW;
        }

      PUTSTACK(st, TOS(st, 0), 0);
      PUTSTACK(st, TOS(st, 1), value);
      DISCARD(st, 4);
      break;

      /* Initialize a new string variable. Create a string buffer.
       *   procedure mkstk(VAR str : string);
       *
       * ON INPUT
       *   TOS(st, 1)=pointer to the newly string variable to be initialized
       * ON RETURN
       */

    case lbSTRINIT :
      /* Get input parameters */

      POP(st, addr1);                  /* Address of dest string variable */

      /* Check if there is space on the string stack for the new string
       * buffer.
       */

      if (st->csp + st->stralloc >= st->spb)
        {
          return eSTRSTKOVERFLOW;
        }

      /* Allocate a string buffer on the string stack for the new string. */

      addr2   = ((st->csp + 1) & ~1);
      st->csp = addr2 + st->stralloc;

      /* Initialize the new string.  Order:
       *
       *   TOS(n)     = 16-bit pointer to the string data.
       *   TOS(n + 1) = String size
       */

      PUTSTACK(st, addr2, addr1 + sSTRING_DATA_OFFSET);
      PUTSTACK(st, 0,     addr1 + sSTRING_SIZE_OFFSET);
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
       *   TOS(0)=Pointer to the string buffer on the stack.
       *   TOS(1)=String size (zero)
       */

    case lbSTRTMP :
      /* Check if there is space on the string stack for the new string
       * buffer.
       */

      if (st->csp + st->stralloc >= st->spb)
        {
          return eSTRSTKOVERFLOW;
        }

      /* Allocate a string buffer on the string stack for the new string. */

      addr1   = ((st->csp + 1) & ~1);
      st->csp = addr1 + st->stralloc;

      /* Create the new string.  Order:
       *
       *   TOS(n)     = 16-bit pointer to the string data.
       *   TOS(n + 1) = String size
       */

      PUSH(st, 0);     /* String size */
      PUSH(st, addr1); /* String buffer address */
      break;

      /* Replace a string with a duplicate string residing in allocated
       * string stack.
       *
       *   function strdup(name : string) : string;
       *
       * ON INPUT
       *   TOS(st, 0)=pointer to original string data
       *   TOS(st, 1)=length of original string
       * ON RETURN
       *   TOS(st, 0)=pointer to new string data
       *   TOS(st, 1)=length of new string (unchanged)
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
          return eSTRSTKOVERFLOW;
        }

      /* Allocate space on the string stack for the new string */

      addr2    = ((st->csp + 1) & ~1);
      st->csp += st->stralloc;                    /* Allocate max size */

      /* Copy the string into the string stack */

      src      = (uint8_t *)&GETSTACK(st, addr1); /* Pointer to original string */
      dest     = (uint8_t *)&GETSTACK(st, addr2); /* Pointer to new string */
      memcpy(dest, src, size);

      /* Update the string buffer address */

      TOS(st, 0) = addr2;
      break;

      /* Replace a character with a string residing in allocated string stack.
       *   function mkstkc(c : char) : string;
       * ON INPUT
       *   TOS(st, 0)=Character value
       * ON RETURN
       *   TOS(st, 0)=pointer to new string
       *   TOS(st, 1)=length of new string
       */

    case lbMKSTKC :
      /* Check if there is space on the string stack for the new string */

      if (st->csp + st->stralloc >= st->spb)
        {
          return eSTRSTKOVERFLOW;
        }

      /* Allocate space on the string stack for the new string */

      addr2    = ((st->csp + 1) & ~1);
      st->csp += st->stralloc;                     /* Allocate max size */

      /* Save the length at the beginning of the copy */

      dest     = (uint8_t *)&GETSTACK(st, addr2);  /* Pointer to new string */

      /* Copy the character into the string stack */

      *dest++  = TOS(st, 0);                       /* Save character as string */

      /* Update the stack content */

      TOS(st, 0) = 1;                             /* String length */
      PUSH(st, addr2);                            /* String address */
      break;

      /* Concatenate a string to the end of a string.
       *   function strcat(name : string1, c : char) : string;
       *
       * ON INPUT
       *   TOS(st, 0)=pointer to source string1 data
       *   TOS(st, 1)=length of source string1
       *   TOS(st, 2)=pointer to dest string2 data
       *   TOS(st, 3)=length of dest string2
       * ON OUTPUT
       *   TOS(st, 0)=pointer to dest string2 (unchanged)
       *   TOS(st, 1)=new length of dest string2
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
          return eSTRSTKOVERFLOW;
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

      /* Concatenate a character  to the end of a string.
       *   function strcatc(name : string, c : char) : string;
       *
       * ON INPUT
       *   TOS(st, 0)=character to concatenate
       *   TOS(st, 1)=pointer to string
       *   TOS(st, 2)=length of string
       * ON OUTPUT
       *   TOS(st, 0)=pointer to string
       *   TOS(st, 1)=new length of string
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
          return eSTRSTKOVERFLOW;
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

      /* Compare two pascal strings
       *   function strcmp(name1 : string, name2 : string) : integer;
       * ON INPUT
       *   TOS(st, 2)=address of string2 data
       *   TOS(st, 1)=length of string2
       *   TOS(st, 4)=address of string1 data
       *   TOS(st, 3)=length of string1
       * ON OUTPUT
       *   TOS(st, 0)=(-1=less than, 0=equal, 1=greater than}
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

    default :
      return eBADSYSLIBCALL;
    }

  return eNOERROR;
}
