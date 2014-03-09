#define TRACE_ON

#include <arpa/inet.h>  // inet_pton

#include <net/if.h>     // struct ifreq
#include <netinet/in.h> // struct sockaddr_in, in_port_t

#include <sys/ioctl.h>  // ioctl()
#include <sys/socket.h> // socket()

#include <errno.h>      // errno
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
#include "Thread.h"

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

struct MessageHeader
{
	message_t type;
	message_len_t length;
};

//--- MutexLock ---//

MutexLock::MutexLock (pthread_mutex_t &mutex):
	mutex_p (&mutex),
	locked  (false)
{ }

MutexLock::~MutexLock ()
{
	unlock();
}

void
MutexLock::unlock ()
{
	if (locked)
	{
		int status;
		if ((status = pthread_mutex_unlock(mutex_p))) {
			WARNING("Unable to unlock mutex: " << strerror(status));
		}

		locked = false;
	}
}

void
MutexLock::relock ()
{
	if (!locked)
	{
		int status;
		if ((status = pthread_mutex_trylock(mutex_p))) {
			TRACE("Mutex is already locked. Going to wait in line...");

			if ((status = pthread_mutex_lock(mutex_p))) {
				THROW_ERROR("Unable to lock mutex: " << strerror(status));
			}
		}

		locked = true;
	}
}

//--- Connection ---//

Connection::Connection (int fd_, in_addr_t remoteAddress_, in_port_t remotePort_):
	fd              (fd_),
	remoteAddress   (remoteAddress_),
	remotePort      (remotePort_),
	stopReadingFlag (true)
{
	TRACE_ENTER;

	int err;
	if ((err = pthread_mutex_init(&writerMutex, NULL))) {
		THROW_ERROR("Error creating mutex: " << strerror(err));
	}

	TRACE_EXIT;
}

Connection::~Connection ()
{
	close();
	pthread_mutex_destroy(&writerMutex);
}

void
Connection::close ()
{
	//TODO
	TRACE_ENTER;

	// Signal the reader thread to stop
	stopReadingFlag = true;

	if (fd != -1)
	{
		// Reader thread is probably still waiting on read()/recv().
		// Calling shutdown() forces that call to return, which in theory
		// should be enough to make the reader thread return by itself.
		shutdown(fd, SHUT_RDWR);

		::close(fd);
		fd = -1;
	}

	TRACE_EXIT;
}

void
Connection::startReaderThread ()
{
	TRACE_ENTER;
	stopReadingFlag = false;
	readerThreadHandle = pthread_create_using_method<Connection, void*>(
		*this, &Connection::readerThread, NULL
	);
	TRACE_EXIT;
}

void
Connection::joinReaderThread ()
{
	int err = pthread_join(readerThreadHandle, NULL);
	if (err)
	{
		THROW_ERROR("pthread_join() failed: " << strerror(err));
	}
}

void
Connection::sendMessage (message_t type, size_t length, void* data)
{
	TRACE_ENTER;

	int bytesWritten;

	MessageHeader header;
	header.type = type;
	header.length = length;

	if (data == NULL && length != 0) {
		THROW_ERROR("Null data pointer given with nonzero length = " << length
		         << " (message type = " << type << ")");
	}

	MutexLock lock(writerMutex);
	lock.relock();

	TRACE("Writing message of type " << type << " and length " << length
	   << " to socket " << fd);

	bytesWritten = send(fd, &header, sizeof(header), 0);
	if (bytesWritten < 0) {
		THROW_ERROR("Failed to write message header to socket: " << strerror(errno)
			     << " (error code " << errno << ")");
	} else if (bytesWritten != sizeof(header)) {
		THROW_ERROR("Size mismatch writing message header to socket: Wrote "
			     << bytesWritten << " bytes, but message header is "
			     << sizeof(header) << " bytes instead.");
	}

	if (data != NULL && length != 0)
	{
		bytesWritten = send(fd, data, length, 0);
		if (bytesWritten < 0) {
			THROW_ERROR("Failed to write data to socket: " << strerror(errno));
		} else if (bytesWritten != length) {
			THROW_ERROR("Size mismatch writing data to socket: Wrote "
			     	 << bytesWritten << " bytes, but data is "
			     	 << length << " bytes instead.");
		}
	}

	TRACE_EXIT;
}

void
Connection::sendMessage (message_t type, string text)
{
	sendMessage(type, text.length(), (void*) text.c_str());
}

void
Connection::sendMessage (message_t type)
{
	sendMessage(type, 0, NULL);
}

void
Connection::addMessageHandler (message_t type, const message_handler_t& handler)
{
	TRACE_ENTER;

	//shared_ptr<message_handler_set> hs = handlers[type];
	message_handler_set &hs = handlers[type];
	//handlers[type] = hs; // Inserts it if it doesn't already exist

	// Abort insertion if the handler is already present
	for (message_handler_set::iterator itr = hs.begin();
	     itr != hs.end();
	     itr++)
	{
		if (*itr == &handler)
		{
			TRACE("Handler 0x" << hex << &handler << " already exists for message type " << dec << type);
			return;
		}
	}

	TRACE("Adding handler 0x" << hex << &handler << " for message type " << dec << type);
	TRACE("# of handlers was: " << hs.size());
	hs.push_back(&handler);
	TRACE("# of handlers is: " << hs.size());

	TRACE_EXIT;
}

