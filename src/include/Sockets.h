#ifndef SOCKETS_H
#define SOCKETS_H

#include <arpa/inet.h>  // inet_pton
#include <netinet/in.h> // in_addr_t, in_port_t

#include <functional>   // lambdas (:D)
#include <map>          // maps
#include <memory>       // shared_ptr
#include <list>         // doubly-linked lists
#include <pthread.h>    // multithreading
#include <string>       // strings
#include <vector>       // vectors

/**
 * Converts an IP address into a string
 *
 * @param ipAddr A binary representation of an IP address
 * @return       A human-readable string. Look, do I really need to explain this?
 */
std::string
ip2string (in_addr_t ipAddr);

typedef uint32_t message_t;
typedef uint32_t message_len_t;

/**
 * Wraps around a mutex, locks it, and unlocks it when it goes out of scope.
 * Kinda like how auto_ptr frees a pointer when it's destructed.
 */
class MutexLock {

	pthread_mutex_t *mutex_p;

	bool locked;

  public:
	/**
	 * Locks the mutex
	 * @param mutex  The mutex to lock
	 */
	MutexLock (pthread_mutex_t &mutex);

	/**
	 * Unlocks the mutex if unlock() hasn't been called already.
	 */
	~MutexLock ();

	/**
	 * May be used to unlock the mutex early. If this is used, the destructor
	 * does not unlock the mutex.
	 */
	void
	unlock();

	/**
	 * Re-locks the mutex after calling unlock(). If the mutex has already
	 * been locked once by this object, nothing happens.
	 */
	void
	relock();
};

class Connection {

  public:
	/**
	 * Message handler function prototype
	 *
	 * @param type    Type ID passed to sendMessage()
	 * @param length  Length of the message, in bytes
	 * @param buffer  Pointer to the message buffer
	 */
	//typedef void (Connection::* message_handler_t) (
	//		message_t type, message_len_t length, void* buffer
	//	);
	//typedef std::shared_ptr< std::function<void(message_t&, message_len_t&, void*)> > message_handler_t;
	typedef std::function<void(message_t&, message_len_t&, void*)> message_handler_t;

	/**
	 * A set of message handler function pointers.
	 * Not actually a set because apparently sets require their contents
	 * to be sortable, and functions don't implement the "<" operator.
	 */
	typedef std::list< const message_handler_t* > message_handler_set;

	/// A mapping between a message type and the set of functions to process it
	typedef std::map< message_t, message_handler_set > message_handler_map;

	/// IP address of the remote computer
	const in_addr_t remoteAddress;

	/// Port of the remote computer
	const in_port_t remotePort;

  protected:

	/// Sample message handler to notify the user about unhandled messages
	const message_handler_t handleDefault;

  private:

	/// File descriptor for the connection
	int fd;

	/// Mutex for writing to the connection
	pthread_mutex_t writerMutex;

	/// Handle for the reader thread
	pthread_t readerThreadHandle;

	/// Functions to process expected message types
	message_handler_map handlers;

	/// Functions to process unexpected message types
	message_handler_set defaultHandlers;

	/// Flag telling the reader thread to exit
	bool stopReadingFlag;

	/// Flag indicating that the connection is closed (set by the reader thread)
	bool connectionClosedFlag;

  public:

	Connection (int fd, in_addr_t remoteAddress, in_port_t remotePort);

	~Connection ();

	/// Closes the connection, terminating the reader thread.
	void 	
	close ();

	/// Begins listening for and processing messages in another thread
	void
	startReaderThread ();

	/// Waits for the thread which reads data from the server to terminate.
	void
	joinReaderThread ();

	/**
	 * Sends data through the socket
	 *
	 * @param type    Integer indicating the type of the message
	 * @param length  Number of bytes to send from *data
	 * @param data    Data to send
	 */
	void
	sendMessage (message_t type, size_t length, void* data);

	/**
	 * Wrapper that calls sendMessage(message_t, size_t, void*) on a string
	 * buffer. This only simplifies the sending process; the string shows up
	 * on the other side as raw binary data.
	 *
	 * @param type    Integer indicating the type of the message
	 * @param data    Data to send
	 */
	void
	sendMessage (message_t type, std::string data);

