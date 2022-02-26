#############################################################################
# NuttX.mk
# NuttX Makefile
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

# ---------------------------------------------------------------------------
# Directories

MENUDESC   = "Pascal Language Support"

PASCAL    := ${shell pwd}
include $(PASCAL)/tools/Config.mk

DELIM     ?=  $(strip /)
BUILDDIRS := $(dir $(wildcard *$(DELIM)NxMakefile))

# ---------------------------------------------------------------------------
# Definitions

define Make_template
$(1)_$(2):
	+$(Q) $(MAKE) -C $(1) -f NxMakefile $(2) TOPDIR="$(TOPDIR)" APPDIR="$(APPDIR)"

endef

# ---------------------------------------------------------------------------
# Targets

$(TOPDIR)/include/nuttx/config.h:

$(PINCDIR)/config.h:  $(TOPDIR)/include/nuttx/config.h
	$(Q) cp $(TOPDIR)/include/nuttx/config.h $(PINCDIR)/config.h

config.h: $(PINCDIR)/config.h

$(foreach DIR, $(BUILDDIRS), $(eval $(call Make_template,$(DIR),all)))
$(foreach DIR, $(BUILDDIRS), $(eval $(call Make_template,$(DIR),archive)))
$(foreach DIR, $(BUILDDIRS), $(eval $(call Make_template,$(DIR),install)))
$(foreach DIR, $(BUILDDIRS), $(eval $(call Make_template,$(DIR),context)))
$(foreach DIR, $(BUILDDIRS), $(eval $(call Make_template,$(DIR),register)))
$(foreach DIR, $(BUILDDIRS), $(eval $(call Make_template,$(DIR),depend)))
$(foreach DIR, $(BUILDDIRS), $(eval $(call Make_template,$(DIR),clean)))
$(foreach DIR, $(BUILDDIRS), $(eval $(call Make_template,$(DIR),distclean)))

Kconfig:
	$(Q) echo "# Configuration for use with kconfig-frontends" >Kconfig
	$(Q) echo "" >>Kconfig
	$(Q) echo "config PASDIR" >>Kconfig
	$(Q) echo "	string" >>Kconfig
	$(Q) echo "	default $(PASDIR)" >>Kconfig
	$(Q) echo "" >>Kconfig
	$(Q) echo "menu \"Pascal Support\"" >>Kconfig
	$(Q) echo "" >>Kconfig
	$(Q) echo "config PASCAL" >>Kconfig
	$(Q) echo "	bool \"Enable Pascal Support\"" >>Kconfig
	$(Q) echo "	default n" >>Kconfig
	$(Q) echo "	---help---" >>Kconfig
	$(Q) echo "		Enable building of Pascal support" >>Kconfig
	$(Q) echo "" >>Kconfig
	$(Q) echo "if PASCAL" >>Kconfig
	$(Q) echo "" >>Kconfig
	$(Q) echo "config PASCAL_BUILD_LINUX" >>Kconfig
	$(Q) echo "	bool" >>Kconfig
	$(Q) echo "	default n" >>Kconfig
	$(Q) echo "" >>Kconfig
	$(Q) echo "config PASCAL_BUILD_NUTTX" >>Kconfig
	$(Q) echo "	bool" >>Kconfig
	$(Q) echo "	default y" >>Kconfig
	$(Q) echo "" >>Kconfig
	$(Q) echo "source $(PASCAL)/tools/Kconfig.main" >>Kconfig
	$(Q) echo "source $(PASCAL)/papps/Kconfig.papps" >>Kconfig
	$(Q) echo "" >>Kconfig
	$(Q) echo "endif # PASCAL" >>Kconfig
	$(Q) echo "endmenu # Pascal Support" >>Kconfig

all:       config.h $(foreach DIR, $(BUILDDIRS), $(DIR)_all)

archive:   $(foreach DIR, $(BUILDDIRS), $(DIR)_archive)

install:   $(foreach DIR, $(BUILDDIRS), $(DIR)_install)

preconfig: Kconfig

context:   Kconfig $(foreach DIR, $(BUILDDIRS), $(DIR)_context)

register:  $(foreach DIR, $(BUILDDIRS), $(DIR)_register)

depend:    config.h $(foreach DIR, $(BUILDDIRS), $(DIR)_depend)

clean:     $(foreach DIR, $(BUILDDIRS), $(DIR)_clean)
	$(Q) $(RM) core *~
	$(Q) $(RM) -rf $(PLIBDIR)
	$(Q) $(RM) -rf $(PBINDIR)

distclean: $(foreach DIR, $(BUILDDIRS), $(DIR)_distclean)
	$(Q) $(RM) $(PINCDIR)/config.h
	$(Q) $(RM) Kconfig .config .config.old
