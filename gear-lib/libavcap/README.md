## libavcap
This is a simple audio video capture library. 
It's aim is to provide a unified API for Linux, Windows and Mac OS X to capture audio and video from appropriate hardware. 
The supported hardware contain Webcam, RaspberryPi, Laptop Camera which based onUVC. 
refer to https://sourceforge.net/projects/libavcap/

### Video
* uvc: USB Video Class (depend on libuvc https://github.com/ktossell/libuvc)
* v4l2: video for linux
* dshow: windows video api
* dummy: file of frame sequence, as virtual device
* esp32-camera: ESP32 camera (https://github.com/espressif/esp32-camera.git)
* xcb: linux desktop screen capture, XCB(X protocol C-language Binding)

### Audio
* pulseaudio: capture audio sample via PulseAudio API

#### Test
* ffplay -f rawvideo -pixel_format yuyv422 -video_size 640x480 v4l2.yuv
* ffplay -f rawvideo -pixel_format yuv420p -video_size 640x480 v4l2.yuv (raspberry pi)
* ffplay -ar 48000 -channels 2 -f s16le -i pulseaudio.pcm
* ffplay -f x11grab -i :0.0
* ffplay -pix_fmts
* ffmpeg -f x11grab -framerate 25 -video_size 1366*768 -i :0.0 out.mp4

# raspberry pi
`$ sudo modprobe bcm2835_v4l2 `

```
x86 $ ./test_libavcap
[V4L2 Input information]:
	input.name: 	"Camera 1"
[V4L2 Capability Information]:
	cap.driver: 	"uvcvideo"
	cap.card: 	"Lenovo EasyCamera: Lenovo EasyC"
	cap.bus_info: 	"usb-0000:00:1d.0-1.6"
	cap.version: 	"266002"
	cap.capabilities:
			 VIDEO_CAPTURE
			 STREAMING
[V4L2 Support Format]:
	0. [YUYV] "YUYV 4:2:2"
	1. [MJPG] "Motion-JPEG"
[V4L2 Supported Control]:
	Brightness, range: [-64, 64], default: 0, current: 0
	Contrast, range: [0, 64], default: 32, current: 32
	Saturation, range: [0, 128], default: 64, current: 64
	Hue, range: [-180, 180], default: 0, current: 0
	White Balance Temperature, Auto, range: [0, 1], default: 1, current: 1
	Gamma, range: [100, 500], default: 300, current: 300
	Power Line Frequency, range: [0, 2], default: 1, current: 1
	White Balance Temperature, range: [2800, 6500], default: 4650, current: 4650
	Sharpness, range: [0, 8], default: 6, current: 6
	Backlight Compensation, range: [0, 2], default: 0, current: 0
```


```
raspberrypi0w $ ./test_libavcap
[V4L2 Input information]:
        input.name:     "Camera 0"
[V4L2 Capability Information]:
        cap.driver:     "bm2835 mmal"
        cap.card:       "mmal service 16.1"
        cap.bus_info:   "platform:bcm2835-v4l2"
        cap.version:    "265799"
        cap.capabilities:
                         VIDEO_CAPTURE
                         VIDEO_OVERLAY
                         READWRITE
                         STREAMING
[V4L2 Support Format]:
        0. [YU12] "Planar YUV 4:2:0"
        1. [YUYV] "YUYV 4:2:2"
        2. [RGB3] "24-bit RGB 8-8-8"
        3. [JPEG] "JFIF JPEG"
        4. [H264] "H.264"
        5. [MJPG] "Motion-JPEG"
        6. [YVYU] "YVYU 4:2:2"
        7. [VYUY] "VYUY 4:2:2"
        8. [UYVY] "UYVY 4:2:2"
        9. [NV12] "Y/CbCr 4:2:0"
        10. [BGR3] "24-bit BGR 8-8-8"
        11. [YV12] "Planar YVU 4:2:0"
        12. [NV21] "Y/CrCb 4:2:0"
        13. [BGR4] "32-bit BGRA/X 8-8-8-8"
[V4L2 Supported Control]:
        Brightness, range: [0, 100], default: 50, current: 50
        Contrast, range: [-100, 100], default: 0, current: 0
        Saturation, range: [-100, 100], default: 0, current: 0
        Power Line Frequency, range: [0, 3], default: 1, current: 1
        Sharpness, range: [-100, 100], default: 0, current: 0
```


```
raspberrypi2b $ ./test_libavcap
[V4L2 Input information]:
        input.name:     "Camera 0"
[V4L2 Capability Information]:
        cap.driver:     "unicam"
        cap.card:       "unicam"
        cap.bus_info:   "platform:3f801000.csi"
        cap.version:    "330303"
        cap.capabilities:
                         VIDEO_CAPTURE
                         READWRITE
                         STREAMING
[V4L2 Support Format]:
        0. [BA81] "8-bit Bayer BGBG/GRGR"
        1. [pBAA] "10-bit Bayer BGBG/GRGR Packed"
        2. [BG10] "10-bit Bayer BGBG/GRGR"
[V4L2 Supported Control]:
        White Balance, Automatic, range: [0, 1], default: 0, current: 0
```
