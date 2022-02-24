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

PBINDIR=../../bin16
PASCAL=${PBINDIR}/pascal
POPT=${PBINDIR}/popt
PLINK=${PBINDIR}/plink
PRUN=${PBINDIR}/prun

PASOPTS=
PRUNOPTS="-t 1024 -n 256"

MAIN=${1}
UNITS=${2}
INPUT=${3}

FILES="${MAIN} ${UNITS}"

basefile=`basename ${MAIN} .pas`
PROG=${basefile}.pex

cd units

for file in ${FILES}; do
    echo "########${file}########";

    basefile=`basename ${file} .pas`
    ${PASCAL} ${PASOPTS} ${file} 2>&1 || rm -f ${basefile}.o1
    if [ -f ${basefile}.o1 ] ; then
        ${POPT} ${basefile}.o1 2>&1 || rm -f ${basefile}.o*
    fi

    cat ${basefile}.err | grep Line
    OBJS="${OBJS} ${basefile}.o"
done

echo "########${PROG}########";

${PLINK} ${OBJS} ${PROG} 2>&1

if [ -z "${INPUT}" ]; then
    ${PRUN} ${PRUNOPTS} ${PROG} 2>&1
else
    ${PRUN} ${PRUNOPTS} ${PROG} 2>&1 <${INPUT}
fi
exit
