
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

#include "../Log.h"

using namespace std;

const unsigned char _localhostIp[4] = { 127, 0, 0, 1 };
const in_addr_t LOCALHOST_IP_ADDR = *((in_addr_t*) _localhostIp);

const in_port_t DEFAULT_PORT = 32123;

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

#define LOCK_MUTEX(m) \
	do { \
		int err; \
		if ((err = pthread_mutex_lock(&(m)))) { \
			THROW_ERROR("Unable to lock mutex: " << strerror(err)); \
		} \
	} while(0);
#define UNLOCK_MUTEX(m) \
	do { \
		int err; \
		if ((err = pthread_mutex_unlock(&(m)))) { \
			THROW_ERROR("Unable to unlock mutex: " << strerror(err)); \
		} \
	} while(0);

//#define TRACE_ON
#if defined(TRACE_ON)
# define TRACE_ENTER \
	cout << "-- Trace -- Entering " << __FUNCTION__
# define TRACE_EXIT \
	cout << "-- Trace -- Exiting " << __FUNCTION__
# define TRACE(statement) statement
#else
# define TRACE_ENTER
# define TRACE_EXIT
# define TRACE(statement)
#endif

enum MSG_TYPE
{
	MSG_TYPE_MARCO,
	MSG_TYPE_POLO
};

struct Message
{
	uint32_t type;
	uint32_t length;
};

class Socket {

  public:
	int fd;

	pthread_mutex_t writerMutex;

	pthread_t readerHandle;

  protected:

	/**
	 * This method must be overridden to process incoming messages.
	 */
	virtual void
	handleMessage (uint32_t type, uint32_t length, void* buffer) = 0;

  private:
	void
	_openSocket ()
	{
		TRACE_ENTER;
		TRACE(cout << "Opening socket" << endl);
		fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (fd == -1) {
			THROW_ERROR("Failed to open socket.");
		}
		TRACE(cout << "Socket is " << fd << endl);
	}

	void
	_closeSocket ()
	{
		TRACE_ENTER;
		if (fd != -1) {
			shutdown(fd, SHUT_RDWR); // forces read()/recv() to return
			close(fd);
			fd = -1;
		}
	}

