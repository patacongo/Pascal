/****************************************************************************
 * psetops.c
 *
 *   Copyright (C) 2021 Gregory Nutt. All rights reserved.
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

#include "psetops.h"

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* Receive two sets, return one */

static int pexec_intersection(const uint16_t *src, uint16_t *dest);
static int pexec_union(const uint16_t *src, uint16_t *dest);
static int pexec_difference(const uint16_t *src, uint16_t *dest);
static int pexec_symmetricdiff(const uint16_t *src, uint16_t *dest);

/* Receive two sets, returns a boolean */

static int pexec_equality(const uint16_t *src1, const uint16_t *src2,
                          uint16_t *result);
static int pexec_nonequality(const uint16_t *src1, const uint16_t *src2,
                             uint16_t *result);
static int pexec_contains(const uint16_t *src1, const uint16_t *src2,
                          uint16_t *result);

/* Receive a set member and one set, returns a boolean */

static int pexec_member(uint16_t member, const uint16_t *src,
                        uint16_t *result);

/* Receive one set and a set member, returns the modified set */

static int pexec_include(uint16_t member, uint16_t *dest);
static int pexec_exclude(uint16_t member, uint16_t *dest);
static int pexec_card(const uint16_t *src, uint16_t *dest);
static int pexec_singleton(uint16_t member, uint16_t *dest);
static int pexec_subrange(uint16_t member1, uint16_t member2, uint16_t *dest);

