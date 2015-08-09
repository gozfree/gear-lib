#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include "video.h"
#include "opt.h"

#define BUFFER_NUM 5

static int stream_flag = V4L2_BUF_TYPE_VIDEO_CAPTURE;
extern struct options opt;
struct video video;

static void video_open();
static void video_close();
static void video_set_format();
static void video_streamon();
static void video_streamoff();
static void buffer_init();
static void buffer_free();
static void buffer_request();
static void buffer_mmap(int index);

static void
video_open()
{
	int i, fd;
	char device[13];

	for (i = 0; i < 99; i++) {
		sprintf(device, "%s%d", "/dev/video", i);
		fd = open(device, O_RDWR);
		if (fd != -1) {
			if (opt.verbose) {
				printf("open %s success\n", device);
			}
			break;
		}
	}
	if (i == 100) {
		perror("video open fail");
		exit(EXIT_FAILURE);
	}
	video.fd = fd;
}

static void
video_close()
{
	  close(video.fd);
}

static void
video_set_format()
{
	memset(&video.format, 0, sizeof(video.format));
	video.format.type = stream_flag;
	video.format.fmt.pix.width = opt.width;
	video.format.fmt.pix.height = opt.height;
	video.format.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
	if (ioctl(video.fd, VIDIOC_S_FMT, &video.format) == -1) {
		perror("VIDIOC_S_FORMAT");
		exit(EXIT_FAILURE);
	}
}

static void
video_streamon()
{
	if (ioctl(video.fd, VIDIOC_STREAMON, &stream_flag) == -1) {
		if (errno == EINVAL) {
			perror("streaming i/o is not support");
		}
		else {
			perror("VIDIOC_STREAMON");
		}
		exit(EXIT_FAILURE);
	}
}

static void
video_streamoff()
{
	if (ioctl(video.fd, VIDIOC_STREAMOFF, &stream_flag) == -1) {
		if (errno == EINVAL) {
			perror("streaming i/o is not support");
		}
		else {
			perror("VIDIOC_STREAMOFF");
		}
		exit(EXIT_FAILURE);
	}
}

static void
buffer_init()
{
	int i;

	buffer_request();
	for (i = 0; i < video.buffer.req.count; i++) {
		buffer_mmap(i);
		buffer_enqueue(i);
	}
}

static void
buffer_free()
{
	int i;

	for (i = 0; i < video.buffer.req.count; i++) {
		munmap(video.buffer.buf[i].start, video.buffer.buf[i].length);
	}
	free(video.buffer.buf);
}

static void
buffer_request()
{

	memset(&video.buffer.req, 0, sizeof(video.buffer.req));
	video.buffer.req.type = stream_flag;
	video.buffer.req.memory = V4L2_MEMORY_MMAP;
	video.buffer.req.count = BUFFER_NUM;
	/* 设置视频帧缓冲规格 */
	if (ioctl(video.fd, VIDIOC_REQBUFS, &video.buffer.req) == -1) {
		if (errno == EINVAL) {
			perror("video capturing or mmap-streaming is not support");
		}
		else {
			perror("VIDIOC_REQBUFS");
		}
		exit(EXIT_FAILURE);
	}
	if (video.buffer.req.count < BUFFER_NUM) {
		perror("no enough buffer");
		exit(EXIT_FAILURE);
	}
	video.buffer.buf = calloc(video.buffer.req.count, sizeof(*video.buffer.buf));
	assert(video.buffer.buf != NULL);
}

static void
buffer_mmap(int index)
{
	memset(&video.buffer.query, 0, sizeof(video.buffer.query));
	video.buffer.query.type = video.buffer.req.type;
	video.buffer.query.memory = V4L2_MEMORY_MMAP;
	video.buffer.query.index = index;
	/* 视频帧缓冲映射 */
	if (ioctl(video.fd, VIDIOC_QUERYBUF, &video.buffer.query) == -1) {
		perror("VIDIOC_QUERYBUF");
		exit(EXIT_FAILURE);
	}
	video.buffer.buf[index].length = video.buffer.query.length;
	video.buffer.buf[index].start = mmap(NULL, video.buffer.query.length, PROT_READ | PROT_WRITE, MAP_SHARED, video.fd, video.buffer.query.m.offset);
	if (video.buffer.buf[index].start == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}
}

void
video_init()
{
	video_open();
	video_set_format();
	buffer_init();
	video_streamon();
}

void
video_quit()
{
	video_streamoff();
	video_close();
	buffer_free();
}

void
buffer_enqueue(int index)
{
	memset(&video.buffer.query, 0, sizeof(video.buffer.query));
	video.buffer.query.type = video.buffer.req.type;
	video.buffer.query.memory = V4L2_MEMORY_MMAP;
	video.buffer.query.index = index;
	/* 视频帧缓冲入队 */
	if (ioctl(video.fd, VIDIOC_QBUF, &video.buffer.query) == -1) {
		perror("VIDIOC_QBUF");
		exit(EXIT_FAILURE);
	}
}

void
buffer_dequeue(int index)
{
	memset(&video.buffer.query, 0, sizeof(video.buffer.query));
	video.buffer.query.type = video.buffer.req.type;
	video.buffer.query.memory = V4L2_MEMORY_MMAP;
	video.buffer.query.index = index;
	/* 视频帧缓冲出队 */
	if (ioctl(video.fd, VIDIOC_DQBUF, &video.buffer.query) == -1) {
		perror("VIDIOC_DQBUF");
		exit(EXIT_FAILURE);
	}
}