	/**
	 * Buffers message objects from the socket and passes them to the
	 * user-defined handler method
	 */
	void*
	_readerThread()
	{
		try
		{
			cout << "Reader thread started.\n";

			while (1 /* ?? */)
			{
				Message msg;
				int bytesReceived;
				shared_ptr< vector<uint8_t> > buffer;

				// Read in a message header
				bytesReceived = recv(fd, &msg, sizeof(Message), MSG_WAITALL);
				if(bytesReceived < 0) {
					THROW_ERROR("Error reading from socket: " << strerror(errno)
			                 << " (error code " << errno << ")");
				} else if (bytesReceived == 0) {
					cerr << "Peer has closed the connection; Terminating thread." << endl;
					break;
				}
				
				// Allocate space to put the message body
				buffer = shared_ptr< vector<uint8_t> >(new vector<uint8_t>(msg.length));

				// Read in the message body
				bytesReceived = recv(fd, &(*buffer)[0], msg.length, MSG_WAITALL);
				if(bytesReceived < 0) {
					THROW_ERROR("Error reading from socket: " << strerror(errno)
			                 << " (error code " << errno << ")");
				//} else if (bytesReceived == 0) {
				//	cerr << "Peer has closed the connection; Terminating thread." << endl;
				//	break;
				}

				handleMessage(msg.type, msg.length, (void*) &(*buffer)[0]);
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

	/**
	 * Calls _readerThread on the specified function.
	 *   Workaround for the fact that you can't pass a pointer to a non-static
	 * member function to pthread_create().
	 */
	static void*
	_readerThreadCaller (void* socketObject)
	{
		TRACE_ENTER;
		void* retval = static_cast<Socket*>(socketObject)->_readerThread();
		TRACE_EXIT;
		return retval;
	}

  protected:
	void
	_initReaderThread()
	{
		int err;

		TRACE(cout << "Creating mutex..." << endl);
		if((err = pthread_mutex_init(&writerMutex, NULL))) {
			THROW_ERROR("Error creating mutex: " << strerror(err));
		}

		TRACE(cout << "Creating reader thread..." << endl);
		if((err = pthread_create(&readerHandle, NULL, _readerThreadCaller, this))) {
			THROW_ERROR("Unable to start reader thread: " << strerror(err));
		}
	}

  public:
	Socket ():
		fd(-1)
	{
		cout << "-- Trace -- " << __FUNCTION__ << endl;
		_openSocket();
	}

	~Socket ()
	{
		cout << "-- Trace -- " << __FUNCTION__ << endl;
		_closeSocket();
		pthread_join(readerHandle, NULL);
		pthread_mutex_destroy(&writerMutex);
	}

	/**
	 * Sends data through the socket
	 * @param type    Integer indicating the type of the message
	 * @param length  Number of bytes to send from *data
	 * @param data    Data to send
	 */
	void
	sendMessage (uint32_t type, size_t length, void* data)
	{
		TRACE_ENTER;

		int status;
		if((status = pthread_mutex_trylock(&writerMutex))) {
			TRACE(cout << "writerMutex is currently locked. Going to stand in line..." << endl);

			LOCK_MUTEX(writerMutex);
		}

		int bytesWritten;

		Message msg;
		msg.type = type;
		msg.length = length;

		TRACE(cout << "Sending message of type " << type << " to socket " << fd << endl);

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

		TRACE(cout << "Done sending." << endl);

		UNLOCK_MUTEX(writerMutex);

		TRACE_EXIT;
	}
};

class Server: public Socket
{
  public:
	Server (int port) :
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
};

class Client: public Socket
{
  public:
	Client (string ipAddr, int port) :
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
		if(!inet_pton(AF_INET, ipAddr.c_str(), &address)) {
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

		char helloworld[] = "Hello server, this is client.";
		sendMessage(MSG_TYPE_MARCO, sizeof(helloworld), (void*) helloworld);

		// Start the reader thread so we can start processing messages
		_initReaderThread();
	}
};

class EchoServer: public Server
{
  protected:
	virtual void
	handleMessage (uint32_t messageType, uint32_t length, void* buffer) {
		string text;
		switch (messageType) {
			case MSG_TYPE_MARCO:
				text = string((char*) buffer, length);
				cout << "Server: Received MARCO from server: \"" << text << "\" " << endl;
				sendMessage(MSG_TYPE_POLO, length, buffer);
				break;
			default:
				cerr << "Server: Received unknown message type " << messageType << endl;
				break;
		}
	}

  public:
	EchoServer(int port) :
		Server(port)
	{ }
};

class EchoClient: public Client
{
  protected:
	virtual void
	handleMessage (uint32_t messageType, uint32_t length, void* buffer) {
		string text;
		switch (messageType) {
			case MSG_TYPE_POLO:
				text = string((char*) buffer, length);
				cout << "Client: Received POLO from server: \"" << text << "\" " << endl;
				break;
			default: {
				cerr << "Client: Received unknown message type " << messageType << endl;
				break;
			}
		}
	}

  public:
	EchoClient (string ipAddress, int port) :
		Client(ipAddress, port)
	{ }
};


void
usage (char* basename)
{
	cout << "Usage: " << basename << " { server | client } [port]" << endl;
}

int
main (int argc, char* args[]) {

	try {
		string mode;
		int port;

		if (argc < 2) {
			usage(args[0]);
			return 0;
		}

		mode = args[1];

		if (argc >= 3) {
			istringstream iss(args[2]);
			iss >> port;
			if (port == 0) {
				cerr << "Bad port number: " << args[2];
			}
		} else {
			port = DEFAULT_PORT;
		}

		if (mode == "server")
		{
			shared_ptr<EchoServer> server(new EchoServer(port));

			cout << "Press enter to end." << endl;
			getchar(); // Wait for manual termination

			return 0;
		}
		else if (mode == "client")
		{
			shared_ptr<EchoClient> client(new EchoClient("10.0.0.13", port));
			
			string input;
			do {
				cout << "?> ";
				getline(cin, input);
				cout << "Sending " << input << endl;
				client->sendMessage(MSG_TYPE_MARCO, input.length(), (void*) input.c_str());
			}
			while (input != ".");

			return 0;
		}
		else
		{
			usage(args[0]);
			return 1;
		}
	}
	catch (runtime_error e)
	{
		cerr << "!! Caught exception:" << endl
		     << "!! " << e.what() << endl;
		return 1;
	}
}

