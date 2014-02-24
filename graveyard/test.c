#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
//#include <linux/ioctl.h> // If the above fails.
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <linux/videodev2.h>

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

int main(int argc, char* args[]) {

	struct v4l2_input input;

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

	// Query all input devices
	printf("Supported inputs on device:\n");
	for(int index = 0; index < 10; index++) {

		// Clear buffer
		memset(&input, 0, sizeof(v4l2_input));

		// Select input with index `index`. The driver will fill in the rest of the struct.
		input.index = index;

		ioctl(fd, VIDIOC_ENUMINPUT, &input);

		if(errno == EINVAL) {
			printf("... and there are no more inputs.\n");
			break;
		} else if(errno > 0) {
			printf("Error: %s\n", strerror(errno));
		} else {
			printf("%d: '%s'\n"
			       "    type: %u (%s)\n"
			       "    audioset: %u\n"
			       "    tuner: %u\n"
			       "    supported format(s): 0x%llx\n"
			       "    status: %u ( %s%s%s)\n",
			       index,
			       input.name,
			       input.type,
			       ( input.type == V4L2_INPUT_TYPE_TUNER ? "tuner" :
			         input.type == V4L2_INPUT_TYPE_CAMERA ? "camera" :
			         "unknown"
			       ),
			       input.audioset,
			       input.tuner,
			       input.std,
			       input.status,
			       ( input.status == V4L2_IN_ST_NO_POWER ? "no power " : ""),
			       ( input.status == V4L2_IN_ST_NO_COLOR ? "no color " : ""),
			       ( input.status == V4L2_IN_ST_NO_SIGNAL ? "no signal " : "")
			);
			if(input.std == 0) {
				printf("Yeah, no supported formats. :(\n");
			} else {
				printf("00\n");
			}
		}
	}

	close(fd);

	free(deviceName);

}

