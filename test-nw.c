#include "lite.h"
#include "nw.h"
#define HEADER \
	"HTTP/1.1 200 OK\r\n" \
	"Content-Type: text/html\r\n" \
	"Content-Length: %d\r\n\r\n"

/*Read*/
_Bool _read (Recvr *r, void *ud, char *err) {
	/*Echo what was sent.*/
	write(2, r->request, r->recvd);
	r->stage = NW_AT_PROC;
	return 1;
}

/*Process*/
_Bool _process (Recvr *r, void *ud, char *err) {
	char header[256] = {0};
	int  len = r->recvd - 4;
	snprintf(header, 256, HEADER, (int)len);

	/*Start at end and reverse the original message*/
	for (int i=0, j = r->len = len + strlen(header); i < len; i++, j--)
		r->response[j] = r->request[i];

	memcpy(r->response, header, strlen(header));
	r->stage = NW_AT_WRITE;
	return 1;	
}

/*Write*/
_Bool _write (Recvr *r, void *ud, char *err) {
	r->stage = NW_COMPLETED;
	return 1;	
}

/*Close the descriptor*/
_Bool _done (Recvr *r, void *ud, char *err) {
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


#if 0
/*Test of initialization of different local userdata values*/
struct bunk { int fd, gd, pd; char *j; } bunk;  
void dumpval (Selector *s) {
	int g=0;
	for (int n=0; n<s->max_events; n++, g++) {
		struct bunk *b = (struct bunk *)(&s->rarr[n])->userdata;
		b->fd = g + 1, 
		b->gd = g + 3,
		b->pd = g + 5;
		b->j = "I love pajamas";
	}

	for (int n=0; n<s->max_events; n++, g++) {
		struct bunk *b = (struct bunk *)(&s->rarr[n])->userdata;
		fprintf(stderr, "%d, %d, %d, %s\n", b->fd, b->gd, b->pd, b->j);	
	}
} 
#endif

/*Options to test.*/
Option opts[] = {
	{ "-t", "--tcp",    "Use TCP packets."          },
	{ "-u", "--udp",    "Use UDP packets."          },
	{ "-s", "--server", "Run in server mode."       },
	{ "-c", "--client", "Run in client mode."       },
	{ "-h", "--help",   "Show help and quit."       },
	{ .sentinel=1 }
};


/*select-test.c*/
int main(int argc, char *argv[]) {
	if (argc < 2)
		opt_usage(opts, "Nothing to do.", 1);
	if (!opt_eval(opts, argc, argv) /*You ought to handle the error*/)
		return (/*errprintf(...) &&*/	1); 

	/*Data*/
	/*executors[x] = ?, and others... then I don't have to worry about it...*/
	Socket    sock = { 1/*Server or client?*/, "tcp", 2000, "localhost" };
	Selector  sel = {
#if 0
	.min          = { 64, 64 },
	.retry        = { 3, 3 },
	.max_events   = 1000,
	.runners= runners,
	.errors = errors,
#endif
		.read_min   = 64, 
		.write_min  = 64, 
		.max_events = 10,
		.recv_retry = 3, 
		.send_retry = 3, 
		.runners    = runners, 
		.errors     = _nw_errors,
	#if 0
		/*If you want local data, you just tell it the size*/ 
		.lsize      = sizeof(struct bunk), 
	#endif
	};

	/*Open the socket*/
	if (!socket_open(&sock) || !socket_bind(&sock) || !socket_listen(&sock))
		return (fprintf(stderr, "Socket init error.\n") ? 1 : 1);

	/*Initialize the server loop*/
	if (!initialize_selector(&sel, &sock))
		return nw_err(0, "Selector init error.\n"); 

	/*What port am I running on?*/
	fprintf(stderr, "Listening for requests at localhost:%d\n", 2000);

	/*Start the loop*/
	if (!activate_selector(&sel))
		return (fprintf(stderr, "Something went wrong inside the select loop.\n")?1:1);

	/*Clean up and tear down.*/
	free_selector(&sel);
	fprintf(stderr, "Done...\n");
	return 0;
}
