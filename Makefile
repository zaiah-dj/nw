NAME = tmnw
TEST = test-nw
SRC = single.c tmnw.c
OBJ = ${SRC:.c=.o}
RUNARGS = ./test-http
PREFIX = /usr/local
ARCHIVEDIR = ..
MANPREFIX = ${PREFIX}/share/man
DFLAGS = -DNW_FOLLOW -DNW_DISABLE_LOCAL_USERDATA -DNW_VERBOSE
ADDLFLAGS = ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer
CLANGFLAGS = -g -Wall -Werror -std=c99 -Wno-unused -fsanitize=address -fsanitize=undefined-trap -fsanitize-undefined-trap-on-error
CFLAGS = $(CLANGFLAGS)
INVOKE = ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer
CC = clang
GCCFLAGS = -g -Wall -Werror -Wno-unused -Wstrict-overflow -ansi -std=c99 -Wno-deprecated-declarations -O0 -pedantic-errors
CFLAGS=$(GCCFLAGS)
INVOKE=
CC = gcc
CLEAN = $(NAME) $(OBJ)

#.PHONY: all clean debug echo http archive

# Build all requested targets
main: tmnw http run
main:
	@printf ''>/dev/null

lib:
	$(CC) $(CFLAGS) -c $(NAME).c
 
# Run it
run:
	@echo $(INVOKE) $(RUNARGS)
	@$(INVOKE) $(RUNARGS)
	
# Build the library and it's tests
tmnw: tmnw.o
	@echo $(CC) -o $(TEST) $^ $(TEST).c $(CFLAGS) -DSQROOGE_H
	@$(CC) -o $(TEST) $^ $(TEST).c $(CFLAGS) -DSQROOGE_H

# Build the http example
http: tmnw.o
	@echo $(CC) -o test-http $^ test-http.c $(CFLAGS) -DSQROOGE_H
	@$(CC) -o test-http $^ test-http.c $(CFLAGS) -DSQROOGE_H

# Single is the last thing needed
single.o:
	$(CC) $(CFLAGS) -DSQROOGE_H -c single.c 

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

# Test that POST requests can be received
post: TESTNAME=post
post:
	curl --verbose --request POST \
		--form my_file=@tests/red52x35.jpg \
		--form paragraph="Flame flame flame" \
		--form author="Antonio R. Collins II" \
		http://localhost:2000 

# Test that forked requests work
fork: TESTNAME=fork
fork:
	test -d tests || mkdir tests
	for n in `seq 1 32`; do \
		wget -o tests/index.${n}.html http://localhost:2000 & 
	done 
#endif
