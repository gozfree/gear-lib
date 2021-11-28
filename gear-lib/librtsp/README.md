## librtsp

This is a simple librtsp library.

```
if you want to test liveview via usbcam, please install libx264-dev
make ENABLE_LIVEVIEW=1
./test_librtsp
ffplay rtsp://localhost:8554/uvc
```

one file/stream: one media_source  
one connect : one connect_session  

```
file \ read                  write
     |=======> media_source =======> transport_session ||
live /                                                 || <= rtsp client request
                                                       ||
```
