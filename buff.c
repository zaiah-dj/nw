/*Make the buffer implementation as simple as you can*/
#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "buff.h"


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
	return b->written;
}



//Get the unsigned char data
uint8_t *bf_data (Buffer *b) {
	return b->buffer;
}


//Write out errors
const char *bf_err (Buffer *b) {
	return Buff_errors[b->error];
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