void
Connection::addDefaultMessageHandler (const message_handler_t& handler)
{
	TRACE_ENTER;

	// Abort insertion if the handler is already present
	for (message_handler_set::iterator itr = defaultHandlers.begin();
	     itr != defaultHandlers.end();
	     itr++)
	{
		if (*itr == &handler)
		{
			TRACE("Default handler 0x" << hex << &handler << " already exists");
			return;
		}
	}

	TRACE("Adding default message handler 0x" << hex << &handler);
	defaultHandlers.push_back(&handler);

	TRACE_EXIT;
}

void
Connection::removeMessageHandler (message_t type, const message_handler_t& handler)
{
	TRACE_ENTER;

	message_handler_set &hs = handlers[type];

	// Search for the handler in the list
	for (message_handler_set::iterator itr = hs.begin();
	     itr != hs.end();
	     /* increment manually */)
	{
		if (*itr == &handler)
		{
			TRACE("Removing message handler 0x" << hex << &handler << " for message type " << dec << type);
			itr = hs.erase(itr);
			break;
		}
		else
		{
			itr++;
		}
	}

	// Remove key if no values are left
	if (handlers[type].size() == 0)
	{
		TRACE("Last handler of type " << dec << type << " removed; removing key from handler map");
		handlers.erase(type);
	}

	TRACE_EXIT;
}

void
Connection::removeDefaultMessageHandler (const message_handler_t& handler)
{
	TRACE_ENTER;

	// Search for the handler in the list
	for (message_handler_set::iterator itr = defaultHandlers.begin();
	     itr != defaultHandlers.end();
	     /* increment manually */)
	{
		if (*itr == &handler)
		{
			TRACE("Removing default message handler 0x" << hex << &handler);
			itr = defaultHandlers.erase(itr);
			break;
		}
		else
		{
			itr++;
		}
	}

	TRACE_EXIT;
}

void
Connection::readerThread (void* unused)
{
	TRACE_ENTER;
	try
	{
		while (!stopReadingFlag)
		{
			MessageHeader header;
			int bytesReceived;
			shared_ptr< vector<uint8_t> > buffer;

			// Read in a message header
			TRACE("Awaiting incoming message");
			bytesReceived = recv(fd, &header, sizeof(header), MSG_WAITALL);
			if (bytesReceived < 0) {
				THROW_ERROR("Error reading from socket: " << strerror(errno)
			             << " (error code " << errno << ")");
			} else if (bytesReceived == 0) {
				MESSAGE("Peer has closed the connection; Terminating thread.");
				break;
			}

			TRACE("Received message type " << header.type << ", " << header.length << " bytes.");
			
			// Read in the message body if it exists
			if (header.length > 0)
			{
				buffer = shared_ptr< vector<uint8_t> >(new vector<uint8_t>(header.length));

				bytesReceived = recv(fd, &(*buffer)[0], header.length, MSG_WAITALL);
				if (bytesReceived < 0) {
					THROW_ERROR("Error reading from socket: " << strerror(errno)
			                 << " (error code " << errno << ")");
				} else if (bytesReceived == 0) {
					MESSAGE("Peer has closed the connection. Terminating thread.");
					break;
				}
			}
			else
			{
				// If no data was expected, allocate a blank, single-byte
				// buffer in case a handler accidentally dereferences it.
				buffer = shared_ptr< vector<uint8_t> >(new vector<uint8_t>(1));
				(*buffer)[0] = 0;
			}

			// Call the handlers for the given message type, or the default
			// handlers if none exist.

			// First, figure out if any handlers exist which match the message
			// type.
			message_handler_map::iterator handlersItr = handlers.find(header.type);

			if (handlersItr != handlers.end() && handlersItr->second.size() >= 0)
			{
				TRACE("There are " << handlersItr->second.size()
				   << " handlers for message type " << header.type);
				for (message_handler_set::iterator itr = handlersItr->second.begin();
			     	 itr != handlersItr->second.end();
			     	 itr++)
				{
					TRACE("Calling handler");
					// Call the handler
					(**itr)(header.type, header.length, (void*) &(*buffer)[0]);
				}
			}
			else
			{
				TRACE("There are " << defaultHandlers.size() << " default handlers");
				// If no handlers exist, process the default handlers
				for (message_handler_set::iterator itr = defaultHandlers.begin();
			     	 itr != defaultHandlers.end();
			     	 itr++)
				{
					TRACE("Calling default handler");
					(**itr)(header.type, header.length, (void*) &(*buffer)[0]);
				}
			}
		}
	}
	catch (runtime_error e)
	{
		cerr << "!! Caught exception in reader thread:" << endl
			 << "!! " << e.what() << endl;
	}
	TRACE_EXIT;
}

