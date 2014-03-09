
#include <functional>

#include "Log.h"
#include "Sockets.h"

#include "chat_common.h"

using namespace std;

class ChatServerConnection: public Connection
{
	const message_handler_t handleHello =
		[this] (message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;
		MESSAGE("MSG_TYPE_HELLO received.");
		TRACE_EXIT;
	};

	const message_handler_t handleMessage =
		[this] (message_t type, message_len_t length, void* buffer)
	{
		TRACE_ENTER;
		MESSAGE("MSG_TYPE_MESSAGE received: " << string((char*) buffer, length));
		TRACE_EXIT;
	};

public:
	ChatServerConnection (int fd, in_addr_t remoteAddress, in_port_t remotePort):
		Connection (fd, remoteAddress, remotePort)
	{
		TRACE_ENTER;
		addDefaultMessageHandler(handleDefault);
		addMessageHandler(MSG_TYPE_HELLO, handleHello);
		addMessageHandler(MSG_TYPE_MESSAGE, handleMessage);
		TRACE_EXIT;
	}
};


class ChatServer: public Server {
public:
	ChatServer ()
	{ }

	shared_ptr<Connection>
	newConnection (int fd, in_addr_t remoteAddress, in_port_t remotePort)
	{
		TRACE_ENTER;
		return shared_ptr<Connection>(new ChatServerConnection(fd, remoteAddress, remotePort));
		TRACE_EXIT;
	}
};

int main (int argc, char *args[])
{
	try
	{
		ChatServer chatServer;
		chatServer.start(32123);
	}
	catch (runtime_error err)
	{
		ERROR("Caught runtime error:");
		ERROR(err.what());
	}
}

