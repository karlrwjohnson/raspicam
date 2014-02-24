
#include <arpa/inet.h>  // inet_pton

#include <net/if.h>     // struct ifreq
#include <netinet/in.h> // struct sockaddr_in

#include <sys/ioctl.h>  // ioctl()
#include <sys/socket.h> // socket()

#include <errno.h>      // errno
#include <cstdio>       // printf()
#include <cstring>      // strerror()
#include <iostream>     // cout
#include <pthread.h>    // multithreading
#include <memory>       // shared_ptr
#include <stdexcept>    // exceptions
#include <sstream>      // stringstream (used by Log.h)
#include <string>       // strings
#include <vector>       // vector
#include <unistd.h>     // close()

#include "Log.h"
#include "Sockets.h"

using namespace std;

const unsigned char _localhostIp[4] = { 127, 0, 0, 1 };
const in_addr_t LOCALHOST_IP_ADDR = *((in_addr_t*) _localhostIp);

string
ip2string (in_addr_t ipAddr)
{
	uint8_t* ipAddr_p = (uint8_t*) &ipAddr;
	uint32_t a = ipAddr_p[0];
	uint32_t b = ipAddr_p[1];
	uint32_t c = ipAddr_p[2];
	uint32_t d = ipAddr_p[3];
	stringstream ss;
	ss << a << '.' << b << '.' << c << '.' << d;
	return ss.str();
}

struct Message
{
	uint32_t type;
	uint32_t length;
};

///// Socket /////

Socket::Socket ():
	fd(-1)
{
	cout << "-- Trace -- " << __FUNCTION__ << endl;
	_openSocket();
}

Socket::~Socket ()
{
	cout << "-- Trace -- " << __FUNCTION__ << endl;
	_closeSocket();
	pthread_join(readerHandle, NULL);
	pthread_mutex_destroy(&writerMutex);
}

void
Socket::_openSocket ()
{
	TRACE_ENTER;
	IF_TRACE(cout << "Opening socket" << endl);
	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd == -1) {
		THROW_ERROR("Failed to open socket.");
	}
	IF_TRACE(cout << "Socket is " << fd << endl);
}

void
Socket::_closeSocket ()
{
	TRACE_ENTER;
	if (fd != -1) {
		shutdown(fd, SHUT_RDWR); // forces read()/recv() to return
		close(fd);
		fd = -1;
	}
}

/**
 * Sends data through the socket
 * @param type    Integer indicating the type of the message
 * @param length  Number of bytes to send from *data
 * @param data    Data to send
 */
void
Socket::sendMessage (uint32_t type, size_t length, void* data)
{
	TRACE_ENTER;

	int status;
	if ((status = pthread_mutex_trylock(&writerMutex))) {
		IF_TRACE(cout << "writerMutex is currently locked. Going to stand in line..." << endl);

		if ((status = pthread_mutex_lock(&writerMutex))) {
			THROW_ERROR("Unable to lock writerMutex: " << strerror(status));
		}
	}

	int bytesWritten;

	Message msg;
	msg.type = type;
	msg.length = length;

	IF_TRACE(cout << "Sending message of type " << type << " to socket " << fd << endl);

	bytesWritten = send(fd, &msg, sizeof(msg), 0);
	if (bytesWritten < 0) {
		THROW_ERROR("Failed to write message header to socket: " << strerror(errno)
			     << " (error code " << errno << ")");
	} else if (bytesWritten != sizeof(msg)) {
		THROW_ERROR("Size mismatch writing message header to socket: Wrote "
			     << bytesWritten << " bytes, but message header is "
			     << sizeof(msg) << " bytes instead.");
	}

	bytesWritten = send(fd, data, length, 0);
	if (bytesWritten < 0) {
		THROW_ERROR("Failed to write data to socket: " << strerror(errno));
	} else if (bytesWritten != length) {
		THROW_ERROR("Size mismatch writing data to socket: Wrote "
			     << bytesWritten << " bytes, but data is "
			     << length << " bytes instead.");
	}

	IF_TRACE(cout << "Done sending." << endl);

	if ((status = pthread_mutex_unlock(&writerMutex))) {
		THROW_ERROR("Unable to unlock mutex: " << strerror(status));
	}

	TRACE_EXIT;
}

void
Socket::_initReaderThread()
{
	int err;

	IF_TRACE(cout << "Creating mutex..." << endl);
	if ((err = pthread_mutex_init(&writerMutex, NULL))) {
		THROW_ERROR("Error creating mutex: " << strerror(err));
	}

	IF_TRACE(cout << "Creating reader thread..." << endl);
	if ((err = pthread_create(&readerHandle, NULL, _readerThreadCaller, this))) {
		THROW_ERROR("Unable to start reader thread: " << strerror(err));
	}
}

/**
 * Calls _readerThread on the specified function.
 *   Workaround for the fact that you can't pass a pointer to a non-static
 * member function to pthread_create().
 */
void*
Socket::_readerThreadCaller (void* socketObject)
{
	TRACE_ENTER;
	void* retval = static_cast<Socket*>(socketObject)->_readerThread();
	TRACE_EXIT;
	return retval;
}

/**
 * Buffers message objects from the socket and passes them to the
 * user-defined handler method
 */
