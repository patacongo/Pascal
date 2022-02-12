/***************************************************************************
 * popt.h
 * External Declarations associated with popt.c
 *
 *   Copyright (C) 2008, 2022 Gregory Nutt. All rights reserved.
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
 ***************************************************************************/

#ifndef __POPT_H
#define __POPT_H

/***************************************************************************
* Included Files
****************************************************************************/

/***************************************************************************
* Public Types
****************************************************************************/

/* This structure represents one opcode plus includes section offset
 * information to handle relocations.  A pointer to this structure must be
 * cast-compatible with opTypeR_t.
 */

struct opTypeR_s
{
  uint8_t  op;      /* Instruction opcode */
  uint8_t  arg1;    /* 8-bit instruction argument */
  uint16_t arg2;    /* 16-bit instruction argument */
  uint32_t offset;  /* Program section offset on input */
};

typedef struct opTypeR_s opTypeR_t;


/***************************************************************************
* Public Function Prototypes
****************************************************************************/

/***************************************************************************
 * Public Data
 ****************************************************************************/

#endif /* __POPT_H */
