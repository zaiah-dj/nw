/*lite.h*/
/*Headers*/
#define _POSIX_C_SOURCE 200809L
#ifndef LITE_H
#define LITE_H
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

/*Track memory allocations*/
#define nalloc(x, y) malloc(x)

/*Some printfs*/
#define stprintf(k, v) \
	fprintf(stderr, "%30s: '%s'\n", k, v);

#define nmprintf(k, v) \
	fprintf(stderr, "%30s: %d\n", k, v);

#define spprintf(k, v) \
	fprintf(stderr, "%30s: %p\n", k, v);

/*Datatypes*/
typedef struct Error Error;
struct Error { char *str, *name; FILE *file; };


typedef struct Buffer Buffer;
struct Buffer {
	uint8_t *body;
	uint32_t pos;
	uint32_t size;
};


typedef struct ListNode ListNode;
struct ListNode {
	void *item;
	struct ListNode *next;
};


typedef struct List List;
struct List {
	uint32_t count;
	uint32_t element_size;
	void     (*free)(void *self);  /*Remember how this works.*/
	ListNode *head;
	ListNode *curr;
	ListNode *tail;
}; 


typedef struct KVList KVList;
struct KVList {
	List *list;        /* List of values. */
	List *index;       /* List of indices (could be const char ** too) */
	unsigned int size;  /* Size of set */
	unsigned int limit; /* Limit to keys added */
	unsigned int pos;   /* Where are we within the structure */
	KVList *kvlist;      /* Pointer to current record */
	KVList *kv;          /* If inner iterator method is written, this is handy */
}; 

typedef struct Value Value;
struct Value {
	#if 0
	union { int32_t n; char *s; char c; void *v; } value;
	#endif
	int32_t n; char *s; char c; void *v; 
};

typedef struct Option Option;
struct Option {
	char  *sht, *lng; 
	char  *description;
	char  type; /*n, s or c*/	
	_Bool set;  /*If set is not bool, it can define the order of demos*/
	Value v;
	_Bool (*callback)(char **av, Value *v, char *err);
	_Bool sentinel;
};

#ifndef NOFILES
/*Define this within the C file as a static structure.  No one else knows this exists*/
typedef struct OSInfo OSInfo;
struct OSInfo {
	uint8_t  dirsep;  /* '/' or '\' */
	uint32_t win_filename_limit;
	uint32_t nix_filename_limit;
	uint32_t msg_lim;
	uint32_t max_dir_depth;
};

typedef struct FileInfo FileInfo;
struct FileInfo {
	uint32_t mode;
	uint32_t inode;
	uint32_t device;
	uint32_t uid;
	uint32_t gid;
	uint32_t atime;
	uint32_t ctime;
	uint32_t mtime;
	uint32_t links;
	uint32_t size;
	uint32_t bs;
	uint32_t bsalloc;
	char type;
	char * extension;
	char * path;
	char * basename;
	char * dirname;
	char * realpath;
	char * filetype;
};
#endif



#ifndef NOSTATE
typedef struct Reference Reference;
struct Reference {
	const char *name;
	unsigned int size;	
	void *ptr;
};

typedef struct Tracker Tracker;
struct Tracker {
	char *file;
	char *fname;      /*Specify the current function name */
	char *msg;        /*A buffer for error messages */
	List *elements;   /*Keep track of allocated elements*/
	uint32_t line;
	uint32_t memlim;  /*Specify a bound on memory usage */
	uint32_t rc;      /*The return code of most recently executed function */
	uint32_t last;    /*The last allocated element */
};
#endif


typedef struct Socket Socket;
struct Socket {
	/*User settable stuff*/
	_Bool server;        //Is this a server or client?
	char *proto;         //Is this udp or tcp?
	unsigned int port;   //Define a port
	char *hostname;      //Hostname to BIND to

	/*...*/
	int domain;              /* AF_INET, AF_INET6, AF_UNIX */
	int protocol;            /* Transport protocol to use. */
	int conntype;            /* TCP, UDP, UNIX, etc. (sock_stream) */
	int err;

