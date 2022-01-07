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
  return eNOTYET;
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
  uint16_t same = PASCAL_FALSE;

  if (src1[0] != src2[0] ||
      src1[1] != src2[1] ||
      src1[2] != src2[2] ||
      src1[3] != src2[3])
    {
      same = PASCAL_TRUE;
    }

  *result = same;
  return eNOERROR;
}

static int pexec_contains(const uint16_t *src1, const uint16_t *src2,
                          uint16_t *result)
{
  uint16_t same = PASCAL_FALSE;

  if ((src1[0] & src2[0]) == src2[0] ||
      (src1[1] & src2[1]) == src2[1] ||
      (src1[2] & src2[2]) == src2[2] ||
      (src1[3] & src2[3]) == src2[3])
    {
      same = PASCAL_TRUE;
    }

  *result = same;
  return eNOERROR;
}

static int pexec_member(uint16_t member, const uint16_t *src,
                        uint16_t *result)
{
  return eNOTYET;
}

static int pexec_include(uint16_t src, uint16_t *dest)
{
  return eNOTYET;
}

static int pexec_exclude(uint16_t src, uint16_t *dest)
{
  return eNOTYET;
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
  uint16_t member;
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
        POP(st, member);
        src1 = (const uint16_t *)&TOS(st, 0);
        dest = (uint16_t *)&TOS(st, sSET_SIZE / sINT_SIZE - 1);
        errorCode = pexec_member(member, src1, dest);
        DISCARD(st, sSET_SIZE / sINT_SIZE  - 1);
        break;

      /* Receive one set and a set member, returns the modified set */

      case setINCLUDE :
        POP(st, member);
        dest = (uint16_t *)&TOS(st, 0);
        errorCode = pexec_include(member, dest);
        break;

      case setEXCLUDE :
        POP(st, member);
        dest = (uint16_t *)&TOS(st, 0);
        errorCode = pexec_exclude(member, dest);
        break;

      case setINVALID :
      default:
        errorCode = eBADSETOPCODE;
        break;
    }

  return errorCode;
}
