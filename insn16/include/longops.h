/****************************************************************************
 * longop.h
 * P-code operation code definitions for long (32-bit) operations
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
 ****************************************************************************/

#ifndef __LONGOPS_H
#define __LONGOPS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include "insn16.h"

/****************************************************************************
 * Public Types
 ****************************************************************************/

/** OPCODES WITH NO ARGUMENTS ***********************************************/

enum longOp8_e
{
  /* No long integer operation */

  oDNOP = o8,

  /* Arithmetic & logical & and integer conversions (One 32-bit stack argument) */

  oDNEG,
  oDABS,
  oDINC,
  oDDEC,
  oDNOT,

  /* Arithmetic & logical (Two 32-bit stack arguments) */

  oDADD,
  oDSUB,
  oDMUL,
  oDDIV,
  oDMOD,
  oDSLL,
  oDSRL,
  oDSRA,
  oDOR,
  oDAND,

  /* Comparisons (One 32-bit stack argument) */

  oDEQUZ,
  oDNEQZ,
  oDLTZ,
  oDGTEZ,
  oDGTZ,
  oDLTEZ,

  /* Comparisons (Two 32-bit stack arguments) */

  oDEQU,
  oDNEQ,
  oDLT,
  oDGTE,
  oDGT,
  oDLTE,

  /* Data stack */

  oDDUP,    /* (One 32-bit stack argument */
  oDXCHG,   /* (Two 32-bit stack arguments) */
  oCNVD,    /* (One 16-bit stack argument) */
  oDCNV,    /* (One 32-bit stack argument) */

  /* Unsigned arithmetic and comparisons */

  oDUMUL,
  oDUDIV,
  oDUMOD,
  oDULT,
  oDUGTE,
  oDUGT,
  oDULTE,

  /* Additional bitwise binary operator */

  oDXOR
};

/** OPCODES WITH SINGLE BYTE ARGUMENT (arg8) ********************************/
/* NONE */

/** OPCODES WITH SINGLE 16-BIT ARGUMENT (arg16) *****************************/

enum longOp24_e
{
  /* Program control:  arg16 = unsigned label (One 32-bit stack argument) */

  oDJEQUZ = o8 | o16,
  oDJNEQZ,
  oDJLTZ,
  oDJGTEZ,
  oDJGTZ,
  oDJLTEZ,

  /* Program control:  arg16 = unsigned label (One 32-bit stack argument) */

  oDJEQU,
  oDJNEQ,

  oDJLT,
  oDJGTE,
  oDJGT,
  oDJLTE,

  oDJULT,
  oDJUGTE,
  oDJUGT,
  oDJULTE
};

/** OPCODES WITH 24-BITS OF ARGUMENT (arg8 + arg16) *************************/
/* NONE */

#endif /* __LONGOPS_H */
