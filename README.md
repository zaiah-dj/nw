nw
==
The simple network loop.

Summary
-------
nw is a single file library for managing network messages using non-blocking sockets.
<ul>
- Server loop handles non-blocking sockets with poll
- This aims to be simpler to use than libevent or libuv by being:
		*one file
		*one object
		*being single threaded (do not run your multi-threaded nginx beater with this.  just don't.  don't even try.  you will fail miserably...)
		*not trying to run on every *nix known to man
		*allowing an implementor to use callbacks to handle protocols that need to run on top of these platforms
		*focus on the network (UDP and TCP, not really worried about IPC)

</ul>


Dependencies
============
nw has only one real dependency.  poll().  It's on most <code>*nix*</code> systems and will give you a nice level of performance right out of the box.  See `man poll` for more information.

Also, nw relies on another library of mine titled 'lite'.  I have not released all of it's primitives yet.


Compiling
=========
<p>
On Linux:
gcc -Wall are the only flags you will need to get a nice and purty library.
</p>
<p>
nw is so far untested on OSX and Windows
</p>


Usage
=====
Using nw in your application is pretty simple. Unfortunately, I only 
have directions for C right now, as I have not had time to test with C++.

	1. Declare and define a socket 
	*C:
	Socket sk   = { 
		.server   = 1,         // 'True' means we want a server
		.proto    = "tcp"      // Select TCP, not UDP or IPC
		.port     = [0-65535]  // Choose a port
		.hostname = "localhost"// Choose a hostname
	};

	2. Open a socket and bind and listen if successful
	*C:	
	//Notice how we can chain these together
	if ( !socket_open(&sock) || !socket_bind(&sock) || !socket_listen(&sock) )
		return socket_error(&sock);  

	3. Check and initialize the socket structure
	*C:
	if (!initialize_selector(&sel, &sock)) //&l, local_index))
		return nw_err(&sel);

	4. Start a non-blocking server loop
	*C:
	if (!activate_selector(&sel))
		return nw_err(&sel);

	5. Test that all is well.
	Our application is not doing anything useful at the moment.  It is just an
	open drain waiting for water. (Or more technically speaking, an open wire
	waiting for digits.)  Running the following should let you see how it
	works and test that non-blocking sockets are working as they should.
	
	for n in `seq 1 10`; do
		curl http://localhost:2000 &
	done

	6. Define actions.
	Now we can define "actions" for our application to take at certain points
	in the request-response chain.  nw breaks this chain into 4 steps:

	NW_AT_READ      - The stage immediately after nw reads a 
                    message from a client
	NW_AT_PROC      - An optional intermediate stage that nw
							      reaches after successfully reading a 
                    client's message.  Assumes that no 
                    further reading is needed.
  NW_AT_WRITE     - Stage in which message has been sent
                    from server.  If the message were 
                    extremely large, one could optionally
                    send the rest of the message to the
                    client from this stage.
	NW_AT_COMPLETED - The message is assumed to be done
                    at this point.  You can close the
                    file descriptor or keep it open if
                    a protocol like WebSockets is being
                    used and you would like to send more 
                    data. 


Customization
=============
The	list of defines to change the size of nw are below:

	NW_BEATDOWN_TEST - Enables some test friendly options, like stopping after
										 a certain number of requests
	NW_CATCH_SIGNAL  - Catch signals or not.  Will define free_selector by default.
	NW_VERBOSE       - Be verbose
	NW_FOLLOW        - Dump a message when moving through each part of the program.
	NW_SKIP_PROC     - Compile without a "processing" part of the server loop.
	NW_KEEP_ALIVE    - Define whether or not the connection should stay open after 
										 reaching the write portion of the connection's life cycle.
	NW_LOCAL_USERDAT - Define whether or not to use local userdata.
	NW_GLOBAL_USERDA - Define whether or not to use global userdata.
	NW_STREAM_TYPE   - Define which way to read requests...


Here is an example of using a header file 
to define settings. We'll call our file
whatever.h:

<pre>
//Contents of "whatever.h"
#define MIN_ACCEPTABLE_READ 64   	//Kick requests that fail to read this much
#define NW_MAX_BUFFER_SIZ   8192 	//Read no more than this much to buffer
#define NW_MAX_BUFFER_SIZ   8192 	//Write no more than this much to buffer
#define MAX_OPEN_FILES      256  	//How many files can I have open at once?
#define READ_TO_FILE        0    	//When reading, use a file and stream
#define READ_TO_BUF         1    	//When reading, use a buffer
</pre>
