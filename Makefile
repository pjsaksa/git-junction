# git-junction
# Copyright (c) 2016-2017 by Pauli Saksa
#
# Licensed under The MIT License, see file LICENSE.txt in this source tree.

CONSOLE_OBJECTS =cgitrc.o config.o console.o exception.o input.o key_menu.o main_menu.o \
  process_io.o repository_menu.o ssh_key.o terminal_input.o utils.o
SHELL_OBJECTS =shell.o quote.o config.o cgitrc.o

OBJECTS =$(CONSOLE_OBJECTS) $(SHELL_OBJECTS)
BINARIES=junction-console junction-shell

CONFIGURATION_FLAGS=
C_CXX_FLAGS        =-O2 -Wall -Wextra -Werror

CC =gcc -std=gnu99
CXX=g++ -std=gnu++11
CPPFLAGS =$(CONFIGURATION_FLAGS)
CFLAGS   =$(C_CXX_FLAGS)
CXXFLAGS =$(C_CXX_FLAGS)
LDFLAGS  =-s

#

all : $(OBJECTS) $(BINARIES)

# manual dependencies

junction-console : $(CONSOLE_OBJECTS)
junction-shell : $(SHELL_OBJECTS)

# rules

.SILENT:

%.o : %.cc
	@echo "COMPILE: $@"
	$(CXX) -c -o $@ $(CPPFLAGS) $(CXXFLAGS) $<

%.o : %.c
	@echo "COMPILE: $@"
	$(CC) -c -o $@ $(CPPFLAGS) $(CFLAGS) $<

$(BINARIES) :
	@echo "LINK:    $@"
	$(CXX) $(LDFLAGS) -o $@ $^

# clean

clean :
	$(RM) $(BINARIES) $(OBJECTS) $(OBJECTS:.o=.o.d)

distclean : clean
	$(RM) config.hh config.cc

# dependencies

%.o.d : %.cc
	@$(CXX) -o $@ -MM -MT "$@ $(@:.o.d=.o)" $(CPPFLAGS) $(@:.o.d=.cc)

%.o.d : %.c
	@$(CC) -o $@ -MM -MT "$@ $(@:.o.d=.o)" $(CPPFLAGS) $(@:.o.d=.c)

ifneq ($(strip $(OBJECTS)),)
ifneq ($(MAKECMDGOALS),clean)
$(OBJECTS) : Makefile
-include $(OBJECTS:.o=.o.d)
endif
endif
