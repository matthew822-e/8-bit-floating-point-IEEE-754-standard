#-----------------------------------------------------------------------------
# Makefile
#-----------------------------------------------------------------------------

#-----------------------------------------------------------------------------
# Choose a compiler and its options
#--------------------------------------------------------------------------
CC   = gcc -std=gnu99	# Use gcc for Zeus
OPTS = -Og -Wall -Werror -Wno-error=unused-variable -Wno-error=unused-function
DEBUG = -g					# -g for GDB debugging

#--------------------------------------------------------------------
# Build Environment
#--------------------------------------------------------------------
UID := $(shell id -u)
SRCDIR=./src
OBJDIR=./obj
INCDIR=./inc
BUILDDIR=./build
BINDIR=.

#--------------------------------------------------------------------
# Build Files
#--------------------------------------------------------------------

#--------------------------------------------------------------------
# Compiler Options
#--------------------------------------------------------------------
INCLUDE=$(addprefix -I,$(INCDIR))
HEADERS=$(wildcard $(INCDIR)/*.h)
SOURCES=$(OBJDIR)/zuon.o $(BUILDDIR)/lex.yy.c $(BUILDDIR)/zuon_grammar.tab.c $(SRCDIR)/kifp.c $(SRCDIR)/common_functions.c $(SRCDIR)/symtab.c $(SRCDIR)/hashmap.c 
CFLAGS=$(OPTS) $(INCLUDE) $(DEBUG)

#--------------------------------------------------------------------
# Build Recipies for the Executables (binary)
#--------------------------------------------------------------------
TARGET=$(BINDIR)/zuon $(BINDIR)/ref_all_values

all: $(TARGET)

$(BINDIR)/ref_all_values: $(OBJDIR)/ref_all_values.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

$(BINDIR)/zuon: $(SOURCES) $(HEADERS)
	$(CC) -I./inc -g -o $@ $(SOURCES) -lfl -ly

$(OBJDIR)/zuon.o: $(SRCDIR)/zuon.c
	$(CC) -c $(CFLAGS) -o $@ $^

$(BUILDDIR)/lex.yy.c: $(SRCDIR)/zuon_tokens.l
	flex -o $@ $^

$(BUILDDIR)/zuon_grammar.tab.c $(BUILDDIR)/zuon_grammar.tab.h: $(SRCDIR)/zuon_grammar.y
	bison -vd -o $@ $^

clean:
	rm -f $(TARGET) $(BUILDDIR)/* $(OBJDIR)/zuon.o