void*
Socket::_readerThread()
{
	try
	{
		IF_TRACE(cout << "Reader thread started.\n";)

		while (1 /* ?? */)
		{
			Message msg;
			int bytesReceived;
			shared_ptr< vector<uint8_t> > buffer;

			// Read in a message header
			bytesReceived = recv(fd, &msg, sizeof(Message), MSG_WAITALL);
			if (bytesReceived < 0) {
				THROW_ERROR("Error reading from socket: " << strerror(errno)
			             << " (error code " << errno << ")");
			} else if (bytesReceived == 0) {
				cerr << "Peer has closed the connection; Terminating thread." << endl;
				break;
			}
			
			// Allocate space to put the message body
			buffer = shared_ptr< vector<uint8_t> >(new vector<uint8_t>(msg.length));

			// Read in the message body
			if (msg.length > 0)
			{
				bytesReceived = recv(fd, &(*buffer)[0], msg.length, MSG_WAITALL);
				if (bytesReceived < 0) {
					THROW_ERROR("Error reading from socket: " << strerror(errno)
			                 << " (error code " << errno << ")");
				} else if (bytesReceived == 0) {
					cerr << "Peer has closed the connection; Terminating thread." << endl;
					break;
				}

				handleMessage(msg.type, msg.length, (void*) &(*buffer)[0]);
			}
			else
			{
				// Allocate _some_ space to pass as a pointer so there isn't a
				// segfault on a null pointer.
				int8_t dummy;
				handleMessage(msg.type, msg.length, (void*) &dummy);
			}
		}

		return NULL;
	}
	catch (runtime_error e)
	{
		cerr << "!! Caught exception in reader thread:" << endl
			 << "!! " << e.what() << endl;
		return NULL;
	}
}

void
Socket::joinReaderThread ()
{
	TRACE_ENTER();
	int err = pthread_join(readerHandle, NULL);
	if (err) {
		THROW_ERROR("pthread_join() failed: " << strerror(err));
	}
	TRACE_EXIT();
}

///// Server /////

Server::Server (int port) :
	Socket()
{
	TRACE_ENTER;

	// Prepare the arguments for bind()
	sockaddr_in bindAddress;
	memset(&bindAddress, 0, sizeof(bindAddress));
	bindAddress.sin_family      = AF_INET;
	bindAddress.sin_port        = htons(port);
	bindAddress.sin_addr.s_addr = htonl(INADDR_ANY);

	// Associate the socket with a port and (any) IP address
	cout << "Binding to socket with IP address "
	     << ip2string(ntohl(bindAddress.sin_addr.s_addr))
	     << ", port " << ntohs(bindAddress.sin_port) << endl;	
	if (bind(fd, (sockaddr *) &bindAddress, sizeof(bindAddress))) {
		THROW_ERROR("bind() failed: " << strerror(errno));
	}

	// Start listening for connections. We really only want to handle one
	// at a time, so we set the queue length to zero.
	cout << "Listening for connections..." << endl;
	if (listen(fd, 1)) {
		THROW_ERROR("listen() failed: " << strerror(errno));
	}

	// Wait for new connections. This will hang until a connection is made,
	// or the process is killed.
	cout << "Awaiting new connections..." << endl;
	sockaddr_in clientAddress;
	socklen_t   clientAddressSize = sizeof(clientAddress);
	int connection = accept(fd, (sockaddr *) &clientAddress, &clientAddressSize);
	if (connection < 0) {
		THROW_ERROR("accept() failed: " << strerror(errno));
	}

	cout << "Received connection from client at "
		 << ip2string(clientAddress.sin_addr.s_addr)
	     //// I'm not sure if the port number needs to be reversed or not
		 //<< ", port " << ntohs(clientAddress.sin_port) << endl; 
		 << ", port " << clientAddress.sin_port << endl;

	// Close the original descriptor because we're no longer accepting connections,
	// and replace it with the new one from accept()
	close(fd);
	fd = connection;

	// Start the reader thread so we can start processing messages
	_initReaderThread();

	TRACE_EXIT;
}

///// Client /////

Client::Client (string ipAddr, int port) :
	Socket()
{
	TRACE_ENTER;

	// Prepare the arguments for connect()
	sockaddr_in serverAddress;
	socklen_t   serverAddressLength = sizeof(serverAddress);
	memset(&serverAddress, 0, sizeof(serverAddress));

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);

	// Parse the IP address
	uint32_t address;
	if (!inet_pton(AF_INET, ipAddr.c_str(), &address)) {
		THROW_ERROR("inet_pton() failed: " << strerror(errno)
			      << ". Is the address bad?");
	}

	//serverAddress.sin_addr.s_addr = ntohl(address);
	serverAddress.sin_addr.s_addr = address;

	cout << "Connecting to server with IP address "
	     //<< ip2string(ntohl(serverAddress.sin_addr.s_addr))
	     << ip2string(serverAddress.sin_addr.s_addr)
	     << ", port " << ntohs(serverAddress.sin_port) << endl;

	// Connect to the server
	if (connect(fd, (sockaddr *) &serverAddress, sizeof(serverAddress))) {
		THROW_ERROR("connect() failed: " << strerror(errno));
	}

	// Start the reader thread so we can start processing messages
	_initReaderThread();
}

