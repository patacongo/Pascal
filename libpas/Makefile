############################################################################
# libpas/Makefile
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

LIBPASDIR  = ${shell pwd}
PASCAL     = $(LIBPASDIR)/..
include $(PASCAL)/tools/Config.mk

#
# Objects and targets
#

LIBPASSRCS = pextension.c pbasename.c psignextend8.c psignextend16.c pswap.c
LIBPASOBJS = $(LIBPASSRCS:.c=.o)

OBJS       = $(LIBPASOBJS)
DEPS       = $(LIBPASSRCS:.c=.d)

SUFFIXES  += .d

#
# Targets
#

all: libpas.a
.PHONY: all libpas.a clean distclean

$(DEPS): %.d: %.c
	$(Q) echo "  $@"
	$(Q) $(CC) $(CFLAGS) -MM $< -MF $@

$(OBJS): %.o: %.c
	$(Q) echo "  $@"
	$(Q) $(CC) -c $(CFLAGS) $< -o $@

$(PLIBDIR)/libpas.a: $(OBJS)
	$(Q) echo "  $@"
	$(Q) $(AR) $(ARFLAGS) $@ $^ 1>/dev/null

libpas.a: $(DEPS) $(PLIBDIR)/libpas.a

clean:
	$(Q) $(RM) libpas.a *.o core *~

distclean: clean
	$(Q) $(RM) *.d .depend Make.dep

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),distclean)
-include $(DEPS)
endif
endif
