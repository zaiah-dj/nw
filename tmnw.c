#include "tmnw.h"
#include <poll.h>

#ifdef NW_CATCH_SIGNAL
 #include <signal.h>
#endif

#ifndef SINGLE_H
 #ifdef NW_DEBUG
  #define SHOWDATA(...) do { \
   fprintf(stderr, "%-30s [ %s %d ] -> ", __func__, __FILE__, __LINE__); \
   fprintf( stderr, __VA_ARGS__ ); \
   fprintf( stderr, "\n"); } while (0)
 #else
  #define SHOWDATA( ... )
 #endif

enum 
{
  ERR_NONE,
 #ifndef BUFF_H
  /*Buffer*/
  ERR_BUFF_ALLOC_FAILURE,
  ERR_BUFF_REALLOC_FAILURE,
  ERR_BUFF_OUT_OF_SPACE,
 #endif
	ERR_ERR_INDEX_MAX
};

static const char *__errors[] = 
{
  [ERR_NONE] = "No errors",
 #ifndef BUFF_H
  /*Buffer*/
  [ERR_BUFF_ALLOC_FAILURE] = "Buffer allocation failure",
  [ERR_BUFF_REALLOC_FAILURE] = "Buffer reallocation failure",
  [ERR_BUFF_OUT_OF_SPACE] = "Fixed buffer is out of space.",
 #endif
	[ERR_ERR_INDEX_MAX]        = "No errors",
};

 #ifndef BUFF_H
//Initialize a buffer
Buffer *bf_init (Buffer *b, uint8_t *mem, int size) 
{
	memset( b, 0, sizeof (Buffer) );
	if ( mem )
	{
		b->fixed = 1;
		b->size  = size;	
		b->buffer = mem; 
		b->written = 0;
	}
	else {
		b->written = 0;
		b->size = 0;
		b->fixed = 0;
		if ( !(b->buffer = malloc( size )) )
			return NULL;
	}

	return b;
}


//This will help "clean" things
void bf_softinit (Buffer *b)
{
	memset( b, 0, sizeof (Buffer) );
	b->buffer = NULL;
}


//Set the written size if a buffer has data already
void bf_setwsize (Buffer *b, int size) 
{
	b->size = size;
}



//Write data to buffer
int bf_append (Buffer *b, uint8_t *s, int size) 
{
	if ( !b->fixed ) {
		if ((size + b->written) > b->size) 
		{
			uint8_t *c = NULL;
			if ( !(c = realloc( b->buffer, size + b->size )) )
			{
				b->error = ERR_BUFF_REALLOC_FAILURE;
				return 0;
			}
			b->size    = size + b->size;
			b->buffer  = c;		
		}
	}
	else {
		if ((size + b->written) > b->size) 
		{
			b->error = ERR_BUFF_OUT_OF_SPACE;
			return 0;	
		}	
	}

	memcpy( &b->buffer[ b->written ], s, size );
	b->written += size;
	return 1;
}



//Move data in a buffer
int bf_prepend (Buffer *b, uint8_t *s, int size)
{
	//If it's malloc'd, allocate the new space and move the memory
	if ( !b->fixed ) {
		uint8_t *c = NULL;
		if ( !(c = realloc( b->buffer, size + b->size )) )
		{
			b->error = ERR_BUFF_REALLOC_FAILURE;
			return 0;
		}
		b->size    = size + b->size;
		b->buffer  = c;		
		memmove( &b->buffer[size], &b->buffer[0], b->written );
		memcpy( &b->buffer[ 0 ], s, size );
		b->written += size;
	}

	//If it's not, check that the buffer has enough and then move
	return 1;
}


//Get currently written amount
int bf_written (Buffer *b) {
	return (b) ? b->written : 0;
}



//Get the unsigned char data
uint8_t *bf_data (Buffer *b) {
	return (b) ? b->buffer : 0;
}


//Write out errors
const char *bf_err (Buffer *b) {
	return __errors[b->error];
}

//Destroy a buffer's data
void bf_free (Buffer *b) 
{
	if (( b->buffer ) && (!b->fixed)) 
	{
		free( b->buffer ); 
		b->buffer  = NULL;
		b->size    = 0;
		b->written = 0;
		b->error   = 0;
		b->fixed   = 0;
	}
}
 #endif

 #ifndef SOCKET_H
//Get the address information of a socket.
void socket_addrinfo (Socket *sock)
{
	//fprintf(stderr, "%s\n", "Getting address information for socket.");

	struct addrinfo *peek; 
	struct sockaddr_in *ipv4;
	struct sockaddr_in6 *ipv6;
	int status = getaddrinfo(sock->hostname, sock->portstr, &sock->hints, &sock->res);

	if (status != 0) 
	{
		//err(1, "Could not get address information for this socket.");
		return;
	}

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
}



