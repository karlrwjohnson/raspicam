#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
//#include <linux/ioctl.h> // If the above fails.
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <linux/videodev2.h>
#include <sys/mman.h> // PROT_READ and PROT_WRITE

#include "enum.h"

/*
	Usage: int ioctl(int d, unsigned long request, ...);

	ioctl error codes:

	EBADF		d is not a valid descriptor.

	EFAULT		argp references an inaccessible memory area.

	EINVAL		Request or argp is not valid.

	ENOTTY		d is not associated with a character special device.

	ENOTTY		The specified request does not apply to the kind of object
				that the descriptor d references.
*/

int fd;

char* deviceName;

static int xioctl(int fd, int request, void *arg) {
	int r;
	do {
		r = ioctl(fd, request, arg);
	} while (r == -1 && errno == EINTR);
	return r;
}

int main(int argc, char* args[]) {
	int status;


	if(argc < 2) {
		deviceName = strdup("/dev/video0");
	} else {
		deviceName = strdup(args[1]);
	}

	if((fd = open(deviceName, O_RDWR)) == -1) {
		printf("Error opening device %s: %s\n", deviceName, strerror(errno));
	} else {
		printf("Opened device %s as file descriptor %d\n", deviceName, fd);
	}

	struct v4l2_capability caps = {0};
	if( xioctl(fd, VIDIOC_QUERYCAP, &caps) == -1 ) {
		printf("Error querying capabilities\n");
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
	printf("Supported inputs on device:\n");
	for(int index = 0; index < 10; index++) {

		struct v4l2_input input;

		// Clear buffer
		memset(&input, 0, sizeof(v4l2_input));

		input.index = index;

		ioctl(fd, VIDIOC_ENUMINPUT, &input);

		if(errno == EINVAL) {
			printf("... and there are no more inputs.\n");
			break;
		} else if(errno > 0) {
			printf("Error: %s\n", strerror(errno));
		} else {
			printf("%d: '%s'\n"
			       "    type:                %u (%s)\n"
			       "    audioset:            0x%x\n"
			       "    tuner:               %u\n"
			       "    supported format(s): 0x%llx (%s)\n"
			       "    status:              %u ( %s%s%s)\n",
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
			if(input.std == 0) {
				printf("Yeah, no supported formats. :(\n");
			} else {
				printf("Wait, there are supported formats; you just can't see them!\n");
			}
		}
	}

	printf("Selecting input %d\n", 0);
	int input_num = 0;
	if(xioctl(fd, VIDIOC_S_INPUT, &input_num) == -1) {
		printf("Error setting input\n");
		return 1;
	}

	// Irrelevant for webcams
	/*printf("Getting current video standard...\n");
	v4l2_std_id std_id;
	if(xioctl(fd, VIDIOC_G_STD, &std_id) == -1) {
		printf("Error getting standard\n");
		return 1;
	} else {
		printf("  Current standard is 0x%llx, std_id\n");
	}*/

	printf("Getting current format...\n");
	struct v4l2_format fmt_get = {0};
	fmt_get.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if(xioctl(fd, VIDIOC_G_FMT, &fmt_get) == -1) {
		printf("!! Unable to get the current image format\n");
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

	/*
		Link: http://www.linuxtv.org/downloads/legacy/video4linux/API/V4L2_API/spec-single/v4l2.html#rw

		At this point, I should be able to use the read() method to capture a single frame
		at a time. According to the spec (link above), this might have worse performance
		than buffer switching, but it's simpler. It also doesn't account for dropped frames,
		but I REALLY don't care about that.

		UPDATE: scratch that. It doesn't support the V4L2_CAP_READWRITE flag, so that's out.
	*/

	/*struct v4l2_format fmt = {0};
	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt.fmt.pix.width = 320;
	fmt.fmt.pix.height = 240;
	fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
	fmt.fmt.pix.field = V4L2_FIELD_NONE;

	printf("Setting pixel format...\n");
	if(xioctl(fd, VIDIOC_S_FMT, &fmt) == -1) {
		printf("Error setting pixel format\n");
		return 1;
	}*/

	printf("Requesting buffer...\n");
	struct v4l2_requestbuffers req = {0};
	req.count = 1;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	status = xioctl(fd, VIDIOC_REQBUFS, &req);
	if (status == EINVAL) {
		printf("!! Memory-map streaming is NOT supported.\n");
		return 1;
	} else if (status == -1) {
		printf("Error requesting buffer\n");
		return 1;
	} else {
		printf("  Memory-map streaming IS supported.\n");
		printf("  Buffers allocated: %d\n", req.count);
	}

	struct v4l2_buffer buf = {0};
	buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	buf.memory = V4L2_MEMORY_MMAP;
	buf.index = 0;//bufferindex;
	printf("Querying buffer...\n");
	if(xioctl(fd, VIDIOC_QUERYBUF, &buf)) {
		printf("Error querying buffer\n");
		return 1;
	}

	void* buffer = mmap (NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);

return 0;
	printf("Starting capture...\n");
	if(xioctl(fd, VIDIOC_STREAMON, &buf.type)) {
		printf("Error starting capture\n");
		return 1;
	}

	fd_set fds;
	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	struct timeval tv = {0};
	tv.tv_sec = 2;
	int r = select(fd+1, &fds, NULL, NULL, &tv);
	printf("Waiting for frame...\n");
	if(r == -1) {
		printf("Error waiting for frame\n");
		return 1;
	}

	printf("Retrieving frame...\n");
	if(xioctl(fd, VIDIOC_DQBUF, &buf) == -1) {
		printf("Error retrieving frame\n");
		return 1;
	}

	close(fd);

	free(deviceName);

}

