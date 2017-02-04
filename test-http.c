/* ------------------------------------------- *
 test-http.c
 ============

 Below is a test of basic HTTP streaming.  This 
 supports reading multipart POSTs of any size 
 (within the server's defined limit) and writing 
 a copy of the data back to the client.

 This is the real reason why nw exists.  
 * ------------------------------------------- */
 
#include "lite.h"
#include "nw.h"
#include "debug.h"

/*Keep it thin*/
#define HTTP_405 \
	"HTTP/1.1 405 Method Not Allowed\r\n"

#define HTTP_100 \
	"HTTP/1.1 100 Continue\r\n"

#define HEADER \
	"HTTP/1.1 200 OK\r\n" \
	"Content-Type: text/html\r\n" \
	"Content-Length: %d\r\n\r\n" \

#define FORM \
	"HTTP/1.1 200 OK\r\n" \
	"Content-Type: text/html\r\n" \
	"Content-Length: %d\r\n\r\n" \
	"<form method=POST enctype=multipart/form-data>" \
		"<fieldset>Mouse</fieldset>" \
		"<input type=text name=mouse></input>" \
		"<fieldset>Cat</fieldset>" \
		"<input type=file name=cat></input>"

//Short version of an HTTP reader library
_Bool _read (Recvr *r, void *p, char *e) 
{
	//Define
	int rdAll = 0;
	struct as { int c, h, m; };
	struct as *len = (struct as *)p;
	if ( !len )
	{
		//fprintf( stderr, "allocating something...\n" );
		len = malloc( sizeof( struct as ) );
		memset( len, 0, sizeof( struct as ));
	}

	//Check some stuff
	if ( memcmp( r->request, "GET", 3 ) == 0 )
		return 0;
	else if ( memcmp( r->request, "POST", 4 ) == 0 )
	{
		//Get content length first
		//fprintf( stderr, "getting content length (currently %d)...\n", clen );
		if ( !len->c )
		{
			int a, b;
			char length_char[64]={0};
			if ((a = memstrat( r->request, "Content-Length", r->recvd )) > -1) 
			{
				a += strlen("Content-Length: ");
				b = memchrat( &r->request[a], '\r', r->recvd - a); 

				if (b > 63 || b == -1) return 0;
				memcpy( length_char, &r->request[a], b );
				length_char[b] = '\0';

				for (int i=0;i<strlen(length_char); i++)  
				{
					if ((int)length_char[i] < 48 || (int)length_char[i] > 57)
					{
						fprintf(stderr, "content length was not number\n");
						exit( 0 ); 
					}
				}
				len->c = atoi(length_char);
			}
		}

		//Get the header length next
		//fprintf( stderr, "getting header length...\n" );
		if ( !len->h ) /*That's the total amount in the headers...*/
			if ((len->h = memstrat(r->request, "\r\n\r\n", r->recvd)) > -1 ) len->h += 4;  

		//print important values
		fprintf( stderr, "RECVD: %d, clen: %d, hlen: %d, mlen: %d\n",
			r->recvd, len->c, len->h, (len->m = r->recvd - len->h) );

		//Check if we've finally received everything
		if ( len->m == len->c )
			rdAll = 1, len->m += len->h;
		else
		{
			//write 100 continue if not
			if (write(r->client->fd, HTTP_100, strlen(HTTP_100)) == -1)
				return 0;/*handle errno*/
			return 0;
		}
	}
	else 
	{
		//buffer should be freed and expanded here
		memcpy(r->response, HTTP_405, strlen(HTTP_405));
		//http_err( h, 405, "The method requested by the client was invalid." );
		return 1;
	}

	if (NW_CALL( rdAll )) {
		r->stage = NW_AT_PROC;
		free( len );
		len = NULL;
	}

	return 1;
}



//...
_Bool _process (Recvr *r, void *ud, char *err)
{
	//We want an echo server (to test sending of large requests as well as receiving)
	char bf[2048]= { 0 };
	const char *x= "<p>Message received is below:</p>\n";
	int     size = bf_written( &r->_request );
	int     mlen = strlen(x);
	int     hlen = snprintf( bf, 2047, HEADER, size + mlen ); 

	//...
	if ( !bf_append( &r->_response, (uint8_t *)x, mlen) )
		return ( fprintf( stderr, "append to response failed...\n") ? 0 : 0 );
	if ( !bf_append( &r->_response, bf_data( &r->_request ), size ) )
		return ( fprintf( stderr, "append to response failed...\n") ? 0 : 0 );
	if ( !bf_prepend( &r->_response, (uint8_t *)bf, hlen ) )
		return ( fprintf( stderr, "prepend to response failed...\n") ? 0 : 0 );

#if 0
	write( 2, bf_data( &r->_response ), hlen );
	exit( 0 );
#endif

	int a = bf_written( &r->_response );
	fprintf( stderr, "bf_written( &rsb ): %d\n",  a );	
	r->len += size + hlen + mlen;
	return 1;
}



//...
_Bool _write (Recvr *r, void *ud, char *err) 
{
	/*Hopefully it's easy to see how Chunked-Encoding would be done*/


	r->stage = NW_COMPLETED;
	return 1;
}



//...
_Bool _done (Recvr *r, void *ud, char *err) 
{
	return 1;
}



#if 1
/*Executor table of read, process and write functions*/
Executor rwp[] = {
	[NW_AT_READ]        = { _read    , NW_NOTHING },
	[NW_AT_PROC]        = { _process , NW_NOTHING },
	[NW_AT_WRITE]       = { _write   , NW_NOTHING },
	[NW_COMPLETED]      = { _done    , NW_NOTHING }
};
#endif




int main(int argc, char *argv[]) 
{
	/*Data*/
	Recvr     rec;
	Socket    sock = { 1/*Server or client?*/, "tcp", 2000, "localhost" };
	Selector  sel  = {
		.read_min   = 64, .write_min  = 64, .max_events = 1000,
		.recv_retry = 3 , .send_retry = 3 , .runners    = rwp , 
		.errors = _nw_errors 
	};

	//Tell what we're doing
	fprintf( stderr, "Listening on localhost: 2000\n" );	

	/*Open the socket*/
	if (!socket_open(&sock) || !socket_bind(&sock) || !socket_listen(&sock))
		return (fprintf(stderr, "Socket init error.\n") ? 1 : 1);

	/*Start and process a non-blocking server loop*/
	if (!initialize_selector(&sel, &sock))
		return nw_err(0, "Selector init error.\n"); 

	if (!activate_selector(&sel))
		return (fprintf(stderr, "Something went wrong inside the select loop.\n") && 0);

	/*Clean up and tear down.*/
	free_selector(&sel);
	fprintf(stderr, "Done...\n");
	return 0;
}
