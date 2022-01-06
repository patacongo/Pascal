/***********************************************************************
 * pas_setops.h
 * Set operation codes
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
 ***********************************************************************/

#ifndef __PAS_SETOPS_H
#define __PAS_SETOPS_H

/***********************************************************************
 * Pre-processor Definitions
 ***********************************************************************/

/* Set operations.  Given S1 = [A,B,C]; and S2 = [C,D,E]:
 *
 *   *       Intersection of two sets         S1 *  S2 = [C]
 *   +       Union of two sets:               S1 +  S2 = [A,B,C,D,E]
 *   -       Difference of two sets           S1 -  S2 = [A,B]
 *   ><      Symmetric difference of two sets S1 >< S2 = [A,B,D,E]
 *   =       Checks equality of two sets      S1 =  S2 is FALSE
 *   <>      Checks non-equality of two sets  S1 <> S2 is TRUE
 *   <=      Contains (subset)                S1 <= S2 is FALSE
 *   In      Check for membership in set      [E] In S2 gives TRUE
 *   Include Includes element in set          Include(S1, [D]) gives [A,B,C,D]
 *   Exclude Excludes element from set        Exclude(S2, [D]) gives [C,E]
 *
 * Precedence:
 *   Highest *
 *           +, -, ><
 *   Lowest  =, <>, <=, In
 */

#define setINVALID       (0x00)
#define setINTERSECTION  (0x01)
#define setUNION         (0x02)
#define setDIFFERENCE    (0x03)
#define setSYMMETRICDIFF (0x04)
#define setEQUALITY      (0x05)
#define setNONEQUALITY   (0x06)
#define setCONTAINS      (0x07)
#define setMEMBER        (0x08)
#define setINCLUDE       (0x09)
#define setEXCLUDE       (0x0a)

#define MAX_SETOP        (0x0b) /* Number of set operations */

#endif /* __PAS_SETOPS_H */