	/*Non user settable stuff*/
	unsigned int connections;/* How many people have tried to connect to you? */
	
	/* Socket settings */
	unsigned int bufsz;             /* Size of read buffer */
	unsigned int backlog;           /* Number of queued connections. */
	unsigned int waittime;          /* There is a way to query socket, but 
                                      do this for now to ensure your sanity. */
	char opened;             /* True or false? */

	/* More data */
	char *service;                  /* A string representation of service */
	char _class;                    /* Listener, receiver or child? */
	struct addrinfo hints;          /* Hints on addresses */
	struct addrinfo *res;           /* Results of address call. */
	Buffer *buffer;                 /* A buffer */
	//OBJECT(urn) *urn;               /* A URI parser */

	/* Server information -  used by bind() */
	int fd;                      /* Parent file descriptor */
	char portstr[6];                /* Write it here */
	struct sockaddr *srvaddr;       /* Server address structure */ 
	struct sockaddr_in tmpaddrinfo;
	struct sockaddr_in *srvaddrinfo;  /* Server's address information */
	socklen_t srvaddrlen;           /* Size of the above structure */
	char address[INET6_ADDRSTRLEN]; /* Address gets stored here */
	size_t addrsize;                /* Address information size */

	/* Client information - used by accept() */
	int clifd;                      /* Child file descriptor */
	struct sockaddr *cliaddr;       /* Client address structure */
	struct sockaddr_in *cliaddrinfo;  /* Client's address information */
	socklen_t cliaddrlen;           /* Size of the above structure */

	/* SSL / TLS */
	void *ssl_ctx;                  /* If SSL is in use, use this */
} socket_t;


typedef struct URIData {
	char scheme[64];
	char hostname[256];
	char resource[2056];
	char username[64];
	char password[64];
	char sport[6];
	uint16_t port;
} URIData;


#if 0
//#ifndef NOSOCKET
 #error "Please double check Socket datatypes.  Most char(s) should be _Bool(s)."

typedef struct AddressParser AddressParser;
struct AddressParser {
	char *protocol;   // max protocol size helps here...
	char *name;       // hostname (could be long, dunno how much)
	char *resource;   // 256 max per HTTP
	char *username;   // ?
	char *password;   // ?
	char sport[6];
	uint32_t port;
} uri_t;

typedef struct Connection Connection;
struct Connection {
	char msg[1024];
	Buffer *buffer;
	uint32_t size;
	uint32_t read;
	uint32_t written;
	uint32_t fd;
};


typedef struct Socket Socket;
struct Socket {
	/* Initial socket connection - socket() */
	int conntype;            				/* TCP, UDP, UNIX, etc. (sock_stream) */
	int domain;                     /* AF_INET, AF_INET6, AF_UNIX */
	int protocol;                   /* Transport protocol to use. */
	//_Bool opened;                    /* True or false? */
	char opened;                    /* True or false? */
	unsigned int connections;       /* How many people have tried to connect to you? */
	
	/* Socket settings */
	unsigned int bufsz;             /* Size of read buffer */
	unsigned int backlog;           /* Number of queued connections. */
	unsigned int waittime;          /* There is a way to query socket, but 
                                      do this for now to ensure your sanity. */

	/* More data */
	char *hostname;                 /* Hostname to BIND to */
	char *service;                  /* A string representation of service */
	char _class;                    /* Listener, receiver or child? */
	struct addrinfo hints;          /* Hints on addresses */
	struct addrinfo *res;           /* Results of address call. */
	Buffer *buffer;                 /* A buffer */
	OBJECT(urn) *urn;               /* A URI parser */

	/* Server information -  used by bind() */
	int fd;                      /* Parent file descriptor */
	unsigned int port;              /* Define a port */
	char portstr[6];                /* Write it here */
	struct sockaddr *srvaddr;       /* Server address structure */ 
	struct sockaddr_in *srvaddrinfo;  /* Server's address information */
	socklen_t srvaddrlen;           /* Size of the above structure */
	char address[INET6_ADDRSTRLEN]; /* Address gets stored here */
	size_t addrsize;                /* Address information size */

