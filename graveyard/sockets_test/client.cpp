
#include <arpa/inet.h>  // inet_pton()

#include <net/if.h>     // struct ifreq
#include <netinet/in.h> // struct sockaddr_in

#include <sys/ioctl.h>  // ioctl()
#include <sys/socket.h> // socket()

#include <errno.h>    // errno
#include <cstdio>     // printf()
#include <cstring>    // strerror()
#include <iostream>   // cout
#include <stdexcept>  // exceptions
#include <sstream>    // stringstream (used by Log.h)
#include <string>     // strings
#include <vector>     // vector
#include <unistd.h>   // close()

#include "../Log.h"

using namespace std;

const unsigned char _localhostIp[4] = { 127, 0, 0, 1 };
const in_addr_t LOCALHOST_IP_ADDR = *((in_addr_t*) _localhostIp);

const in_port_t DEFAULT_PORT = 32123;

string
ip2string (in_addr_t ipAddr) {
	uint8_t* ipAddr_p = (uint8_t*) &ipAddr;
	uint32_t a = ipAddr_p[0];
	uint32_t b = ipAddr_p[1];
	uint32_t c = ipAddr_p[2];
	uint32_t d = ipAddr_p[3];
	stringstream ss;
	ss << a << '.' << b << '.' << c << '.' << d;
	return ss.str();
}

int
main (int argc, char* args[]) {

	try {

		cout << "Opening socket" << endl;
		int sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd == -1) {
			THROW_ERROR("Failed to open socket.");
		}

		sockaddr_in serverAddress;
		socklen_t   serverAddressLength = sizeof(serverAddress);
		memset(&serverAddress, 0, sizeof(serverAddress));
		serverAddress.sin_family = AF_INET;
		serverAddress.sin_port = htons(DEFAULT_PORT);
		//serverAddress.sin_addr.s_addr = htonl(LOCALHOST_IP_ADDR);

		if(!inet_pton(AF_INET, "10.0.0.13", &serverAddress.sin_addr)) {
			THROW_ERROR("inet_pton() failed: " << strerror(errno)
			          << ". Is the address bad?");
		}

		cout << "Connecting to server with IP address "
	     	 << ip2string(ntohl(serverAddress.sin_addr.s_addr))
	     	 << ", port " << ntohs(serverAddress.sin_port) << endl;
		if (connect(sockfd, (sockaddr *) &serverAddress, sizeof(serverAddress))) {
			THROW_ERROR("connect() failed: " << strerror(errno));
		}


		/*cout << "Received connection from client at "
		     << ip2string(ntohl(clientAddress.sin_addr.s_addr))
		     << ", port " << ntohs(clientAddress.sin_port) << endl;*/

		close(sockfd);

	}
	catch (runtime_error e)
	{
		cerr << "!! Caught exception: " << e.what() << endl;
	}
}
