# Directories
PREFIX = /usr/local

# Deploy source to other directories via this variable.
# See target titled 'depd' for more information.
DIR =

# You probably don't need to touch this
ARCHIVEDIR=..
MANPREFIX = ${PREFIX}/share/man
LDDIRS = -L$(PREFIX)/lib
#LDFLAGS = -lmbedtls -lmbedx509 -lmbedcrypto -ldl -lm -laxtls
LDFLAGS = -ldl -lm 
DSOFLAGS = -fPIC -shared 
DSOEXT = so 
SHARED_LDFLAGS =
CC = cc #clang is also a good choice
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
	-DNW_BEATDOWN_MODE \
	-DNW_VERBOSE

# Flags
# -g allows for source annotations
# -pg allows for regular profiling
#PFLAGS = -g -pg 
#CFLAGS = $(PFLAGS) -fPIC -Wall -Werror -Wno-unused -std=c99 -Wno-deprecated-declarations -O2 -pedantic $(LDDIRS) $(LDFLAGS) $(DFLAGS)
#CFLAGS = -g -Wall -Werror -Wno-unused -Wstrict-overflow -ansi -std=c99 -Wno-deprecated-declarations -O2 -pedantic-errors $(LDDIRS) $(LDFLAGS) $(DFLAGS)
# No optimizations...
CFLAGS = -g -Wall -Werror -Wno-unused -Wstrict-overflow -ansi -std=c99 -Wno-deprecated-declarations -O0 -pedantic-errors $(LDDIRS) $(LDFLAGS) $(DFLAGS)

# Always make sure you set something via LCFLAGS +=
LCFLAGS = --leak-check=full ./test-nw -t

# Always make sure you set something via DBFLAGS +=
DBFLAGS = -ex run --args ./test-nw -t
#DBFLAGS = -ex run --args ./test-http

# Cygwin 
#CFLAGS = -Wall -Werror -Wno-unused -std=c99 -Wno-deprecated-declarations -O2 -pedantic ${LDFLAGS}
#DSOFLAGS = -fpic
#DSOEXT = dll

# OSX 
#CFLAGS = -Wall -Werror -Wno-unused -std=c99 -Wno-deprecated-declarations -O2 -pedantic ${LDFLAGS}
#DSOFLAGS = -fpic
#DSOEXT = dylib
