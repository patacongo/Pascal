#!/bin/sh
#############################################################################
# zipme.sh
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

DATECODE=$1

TAR="tar cvf"
ZIP=gzip

# Move up one directory

cd ..
HOME=`pwd`
DIR=${HOME}/pascal
SUBDIR=pascal-${DATECODE}

# Make sure we know what is going on

if [ -z ${DATECODE} ] ; then
   echo "You must supply a date code like a.b.c as a parameter"
   exit 1;
fi

if [ ! -d ${SUBDIR} ] ; then
   echo "Directory ${SUBDIR} does not exist."
   exit 1;
fi

if [ ! -x ${SUBDIR}/$0 ] ; then
   echo "You must cd to the directory containing this script."
   exit 1;
fi

# Define the ZIP file pathes

TAR_NAME=${SUBDIR}.tar
ZIP_NAME=${TAR_NAME}.gz

# Prepare the directory

make -C ${SUBDIR} deep-clean

find ${SUBDIR} -name \*~ -exec rm -f {} \; || \
	{ echo "Removal of emacs garbage failed!" ; exit 1 ; }

find ${SUBDIR} -name \#\* -exec rm -f {} \; || \
	{ echo "Removal of emacs garbage failed!" ; exit 1 ; }

find ${SUBDIR} -name .\*swp\* -exec rm -f {} \; || \
	{ echo "Removal of vi garbage failed!" ; exit 1 ; }

# Remove any previous tarballs

if [ -f ${TAR_NAME} ] ; then
    echo "Removing ${HOME}/${TAR_NAME}"
    rm -f ${TAR_NAME} || \
	{ echo "rm ${TAR_NAME} failed!" ; exit 1 ; }
fi

if [ -f ${ZIP_NAME} ] ; then
    echo "Removing ${HOME}/${ZIP_NAME}"
    rm -f ${ZIP_NAME} || \
	{ echo "rm ${ZIP_NAME} failed!" ; exit 1 ; }
fi

# Then zip it

${TAR} ${TAR_NAME} ${SUBDIR} || \
    { echo "tar of ${DIR} failed!" ; exit 1 ; }
${ZIP} ${TAR_NAME} || \
    { echo "zip of ${TAR_NAME} failed!" ; exit 1 ; }
