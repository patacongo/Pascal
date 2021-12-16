/***********************************************************************
 * pas_machine.h
 * Characteristics of the PCODE machine
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
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 ***********************************************************************/

#ifndef __PAS_MACHINE_H
#define __PAS_MACHINE_H

/***********************************************************************
 * Included Files
 ***********************************************************************/

#include <stdint.h>
#include <stdio.h> /* for FILE */
#include "config.h"

/***********************************************************************
 * Pre-processor Definitions
 ***********************************************************************/

/* Common Sizing Parameters */

#define FNAME_SIZE          40           /* Max size file name */
#define LINE_SIZE           256          /* Max size of input line buffer */

/* Target P-Machine Data Storage Sizes */

#ifdef CONFIG_INSN16
#  define sINT_SIZE         2
#  define MAXINT            32767
#  define MININT           -32768
#  define BITS_IN_INTEGER   16
#  define MAXUINT           0xffff
#  define MINUINT           0
#endif

#ifdef CONFIG_INSN32
#  define sINT_SIZE         4
#  define MAXINT            2147483647
#  define MININT           -2147483648
#  define BITS_IN_INTEGER   32
#  define MAXUINT           0xffffffff
#  define MINUINT           0
#endif

#define sCHAR_SIZE          1
#define sBOOLEAN_SIZE       sINT_SIZE
#define sREAL_SIZE          8
#define sPTR_SIZE           sINT_SIZE
#define sRETURN_SIZE       (3*sPTR_SIZE)

/* Traditional Pascal strings are a block of 256 bytes on the stack arranged
 * as follows.  This organization is only used in the string stack.
 */

#define sSTRING_HDR_SIZE    2
#define sSTRING_SIZE        256                    /* size(2) + string(254) */
#define sSTRING_MAX_SIZE   (sSTRING_SIZE - 2)      /* string storage size(254) */

/* The representation used on the Pascal runtime stack then duplicates the
 * size and includes a reference to the string stack data.  Order on the
 * run-time stack is like (assuming push-down stack):
 *
 *  TOS + n     = String size
 *  TOS + n + 1 = 16-bit pointer to the string data.
 */

#define sRSTRING_SIZE      (sPTR_SIZE + sINT_SIZE) /* ptr + size */

/* And there are also raw, NUL-terminated "C" strings. */

#define sCSTRING_SIZE      (sizeof(void *))        /* absolute C pointer */

#define MAXCHAR             255
#define MINCHAR             0

/***********************************************************************
 * Public Structure/Types
 ***********************************************************************/

/* Representation of one P-Code */

#ifdef CONFIG_INSN16
struct opType_s
{
  uint8_t  op;
  uint8_t  arg1;
  uint16_t arg2;
};
#endif

#ifdef CONFIG_INSN32
struct opType_s
{
  uint8_t  op;
  uint32_t arg;
};
#endif

typedef struct opType_s opType_t;

#endif /* __PAS_MACHINE_H */
