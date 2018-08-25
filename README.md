tmnw
====
The simple network loop.


## Summary
tmnw is a two file library for managing network messages using non-blocking sockets.

Libraries like libevent and libuv will outperform tmnw on raw speed, but tmnw aims to be 
simpler to use than libevent or libuv by adhering to some simple goals.

- Be useful with two files (tmnw.c and tmnw.h), one object (tmnw.o)
- Operate well in both single threaded and multi threaded environments
- Do not try to run on every Unix known to man
- Allow an implementor to use callbacks to handle protocols that need to run on top of these platforms (see Usage for more details)
- Focus on the network (UDP and TCP, not really worried about IPC)


## Dependencies
tmnw has only one real dependency: poll().  It can be found on most Unix systems and will give you a nice level of performance right out of the box.  See `man poll` for more information.  Windows is another story.


## Compatibility 

At the moment, tmnw will only work in C projects.  I have simply not had the time to test with C++.


### Compiling

To compile `tmnw` for your own apps, do the following:

#### Linux
<code>gcc -Wall -std=c99</code> should yield the library you need.  If you do run into any build errors, please notify me at ramar.collins@gmail.com or send me a pull request.

#### OSX
tmnw is so far untested on OSX, but it is down the pipeline as I have a Mac Mini to test with.

#### Windows
Windows has poll() support now, but internet lore tells me this was not always the case.  As time avails, I may look into bringing this to Windows as it could be useful for game developers.



### Example

Below you can see some example code on how to actually use `tmnw` as a networking backend.
The following example shows you how to set up a TCP listener on port 2000.

<ol>
<li>
Declare and define a Socket.

<p>
So, whenever the term Socket is read in this manual, realize that I'm not speaking about an actual file that you can send and receive data packets over.   In this library, the term 'Socket' refers to a datatype that serves as a template for a connection you would like to initiate.  For example, you can tell the library that you intend to make a server that only listens for UDP messages from a certain port.  The C code for it is below:
</p>

<pre>
	Socket sk   = { 
		.server   = 1,          // 'True' means we want a server, 'False' makes a client
		.proto    = "tcp",      // Select TCP, not UDP
		.port     = 2000,       // Choose a port from 1 to 65534
		.hostname = "localhost" // Choose a hostname
	};
</pre>
</li>


<li>
Open a socket and bind and listen if successful

<p>
In Unix-like systems, creating a socket is, frankly, a pain in the ass.  There are tons of flags and behaviors that you need to keep track of that really won't matter to you if you're not planning doing anything particularly unusual with the connection.
</p>

<p>
Also notice that you <b>don't</b> need to check errno when using this library.  Each of the functions was written to return either true or false depending on whether or not it was successful.
</p>

<pre>
	//Notice how we can chain these together
	if ( !socket_open(&sock) || !socket_bind(&sock) || !socket_listen(&sock) )
	{
		fprintf( stderr, "Couldn't create socket.\n" );
		return 0;
	}
</pre>

</li>

<li>
Open the Selector structure.

<p>
Modern systems allow a lot of modifications to regular socket behaivor.   
Most of that can be controlled through an abstraction called a Selector.  
An example of this structure is here.
</p>

<pre>
Selector  sel = {
	.read_min   = 64, 
	.write_min  = 64, 
	.max_events = 1000,
	.recv_retry = 3, 
	.send_retry = 3, 
	.runners    = runners, 
	.errors     = _nw_errors,
};
</pre>

<p>
So, what do each of these keys actually do?

keyname     | type | description
-------     | ---- | ------------
read_min    | 64   | Read x bytes, throwing an error if threshold is not reached. 
write_min   | 64   | Write x bytes, throwing an error if threshold is not reached. 
max_events  | 1000 | How many poll members should be allowed?
recv_retry  | 3    | Times that tmnw should try to read from an open socket.
send_retry  | 3    | Times that tmnw should try to send from an open socket.
runners     | ??   | Array of functions used to read different points of nw's process
errors      | ??   | Array of functions used to handle errors that occur at different parts of nw's process.

</p>

</li>


<li>
Check and initialize the socket structure

<p>
Unfortunately, we are still writing in C and zero'ing out and initializing data is still a task I have to delegate to you.  Fortunately, this simple function makes it really easy for you to do.  This function also handles any validation issues that you may have in your code, such as invalid ports or plain errant setup.
</p>
 
<pre>
	if (!initialize_selector( &sel, &sock ))
		return tmnw_err( &sel );
</pre>

<p>
That first argument is something that probably warrants a little bit of an explanation.  tmnw's behavior can be controlled at the lower levels of the socket protocol via the Selector structure.  For the purposes of this introduction, I am going to use the defualt one.
</p>

</li>

<li>
Start a non-blocking server loop

<p>
`tmnw` does not block at all.  It will never set up a blocking server no matter what you do.  Yay. 
</p>

<pre>
	if (!activate_selector( &sel ))
		return tmnw_err( &sel );
</pre>
</li>


<li> 

Test that all is well.
<p>
	If you've gotten this far and have compiled the program, you'll find that it does nothing but tell you which port it is running on.   But hey, my friend, you'd be mistaken.  Because there is an invisible server running via port 2000 that is just waiting on you to send it something...
</p>

<p>
	You can run the following script via Bash to see how it works.
	You can even test that those non-blocking sockets are actually non-blocking working as they should.
