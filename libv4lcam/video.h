#ifndef VIDEO_H
#define VIDEO_H

#include <linux/videodev2.h>

struct buffer {
	struct v4l2_requestbuffers req; // 请求/
	struct v4l2_buffer query;      // 获取/
	struct {              // 缓冲/
		void *start;
		size_t length;
		} *buf;
};

struct video {
	int fd;
	struct v4l2_format format;    // 视频帧格式
	struct buffer buffer;        // 视频缓冲
};

void video_init();
void video_quit();
void buffer_enqueue(int index);
void buffer_dequeue(int index);

#endif
