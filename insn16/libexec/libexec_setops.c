/****************************************************************************
 * libexec_setops.c
 *
 *   Copyright (C) 2021-2022 Gregory Nutt. All rights reserved.
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
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "pas_debug.h"
#include "config.h"
#include "pas_machine.h"
#include "pas_setops.h"
#include "pas_errcodes.h"
#include "pas_error.h"

#include "libexec_setops.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Receive two sets, return one */

static int libexec_intersection(const uint16_t *src, uint16_t *dest);
static int libexec_union(const uint16_t *src, uint16_t *dest);
static int libexec_difference(const uint16_t *src, uint16_t *dest);
static int libexec_symmetricdiff(const uint16_t *src, uint16_t *dest);

/* Receive two sets, returns a boolean */

static int libexec_equality(const uint16_t *src1, const uint16_t *src2,
             uint16_t *result);
static int libexec_nonequality(const uint16_t *src1, const uint16_t *src2,
             uint16_t *result);
static int libexec_contains(const uint16_t *src1, const uint16_t *src2,
                          uint16_t *result);

/* Receive a set member and one set, returns a boolean */

static int libexec_member(int16_t member, const uint16_t *src,
                        uint16_t *result);

/* Receive one set and a set member, returns the modified set */

static int libexec_include(int16_t member, uint16_t *dest);
static int libexec_exclude(int16_t member, uint16_t *dest);
static int libexec_card(const uint16_t *src, uint16_t *dest);
static int libexec_singleton(int16_t minValue, int16_t member,
             uint16_t *dest);
static int libexec_subrange(int16_t minValue, int16_t member1,
             int16_t member2, uint16_t *dest);