//--- Server ---//

Server::Server ():
	fd                (-1),
	stopAcceptingFlag (true)
{
	TRACE_ENTER;

	int err;
	if ((err = pthread_mutex_init(&connectionsMutex, NULL))) {
		THROW_ERROR("Error creating mutex: " << strerror(err));
	}

	TRACE_EXIT;
}

Server::~Server ()
{
	stop();
	pthread_mutex_destroy(&connectionsMutex);
}

void
Server::start (in_port_t port)
{
	TRACE_ENTER;

	fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd == -1) {
		THROW_ERROR("Failed to open socket.");
	}
	MESSAGE("Opened socket with descriptor " << fd);

	// Prepare the arguments for bind()
	sockaddr_in bindAddress;
	memset(&bindAddress, 0, sizeof(bindAddress));
	bindAddress.sin_family      = AF_INET;
	bindAddress.sin_port        = htons(port);
	bindAddress.sin_addr.s_addr = htonl(INADDR_ANY);

	// Associate the socket with a port and (any) IP address
	MESSAGE("Binding to socket with IP address "
	     << ip2string(ntohl(bindAddress.sin_addr.s_addr))
	     << ", port " << ntohs(bindAddress.sin_port));
	if (bind(fd, (sockaddr *) &bindAddress, sizeof(bindAddress))) {
		close(fd);
		fd = -1;
		THROW_ERROR("bind() failed: " << strerror(errno));
	}

	// Start listening for connections.
	MESSAGE("Listening for connections...");
	if (listen(fd, INCOMING_CONNECTION_QUEUE_SIZE)) {
		close(fd);
		fd = -1;
		THROW_ERROR("listen() failed: " << strerror(errno));
	}

	stopAcceptingFlag = false;
	while(!stopAcceptingFlag)
	{
		// Wait for new connections. This will hang until a connection is made,
		// or the process is killed.
		MESSAGE("Awaiting new connections...");
		sockaddr_in clientAddress;
		socklen_t   clientAddressSize = sizeof(clientAddress);
		int connFd = accept(fd, (sockaddr *) &clientAddress, &clientAddressSize);
		if (connFd < 0) {
			close(fd);
			fd = -1;
			THROW_ERROR("accept() failed: " << strerror(errno));
		}

		MESSAGE("Received connection from client at "
		     << ip2string(clientAddress.sin_addr.s_addr)
	         // I'm not sure if the port number needs to be reversed or not:
		     // try "ntohs(clientAddress.sin_port)" if this is wrong
		     << ", port " << clientAddress.sin_port);

		MutexLock lock(connectionsMutex);
		lock.relock();

		shared_ptr<Connection> conn = newConnection(
				connFd, clientAddress.sin_addr.s_addr, clientAddress.sin_port
		);
		conn->startReaderThread();
		connections.push_back(conn);

		lock.unlock();
	}

	MESSAGE("Socket closed.");

	TRACE_EXIT;
}

void
Server::stop ()
{
	stopAcceptingFlag = true;
	close(fd);
	forEachConnection([] (Connection &c) { c.close(); });
}

void
Server::forEachConnection (function<void(Connection&)> f)
{
	MutexLock lock(connectionsMutex);
	lock.relock();

	for (connection_list_t::iterator itr = connections.begin();
	     itr != connections.end();
	     itr++)
	{
		f(**itr);
	}
}

//--- Client ---//

Client::Client ()
{ }

Client::~Client ()
{
	//TODO
}

shared_ptr<Connection>
Client::connect (string ipAddress, in_port_t port)
{
	TRACE_ENTER;

	// Parse the IP address
	uint32_t address;
	if (!inet_pton(AF_INET, ipAddress.c_str(), &address)) {
		THROW_ERROR("inet_pton() failed: " << strerror(errno)
			      << ". Is the address bad?");
	}

	int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd == -1) {
		THROW_ERROR("Failed to open socket.");
	}
	MESSAGE("Opened socket with descriptor " << fd);

	// Prepare the arguments for connect()
	sockaddr_in serverAddress;
	socklen_t   serverAddressLength = sizeof(serverAddress);
	memset(&serverAddress, 0, sizeof(serverAddress));

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(port);

	//serverAddress.sin_addr.s_addr = ntohl(address);
	serverAddress.sin_addr.s_addr = address;

	MESSAGE("Connecting to server with IP address "
	     << ip2string(serverAddress.sin_addr.s_addr)
	     << ", port " << ntohs(serverAddress.sin_port))

	// Connect to the server
	if (::connect(fd, (sockaddr *) &serverAddress, sizeof(serverAddress))) {
		close(fd);
		THROW_ERROR("connect() failed: " << strerror(errno));
	}

	connection = newConnection(fd, serverAddress.sin_addr.s_addr, serverAddress.sin_port);

	return connection;
}

