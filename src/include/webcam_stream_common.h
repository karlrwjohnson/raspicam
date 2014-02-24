

enum MSG_TYPE
{
	MSG_TYPE_START_STREAM,
	MSG_TYPE_PAUSE_STREAM,
	MSG_TYPE_QUERY_IMAGE_INFO,
	MSG_TYPE_IMAGE_INFO_RESPONSE,
	MSG_TYPE_FRAME
};

struct image_info
{
	uint32_t width;
	uint32_t height;
	uint32_t fmt;
}


