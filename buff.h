#include <inttypes.h>

#ifndef BUFF_H
#define BUFF_H
enum {	
	ERR_NONE,
	ERR_BUFF_ALLOC_FAILURE,
	ERR_BUFF_REALLOC_FAILURE,
	ERR_BUFF_OUT_OF_SPACE,
};

#if 1
static const char *Buff_errors[] = {
	[ERR_NONE] = "No errors",
	[ERR_BUFF_ALLOC_FAILURE] = "Buffer allocation failure",
	[ERR_BUFF_REALLOC_FAILURE] = "Buffer reallocation failure",
	[ERR_BUFF_OUT_OF_SPACE] = "Fixed buffer is out of space.",
};
#endif

typedef struct Buffer Buffer;
struct Buffer {
	uint8_t *buffer;
	int size;
	int written;
	int fixed;
	int error;
};


Buffer *bf_init (Buffer *b, uint8_t *mem, int size);
void bf_setwsize (Buffer *b, int size); 
int bf_append (Buffer *b, uint8_t *s, int size); 
int bf_prepend (Buffer *b, uint8_t *s, int size); 
void bf_free (Buffer *b); 
const char *bf_err (Buffer *b);
int bf_written (Buffer *b) ;
uint8_t *bf_data (Buffer *b) ;
#endif