static uint16_t pexec_BitsInWord(uint16_t word);
static uint16_t pexec_BitsInByte(uint8_t byte);
static uint16_t pexec_BitsInNibble(uint8_t nibble);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint8_t g_bitsInNibble[16] =
{
/* 00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f */
   0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int pexec_intersection(const uint16_t *src, uint16_t *dest)
{
  dest[0] &= src[0];
  dest[1] &= src[1];
  dest[2] &= src[2];
  dest[3] &= src[3];
  return eNOERROR;
}

static int pexec_union(const uint16_t *src, uint16_t *dest)
{
  dest[0] |= src[0];
  dest[1] |= src[1];
  dest[2] |= src[2];
  dest[3] |= src[3];
  return eNOERROR;
}

static int pexec_difference(const uint16_t *src, uint16_t *dest)
{
  dest[0] &= ~src[0];
  dest[1] &= ~src[1];
  dest[2] &= ~src[2];
  dest[3] &= ~src[3];
  return eNOERROR;
}

static int pexec_symmetricdiff(const uint16_t *src, uint16_t *dest)
{
  dest[0] ^= src[0];
  dest[1] ^= src[1];
  dest[2] ^= src[2];
  dest[3] ^= src[3];
  return eNOERROR;
}

static int pexec_equality(const uint16_t *src1, const uint16_t *src2,
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

static int pexec_nonequality(const uint16_t *src1, const uint16_t *src2,
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

static int pexec_contains(const uint16_t *src1, const uint16_t *src2,
                          uint16_t *result)
{
  uint16_t contains = PASCAL_FALSE;

  if ((src1[0] & src2[0]) == src2[0] ||
      (src1[1] & src2[1]) == src2[1] ||
      (src1[2] & src2[2]) == src2[2] ||
      (src1[3] & src2[3]) == src2[3])
    {
      contains = PASCAL_TRUE;
    }

  *result = contains;
  return eNOERROR;
}

static int pexec_member(uint16_t member, const uint16_t *src,
                        uint16_t *result)
{
  uint16_t wordIndex = member >> 4;
  uint16_t bitIndex  = member & 0x0f;

  *result = ((src[wordIndex] & (1 << bitIndex)) != 0) ?
          PASCAL_TRUE : PASCAL_FALSE;
  return eNOERROR;
}

static int pexec_include(uint16_t member, uint16_t *dest)
{
  uint16_t wordIndex = member >> 4;
  uint16_t bitIndex  = member & 0x0f;

  dest[wordIndex] |= (1 << bitIndex);
  return eNOERROR;
}

static int pexec_exclude(uint16_t member, uint16_t *dest)
{
  uint16_t wordIndex = member >> 4;
  uint16_t bitIndex  = member & 0x0f;

  dest[wordIndex] &= ~(1 << bitIndex);
  return eNOERROR;
}

static int pexec_card(const uint16_t *src, uint16_t *dest)
{
  *dest = pexec_BitsInWord(src[0]) + pexec_BitsInWord(src[1]) +
          pexec_BitsInWord(src[2]) + pexec_BitsInWord(src[3]);
  return eNOERROR;
}

static int pexec_singleton(uint16_t member, uint16_t *dest)
{
  int wordIndex;
  int bitIndex;

  /* Initialize the result */

  dest[0] = 0;
  dest[1] = 0;
  dest[2] = 0;
  dest[3] = 0;

  /* Check that subrange values are in order and in range */

  if (member >= BPERI * sSET_SIZE)
    {
      return eVALUERANGE;
    }

  wordIndex = member >> 4;
  bitIndex  = member & 0x0f;

  dest[wordIndex] |= (1 << bitIndex);
  return eNOERROR;
}

static int pexec_subrange(uint16_t member1, uint16_t member2, uint16_t *dest)
{
  uint16_t leadMask;
  uint16_t tailMask;
  uint16_t bitMask;
  int      nBits;
  int      leadBits;
  int      tailBits;
  int      wordIndex;
  int      bitsInWord;

  /* Initialize the result */

  dest[0] = 0;
  dest[1] = 0;
  dest[2] = 0;
  dest[3] = 0;

  /* Check that subrange values are in order and in range */

  if (member2 >= BPERI * sSET_SIZE)
    {
      return eVALUERANGE;
    }

  if (member1 > member2)
    {
      return eVALUERANGE;
    }

  /* Set all bits from member1 through member2.
   * Eg., Given
   *    member1  = 12
   *    member2  = 35
   * Then
   *    nBits    = 35 - 12 + 1        = 24
   *    leadMask = 0xffff << 12       = 0xf000
   *    tailMask = 0xffff >> (15 - 3) = 0x000f
   *    leadBits = 16 - 12            = 4
   *    tailBits = 3 + 1              = 4
   */

  nBits      = member2 - member1 + 1;
  leadMask   = (0xffff << (member1 & 0x0f));
  tailMask   = (0xffff >> ((BITS_IN_INTEGER - 1) - (member2 & 0x0f)));
  leadBits   = BITS_IN_INTEGER - member1;
  tailBits   = (member2 & 0x0f) + 1;

  /* First time through the loop:
   *    wordIndex  =                    = 0
   *    bitMask    =                    = 0xf000
   *    bitsInWord =                    = 4
   *    nBits      =                    = 20
   * Second time through the loop:
   *    wordIndex  =                    = 1
   *    bitMask    =                    = 0xffff
   *    bitsInWord =                    = 16
   *    nBits      =                    = 4
   * Last time through the loop:
   *    wordIndex  =                    = 2
   *    bitMask    =                    = 0x000f
   *    bitsInWord =                    = 4
   *    nBits      =                    = 0
   */

  for (wordIndex = member1 >> 4, bitMask = leadMask, bitsInWord = leadBits;
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

  return eNOERROR;
}

static uint16_t pexec_BitsInWord(uint16_t word)
{
  return pexec_BitsInByte(word >> 8) + pexec_BitsInByte(word & 0xff);
}

static uint16_t pexec_BitsInByte(uint8_t byte)
{
  return pexec_BitsInNibble(byte >> 4) + pexec_BitsInNibble(byte & 0x0f);
}

static uint16_t pexec_BitsInNibble(uint8_t nibble)
{
  return (uint16_t)g_bitsInNibble[nibble & 0x0f];
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: pexec_setops
 *
 * Description:
 *   This function handles operations on SETs.
 *
 ****************************************************************************/

int pexec_setops(struct pexec_s *st, uint16_t subfunc)
{
  const uint16_t *src1;
  const uint16_t *src2;
  uint16_t *dest;
  uint16_t member1;
  uint16_t member2;
  int errorCode = eNOERROR;

  switch (subfunc)
    {
      /* Receive two sets, return one */

      case setINTERSECTION :
        src1 = (const uint16_t *)&TOS(st, 0);
        dest = (uint16_t *)&TOS(st, sSET_SIZE / sINT_SIZE);
        errorCode = pexec_intersection(src1, dest);
        break;

      case setUNION :
        src1 = (const uint16_t *)&TOS(st, 0);
        dest = (uint16_t *)&TOS(st, sSET_SIZE / sINT_SIZE);
        errorCode = pexec_union(src1, dest);
        DISCARD(st, sSET_SIZE / sINT_SIZE);
        break;

      case setDIFFERENCE :
        src1 = (const uint16_t *)&TOS(st, 0);
        dest = (uint16_t *)&TOS(st, sSET_SIZE / sINT_SIZE);
        errorCode = pexec_difference(src1, dest);
        DISCARD(st, sSET_SIZE / sINT_SIZE);
        break;

      case setSYMMETRICDIFF :
        src1 = (const uint16_t *)&TOS(st, 0);
        dest = (uint16_t *)&TOS(st, sSET_SIZE / sINT_SIZE);
        errorCode = pexec_symmetricdiff(src1, dest);
        DISCARD(st, sSET_SIZE / sINT_SIZE);
        break;

      /* Receive two sets, return a boolean */

      case setEQUALITY :
        src1 = (const uint16_t *)&TOS(st, 0);
        src2 = (const uint16_t *)&TOS(st, sSET_SIZE / sINT_SIZE);
        dest = (uint16_t *)&TOS(st, 2 * sSET_SIZE / sINT_SIZE - 1);
        errorCode = pexec_equality(src1, src2, dest);
        DISCARD(st, 2 * sSET_SIZE / sINT_SIZE  - 1);
        break;

      case setNONEQUALITY :
        src1 = (const uint16_t *)&TOS(st, 0);
        src2 = (const uint16_t *)&TOS(st, sSET_SIZE / sINT_SIZE);
        dest = (uint16_t *)&TOS(st, 2 * sSET_SIZE / sINT_SIZE - 1);
        errorCode = pexec_nonequality(src1, src2, dest);
        DISCARD(st, 2 * sSET_SIZE / sINT_SIZE  - 1);
        break;

      case setCONTAINS :
        src1 = (const uint16_t *)&TOS(st, 0);
        src2 = (const uint16_t *)&TOS(st, sSET_SIZE / sINT_SIZE);
        dest = (uint16_t *)&TOS(st, 2 * sSET_SIZE / sINT_SIZE - 1);
        errorCode = pexec_contains(src1, src2, dest);
        DISCARD(st, 2 * sSET_SIZE / sINT_SIZE  - 1);
        break;

      /* Receive a set member and one set, returns a boolean */

      case setMEMBER :
        POP(st, member1);
        src1 = (const uint16_t *)&TOS(st, 0);
        dest = (uint16_t *)&TOS(st, sSET_SIZE / sINT_SIZE - 1);
        errorCode = pexec_member(member1, src1, dest);
        DISCARD(st, sSET_SIZE / sINT_SIZE  - 1);
        break;

      /* Receive one set and a set member1, returns the modified set */

      case setINCLUDE :
        POP(st, member1);
        dest = (uint16_t *)&TOS(st, 0);
        errorCode = pexec_include(member1, dest);
        break;

      case setEXCLUDE :
        POP(st, member1);
        dest = (uint16_t *)&TOS(st, 0);
        errorCode = pexec_exclude(member1, dest);
        break;

      /* Reveives on set, returns the cardinality of the set */

      case setCARD :
        src1 = (const uint16_t *)&TOS(st, 0);
        dest = (uint16_t *)&TOS(st, sSET_SIZE / sINT_SIZE - 1);
        errorCode = pexec_card(src1, dest);
        DISCARD(st, sSET_SIZE / sINT_SIZE  - 1);
        break;

      /* Receives one integer value, returns a set representing the subrange */

      case setSINGLETON :
        POP(st, member1);
        st->sp += sSET_SIZE;
        dest = (uint16_t *)&TOS(st, 0);
        errorCode = pexec_singleton(member1, dest);
        break;

      /* Receives two integer values, returns a set representing the subrange */

      case setSUBRANGE :
        POP(st, member2);
        POP(st, member1);
        st->sp += sSET_SIZE;
        dest = (uint16_t *)&TOS(st, 0);
        errorCode = pexec_subrange(member1, member2, dest);
        break;

      case setINVALID :
      default:
        errorCode = eBADSETOPCODE;
        break;
    }

  return errorCode;
}
