/***************************************************************************
 * file.h
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
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
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

#ifndef __PFILES_H
#define __PFILES_H

/***************************************************************************
 * Included Files
 ***************************************************************************/

#include "stdint.h"
#include "stdbool.h"

/***************************************************************************
 * Pre-processor Definitions
 ***************************************************************************/

/* Maximum number of files that can be opened at run-time */

#define MAX_OPEN_FILES 8

/***************************************************************************
 * Public Types
 ***************************************************************************/

enum openMode_e
{
  eOPEN_READ = 0,
  eOPEN_WRITE,
  eOPEN_APPEND
};

typedef enum openMode_e openMode_t;

/***************************************************************************
 * Public Function Prototypes
 ***************************************************************************/

void     pexec_assignfile(uint16_t fileNumber, bool text, const char *filename);
void     pexec_openfile(uint16_t fileNumber, openMode_t openMode);
void     pexec_closefile(uint16_t fileNumber);
void     pexec_recordsize(uint16_t fileNumber, uint16_t size);
uint16_t pexec_eof(uint16_t fileNumber);

#endif /* __PFILES_H */
