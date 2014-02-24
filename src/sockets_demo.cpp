
#include <iostream>     // cout
#include <pthread.h>    // multithreading
#include <memory>       // shared_ptr
#include <stdexcept>    // exceptions
#include <sstream>      // stringstream (used by Log.h)
#include <string>       // strings

#include "Sockets.h"

#include "Log.h"

using namespace std;

const unsigned char _localhostIp[4] = { 127, 0, 0, 1 };
const in_addr_t LOCALHOST_IP_ADDR = *((in_addr_t*) _localhostIp);

const in_port_t DEFAULT_PORT = 32123;

enum MSG_TYPE
{
	MSG_TYPE_MARCO,
	MSG_TYPE_POLO
};

class EchoServer: public Server
{
  protected:
	virtual void
	handleMessage (uint32_t messageType, uint32_t length, void* buffer)
	{
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
	handleMessage (uint32_t messageType, uint32_t length, void* buffer)
	{
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

			// Wait for manual termination
			cout << "Press enter to end." << endl;
			getchar();

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

