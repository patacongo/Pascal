############################################################################
# insn16/popt/Makefile
# Host system makefile
#
#   Copyright (C) 2008, 2021-2022 Gregory Nutt. All rights reserved.
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

POPTDIR = ${shell pwd}
INSNDIR = $(POPTDIR)/..
PASCAL = $(POPTDIR)/../..

include $(PASCAL)/tools/Config.mk

#
# Tools
#

EXTRA_INCLUDES	= -I$(INSNDIR)/include
INCLUDES += $(EXTRA_INCLUDES)
CFLAGS   += $(EXTRA_INCLUDES)

#
# Objects and targets
#

POPTSRCS  = popt.c popt_strings.c popt_strconst.c popt_local.c
POPTSRCS += popt_constants.c popt_longconst.c popt_branch.c
POPTSRCS += popt_loadstore.c popt_reloc.c popt_finalize.c
POPTSRCS += popt_peephole.c popt_util.c
POPTOBJS  = $(POPTSRCS:.c=.o)

OBJS      = $(POPTOBJS)
DEPS      = $(POPTSRCS:.c=.d)
LIBS      = libpoff.a libpas.a

SUFFIXES += .d

#
# Targets
#

all: popt
.PHONY: all check_libs popt check_libs clean distclean

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

$(PBINDIR)/popt: check_libs $(DEPS) $(OBJS)
	$(Q) echo "  popt$(TOOLEXEEXT)"
	$(CC) -o $@ $(LDFLAGS) $(OBJS) -linsn -lpoff -lpas

popt: $(PBINDIR)/popt

clean:
	$(Q) $(RM) popt *.o core *~

distclean: clean
	$(Q) $(RM) *.d .depend Make.dep

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),distclean)
-include $(DEPS)
endif
endif
