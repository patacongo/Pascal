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

-include $(PASCAL)/.config
include $(PASCAL)/tools/Config.mk

CLEANDIRS  = $(ALLDIRS)

# ---------------------------------------------------------------------------
# Tools

MKCONFIG = $(PTOOLDIR)/mkconfig$(HOSTEXEEXT)

# ---------------------------------------------------------------------------
# Objects and targets

LIBS  = $(PLIBDIR)/libpoff.a $(PLIBDIR)/libpas.a $(PLIBDIR)/libinsn.a 
LIBS += $(PLIBDIR)/libexec.a

define MAKE_template
$(1)_$(2):
	+$(Q) $(MAKE) -C $(1) $(2)
endef

all: pascal popt plink plist prun papps
.PHONY: all check_config mkconfig config.h libpoff.a libpas.a libinsn.a libexec.a pascal popt plink plist prun menuconfig clean distclean

check_config:
ifeq ($(wildcard $(PASCAL)/.config),)
	$(warning "== Pascal has not been configured! ==")
	$(error   "Try:  'make menuconfig' to create a configuration");
endif

$(MKCONFIG) : $(PTOOLDIR)/mkconfig.c
	$(Q) $(MAKE) -C $(PTOOLDIR) mkconfig$(HOSTEXEEXT)

$(PINCDIR)/config.h: .config $(MKCONFIG)
	$(Q) $(MKCONFIG) . >$(PINCDIR)/config.h

config.h: $(PINCDIR)/config.h

Kconfig:
	$(Q) echo "# Configuration for use with kconfig-frontends" >Kconfig
	$(Q) echo "" >>Kconfig
	$(Q) echo "config PASCAL" >>Kconfig
	$(Q) echo "	bool" >>Kconfig
	$(Q) echo "	default y" >>Kconfig
	$(Q) echo "" >>Kconfig
	$(Q) echo "config PASTOPDIR" >>Kconfig
	$(Q) echo "	string" >>Kconfig
	$(Q) echo "	default $(PASCAL)" >>Kconfig
	$(Q) echo "" >>Kconfig
	$(Q) echo "config PASCAL_BUILD_LINUX" >>Kconfig
	$(Q) echo "	bool" >>Kconfig
	$(Q) echo "	default y" >>Kconfig
	$(Q) echo "" >>Kconfig
	$(Q) echo "config PASCAL_BUILD_NUTTX" >>Kconfig
	$(Q) echo "	bool" >>Kconfig
	$(Q) echo "	default n" >>Kconfig
	$(Q) echo "" >>Kconfig
	$(Q) echo "source $(PASCAL)/tools/Kconfig.main" >>Kconfig
	$(Q) echo "source $(PASCAL)/insn16/Kconfig.insn" >>Kconfig
	$(Q) echo "source $(PASCAL)/papps/Kconfig.papps" >>Kconfig

$(PLIBDIR):
	$(Q) mkdir $(PLIBDIR)

$(PLIBDIR)/libpoff.a: check_config $(PLIBDIR) $(PINCDIR)/config.h
	$(Q) $(MAKE) -C $(LIBPOFFDIR) libpoff.a

libpoff.a: $(PLIBDIR)/libpoff.a

$(PLIBDIR)/libpas.a: check_config $(PLIBDIR) $(PINCDIR)/config.h
	$(Q) $(MAKE) -C $(LIBPASDIR) libpas.a

libpas.a: $(PLIBDIR)/libpas.a

$(PLIBDIR)/libinsn.a: check_config $(PLIBDIR) $(PINCDIR)/config.h
	$(Q) $(MAKE) -C $(LIBINSNDIR) libinsn.a

libinsn.a: $(PLIBDIR)/libinsn.a

$(PLIBDIR)/libexec.a: check_config $(PLIBDIR) $(PINCDIR)/config.h
	$(Q) $(MAKE) -C $(LIBEXECDIR) libexec.a

libexec.a: $(PLIBDIR)/libexec.a

$(PBINDIR):
	$(Q) mkdir $(PBINDIR)

$(PBINDIR)/pascal: check_config $(PBINDIR) $(PINCDIR)/config.h $(LIBS)
	$(Q) $(MAKE) -C $(PASDIR)

pascal: $(PBINDIR)/pascal

$(PBINDIR)/plink: check_config $(PBINDIR) $(PINCDIR)/config.h $(LIBS)
	$(Q) $(MAKE) -C $(PLINKDIR)

plink: $(PBINDIR)/plink

$(PBINDIR)/popt: check_config $(PBINDIR) $(PINCDIR)/config.h $(LIBS)
	$(Q) $(MAKE) -C $(INSNDIR) popt

popt: $(PBINDIR)/popt

$(PBINDIR)/prun: check_config $(PBINDIR) $(PINCDIR)/config.h $(LIBS)
	$(Q) $(MAKE) -C $(INSNDIR) prun

prun: $(PBINDIR)/prun

$(PBINDIR)/plist: check_config $(PBINDIR) $(PINCDIR)/config.h $(LIBS)
	$(Q) $(MAKE) -C $(INSNDIR) plist

plist: $(PBINDIR)/plist

$(PAPPSDIR)/papps: check_config $(PINCDIR)/config.h $(LIBS)
	$(Q) $(MAKE) -C $(PAPPSDIR) all

papps: $(PAPPSDIR)/papps

menuconfig: Kconfig
	$(Q) kconfig-mconf Kconfig

olddefconfig: Kconfig
	$(Q) kconfig-conf --olddefconfig Kconfig

$(foreach SDIR, $(CLEANDIRS), $(eval $(call MAKE_template,$(SDIR),clean)))
$(foreach SDIR, $(CLEANDIRS), $(eval $(call MAKE_template,$(SDIR),distclean)))

clean: $(foreach SDIR, $(CLEANDIRS), $(SDIR)_clean)
	$(Q) $(RM) core *~
	$(Q) $(RM) -rf $(PLIBDIR)
	$(Q) $(RM) -rf $(PBINDIR)

distclean: clean $(foreach SDIR, $(CLEANDIRS), $(SDIR)_distclean)
	$(Q) $(RM) $(PINCDIR)/config.h
	$(Q) $(RM) Kconfig .config .config.old
