/***************************************************************************
 * libexec_longops.h
 * External Declarations associated with the run-time file table
 *
 *   Copyright (C) 2008, 2021 Gregory Nutt. All rights reserved.
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

#ifndef __LIBEXEC_LONGOPS_H
#define __LIBEXEC_LONGOPS_H

/***************************************************************************
 * Included Files
 ***************************************************************************/

#include <stdint.h>
#include "longops.h"
#include "libexec.h"

/***************************************************************************
 * Public Function Prototypes
 ***************************************************************************/

uint32_t libexec_UPop32(struct libexec_s *st);
void libexec_UPush32(struct libexec_s *st, uint32_t value);
uint32_t libexec_UGetTos32(struct libexec_s *st, int offset32);
void libexec_UPutTos32(struct libexec_s *st, uint32_t value, int offset32);

int libexec_LongOperation8(struct libexec_s *st, enum longOp8_e opcode);
int libexec_LongOperation24(struct libexec_s *st,  enum longOp24_e opcode,
                          uint16_t imm16);

#endif /* __LIBEXEC_LONGOPS_H */
