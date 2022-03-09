############################################################################
# insn16/Makefile
#
#   Copyright (C) 2008, 2022 Gregory Nutt. All rights reserved.
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

INSNDIR    = ${shell pwd}
PASCAL     = $(INSNDIR)/..

include $(PASCAL)/tools/Config.mk

POPTDIR    = $(INSNDIR)/popt
PRUNDIR    = $(INSNDIR)/prun
PLISTDIR   = $(INSNDIR)/plist

#
# Definitions
#

DELIM     ?=  $(strip /)
BUILDDIRS := $(dir $(wildcard *$(DELIM)Makefile))

define Make_template
$(1)_$(2):
	+$(Q) $(MAKE) -C $(1) $(2) TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)"

endef

#
# Targets
#

all: libinsn.a libexec.a popt prun plist
PHONY: all libinsn.a libexec.a popt prun plist clean distclean

$(LIBINSNDIR)/libinsn.a:
	$(Q) $(MAKE) -C $(LIBINSNDIR)

libinsn.a: $(LIBINSNDIR)/libinsn.a

$(LIBEXECDIR)/libexec.a:
	$(Q) $(MAKE) -C $(LIBEXECDIR)

libexec.a: $(LIBEXECDIR)/libexec.a

$(PBINDIR)/popt:
	$(Q) $(MAKE) -C $(POPTDIR)

popt: $(PBINDIR)/popt

$(PBINDIR)/prun:
	$(Q) $(MAKE) -C $(PRUNDIR)

prun: $(PBINDIR)/prun

$(PBINDIR)/plist:
	$(Q) $(MAKE) -C $(PLISTDIR)

plist: $(PBINDIR)/plist

$(foreach DIR, $(BUILDDIRS), $(eval $(call Make_template,$(DIR),clean)))
$(foreach DIR, $(BUILDDIRS), $(eval $(call Make_template,$(DIR),distclean)))

clean: $(foreach DIR, $(BUILDDIRS), $(DIR)_clean)
	$(Q) $(RM) libinsn.a core *~

distclean: $(foreach DIR, $(BUILDDIRS), $(DIR)_distclean)
