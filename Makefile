include config.mk
NAME = nw
TEST = test-nw
SRC = lite.c nw.c $(TEST).c
OBJ = ${SRC:.c=.o}
#if 0
#BONUS POINTS: Make the cpp run on the Makefile to chop your dev targets when packaging.
VERSION_MAJOR=0
VERSION_MINOR=2
IGNORE = $(ALIAS) archive vendor
AR = tar
ARFLAGS = czf
ARCHIVEDIR = ..
ARCHIVEFMT = gz
ARCHIVENAME = $(NAME).`date +%F`.`date +%H.%M.%S`
ARCHIVEFILE = $(ARCHIVENAME).tar.${ARCHIVEFMT}
PKGNAME = $(NAME)-$(VERSION_MAJOR).$(VERSION_MINOR)
COMMIT = *.c *.h README.md CHANGELOG
#endif

#if 1 
CLEAN = $(NAME)
.PHONY: all clean debug echo http archive
#else
CLEAN = $(NAME) $(TEST) *.o *.exe *.out *.stackdump *.swp *.swo
.PHONY: pkg debug leak commit permissions restore-permissions backup archive
#endif

# Build the library and it's tests
main: $(OBJ)
main: build

# Build the http example
http: OBJ = lite.o nw.o test-http.o
http: TEST = test-http
http: lite.o nw.o test-http.o
http: build

# Run any build
build:
	@echo CC -o $(TEST) $(OBJ) $(CFLAGS)
	@$(CC) -o $(TEST) $(OBJ) $(CFLAGS)

.c.o:
	@echo CC $<
	@${CC} -c ${CFLAGS} $<

clean:
	-echo find . -type f `echo $(CLEAN) | sed '{ s; ; -o -iname ;g; s;^;-iname ;'}`
	-find . -type f `echo $(CLEAN) | sed '{ s; ; -o -iname ;g; s;^;-iname ;'}` | \
		xargs rm

#if 0
# Install target if it's ever needed
install:

# Uninstall target if it's ever needed
uninstall:
	-rm -rf $(PREFIX)/lib/lib$(NAME).a
	-rm -rf $(PREFIX)/lib/lib$(NAME).so
	-rm -rf $(PREFIX)/lib/lib$(NAME).dll
	-rm -f $(PREFIX)/include/$(NAME).h
	-rm -rf $(PREFIX)/include/$(NAME)

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

# Debug target
debug: main
debug:
	$(DB) $(DBFLAGS)

# Leak check target
leak: main
leak:
	$(LC) $(LCFLAGS)

# Git commits
commit:
	git add $(COMMIT)
	git commit -a

# Github documentation
ghpages:
	git branch gh-pages

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

# Set permissions when unpacking to unruly systems (like Windows)
permissions:
	@find | grep -v './tools' | grep -v './examples' | grep -v './.git' | sed '1d' | xargs stat -c 'chmod %a %n' > PERMISSIONS

# Reset permissions according to file that should exist.
restore-permissions:
	chmod 744 PERMISSIONS
	./PERMISSIONS
	chmod 644 PERMISSIONS

# Create an archive for backup purposes 
backup:
	@echo tar chzf $(ARCHIVEDIR)/${ARCHIVEFILE} --exclude-backups \
		`echo $(IGNORE) | sed '{ s/^/--exclude=/; s/ / --exclude=/g; }'` ./
	@tar chzf $(ARCHIVEDIR)/${ARCHIVEFILE} --exclude-backups \
		`echo $(IGNORE) | sed '{ s/^/--exclude=/; s/ / --exclude=/g; }'` ./

# Create an archive for historical purposes
archive: ARCHIVEDIR = archive
archive: backup

# Extract a test for Bash (this will freeze if TESTNAME is not spec'd)
bash:	START=`nl -b a $(NAME).h | sed -n '/begin $(TESTNAME) test/p' | awk '{ print $$1 + 1 }'`
bash:	END  =`nl -b a $(NAME).h | sed -n '/end $(TESTNAME) test/p' | awk '{ print $$1 - 1 }'`
bash:
	@sed -n $(START),$(END)p $(NAME).h | /bin/bash -
	
# Different tests 
post: TESTNAME=post
post: bash

fork: TESTNAME=fork
fork: bash

#endif

