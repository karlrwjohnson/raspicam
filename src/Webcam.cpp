#include <errno.h>           // errno
#include <fcntl.h>           // open(), O_RDWR -- for opening /dev/video*
#include <unistd.h>          // close(), believe it or not.

#include <cstdio>            // printf()
#include <cstring>           // strerror()
#include <iostream>          // cout
#include <memory>            // shared_ptr()
#include <stdexcept>         // runtime_exception
#include <sstream>           // stringstream
#include <string>            // strings
#include <vector>            // vectors

#include <linux/videodev2.h> // video4linux v2, of course

#include <sys/ioctl.h> 		 // ioctl()
#include <sys/mman.h>        // mmap(), PROT_READ and PROT_WRITE

#include "Webcam.h"
#include "Log.h"

using namespace std;

/**
 * Maps a V4L2_CAP_* flag to its associated string.
 * These flags and strings are ripped straight out of videodev2.h.
 * This function and its brother aren't terribly useful anymore, but they were
 * when I was figuring out how to use V4L2.
 */
string
v4l2_cap_flags(int32_t flags)
{
	std::stringstream ret;

#define CHECK_FLAG(flagname, desc) \
	if (flags & flagname) ret << "\t * " << #flagname << " : " desc << "\n";

	CHECK_FLAG(V4L2_CAP_VIDEO_CAPTURE, "Is a video capture device")
	CHECK_FLAG(V4L2_CAP_VIDEO_OUTPUT, "Is a video output device")
	CHECK_FLAG(V4L2_CAP_VIDEO_OVERLAY, "Can do video overlay")
	CHECK_FLAG(V4L2_CAP_VBI_CAPTURE, "Is a raw VBI capture device")
	CHECK_FLAG(V4L2_CAP_VBI_OUTPUT, "Is a raw VBI output device")
	CHECK_FLAG(V4L2_CAP_SLICED_VBI_CAPTURE, "Is a sliced VBI capture device")
	CHECK_FLAG(V4L2_CAP_SLICED_VBI_OUTPUT, "Is a sliced VBI output device")
	CHECK_FLAG(V4L2_CAP_RDS_CAPTURE, "RDS data capture")
	CHECK_FLAG(V4L2_CAP_VIDEO_OUTPUT_OVERLAY, "Can do video output overlay")
	CHECK_FLAG(V4L2_CAP_HW_FREQ_SEEK, "Can do hardware frequency seek ")
	CHECK_FLAG(V4L2_CAP_RDS_OUTPUT, "Is an RDS encoder")

	CHECK_FLAG(V4L2_CAP_VIDEO_CAPTURE_MPLANE, "Is a video capture device that supports multiplanar formats")
	CHECK_FLAG(V4L2_CAP_VIDEO_OUTPUT_MPLANE, "Is a video output device that supports multiplanar formats")
//	CHECK_FLAG(V4L2_CAP_VIDEO_M2M_MPLANE, "Is a video mem-to-mem device that supports multiplanar formats")
//	CHECK_FLAG(V4L2_CAP_VIDEO_M2M, "Is a video mem-to-mem device")

	CHECK_FLAG(V4L2_CAP_TUNER, "has a tuner")
	CHECK_FLAG(V4L2_CAP_AUDIO, "has audio support")
	CHECK_FLAG(V4L2_CAP_RADIO, "is a radio device")
	CHECK_FLAG(V4L2_CAP_MODULATOR, "has a modulator")

	CHECK_FLAG(V4L2_CAP_READWRITE, "read/write systemcalls")
	CHECK_FLAG(V4L2_CAP_ASYNCIO, "async I/O")
	CHECK_FLAG(V4L2_CAP_STREAMING, "streaming I/O ioctls")

#undef CHECK_FLAG

	return ret.str();
}

