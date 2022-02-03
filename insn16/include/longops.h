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
 * Pre-processor Definitions
 ****************************************************************************/

/* Opcode Encoding Summary:
 *
 *            NO ARGS    arg8 ONLY      arg16 ONLY     BOTH
 *            00xx xxxx  01xx xxxx      10xx xxxx      11xx xxxx
 * xx00 0000  ---        ---            ---            ---
 * xx00 0001  DNEG       ---            ---            ---
 * xx00 0010  DABS       ---            ---            ---
 * xx00 0011  DINC       ---            ---            ---
 * xx00 0100  DDEC       ---            ---            ---
 * xx00 0101  DNOT       ---            ---            ---
 * xx00 0110  DADD       ---            ---            ---
 * xx00 0111  DSUB       ---            ---            ---
 * xx00 1000  DMUL       ---            ---            ---
 * xx00 1001  DDIV       ---            ---            ---
 * xx00 1010  DMOD       ---            ---            ---
 * xx00 1011  DSLL       ---            ---            ---
 * xx00 1100  DSRL       ---            ---            ---
 * xx00 1101  DSRA       ---            ---            ---
 * xx00 1110  DOR        ---            ---            ---
 * xx00 1111  DAND       ---            ---            ---
 *
 * xx01 0000  DEQUZ      ---            DJEQUZ ilbl    ---
 * xx01 0001  DNEQZ      ---            DJNEQZ ilbl    ---
 * xx01 0010  DLTZ       ---            DJLTZ  ilbl    ---
 * xx01 0011  DGTEZ      ---            DJGTEZ ilbl    ---
 * xx01 0100  DGTZ       ---            DJGTZ  ilbl    ---
 * xx01 0101  DLTEZ      ---            DJLTEZ ilbl    ---
 * xx01 0110  ---        ---            DJMP   ilbl    ---
 * xx01 0111  ---        ---            ---            ---
 * xx01 1000  DEQU       ---            DJEQU  ilbl    ---
 * xx01 1001  DNEQ       ---            DJNEQ  ilbl    ---
 * xx01 1010  DLT        ---            DJLT   ilbl    ---
 * xx01 1011  DGTE       ---            DJGTE  ilbl    ---
 * xx01 1100  DGT        ---            DJGT   ilbl    ---
 * xx01 1101  DLTE       ---            DJLTE  ilbl    ---
 * xx01 1110  ---        ---            ---            ---
 * xx01 1111  ---        ---            ---            ---
 *
 * xx10 0000  DLDI       ---            DLD uoffs      DLDS loff,offs
 * xx10 0001  ---        ---            ---            ---
 * xx10 0010  ---        ---            ---            ---
 * xx10 0011  ---        ---            ---            ---
 * xx10 0100  DSTI       ---            DST uoffs      DSTS loff,offs
 * xx10 0101  ---        ---            ---            ---
 * xx10 0110  ---        ---            ---            ---
 * xx10 0111  ---        ---            ---            ---
 * xx10 1000  DDUP       ---            DLDX uoffs     DLDSX loff,offs
 * xx10 1001  ---        ---            ---            ---
 * xx10 1010  DXCHG      ---            ---            ---
 * xx10 1011  ---        ---            ---            ---
 * xx10 1100  ---        ---            DSTX uoffs     DSTSX loff,offs
 * xx10 1101  ---        ---            ---            ---
 * xx10 1110  ---        ---            ---            ---
 * xx10 1111  ---        ---            ---            ---
 *
 * xx11 0000  ---        ---            ---            ---
 * xx11 0001  ---        ---            ---            ---
 * xx11 0010  ---        ---            ---            ---
 * xx11 0011  ---        ---            ---            ---
 * xx11 0100  ---        ---            ---            ---
 * xx11 0101  ---        ---            ---            ---
 * xx11 0110  ---        ---            ---            ---
 * xx11 0111  DUMUL      ---            ---            ---
 * xx11 1000  DUDIV      ---            ---            ---
 * xx11 1001  DUMOD      ---            ---            ---
 * xx11 1010  DULT       ---            ---            ---
 * xx11 1011  DUGTE      ---            ---            ---
 * xx11 1100  DUGT       ---            ---            ---
 * xx11 1101  DULTE      ---            ---            ---
 * xx11 1110  DXOR       ---            ---            ---
 * xx11 1111  ---        ---            ---            ---
 *
 * KEY:
 *   loff  = 8-bit static nesting level offset (unsigned)
 *   fop   = 8-bit floating point operation
 *   ilbl  = instruction space label
 *   offs  = 16-bit frame offset (signed)
 *   uoffs = 16-bit base offset (unsigned)
 *   *     = Indicates pseudo-operations (these are removed
 *           after final fixup of the object file).
 */

/** OPCODES WITH NO ARGUMENTS ***********************************************/

/* 0x00 -- unassigned */

/* Arithmetic & logical & and integer conversions (One 16-bit stack argument) */

#define oDNEG   (0x01)
#define oDABS   (0x02)
#define oDINC   (0x03)
#define oDDEC   (0x04)
#define oDNOT   (0x05)

/* Arithmetic & logical (Two 16-bit stack arguments) */