	/**
	 * Wrapper that calls sendMessage(message_t, size_t, void*) with no data.
	 *
	 * @param type    Integer indicating the type of the message
	 */
	void
	sendMessage (message_t type);

  protected:

	/**
	 * Registers a function to process a given type of message.
	 *
	 * Multiple functions may be registered for a given message type, but each
	 * function may be registered exactly once. Duplicated registration
	 * attempts fail silently.
	 *
	 * For a given message type, each handler will be run exactly once. There
	 * is no guarantee that a particular handler will be run before another
	 * one because a particular handler may have already been registered.
	 * 
	 * @param type     Type ID passed to sendMessage()
	 * @param handler  Function to run each time that type of message is received
	 */
	void
	addMessageHandler (message_t type, const message_handler_t& handler);

	/**
	 * Registers a function to process messages for which no handler has been
	 * defined.
	 *
	 * As with addMessageHandler(), each handler may only be registered once
	 * and each handler is only executed once.
	 * 
	 * @param handler  Function to run each time an unhandled message arrives
	 */
	void
	addDefaultMessageHandler (const message_handler_t& handler);

	/**
	 * Unregisters a function previously registered with a corresponding call to
	 * addMessageHandler().
	 *
	 * If it was not registered to begin with, nothing happens.
	 *
	 * @param type     Type ID passed to sendMessage()
	 * @param handler  Function to run each time that type of message is received
	 */
	void
	removeMessageHandler (message_t type, const message_handler_t& handler);

	/**
	 * Unregisters a function previously registered with a corresponding call to
	 * addDefaultMessageHandler().
	 *
	 * If it was not registered to begin with, nothing happens.
	 *
	 * @param type     Type ID passed to sendMessage()
	 * @param handler  Function to run each time that type of message is received
	 */
	void
	removeDefaultMessageHandler (const message_handler_t& handler);

  private:

	/**
	 * Buffers message objects from the socket and passes them to the
	 * user-defined handler method
	 */
	void
	readerThread (void* unused);
	
};

class Server
{
	int fd;

	typedef std::list< std::shared_ptr<Connection> > connection_list_t;

	/// List of currently active connections
	connection_list_t connections;

	/// Mutex for adding/removing elements to/from the list of connections
	pthread_mutex_t connectionsMutex;

	/// Flag to tell start() to stop accepting new connections
	bool stopAcceptingFlag;

	/// How many connections can wait in line for accept()
	const int INCOMING_CONNECTION_QUEUE_SIZE = 5;

  protected:

	/**
	 * Returns a contructed connection with the given file descriptor.
	 * This allows subclasses to extend Connection with their own behavior,
	 * i.e. their own handlers.
	 */
	virtual std::shared_ptr<Connection>
	newConnection (int fd, in_addr_t remoteAddress, in_port_t remotePort) = 0;

  public:

	Server ();

	~Server ();

	/**
	 * Opens an incoming socket at the specified port and accepts new connections.
	 * If it succeeds, the calling thread is suspended until it receives a signal
	 * to stop from the operating system, the incoming socket is broken or one of
	 * the threads tells it to stop.
	 *
	 * @param port  A number between and 1 (inclusive) and 2^16 (exclusive).
	 */
	void
	start (in_port_t port);

	/**
	 * If the server is running, stop accepting new connections
	 */
	void
	stop ();

	void
	forEachConnection (std::function<void(Connection&)> f);
};

class Client
{
	std::shared_ptr<Connection> connection;

  protected:

	/**
	 * Returns a contructed connection with the given file descriptor.
	 * This allows subclasses to extend Connection with their own behavior,
	 * i.e. their own handlers.
	 */
	virtual std::shared_ptr<Connection>
	newConnection (int fd, in_addr_t remoteAddress, in_port_t remotePort) = 0;
	
  public:
	Client ();

	~Client ();

	/**
	 * Attempts to connect to a server at the specified IP address and port.
	 */
	std::shared_ptr<Connection>
	connect (std::string ipAddress, in_port_t port);
};

#endif // SOCKETS_H

