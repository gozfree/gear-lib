## libuvc
This is a simple libuvc library.
if you want to enable HAVE_LIBV4L2, please install libv4l2-dev

* v4l2: video for linux
* dummy: file of frame sequence, as virtual device
* dshow: windows video api

# pi
sudo modprobe bcm2835_v4l2

ffplay -pix_fmts
x86_usbcam:
ffplay -f rawvideo -pixel_format yuyv422 -video_size 640x480 uvc.yuv
raspberrypi:
ffplay -f rawvideo -pixel_format yuv420p -video_size 640x480 uvc.yuv
