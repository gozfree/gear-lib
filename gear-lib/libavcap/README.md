## libavcap
This is a simple audio video capture library. 
It's aim is to provide a unified API for Linux, Windows and Mac OS X to capture audio and video from appropriate hardware. 
The supported hardware contain Webcam, RaspberryPi, Laptop Camera which based onUVC. 

### Video
* uvc: USB Video Class
* v4l2: video for linux
* dshow: windows video api
* dummy: file of frame sequence, as virtual device

### Audio
* pulseaudio: capture audio sample via PulseAudio API

#### Test
ffplay -f rawvideo -pixel_format yuyv422 -video_size 640x480 v4l2.yuv

