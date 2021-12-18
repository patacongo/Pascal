############################################################################
# Make.config.h
#
#   Copyright (C) 2008 Gregory Nutt. All rights reserved.
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
PASCAL   = ${shell pwd}

include $(PASCAL)/Make.config
include $(PASCAL)/Make.defs

INCDIR   = $(PASCAL)/include
CONFIGH  = $(INCDIR)/config.h

CONFIGS  = CONFIG_DEBUG CONFIG_TRACE CONFIG_INSN16 CONFIG_INSN32
CONFIGS += CONFIG_REGM CONFIG_HAVE_LIBM

# ----------------------------------------------------------------------
# Objects and targets

all: config.h
.PHONY: all config.h $(CONFIGS) header trailer clean

$(CONFIGS):
	@if [ "$($@)" = "y" ] ; then \
		echo "#define $@ 1" >>$(CONFIGH) ; \
	else \
		echo "#undef  $@" >>$(CONFIGH) ; \
	fi

header:
	@$(RM) $(INCDIR)/config.h
	@echo "/* config.h:  Autogenerated -- Do not edit. */" >$(CONFIGH)
	@echo "" >>$(CONFIGH)
	@echo "#ifndef __CONFIG_H" >>$(CONFIGH)
	@echo "#define __CONFIG_H 1" >>$(CONFIGH)
	@echo "" >>$(CONFIGH)

trailer:
	@echo "" >>$(CONFIGH)
	@echo "#endif /* __CONFIG_H */" >>$(CONFIGH)

$(CONFIGH): Make.config header $(CONFIGS) trailer

config.h: $(CONFIGH)

clean:
	$(RM) $(INCDIR)/config.h
