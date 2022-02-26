#############################################################################
# Makefile
# Makefile for Unix-like environment
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

PASCAL     = ${shell pwd}/..

-include $(PASCAL)/.config
include $(PASCAL)/tools/Config.mk

PIMAGEDIR   = $(PAPPSDIR)/image
PUNITDIR    = $(PAPPSDIR)/units

DELIM      ?=  $(strip /)
BUILDDIRS  := $(dir $(wildcard *$(DELIM)Makefile))
PBUILDDIRS := $(dir $(wildcard *$(DELIM)PasMakefile))

# ---------------------------------------------------------------------------
# Objects and targets

define MAKE_template
$(1)_$(2):
	+$(Q) $(MAKE) -C $(1) $(2)
endef

define PasMake_template
$(1)_$(2):
	+$(Q) $(MAKE) -C $(1) -f PasMakefile $(2)

endef

$(foreach DIR, $(BUILDDIRS), $(eval $(call MAKE_template,$(DIR),all)))
$(foreach DIR, $(BUILDDIRS), $(eval $(call MAKE_template,$(DIR),clean)))
$(foreach DIR, $(BUILDDIRS), $(eval $(call MAKE_template,$(DIR),distclean)))

$(foreach DIR, $(PBUILDDIRS), $(eval $(call PasMake_template,$(DIR),all)))
$(foreach DIR, $(PBUILDDIRS), $(eval $(call PasMake_template,$(DIR),clean)))
$(foreach DIR, $(PBUILDDIRS), $(eval $(call PasMake_template,$(DIR),distclean)))

$(PIMAGEDIR):
	$(Q) $(MKDIR) $(PIMAGEDIR)

all:    $(PIMAGEDIR) \
        $(foreach DIR, $(BUILDDIRS), $(DIR)_all) \
        $(foreach DIR, $(PBUILDDIRS), $(DIR)_all)

.PHONY: $(foreach DIR, $(BUILDDIRS), $(DIR)_all) \
        $(foreach DIR, $(BUILDDIRS), $(DIR)_clean) \
        $(foreach DIR, $(BUILDDIRS), $(DIR)_distclean) \
        $(foreach DIR, $(PBUILDDIRS), $(DIR)_all) \
        $(foreach DIR, $(PBUILDDIRS), $(DIR)_clean) \
        $(foreach DIR, $(PBUILDDIRS), $(DIR)_distclean)

clean:  $(foreach DIR, $(BUILDDIRS), $(DIR)_clean) \
        $(foreach DIR, $(PBUILDDIRS), $(DIR)_clean)
	$(Q) $(RM) -rf $(PIMAGEDIR)

distclean: clean \
           $(foreach DIR, $(BUILDDIRS), $(DIR)_distclean) \
           $(foreach DIR, $(PBUILDDIRS), $(DIR)_distclean)
