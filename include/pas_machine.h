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

#define INT_ALIGNUP(n)      (((n) + sINT_SIZE - 1) & ~(sINT_SIZE - 1))
#define INT_ALIGNDOWN(n)    ((n) & ~sINT_SIZE)
#define INT_ISALIGNED(n)    (((n) & (sINT_SIZE - 1)) == 0)

#define sCHAR_SIZE          1
#define sBOOLEAN_SIZE       sINT_SIZE
#define sREAL_SIZE          8
#define sPTR_SIZE           sINT_SIZE
#define sRETURN_SIZE       (3*sPTR_SIZE)

/* Pascal string variables consist of:
 *
 * - A fixed size, large string buffer, and
 * - A small string variable that includes the size of the string and a
 *   pointer to the string buffer.  It must always appear on the stack
 *   in this order.
 *
 *      TOS(n)     = 16-bit pointer to the string data.
 *      TOS(n + 1) = String size
 *
 * The string is usually access via opLDSM or opSTSM instructions.  The
 * relation ship between the ordering is confusing.  It is a push up stack.
 * when the memory is copied onto the stack:
 *
 *    STORAGE LOCATION      -> STACK
 *      Size    (Offset 0)  -> TOS(1)
 *      Pointer (Offset 4)  -> TOS(0)
 *
 * The size is pushed then the pointer is pushed.  The order is retained in
 * memory, but the offset relative to TOS is confusing.
 */

#define sSTRING_SIZE        (sPTR_SIZE + sINT_SIZE)
#define sSTRING_SIZE_OFFSET (0)         /* Byte offset to string size */
#define sSTRING_DATA_OFFSET (sINT_SIZE) /* Byte offset to buffer pointer */
#define STRING_BUFFER_SIZE  (256)       /* Size of string buffer */

/* Range of unsigned character type */

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
