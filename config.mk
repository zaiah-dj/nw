# Directories
PREFIX = /usr/local

# Deploy source to other directories via this variable.
# See target titled 'depd' for more information.
DIR =

# You probably don't need to touch this
ARCHIVEDIR=..
MANPREFIX = ${PREFIX}/share/man
LDDIRS = -L$(PREFIX)/lib
LDFLAGS = -ldl -lm 
DSOFLAGS = -fPIC -shared 
DSOEXT = so 
SHARED_LDFLAGS =
#CC = gcc
CC = clang 
LC = valgrind
DB = gdb

# Defines
#	-DNW_FOLLOW     - Print each branchable call as the program passes 
#    through execution.
#	-DNW_VERBOSE    - Print each error message to stderr if anything fails.
#	-DNW_KEEP_ALIVE - Define whether or not to leave connections open 
#    after NW_COMPLETED is reached.
#	-DNW_SKIP_PROC  - Define whether or not to skip the processing step 
#    of a connection. 
# -DNW_MIN_ACCEPTABLE_READ   - Define a floor for the number of bytes 
#    received during read and drop the connection if it's not reached.
# -DNW_MIN_ACCEPTABLE_WRITE  - Define a floor for the number of bytes 
#    received during write and drop the connection if it's not reached.
#	-DNW_LOCAL_USERDATA  - Compile with space for local userdata.
#	-DNW_GLOBAL_USERDATA - Compile with space for global userdata.
#	-DNW_CATCH_SIGNAL    - Listen for signals...
DFLAGS = \
	-DNW_FOLLOW \
	-DNW_DISABLE_LOCAL_USERDATA \
	-DNW_VERBOSE

# Flags
COMPLAIN = -Wno-unused
ADDLFLAGS = ASAN_SYMBOLIZER_PATH=/usr/bin/llvm-symbolizer
CLANGFLAGS = -g -Wall -Werror -std=c99 $(COMPLAIN) \
	-fsanitize=address -fsanitize=undefined-trap \
	-fsanitize-undefined-trap-on-error $(DFLAGS)
GCCFLAGS = -g -Wall -Werror $(COMPLAIN) \
	-Wstrict-overflow -ansi -std=c99 \
	-Wno-deprecated-declarations -O0 \
	-pedantic-errors $(LDDIRS) $(LDFLAGS) $(DFLAGS)
CFLAGS=$(CLANGFLAGS)
#CFLAGS=$(GCCFLAGS)

# Always make sure you set something via LCFLAGS +=
LCFLAGS = --leak-check=full

# Always make sure you set something via DBFLAGS +=
DBFLAGS = -ex run --args
