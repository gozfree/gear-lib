## libavcap
This is a simple audio video capture library. 
It's aim is to provide a unified API for Linux, Windows and Mac OS X to capture audio and video from appropriate hardware. 
The supported hardware contain Webcam, RaspberryPi, Laptop Camera which based onUVC. 

### Video
* uvc: USB Video Class
* v4l2: video for linux
* dshow: windows video api
* dummy: file of frame sequence, as virtual device
* xcb: linux desktop screen capture, XCB(X protocol C-language Binding)

### Audio
* pulseaudio: capture audio sample via PulseAudio API

#### Test
* ffplay -f rawvideo -pixel_format yuyv422 -video_size 640x480 v4l2.yuv
* ffplay -ar 48000 -channels 2 -f s16le -i pulseaudio.pcm
* ffplay -f x11grab -i :0.0
* ffmpeg -f x11grab -framerate 25 -video_size 1366*768 -i :0.0 out.mp4
