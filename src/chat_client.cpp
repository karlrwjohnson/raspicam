
#include <functional>

#include "Log.h"
#include "Sockets.h"

#include "chat_common.h"

using namespace std;

class ChatClientConnection: public Connection
{
	message_handler_t handleHello =
		[this] (message_t type, message_len_t length, void* buffer)
	{
		MESSAGE("MSG_TYPE_HELLO received.");
	};

	message_handler_t handleMessage =
		[this] (message_t type, message_len_t length, void* buffer)
	{
		MESSAGE("MSG_TYPE_MESSAGE received: " << string((char*) buffer, length));
	};

	message_handler_t handleHelloAck =
		[this] (message_t type, message_len_t length, void* buffer)
	{
		MESSAGE("MSG_TYPE_HELLO_ACK received.");
	};

	message_handler_t handleMessageAck =
		[this] (message_t type, message_len_t length, void* buffer)
	{
		MESSAGE("MSG_TYPE_MESSAGE_ACK received: " << string((char*) buffer, length));
	};;

public:
	ChatClientConnection (int fd, in_addr_t remoteAddress, in_port_t remotePort):
		Connection (fd, remoteAddress, remotePort)
	{
		TRACE_ENTER;
		addDefaultMessageHandler(handleDefault);
		addMessageHandler(MSG_TYPE_HELLO, handleHello);
		addMessageHandler(MSG_TYPE_MESSAGE, handleMessage);
		addMessageHandler(MSG_TYPE_HELLO_ACK, handleHelloAck);
		addMessageHandler(MSG_TYPE_MESSAGE_ACK, handleMessageAck);
		TRACE_EXIT;
	}
};


class ChatClient: public Client {
public:
	ChatClient ()
	{ }

	shared_ptr<Connection>
	newConnection (int fd, in_addr_t remoteAddress, in_port_t remotePort)
	{
		TRACE_ENTER;
		return shared_ptr<Connection>(new ChatClientConnection(fd, remoteAddress, remotePort));
		TRACE_EXIT;
	}
};

int main (int argc, char *args[])
{
	try
	{
		ChatClient chatClient;
		shared_ptr<Connection> conn = chatClient.connect("127.0.0.1", 32123);

		conn->sendMessage(MSG_TYPE_HELLO);

		string input;
		do {
			getline(cin, input);
			MESSAGE("Sending \"" << input << '"');
			conn->sendMessage(MSG_TYPE_MESSAGE, input);
		} while(input != ".");
	}
	catch (runtime_error err)
	{
		ERROR("Caught runtime error:");
		ERROR(err.what());
	}
}

