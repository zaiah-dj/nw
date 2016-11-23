#include "lite.h"
#include "nw.h"

/*Keep it thin*/
#define METHOD_NOT_SUPPORTED \
	"HTTP/1.1 405 Method Not Allowed\r\n"

#define HTTP_100 \
	"HTTP/1.1 100 Continue\r\n"

#define HEADER \
	"HTTP/1.1 200 OK\r\n" \
	"Content-Type: text/html\r\n" \
	"Content-Length: %d\r\n\r\n" \
	"<p>Your message was successfully received.</p>"

#define FORM \
	"HTTP/1.1 200 OK\r\n" \
	"Content-Type: text/html\r\n" \
	"Content-Length: %d\r\n\r\n" \
	"<form method=POST enctype=multipart/form-data>" \
		"<fieldset>Mouse</fieldset>" \
		"<input type=text name=mouse></input>" \
		"<fieldset>Cat</fieldset>" \
		"<input type=file name=cat></input>"
	
_Bool _read (Recvr *r, void *ud, char *err) {
	_Bool rd_all_data=0;
	/*Echo what was sent.*/
	//write(1, r->request, r->recvd);
	/*Figure out what kind of request it is once and save it*/
	if (NW_CALL( memstrat(r->request, "GET", r->recvd) > -1 )) {
		fprintf(stderr, "GET received\n");
		exit( 0 );
	}
	else if (NW_CALL( memstrat(r->request, "POST", r->recvd) > -1 )) {
		/*If all data not received, send another message (100-continue)*/	
		fprintf(stderr, "POST received %ld bytes.\n", r->recvd);
		write(r->client->fd, HTTP_100, strlen(HTTP_100));
		exit( 0 );
	}
	else {
		memcpy(r->response, METHOD_NOT_SUPPORTED, strlen(METHOD_NOT_SUPPORTED));
		return 0;
	}

	/*If all data was recvd, move on*/
	if (NW_CALL( rd_all_data )) {
		r->stage = NW_AT_PROC;
exit(0);
		return 1;
	}

	fprintf(stderr, "All data not read, trying again...\n");
	/*Repeat this until all the data is received*/
	return 0;
}

_Bool _process (Recvr *r, void *ud, char *err) {
	return 0;
}

_Bool _write (Recvr *r, void *ud, char *err) {
	return 0;
}

_Bool _done (Recvr *r, void *ud, char *err) {
	return 0;
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

int main(int argc, char *argv[]) {
	/*Data*/
	Socket    sock = { 
		1/*Server or client?*/, "tcp", 2000, "localhost" };
	Selector  sel  = {
		.read_min   = 64  , .write_min  = 64, .max_events = 1000,
		.recv_retry = 3   , .send_retry = 3 , 
		.runners    = rwp , .errors = _nw_errors                };

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