	/* Client information - used by accept() */
	int clifd;                      /* Child file descriptor */
	struct sockaddr *cliaddr;       /* Client address structure */
	struct sockaddr_in *cliaddrinfo;  /* Client's address information */
	socklen_t cliaddrlen;           /* Size of the above structure */

	/* SSL / TLS */
	void *ssl_ctx;                  /* If SSL is in use, use this */
} socket_t;


/*All sockets should be capable of both ipv4 and ipv6*/
 #define tcp_listener(data, hostname, port) \
 	NEW(socket)(data, SOCK_STREAM, PF_INET, IPPROTO_TCP, 's', port, hostname, NULL)
 
 #define tcp_sender(data) \
 	NEW(socket)(data, SOCK_STREAM, PF_INET, IPPROTO_TCP, 'c', port, NULL, NULL)
 
 #define udp_listener(data, hostname, port) \
 	NEW(socket)(data, SOCK_DGRAM, PF_INET, IPPROTO_UDP, 's', port, hostname, NULL)
 
 #define udp_sender(data) \
 	NEW(socket)(data, SOCK_DGRAM, PF_INET, IPPROTO_UDP, 'c', port, NULL, NULL)
 
 #define unix_listener(data) \
 	NEW(socket)(data, SOCK_STREAM, AF_UNIX, 's', port, hostname, NULL)
 
 #define unix_server(data) \
 	NEW(socket)(data, SOCK_STREAM, AF_UNIX, 'c', port, hostname, NULL)
 
 #define ssl_listener(data, hostname, port, ssl_ctx) \
 	NEW(socket)(data, SOCK_STREAM, PF_INET, IPPROTO_TCP, 's', port, hostname, ssl_ctx)
 
 #define ssl_server(data, hostname, port, ssl_ctx) \
 	NEW(socket)(data, SOCK_STREAM, PF_INET, IPPROTO_TCP, 'c', port, hostname, ssl_ctx)
#endif


/*Error handling and logging.*/
#define pprintf(x) do { fprintf(stderr, "%s\n", x); getchar(); } while (0)
void errname (char *msg); 
void errfile (FILE *file);


