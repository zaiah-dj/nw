NAME = nw
TEST = test-nw
SRC = buff.c lite.c nw.c
OBJ = ${SRC:.c=.o}
RUNARGS = ./test-http
PREFIX = /usr/local
ARCHIVEDIR=..
MANPREFIX = ${PREFIX}/share/man
DFLAGS = \
	-DNW_FOLLOW \
	-DNW_DISABLE_LOCAL_USERDATA \
	-DNW_VERBOSE
ADDLFLAGS = ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer
CLANGFLAGS = -g -Wall -Werror -std=c99 -Wno-unused -fsanitize=address -fsanitize=undefined-trap -fsanitize-undefined-trap-on-error
CFLAGS=$(CLANGFLAGS)
INVOKE=ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer
CC = clang 
GCCFLAGS = -g -Wall -Werror -Wno-unused -Wstrict-overflow -ansi -std=c99 -Wno-deprecated-declarations -O0 -pedantic-errors
CFLAGS=$(GCCFLAGS)
INVOKE=
CC = gcc
CLEAN = $(NAME) $(OBJ)
.PHONY: all clean debug echo http archive

# Build all requested targets
main: nw http run
main:
	@printf ''>/dev/null

# Run it
run:
	@echo $(INVOKE) $(RUNARGS)
	@$(INVOKE) $(RUNARGS)
	
# Build the library and it's tests
nw: $(OBJ)
	@echo $(CC) -o $(TEST) $^ $(TEST).c $(CFLAGS)
	@$(CC) -o $(TEST) $^ $(TEST).c $(CFLAGS)

# Build the http example
http: $(OBJ)
	@echo $(CC) -o test-http $^ test-http.c $(CFLAGS)
	@$(CC) -o test-http $^ test-http.c $(CFLAGS)

# Objects
.c.o:
	@echo $(CC) -c ${CFLAGS} $<
	@${CC} -c ${CFLAGS} $<

# Clean target
clean:
	-rm *.exe
	-echo find . -type f `echo $(CLEAN) | sed '{ s; ; -o -iname ;g; s;^;-iname ;'}`
	-find . -type f `echo $(CLEAN) | sed '{ s; ; -o -iname ;g; s;^;-iname ;'}` | \
		xargs rm

#if 0
# Package target (create gzip or bz2 - fails if PKGNAME is not set)
pkg: clean
pkg:
	@test ! -z $(PKGNAME)
	@mkdir $(PKGNAME)/
	@cp *.c *.h *.md config.mk $(PKGNAME)/
	@cpp Makefile | sed '/^#/d' > $(PKGNAME)/Makefile
	@echo $(AR) $(ARFLAGS) $(PKGNAME).tar.$(ARCHIVEFMT)
	@$(AR) $(ARFLAGS) $(PKGNAME).tar.$(ARCHIVEFMT) $(PKGNAME) 
	@rm -rf $(PKGNAME)

# Github documentation
ghpages:
	git branch gh-pages

# Make a changelog
changelog:
	@echo "Creating / updating CHANGELOG document..."
	@touch CHANGELOG

# Notate a change (Target should work on all *nix and BSD)
change:
	@test -f CHANGELOG || printf "No changelog exists.  Use 'make changelog' first.\n\n"
	@test -f CHANGELOG
	@echo "Press [Ctrl-D] to save this file."
	@cat > CHANGELOG.USER
	@date > CHANGELOG.ACTIVE
	@sed 's/^/\t -/' CHANGELOG.USER >> CHANGELOG.ACTIVE
	@printf "\n" >> CHANGELOG.ACTIVE
	@cat CHANGELOG.ACTIVE CHANGELOG > CHANGELOG.NEW
	@rm CHANGELOG.ACTIVE CHANGELOG.USER
	@mv CHANGELOG.NEW CHANGELOG

# Extract a test for Bash (this will freeze if TESTNAME is not spec'd)
bash:	START=`nl -b a $(NAME).h | sed -n '/begin $(TESTNAME) test/p' | awk '{ print $$1 + 1 }'`
bash:	END  =`nl -b a $(NAME).h | sed -n '/end $(TESTNAME) test/p' | awk '{ print $$1 - 1 }'`
bash:
	@sed -n $(START),$(END)p $(NAME).h | /bin/bash -
	
# ??? test 
post: TESTNAME=post
post: bash

# Fork test
fork: TESTNAME=fork
fork: bash
#endif
