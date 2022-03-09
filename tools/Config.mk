############################################################################
# Config.mk
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
############################################################################
# Control build verbosity
#
#  V=1,2: Enable echo of commands
#  V=2:   Enable bug/verbose options in tools and scripts

ifeq ($(V),1)
export Q :=
else ifeq ($(V),2)
export Q :=
else
export Q := @
endif

#
# Directories (relative to PASCAL)
#

PINCDIR     := $(PASCAL)/include
PLIBDIR     := $(PASCAL)/lib
PBINDIR     := $(PASCAL)/bin16
PASDIR      := $(PASCAL)/pascal
PLINKDIR    := $(PASCAL)/plink
INSNDIR     := $(PASCAL)/insn16
PAPPSDIR    := $(PASCAL)/papps
PTESTDIR    := $(PASCAL)/tests
PTOOLDIR    := $(PASCAL)/tools

LIBPOFFDIR  := $(PASCAL)/libpoff
LIBPASDIR   := $(PASCAL)/libpas
LIBINSNDIR  := $(INSNDIR)/libinsn
LIBEXECDIR  := $(INSNDIR)/libexec

ALLDIRS      = $(LIBPOFFDIR) $(LIBPASDIR) $(PASDIR) $(PLINKDIR) $(INSNDIR)
ALLDIRS     += $(PAPPSDIR) $(PTESTDIR) $(PTOOLDIR)

#
# Tools.  Most of these will be set in the NuttX build
#

CC          ?= /usr/bin/gcc
CPP         ?= /usr/bin/cpp
LD          ?= /usr/bin/ld
MAKE        ?= /usr/bin/make
AR          ?= /usr/bin/ar

RM          ?= /bin/rm -f
INSTALL     ?= install -m 0644
MKDIR       ?= /usr/bin/mkdir -p

HOSTCC      ?= /usr/bin/gcc
HOSTCPP     ?= /usr/bin/cpp
HOSTLD      ?= /usr/bin/ld
HOSTMAKE    ?= /usr/bin/make
HOSTAR      ?= /usr/bin/ar

HOSTRM      ?= /bin/rm -f

CFLAGS      ?= -Wall -g $(DEFINES) $(INCLUDES)
CFLAGS      += -I. -I$(PINCDIR)
LDFLAGS     += -L$(PLIBDIR)
ARFLAGS     ?= -r

HOSTCFLAGS  ?= -Wall -g $(DEFINES) $(INCLUDES)
HOSTCFLAGS  += -I. -I$(PINCDIR)
HOSTLDFLAGS += -L$(PLIBDIR)
HOSTARFLAGS ?= -r

PAS          = $(PBINDIR)/pascal$(TOOLEXEEXT)
POPT         = $(PBINDIR)/popt$(TOOLEXEEXT)
PLINK        = $(PBINDIR)/plink$(TOOLEXEEXT)

PASOPTS      = -I$(PUNITDIR)
POPTOPTS     =
PLINKOPTS    =

#
# Executable File Extension
#

ifdef CONFIG_HOST_WINDOWS
  HOSTEXEEXT ?= .exe
endif

ifndef CONFIG_PASCAL_TARGET_TOOLS
  TOOLEXEEXT ?= $(HOSTEXEEXT)
endif
