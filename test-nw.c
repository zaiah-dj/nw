//#include "lite.h"
#include "tmnw.h"

#define CMP( a, b ) \
	!strcmp( *argv, a ) || !strcmp( *argv, b ) 

#define HEADER \
	"HTTP/1.1 200 OK\r\n" \
	"Content-Type: text/html\r\n" \
	"Content-Length: %d\r\n\r\n"

#define DIAGNOSTICS_FMT \
	"%-3s %s listening for connections on %s:%d\n" 

const char usage[] =
	"Usage: \n"
	"-t, --tcp     Use TCP packets.          \n"
	"-u, --udp     Use UDP packets.          \n"
	"-s, --server  Run in server mode.       \n"
	"-c, --client  Run in client mode.       \n"
	"-h, --help    Show help and quit.       \n"
;

//Read
_Bool _read (Recvr *r, void *ud, char *err) 
{
	/*Echo what was sent.*/
	fprintf( stderr, "REQUEST RECEIVED\n" );
	fprintf( stderr, "================\n" );
	write(2, r->request, r->recvd);
	write( 2, "\n", 1 );
	r->stage = NW_AT_PROC;
	return 1;
}



//Process
_Bool _process (Recvr *r, void *ud, char *err) 
{
	char header[256] = {0};
	int  len = r->recvd - 4;
	int  written = snprintf(header, 256, HEADER, (int)len);

	//Write the message in reverse
	for ( int i = len, j = 0; i >= 0; i--, j++ ) {
		if ( !bf_append( &r->_response, &r->request[ i ], 1 ) ) {
			fprintf( stderr, "Failed to append %d byte of request...\n", j );
			exit ( 1 );	  
		}
	}

	if ( !bf_prepend( &r->_response, (uint8_t *)header, written ) ) {
		fprintf( stderr, "Failed to prepend header to response...\n" );
		exit ( 1 );	  
	}


	//Dump the response before sending
	fprintf( stderr, "RESPONSE BEFORE SENDING\n" );
	fprintf( stderr, "=======================\n" );
	write( 2, (&r->_response)->buffer, r->len = r->_response.written );
	write( 2, "\n", 1 );
	r->stage = NW_AT_WRITE;
	return 1;	
}



//Write
_Bool _write (Recvr *r, void *ud, char *err) 
{
	r->stage = NW_COMPLETED;
	return 1;	
}



//Close the descriptor
_Bool _done (Recvr *r, void *ud, char *err) 
{
	if ( close(r->client->fd) == -1 ) {
		/*Have to do something with errno here*/
		return 0;
	}
	fprintf(stderr, "fd: %d\n", r->client->fd);
	r->client->fd = -1;	
	return 1;
}


/*Executor table of read, process and write functions*/
Executor runners[] = {
	[NW_AT_READ]        = { _read    , NW_NOTHING },
	[NW_AT_PROC]        = { _process , NW_NOTHING },
	[NW_AT_WRITE]       = { _write   , NW_NOTHING },
	[NW_COMPLETED]      = { _done    , NW_NOTHING }
};



//A 'Selector' defines acceptable behavior for an open network socket.
Selector  sel = 
{
#if 0
	.min          = { 64, 64 },
	.retry        = { 3, 3 },
	.max_events   = 1000,
	.runners= runners,
	.errors = errors,
#endif
	.read_min   = 64, 
	.write_min  = 64, 
	.max_events = 1000,
	.recv_retry = 3, 
	.send_retry = 3, 
	.runners    = runners, 
	.errors     = _nw_errors,
#if 0
	/*If you want local data, you just tell it the size*/ 
	.lsize      = sizeof(struct bunk), 
#endif
};


//A 'Socket' defines how I should tell the OS to open a network socket
Socket sock = 
{ 
	1,             //Server or client?
	"tcp",         //TCP or UDP
	"localhost",   //Which address to bind to?
	.port = 2000   //What port?
};


int main(int argc, char *argv[]) 
{
	if (argc < 2) {
		fprintf( stderr, "%s\n", usage );
		return 1;
	}

	while ( *argv )
	{
		if ( CMP( "-t", "--tcp" ) )
		{
			goto opt;
		}

		if ( CMP( "-u", "--udp" ) )
		{
			goto opt;
		}

		if ( CMP( "-s", "--server" ) )
		{
			goto opt;
		}

		if ( CMP( "-c", "--client" ) )
		{
			goto opt;
		}

		if ( CMP( "-h", "--help" ) )
		{
			goto opt;
		}

opt:
		argv++;
	}

	//Open the socket
	if (!socket_open(&sock) || !socket_bind(&sock) || !socket_listen(&sock))
		return (fprintf(stderr, "Socket init error.\n") ? 1 : 1);

	//Initialize the server loop
	if (!initialize_selector(&sel, &sock))
		return nw_err(0, "Selector init error.\n"); 

	//Let me know what port and server type
	fprintf( stderr, DIAGNOSTICS_FMT, 
		sock.proto, sock.server ? "server" : "client", sock.hostname, sock.port );

	//Start the loop
	if (!activate_selector(&sel))
		return (fprintf(stderr, "Something went wrong inside the select loop.\n")?1:1);

	//Clean up and tear down.
	free_selector(&sel);
	fprintf(stderr, "Done...\n");
	return 0;
}
