/***************************************************************************
 * libexec_heap.h
 * External Declarations associated with the run-time memory manager
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
 ***************************************************************************/

#ifndef __LIBEXEC_HEAP_H
#define __LIBEXEC_HEAP_H

/***************************************************************************
 * Included Files
 ***************************************************************************/

#include <stdint.h>

#include "config.h"
#include "libexec.h"

/***************************************************************************
 * Pre-precessor Definitions
 ***************************************************************************/

/* The top 4 bits of the string allocation are reserved for encoding
 * information.  The character buffer for string variables resides in a
 * string stack, but temporary strings using the heap because it permits
 * "random access" freeing of string buffers.
 */

#define HEAP_SIZE_MASK (uint16_t)(0x0fff)  /* Bits 0-14:  The size */
#define HEAP_STRING    (uint16_t)(1 << 15) /* Bit 15: Temporary string */

/***************************************************************************
 * Public Function Prototypes
 ***************************************************************************/

void     libexec_InitializeHeap (struct libexec_s *st);
int      libexec_New            (struct libexec_s *st, uint16_t size);
int      libexec_Dispose        (struct libexec_s *st, uint16_t address);
uint16_t libexec_AllocTmpString (struct libexec_s *st, uint16_t reqSize,
                                 uint16_t *allocSize);
int      libexec_FreeTmpString  (struct libexec_s *st, uint16_t allocAddr,
                                 uint16_t allocSize);
#endif /* __LIBEXEC_HEAP_H */
