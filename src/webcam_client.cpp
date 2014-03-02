



#include "webcam_stream_common.h"
#include "Log.h"
#include "Sockets.h"
#include "WebcamViewer.h"

using namespace std;

class WebcamClient: public Client
{
  private:

	shared_ptr<WebcamViewer> viewer;

	pthread_t viewerMutex;

  protected:

	virtual void
	handleMessage (uint32_t messageType, uint32_t length, void* buffer)
	{
		switch (messageType)
		{
			case MSG_TYPE_STREAM_IS_STARTED:
			case MSG_TYPE_STREAM_IS_STOPPED:
			case MSG_TYPE_IMAGE_INFO_RESPONSE:
			case MSG_TYPE_SUPPORTED_FORMATS_RESPONSE:
			case MSG_TYPE_FRAME:
			case MSG_TYPE_
			case MSG_TYPE_
			case 
			case 
			case 
			case 
			case 
			case 
		}
	}