static uint16_t libexec_BitsInWord(uint16_t word);
static uint16_t libexec_BitsInByte(uint8_t byte);
static uint16_t libexec_BitsInNibble(uint8_t nibble);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const uint8_t g_bitsInNibble[16] =
{
/* 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f */
   0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int libexec_intersection(const uint16_t *src, uint16_t *dest)
{
  dest[0] &= src[0];
  dest[1] &= src[1];
  dest[2] &= src[2];
  dest[3] &= src[3];
  return eNOERROR;
}

static int libexec_union(const uint16_t *src, uint16_t *dest)
{
  dest[0] |= src[0];
  dest[1] |= src[1];
  dest[2] |= src[2];
  dest[3] |= src[3];
  return eNOERROR;
}

static int libexec_difference(const uint16_t *src, uint16_t *dest)
{
  dest[0] &= ~src[0];
  dest[1] &= ~src[1];
  dest[2] &= ~src[2];
  dest[3] &= ~src[3];
  return eNOERROR;
}

static int libexec_symmetricdiff(const uint16_t *src, uint16_t *dest)
{
  dest[0] ^= src[0];
  dest[1] ^= src[1];
  dest[2] ^= src[2];
  dest[3] ^= src[3];
  return eNOERROR;
}

static int libexec_equality(const uint16_t *src1, const uint16_t *src2,
                            uint16_t *result)
{
  uint16_t same = PASCAL_FALSE;

  if (src1[0] == src2[0] &&
      src1[1] == src2[1] &&
      src1[2] == src2[2] &&
      src1[3] == src2[3])
    {
      same = PASCAL_TRUE;
    }

  *result = same;
  return eNOERROR;
}

static int libexec_nonequality(const uint16_t *src1, const uint16_t *src2,
                               uint16_t *result)
{
  uint16_t different = PASCAL_FALSE;

  if (src1[0] != src2[0] ||
      src1[1] != src2[1] ||
      src1[2] != src2[2] ||
      src1[3] != src2[3])
    {
      different = PASCAL_TRUE;
    }

  *result = different;
  return eNOERROR;
}

static int libexec_contains(const uint16_t *src1, const uint16_t *src2,
                            uint16_t *result)
{
  uint16_t contains = PASCAL_FALSE;

  if ((src1[0] & src2[0]) == src2[0] &&
      (src1[1] & src2[1]) == src2[1] &&
      (src1[2] & src2[2]) == src2[2] &&
      (src1[3] & src2[3]) == src2[3])
    {
      contains = PASCAL_TRUE;
    }

  *result = contains;
  return eNOERROR;
}

static int libexec_member(int16_t member, const uint16_t *src,
                          uint16_t *result)
{
  int errorCode = eNOERROR;

  if (member < 0 || member > 8 * sSET_SIZE)
    {
      errorCode = eVALUERANGE;
    }
  else
    {
      uint16_t wordIndex = member >> 4;
      uint16_t bitIndex  = member & 0x0f;

      *result = ((src[wordIndex] & (1 << bitIndex)) != 0) ?
              PASCAL_TRUE : PASCAL_FALSE;
    }

  return errorCode;
}

static int libexec_include(int16_t member, uint16_t *dest)
{
  uint16_t wordIndex = member >> 4;
  uint16_t bitIndex  = member & 0x0f;

  dest[wordIndex] |= (1 << bitIndex);
  return eNOERROR;
}

static int libexec_exclude(int16_t member, uint16_t *dest)
{
  uint16_t wordIndex = member >> 4;
  uint16_t bitIndex  = member & 0x0f;

  dest[wordIndex] &= ~(1 << bitIndex);
  return eNOERROR;
}

static int libexec_card(const uint16_t *src, uint16_t *dest)
{
  *dest = libexec_BitsInWord(src[0]) + libexec_BitsInWord(src[1]) +
          libexec_BitsInWord(src[2]) + libexec_BitsInWord(src[3]);
  return eNOERROR;
}

static int libexec_singleton(int16_t minValue, int16_t member, uint16_t *dest)
{
  int wordIndex;
  int bitIndex;

  /* Initialize the result */

  dest[0] = 0;
  dest[1] = 0;
  dest[2] = 0;
  dest[3] = 0;

  /* Check that subrange values are in order and in range */

  member -= minValue;
  if (member < 0 || member >= sSET_MAXELEM)
    {
      return eVALUERANGE;
    }

  wordIndex = member >> 4;
  bitIndex  = member & 0x0f;

  dest[wordIndex] |= (1 << bitIndex);
  return eNOERROR;
}

static int libexec_subrange(int16_t minValue, int16_t member1, int16_t member2,
                          uint16_t *dest)
{
  uint16_t leadMask;
  uint16_t tailMask;
  int      wordIndex;

  /* Initialize the result */

  dest[0] = 0;
  dest[1] = 0;
  dest[2] = 0;
  dest[3] = 0;

  /* Check that subrange values are in order and in range */

  member2 -= minValue;
  if (member2 < 0 || member2 >= sSET_MAXELEM)
    {
      return eVALUERANGE;
    }

  member1 -= minValue;
  if (member1 < 0 || member1 > member2)
    {
      return eVALUERANGE;
    }

  /* Set all bits from member1 through member2. */

  leadMask = (0xffff << (member1 & 0x0f));
  tailMask = (0xffff >> ((BITS_IN_INTEGER - 1) - (member2 & 0x0f)));

  /* Special case:  The entire sub-range fits in one word */

  wordIndex = member1 >> 4;
  if (wordIndex == (member2 >> 4))
    {
      dest[wordIndex] = (leadMask & tailMask);
    }

  /* No, the last bit lies in a different word than the first */

  else
    {
      uint16_t bitMask;
      int      nBits;
      int      leadBits;
      int      tailBits;
      int      bitsInWord;

      nBits    = member2 - member1 + 1;
      leadBits = BITS_IN_INTEGER - member1;
      tailBits = (member2 & 0x0f) + 1;

      /* Loop for each word */

      for (bitMask = leadMask, bitsInWord = leadBits;
           nBits > 0;
           wordIndex++)
        {
          dest[wordIndex] = bitMask;
          nBits          -= bitsInWord;

          if (nBits >= BITS_IN_INTEGER)
            {
              bitsInWord = BITS_IN_INTEGER;
              bitMask    = 0xffff;
            }
          else if (nBits > 0)
            {
              nBits   = tailBits;
              bitMask = tailMask;
            }
        }
    }

  return eNOERROR;
}

static uint16_t libexec_BitsInWord(uint16_t word)
{
  return libexec_BitsInByte(word >> 8) + libexec_BitsInByte(word & 0xff);
}

static uint16_t libexec_BitsInByte(uint8_t byte)
{
  return libexec_BitsInNibble(byte >> 4) + libexec_BitsInNibble(byte & 0x0f);
}

static uint16_t libexec_BitsInNibble(uint8_t nibble)
{
  return (uint16_t)g_bitsInNibble[nibble & 0x0f];
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: libexec_SetOperations
 *
 * Description:
 *   This function handles operations on SETs.
 *
 ****************************************************************************/

int libexec_SetOperations(struct libexec_s *st, uint8_t subfunc)
{
  const uint16_t *src1;
  const uint16_t *src2;
  uint16_t *dest;
  int16_t member1;
  int16_t member2;
  int16_t minValue;
  int16_t offset;
  int errorCode = eNOERROR;

  switch (subfunc)
    {
      /* No inputs, generate an empty set */

      case setEMPTY:
        PUSH(st, 0);
        PUSH(st, 0);
        PUSH(st, 0);
        PUSH(st, 0);
        break;

      /* Receive two sets, return one.  On entry:
       *
       * On entry:
       *   TOS[0-(sSET_WORDS - 1)]   = Set2
       *   TOS[X-(2*sSET_WORDS - 1)] = Set1
       * On return:
       *   TOS[0-(sSET_WORDS - 1)]   = Resulting set.
       */

      case setINTERSECTION :
        src1 = (const uint16_t *)&TOS(st, sSET_WORDS - 1);
        dest = (uint16_t *)&TOS(st, 2 * sSET_WORDS - 1);
        errorCode = libexec_intersection(src1, dest);
        DISCARD(st, sSET_WORDS);
        break;

      case setUNION :
        src1 = (const uint16_t *)&TOS(st, sSET_WORDS - 1);
        dest = (uint16_t *)&TOS(st, 2 * sSET_WORDS - 1);
        errorCode = libexec_union(src1, dest);
        DISCARD(st, sSET_WORDS);
        break;

      case setDIFFERENCE :
        src1 = (const uint16_t *)&TOS(st, sSET_WORDS - 1);
        dest = (uint16_t *)&TOS(st, 2 * sSET_WORDS - 1);
        errorCode = libexec_difference(src1, dest);
        DISCARD(st, sSET_WORDS);
        break;

      case setSYMMETRICDIFF :
        src1 = (const uint16_t *)&TOS(st, sSET_WORDS - 1);
        dest = (uint16_t *)&TOS(st, 2 * sSET_WORDS - 1);
        errorCode = libexec_symmetricdiff(src1, dest);
        DISCARD(st, sSET_WORDS);
        break;

      /* Receive two sets, return a boolean.
       *
       * On entry:
       *   TOS[0-(sSET_WORDS - 1)]   = Set2
       *   TOS[X-(2*sSET_WORDS - 1)] = Set1
       * On return:
       *   TOS[0]                    = Boolean result
       */

      case setEQUALITY :
        src1 = (const uint16_t *)&TOS(st, sSET_WORDS - 1);
        src2 = (const uint16_t *)&TOS(st, 2 * sSET_WORDS - 1);
        dest = (uint16_t *)&TOS(st, 2 * sSET_WORDS - 1);
        errorCode = libexec_equality(src1, src2, dest);
        DISCARD(st, 2 * sSET_WORDS  - 1);
        break;

      case setNONEQUALITY :
        src1 = (const uint16_t *)&TOS(st, sSET_WORDS - 1);
        src2 = (const uint16_t *)&TOS(st, 2 * sSET_WORDS - 1);
        dest = (uint16_t *)&TOS(st, 2 * sSET_WORDS - 1);
        errorCode = libexec_nonequality(src1, src2, dest);
        DISCARD(st, 2 * sSET_WORDS  - 1);
        break;

      case setCONTAINS :
        src1 = (const uint16_t *)&TOS(st, sSET_WORDS - 1);
        src2 = (const uint16_t *)&TOS(st, 2 * sSET_WORDS - 1);
        dest = (uint16_t *)&TOS(st, 2 * sSET_WORDS - 1);
        errorCode = libexec_contains(src1, src2, dest);
        DISCARD(st, 2 * sSET_WORDS  - 1);
        break;

      /* Receives a set member, one set, and an offset.  Returns a boolean.
       *
       * On entry:
       *   TOS(0)            = offset value
       *   TOS(1-sSET_WORDS) = set value
       *   TOS(sSET_WORDS+1) = member to test
       * On return:
       *   TOS[0]            = Boolean result
       */

      case setMEMBER :
        offset    = TOS(st, 0);
        src1      = (const uint16_t *)&TOS(st, sSET_WORDS);
        member1   = TOS(st, sSET_WORDS + 1);
        dest      = (uint16_t *)&TOS(st, sSET_WORDS + 1);
        errorCode = libexec_member((int16_t)member1 - (int16_t)offset,
                                   src1, dest);
        DISCARD(st, sSET_WORDS + 1);
        break;

      /* Receive one set and a set member1, returns the modified set.
       *
       * On entry:
       *   TOS(0)                = member to test
       *   TOS(1-sSET_WORDS)     = set value
       * On return:
       *   TOS[0-(sSET_WORDS-1)] = Set result
       */

      case setINCLUDE :
        POP(st, member1);
        dest = (uint16_t *)&TOS(st, sSET_WORDS - 1);
        errorCode = libexec_include(member1, dest);
        break;

      case setEXCLUDE :
        POP(st, member1);
        dest = (uint16_t *)&TOS(st, sSET_WORDS - 1);
        errorCode = libexec_exclude(member1, dest);
        break;

      /* Reveives on set, returns the cardinality of the set.
       *
       * On entry:
       *   TOS(0-(sSET_WORDS-1)) = Set value
       * On return:
       *   TOS[0]                = Cardinality of set
       */

      case setCARD :
        src1 = (const uint16_t *)&TOS(st, sSET_WORDS - 1);
        dest = (uint16_t *)&TOS(st, sSET_WORDS - 1);
        errorCode = libexec_card(src1, dest);
        DISCARD(st, sSET_WORDS  - 1);
        break;

      /* Receives one integer value, returns a set representing the subrange:
       *
       * On entry:
       *   TOS(0)               = minimum value of a member
       *   TOS(2)               = member
       * On return:
       *   TOS(0-(sSET_WORDS-1) = Set result
       */

      case setSINGLETON :
        POP(st, minValue);
        POP(st, member1);
        st->sp += sSET_SIZE;
        dest = (uint16_t *)&TOS(st, sSET_WORDS - 1);
        errorCode = libexec_singleton(minValue, member1, dest);
        break;

      /* Receives two integer values, returns a set representing the subrange:
       *
       * On entry:
       *   TOS(0)               = minimum value of a member
       *   TOS(1)               = member2
       *   TOS(2)               = member1
       * On return:
       *   TOS(0-(sSET_WORDS-1) = Set result
       */

      case setSUBRANGE :
        POP(st, minValue);
        POP(st, member2);
        POP(st, member1);
        st->sp += sSET_SIZE;
        dest = (uint16_t *)&TOS(st, sSET_WORDS - 1);
        errorCode = libexec_subrange(minValue, member1, member2, dest);
        break;

      case setINVALID :
      default:
        errorCode = eBADSETOPCODE;
        break;
    }

  return errorCode;
}
