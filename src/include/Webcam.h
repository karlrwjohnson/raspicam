
#include <memory>
#include <string>
#include <vector>

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

	/** Some methods need a pointer to the constant defined in videodev2.h **/
	int v4l2_buf_type_video_capture;

	std::shared_ptr<File> device;

	std::vector< std::shared_ptr<MappedBuffer> > framebuffers;

	int currentBuffer;

	bool capturing;

  public:

	Webcam (std::string filename);

	~Webcam ();

	void
	setDimensions (uint32_t width, uint32_t height);

	std::vector<uint32_t>
	getDimensions ();

	uint32_t
	getImageFormat ();

	void
	displayInfo ();

	void
	startCapture ();

	void
	stopCapture ();

	std::shared_ptr<MappedBuffer>
	getFrame();
};

