/***********************************************************************
 * pas_longops.h
 * Logical P-code long (32-bit) operation code definitions
 *
 *   Copyright (C) 2022 Gregory Nutt. All rights reserved.
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

#ifndef __PAS_LONGOPS_H
#define __PAS_LONGOPS_H

/* These definitions represent logical operations as needed by the
 * the compiler.  The specific INSN generation layer must interpret
 * these requests as is appropriate to the supported INSNS.
 */

enum longops_e
{
  /**-------------------------------------------------------------------
   * OPCODES WITH NO ARGUMENTS
   **-------------------------------------------------------------------**/

  /* Arithmetic & logical & and integer conversions (One stack argument) */

  opDNEG, opDABS, opDINC, opDDEC, opDNOT,

  /* Arithmetic & logical (Two stack arguments):
   *
   *   opDMUL, opDDIV, opDMOD, opDSRA - Signed long integers only
   *   opDUMUL, opDUDIV, opDUMOD      - Unsigned long words only
   *
   * Logical opDerations are inherently unsigned but support integers as well.
   */

  opDADD, opDSUB, opDMUL, opDUMUL, opDDIV, opDUDIV, opDMOD, opDUMOD,
  opDSLL, opDSRL, opDSRA, opDOR,   opDXOR, opDAND,

  /* Comparisons (One stack argument) */

  opDEQUZ, opDNEQZ, opDLTZ, opDGTEZ, opDGTZ, opDLTEZ,

  /* Comparisons (Two stack arguments)
   *
   *   opDLT,  opDGTE,  opDGT,  opDLTE  - Comparizons of signed long integers
   *   opDULT, opDUGTE, opDUGT, opDULTE - Comparizons of unsigned long words
   */

  opDEQU, opDNEQ,
  opDLT,  opDGTE,  opDGT,  opDLTE,
  opDULT, opDUGTE, opDUGT, opDULTE,

  /* Load Immediate */

  opDLDI,

  /* Store Immediate */

  opDSTI,

  /* Data stack */

  opDDUP, opDXCHG,

  /**-------------------------------------------------------------------
   ** OPCODES WITH ONE ARGUMENT
   **-------------------------------------------------------------------**/

  /* Program control:  arg = unsigned label (One stack argument) */

  opDJEQUZ, opDJNEQZ,

  /* Program control:  arg = unsigned label (One stack argument) */

  opDJEQU, opDJNEQ, opDJLT, opDJGTE, opDJGT, opDJLTE,

  /* Load:  arg = unsigned base offset */

  opDLD,

  /* Store: arg = unsigned base offset */

  opDST,

  /* Load Indexed: arg = unsigned base offset */

  opDLDX,

  /* Store Indexed: arg16 = unsigned base offset */

  opDSTX,

  /**-------------------------------------------------------------------
   ** OPCODES WITH TWO ARGUMENTS
   **-------------------------------------------------------------------**/

  /* Load:  arg1 = level; arg2 = signed frame offset */

  opDLDS,

  /* Store: arg1 = level; arg2 = signed frame offset */

  opDSTS,

  /* Load Indexed: arg1 = level; arg2 = signed frame offset */

  opDLDSX,

  /* Store Indexed: arg1 = level; arg2 = signed frame offset */

  opDSTSX,

  NUM_LONGOPS
};

#endif /* __PAS_LONGOPS_H */
