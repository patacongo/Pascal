/****************************************************************************
 * pexec.h
 *
 *   Copyright (C) 2008, 2021 Gregory Nutt. All rights reserved.
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

#ifndef __PEXEC_H
#define __PEXEC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

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
typedef uint16_t paddr_t;    /* Addresses are 16-bits in length */
typedef uint16_t level_t;    /* Limits to MAXUINT16 levels */

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

struct pexec_attr_s
{
  /* Instruction space (I-Space) */

  FAR uint8_t *ispace;   /* Allocated I-Space containing p-code data */
  paddr_t      entry;    /* Entry point */
  paddr_t      maxpc;    /* Last valid p-code address */

  /* Read-only data block */

  FAR uint8_t *rodata;   /* Address of read-only data block */
  paddr_t      rosize;   /* Size of read-only data block */

  /* Allocate for variable storage */

  paddr_t      stralloc; /* Size of string buffer allocation */
  paddr_t      varsize;  /* Variable storage size */
  paddr_t      strsize;  /* String storage size */
};

/* This structure defines the current state of the p-code interpreter */

struct pexec_s
{
  /* This is the emulated P-Machine stack (D-Space) */

  stackType_t dstack;

  /* This is the emulated P-Machine instruction space (I-Space) */

  FAR uint8_t *ispace;

 /* Address of last valid P-Code */

  paddr_t maxpc;

  /* These are the emulated P-Machine registers:
   *
   * spb: Base of the stack
   * sp: The Pascal stack pointer
   * csp: The current top of the stack used to manage string
   *     storage
   * fp: Base Register of the current stack frame.  Holds the address
   *     of the base of the stack frame of the current block.
   * fop: Pointer to section containing read-only data
   * pc: Holds the current p-code location
   */

  paddr_t spb;        /* Pascal stack base */
  paddr_t sp;         /* Pascal stack pointer */
  paddr_t csp;        /* Character stack pointer */
  paddr_t fp;         /* Base of the current frame */
  paddr_t rop;        /* Read-only data pointer */
  paddr_t pc;         /* Program counter */

  /* Info needed to perform a simulated reset.  Memory orgainization:
   *
   *  0                : String stack
   *  strsize          : RO-only data
   *  strsize + rosize : "Normal" Pascal stack
   */

  paddr_t stralloc;   /* String buffer allocation size */
  paddr_t strsize;    /* String stack size */
  paddr_t rosize;     /* Read-only stack size */
  paddr_t entry;      /* Entry point */
  paddr_t stacksize;  /* (debug only) */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef __cplusplus
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

FAR struct pexec_s *pexec_Load(const char *filename, paddr_t stralloc,
                               paddr_t varsize, paddr_t strsize);
FAR struct pexec_s *pexec_Initialize(struct pexec_attr_s *attr);
int pexec_Execute(FAR struct pexec_s *st);
void pexec_Reset(struct pexec_s *st);
void pexec_Release(struct pexec_s *st);

#undef EXTERN
#ifdef __cplusplus
}
#endif

#endif /* __PEXEC_H */
