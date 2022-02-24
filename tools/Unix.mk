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

# Check if the system has been configured

PASCAL = ${shell pwd}

ifneq ($(MAKECMDGOALS),menuconfig)
ifeq ($(wildcard $(PASCAL)/.config),)
.DEFAULT default:
	$(warning "== Pascal has not been configured! ==")
	$(error   "Try:  'make menuconfig' to create a configuration");
endif
endif

-include $(PASCAL)/.config
include $(PASCAL)/tools/Config.mk

CLEANDIRS  = $(ALLDIRS)

# ---------------------------------------------------------------------------
# Tools

MKCONFIG = $(PTOOLDIR)/mkconfig$(HOSTEEXT)

# ---------------------------------------------------------------------------
# Objects and targets

LIBS = $(PLIBDIR)/libpoff.a $(PLIBDIR)/libpas.a  $(PLIBDIR)/libinsn.a

define MAKE_template
$(1)_$(2):
	+$(Q) $(MAKE) -C $(1) $(2)
endef

all: pascal popt plink plist prun papps
.PHONY: all config.h mkconfig libpoff.a libpas.a libinsn.a pascal popt plink plist prun menuconfig clean distclean

$(MKCONFIG) : $(PTOOLDIR)/mkconfig.c
	$(Q) $(MAKE) -C $(PTOOLDIR) mkconfig$(HOSTEEXT)

$(PINCDIR)/config.h: .config $(MKCONFIG)
	$(Q) $(MKCONFIG) . >$(PINCDIR)/config.h

config.h: $(PINCDIR)/config.h

$(PLIBDIR):
	$(Q) mkdir $(PLIBDIR)

$(PLIBDIR)/libpoff.a: $(PLIBDIR) $(PINCDIR)/config.h
	$(Q) $(MAKE) -C $(LIBPOFFDIR) libpoff.a

libpoff.a: $(PLIBDIR)/libpoff.a

$(PLIBDIR)/libpas.a: $(PLIBDIR) $(PINCDIR)/config.h
	$(Q) $(MAKE) -C $(LIBPASDIR) libpas.a

libpas.a: $(PLIBDIR)/libpas.a

$(PLIBDIR)/libinsn.a: $(PLIBDIR) $(PINCDIR)/config.h
	$(Q) $(MAKE) -C $(LIBINSNDIR) libinsn.a

libinsn.a: $(PLIBDIR)/libinsn.a

$(PBINDIR):
	$(Q) mkdir $(PBINDIR)

$(PBINDIR)/pascal: $(PBINDIR) $(PINCDIR)/config.h $(LIBS)
	$(Q) $(MAKE) -C $(PASDIR)

pascal: $(PBINDIR)/pascal

$(PBINDIR)/plink: $(PBINDIR) $(PINCDIR)/config.h $(LIBS)
	$(Q) $(MAKE) -C $(PLINKDIR)

plink: $(PBINDIR)/plink

$(PBINDIR)/popt: $(PBINDIR) $(PINCDIR)/config.h $(LIBS)
	$(Q) $(MAKE) -C $(INSNDIR) popt

popt: $(PBINDIR)/popt

$(PBINDIR)/prun: $(PBINDIR) $(PINCDIR)/config.h $(LIBS)
	$(Q) $(MAKE) -C $(INSNDIR) prun

prun: $(PBINDIR)/prun

$(PBINDIR)/plist: $(PBINDIR) $(PINCDIR)/config.h $(LIBS)
	$(Q) $(MAKE) -C $(INSNDIR) plist

plist: $(PBINDIR)/plist

$(PBINDIR)/papps: $(PBINDIR) $(PINCDIR)/config.h $(LIBS)
	$(Q) $(MAKE) -C $(INSNDIR) all

papps: $(PBINDIR)/papps

menuconfig:
	$(Q) kconfig-mconf Kconfig

$(foreach SDIR, $(CLEANDIRS), $(eval $(call MAKE_template,$(SDIR),clean)))
$(foreach SDIR, $(CLEANDIRS), $(eval $(call MAKE_template,$(SDIR),distclean)))

clean:  $(foreach SDIR, $(CLEANDIRS), $(SDIR)_clean)
	$(Q) $(RM) core *~
	$(Q) $(RM) -rf $(PLIBDIR)
	$(Q) $(RM) -rf $(PBINDIR)

distclean:  $(foreach SDIR, $(CLEANDIRS), $(SDIR)_distclean)
	$(Q) $(RM) $(PINCDIR)/config.h
	$(Q) $(RM) .config .config.old
