



#include "webcam_stream_common.h"
#include "Log.h"
#include "Sockets.h"
#include "Thread.h"
#include "WebcamViewer.h"

using namespace std;

class WebcamClientConnection: public Connection
{
  private:

	shared_ptr<WebcamViewer> viewer;

	pthread_t viewerMutex;

	bool ViewerIsActive

  public
	WebcamClient (int fd, in_addr_t remoteAddress, in_port_t remotePort):
		Connection (fd, remoteAddress, remotePort);

	~WebcamClient ()
};

class WebcamClient: public Client
{
	shared_ptr<Connection>
	newConnection (int fd, in_addr_t remoteAddress, in_port_t remotePort);
};

