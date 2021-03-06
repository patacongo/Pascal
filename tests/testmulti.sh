#!/bin/sh
############################################################################
# tests/testmulti.sh
#
#   Copyright (C) 2008, 2021-2022 Gregory Nutt. All rights reserved.
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
############################################################################
#set -x

source ../.config

PBINDIR=../bin16

PRUN=${PBINDIR}/prun
STRSTKSZ=1024
HEAPSIZE=256

PASFILENAME=${1}
INPUT=${2}

PASBASENAME=`basename ${PASFILENAME} .pex`
PASDIRNAME=`dirname ${PASFILENAME}`
if [ "${PASDIRNAME}" == "." ]; then
  PASDIRNAME=units
fi

# See if this test requires special options

OPTFILENAME=${PASDIRNAME}/${PASBASENAME}.opt
if [ -f ${OPTFILENAME} ]; then
  LINE=`grep "^T " ${OPTFILENAME}`
  if [ ! -z "${LINE}" ]; then
    STRSTKSZ=`echo ${LINE} | cut -d' ' -f2`
  fi    

  LINE=`grep "^N " ${OPTFILENAME}`
  if [ ! -z "${LINE}" ]; then
    HEAPSIZE=`echo ${LINE} | cut -d' ' -f2`
  fi    
fi

echo "########${PASFILENAME}########";

make -C ${PASDIRNAME} -f PasMakefile ${PASBASENAME}.pex

if [ ! -f ${PASDIRNAME}/${PASBASENAME}.pex ]; then
  echo "Failed to build ${PASBASENAME}.pex"
  if [ -f ${PASDIRNAME}/${PASBASENAME}.err ]; then
    cat ${PASDIRNAME}/${PASBASENAME}.err | grep Line
  fi
fi

PRUNOPTS="-t ${STRSTKSZ} -n ${HEAPSIZE}"
echo "Using options:  ${PRUNOPTS}"

if [ -z "${INPUT}" ]; then
    ${PRUN} ${PRUNOPTS} ${PASDIRNAME}/${PASBASENAME}.pex 2>&1
else
    ${PRUN} ${PRUNOPTS} ${PASDIRNAME}/${PASBASENAME}.pex 2>&1 <${PASDIRNAME}/${INPUT}
fi
