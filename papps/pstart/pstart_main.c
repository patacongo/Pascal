/****************************************************************************
 * pstart_main.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "paslib.h"
#include "execlib.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: main
 *
 * Description:
 *  This is the application entry when so configured.  It will start an
 *  initial Pascal program.
 *
 ****************************************************************************/

int main(int argc, char *argv[], char *envp[])
{
  EXEC_HANDLE_t handle;
  char fileName[FNAME_SIZE + 1];  /* Object file name */
  char *filePath;
#ifdef CONFIG_PASCAL_AUTOMOUNT
  int ret;
#endif

#ifdef CONFIG_PASCAL_HAVEHOME
  /* Change to the home directory */

  chdir(CONFIG_PASCAL_HOMEDIR);
#endif

#ifdef CONFIG_PASCAL_AUTOMOUNT
  /* Mount the filesystem containing the pascal executables */

  ret = nx_mount(CONFIG_PASCAL_MOUNTDEV, CONFIG_PASCAL_EXECDIR,
                 CONFIG_PASCAL_FSTYPE, 0, CONFIG_PASCAL_MOUNT_OPTIONS);
  if (ret < 0)
    {
      fprintf(stderr,
              "ERROR: Failed to mount %s filesystem at %s: %d\n",
              CONFIG_PASCAL_FSTYPE, CONFIG_PASCAL_EXECDIR, errno);
    }
#endif

  /* Load the POFF files specified on the command line.
   * Use .pex or command line extension, if supplied.
   */

  (void)extension(CONFIG_PASCAL_STARTUP_FILENAME, "pex", fileName,
                  FNAME_SIZE + 1, 0);

  filePath = NULL;
  asprintf(&filePath, "%s%s", CONFIG_PASCAL_EXECDIR, fileName);
  if (filePath == NULL)
    {
      fprintf(stderr, "ERROR: Failed to create filePath\n");
      exit(1);
    }

  /* Initialize the P-machine and load the POFF file */

  handle = libexec_Load(filePath,
                        CONFIG_PASCAL_STARTUP_STRSIZE,
                        CONFIG_PASCAL_STARTUP_STKSIZE,
                        CONFIG_PASCAL_STARTUP_HEAPSIZE);
  if (handle == NULL)
    {
      fprintf(stderr, "ERROR: Could not load %s\n", filePath);
      exit(1);
    }

  printf("%s Loaded\n", filePath);
  free(filePath);

  /* And start program execution in the specified mode */

#ifdef CONFIG_PASCAL_STARTUP_DEBUG
  libexec_DebugLoop(handle);
#else
  libexec_RunLoop(handle);
#endif

  /* Clean up resources used by the interpreter */

  libexec_Release(handle);
  return 0;
}
