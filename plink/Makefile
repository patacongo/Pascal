############################################################################
# plink/Makefile
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

PLINKDIR  = ${shell pwd}
PASCAL    = $(PLINKDIR)/..

include $(PASCAL)/tools/Config.mk

#
# Objects and targets
#

PLINKSRCS = plink.c plsym.c plreloc.c
PLINKOBJS = $(PLINKSRCS:.c=.o)

OBJS      = $(PLINKOBJS)
DEPS      = $(PLINKSRCS:.c=.d)

SUFFIXES += .d

#
# Targets
#

all: plink
.PHONY: all plink check_libs clean

$(DEPS): %.d: %.c
	$(Q) echo "  $@"
	$(Q) $(CC) $(CFLAGS) -MM $< -MF $@

$(OBJS): %.o: %.c
	$(Q) echo "  $@"
	$(Q) $(CC) -c $(CFLAGS) $< -o $@

check_libs:
	$(Q) if [ ! -f $(PLIBDIR)/libpoff.a ] ; then \
		echo "$(PLIBDIR)/libpoff.a does not exist" ; \
		exit 1 ; \
	fi
	$(Q) if [ ! -f $(PLIBDIR)/libpas.a ] ; then \
		echo "$(PLIBDIR)/libpas.a does not exist" ; \
		exit 1 ; \
	fi
	$(Q) if [ ! -f $(PLIBDIR)/libinsn.a ] ; then \
		echo "$(PLIBDIR)/libinsn.a does not exist" ; \
		exit 1 ; \
	fi

$(PBINDIR)/plink: check_libs $(DEPS) $(OBJS)
	$(Q) echo "  plink$(HOSTEEXT)"
	$(Q) $(CC) -o $@ $(LDFLAGS) $(OBJS) -linsn -lpoff -lpas

plink: $(PBINDIR)/plink

clean:
	$(Q) $(RM) plink *.o core *~

distclean: clean
	$(Q) $(RM) *.d .depend Make.dep

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),distclean)
-include $(DEPS)
endif
endif
