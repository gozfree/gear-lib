## libuvc
This is a simple libuvc library.
if you want to enable HAVE_LIBV4L2, please install libv4l2-dev

# pi
sudo modprobe bcm2835_v4l2

ffplay -pix_fmts
ffplay -f rawvideo -pixel_format yuyv422 -video_size 640x480 uvc.yuv