string
v4l2_std_flags(int64_t flags)
{
	std::stringstream ret;

#define CHECK_FLAG(flagname) \
	if(flags && flagname) ret << #flagname << " ";

	CHECK_FLAG(V4L2_STD_PAL_B)
	CHECK_FLAG(V4L2_STD_PAL_B1)
	CHECK_FLAG(V4L2_STD_PAL_G)
	CHECK_FLAG(V4L2_STD_PAL_H)
	CHECK_FLAG(V4L2_STD_PAL_I)
	CHECK_FLAG(V4L2_STD_PAL_D)
	CHECK_FLAG(V4L2_STD_PAL_D1)
	CHECK_FLAG(V4L2_STD_PAL_K)

	CHECK_FLAG(V4L2_STD_PAL_M)
	CHECK_FLAG(V4L2_STD_PAL_N)
	CHECK_FLAG(V4L2_STD_PAL_Nc)
	CHECK_FLAG(V4L2_STD_PAL_60)

	CHECK_FLAG(V4L2_STD_NTSC_M)
	CHECK_FLAG(V4L2_STD_NTSC_M_JP)
	CHECK_FLAG(V4L2_STD_NTSC_443)
	CHECK_FLAG(V4L2_STD_NTSC_M_KR)

	CHECK_FLAG(V4L2_STD_SECAM_B)
	CHECK_FLAG(V4L2_STD_SECAM_D)
	CHECK_FLAG(V4L2_STD_SECAM_G)
	CHECK_FLAG(V4L2_STD_SECAM_H)
	CHECK_FLAG(V4L2_STD_SECAM_K)
	CHECK_FLAG(V4L2_STD_SECAM_K1)
	CHECK_FLAG(V4L2_STD_SECAM_L)
	CHECK_FLAG(V4L2_STD_SECAM_LC)

	CHECK_FLAG(V4L2_STD_ATSC_8_VSB)
	CHECK_FLAG(V4L2_STD_ATSC_16_VSB)
#undef CHECK_FLAG

	return ret.str();
}


static int
xioctl (int fd, int request, void *arg) {
	int r;
	do {
		r = ioctl(fd, request, arg);
	} while (r == -1 && errno == EINTR);
	return r;
}

///// File /////

	void
	File::_close ()
	{
		if (fd != 0) {
			cout << "Closing file " << filename << "\n";
			close(fd);
			fd = 0;
		}
	}

	File::File (string filename_, int flags) :
		filename (filename_),
		fd (0)
	{
		fd = open(filename.c_str(), flags);

		if (fd == -1) {
			fd = 0;
			THROW_ERROR("Error opening " << filename << ": " << strerror(errno));
		} else {
			TRACE("Opened device " << filename << " as file descriptor " << fd);
		}
	}

	File::~File ()
	{
		_close();
	}

///// MappedBuffer /////

	MappedBuffer::MappedBuffer (int _fd, int _index) :
		fd     (_fd),
		index  (_index),
		data   (NULL),
		length (0)
	{
		_buffer = {0};
		_buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		_buffer.memory = V4L2_MEMORY_MMAP;
		_buffer.index = index;

		TRACE("Getting information on frame buffer " << index);
		if (xioctl(fd, VIDIOC_QUERYBUF, &_buffer)) {
			THROW_ERROR("Error getting information on buffer " << index << ": " << strerror(errno));
		}

		length = _buffer.length;

		data = mmap (NULL, _buffer.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, _buffer.m.offset);
		if (errno) {
			THROW_ERROR("Failed to map buffer " << index << ": " << strerror(errno));
		}
	}

	MappedBuffer::~MappedBuffer ()
	{
		if (data != NULL)
		{
			TRACE("Unmapping buffer " << index)
			munmap(data, length);
			data = NULL;
		}
	}

	void
	MappedBuffer::enqueue ()
	{
		TRACE("Enqueing buffer " << index);
		if (xioctl(fd, VIDIOC_QBUF, &_buffer)) {
			THROW_ERROR("Error enqueing buffer " << index << ": " << strerror(errno));
		}
	}

	void
	MappedBuffer::dequeue()
	{
		TRACE("Retrieving frame from buffer " << index);
		if (xioctl(fd, VIDIOC_DQBUF, &_buffer)) {
			THROW_ERROR("Error retrieving frame from buffer " << index << ": " << strerror(errno));
		} else {
			TRACE("Frame retrieved from buffer " << index);
		}
	}

