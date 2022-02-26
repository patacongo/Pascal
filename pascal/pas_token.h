/***************************************************************************
 * pas_token.h
 * External Declarations associated with pas_token.c
 *
 *   Copyright (C) 2008-2009, 2021 Gregory Nutt. All rights reserved.
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

#ifndef __PAS_TOKEN_H
#define __PAS_TOKEN_H

/***************************************************************************
 * Included Files
 ***************************************************************************/

#include <stdint.h>
#include <stdbool.h>

/***************************************************************************
 * Public Variables
 ***************************************************************************/

/* String stack access variables */

extern char *g_tokenString;          /* Start of token in string stack */
extern char *g_stringSP;             /* Top of string stack */

/* Level-related data */

extern int   g_levelSymOffset;       /* Index to symbols for this level */
extern int   g_levelConstOffset;     /* Index to constants for this level */

/***************************************************************************
 * Public Function Prototypes
 ***************************************************************************/

void    getToken(void);
void    getLevelToken(void);
char    pas_GetNextCharacter(bool skipWhiteSpace);
int16_t pas_PrimeTokenizer(unsigned long stringStackSize);
int16_t pas_RePrimeTokenizer(void);

#endif /* __PAS_TOKEN_H */
