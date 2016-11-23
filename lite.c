/*lite.c*/
#include "lite.h"

/* Local data structures and setters */
static Error err = { .str = NULL, .name = NULL, .file = NULL };

inline void errname (char *nam) { err.name = nam; }

inline void errfile (FILE *file) { err.file = file; }

inline static void 
errprintf (char *msg)
{
	/* This needs to eat more arguments */
	fprintf(!err.file ? stderr : err.file, "%s: %s\n",
		!err.name ? "(anonymous program)" : err.name, msg); 
}


/* Codes to be used with vvprintf */
#if VVPRINTF 
inline void * 
erritemf (char *file, int line, void *p) 
{
	fprintf(stderr, STAT_STR, file, line);
	fprintf(stderr, "%s\n", !p? "FAIL!" : "SUCCESS!");
	return p;
}


inline int
errstatusf (char *file, int line, int code)
{
	fprintf(stderr, STAT_STR, file, line);
	fprintf(stderr, "%s\n", !code ? "FAIL!" : "SUCCESS!");
	return code;
}
#else
#endif


/* One-line returns cannot be done with macros, so we resort to this... */
#if VVPRINTF
 inline void *__errnull (char *file, int line, char *msg) {
	fprintf(stderr, STAT_STR, file, line);
#else
 inline void *__errnull (char *msg) {
#endif
	errprintf(msg);
	return NULL;
}


#if VVPRINTF
 inline void __errstep (char *file, int line, char *msg) {
	fprintf(stderr, STAT_STR, file, line);
#else
 inline void __errstep (char *msg) {
#endif
	errprintf(msg);
	getchar();
}


#if VVPRINTF
 inline int __errstat (char *file, int line, int code, char *msg) {
	fprintf(stderr, STAT_STR, file, line);
#else
 inline int __errstat (int code, char *msg) {
#endif
	errprintf(msg);
	return code;
}

#if VVPRINTF
 inline void __errexit (char *file, int line, int code, char *msg) {
	fprintf(stderr, STAT_STR, file, line);
#else
 inline void __errexit (int code, char *msg) {
#endif
	errprintf(msg);
	exit(code);
}


#if VVPRINTF
 inline int __errfree (char *file, int line, int code, void *p, char *msg) {
	fprintf(stderr, STAT_STR, file, line);
#else
inline int __errfree (int code, void *p, char *msg) {
#endif
	free(!p ? NULL : p);
	return code;
}


#if VVPRINTF
 inline int __errsys (char *file, int line, char *msg) {
	fprintf(stderr, STAT_STR, file, line);
#else
 inline int __errsys (char *msg) {
#endif
	char buf[1024] = { 0 };
	if (msg)
		sprintf(buf, "%s - %s", strerror(errno), msg);
	else
		sprintf(buf, "%s", strerror(errno));
	errprintf(buf);
	return 0;
}


/*Functions for printing data*/
#ifndef NOBUFFER
void buffer_print (Buffer *self) {
	uint8_t *p = self->body;
	if (p)
		for (int g=0;g<self->size;g++)
			fprintf(stderr, "%c", p[g]);
	 #if 0 
		int g;
		char *p = (char *)b;
		fprintf(stderr, "%s\n", !msg ? "Buffer contents:": msg);
		for (g=0;g<length;g++)
			fprintf(stderr, "%c", p[g]);
		fprintf(stderr, "Message was %d bytes long.\n", g);
	 #endif
}


/*Add to and return a buffer*/
static uint8_t *allocate_buffer (uint8_t *old, uint8_t *new, uint32_t size) {
	if (!old) {
		if (!(old = malloc(size)))
			return errnull("Failed to allocate bytes for new buffer.");
	}
	else if ((new = realloc(old, size)) == NULL) {
		return errnull("Failed to allocate additional bytes for buffer.");
	}
	return new;
}


void buffer_free (Buffer *self) {
	if (self->body) 
		free(self->body);
	free(self);
}


_Bool buffer_append (Buffer *self, void *body, unsigned int size)
{
	vvprintf("Appending %d bytes to current buffer %p.", size, (void *)self);
	uint32_t p = self->pos;
	uint8_t *nbody=NULL;

	#if 1  /*Use 'allocate_buffer'*/
	if (!self->body) {
		if (!(self->body = malloc(p + size)))
			return errstat(1, "Failed to allocate bytes for new buffer.");
	}
	else if ((nbody = realloc(self->body, p + size)) == NULL) {
		return errstat(1, "Failed to allocate additional bytes for buffer.");
	}
	#endif
	
	if (memset(&nbody[p], 0, size) == NULL)
		return errstat(1, "Failed to initialize memory.");

	if (memcpy(&nbody[p], body, size) == NULL)
		return errstat(1, "Failed to append to data block.");

	/* A final update */
	self->pos = self->size += size;
	self->body = nbody;

	vvprintf("\tCurrent buffer size: %d.", self->size);
	return SUCCESS;
}



_Bool buffer_prepend (Buffer *self, void *body, unsigned int size)
{
	vvprintf("Prepending %d bytes to current buffer %p.", size, (void *)self);
	int c;
	#define db(s, i, e) \
		fprintf(stderr, "%s\n%s\n", s, "======================"); \
		vvprintf("Current size is %d", e); \
		for (c=i;c<e;c++) \
			fprintf(stderr, "%d: %c\n", c, nbody[c]); \
		pprintf("...")


	/*Define*/
	//buffer_t *buffer = self->data;
	uint32_t pos = self->pos;
	uint8_t *nbody = NULL;
	/* Move the original size to the end. */
	int32_t ss = pos - size;
	int32_t p = pos;
	int32_t z = size;

	/* Try a realloc */
	#if 1  /*Use 'allocate_buffer'*/
	if (!self->body) {
		if (!(self->body = malloc(pos + size)))
			return errstat(1, "Failed to allocate bytes for new buffer.");
	}
	else if ((nbody = realloc(self->body, pos + size)) == NULL) {
		return errstat(1, "Failed to allocate additional bytes for buffer.");
	}
	#endif

	/* memset the area after... */
	if (memset(&nbody[pos], 0, size) == NULL)
		return errstat(1, "Failed to initialize memory.");

	/* Move everything up in this ridiculous fashion, b/c memmove() fucking sucks! */
	while (p > 0) {
		if (memmove(&nbody[p], &nbody[ss], z) == NULL)
			return errstat(1, "Failed to prepend to data block.");
		if (memset(&nbody[ss], 0, size) == NULL)
			return errstat(1, "Failed to initialize memory.");
		p -= size;
		ss -= size;

		if (ss < 0) {
			ss = 0;
			if (memmove(&nbody[z], &nbody[ss], p) == NULL)
				return errstat(1, "Failed to prepend to data block.");
			if (memset(&nbody[ss], 0, z) == NULL)
				return errstat(1, "Failed to initialize memory.");
			break;
		}
	}

	/* Prepend the body. */
	if (memcpy(&nbody[0], body, size) == NULL)
		return errstat(1, "Failed to prepend to data block.");

	/* A final update */
	self->pos = self->size += size;
	self->body = nbody;
	return SUCCESS; 
}




_Bool buffer_insert (Buffer *self, void *body, unsigned int size)
{
	vvprintf("Adding %d bytes into buffer at %p", size, (void *) self);
	if ((self->pos + size) > self->size)
		return errstat(0, "Chunk too large for self.");

	if (memcpy(&self->body[self->pos], body, size) == NULL)
		return errstat(0, "Could not append to string.");

	self->pos += size;
	return SUCCESS; 
}

#if 0
/* Read a file or string into a buffer. */
Buffer *buffer_read (Buffer *self, void *p, unsigned int tsize, unsigned int bsize)
{
	FILE *fh = (FILE *)p;
	int b, c = 0, buf = (!bsize) ? 1024 : bsize;
	unsigned char mybuf[(const int)buf];
	buffer_t *data = self->data;

	/* You shouldn't have to, but it might be good to get the filesize */
	while ((b = fgetc(fh)) != EOF) {
		mybuf[c] = b;
		c++;
		if (c == buf) {
			if (!self->append(self, &mybuf, c)) return NULL;
			reseti(c);
		}
	}

	/* Finalize this */
	return (!self->append(self, &mybuf, c)) ? NULL : self->data;
}




/* ... */
_Bool buffer_write (Buffer *self, char *filename, unsigned int s, unsigned int e)
{
	int fd; 
	ssize_t wb = 0;
	/* BufferData *data = self->data; */

	if ((fd = open(filename, O_RDONLY)) < 0)
		return errstat(0, "Failed to open file at filename.");

	wb = write(fd, &self->data->body[s], !e ? self->data->size : (e - s));
	return (wb < (e - s)) ? 0 : 1;
}
 #endif
#endif



#ifndef NOLIST
_Bool list_prepend (List *self, void *item) 
{
	//List *list = self; 
	ListNode *node; 

	if (!(node = nalloc(sizeof(ListNode), NULL))) 
		return 0;
	if (!(node->item = nalloc(self->element_size, NULL))) 
		return 0;
	memcpy(node->item, item, self->element_size);
	node->next = self->head;
	self->head = node;

	if (!self->tail)
		self->tail = self->head;

	self->count++; 
	return 1;
}


_Bool list_append (List *self, void *item) 
{
	//list_t *list = self->self; 
	ListNode *node; 

	if (!(node = nalloc(sizeof(ListNode), NULL))) return 0;
	if (!(node->item = nalloc(self->element_size, NULL))) return 0;
	node->next = NULL;
	memcpy(node->item, item, self->element_size);

	if (!self->count)
		self->head = self->tail = node;
	else {
		self->tail->next = node; // Why is this?
		self->tail = node;
	}

	self->count++; 
	return 1;
}


uint32_t list_count (List *self) { 
	return self->count; 
}


uint32_t list_size (List *self) { 
	return (self->count * self->element_size); 
}


void list_free (List *self) 
{
	ListNode *node;
	 
	while (self->head != NULL) {
		node = self->head;
		self->head = node->next;	
		if (self->free) 
			self->free(node->item);
		free(node->item);
		free(node);
	}

	free(self);
}


ListNode *list_next (List *self) {
	return self->curr = (!self->curr) ? self->head : self->curr->next;
}


List *list_new (uint32_t size, void (*free_f)(void *self)) {
	List *list = nalloc(sizeof(List), NULL);
	list->count=0;
	list->element_size = size;
	list->head = list->tail = list->curr = NULL;
	list->free = !free_f ? NULL : free_f;
	return list;
}
#endif


#ifdef NOKVLIST
/*Free text within value fields of list*/
static void __cfree (void *item) {
	free(*(char **)item);
}


/*Free each inner list*/
static void __ifree (void *item) {
	// free the strdup'd strings
	kv_t *k = (kv_t *)item;
	while (k->key) {
		// fprintf(stderr, "\t\tfreeing k-> %s v-> %s...\n", k->key, k->value);
		free(k->key);
		free(k->value);
		k++;
	}
}


/*Initialize new kv_t blocks*/
static uint32_t __init (KVList *self) {
	int x = 0;
	if (!(self->kvlist = (KVList *)nalloc(self->size, NULL)))
		return 0;
	memset(self->kvlist, 0, self->size);
	for (x=0; x<(self->limit+1); x++) {
		(&self->kvlist[x])->key = NULL; 
		(&self->kvlist[x])->value = NULL; 
	}
	self->pos = 0;
	return 1;
}


KVList *kvlist_new (uint32_t elements) {
	/*Define*/
	KVList *self=NULL; 
	uint32_t sz = sizeof(KVList) * (elements + 1);

	/*Allocate object and local data*/
	if (!(self = nalloc(sizeof(KVList), 0)))
		return errnull("Failed to initialize KVList");

	/*Initialize data*/
	self->size = sz;
	self->limit = elements;
	self->pos = 0;

	/*Allocate a list*/
	if (!(self->list = list_new(sz, __ifree))) {
		free(self);
		return NULL;
	}

	/*Allocate an index*/
	if (!(self->index = list_new(sizeof(char *), __cfree))) {
		self->list->free(self->list);
		free(self);
		return NULL;
	}

	return INITIALIZED(self);
}

#endif


#define domain_type(domain) \
	(domain == AF_INET) ? "ipv4" : "ipv6"

#define type_type(type) \
		(type == SOCK_STREAM) ? "SOCK_STREAM" : \
		(type == SOCK_DGRAM) ? "SOCK_DGRAM" : \
		(type == SOCK_SEQPACKET) ? "SOCK_SEQPACKET" : \
		(type == SOCK_RAW) ? "SOCK_RAW" : \
		(type == SOCK_RDM) ? "SOCK_RDM" : "UNKNOWN SOCKET TYPE"

#define proto_type(proto) \
	struct protoent *__mp; \
	char *tb = !(__mp=getprotobynumber(proto)) ? "unknown" : __mp->p_name; \
	endprotoent();

#define class_type(sockclass) \
	(sockclass == 'c') ? "client" : \
	(sockclass == 'd') ? "server (child)" : "server (parent)"

#define buffer_type(sockclass)


void socket_printf (Socket *sock) {
	stprintf("class",       class_type(sock->_class));
	stprintf("conntype",    type_type(sock->conntype));
	stprintf("opened",      sock->opened ? "yes" : "no"); 
	stprintf("domain",      domain_type(sock->domain));
	nmprintf("buffer size", sock->bufsz);
	nmprintf("backlog",     sock->backlog);
	stprintf("hostname",    sock->hostname);
	nmprintf("fd",          sock->fd);
	nmprintf("port",        sock->port);
	 fprintf(stderr, "tcp socket buffer: %p (%s)\n", (void *)sock->buffer,
		sock->_class == 'c' ? "client" : sock->_class == 'd' ? "child" : "server");
}



void socket_addrinfo (Socket *sock)
{
	fprintf(stderr, "%s\n", "Getting address information for socket.");

	struct addrinfo *peek; 
	struct sockaddr_in *ipv4;
	struct sockaddr_in6 *ipv6;
	int status = getaddrinfo(sock->hostname, sock->portstr, &sock->hints, &sock->res);
	if (status != 0)
		errexit(1, "Could not get address information for this socket.");

	/* Loop through each */
	for (peek = sock->res; peek != NULL; peek = peek->ai_next) {
		void *addr;	
		char *ipver;
		if (peek->ai_family == AF_INET) {
			ipv4 = (struct sockaddr_in *)peek->ai_addr;
			addr = &(ipv4->sin_addr);
			ipver = "IPv4";
		}
		else {
			ipv6 = (struct sockaddr_in6 *)peek->ai_addr;
			addr = &(ipv6->sin6_addr);
			ipver = "IPv4";
		}

		if (inet_ntop(peek->ai_family, addr, sock->address, sizeof(sock->address)) == NULL)
			continue;
	
		fprintf(stderr, "%s: %s\n", ipver, sock->address);
		/* Break somewhere after finding a valid address. */
		break;
	}

	/* Free this */
	freeaddrinfo(peek);

	/* A list will probably be used here */
	// OBJECT(list) *addresses
	// { .ipver = 4, ipstr = 192.10..., .ipbin = peek->ai_addr, .len = peek->ai_addrlen }
}

#define DEFAULT_HOST "localhost"

#define set_sockopts(type,dom,prot) \
	sock->conntype = type, \
	sock->domain = dom, \
	sock->protocol = prot, \
	sock->hostname = (sock->server) ? ((!sock->hostname) ? DEFAULT_HOST : sock->hostname) : NULL, \
	sock->_class = (sock->server) ? 's' : 'c'

//initialize a socket...
//void socket_open (Socket *sock, int type, int domain, int proto, char sr, unsigned int port, const char *hostname, void *ssl_ctx)
_Bool socket_open (Socket *sock) {
	/* All of this can be done with a macro */
	sock->addrsize = sizeof(struct sockaddr);
	sock->bufsz = !sock->bufsz ? 1024 : sock->bufsz;
	sock->opened = 0;
	sock->backlog = 500;
	sock->waittime = 5000;  // 3000 microseconds

	if (!sock->proto || strcasecmp(sock->proto, "tcp") == 0)
		set_sockopts(SOCK_STREAM, PF_INET, IPPROTO_TCP);
	else if (strcasecmp(sock->proto, "udp") == 0)
		set_sockopts(SOCK_DGRAM, PF_INET, IPPROTO_UDP);

	/* Check port number (clients are zero until a request is made) */
	if (!sock->port || sock->port < 0 || sock->port > 65536) {
		/* Free allocated socket */
		vvprintf("Invalid port specified.");
		return 0;
		//return errnull("Invalid port specified.");
	}


	/* Set up the address data structure for use as either client or server */	
	if (sock->_class == 's') 
	{
		/*there must be a way to do this WITHOUT malloc*/
		//if ((sock->srvaddrinfo = (struct sockaddr_in *)nalloc(sizeof(struct sockaddr_in), "sockaddr.info")) == NULL)
		//	return;
			//return errnull("Could not allocate structure specified.");

		/*This ought to work...*/
		sock->srvaddrinfo = &sock->tmpaddrinfo;
		memset(sock->srvaddrinfo, 0, sizeof(struct sockaddr_in));
		struct sockaddr_in *saa = sock->srvaddrinfo; 
		saa->sin_family = AF_INET;
		saa->sin_port = htons(sock->port);
		(&saa->sin_addr)->s_addr = htonl(INADDR_ANY);
	}
	else if (sock->_class == 'c') 
	{
		/* Set up the addrinfo structure for a future client request. */
		struct addrinfo *h; 
		memset(&sock->hints, 0, sizeof(sock->hints));
		h = &sock->hints;
		h->ai_family = sock->domain;
		h->ai_socktype = sock->conntype;
	}

	/* Finally, create a socket. */
	if ((sock->fd = socket(sock->domain, sock->conntype, sock->protocol)) == -1) {
		return 0;
	}
	sock->opened = 1;

	/* Set timeout, reusable bit and any other options */
	struct timespec to = { .tv_sec = 2 };
	if (setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, &to, sizeof(to)) == -1) {
		// sock->free(sock);
		sock->err = errno;
		errsys("Could not reopen socket.");
		return 0;
	}
	return 1;
}

_Bool socket_bind (Socket *sock) {
	//set errno
	return (bind(sock->fd, (struct sockaddr *)sock->srvaddrinfo, sizeof(struct sockaddr_in)) != -1);
//		return errsys("Could not bind to socket.");
}

_Bool socket_listen (Socket *sock)
{
	//set errno
	return (listen(sock->fd, sock->backlog) != -1);
		//return errsys("Could not listen out on socket.");
}

_Bool socket_udp_recv (Socket *self)  {
#if 0
		case SOCK_DGRAM:
			rcvd = recvfrom(sock->fd, msg, ws, 0,
				NULL, NULL);
				//sock->cliaddr, &sock->cliaddrlen);

			if (rcvd == -1) return 0;
			msg[rcvd] = 0;
			fprintf(stderr, "udp recv'd bytes: %d\n", rcvd);
			fprintf(stderr, "%s\n", msg);	

			while (1) {
				rcvd = recvfrom(sock->fd, msg, ws, 0,
					NULL, NULL);
				fprintf(stderr, "udp recv'd bytes: %d\n", rcvd);
				fprintf(stderr, "%s\n", msg);	

				if (rcvd == -1)
					return 0;  // return false and sock->error = errno;
				if (rcvd < ws)
					break;	
			}
			return 1;
#endif
	return 0;
}


//This function is not a good idea for select I don't think...
_Bool socket_tcp_recv (Socket *sock, uint8_t *msg, uint32_t *len) {
	int32_t size=0, rcvd=0, ws=sock->bufsz;

	//If it's -1, die.  If it's less than buffer, die
	while (1) {
		size += rcvd = recv(sock->fd, &msg[size], ws, 0);
fprintf(stderr, "rcvd: %d\n", rcvd);
		//Error occurred, free or reset the buffer and die
		if (rcvd == -1) {
			//handle recv() errors...
			sock->err = errno;
			return errsys("recv() error occurred");
		}
		//End of message reached before end of buffer
		else if (rcvd < ws) {	
			*len = size;
			return 1;
		}
	}

	//Only an outside event (like reaching the -- on a 
	//mutipart request) can really signal that we're done.
	*len = size;
	return 1;
}

#if 0
_Bool socket_udp_send (Socket *sock, uint8_t *msg, uint32_t len) {
	int bs=0; 
#if 0
	bs = sendto(sock->fd, msg, msglen, 0, 
		(struct sockaddr *)sock->srvaddrinfo, 
		sizeof(struct sockaddr_in)); 
	fprintf(stderr, "Size of message to be sent: %d\n", msglen);
	fprintf(stderr, "Message to be sent:\n %s\n", msg);
	fprintf(stderr, "(press enter to continue...)\n");
	getchar();	
#endif
	bs = sendto(sock->fd, msg, len, 0, 
		(struct sockaddr *)sock->cliaddrinfo, 
		sizeof(sock->cliaddr)); 


	if (bs == -1)
		return 0;

	fprintf(stderr, "Sent UDP bytes: %d\n", bs);
	fprintf(stderr, "Sent message:   %s\n", msg);
#if 0
	while (1) {
		if (bs==msglen)
			break;

		if (bs == -1)
			return 0;
	
		fprintf(stderr, "Sent UDP bytes: %d\n", bs);
		pprintf("..>");
	}
#endif
	return 1;
}


_Bool socket_tcp_send (Socket *sock, uint8_t *msg, uint32_t len) {
	/* What is going on? */
	int bs = 0;

#if 0
	fprintf(stderr, "Attempting to send message over fd '%d'\n", sock->fd);
	fprintf(stderr, "message contents:\n");
		
	int c = 0;
	// chardump(msg, ws > rcvd ? rcvd : ws);
	for (c=0;c<msglen;c++)
		fprintf(stderr, "'%c' ", msg[c]);
#endif
	while (1) { 
		bs = send(sock->fd, msg, len, 0);
		// usleep(sock->waittime);
		if (bs==len)
			break;
		if (bs == -1)
			return 0;
		// keep trying
	}
	return SUCCESS;
}
#endif

_Bool socket_accept (Socket *sock, Socket *new) {
	/* Clone current socket data */
	if (!memcpy(new, sock, sizeof(Socket))) {
		fprintf(stderr, "Could not copy original parent socket data.\n");
		return 0;
	}

	/* Accept a connection. */	
	if ((new->fd = accept(sock->fd, NULL, NULL)) == -1) 
		return ((sock->err = errno) ? 0 : 0);
		
	/* Set socket description */
	new->_class = 'd';
	return 1;
}

_Bool socket_udp_send (Socket *sock, uint8_t *msg, uint32_t length) {
	//int msglen = (!length) ? strlen(msg) : length;	
	int32_t bs = 0;
#if 0
	bs = sendto(sock->fd, msg, msglen, 0, 
		(struct sockaddr *)sock->srvaddrinfo, 
		sizeof(struct sockaddr_in)); 
	fprintf(stderr, "Size of message to be sent: %d\n", msglen);
	fprintf(stderr, "Message to be sent:\n %s\n", msg);
	fprintf(stderr, "(press enter to continue...)\n");
	getchar();	
#else
	bs = sendto(sock->fd, msg, length, 0, 
		(struct sockaddr *)sock->cliaddrinfo, 
		sizeof(sock->cliaddr)); 
#endif

	if (bs == -1) {
		sock->err = errno;
		return 0;
	}

	fprintf(stderr, "Sent UDP bytes: %d\n", bs);
	fprintf(stderr, "Sent message:   %s\n", msg);
#if 0
	while (1) {
		if (bs==msglen) break;
		if (bs == -1) return 0;
		fprintf(stderr, "Sent UDP bytes: %d\n", bs);
	}
#endif
	return 1;
}


#if 0
_Bool socket_parse_uri (URIData *u, const char *uri) {
	int p, len=strlen(uri);
	//Pack protocol
	if (!memstr(uri, "://", len)) return 0;
	memcpy(u->protocol, 0, (p = memstrat(uri, "://", len)));
	//Check for a port (that's the end of host)
	if (!memchr(&uri[p+3], '/', len - (p+3)))
		if ((p = memchrat(&uri[p+3], ':', len - (p+3)))) {
		//Pack hostname if you can find it
	else {

	}	

	//Pack port
	return 1;
}
#endif

_Bool socket_getaddrinfo (Socket *self) {
	return 0;
}


#if 0
_Bool socket_connect (Socket *self, const char *uri, unsigned int port) {
	int stat;
	char *ps;
	uri_t *h;
	struct addrinfo *r;
	socket_t *s = self->data;
	OBJECT(urn) *urn = NEW(urn)(NULL);

	/* Parse the string */
	if (!urn->parse(urn, (!uri ? s->hostname : uri), port))
		return 0;

	/* Information */
	h = urn->parsed(urn); 
	urn->info(urn);

	/* Debuggable dump */
	fprintf(stderr, "protocol:       %s\n", h->protocol);
	fprintf(stderr, "hostname:       %s\n", h->name);
	fprintf(stderr, "resource:       %s\n", h->resource);
	fprintf(stderr, "username:       %s\n", h->username);
	fprintf(stderr, "password:       %s\n", h->password);
	fprintf(stderr, "port:           %d\n", h->port ? h->port : 0);
	fprintf(stderr, "port (str):     %s\n", h->sport);

	/* Finally, get address information and ... */
	// sock->error = gai_strerror(stat)); 
	if ((stat = getaddrinfo(h->name, h->sport, &s->hints, &s->res)) != 0)
		errexit(1, (char *)gai_strerror(stat));

	/* ... connect! */
	// sock->error = "connection error..."
	// r = s->res;
	if (connect(s->fd, s->res->ai_addr, s->res->ai_addrlen) == -1)
		errexit(1, strerror(errno));	

	/* Set this for later calls */
	self->data->urn = urn;
	return 1;
}
#endif

_Bool socket_tcp_send (Socket *sock, uint8_t *msg, uint32_t length) {
	int32_t bs = 0;

#if 0
	fprintf(stderr, "Attempting to send message over fd '%d'\n", sock->fd);
	fprintf(stderr, "message contents:\n");
		
	int c = 0;
	// chardump(msg, ws > rcvd ? rcvd : ws);
	for (c=0;c<msglen;c++)
		fprintf(stderr, "'%c' ", msg[c]);
#endif
		
	while (1) { 
		bs = send(sock->fd, msg, length, 0);
		// usleep(sock->waittime);
		if (bs==length)
			break;
		if (bs == -1)
			return 0;
	}
	return SUCCESS;
}


/*Compare to uint8_t's*/
_Bool memstr (const void * a, const void *b, int size) {
	int32_t ct=0, len = strlen((const char *)b);
	const uint8_t *aa = (uint8_t *)a;
	const uint8_t *bb = (uint8_t *)b;
	_Bool stop=1;
	while (stop) {
		while ((stop = (ct < (size - len))) && memcmp(aa + ct, bb, 1) != 0) { 
			//fprintf(stderr, "%c", aa[ct]);
			ct++; continue; }
		if (memcmp(aa + ct, bb, len) == 0)
			return 1;	
		ct++;
	}
	return 0;	
}

//Return count of occurences of a character in some block.
int32_t memchrocc (const void *a, const char b, int32_t size) {
	_Bool stop=1;
	int32_t ct=0, occ=-1;
	uint8_t *aa = (uint8_t *)a;
	char bb[1] = { b };
	while (stop) {
		occ++;
		while ((stop = (ct < size)) && memcmp(aa + ct, bb, 1) != 0) ct++;
		ct++;
	}
	return occ;
}


//Return count of occurences of a string in some block.
int32_t memstrocc (const void *a, const void *b, int32_t size) {
	_Bool stop=1;
	int32_t ct=0, occ=0;
	uint8_t *aa = (uint8_t *)a;
	uint8_t *bb = (uint8_t *)b;
	int len     = strlen((char *)b);
	while (stop) {
		while ((stop = (ct < (size - len))) && memcmp(aa + ct, bb, 1) != 0) ct++;
		if (memcmp(aa + ct, bb, len) == 0) occ++;
		ct++;
	}
	return occ;
}


//Where exactly is a substr in memory
int32_t memstrat (const void *a, const void *b, int32_t size) {
	_Bool stop=1;
	int32_t ct=0, occ=0;
	uint8_t *aa = (uint8_t *)a;
	uint8_t *bb = (uint8_t *)b;
	int len     = strlen((char *)b);
	//while (stop = (ct < (size - len)) && memcmp(aa + ct, bb, len) != 0) ct++; 
	while (stop) {
		while ((stop = (ct < (size - len))) && memcmp(aa + ct, bb, 1) != 0) ct++;
		if (memcmp(aa + ct, bb, len) == 0)
			return ct; 
		ct++;
	}
	return -1;
}

//Where exactly is a substr in memory
int32_t memchrat (const void *a, const char b, int32_t size) {
	_Bool stop=1;
	int32_t ct=0, occ=0;
	uint8_t *aa = (uint8_t *)a;
	//uint8_t *bb = (uint8_t *)b;
	char bb[1] = { b };
	//while (stop = (ct < (size - len)) && memcmp(aa + ct, bb, len) != 0) ct++; 
	while ((stop = (ct < size)) && memcmp(aa + ct, bb, 1) != 0) ct++;
	return (ct == size) ? -1 : ct;
}


//Finds the first occurence of one char, Keep running until no tokens are found in range...
int32_t memtok (const void *a, const uint8_t *tokens, int32_t sz, int32_t tsz) {
	uint8_t *tk = (uint8_t *)tokens;
	int32_t p=-1,n;
	
	for (int i=0; i<tsz; i++)
	#if 1
		p = ((p > (n = memchrat(a, tokens[i], sz)) && n > -1) || p == -1) ? n : p;
	#else
	{
		p = ((p > (n = memchrat(a, tokens[i], sz)) && n > -1) || p == -1) ? n : p;
		fprintf(stderr, "found char %d at %d\n", tokens[i], memchrat(a, tokens[i], sz));
		nmprintf("p is", p);
	}
	#endif
	
	return p;
}


#ifndef NOOPTIONS
/*A usage function.*/
void opt_usage (Option *opts, const char *msg, int status) {
	if (msg)
		fprintf(stderr, "%s\n", msg);
	while (!opts->sentinel) { 
		if (opts->type != 'n' && opts->type != 'c' && opts->type != 's') 
			fprintf(stderr, "%-2s, %-20s %s\n", opts->sht ? opts->sht : " " , opts->lng, opts->description);
		else {
			char argn[1024]; memset(&argn, 0, 1024);
			sprintf(argn, "%s <arg>", opts->lng);
			fprintf(stderr, "%-2s, %-20s %s\n", opts->sht ? opts->sht : " " , argn, opts->description);
		}
		opts++;
	}
	exit(status);
}


static void opt_dump (Option *opts) {
	/*Just for testing options*/
	Option *all=opts;
	while (!all->sentinel) {
		fprintf(stderr, "%20s: %s\n", all->lng, all->set ? "true" : "false");
		all++;
	}
}


_Bool opt_set (Option *opts, char *flag) {
	Option *o = opts;
	while (!o->sentinel) {
		if (strcmp(o->lng, flag) == 0 && o->set)
			return 1; 
		o++;
	}
	return 0;
}


Value opt_get (Option *opts, char *flag) {
	Option *o = opts;
	while (!o->sentinel) {
		if (strcmp(o->lng, flag) == 0)
			return o->v; 
		o++;
	}
	return o->v; /*Should be the last value, and it should be blank*/
}

static _Bool opt_set_value (char **av, Value *v, char type, char *err) {
	/*Get original flag and other things.*/
	char flag[64]={0}; 
	snprintf(flag, 63, "%s", *av);
	av++;

	fprintf(stderr, "%s %s\n", flag, *av);

	/*Catch what may be a flag*/
	if (!*av || (strlen(*av) > 1 && *av[0] == '-' && *av[1] == '-'))
		return (snprintf(err, 1023, "Expected argument after flag %s\n", flag) && 0); 
	
	/*Evaluate the three different types*/
	if (type == 'c')
		v->c = *av[0];	
	else if (type == 's') {
		_Bool isstr=0;
		for (int i=0;i<strlen(*av); i++) { 
			/*We can safely assume this is an ascii string, if this passes*/
			if ((*av[i] > 32 && *av[i] < 48) || (*av[i] > 57 && *av[i] < 127)) {
				isstr = 1;
				break;
			}
		}

		if (!isstr)
			return (snprintf(err, 1023, "Expected string after flag %s\n", flag) && 0);
		v->s = *av;	
	}
	else if (type == 'n') {
		char *a = *av; /*Crashes for some reason if I just use dereference*/
		for (int i=0;i<strlen(a); i++)
			if ((int)a[i] < 48 || (int)a[i] > 57) /*Not a number check*/
				return (snprintf(err, 1023, "Expected number after flag %s\n", flag) && 0);
		v->n = atoi(*av);	
	}

	return 1; 
}


_Bool opt_eval (Option *opts, int argc, char **av) {
	/*Evaulate options*/
	char buf[1024]={0};
	while (*av) {
		Option *o=opts;
		while (!o->sentinel) {
			//Find option, set boolean, and run a validator callback
			if ((o->sht && strcmp(*av, o->sht) == 0) || (o->lng && strcmp(*av, o->lng) == 0)) {
				o->set=1;
				if (o->callback) {
					if (!o->callback(++av, &o->v, buf))
						return 0;
				}
				else if (o->type == 'n' || o->type == 's' || o->type == 'c') {
					if (!opt_set_value(av, &o->v, o->type, buf)) {
						errprintf(buf);
						return 0;
					}
				}
			}
			o++;
		}
		av++;
	}
	return 1;
}
#endif


#if 0
#ifndef NOSOCKET
unsigned int socket_connect (SOCKET *self, const char *uri, unsigned int port)
{
	int stat;
	char *ps;
	uri_t *h;
	struct addrinfo *r;
	socket_t *s = self->data;
	OBJECT(urn) *urn = NEW(urn)(NULL);

	/* Parse the string */
	if (!urn->parse(urn, (!uri ? s->hostname : uri), port))
		return 0;

	/* Information */
	h = urn->parsed(urn); 
	urn->info(urn);

	/* Debuggable dump */
	fprintf(stderr, "protocol:       %s\n", h->protocol);
	fprintf(stderr, "hostname:       %s\n", h->name);
	fprintf(stderr, "resource:       %s\n", h->resource);
	fprintf(stderr, "username:       %s\n", h->username);
	fprintf(stderr, "password:       %s\n", h->password);
	fprintf(stderr, "port:           %d\n", h->port ? h->port : 0);
	fprintf(stderr, "port (str):     %s\n", h->sport);

	/* Finally, get address information and ... */
	// sock->error = gai_strerror(stat)); 
	if ((stat = getaddrinfo(h->name, h->sport, &s->hints, &s->res)) != 0)
		errexit(1, (char *)gai_strerror(stat));

	/* ... connect! */
	// sock->error = "connection error..."
	// r = s->res;
	if (connect(s->fd, s->res->ai_addr, s->res->ai_addrlen) == -1)
		errexit(1, strerror(errno));	

	/* Set this for later calls */
	self->data->urn = urn;
	return 1;
}


#endif
#endif
