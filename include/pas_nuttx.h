/**********************************************************************
 * pas_nuttx.h
 * Adaptations for building under NuttX
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

#ifndef __PAS_NUTTX_H
#define __PAS_NUTTX_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>

#include "config.h"

#ifdef CONFIG_PASCAL_BUILD_NUTTX

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Relevant NuttX configuration settings:
 *
 *   CONFIG_BUILD_KERNEL
 *     Selected KERNEL build modes; all applications will be built as ELF
 *     modules.
 *   CONFIG_BUILD_LOADABLE
 *     Automatically selected if KERNEL build is selected. This selection
 *     only effects the behavior of the 'make export' target and currently
 *     has no effect unless you wish to build loadable applications in a FLAT
 *     build.
 *
 * Relevant NuttX configuration settings:
 *
 *  CONFIG_PASCAL_TARGET_TOOLS
 *     Determines if the compiler (pascal), popt, plink, or plist are built
 *     on the target machine.  The run-time program, prun is always built
 *     on the target.
 *
 * Each NuttX makefile (NxMakefile) includes apps/Application.mk.  Make
 * variable inputs to Application.mk:
 *
 *   MAINSRC - if defined, then an executable is built
 *   MODULE  - A module is built if MODULE=m (or CONFIG_BUILD_KERNEL=y).
 *             MODULE is normally defined as the tristate configuartion
 *             that selects an application.
 *
 * Note:  That for Pascal, there will never be a case where some tools are
 * modules and others are built-in.  They will either all be modules (if
 * CONFIG_BUILD_KERNEL) or all be builtin (otherwise).
 */

#define USE_BUILTIN    (!CONFIG_BUILD_KERNEL)
#define PRUN_BUILTIN   (!CONFIG_BUILD_KERNEL)
#define PASCAL_BUILTIN (!CONFIG_BUILD_KERNEL && CONFIG_PASCAL_TARGET_TOOLS)
#define POPT_BUILTIN   (!CONFIG_BUILD_KERNEL && CONFIG_PASCAL_TARGET_TOOLS)
#define PLINK_BUILTIN  (!CONFIG_BUILD_KERNEL && CONFIG_PASCAL_TARGET_TOOLS)
#define PLIST_BUILTIN  (!CONFIG_BUILD_KERNEL && CONFIG_PASCAL_TARGET_TOOLS)

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifdef PASCAL_BUILTIN
/* If the compiler is built-in, then this is the protype of its main function */

int pas_main(int argc, FAR char *argv[]);
#endif

#ifdef POPT_BUILTIN
/* If the optimizer is built-in, then this is the protype of its main
 * function
 */

int popt_main(int argc, char *argv[])
#endif

#ifdef PLINK_BUILTIN
/* If the linker is built-in, then this is the protype of its main function */

int plink_main(int argc, FAR char *argv[]);
#endif

#ifdef PLIST_BUILTIN
/* If the lister is built-in, then this is the protype of its main function */

int plist_main(int argc, FAR char *argv[]);
#endif

#ifdef PRUN_BUILTIN
/* If the run-time is built-in, then this is the protype of its main function */

int prun_main(int argc, FAR char *argv[]);
#endif

#endif /* CONFIG_PASCAL_BUILD_NUTTX */
#endif /* __PAS_NUTTX_H */