</p>

<pre>	
	#/bin/sh -
	for n in `seq 1 10`; do
		curl http://localhost:2000 &
	done
</pre>

<p>
	The above script just asks curl to make a GET request for "/" 10 times.
</li>
</ol>


## Doing Slightly More Than the Minimum

<p>
So far, our server is not doing very much except sending back a piddly little message and calling it a day.
Right, so you just wasted all of your time reading through this manual.  Good for you, you've won the "Captain Dingus" award.
</p>

<p>
Fortunately, there is more to the story.  tmnw tries pretty hard to abstract the process of network communication.  To make it easy to write apps that can handle networking, tmnw breaks up the server loop into four distinct parts.  This way, you don't even have to write your own event loop.  You are just writing handlers that plug in to said loop at different parts of the process.
</p>


## Handlers

<p>
	Now we can define "actions" for our application to take at certain points
	in the request-response chain.  
</p>

<p>
	In the theoretical sense, a handler is really just a specific action to follow when a certain condition is met.  For example, let's suppose you want to do more than send my trite little message back to your users.   Perhaps you want to show a picture.  Or maybe you want to be a real web server and handle requests for certain directories on your system.  Or maybe you don't want anybody looking at anything ever, so you decide to automatically '400 Unauthorized' headers in succession.   Perhaps you can serve rudimentary CSS along with this message so that your users can '400 Unauthorized' headers with pages in colors matching each spectrum of the rainbow.   Whatever floats your boat, my friend.
</p>

<p>
	In the technical sense, a handler is nothing more than a function pointer.  
	When writing your own handlers, use the following defintion. 
	The following snippet shows the declaration you'll need to use when binding custom actions to `tmnw`.
<p>

<pre>
//Always return a \_Bool, works via both C and C++
\_Bool (\*fn_name) (Recvr \*r, void \*ud, char \*err) 
</pre>

<p>
Looks pretty easy, right?  Except, what's that Recvr * thing?   Well, I'm glad you asked.
</p>

<p>
The Recvr structure is used by tmnw to keep track of the status of each request.  To keep it simple, pretty much <i>every</i> possible piece of metadata about an open socket connection is stored here.   I could go into great detail about how much data is stored here, but I'll let the code speak for me this time:
</p>

<pre>
typedef struct {
  Socket            child;  //Child file descriptor
	Stage             stage;  //An enum defining where we are currently in the process
	int              rb, sb;  //Bytes received at an invocation of the "read loop"
  int         recvd, sent;  //Total bytes sent or received
  int                 len;  //Length of message to be sent?
	int               error;  //Was an error detected?

	//For error messages when everything may fail
  uint8_t          errbuf[NW_ERROR_BUFFER_LENGTH];  

	//Notice that tmnw can set a size limit on received requests.
#ifdef NW_BUFF_FIXED
  uint8_t        request_[NW_MAX_BUFFER_SIZE];
  uint8_t       response_[NW_MAX_BUFFER_SIZE];
#endif
  uint8_t        *request;
  uint8_t       *response;
	
	//What the client sends goes here
	Buffer       _request;

	//What you prepare goes here
	Buffer      _response;

	//...you can skip these...
	int      sretry, rretry;
	int          recv_retry;
	int          send_retry;
	int          *socket_fd;  //Pointer to parent socket?

	//The original pollfd structure
	struct pollfd   *client;  //Pointer to currently being served client

#ifndef NW_DISABLE_LOCAL_USERDATA
	void          *userdata;
#endif

	//Track the time that a request starts and ends, also 
	//used to track how long the connection has been opened.
	struct timespec start;
	struct timespec end;
} Recvr;
</pre>

<li>
NW_AT_READ
<p>
The stage immediately after tmnw reads a message from a client
</p>
</li>

<li>
NW_AT_PROC
<p>An optional intermediate stage that tmnw reaches after successfully reading a  client's message.  Assumes that no further reading is needed.</p>
</li>

<li>
NW_AT_WRITE
<p>
Stage in which message has been sent from server.  If the message were extremely large, one could optionally send the rest of the message to the client from this stage.
</p>
</li>

<li>
NW_AT_COMPLETED
<p>
The message is assumed to be done at this point.  You can close the file descriptor or keep it open if you are using some kind of protocol that is capable of re-using an open socket.
</p>
</li>



Customization
=============
The	list of defines to change the size of tmnw are below:

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
#	-DNW_FOLLOW     - Print each branchable call as the program passes 
#    through execution.
#	-DNW_VERBOSE    - Print each error message to stderr if anything fails.
#	-DNW_KEEP_ALIVE - Define whether or not to leave connections open 
#    after NW_COMPLETED is reached.
#	-DNW_SKIP_PROC  - Define whether or not to skip the processing step 
#    of a connection. 
# -DNW_MIN_ACCEPTABLE_READ   - Define a floor for the number of bytes 
#    received during read and drop the connection if it's not reached.
# -DNW_MIN_ACCEPTABLE_WRITE  - Define a floor for the number of bytes 
#    received during write and drop the connection if it's not reached.
#	-DNW_LOCAL_USERDATA  - Compile with space for local userdata.
#	-DNW_GLOBAL_USERDATA - Compile with space for global userdata.
#	-DNW_CATCH_SIGNAL    - Listen for signals...
</pre>
