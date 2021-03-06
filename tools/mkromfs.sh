#!/bin/sh
#############################################################################
# mkromfs.sh
#
#   Copyright (C) 2022 Gregory Nutt. All rights reserved.
#   Author: Gregory Nutt <gnutt@nuttx.org>
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in
#    the documentation and/or other materials provided with the
#    distribution.
# 3. Neither the name of the copyright holder nor the names of its
#    contributors may be used to endorse or promote products derived from
#    this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
# COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
# OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
# AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
# ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#
#############################################################################

#set -x

IMAGEDIR=${1}
OUTDIR=${2}

USAGE="USAGE: ${0} <image-directory> <output-directory>"

if [ -z "${IMAGEDIR}" ]; then
  echo "Missing <image-directory>"
  echo ${USAGE}
  exit 1
fi

if [ ! -d "${IMAGEDIR}" ]; then
  echo "Directory ${IMAGEDIR} does not exist"
  echo ${USAGE}
  exit 1
fi

if [ -z "${OUTDIR}" ]; then
  echo "Missing <output-file>"
  echo ${USAGE}
  exit 1
fi

if [ ! -d "${OUTDIR}" ]; then
  echo "Directory ${OUTDIR} does not exist"
  echo ${USAGE}
  exit 1
fi

cd ${OUTDIR} || { echo "Failed to CD to ${OUTDIR}"; exit 1; }

genromfs -h 1>/dev/null 2>&1 || { \
  echo "Host executable genromfs not available in PATH"; \
  echo "You may need to download in from http://romfs.sourceforge.net/"; \
  exit 1; \
}

genromfs -f romfs.img -d ${IMAGEDIR} -V NuttXPcodeVol
xxd -i romfs.img  >romfs.c
sed -i -e 's/^unsigned char/const unsigned char aligned_data(4)/g' romfs.c
sed -i -e 's/aligned_data[(]4[)] //g' romfs.c
