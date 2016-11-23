/*select-loop.c*/
#include "lite.h" //TODO: Modify nw to be totally reliant on regular socket calls
#include "nw.h"
#include <poll.h>
#ifdef NW_CATCH_SIGNAL
 #include <signal.h>
#endif

#ifdef NW_VERBOSE
/*Dump the selector*/
void print_selector (Selector *s) {
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
void print_recvr (Recvr *r) {
	fprintf(stderr, "child:       %p\n", (void *)&r->child);
	fprintf(stderr, "recvd:       %d\n", r->recvd);
	fprintf(stderr, "sent:        %d\n", r->sent);
	fprintf(stderr, "len:         %d\n", r->len);
	fprintf(stderr, "stage:       %d\n", r->stage);
	fprintf(stderr, "request_fd:  %d\n", r->request_fd);
	fprintf(stderr, "response_fd: %d\n", r->response_fd);
	fprintf(stderr, "request:     %p\n", (void *)r->request);
	fprintf(stderr, "response:    %p\n", (void *)r->response);
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

#ifdef NW_CATCH_SIGNAL
void nw_set_sighup (int) {
	free_selector();
}
#endif

/*Static list of error codes in text*/
const char *nw_error_map[] = {
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

/*Call logging function*/
#ifdef NW_VERBOSE 
 #define nw_log(...) \
	fprintf(stderr, __VA_ARGS__);
 #define nw_error_log(map, code) \
	write(2, map[code], strlen(map[code]))
#else
 #define nw_log(...)
 #define nw_error_log(map, code)
#endif

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
	if (NW_CALL( (&s->runners[CODE])->exe(r, s->global_ud, (&s->runners[CODE])->err) )) { \
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

/*Default executor table for those who don't want to be bothered*/
Executor _nw_errors[ERR_WRITE_MAX_WRITE_RETRY_REACHED + 1] = {
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
static void reset_recvr (Recvr *r) {
	memset(&r->child, 0, sizeof(Socket));
	memset(&r->request[0], 0, NW_MAX_BUFFER_SIZE); 
	memset(&r->response[0], 0, NW_MAX_BUFFER_SIZE);	
	r->rb = 0, 
	r->sb = 0, 
	r->recvd = 0, 
	r->sent = 0, 
	r->len = 0, 
	r->stage = 0;
	r->recv_retry = 0,
	r->send_retry = 0;
}

/*Readers and writers*/
_Bool dummy_reader (void *in, void *out) { return 0; } 

_Bool dummy_writer (void *in, void *out) { return 0; }

_Bool __breader__ (Recvr *r) {
	r->recvd += r->rb = read(r->client->fd, &r->request[r->recvd], NW_MAX_BUFFER_SIZE - r->recvd);
	return 1;
}

_Bool __bwriter__ (Recvr *r) {
	/*Check it's not bigger than the NW_MAX_BUFFER_SIZE*/
	r->sent  += r->sb = write(r->client->fd, &r->response[r->sent], r->len - r->sent);
	return 1;
} 

/*This should be the slowest...*/
_Bool __freader__ (Recvr *r) {
	/*Make a buffer, since there's no way to read directly without using a pipe*/
	uint8_t tmp[NW_MAX_BUFFER_SIZE] = {0};
	int status;

	/*Open or set the file reference (If speed is a problem, pre-open all of the files...)*/
	r->request_fd = (!r->request_fd) ? open( "a", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR ) : r->request_fd;

	/*Open the file*/
	r->recvd += r->rb = read(r->client->fd, tmp, NW_MAX_BUFFER_SIZE);

	/*Write the results somewhere...*/
	if ((status = write(r->request_fd, tmp, r->rb)) == -1)
		return 0;
	return 1;
}

/*This isn't technically a writer...*/
_Bool __fwriter__ (Recvr *r) {
	/*Make a buffer, since there's no way to read directly without using a pipe*/
	uint8_t tmp[NW_MAX_BUFFER_SIZE] = {0};
	int status;

	/*Open or set the file reference (If speed is a problem, pre-open all of the files...)*/
	r->response_fd = (!r->response_fd) ? open( "a", O_RDWR | O_CREAT, S_IRUSR | S_IWUSR ) : r->response_fd;

	/*Open the file*/
	r->sent += r->sb = write(r->client->fd, tmp, NW_MAX_BUFFER_SIZE);

	/*Write the results somewhere...*/
	if ((status = write(r->response_fd, tmp, r->sb)) == -1)
		return 0;

	return 1;
} 

_Bool __preader__ (Recvr *r) {
	r->recvd += r->rb = read(r->client->fd, &r->request[r->recvd], NW_MAX_BUFFER_SIZE - r->recvd);
	return 1;
}

_Bool __pwriter__ (Recvr *r) {
	r->sent  += r->sb = write(r->client->fd, &r->response[r->sent], r->len - r->sent);
	//provided the FINISHED response is saved to pipe, then you should be fine...
	//sendfile(r->client->fd, pipe_read_end);
	return 1;
} 

/*Stacked functions for reading, writing, and manipulating buffers*/
Streamer stream[] = {
	[NW_STREAM_BUF] = { __breader__, __bwriter__ },
	[NW_STREAM_FD]  = { __freader__, __fwriter__ },
	[NW_STREAM_PIPE]= { __preader__, __pwriter__ },
};

/*Reset file descriptor for reading*/
_Bool reset_read_fd (Recvr *r, void *ud, char *err) {
	r->client->events = POLLRDNORM;
	return 1;
}

/*Reset file descriptor for writing */
_Bool reset_write_fd (Recvr *r, void *ud, char *err) {
	r->client->events = POLLWRNORM;
	return 1;
}

/*Clear buffer*/
_Bool reset_buffer (Recvr *r, void *ud, char *err) {
	return (memset(r->response, 0, NW_MAX_BUFFER_SIZE) != NULL);
}

/*A dummy function for the purposes of this tutorial*/
static _Bool dummy (Recvr *r, void *ud, char *err) {
	return 0;
}

/*Close*/
_Bool nw_close_fd (Recvr *r, void *ud, char *err) {
	return ((close(r->client->fd) != -1) && (r->client->fd = -1));
}

/*...*/
_Bool nw_reset_fd (Recvr *r, void *ud, char *err) {
	print_recvr(r);
	fprintf(stderr, "r->client is:   %p\n",  (void *)r->client);
	fprintf(stderr, "r->client->fd:  %d\n",  r->client->fd);
	reset_recvr(r);
	if (close(r->client->fd) == -1) {
		return 0;
	}
	r->client->fd = -1;
	print_recvr(r);
	return 1;
}


/*Initialize the selectors*/
_Bool initialize_selector (Selector *s, Socket *sock) {
//_Bool initialize_selector (Selector *s, Socket *sock, void *ud, void * (*ud_init)(void *, int)) {
	/*Always set up the parent on behalf of the user*/
	s->parent = sock;

	/*Always set the parent to non-blocking*/
	if (NW_CALL( fcntl(s->parent->fd, F_SETFD, O_NONBLOCK) == -1 )) 
		return nw_err(0, "fcntl error: %s\n", strerror(errno)); 

	/*Allocate needed pollfd structures here*/
	if (NW_CALL( !(s->clients = malloc(sizeof(struct pollfd) * s->max_events)) ))
		return nw_err(0, "Failed to allocate poll structures.\n");

	/*Allocate needed recvr structures as well.*/
	//if (NW_CALL( !(s->rarr = calloc(s->max_events, sizeof(Recvr))) ))
	if (NW_CALL( !(s->rarr = malloc(sizeof(Recvr) * s->max_events)) ))
		return nw_err(0, "Failed to allocate space for network receiver structures.\n");

	/*Allocate any local userdata*/
 #ifndef NW_DISABLE_LOCAL_USERDATA
	if (s->lsize && s->lsize < 0)
		return nw_err(0, "Local userdata size cannot be negative.\n");
	else if (s->lsize) {
		if (NW_CALL( !(s->local_ud = malloc(s->lsize * s->max_events)) ))
			return nw_err(0, "Failed to allocate space for local userdata.\n");
		memset(s->local_ud, 0, s->lsize * s->max_events);
		s->tsize = (s->lsize * s->max_events);
	}
 #endif

	/*Initialize all fds in pollfd to -1, and allocate any local userdata*/
	for (int i=0; i<s->max_events; i++) {
	 #ifndef NW_DISABLE_LOCAL_USERDATA
		char *p = (!s->lsize) ? NULL : (char *)s->local_ud;
		//memset(&p[u], ++a, s->lsize);
		(&s->rarr[i])->userdata = (!s->lsize) ? NULL : (void *)&p[i * s->lsize];
   #endif
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


/*If fd's are in use, you might not want to do this...*/
void free_selector (Selector *s) {
	free(s->clients);
	free(s->rarr);
#ifndef NW_DISABLE_LOCAL_USERDATA
	free(s->local_ud);
	s->local_ud = NULL,
#endif
	s->clients  = NULL, 
	s->rarr     = NULL; 
}


/*Activate the poll server loop*/
_Bool activate_selector (Selector *s) {
	/*Define stuff*/
	Recvr *rr = s->rarr;
	int maxi = 0, conn = 1, ready;
 #ifdef NW_BEATDOWN_MODE
	#ifndef NW_BEATDOWN_STOP_AFTER
	 #define NW_BEATDOWN_STOP_AFTER 100
	#endif
	int stop_after = NW_BEATDOWN_STOP_AFTER, stop_ct = 0;
 #endif

	/*Wait for new connections and spawn children*/
	for (;;) {
		if (NW_CALL(((ready = poll(s->clients, maxi + 1, -1)) == -1))) {
			switch (errno) {
				case EAGAIN:
					//handle(ERR_POLL_INITIAL_ALLOCATOR);
				case EINVAL:
					//handle(ERR_POLL_TOO_MANY_FILES);
				case EINTR:
					//handle(ERR_POLL_RECVD_SIGNAL );
				default:
					break;
			}	
		}

		/*Check event, accept, set non-block and set last open file*/
		if (NW_CALL( s->clients[0].revents & POLLRDNORM )) {
			Recvr  *r     = &rr[conn]; 
			Socket *child = &r->child;

			if (NW_CALL( !socket_accept(s->parent, child) ))
				handle(ERR_SPAWN_ACCEPT);

		#ifdef NW_BEATDOWN_MODE
			if (++stop_ct > stop_after)
				return 1;
		#endif

		#if 0
			/*Handle for socket data goes here*/
			uhandle(NW_AT_ACCEPT);
		#endif

			/*Make the new socket non-blocking*/
			if (NW_CALL( fcntl(child->fd, F_SETFD, O_NONBLOCK) == -1 ))
				handle(ERR_SPAWN_NON_BLOCK_SET);

			/*Find the last open connection (there must be a better way)*/
			for (conn=1; conn<s->max_events; conn++) {
				if (s->clients[conn].fd < 0) {
					s->clients[conn].fd = child->fd;
					break;
				}
			}

			/*What does the server do when we reach the maximum connections?*/
			if (NW_CALL( conn == s->max_events ))
				handle(ERR_SPAWN_MAX_CLIENTS);

			/*Initialize the "Recvr" and set descriptor event*/
			s->clients[conn].events = POLLRDNORM;

			/*Finally, set the new top and start the real work*/
			if (conn > maxi)
				maxi = conn;
			if (--ready <= 0)
				continue;	
		} /*(NW_CALL( s->clients[0].revents & POLLRDNORM ))*/

		/*Loop through each file descriptor*/
		for (int i = 1; i <= maxi; i++) {
			Recvr  *r = &rr[i]; 
			r->client = &s->clients[i];

			/*Skip untouched or closed descriptors*/
			if (NW_CALL( r->client->fd < 0 ))
				continue;

			/*Try to receive as much data as possible*/
			if (NW_CALL( r->client->revents & (POLLRDNORM | POLLERR) )) {
				r->stage = NW_AT_READ; 

				//Though this is clear, I'm missing something...
				stream[s->stream].read(r);
		
				/*Handle errors or bad reads...*/
				if (NW_CALL( r->rb == 0 ))	{
					handle(ERR_READ_CONN_CLOSED_BY_PEER);
				}
				else if (NW_CALL( r->rb == -1)) { 
					switch (errno) {
						case ECONNRESET:
							handle(ERR_READ_CONN_RESET);
						case EAGAIN/*Try reading again in a minute...*/:
							r->client->events = POLLRDNORM;
							handle(ERR_READ_EGAIN);
						case EBADF:
							reset_recvr(r);
							handle(ERR_READ_EBADF);
						case EFAULT:
							reset_recvr(r);
							handle(ERR_READ_EFAULT);
						case EINVAL:
							reset_recvr(r);
							handle(ERR_READ_EINVAL);
						case EINTR:
							reset_recvr(r);
							handle(ERR_READ_EINTR);
						case EISDIR:
							reset_recvr(r);
							handle(ERR_READ_EISDIR);
						default:
							break;
					}
				} 
#if 0
				/*Peer closed connection.  So drop it all...*/	
				else if (r->rb == 0) { 
					handle(ERR_READ_CONN_CLOSED_BY_PEER);
					return 0;
				}
#endif
				else {
					/*ENTRY - Handle rereads, where to put stuff, etc.*/
				#ifdef NW_MIN_ACCEPTABLE_READ
					int min_read = NW_MIN_ACCEPTABLE_READ;
				#else
					int min_read = s->read_min;
				#endif

					/*Close clients that are too slow*/
					if (NW_CALL(r->rb < min_read))
						handle(ERR_READ_BELOW_THRESHOLD);

					/*Call user read handler*/
					uhandle(NW_AT_READ);	

					/*HANDLE - Set retries when receiving TCP*/
					if (NW_CALL( r->stage != NW_AT_READ )) {
						r->client->events = POLLWRNORM;
					}
					else { 
						if (NW_CALL((r->recv_retry += 1) < s->recv_retry)) {
							/*Set event on the newest descriptor*/
							r->client->events = POLLRDNORM;
							continue;
						}
						else {
							handle(ERR_READ_MAX_READ_RETRY_REACHED);
						}
					}
				} /*(NW_CALL( (rb = read(r->client->fd, &r->request[0], 6400)) == -1 ))*/
			} /*(NW_CALL( r->client->revents & (POLLRDNORM | POLLERR) ))*/ 

		#ifndef NW_SKIP_PROC
			//if (r->client->revents & (POLLWRNORM | POLLERR) || r->stage == NW_AT_PROC) {
			if (NW_CALL( r->stage == NW_AT_PROC )) {
				/*Build the response*/ 
				uhandle(NW_AT_PROC);	
				r->stage = NW_AT_WRITE;
				r->client->events = POLLWRNORM; 
				continue;
			}
		#endif

			if (NW_CALL( r->client->revents & (POLLWRNORM | POLLERR) && r->stage == NW_AT_WRITE )) {
				/*Write the response to socket*/
				stream[s->stream].write(r);

				/*Handle errno*/
				if (NW_CALL( r->sb == -1 )) {
					switch (errno) {
						case EAGAIN: /*This shoudn't happen, but if it does...*/
							handle(ERR_WRITE_EGAIN);
						case EBADF:      /*Peer closed early, why is it here?*/
							reset_recvr(r);
							handle(ERR_WRITE_EBADF);
						case EFAULT:   /*I don't have any more space to write*/
							reset_recvr(r);
							handle(ERR_WRITE_EFAULT);
						case EFBIG:             /*I can't send this much data*/
							reset_recvr(r);
							handle(ERR_WRITE_EFBIG);
						/*In these cases, I have little choice but to close the peer*/
						case EDQUOT: 
							reset_recvr(r);
							handle(ERR_WRITE_EDQUOT);
						case EINVAL:
							reset_recvr(r);
							handle(ERR_WRITE_EINVAL);
						case EIO:
							reset_recvr(r);
							handle(ERR_WRITE_EIO);
						case ENOSPC:
							reset_recvr(r);
							handle(ERR_WRITE_ENOSPC);
						case EINTR:
							reset_recvr(r);
							handle(ERR_WRITE_EINTR);
						case EPIPE:
							reset_recvr(r);
							handle(ERR_WRITE_EPIPE);
						case EPERM: /*I can't write, b/c another process disallowed it*/
							reset_recvr(r);
							handle(ERR_WRITE_EPERM);
						/*case EDESTADDREQ: //TODO: GCC complains.  Why?
							handle(ERR_WRITE_EDESTADDREQ); 
							close_fds(i); */
						default:
							break;		
					}
				}
				else if (r->sb == 0) {
					reset_recvr(r);
					handle(ERR_WRITE_CONN_CLOSED_BY_PEER);
				}
				else {
				#ifdef NW_MIN_ACCEPTABLE_WRITE
					int min_write = NW_MIN_ACCEPTABLE_WRITE;
				#else
					int min_write = s->write_min;
				#endif

					/*Close clients that are too slow*/
					if (NW_CALL( r->sb < min_write ))
						handle(ERR_WRITE_BELOW_THRESHOLD);
		
					/*Perform whatever handler*/
					uhandle(NW_AT_WRITE);

					/*Check if all data came off*/
					if (NW_CALL( r->stage == NW_COMPLETED )) {
						uhandle(NW_COMPLETED);
					#ifndef NW_KEEP_ALIVE
						close(r->client->fd);
						r->client->fd = -1;
					#endif
						reset_recvr(r);	
					}
					else { 
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
			}
		}/*for*/
	}/*for*/
	return 1;
}
