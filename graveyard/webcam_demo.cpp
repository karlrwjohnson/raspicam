#include <sys/time.h>

#include "webcam_demo.h"
#include "WebcamViewer.h"

using namespace std;

const string DEFAULT_CAMERA = "/dev/video0";

static int
xioctl (int fd, int request, void *arg) {
	int r;
	do {
		r = ioctl(fd, request, arg);
	} while (r == -1 && errno == EINTR);
	return r;
}

class File {
  public:
	int fd;
	string filename;

	void
	_close()
	{
		if (fd != 0) {
			cout << "Closing file " << filename << "\n";
			close(fd);
			fd = 0;
		}
	}

	File () :
		filename (""),
		fd (0)
	{ }

	File (string filename_, int flags) :
		filename (filename_),
		fd (0)
	{
		fd = open(filename.c_str(), flags);

		if (fd == -1) {
			fd = 0;
			stringstream ss;
			ss << "Error opening " << filename << ": " << strerror(errno);
			throw runtime_error(ss.str());
		} else {
			cout << "Opened device " << filename << " as file descriptor " << fd << "\n";
		}
	}

	~File ()
	{
		_close();
	}
};

class MappedBuffer {
  public: // private:
  	int fd;
	size_t length;
	v4l2_buffer _buffer;

  public:
	int index;
	void *buf;

  public:

	MappedBuffer () :
		index  (-1),
		buf    (NULL),
		length (0)
	{ }

	MappedBuffer (int _fd, int _index) :
		fd     (_fd),
		index  (_index),
		buf    (NULL),
		length (0)
	{
		_buffer = {0};
		_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		_buffer.memory = V4L2_MEMORY_MMAP;
		_buffer.index = index;
		cout << "Querying buffer " << index << "...\n";
		if (xioctl(fd, VIDIOC_QUERYBUF, &_buffer)) {
			stringstream ss;
			ss << "Error querying buffer " << index;
			throw runtime_error(ss.str());
		}

		length = _buffer.length;

		errno=0;
		buf = mmap (NULL, _buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, _buffer.m.offset);
		int errno_ = errno;
		if (errno_) {
			buf = NULL;
			stringstream ss;
			ss << "mmap() failed on buffer " << index << " with code "
			   << errno_ << " (" << strerror(errno_) << ")";
			throw runtime_error(ss.str());
		}
	}

	~MappedBuffer ()
	{
		if (buf != NULL)
		{
			cout << "Unmapping buffer " << index << "\n";
			munmap(buf, length);
			buf = NULL;
		}
	}

	void enqueue ()
	{
		cout << "Enqueing buffer " << index << "\n";
		if (xioctl(fd, VIDIOC_QBUF, &_buffer)) {
			stringstream ss;
			ss << "Error enqueing buffer " << index;
			throw runtime_error(ss.str());
		}
	}

	void dequeue()
	{
		cout << "Retrieving frame from buffer " << index << "\n";
		if (xioctl(fd, VIDIOC_DQBUF, &_buffer) == -1) {
			stringstream ss;
			ss << "Error retrieving frame from buffer " << index << ": " << strerror(errno);
			throw runtime_error(ss.str());
		} else {
			cout << "Frame retrieved from buffer " << index << "\n";
		}
	}
};

