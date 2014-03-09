#ifndef WEBCAM_H
#define WEBCAM_H

#include <memory>
#include <string>
#include <vector>
#include <utility> // pair

#include <linux/videodev2.h>

class File {
  public:
	int fd;
	std::string filename;

	void
	_close ();

	File (std::string filename_, int flags);

	~File ();
};

class MappedBuffer {
  private:
	v4l2_buffer _buffer;

  public:
  	int fd;
	size_t length;
	int index;
	void *data;

  public:

	MappedBuffer (int _fd, int _index);
	
	~MappedBuffer ();

	void
	enqueue ();

	void
	dequeue();
};

/**
 * Wrapper around a v4l2_buffer that automatically maps and unmaps it to memory
 * on {con,de}struction
 */
class Webcam {

  public:
	typedef uint32_t video_fmt_enum_t;

	typedef std::vector<struct v4l2_fmtdesc> fmtdesc_v;

	typedef std::pair<uint32_t, uint32_t> resolution_t;

	typedef std::vector<resolution_t> resolution_set;

  private:
	/** Some methods need a pointer to the constant defined in videodev2.h **/
	int v4l2_buf_type_video_capture;

	std::shared_ptr<File> device;

	std::vector< std::shared_ptr<MappedBuffer> > framebuffers;

	int currentBuffer;

	bool capturing;

  public:

	Webcam (std::string filename);

	~Webcam ();

	std::string
	getFilename ();

	std::shared_ptr<fmtdesc_v>
	getSupportedFormats ();

	video_fmt_enum_t
	getImageFormat ();

	void
	setImageFormat (video_fmt_enum_t fmt, uint32_t width, uint32_t height);

	void
	setImageFormat (video_fmt_enum_t fmt, resolution_t res);

	std::shared_ptr<resolution_set>
	getSupportedResolutions (video_fmt_enum_t format);

	resolution_t
	getResolution ();

	void
	setResolution (uint32_t width, uint32_t height);

	void
	setResolution (resolution_t res);

	void
	displayInfo ();

	void
	startCapture ();

	void
	stopCapture ();

	std::shared_ptr<MappedBuffer>
	getFrame();

	static std::string
	fmt2string (video_fmt_enum_t fmt);
};

#endif // WEBCAM_H