#define oDADD   (0x06)
#define oDSUB   (0x07)
#define oDMUL   (0x08)
#define oDDIV   (0x09)
#define oDMOD   (0x0a)
#define oDSLL   (0x0b)
#define oDSRL   (0x0c)
#define oDSRA   (0x0d)
#define oDOR    (0x0e)
#define oDAND   (0x0f)

/* Comparisons (One 16-bit stack argument) */

#define oDEQUZ  (0x10)
#define oDNEQZ  (0x11)
#define oDLTZ   (0x12)
#define oDGTEZ  (0x13)
#define oDGTZ   (0x14)
#define oDLTEZ  (0x15)

/* 0x16-0x17 -- unassigned */

/* Comparisons (Two 16-bit stack arguments) */

#define oDEQU   (0x18)
#define oDNEQ   (0x19)
#define oDLT    (0x1a)
#define oDGTE   (0x1b)
#define oDGT    (0x1c)
#define oDLTE   (0x1d)

/* 0x1e - 0x1f -- unassigned */

/* Load Immediate */

#define oDLDI   (0x20)    /* (One 16-bit stack argument) */

/* 0x21 - 0x23 -- unassigned */

/* Store Immediate */

#define oDSTI   (0x24)    /* (One 16-bit and one 16-bit stack arguments) */

/* 0x25 - 0x27 -- unassigned */

/* Data stack */

#define oDDUP   (0x28)   /* (One 16-bit stack argument */
                         /* 0x29 -- unassigned */
#define oDXCHG  (0x2a)   /* (Two 16-bit stack arguments) */

/* 0x2b - 0x36 -- unassigned */

/* Unsigned arithmetic and comparisons */

#define oDUMUL  (0x37)
#define oDUDIV  (0x38)
#define oDUMOD  (0x39)
#define oDULT   (0x3a)
#define oDUGTE  (0x3b)
#define oDUGT   (0x3c)
#define oDULTE  (0x3d)

/* Additional bitwise binary operator */

#define oXOR    (0x3e)

/* 0x3f -- unassigned */

/** OPCODES WITH SINGLE BYTE ARGUMENT (arg8) ********************************/

/* (o8|0x00)-(o8|0x3f) -- unassigned */

/** OPCODES WITH SINGLE 16-BIT ARGUMENT (arg16) *****************************/

/* (o16|0x00)-(o16|0x0f) -- unassigned */

/* Program control:  arg16 = unsigned label (One 16-bit stack argument) */

#define oDJEQUZ (o16|0x10)
#define oDJNEQZ (o16|0x11)
#define oDJLTZ  (o16|0x12)
#define oDJGTEZ (o16|0x13)
#define oDJGTZ  (o16|0x14)
#define oDJLTEZ (o16|0x15)

/* (o16|0x15)-(o16|0x17) -- unassigned */

/* Program control:  arg16 = unsigned label (One 16-bit stack argument) */

#define oDJEQU  (o16|0x18)
#define oDJNEQ  (o16|0x19)
#define oDJLT   (o16|0x1a)
#define oDJGTE  (o16|0x1b)
#define oDJGT   (o16|0x1c)
#define oDJLTE  (o16|0x1d)

/* (o16|0x1e)-(o16|0x1f) -- unassigned */

/* Load:  arg16 = unsigned base offset */

#define oDLD    (o16|0x20)       /* (no stack arguments) */

/* (o16|0x21)-(o16|0x23) -- unassigned */

/* Store: arg16 = unsigned base offset */

#define oDST    (o16|0x24)       /* (One 16-bit stack argument) */

/* (o16|0x25)-(o16|0x27) -- unassigned */

/* Load Indexed: arg16 = unsigned base offset */

#define oDLDX   (o16|0x28)       /* (One 16-bit stack argument) */

/* (o16|0x29)-(o16|0x2b) -- unassigned */

/* Store Indexed: arg16 = unsigned base offset */

#define oDSTX   (o16|0x2c)       /* (One 16-bit + one 16-bit stack arguments) */

/* (o16|0x2d)-(o16|0x3f) -- unassigned */

/** OPCODES WITH 24-BITS OF ARGUMENT (arg8 + arg16) *************************/

/* (o16|o8|0x00)-(o8|o16|0x1f) -- unassigned */

/* Load:  arg8 = level; arg16 = signed frame offset */

#define oDLDS   (o16|o8|0x20)    /* (no stack arguments) */

/* (o16|o8|0x21)-(o8|o16|0x2f) -- unassigned */

/* Store: arg8 = level; arg16 = signed frame offset */

#define oDSTS   (o16|o8|0x24)    /* (One 16-bit stack argument) */

/* (o16|o8|0x25)-(o8|o16|0x27) -- unassigned */

/* Load Indexed: arg8 = level; arg16 = signed frame offset */

#define oLDSX   (o16|o8|0x28)    /* (One 16-bit stack argument) */

/* (o16|o8|0x29)-(o8|o16|0x2b) -- unassigned */

/* Store Indexed: arg8 = level; arg16 = signed frame offset */

#define oDSTSX  (o16|o8|0x2c)    /* (One 16-bit + one 16-bit stack arguments) */

/* (o16|o8|0x2d)-(o8|o16|0x3f) -- unassigned */

#endif /* __LONGOPS_H */
