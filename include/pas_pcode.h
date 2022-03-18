/***********************************************************************
 * pas_pcode.h
 * Logical P-code operation code definitions
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
 ***********************************************************************/

#ifndef __PAS_PCODE_H
#define __PAS_PCODE_H

/* These definitions represent logical operations as needed by the
 * the compiler.  The specific INSN generation layer must interpret
 * these requests as is appropriate to the supported INSNS.
 */

enum pcode_e
{
  /**-------------------------------------------------------------------
   * OPCODES WITH NO ARGUMENTS
   **-------------------------------------------------------------------**/

  /* Program control (No stack arguments) */

  opNOP = 0,

  /* Arithmetic & logical & and integer conversions (One stack argument) */

  opNEG, opABS, opINC, opDEC, opNOT,

  /* Arithmetic & logical (Two stack arguments):
   *
   *   opMUL, opDIV, opMOD, opSRA - Signed integers only
   *   opUMUL, opUDIV, opUMOD     - Unsigned words only
   *
   * Logical operations are inherently unsigned but support integers as well.
   */

  opADD, opSUB, opMUL, opUMUL, opDIV, opUDIV, opMOD, opUMOD,
  opSLL, opSRL, opSRA, opOR,   opXOR, opAND,

  /* Comparisons (One stack argument) */

  opEQUZ, opNEQZ, opLTZ, opGTEZ, opGTZ, opLTEZ,

  /* Comparisons (Two stack arguments)
   *
   *   opLT,  opGTE,  opGT,  opLTE  - Comparizons of signed integers
   *   opULT, opUGTE, opUGT, opULTE - Comparizons of unsigned words
   */

  opEQU, opNEQ,
  opLT,  opGTE,  opGT,  opLTE,
  opULT, opUGTE, opUGT, opULTE,

  /* Load Immediate */

  opLDI, opLDIB, opULDIB, opLDIM,

  /* Store Immediate */

  opSTI, opSTIB, opSTIM,

  /* Data stack */

  opDUP, opXCHG,

  /* Program control (No stack arguments)
   * Behavior:
   *   Pop return address
   *   Pop saved base register (BR)
   *   Discard saved base address
   *   Set program counter (PC) to return address
   */

  opRET,

  /* System Functions (No stack arguments) */

  opEND,

  /**-------------------------------------------------------------------
   ** OPCODES WITH ONE ARGUMENT
   **-------------------------------------------------------------------**/

  /* Floating point/Set operations: arg = FP/SET/OSIF op-code */

  opFLOAT,  opSETOP, opOSOP,

  /* Program control:  arg = unsigned label (One stack argument) */

  opJEQUZ, opJNEQZ,

  /* Program control:  arg = unsigned label (no stack arguments) */

  opJMP,

  /* Program control:  arg = unsigned label (One stack argument) */

  opJEQU, opJNEQ, opJLT, opJGTE, opJGT, opJLTE,

  /* Load:  arg = unsigned base offset */

  opLD, opLDB, opULDB, opLDM,

  /* Store: arg = unsigned base offset */

  opST, opSTB, opSTM,

  /* Load Indexed: arg = unsigned base offset */

  opLDX, opLDXB, opULDXB, opLDXM,

  /* Store Indexed: arg16 = unsigned base offset */

  opSTX, opSTXB, opSTXM,

  /* Load address relative to stack base: arg = unsigned offset */

  opLA,

  /* Load RO data address:        arg = RODATA offset (No stack arguments)
   * Load stack relative address: arg = signed stack offset
   */

  opLAC, opLAR,

  /* Data stack:  arg = 16 bit signed data (no stack arguments) */

  opPUSH, opINDS, opINCS,

  /* Load address relative to stack base: arg1 = unsigned offset, TOS=index */

  opLAX,

  /* System functions: arg = 16-bit library/system call identifier */

  opSTRLIB, opSYSIO,

  /* Program control:  arg = unsigned label (no stack arguments) */

  opLABEL,

  /**-------------------------------------------------------------------
   ** OPCODES WITH TWO ARGUMENTS
   **-------------------------------------------------------------------**/

  /* Program Control:  arg1 = level; arg2 = unsigned label */

  opPCAL,

  /* Load:  arg1 = level; arg2 = signed frame offset */

  opLDS, opLDSB, opULDSB, opLDSM,

  /* Store: arg1 = level; arg2 = signed frame offset */

  opSTS, opSTSB, opSTSM,

  /* Load Indexed: arg1 = level; arg2 = signed frame offset */

  opLDSX, opLDSXB, opULDSXB, opLDSXM,

  /* Store Indexed: arg1 = level; arg2 = signed frame offset */

  opSTSX, opSTSXB, opSTSXM,

  /* LAS/LASX        arg1 = level; arg2 = signed frame offset
   *                          (no stack arguments)
   */

  opLAS, opLASX,

  /* Pseudo-operations:
   * LINE:             arg1 = file number; arg2 = line number
   */

  opLINE,

  NUM_OPCODES
};

#endif /* __PAS_PCODE_H */