/*Definitions for verbose & non-verbose printing*/
#ifdef VVPRINTF 
 int errstatusf (char *file, int line, int code);
 void * erritemf (char *file, int line, void *p);
 void * __errnull (char *file, int line, char *msg); 
 void __errstep (char *file, int line, char *msg); 
 int __errstat (char *file, int line, int code, char *msg);
 void __errexit (char *file, int line, int code, char *msg);
 int __errfree (char *file, int line, int code, void *p, char *msg);
 int __errsys (char *file, int line, char *msg);

 #define errnull(x) __errnull(__FILE__, __LINE__, x)
 #define errstep(x) __errstep(__FILE__, __LINE__, x)
 #define errstat(x, y) __errstat(__FILE__, __LINE__, x, y)
 #define errexit(x, y) __errexit(__FILE__, __LINE__, x, y)
 #define errfree(x, y, z) __errfree(__FILE__, __LINE__, x, y, z)
 #define errsys(x) __errsys(__FILE__, __LINE__, x)
 
 #define STAT_STR "%s [line %d]: "
 #define SUCCESS \
 	errstatusf(__FILE__, __LINE__, 1)
 #define INITIALIZED(p) \
 	erritemf(__FILE__, __LINE__, p)
 /* the idea here is to increment automatically when you reach a point */
 /* you can probably use a global array to track this... it's so ugly */
 #define iiprintf(p) \
 	vvprintf("Initializing %s...", #p)
 #define FAILURE \
 	errstatusf(__FILE__, __LINE__, 0)
 #define vvprintf(...) \
 	fprintf(stderr, STAT_STR, __FILE__, __LINE__); \
 	fprintf(stderr, __VA_ARGS__); \
 	fprintf(stderr, "\n")
 #define eeprintf(...) \
 	fprintf(stderr, "%s\n", __VA_ARGS__)

#else
 void * __errnull (char *msg); 
 void __errstep (char *msg); 
 int __errstat (int code, char *msg);
 void __errexit (int code, char *msg);
 int __errfree (int code, void *p, char *msg);
 int __errsys (char *msg);
 
 #define errnull(x) __errnull(x)
 #define errstep(x) __errstep(x)
 #define errstat(x, y) __errstat(x, y)
 #define errexit(x, y) __errexit(x, y)
 #define errfree(x, y, z) __errfree(x, y, z)
 #define errsys(x) __errsys(x)

 #define STAT_STR
 #define SUCCESS 1
 #define iiprintf(p) p
 #define INITIALIZING(p) p
 #define FAILURE 0
 #define vvprintf(...)
 #define eeprintf(...)
#endif



/*Functions*/
/*To replace function pointers: <range>s#(\*\([a-z]*\))#\1#*/
/*free() & printf() are always going to be overwritten.*/
#if 0
void free(KVLIST *self);
void printf(KVLIST *self);
#endif
 
#ifndef NOBUFFER
 /*Buffers*/
 //Buffer * NEW(buffer)(unsigned int type, unsigned int size);
 Buffer * buffer_init (uint32_t size);
 _Bool buffer_append (Buffer *self, void *body, uint32_t size);
 _Bool buffer_prepend(Buffer *self, void *body, uint32_t size);
 _Bool buffer_insert (Buffer *self, void *body, uint32_t size);
 //uint32_t buffer_write (Buffer *self, char *fn, uint32_t s, uint32_t e);
 //uint32_t buffer_read (Buffer *self, void *p, uint32_t tsize, uint32_t bsize);
#endif

#ifndef NOSOCKET 
 /*Socket*/
 //void socket_info(struct SOCKET *self);
 void socket_free(Socket *self);
_Bool socket_open (Socket *sock);
 void socket_printf (Socket *sock);
_Bool socket_accept (Socket *sock, Socket *new);
_Bool socket_tcp_recv (Socket *sock, uint8_t *msg, uint32_t *len);
_Bool socket_tcp_rrecv (Socket *sock, uint8_t *msg, int get, uint32_t *len);
_Bool socket_tcp_send (Socket *sock, uint8_t *msg, uint32_t len);
//_Bool socket_udp_recv (Socket *sock, uint8_t *msg, uint32_t *len);
//_Bool socket_udp_send (Socket *sock, uint8_t *msg, uint32_t len);

unsigned int socket_connect(Socket *self, const char *hostname, unsigned int port);
 //buffer_t * socket_recvd(Socket *self);
void socket_addrinfo(Socket *self);
_Bool socket_bind(Socket *self);
_Bool socket_listen(Socket *self);
//uri_t * socket_parsed(Socket *self);
_Bool socket_shutdown(Socket *self, char *type);
_Bool socket_close(Socket *self);
_Bool socket_recv(Socket *self); 
 //Socket * socket_accept(struct SOCKET *self);
_Bool socket_send(Socket *self, char *msg, unsigned int length); 
void socket_release(Socket *self);
#endif

#ifndef NOMEM
_Bool memstr (const void * a, const void *b, int size);
int32_t memchrocc (const void *a, const char b, int32_t size);
int32_t memstrocc (const void *a, const void *b, int32_t size);
int32_t memstrat (const void *a, const void *b, int32_t size);
int32_t memchrat (const void *a, const char b, int32_t size);
int32_t memtok (const void *a, const uint8_t *tokens, int32_t rng, int32_t tlen);
int32_t memmatch (const void *a, const char *tokens, int32_t sz, char delim); 
#endif

#ifndef NOOPTIONS
void opt_usage(Option *opts, const char *msg, int status);	
_Bool opt_eval (Option *opts, int argc, char **argv);
 //union opt_value *get(Option *opts, const char *name);	
_Bool opt_set (Option *opts, char *flag); 
Value opt_get (Option *opts, char *flag); 
#endif

#if 0
#ifndef NOFILES
 /*Files*/
 char type    (struct FS *self, const char *path);
 char * basename(struct FS *self, const char *path);
 char * dirname (struct FS *self, const char *path);
 char * realpath(struct FS *self, const char *path);
 // LIST * list    (struct FS *self, const char *path, file_t *file[]); 
 LIST * list    (struct FS *self, const char *path);
 unsigned int exists  (struct FS *self, const char *path);
 unsigned int stat    (struct FS *self, const char *path, file_t *file);
 unsigned int chattr  (struct FS *self, const char *path, unsigned int perms);
 unsigned int mkdir   (struct FS *self, const char *path, const char *mode);
 unsigned int move    (struct FS *self, const char *oldpath, const char *newpath);
 unsigned int copy    (struct FS *self, const char *oldpath, const char *newpath);
 unsigned int remove  (struct FS *self, const char *path);
#endif
 
 
 
 
#ifndef NOKVLIST
 /*KV lists*/
 unsigned int append(KVLIST *self, char *k, char *v);
 unsigned int expand(KVLIST *self);
 unsigned int build(KVLIST *self, char **words);
 unsigned int kvbuild(KVLIST *self, kv_t *kv);
 unsigned int size(KVLIST *self);
 unsigned int advance(KVLIST *self); 
 LIST * index(KVLIST *self); 
 char * get(KVLIST *self, char *k);
 kv_t * next(KVLIST *self);
 // unsigned int lock(KVLIST *self);  // no more records... 
 // unsigned int row(KVLIST *self);  select a row...
#endif
 
 
 
 
#ifndef NORANDOM
 /*Random*/
 char * punct(struct RAND *t, unsigned int length);
 char * numbers(struct RAND *t, unsigned int length);
 char * chars(struct RAND *t, unsigned int length);
 char * alnum(struct RAND *t, unsigned int length);
 char * any(void *t, unsigned int length);
 int number(void);
 void test(void);
 int between(struct RAND *t, int x, int y);
 int range(int x, int y);
 void seed(void);
#endif
 
 
#ifndef NORENDERER 
 /*Render*/
 NEW(render)(unsigned int depth, char_t *chars);
 void free(struct RENDER *self);
 void printf(struct RENDER *self);
 unsigned int check(struct RENDER *self, buffer_t *buffer);
 LIST * map(struct RENDER *self, buffer_t *buffer);
 LIST * render(struct RENDER *self, unsigned int count);
 void source(struct RENDER *self, KVLIST *kv);
 void sink(struct RENDER *self, buffer_t *buffer);
#endif
 
 
 
 
#ifndef NOSTATE
 /*State*/
 extern Tracker *state;
 void initialize_tracker (Tracker *lstate);
 unsigned int size(void);
 void *find(const char *symbol);
 void *malloc(unsigned int size, const char *file, unsigned int line);
 void *nalloc(unsigned int size, const char *symbol);
 void free(const char *symbol);
 void dump(void);
 void freelast(void);
 void freestate(void);
#endif
 
 
 /*String*/
#ifndef NOSTRING
 unsigned int end(struct OBJECT(string) *self);
 unsigned int error(struct OBJECT(string) *self);
 char *       (*error_msg)(struct OBJECT(string) *self);
 void         (*clear_err)(struct OBJECT(string) *self);
 
 char *extract(struct OBJECT(string) *self, const char *src, unsigned int start, unsigned int end);
 char *trim(struct OBJECT(string) *self, const char *str);
 
 /* Positional characters */
 unsigned int posf(struct OBJECT(string) *self, const char *str, const char *find, unsigned int at);
 unsigned int pos(struct OBJECT(string) *self, const char *str, const char *find, unsigned int at);
 char *find(struct OBJECT(string) *self, const char *str);
 char *replace(struct OBJECT(string) *self, const char *str, const char *replace, char type);
 // char *concat(struct OBJECT(string) *self, const char *str1, const char *str2);
 char *concat(struct OBJECT(string) *self, const char *cat, const char **strs);
 char **isplit(struct OBJECT(string) *self, const char *str, unsigned int chars);
 char **csplit(struct OBJECT(string) *self, const char *str, char split);
 char **ssplit(struct OBJECT(string) *self, const char *str, char *split);
 char *reverse(struct OBJECT(string) *self, const char *str);
 void free(struct OBJECT(string) *self);
#endif
#endif
#endif