//initialize a socket...
_Bool socket_open (Socket *sock) 
{
	//All of this can be done with a macro
	sock->addrsize = sizeof(struct sockaddr);
	sock->bufsz = !sock->bufsz ? 1024 : sock->bufsz;
	sock->opened = 0;
	sock->backlog = 500;
	sock->waittime = 5000;  // 3000 microseconds

	if (!sock->proto || !strcmp(sock->proto, "TCP") || !strcmp(sock->proto, "tcp"))
	{
		sock->conntype = SOCK_STREAM;
		sock->domain   = PF_INET;
		sock->protocol = IPPROTO_TCP;
		sock->hostname = (sock->server) ? ((!sock->hostname) ? "localhost" : sock->hostname) : NULL;
		sock->_class = (sock->server) ? 's' : 'c';
	}
	else if (!strcmp(sock->proto, "udp") ||!strcmp(sock->proto, "UDP"))
	{
		sock->conntype = SOCK_DGRAM;
		sock->domain   = PF_INET;
		sock->protocol = IPPROTO_UDP;
		sock->hostname = (sock->server) ? ((!sock->hostname) ? "localhost" : sock->hostname) : NULL;
		sock->_class = (sock->server) ? 's' : 'c';
	}

	/* Check port number (clients are zero until a request is made) */
	if (!sock->port || sock->port < 0 || sock->port > 65536) 
	{
	//fprintf( stderr, "Invalid port specified." );
		return 1;
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
		return (0, "Could not reopen socket.");
	}
	return 1;
}



//Bind to a socket
_Bool socket_bind (Socket *sock) {
	//set errno
	return (bind(sock->fd, (struct sockaddr *)sock->srvaddrinfo, sizeof(struct sockaddr_in)) != -1);
//		return errsys("Could not bind to socket.");
}



//Listen for connections over a socket.
_Bool socket_listen (Socket *sock)
{
	//set errno
	return (listen(sock->fd, sock->backlog) != -1);
		//return errsys("Could not listen out on socket.");
}


//Opens a non blocking socket.
//This function is not a good idea for select I don't think...
_Bool socket_tcp_recv (Socket *sock, uint8_t *msg, int *len) 
{
	int t = 0, 
      r = 0, 
      w = ( *len > 0 ) ? *len : 64;

	//If it's -1, die.  If it's less than buffer, die
	while (1) 
	{
		//Error occurred, free or reset the buffer and die
		if ( (r = read(sock->fd, &msg[ t ], w )) == -1 )
		{
			//handle recv() errors...
			sock->err = errno;
			return 0;
		}

		//...
		if ( !r )
		{
			fprintf( stderr, "socket_tcp_recv should be done...\n" );
			break;
		}

		t += r;
	}

	*len = t;
	return 1;
}


//Accept connections
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
 #endif
#endif


/*Choose dynamic buffers by default*/
#ifndef NW_BUFF_FIXED
 #define NW_BUFF_DYNAMIC
#endif

//#define NW_QUEUE_WRITES
//#undef NW_FOLLOW

#ifdef NW_QUEUE_WRITES
 #define NW_QUEUE_WRITE_DIRNAME "local"
#endif

#ifdef NW_VERBOSE 
 #define nw_log(...) \
	fprintf(stderr, __VA_ARGS__);
 #define nw_error_log(map, code) \
	write(2, map[code], strlen(map[code]))
#else
 #define nw_log(...)
 #define nw_error_log(map, code)
#endif

//Deduce the stage of the request
#define GETSTAGE(i) \
	(i == NW_AT_WRITE) ? "write" : (i == NW_AT_READ ) ? "read" : (i == NW_AT_PROC) ? "proc" : "completed"

/*Handle errors via the nw_error_map function pointer table.*/
#define handle(ERRCODE) { \
	nw_error_log(nw_error_map, ERRCODE); \
	if (!(&s->errors[ERRCODE])->exe(r, s->global_ud, (&s->errors[ERRCODE])->err)) { \
		switch ((&s->errors[ERRCODE])->action) { \
			case NW_CONTINUE: \
				continue; \
			case NW_NOTHING: \
				0; break; \
			case NW_RETURN: \
				return (&s->errors[ERRCODE])->status || 0; \
			case NW_EXIT: \
				exit((&s->errors[ERRCODE])->status || 0); \
		} \
	} \
}

