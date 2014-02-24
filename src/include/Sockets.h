
#include <arpa/inet.h>  // inet_pton

#include <pthread.h>    // multithreading
#include <string>       // strings

/**
 * Converts an IP address into a string
 */
std::string
ip2string (in_addr_t ipAddr);

class Socket {

  public:
	Socket ();

	~Socket ();

	/**
	 * Sends data through the socket
	 * @param type    Integer indicating the type of the message
	 * @param length  Number of bytes to send from *data
	 * @param data    Data to send
	 */
	void
	sendMessage (uint32_t type, size_t length, void* data);

	/**
	 * Waits for the thread which reads data from the server to terminate.
	 */
	void
	joinReaderThread ();

  protected:

	int fd;

	/**
	 * Called each time a complete message is received through the socket.
	 * This method must be overridden to process incoming messages.
	 */
	virtual void
	handleMessage (uint32_t type, uint32_t length, void* buffer) = 0;

  private:

	pthread_mutex_t writerMutex;

	pthread_t readerHandle;

	void
	_openSocket ();

	void
	_closeSocket ();

	/**
	 * Buffers message objects from the socket and passes them to the
	 * user-defined handler method
	 */
	void*
	_readerThread();

	/**
	 * Calls _readerThread on the specified function.
	 *   Workaround for the fact that you can't pass a pointer to a non-static
	 * member function to pthread_create().
	 */
	static void*
	_readerThreadCaller (void* socketObject);

  protected:
	void
	_initReaderThread();

};

class Server: public Socket
{
  public:
	Server (int port);
};

class Client: public Socket
{
  public:
	Client (std::string ipAddr, int port);
};

