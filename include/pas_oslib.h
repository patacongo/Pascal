/***********************************************************************
 * pas_oslib.h
 * Definitions of the arguments of the Pascal run-time library
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

#ifndef __PAS_OSLIB_H
#define __PAS_OSLIB_H

/***********************************************************************/
/* Codes for runtime OS interfaces.  These must be confined to the range
 * 0x00 through 0xff.
 */

/* Exit processing.
 *
 *   procedure hist(exitCode : integer);
 *
 * ON INPUT:
 *   TOS(0) - Exit code
 * ON RETURN:
 *   Does not return
 */

#define osEXIT          (0x0000)

/* Heap allocation:
 *
 *   function new(size : integer) : integer;
 *
 * ON INPUT:
 *   TOS(0) - Size of the heap region to create
 *
 * ON RETURN:
 *   TOS(0) - The allocated heap region
 */

#define osNEW           (0x0001)

/* Dispose of a previous heap allocation:
 *
 *   procedure dispose(VAR alloc : integer);
 *
 * ON INPUT:
 *   TOS(0) - Address of the heap region to dispose of
 *
 * ON RETURN:
 *   No value is returned
 */

#define osDISPOSE       (0x0002)

/* Get an environment string.
 *
 *   function getenv(name : string) : string;
 *
 * ON INPUT:
 *   TOS(0) = Address of variable name string
 *   TOS(1) = Length of variable name string
 * ON RETURN:
 *   TOS(0) = Address of variable value string
 *   TOS(1) = Length of variable value string
 */

#define osGETENV        (0x0003)

#define MAX_OSOP        (0x0004)

#endif /* __PAS_OSLIB_H */