/*Handle errors via the rwp_error_map function pointer table.*/
#define uhandle(CODE) \
	if (NW_CALL( ( r->status = (&s->runners[CODE])->exe(r, s->global_ud, (&s->runners[CODE])->err) ) )) { \
		/*Success*/ \
		nw_log("%s successful at %s %d.\n", #CODE, __FILE__, __LINE__); \
		switch ((&s->runners[CODE])->action) { \
			case NW_CONTINUE: \
				continue; \
			case NW_RETURN: \
				return (&s->runners[CODE])->status || 0; \
			case NW_EXIT: \
				exit((&s->runners[CODE])->status || 0); \
			case NW_NOTHING: \
				0; \
		} \
	} \
	else { \
		nw_log("%s failed at %s %d.\n", #CODE, __FILE__, __LINE__); \
		switch ((&s->runners[CODE])->action) { \
			case NW_CONTINUE: \
				continue; \
			case NW_RETURN: \
				return (&s->runners[CODE])->status || 0; \
			case NW_EXIT: \
				exit((&s->runners[CODE])->status || 0); \
			case NW_NOTHING: \
				0; \
		} \
	}

/*Print a message as we move through branches within the program flow*/
#ifdef NW_VERBOSE
 #define NW_LOG(c) \
	(c) || (fprintf(stderr, "%s: %d - %s\n", __FILE__, __LINE__, #c)? 0: 0)
#else
 #define NW_CALL(c) \
	c
#endif

/*Reset read event*/
#define nw_reset_read() \
	r->client->events = POLLRDNORM

/*Reset write event*/
#define nw_reset_write() \
	r->client->events = POLLWRNORM

/*Get fd without worrying about pollfd structure*/
#define nw_get_fd() \
	r->client->fd

#ifdef NW_VERBOSE
/*Dump the selector*/
void print_selector (Selector *s) 
{
	fprintf(stderr, "max_events: %d\n", s->max_events);
	fprintf(stderr, "rarr:       %p\n", (void *)s->rarr);
	fprintf(stderr, "userdata:   %p\n", (void *)s->global_ud);
	fprintf(stderr, "parent:     %p\n", (void *)s->parent);
	fprintf(stderr, "clients:    %p\n", (void *)s->clients);
	fprintf(stderr, "ex:         %p\n", (void *)s->errors);
	fprintf(stderr, "rwp:        %p\n", (void *)s->runners);
	fprintf(stderr, "recv_retry: %d\n", s->recv_retry);
	fprintf(stderr, "send_retry: %d\n", s->send_retry);
}


/*Dump the selector*/
void print_recvr (Recvr *r) 
{
	fprintf(stderr, "child:       %p\n", (void *)&r->child);
	fprintf(stderr, "recvd:       %d\n", r->recvd);
	fprintf(stderr, "sent:        %d\n", r->sent);
	fprintf(stderr, "stage:       %d\n", r->stage);
 #if 0
	fprintf(stderr, "request_fd:  %d\n", r->request_fd);
	fprintf(stderr, "response_fd: %d\n", r->response_fd);
 #endif
 #ifdef NW_BUFF_FIXED
	fprintf(stderr, "request:     %p\n", (void *)r->request);
	fprintf(stderr, "response:    %p\n", (void *)r->response);
 #endif
 #if 0
	fprintf(stderr, "request:     %p\n", (void *)r->_request1->buffer);
	fprintf(stderr, "response:    %p\n", (void *)r->_response1->buffer);
 #endif
 #ifndef NW_DISABLE_LOCAL_USERDATA
	fprintf(stderr, "userdata:    %p\n", r->userdata);
 #endif
	fprintf(stderr, "pollfd:      %p\n", (void *)&r->client);
	fprintf(stderr, "pollfd.fd:   %d\n", r->client->fd);
	/*fprintf(stderr, "pollfd.events:   %d\n", r->client.fd);
	fprintf(stderr, "pollfd.revents:   %d\n", r->client.fd);*/
	fprintf(stderr, "recv_retry:  %d\n", r->recv_retry);
	fprintf(stderr, "send_retry:  %d\n", r->send_retry);
}
#endif


/*Static list of error codes in text*/
const char *nw_error_map[] = 
{
	[ERR_POLL_INITIAL_ALLOCATOR]        = "File allocation failure.\n",
	[ERR_POLL_TOO_MANY_FILES]           = "Attempt to open too many files.\n",
	[ERR_POLL_RECVD_SIGNAL ]            = "Received signal interrupting accept().\n",
	[ERR_SPAWN_ACCEPT]                  = "Accept failure.\n",
	[ERR_SPAWN_NON_BLOCK_SET]           = "Could not make child socket non-blocking.\n",
	[ERR_SPAWN_MAX_CLIENTS]             = "Server has reached maximum number of clients.\n",
	[ERR_READ_CONN_RESET]               = "Connection reset by peer.\n",
	[ERR_READ_EGAIN]                    = "No data received, please try reading again.\n",
	[ERR_READ_EBADF]                    = "No file to receive data from. " \
                                        "Peer probably closed connection.\n",
	[ERR_READ_EFAULT]                   = "Server out of space for reading messages.\n",
	[ERR_READ_EINVAL]                   = "Read of socket is impossible due to " \
                                        "misalignment or use of O_DIRECT.\n",
	[ERR_READ_EINTR]                    = "Fatal signal encountered.\n",
	[ERR_READ_EISDIR]                   = "File descriptor supplied belongs to directory.\n",
	[ERR_READ_CONN_CLOSED_BY_PEER]      = "Connection closed by peer\n",
	[ERR_READ_BELOW_THRESHOLD]          = "Data read was below minimum threshold.\n",
	[ERR_READ_MAX_READ_RETRY_REACHED]   = "Maximum read retry limit reached for " 
                                        "this client\n",
	[ERR_WRITE_CONN_RESET]              = "Connection was reset before writing packet " \
                                        "could resume.\n",
	[ERR_WRITE_EGAIN]                   = "No data written, please try writing again.\n",
	[ERR_OUT_OF_MEMORY]                 = "Out of memory.\n",
	[ERR_REQUEST_TOO_LARGE]             = "The request made was too large.\n",
	[ERR_WRITE_EBADF]                   = "No file to write data to. " \
                                        "Peer probably closed connection.\n",
	[ERR_WRITE_EFAULT]                  = "Attempt to write message too large for buffer.\n",
	[ERR_WRITE_EFBIG]                   = "Attempt to write message too large for buffer.\n",
	[ERR_WRITE_EDQUOT]                  = "File quota of server has been reached.\n",
	[ERR_WRITE_EINVAL]                  = "ERR_WRITE_EINVAL...\n",
	[ERR_WRITE_EIO]                     = "ERR_WRITE_EIO...\n",
	[ERR_WRITE_ENOSPC]                  = "Kernel buffer exhausted.\n",
	[ERR_WRITE_EINTR]                   = "Fatal signal encountered.\n",
	[ERR_WRITE_EPIPE]                   = "Fatal signal encountered.\n",
	[ERR_WRITE_EPERM]                   = "Fatal: Permission denied when attempting to " \
                                        "write to socket.\n",
	[ERR_WRITE_EDESTADDREQ]             = "...\n", /*UDP error*/
	[ERR_WRITE_CONN_CLOSED_BY_PEER]     = "WRITE_CONN_CLOSED_BY_PEER...\n",
	[ERR_WRITE_BELOW_THRESHOLD]         = "Data write was below minimum threshold.\n",
	[ERR_WRITE_MAX_WRITE_RETRY_REACHED] = "Maximum write retry limit reached " \
                                        "for this client\n", 
};


/*Static list of loop process codes in text*/
const char *runner_error_map[] = {
	[NW_AT_READ]     = "Read handler failed:",
	[NW_AT_PROC]     = "Processor handler failed:",
	[NW_AT_WRITE]    = "Write handler failed:",
	[NW_AT_ACCEPT]   = "Accept handler failed:"
};


//Dummy's forward declaration
static _Bool dummy (Recvr *r, void *ud, char *err);

/*Default executor table for those who don't want to be bothered*/
Executor _nw_errors[ERR_END_OF_CHAIN + 1] = {
	[ERR_POLL_INITIAL_ALLOCATOR]        = { dummy          , NW_NOTHING  },
	[ERR_POLL_TOO_MANY_FILES]           = { dummy          , NW_NOTHING  },
	[ERR_POLL_RECVD_SIGNAL ]            = { dummy          , NW_RETURN   },
	[ERR_SPAWN_ACCEPT]                  = { dummy          , NW_CONTINUE },
	[ERR_SPAWN_NON_BLOCK_SET]           = { dummy          , NW_CONTINUE },
	[ERR_SPAWN_MAX_CLIENTS]             = { dummy          , NW_CONTINUE },
	[ERR_READ_CONN_RESET]               = { nw_reset_fd    , NW_CONTINUE },
	[ERR_READ_EGAIN]                    = { dummy          , NW_CONTINUE },
	[ERR_READ_EBADF]                    = { nw_reset_fd    , NW_CONTINUE },
	[ERR_READ_EFAULT]                   = { nw_reset_fd    , NW_CONTINUE },
	[ERR_READ_EINVAL]                   = { nw_reset_fd    , NW_CONTINUE },
	[ERR_READ_EINTR]                    = { nw_reset_fd    , NW_CONTINUE },
	[ERR_READ_CONN_CLOSED_BY_PEER]      = { nw_reset_fd    , NW_CONTINUE },
	[ERR_READ_BELOW_THRESHOLD]          = { dummy          , NW_CONTINUE },
	[ERR_READ_MAX_READ_RETRY_REACHED]   = { dummy          , NW_CONTINUE },
	[ERR_WRITE_CONN_RESET]              = { nw_reset_fd    , NW_CONTINUE },
	[ERR_WRITE_EGAIN]                   = { reset_write_fd , NW_CONTINUE },
	[ERR_OUT_OF_MEMORY]                 = { nw_reset_fd    , NW_CONTINUE },
	[ERR_REQUEST_TOO_LARGE]             = { nw_reset_fd    , NW_CONTINUE },
	[ERR_WRITE_EBADF]                   = { nw_reset_fd    , NW_CONTINUE },
	[ERR_WRITE_EFAULT]                  = { reset_buffer   , NW_CONTINUE },
	[ERR_WRITE_EFBIG]                   = { reset_buffer   , NW_CONTINUE },
	[ERR_WRITE_EDQUOT]                  = { nw_reset_fd    , NW_CONTINUE },
	[ERR_WRITE_EINVAL]                  = { nw_reset_fd    , NW_CONTINUE },
	[ERR_WRITE_EIO]                     = { nw_reset_fd    , NW_CONTINUE },
	[ERR_WRITE_ENOSPC]                  = { nw_reset_fd    , NW_CONTINUE },
	[ERR_WRITE_EINTR]                   = { nw_reset_fd    , NW_CONTINUE },
	[ERR_WRITE_EPIPE]                   = { nw_reset_fd    , NW_CONTINUE },
	[ERR_WRITE_EPERM]                   = { nw_reset_fd    , NW_CONTINUE },
	[ERR_WRITE_EDESTADDREQ]             = { nw_reset_fd    , NW_CONTINUE },
	[ERR_WRITE_CONN_CLOSED_BY_PEER]     = { nw_reset_fd    , NW_CONTINUE },
	[ERR_WRITE_BELOW_THRESHOLD]         = { nw_reset_fd    , NW_CONTINUE },
	[ERR_WRITE_MAX_WRITE_RETRY_REACHED] = { nw_reset_fd    , NW_CONTINUE },
	[ERR_TIMEOUT_CONN]                  = { nw_reset_fd    , NW_CONTINUE },
};

/*A default processor for those who don't set all of them*/
static Executor _nw_runners[] = {
	[NW_AT_READ]    = { dummy , NW_NOTHING },
	[NW_AT_PROC]    = { dummy , NW_NOTHING },
	[NW_AT_WRITE]   = { dummy , NW_NOTHING },
	[NW_AT_ACCEPT]  = { dummy , NW_NOTHING },
	[NW_COMPLETED]  = { dummy , NW_NOTHING },
}; 

/*A default selector for those who don't want to be bothered*/
static Selector _default = {
	.max_events  = 1000,       /*Default 1K events in queue    */
	.rarr        = NULL,       /*Initialize this to same number  
                              as max_events                 */
	.global_ud   = NULL,       /*There is no userdata if the 
                              user does not elect to fill it*/
	.parent      = NULL,       /*User needs to do this for now */
	.clients     = NULL,       /*This will always equal the 
                              max_clients for now           */
	.errors      = _nw_errors,  /*Use the default handlers above*/
	.runners     = _nw_runners, /*Use the default handlers above*/
	.recv_retry  = 5,          /*Only try to receive 5 times   */
	.send_retry  = 5,         /*Only try to send 5 times      */
};


/*Close fd and reset all memory and "trackers" for this connection*/
static void reset_recvr (Recvr *r) 
{
	//SHOWDATA( "resetting recvr structure and freeing scratch space... " );
	memset(&r->child, 0, sizeof(Socket));
#ifdef NW_BUFF_FIXED
	memset(&r->request_, 0, NW_MAX_BUFFER_SIZE); 
	memset(&r->response_, 0, NW_MAX_BUFFER_SIZE);	
#else
	bf_free( &r->_request );
	bf_free( &r->_response );
#endif
	memset(&r->start, 0, sizeof(struct timespec));
	memset(&r->end, 0, sizeof(struct timespec));
	r->rb = 0, 
	r->sb = 0, 
	r->recvd = 0, 
	r->sent = 0, 
	r->stage = 0;
	r->recv_retry = 0,
	r->send_retry = 0;
}



//Read from a socket
int nw_read (Recvr *r) 
{
	uint8_t etc[ NW_MAX_BUFFER_SIZE ] = { 0 };
	r->rb = read(r->client->fd, etc, NW_MAX_BUFFER_SIZE - 1);

	if ( !r->rb )
		return ERR_READ_CONN_CLOSED_BY_PEER;
	else if ( r->rb == -1 ) 
	{
		switch (errno) {
			case ECONNRESET:
				return ERR_READ_CONN_RESET;
			case EAGAIN/*Try reading again in a minute...*/:
				r->client->events = POLLRDNORM;
				return ERR_READ_EGAIN;
			case EBADF:
				reset_recvr(r);
				return ERR_READ_EBADF;
			case EFAULT:
				reset_recvr(r);
				return ERR_READ_EFAULT;
			case EINVAL:
				reset_recvr(r);
				return ERR_READ_EINVAL;
			case EINTR:
				reset_recvr(r);
				return ERR_READ_EINTR;
			case EISDIR:
				reset_recvr(r);
				return ERR_READ_EISDIR;
			default:
				break;
		}
	}

	//Now depending on model, this will or won't shut down on you...
	if ( !bf_append( &r->_request, etc, r->rb ) ) 
	{
		if ( r->_request.error == ERR_BUFF_REALLOC_FAILURE ) {
			return ERR_OUT_OF_MEMORY;
		}
		else if ( r->_request.error == ERR_BUFF_OUT_OF_SPACE ) {
			return ERR_REQUEST_TOO_LARGE;
		}
	}

	r->recvd += r->rb;
	r->request = (&r->_request)->buffer;
	return 0;
}



//Write to socket - always assumes the message is ready (and it should be...)
_Bool nw_write (Recvr *r) 
{
	//Check that you're not writing to uninitialized memory.
	SHOWDATA( "file             %d\n", r->client->fd );	
	SHOWDATA( "actual length    %d\n", bf_written( &r->_response ));

	int len = bf_written( &r->_response );
#ifdef NW_QUEUE_WRITES
	//This has the same failures, we don't really care...
	int fd = 0;
	static int fn = 0;
	struct stat sb;
	uint8_t *wB = bf_data( &r->_response );
	char fbuf[64] = {0};

	//?
	snprintf( fbuf, 63, "%s/%d", NW_QUEUE_WRITE_DIRNAME, fn++ );
	mkdir( NW_QUEUE_WRITE_DIRNAME, S_IRWXU );
	if ( open( fbuf, O_CREAT | O_RDWR, S_IRWXU ) == -1 )
		return 1;
	if ( (r->sb = write( fd, wB, len - r->sent)) == -1 )
		return 1;
	if ( close( fd ) == -1 )
		return 1;
	SHOWDATA( "%-5d bytes written to file %s\n", r->sb, fbuf);

#else
	uint8_t *wB = &r->_response.buffer[r->sent];
	r->sb = write(r->client->fd, wB, len - r->sent);

	SHOWDATA( "write() syscall returned %d\n", r->sb );

	if ( !r->sb ) 
	{
		reset_recvr(r);
		return ERR_WRITE_CONN_CLOSED_BY_PEER;
	}
	else if ( r->sb == -1 ) 
	{
		switch (errno) {
			case EAGAIN: /*This shoudn't happen, but if it does...*/
				return ERR_WRITE_EGAIN;
			case EBADF:      /*Peer closed early, why is it here?*/
				reset_recvr(r);
				return ERR_WRITE_EBADF;
			case EFAULT:   /*I don't have any more space to write*/
				reset_recvr(r);
				return ERR_WRITE_EFAULT;
			case EFBIG:             /*I can't send this much data*/
				reset_recvr(r);
				return ERR_WRITE_EFBIG;
			/*In these cases, I have little choice but to close the peer*/
			case EDQUOT: 
				reset_recvr(r);
				return ERR_WRITE_EDQUOT;
			case EINVAL:
				reset_recvr(r);
				return ERR_WRITE_EINVAL;
			case EIO:
				reset_recvr(r);
				return ERR_WRITE_EIO;
			case ENOSPC:
				reset_recvr(r);
				return ERR_WRITE_ENOSPC;
			case EINTR:
				reset_recvr(r);
				return ERR_WRITE_EINTR;
			case EPIPE:
				reset_recvr(r);
				return ERR_WRITE_EPIPE;
			case EPERM: /*I can't write, b/c another process disallowed it*/
				reset_recvr(r);
				return ERR_WRITE_EPERM;
			/*case EDESTADDREQ: //TODO: GCC complains.  Why?
				handle(ERR_WRITE_EDESTADDREQ); 
				close_fds(i); */
			default:
				break;		
		}
	}

	r->sent += r->sb;
	//This needs to repeat if it wasn't done...
#endif
	return 0;
}



/*Reset file descriptor for reading*/
_Bool reset_read_fd (Recvr *r, void *ud, char *err) 
{
	r->client->events = POLLRDNORM;
	return 1;
}



/*Reset file descriptor for writing */
_Bool reset_write_fd (Recvr *r, void *ud, char *err) 
{
	r->client->events = POLLWRNORM;
	return 1;
}



/*Clear buffer*/
_Bool reset_buffer (Recvr *r, void *ud, char *err) 
{
	return (memset(r->response, 0, NW_MAX_BUFFER_SIZE) != NULL);
}



/*A dummy function for the purposes of this tutorial*/
static _Bool dummy (Recvr *r, void *ud, char *err) 
{
	return 0;
}



/*Close*/
_Bool nw_close_fd (Recvr *r, void *ud, char *err) 
{
	return ((close(r->client->fd) != -1) && (r->client->fd = -1));
}



/*...*/
_Bool nw_reset_fd (Recvr *r, void *ud, char *err) 
{
	fprintf(stderr, "r->client is:   %p\n",  (void *)r->client);
	fprintf(stderr, "r->client->fd:  %d\n",  r->client->fd);
	reset_recvr(r);
	if (close(r->client->fd) == -1) {
		return 0;
	}
	r->client->fd = -1;
	return 1;
}



//Initialize the selectors
_Bool initialize_selector (Selector *s, Socket *sock) 
{
	//Always set up the parent on behalf of the user
	s->parent = sock;

	//Always set the parent to non-blocking
	if (NW_CALL( fcntl(s->parent->fd, F_SETFD, O_NONBLOCK) == -1 )) 
		return nw_err(0, "fcntl error: %s\n", strerror(errno)); 

	//Allocate needed pollfd structures here
	if (NW_CALL( !(s->clients = calloc(s->max_events, sizeof(struct pollfd))) ))
		return nw_err(0, "Failed to allocate poll structures.\n");

	//Allocate needed recvr structures as well.
	if (NW_CALL( !(s->rarr = calloc(s->max_events, sizeof(Recvr))) ))
		return nw_err(0, "Failed to allocate space for network receiver structures.\n");

 #ifndef NW_DISABLE_LOCAL_USERDATA
	//Allocate any local userdata
	if (s->lsize && s->lsize < 0) 
	{
		return nw_err(0, "Local userdata size cannot be negative.\n");
	}
	//TODO: Can't use static userdata yet.
	else if (s->lsize) 
	{
		if (NW_CALL( !(s->local_ud = malloc(s->lsize * s->max_events)) )) {
			return nw_err(0, "Failed to allocate space for local userdata.\n");
		}
		memset(s->local_ud, 0, s->lsize * s->max_events);
		s->tsize = (s->lsize * s->max_events);
	}
 #endif

	//Initialize all fds in pollfd to -1, and allocate any local userdata
	for (int i=0; i<s->max_events; i++) 
	{
		//Initialize local userdata
	 #ifndef NW_DISABLE_LOCAL_USERDATA
		char *p = (!s->lsize) ? NULL : (char *)s->local_ud;
		//memset(&p[u], ++a, s->lsize);
		(&s->rarr[i])->userdata = (!s->lsize) ? NULL : (void *)&p[i * s->lsize];
   #endif

		//Initialize file descriptors to an agreed upon "uninitialized" value (-1 in this case)
		(&s->clients[i])->fd = -1; 
		(&s->rarr[i])->socket_fd = &(&s->clients[i])->fd ;
	
	}

	/*Ready to read regular data over TCP or UDP*/ 
	s->clients[0].fd     = s->parent->fd;
	s->clients[0].events = POLLRDNORM;
 
 #ifdef NW_CATCH_SIGNAL
	/*Free when trapping*/
	signal(SIGHUP, set_sighup);
 #endif

#ifdef NW_VERBOSE
	/*Dump the selector*/
	fprintf(stderr, "Currently allocated %5.2f MB of heap.\n", 
		((float)((
			((sizeof(struct pollfd) + sizeof(Recvr)) * s->max_events)
	 #ifndef NW_DISABLE_LOCAL_USERDATA 
		   + (float)s->tsize
	 #endif
		) / (float)1024) / 1024)
	);
	fprintf(stderr, "Max request size is:  %d bytes.\n", NW_MAX_BUFFER_SIZE);
	fprintf(stderr, "Max response size is: %d bytes.\n", NW_MAX_BUFFER_SIZE);
 #ifndef NW_DISABLE_LOCAL_USERDATA 
	fprintf(stderr, "Userdata size is:     %d bytes.\n", s->tsize);
 #endif
	print_selector(s);
#endif
	return 1;
}



//If fd's are in use, you might not want to do this...
void free_selector (Selector *s) 
{
	free(s->clients);
	free(s->rarr);
#ifndef NW_DISABLE_LOCAL_USERDATA
	free(s->local_ud);
	s->local_ud = NULL,
#endif
	s->clients  = NULL, 
	s->rarr     = NULL; 
}



void wtf (Selector *s, int count) 
{
	//print a header 
	fprintf( stderr, "\n" );
	fprintf( stderr, "%-5s;%-5s;%-5s;%-7s;%-7s;%-7s;%-15s;%-15s;%-15s\n",
		"fd","recvd","sent","stage","reqLen","resLen","reqAddr","resAddr","fdAddr" );
	fprintf( stderr, 
		"==============================================================================================\n" );	

	//print all open files...
	for ( int y=1; y < count; y++ ) 
	{
		//print all data in CSV like format
		Recvr *j = &s->rarr[ y ];
SHOWDATA( "rq: => %p vs rs: => %p\n", (void *)&j->_request, (void *)&j->_response );
		fprintf( stderr,
			"%-5d;%-5d;%-5d;%-7s;%-7d;%-7d;%-15p;%-15p;%-15p\n",
			j->client->fd, j->recvd, j->sent, GETSTAGE(j->stage), 
			bf_written( &j->_request ), bf_written( &j->_response ), bf_data( &j->_request ), bf_data( &j->_response ), (void *)j->client );
	}
}


//Activate the poll server loop
_Bool activate_selector (Selector *s) 
{
	/*Define stuff*/
	Recvr *rr = s->rarr;
	int maxi = 0, conn = 1, ready;
	int connCount = 0;
	int timeout   = -1;
	int watching  = 0;
	int maxbuf    = NW_MAX_BUFFER_SIZE;

	/*Wait for new connections and spawn children*/
	for (;;) 
	{
		if (NW_CALL(((ready = poll(s->clients, (watching = maxi + 1), timeout)) == -1))) 
			{ switch (errno) {
					case EAGAIN: fprintf( stderr, "repeat the call...\n" ); continue;
					case EINVAL: case EINTR: exit( 0 ); 
					default: break; }}

		/*Check event, accept, set non-block and set last open file*/
		/*NOTE: the following is just an unclear way to specify that the parent received a Read event*/
		if (NW_CALL( s->clients[0].revents & POLLRDNORM )) 
		{
			Recvr  *r     = &rr[conn]; 
			Socket *child = &r->child;

			/*What does the server do when we reach the maximum connections?*/
			if (NW_CALL( conn == s->max_events ))
				handle(ERR_SPAWN_MAX_CLIENTS);

			/* if (NW_CALL( accept( s->clients[0].fd, NULL, NULL ) )*/
			if (NW_CALL( !socket_accept(s->parent, child) ))
				handle(ERR_SPAWN_ACCEPT);

		#ifdef NW_BUFF_FIXED
			//Initialize space for messages
			if (NW_CALL( !bf_init( &r->_request, r->request_, maxbuf ) || !bf_init( &r->_response, r->response_, maxbuf )))
			{
				fprintf( stderr, "Failed to allocate thingy." );
				exit( 0 );
			}
		#else
			if ( !bf_init( &r->_request, NULL, maxbuf ) || !bf_init( &r->_response, NULL, maxbuf ) ) 
			{
				fprintf( stderr, "Failed to allocate request buffer." );
				exit( 0 );
			}
		#endif

		#if 0
			/*Start connection timer up here*/
			if ( s->run_limit && r->start.tv_sec ) {
fprintf( stderr, "TIMEOUT CALC!!!\n" );
				if ( clock_gettime( CLOCK_REALTIME, &r->end ) == -1	)
					; /*EFAULT, EINVAL, EPERM - right now, I don't care*/
				else {
					if ((r->end.tv_sec - r->start.tv_sec) > s->run_limit )
						handle( ERR_TIMEOUT_CONN );
				}
			}
		#endif

			/*Make the new socket non-blocking*/
			if (NW_CALL( fcntl(child->fd, F_SETFD, O_NONBLOCK) == -1 ))
				handle(ERR_SPAWN_NON_BLOCK_SET);

			/*Find the last open connection (there must be a better way)*/
			for (conn=1; conn<s->max_events; conn++)
			{
				SHOWDATA( "conn: %d -> connfd: %d\n", conn, s->clients[conn].fd );
				if (s->clients[conn].fd < 0) 
				{
					/*Set descriptor event*/
					s->clients[conn].fd = child->fd;
					s->clients[conn].events = POLLRDNORM;
					SHOWDATA( "conn: %d -> now connfd: %d\n", conn, s->clients[conn].fd );
					conn++;
					SHOWDATA( "next conn: %d -> its fd: %d\n", conn, s->clients[conn].fd );
					break;
				}
			}

			/*Finally, set the new top and start the real work*/
			if ( conn > maxi )
				maxi = conn;
			if ( --ready <= 0 )
				continue;	
		} /*(NW_CALL( s->clients[0].revents & POLLRDNORM ))*/

		/*maxi needs to drop when connections go away - if not, the poll structure
			is watching for events on descriptors that it doesn't have to*/

		for (int i = 1; i <= maxi; i++) 
		{
			Recvr  *r    = &rr[i]; 
			r->client    = &s->clients[i];
			//r->request = r->_request.buffer;
			//r->response = r->_response.buffer;
			r->request   = (&r->_request)->buffer;
			r->response  = (&r->_response)->buffer;
			int error;
			int min_read =
			#ifdef NW_MIN_ACCEPTABLE_READ
				NW_MIN_ACCEPTABLE_READ
			#else
				s->read_min
			#endif
			;
			int min_write = 
			#ifdef NW_MIN_ACCEPTABLE_WRITE
				NW_MIN_ACCEPTABLE_WRITE
			#else
				s->write_min
			#endif
			;

			//Skip untouched or closed descriptors
			if (NW_CALL( r->client->fd < 0 ))
				continue;

		#if 0
			/*Check if the server should stop the timer*/
			if ( s->run_limit && !r->start.tv_sec ) {
				if ( clock_gettime( CLOCK_REALTIME, &r->start ) == -1	)
					; /*EFAULT, EINVAL, EPERM - worth handling...*/
			}
		#endif

			//Read what's on the socket
			if (NW_CALL( r->client->revents & POLLRDNORM /*| POLLERR)*/ )) 
			{
				r->stage = NW_AT_READ; 

				//NOTE: It would be more consitent to set an error within the recvr
				if (NW_CALL( (error  = nw_read( r ) ))) {
					handle ( error );
				}
				else 
				{
					//Close clients that are too slow
					if (NW_CALL( r->rb < min_read ))
						handle(ERR_READ_BELOW_THRESHOLD);

					/*Call user read handler*/
					uhandle(NW_AT_READ);

					/*HANDLE - Set retries when receiving TCP*/
					if (NW_CALL( r->stage != NW_AT_READ )) {
						r->client->events = POLLWRNORM;
					}
					else 
					{
						if (NW_CALL( ++r->recv_retry >= s->recv_retry)) {
							handle(ERR_READ_MAX_READ_RETRY_REACHED);
						}
						else 
						{ /*Set event on the newest descriptor*/
							r->client->events = POLLRDNORM;
							continue;
						}
					}
				} /*(NW_CALL( (rb = read(r->client->fd, &r->request[0], 6400)) == -1 ))*/
			#ifdef NW_QUEUE_READS
				if ( maxi == 6 ) { wtf( s, maxi ); exit( 0 ); }
			#endif
			} /*(NW_CALL( r->client->revents & (POLLRDNORM | POLLERR) ))*/ 

		#ifdef NW_QUEUE_READS
			continue;
		#endif


		#ifndef NW_SKIP_PROC
			if (NW_CALL( r->stage == NW_AT_PROC )) 
			{
				//Build the response, but I need to handle errors...

				//The easiest way to use this is return a status
				//If it's 0, it failed, move on
				//If it's 1, it's good, move on
				//If it's 2 or more, continue until something else happens
				uhandle(NW_AT_PROC);

				SHOWDATA( "r->status %d\n", r->status );

				/*Status of 0, means run again.*/
				if ( r->status > 0 )
				{
					r->stage = NW_AT_WRITE;
					r->client->events = POLLWRNORM;
				}
			#ifdef NW_QUEUE_PROCS
				if ( maxi == 6 ) { wtf( s, maxi ); exit( 0 ); }
			#endif
				continue;
			}
		 #ifdef NW_QUEUE_PROCS
			continue;
		 #endif
		#endif


		#ifdef NW_SKIP_WRITE
		#else
			if (NW_CALL( r->client->revents & POLLWRNORM && r->stage == NW_AT_WRITE )) 
			{
				if (NW_CALL( (error  = nw_write( r ) ))) {
					handle ( error );
				}
			#ifdef NW_QUEUE_WRITES
				if ( maxi == 6 ) { wtf( s, maxi ); exit( 0 ); }
			#else
				else 
				{
					//Close clients that are too slow
					if (NW_CALL( r->sb < min_write ))
						handle(ERR_WRITE_BELOW_THRESHOLD);
		
					//Perform whatever handler
					uhandle( NW_AT_WRITE );
					SHOWDATA( "error:         %d\n", error );
					SHOWDATA( "current stage: %s\n", GETSTAGE( r->stage ) );

					/*Check if all data came off*/
					if (NW_CALL( r->stage == NW_COMPLETED )) 
					{
						uhandle(NW_COMPLETED);
						close(r->client->fd);
						r->client->fd = -1;
						reset_recvr(r);	
					}
					else 
					{
						/*Set event on the newest descriptor or die with an error*/
						if (NW_CALL( (r->send_retry += 1) < s->send_retry )) {
							r->client->events = POLLWRNORM;
							continue;
						}
						else {
							handle(ERR_WRITE_MAX_WRITE_RETRY_REACHED);
						}
					}
				}
			#endif

			#ifdef NW_QUEUE_WRITES
				SHOWDATA( "error before exit:    %d\n", error );
				r->stage = NW_COMPLETED;
				close(r->client->fd);
				r->client->fd = -1;
				reset_recvr( r );
			#endif
			}
		#endif

		#ifdef NW_SKIP_ERR /*Bad idea...*/
			if (NW_CALL( r->client->revents & POLLERR ))
			{
			}
		#endif
		}/*for*/
	}/*for*/
	return 1;
}
