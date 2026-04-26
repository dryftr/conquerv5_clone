# Include/Makefile.inc - Template for Include directory Makefile

SHELL = %%SHELL%%
CD = %%CD%%
CC = gcc
RM = %%RM%%
CP = %%CP%%
ECHO = %%ECHO%%
NULL = %%NULL%%

# Administrator settings
LOGIN = %%LOGIN%%
CFLAGS = -Wall -g -O0 -DDEBUG

# Build target - generate header.h from header.h.dist
all: build

build: header.h

# Generate header.h from template using awk (NOT compiler flags)
header.h: header.h.dist
	@$(ECHO) "Generating header.h for administrator: $(LOGIN)"
	@awk -v login="$(LOGIN)" '
		/^#define[ \t]+LOGIN/ {
			print "#define LOGIN \"" login "\""
			next
		}
		{ print }
	' header.h.dist > header.h
	@$(ECHO) "Header file generated successfully"

clean:
	@$(ECHO) "Include directory: cleaning..."

clobber: clean
	-$(RM) header.h $(NULL)
	@$(ECHO) "Include directory: cleaned"

install:
	@$(ECHO) "Include directory: nothing to install"

info:
	@$(ECHO) "Include directory configuration:"
	@$(ECHO) "  Administrator: $(LOGIN)"

.PHONY: all build clean clobber install info
