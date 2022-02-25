/****************************************************************************
 * libexec.h
 *
 *   Copyright (C) 2008, 2021-2022 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are metPU
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

#ifndef __LIBEXEC_H
#define __LIBEXEC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "pas_machine.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define BPERI         2
#define ITOBSTACK(i)  ((i) << 1)
#define BTOISTACK(i)  ((i) >> 1)
#define ROUNDBTOI(i)  (((i) + 1) >> 1)

/* INPUT/OUTPUT file numbers */

#define INPUT_FILE_NUMBER   0
#define OUTPUT_FILE_NUMBER  1

/* Remove the value from the top of the stack */

#define POP(st, dest) \
  do { \
    dest = (st)->dstack.i[BTOISTACK((st)->sp)]; \
    (st)->sp -= BPERI; \
  } while (0)

/* Add the value to top of the stack */

#define PUSH(st, src) \
  do { \
    (st)->sp += BPERI; \
    (st)->dstack.i[BTOISTACK((st)->sp)] = src; \
  } while (0)

/* Return an rvalue for the (word) offset from the top of the stack */

#define TOS(st, off) \
  (st)->dstack.i[BTOISTACK((st)->sp)-(off)]

/* Save the src (word) at the dest (word) stack position */

#define PUTSTACK(st, src, dest)  \
  do { \
    (st)->dstack.i[BTOISTACK(dest)] = src; \
  } while (0)

/* Return an rvalue for the (word) from the absolute stack position */

#define GETSTACK(st, src) \
  (st)->dstack.i[BTOISTACK(src)]

/* Store a byte to an absolute (byte) stack position */

#define PUTBSTACK(st, src, dest) \
  do { \
    (st)->dstack.b[dest] = src; \
  } while (0)

/* Return an rvalue for the absolute (byte) stack position */

#define GETBSTACK(st, src) \
  (st)->dstack.b[src]

/* Return the address for an absolute (byte) stack position. */

#define ATSTACK(st, src) \
  &(st)->dstack.b[src]

/* Discard n words from the top of the stack */

#define DISCARD(st, n) \
  do { \
    (st)->sp -= BPERI*(n); \
  } while (0)

/* Release a C string */

#define free_cstring(a) \
  free(a)

/****************************************************************************
 * Public Type Definitions
 ****************************************************************************/

typedef uint16_t ustack_t;   /* Stack values are 16-bits in length */
typedef int16_t  sstack_t;
typedef uint16_t level_t;    /* Limits to UINT16_MAX levels */

union stack_u
{
  ustack_t *i;
  uint8_t  *b;
};
typedef union stack_u stackType_t;

/* Floating point */

union fparg_u
{
  double   f;
  uint16_t hw[4];
};

typedef union fparg_u fparg_t;

/* This structure describes the parameters needed to initialize the p-code
 * interpreter.
 */

struct libexec_attr_s
{
  /* Instruction space (I-Space) */

  uint8_t  *ispace;   /* Allocated I-Space containing p-code data */
  pasSize_t entry;    /* Entry point */
  pasSize_t maxpc;    /* Last valid p-code address */

  /* Read-only data block */

  uint8_t  *rodata;   /* Address of read-only data block */
  pasSize_t rosize;   /* Size of read-only data block */

  /* Allocate for variable storage */

  pasSize_t strsize;  /* String storage size */
  pasSize_t stksize;  /* Pascal stack size */
  pasSize_t hpsize;   /* Heap storage size */

  /* String allocation configuration */

  pasSize_t stralloc; /* Size of string buffer allocation */
};

/* This structure defines the current state of the p-code interpreter.  It
 * includes the simulated CPU registers and memory map information.
 */

struct libexec_s
{
  /* This is the emulated P-Machine stack (D-Space) */

  stackType_t dstack;

  /* This is the emulated P-Machine instruction space (I-Space) */

  uint8_t  *ispace;

 /* Address of last valid P-Code */

  pasSize_t maxpc;

  /* These are the emulated P-Machine registers:
   *
   * spb: Base of the stack
   * sp:  The Pascal stack pointer
   * csp: The current top of the stack used to manage string
   *      storage
   * hpb: Base of the heap
   * hsp: Heap stack pointer
   * fp:  Base Register of the current stack frame.  Holds the address
   *      of the base of the stack frame of the current block.
   * fop: Pointer to section containing read-only data
   * pc:  Holds the current p-code location
   */

  pasSize_t spb;        /* Pascal stack base */
  pasSize_t sp;         /* Pascal stack pointer */
  pasSize_t csp;        /* Character stack pointer */
  pasSize_t hpb;        /* Base of the heap */
  pasSize_t hsp;        /* Heap stack pointer */
  pasSize_t fp;         /* Base of the current frame */
  pasSize_t rop;        /* Read-only data pointer */
  pasSize_t pc;         /* Program counter */
  pasSize_t lsp;        /* Static nesting level */

  /* Info needed to perform a simulated reset.  Memory organization:
   *
   *  0                                   : String stack
   *  strsize                             : RO-only data
   *  strsize + rosize                    : "Normal" Pascal stack
   *  strsize + rosize + stksize          : Heap stack
   *  strsize + rosize + stksize + hpsize : "Normal" Pascal stack
   */

  pasSize_t strsize;    /* String stack size */
  pasSize_t rosize;     /* Read-only stack size */
  pasSize_t stksize;    /* Pascal stack size */
  pasSize_t hpsize;     /* Heap stack size */
  pasSize_t stacksize;  /* Total memory allocation */

  pasSize_t entry;      /* Entry point */
  pasSize_t stralloc;   /* String buffer allocation size */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

struct libexec_s *libexec_Initialize(struct libexec_attr_s *attr);
int    libexec_Execute(struct libexec_s *st);
void   libexec_Reset(struct libexec_s *st);

#endif /* __LIBEXEC_H */