///// Webcam /////

	Webcam::Webcam (string filename) :
		device(shared_ptr<File>(new File(filename, O_RDWR))),
		v4l2_buf_type_video_capture(V4L2_BUF_TYPE_VIDEO_CAPTURE)
	{
		int input_num = 0;
		cout << "Selecting input " << input_num << "\n";
		if (xioctl(device->fd, VIDIOC_S_INPUT, &input_num)) {
			THROW_ERROR("Error selecing input " << input_num << ": " << strerror(errno));
		}
	}

	Webcam::~Webcam ()
	{
		if (capturing) {
			stopCapture();
		}
	}

	string
	Webcam::getFilename ()
	{
		return device->filename;
	}

	shared_ptr<Webcam::fmtdesc_v>
	Webcam::getSupportedFormats ()
	{
		TRACE("Getting list of supported formats...");

		shared_ptr<fmtdesc_v> ret = shared_ptr<fmtdesc_v>( new fmtdesc_v() );

		for (int i = 0; ; i++)
		{
			struct v4l2_fmtdesc formatDesc;
			memset(&formatDesc, 0, sizeof(formatDesc));
			formatDesc.index = i;
			formatDesc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

			int code = xioctl(device->fd, VIDIOC_ENUM_FMT, &formatDesc);
			if (code && errno == EINVAL)
			{
				TRACE("Driver reports no more than " << i << " supported formats");
				break;
			}
			else if (code)
			{
				THROW_ERROR("Unable to get list of supported formats from driver: "
				         << strerror(errno));
			}
			else
			{
				ret->push_back(formatDesc);
			}
		}

		return ret;
	}

	uint32_t
	Webcam::getImageFormat ()
	{
		struct v4l2_format fmt_get;
        memset(&fmt_get, 0, sizeof(v4l2_format));
		fmt_get.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (xioctl(device->fd, VIDIOC_G_FMT, &fmt_get)) {
			THROW_ERROR("Unable to get the current image format: " << strerror(errno));
		}

		return fmt_get.fmt.pix.pixelformat;
	}

	void
	Webcam::setImageFormat (video_fmt_enum_t fmt, uint32_t width, uint32_t height)
	{
		TRACE("Getting previous format information");

		struct v4l2_format format;
        memset(&format, 0, sizeof(v4l2_format));
		format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (xioctl(device->fd, VIDIOC_G_FMT, &format)) {
			THROW_ERROR("Unable to get the current image format: "
			         << strerror(errno));
		}

		TRACE("Setting image format and resolution to " fmt2string(fmt) << ", "
		   << width << "x" << height << "px");
		format.fmt.pix.width = width;
		format.fmt.pix.height = height;
		format.fmt.pix.pixelformat = fmt;
		if (xioctl(device->fd, VIDIOC_S_FMT, &format)) {
			THROW_ERROR("Unable to set new image format and resolution to "
			         << fmt2string(fmt) << ", " << width << "x" << height << "px: "
			         << strerror(errno));
		}
	}

	void
	Webcam::setImageFormat (video_fmt_enum_t fmt, resolution_t res)
	{
		setImageFormat(fmt, res.first, res.second);
	}

	shared_ptr<Webcam::resolution_set>
	Webcam::getSupportedResolutions (Webcam::video_fmt_enum_t format)
	{
		TRACE("Getting list of supported resolutions...");

		shared_ptr<resolution_set> ret = shared_ptr<resolution_set>( new resolution_set() );

		for (int i = 0; ; i++)
		{
			struct v4l2_frmsizeenum resolution;
			memset(&resolution, 0, sizeof(resolution));
			resolution.index = i;
			resolution.pixel_format = format;

			int code = xioctl(device->fd, VIDIOC_ENUM_FRAMESIZES, &resolution);
			if (code && errno == EINVAL)
			{
				TRACE("Driver reports no more than " << i << " supported resolutions");
				break;
			}
			else if (code)
			{
				THROW_ERROR("Unable to get list of supported formats from driver: "
				         << strerror(errno));
			}
			else if (resolution.type == V4L2_FRMSIZE_TYPE_STEPWISE ||
			         resolution.type == V4L2_FRMSIZE_TYPE_CONTINUOUS)
			{
				THROW_ERROR("Driver reports that the camera supports a "
				            "step-wise or continuous range of resolutions. "
				            "I don't want to support that right now.");
			}
			else
			{
				ret->push_back(resolution_t(resolution.discrete.width,
				                            resolution.discrete.height));
			}
		}

		return ret;
	}

	Webcam::resolution_t
	Webcam::getResolution ()
	{
		struct v4l2_format fmt_get;
        memset(&fmt_get, 0, sizeof(v4l2_format));
		fmt_get.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (xioctl(device->fd, VIDIOC_G_FMT, &fmt_get)) {
			THROW_ERROR("Unable to get the current image format: " << strerror(errno));
		}

		return resolution_t(fmt_get.fmt.pix.width, fmt_get.fmt.pix.height);
	}

	void
	Webcam::setResolution (uint32_t width, uint32_t height)
	{
		TRACE("Getting previous format information");

		struct v4l2_format format;
        memset(&format, 0, sizeof(v4l2_format));
		format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (xioctl(device->fd, VIDIOC_G_FMT, &format)) {
			THROW_ERROR("Unable to get the current image format: "
			         << strerror(errno));
		}

		TRACE("Setting dimensions to " << width << "x" << height << "px");
		format.fmt.pix.width = width;
		format.fmt.pix.height = height;
		if (xioctl(device->fd, VIDIOC_S_FMT, &format)) {
			THROW_ERROR("Unable to set new image size to " << width << "x"
			         << height << "px: " << strerror(errno));
		}
	}

	void
	Webcam::setResolution (resolution_t res)
	{
		setResolution(res.first, res.second);
	}

	void
	Webcam::displayInfo ()
	{
		printf("Information on webcam %s:\n", device->filename.c_str());

		// Display general information
		struct v4l2_capability caps = {0};
		if (xioctl(device->fd, VIDIOC_QUERYCAP, &caps)) {
			THROW_ERROR("Error querying capabilities: " << strerror(errno));
		}

		printf("Capabilities:\n"
	       	   "\tdriver:       %s\n"
	       	   "\tcard:         %s\n"
	       	   "\tbus_info:     %s\n"
	       	   "\tversion:      %u.%u.%u\n"
	       	   "\tcapabilities: 0x%x\n"
	       	   "%s"
/*	       	   "\tdevice_caps:  0x%x\n"*/,
	       	   caps.driver,
	       	   caps.card,
	       	   caps.bus_info,
	       	   (caps.version >> 16) & 0xff,
	       	   (caps.version >>  8) & 0xff,
	        	caps.version        & 0xff,
	       	   caps.capabilities,
	       	   v4l2_cap_flags(caps.capabilities).c_str()/*,
	       	   caps.device_caps*/
		);

		// Query all input devices
		cout << "Supported inputs on device:\n";
		for (int index = 0; index < 10; index++) {

			struct v4l2_input input;

			// Clear buffer
			memset(&input, 0, sizeof(v4l2_input));

			input.index = index;

			ioctl(device->fd, VIDIOC_ENUMINPUT, &input);

			if (errno == EINVAL) {
				cout << "... and there are no more inputs.\n";
				break;
			} else if (errno > 0) {
				THROW_ERROR("Error while enumerating inputs: " << strerror(errno));
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

		// Display current image format
		struct v4l2_format fmt_get;
        memset(&fmt_get, 0, sizeof(v4l2_format));
		fmt_get.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		cout << "Getting current format...\n";
		if (xioctl(device->fd, VIDIOC_G_FMT, &fmt_get)) {
			THROW_ERROR("Unable to get the current image format: " << strerror(errno));
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
	}

	void
	Webcam::startCapture ()
	{
		struct v4l2_requestbuffers req = {0};
		req.count = 2;
		req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		req.memory = V4L2_MEMORY_MMAP;

		cout << "Requesting " << req.count << " frame buffers...\n";
		xioctl(device->fd, VIDIOC_REQBUFS, &req);
		if (errno == EINVAL) {
			THROW_ERROR("Memory-map streaming is NOT supported.");
		} else if (errno) {
			THROW_ERROR("Error requesting buffer: " << strerror(errno));
		} else {
			cout << "\tMemory-map streaming IS supported.\n"
			     << "\tBuffers allocated: " << req.count << "\n";
		}

		for (int i = 0; i < req.count; i++) {
			framebuffers.push_back(shared_ptr<MappedBuffer>(new MappedBuffer(device->fd, i)));
		}

		// Code for multi-buffer streaming. I don't know how to do this, and so far
		// I've been limited by the image sensor, so I figure two buffers are enough.
/*		fd_set fds;
		FD_ZERO(&fds);
		for (int i = 0; i < req.count
		FD_SET(device->fd, &fds);
		struct timeval timeout = {0};
		timeout.tv_sec = 2;
		int r = select(device->fd+1, &fds, NULL, NULL, &timeout);
		cout << "Waiting for frame...\n";
		if (r == -1) {
			cout << "Error waiting for frame\n";
			return 1;
		}

		cout << "Retrieving frame...\n";
		//if (xioctl(device->fd, VIDIOC_DQBUF, &data)) {
		if (xioctl(device->fd, VIDIOC_DQBUF, &framebuffers[0]->data)) {
			THROW_ERROR("Error retrieving frame: " << strerror(errno));
		} else {
			cout << "Frame retrieved.\n";
		}
		// */

		cout << "Starting capture...\n";
		if (xioctl(device->fd, VIDIOC_STREAMON, &(req.type))) {
			THROW_ERROR("Error starting capture: " << strerror(errno));
		}

		capturing = true;

		// Get the camera started on its first frame
		currentBuffer = 0;
		framebuffers[1 - currentBuffer]->enqueue();
	}

	void
	Webcam::stopCapture ()
	{
		// Tell the webcam to stop
		cout << "Stopping capture...\n";
		if (xioctl(device->fd, VIDIOC_STREAMOFF, &v4l2_buf_type_video_capture)) {
			THROW_ERROR("Error stopping capture: " << strerror(errno));
		}

		// I suppose we should deallocate the framebuffers as well.
		// Heck, the destrctors can take care of it. Just overwrite the
		// array to break the reference.
		framebuffers = vector< shared_ptr<MappedBuffer> >();

		capturing = false;
	}

	shared_ptr<MappedBuffer>
	Webcam::getFrame() {
		currentBuffer = 1 - currentBuffer;
		framebuffers[currentBuffer]->dequeue();
		framebuffers[1 - currentBuffer]->enqueue();
		return framebuffers[0];
	}

	string
	Webcam::fmt2string (video_fmt_enum_t fmt)
	{
		char ret[4] = {
			static_cast<uint8_t>(( fmt >>  0 ) & 0xff),
			static_cast<uint8_t>(( fmt >>  8 ) & 0xff),
			static_cast<uint8_t>(( fmt >> 16 ) & 0xff),
			static_cast<uint8_t>(( fmt >> 24 ) & 0xff)
		};
		return ret;
	}

