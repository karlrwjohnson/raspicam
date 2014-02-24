#include <unistd.h>
#include <sys/ioctl.h>
//#include <linux/ioctl.h> // If the above fails.
#include <fcntl.h> // open(), O_RDWR
#include <errno.h>
#include <string.h>
#include <linux/videodev2.h>
#include <sys/mman.h> // mmap(), PROT_READ and PROT_WRITE

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <string>
#include <vector>

std::string
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
	CHECK_FLAG(V4L2_CAP_VIDEO_M2M_MPLANE, "Is a video mem-to-mem device that supports multiplanar formats")
	CHECK_FLAG(V4L2_CAP_VIDEO_M2M, "Is a video mem-to-mem device")

	CHECK_FLAG(V4L2_CAP_TUNER, "has a tuner")
	CHECK_FLAG(V4L2_CAP_AUDIO, "has audio support")
	CHECK_FLAG(V4L2_CAP_RADIO, "is a radio device")
	CHECK_FLAG(V4L2_CAP_MODULATOR, "has a modulator")

	CHECK_FLAG(V4L2_CAP_READWRITE, "read/write systemcalls")
	CHECK_FLAG(V4L2_CAP_ASYNCIO, "async I/O")
	CHECK_FLAG(V4L2_CAP_STREAMING, "streaming I/O ioctls")

	CHECK_FLAG(V4L2_CAP_DEVICE_CAPS, "sets device capabilities field")
#undef CHECK_FLAG

	return ret.str();
}

std::string
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

