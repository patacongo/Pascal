#############################################################################
# Unix.mk
# Unix-like System Makefile
#
#   Copyright (C) 2008-2009, 2021-2022 Gregory Nutt. All rights reserved.
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

# ---------------------------------------------------------------------------
# Directories

PASCAL    = ${shell pwd}

-include $(PASCAL)/.config
include $(PASCAL)/tools/Config.mk

CLEANDIRS  = $(ALLDIRS)

# ---------------------------------------------------------------------------
# Tools

MKCONFIG = $(PTOOLDIR)/mkconfig$(HOSTEEXT)

# ---------------------------------------------------------------------------
# Objects and targets

LIBS = $(LIBDIR)/libpoff.a $(LIBDIR)/libpas.a  $(LIBDIR)/libinsn.a

define MAKE_template
$(1)_$(2):
	+$(Q) $(MAKE) -C $(1) $(2)
endef

all: pascal popt plink plist prun
.PHONY: all config.h mkconfig libpoff.a libpas.a libinsn.a pascal popt plink plist prun menuconfig clean distclean

$(MKCONFIG) : $(PTOOLDIR)/mkconfig.c
	$(Q) $(MAKE) -C $(PTOOLDIR) mkconfig$(HOSTEEXT)

$(INCDIR)/config.h: .config $(MKCONFIG)
	$(Q) $(MKCONFIG) . >$(INCDIR)/config.h

config.h: $(INCDIR)/config.h

$(LIBDIR):
	$(Q) mkdir $(LIBDIR)

$(LIBDIR)/libpoff.a: $(LIBDIR) $(INCDIR)/config.h
	$(Q) $(MAKE) -C $(LIBPOFFDIR) libpoff.a

libpoff.a: $(LIBDIR)/libpoff.a

$(LIBDIR)/libpas.a: $(LIBDIR) $(INCDIR)/config.h
	$(Q) $(MAKE) -C $(LIBPASDIR) libpas.a

libpas.a: $(LIBDIR)/libpas.a

$(LIBDIR)/libinsn.a: $(LIBDIR) $(INCDIR)/config.h
	$(Q) $(MAKE) -C $(LIBINSNDIR) libinsn.a

libinsn.a: $(LIBDIR)/libinsn.a

$(BINDIR):
	$(Q) mkdir $(BINDIR)

$(BINDIR)/pascal: $(BINDIR) $(INCDIR)/config.h $(LIBS)
	$(Q) $(MAKE) -C $(PASDIR)

pascal: $(BINDIR)/pascal

$(BINDIR)/popt: $(BINDIR) $(INCDIR)/config.h $(LIBS)
	$(Q) $(MAKE) -C $(INSNDIR) popt

popt: $(BINDIR)/popt

$(BINDIR)/plink: $(BINDIR) $(INCDIR)/config.h $(LIBS)
	$(Q) $(MAKE) -C $(PLINKDIR)

plink: $(BINDIR)/plink

$(BINDIR)/prun: $(BINDIR) $(INCDIR)/config.h $(LIBS)
	$(Q) $(MAKE) -C $(INSNDIR) prun

prun: $(BINDIR)/prun

$(BINDIR)/plist: $(BINDIR) $(INCDIR)/config.h $(LIBS)
	$(Q) $(MAKE) -C $(INSNDIR) plist

plist: $(BINDIR)/plist

menuconfig:
	$(Q) kconfig-mconf Kconfig

$(foreach SDIR, $(CLEANDIRS), $(eval $(call MAKE_template,$(SDIR),clean)))
$(foreach SDIR, $(CLEANDIRS), $(eval $(call MAKE_template,$(SDIR),distclean)))

clean:  $(foreach SDIR, $(CLEANDIRS), $(SDIR)_clean)
	$(Q) $(RM) core *~
	$(Q) $(RM) -rf $(LIBDIR)
	$(Q) $(RM) -rf $(BINDIR)

distclean:  $(foreach SDIR, $(CLEANDIRS), $(SDIR)_distclean)
	$(Q) $(RM) $(INCDIR)/config.h
	$(Q) $(RM) .config .config.old
