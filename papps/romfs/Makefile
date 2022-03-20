############################################################################
# Makefile
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
#
# Directories
#

PROMFSDIR = ${shell pwd}
PAPPSDIR  = $(PROMFSDIR)/..
PASCAL    = $(PAPPSDIR)/..
PIMAGEDIR = $(PAPPSDIR)/image
PTOOLDIR  = $(PASCAL)/tools

include $(PASCAL)/tools/Config.mk

#
# Tools
#

MKROMFS = $(PTOOLDIR)/mkromfs.sh

#
# Objects
#

OBJS = romfs.o

#
# Targets
#

all: romfs.o
.PHONY: all clean distclean

$(PIMAGEDIR):
	$(Q) $(MKDIR) $(PIMAGEDIR)

romfs.c: $(PIMAGEDIR)
	$(Q) $(MKROMFS) $(PIMAGEDIR) $(PROMFSDIR)/romfs.c

$(OBJS): %.o: %.c
	$(Q) echo "  $@"
	$(Q) $(CC) -c $(CFLAGS) $< -o $@

clean:
	$(Q) $(RM) -f romfs.*

distclean: clean
	$(Q) $(RM) .depend Make.dep