int
main (int argc, char* args[]) {
	int status;
	int fd;

	vector< shared_ptr<MappedBuffer> > framebuffers;

	try
	{
		shared_ptr<File> webcam;
		shared_ptr<WebcamViewer> viewer;

		if (argc < 2) {
			webcam = shared_ptr<File>(new File(DEFAULT_CAMERA, O_RDWR));
		} else {
			webcam = shared_ptr<File>(new File(args[1], O_RDWR));
		}

		struct v4l2_capability caps = {0};
		if (xioctl(webcam->fd, VIDIOC_QUERYCAP, &caps) == -1) {
			throw runtime_error("Error querying capabilities");
			return 1;
		}

		printf("Capabilities:\n"
	       	   "\tdriver:       %s\n"
	       	   "\tcard:         %s\n"
	       	   "\tbus_info:     %s\n"
	       	   "\tversion:      %u.%u.%u\n"
	       	   "\tcapabilities: 0x%x\n"
	       	   "%s"
	       	   "\tdevice_caps:  0x%x\n",
	       	   caps.driver,
	       	   caps.card,
	       	   caps.bus_info,
	       	   (caps.version >> 16) & 0xff,
	       	   (caps.version >>  8) & 0xff,
	        	caps.version        & 0xff,
	       	   caps.capabilities,
	       	   v4l2_cap_flags(caps.capabilities).c_str(),
	       	   caps.device_caps
		);

		// Query all input devices
		cout << "Supported inputs on device:\n";
		for (int index = 0; index < 10; index++) {

			struct v4l2_input input;

			// Clear buffer
			memset(&input, 0, sizeof(v4l2_input));

			input.index = index;

			ioctl(webcam->fd, VIDIOC_ENUMINPUT, &input);

			if (errno == EINVAL) {
				cout << "... and there are no more inputs.\n";
				break;
			} else if (errno > 0) {
				stringstream ss;
				ss << "Error while enumerating inputs (" << strerror << ")";
				throw runtime_error(ss.str());
			} else {
				printf("%d: '%s'\n"
			       	   "\ttype:                %u (%s)\n"
			       	   "\taudioset:            0x%x\n"
			       	   "\ttuner:               %u\n"
			       	   "\tsupported format(s): 0x%llx (%s)\n"
			       	   "\tstatus:              %u ( %s%s%s)\n",
			       	   index,
			       	   input.name,
			       	   input.type,
			       	   ( input.type == V4L2_INPUT_TYPE_TUNER ? "tuner" :
			         	 input.type == V4L2_INPUT_TYPE_CAMERA ? "camera" :
			         	 "unknown"
			       	   ),
			       	   input.audioset,
			       	   input.tuner,
			       	   input.std, v4l2_std_flags(input.std).c_str(),
			       	   input.status,
			       	   ( input.status == V4L2_IN_ST_NO_POWER ? "no power " : ""),
			       	   ( input.status == V4L2_IN_ST_NO_COLOR ? "no color " : ""),
			       	   ( input.status == V4L2_IN_ST_NO_SIGNAL ? "no signal " : "")
				);
				if (input.std == 0) {
					cout << "\tYeah, no supported formats. :(\n";
				} else {
					cout << " !! Wait, there are supported formats; you just can't see them!\n";
				}
			}
		}

		int input_num = 0;
		cout << "Selecting input " << input_num << "\n";
		if (xioctl(webcam->fd, VIDIOC_S_INPUT, &input_num) == -1) {
			stringstream ss;
			ss << "Error selecing input " << input_num;
			throw runtime_error(ss.str());
		}

		cout << "Getting current format...\n";
		struct v4l2_format fmt_get = {0};
		fmt_get.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (xioctl(webcam->fd, VIDIOC_G_FMT, &fmt_get) == -1) {
			throw runtime_error("Unable to get the current image format\n");
		} else {
			printf("Current image format:\n"
		       	   "\twidth:        %d\n"
		       	   "\theight:       %d\n"
		       	   "\tpixelformat:  %c%c%c%c\n"
		       	   "\tfield:        %d\n"
		       	   "\tbytesperline: %d\n"
		       	   "\tsizeimage:    %d\n"
		       	   "\tcolorspace:   %d\n"
		       	   "\tpriv:         %d\n",
		       	   fmt_get.fmt.pix.width,
		       	   fmt_get.fmt.pix.height,
		       	   (fmt_get.fmt.pix.pixelformat >>  0 ) & 0xff,
		       	   (fmt_get.fmt.pix.pixelformat >>  8 ) & 0xff,
		       	   (fmt_get.fmt.pix.pixelformat >> 16 ) & 0xff,
		       	   (fmt_get.fmt.pix.pixelformat >> 24 ) & 0xff,
		       	   fmt_get.fmt.pix.field,
		       	   fmt_get.fmt.pix.bytesperline,
		       	   fmt_get.fmt.pix.sizeimage,
		       	   fmt_get.fmt.pix.colorspace,
		       	   fmt_get.fmt.pix.priv
			);
		}

		cout << "Setting format to 320x200px...\n";
		fmt_get.fmt.pix.width = 352;
		fmt_get.fmt.pix.height = 288;
		if (xioctl(webcam->fd, VIDIOC_S_FMT, &fmt_get) == -1) {
			throw runtime_error("Unable to set new image size\n");
		}

		cout << "Getting current format...\n";
		if (xioctl(webcam->fd, VIDIOC_G_FMT, &fmt_get) == -1) {
			throw runtime_error("Unable to get the current image format\n");
		} else {
			printf("Current image format:\n"
		       	   "\twidth:        %d\n"
		       	   "\theight:       %d\n"
		       	   "\tpixelformat:  %c%c%c%c\n"
		       	   "\tfield:        %d\n"
		       	   "\tbytesperline: %d\n"
		       	   "\tsizeimage:    %d\n"
		       	   "\tcolorspace:   %d\n"
		       	   "\tpriv:         %d\n",
		       	   fmt_get.fmt.pix.width,
		       	   fmt_get.fmt.pix.height,
		       	   (fmt_get.fmt.pix.pixelformat >>  0 ) & 0xff,
		       	   (fmt_get.fmt.pix.pixelformat >>  8 ) & 0xff,
		       	   (fmt_get.fmt.pix.pixelformat >> 16 ) & 0xff,
		       	   (fmt_get.fmt.pix.pixelformat >> 24 ) & 0xff,
		       	   fmt_get.fmt.pix.field,
		       	   fmt_get.fmt.pix.bytesperline,
		       	   fmt_get.fmt.pix.sizeimage,
		       	   fmt_get.fmt.pix.colorspace,
		       	   fmt_get.fmt.pix.priv
			);
		}

		// VIDIOC_ENUM_FRAMEINTERVALS
		/*struct v4l2_frmivalenum frameIntervalEnum = {0};
		frameIntervalEnum.index = 0;
		frameIntervalEnum.pixel_format = fmt_get.fmt.pix.pixelformat;
		frameIntervalEnum.width        = fmt_get.fmt.pix.width;
		frameIntervalEnum.height       = fmt_get.fmt.pix.height;
		if (xioctl(webcam->fd, VIDIOC_ENUM_FRAMEINTERVALS, frameIntervalEnum)) {
			throw runtime_error("Unable to enumerate supported frame intervals");
		}*/

		struct v4l2_requestbuffers req = {0};
		req.count = 10;
		req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		req.memory = V4L2_MEMORY_MMAP;

		cout << "Requesting " << req.count << " frame buffers...\n";
		status = xioctl(webcam->fd, VIDIOC_REQBUFS, &req);
		if (status == EINVAL) {
			throw runtime_error("Memory-map streaming is NOT supported.");
		} else if (status == -1) {
			throw runtime_error("Error requesting buffer");
		} else {
			cout << "\tMemory-map streaming IS supported.\n"
			     << "\tBuffers allocated: " << req.count << "\n";
		}

		for (int i = 0; i < req.count; i++) {
			framebuffers.push_back(shared_ptr<MappedBuffer>(new MappedBuffer(webcam->fd, i)));
			//framebuffers[i]->enqueue();
		}
		framebuffers[0]->enqueue();

		cout << "Starting capture...\n";
		if (xioctl(webcam->fd, VIDIOC_STREAMON, &(req.type))) {
			cout << "Error starting capture\n";
			return 1;
		}

		viewer = shared_ptr<WebcamViewer>(new WebcamViewer(
			fmt_get.fmt.pix.width,
			fmt_get.fmt.pix.height,
			"Yay, webcam!",
			v4l2sdl_fmt(fmt_get.fmt.pix.pixelformat)
		));

		timeval then, now;
		gettimeofday(&then, NULL);

		int currentBuffer = 0;
		while(1)
		{
			framebuffers[currentBuffer]->dequeue();
			framebuffers[1-currentBuffer]->enqueue();
			viewer->showFrame(framebuffers[currentBuffer]->buf, framebuffers[currentBuffer]->length);
			currentBuffer = 1 - currentBuffer;

			gettimeofday(&now, NULL);
			int dt = (now.tv_sec - then.tv_sec) * 1000000 + (now.tv_usec-then.tv_usec);
			printf("Framerate: %.2f fps\n", (1000000.0 / dt));
			then = now;

			// Check for termination. (This throws an exception when that happens.)
			viewer->checkEvents();
		}

/*		fd_set fds;
		FD_ZERO(&fds);
		for (int i = 0; i < req.count
		FD_SET(webcam->fd, &fds);
		struct timeval timeout = {0};
		timeout.tv_sec = 2;
		int r = select(webcam->fd+1, &fds, NULL, NULL, &timeout);
		cout << "Waiting for frame...\n";
		if (r == -1) {
			cout << "Error waiting for frame\n";
			return 1;
		}

		cout << "Retrieving frame...\n";
		//if (xioctl(webcam->fd, VIDIOC_DQBUF, &buf) == -1) {
		if (xioctl(webcam->fd, VIDIOC_DQBUF, &framebuffers[0]->buf) == -1) {
			stringstream ss;
			ss << "Error retrieving frame: " << strerror(errno);
			throw runtime_error(ss.str());
		} else {
			cout << "Frame retrieved.\n";
		}
		// */
	}
	catch (runtime_error e)
	{
		cerr << "!! Exception thrown: " << e.what() << "\n";
		return 1;
	}

	return 0;
}

